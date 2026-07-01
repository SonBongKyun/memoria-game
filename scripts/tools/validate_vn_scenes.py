#!/usr/bin/env python3
"""Lightweight validator for MEMORIA's JSON-driven VN scenes."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
VN_DIR = ROOT / "data" / "vn_scenes"
MEMORY_SOURCE = ROOT / "scripts" / "systems" / "memory_manager.gd"
PORTRAIT_SOURCE = ROOT / "scripts" / "ui" / "dialogue_box.gd"
VN_UI_SOURCE = ROOT / "scripts" / "ui" / "vn_scene.gd"


def resource_path_exists(value: str) -> bool:
    return value.startswith("res://") and (ROOT / value.removeprefix("res://")).exists()


def known_cg_aliases() -> dict[str, str]:
    source = VN_UI_SOURCE.read_text(encoding="utf-8")
    start = source.find("const CG_ALIAS_FALLBACKS")
    end = source.find("\n}", start)
    block = source[start:end] if start >= 0 and end > start else ""
    return dict(re.findall(r'^\s*"([^"]+)"\s*:\s*"([^"]+)"', block, flags=re.MULTILINE))


def known_memory_ids() -> set[str]:
    source = MEMORY_SOURCE.read_text(encoding="utf-8")
    return set(re.findall(r'Memory\.new\(\s*"([^"]+)"', source))


def known_portrait_ids() -> set[str]:
    source = PORTRAIT_SOURCE.read_text(encoding="utf-8")
    start = source.find("const PORTRAIT_MAP")
    end = source.find("\n}", start)
    block = source[start:end] if start >= 0 and end > start else ""
    return set(re.findall(r'^\s*"([^"]+)"\s*:', block, flags=re.MULTILINE))


def main() -> int:
    errors: list[str] = []
    warnings: list[str] = []
    documents: dict[str, dict[str, Any]] = {}

    for path in sorted(VN_DIR.glob("*.json")):
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            errors.append(f"{path.name}: invalid JSON ({exc})")
            continue
        if not isinstance(data, dict):
            errors.append(f"{path.name}: root must be an object")
            continue
        if not isinstance(data.get("steps"), list):
            errors.append(f"{path.name}: missing or invalid 'steps' array")
            continue
        documents[path.stem] = data

    memory_ids = known_memory_ids()
    portrait_ids = known_portrait_ids()
    cg_aliases = known_cg_aliases()

    def cg_exists(value: Any) -> bool:
        if not isinstance(value, str) or not value:
            return False
        if value.startswith("res://"):
            return resource_path_exists(value)
        if value in cg_aliases:
            return resource_path_exists(cg_aliases[value])
        for candidate in (
            ROOT / "assets" / "cg" / "game_image" / f"{value}.png",
            ROOT / "assets" / "cg" / "game_image" / f"{value}.jpg",
            ROOT / "assets" / "cg" / f"{value}.png",
            ROOT / "assets" / "cg" / f"{value}.jpg",
        ):
            if candidate.exists():
                return True
        return False

    def validate_common_fields(file_name: str, label: str, item: dict[str, Any]) -> None:
        for flag_key in ("requires_flag", "requires_not_flag"):
            if flag_key in item and (not isinstance(item[flag_key], str) or not item[flag_key].strip()):
                errors.append(f"{file_name} {label}: {flag_key} must be a non-empty string")
        for memory_key in ("cost_memory", "burn_memory"):
            if memory_key in item:
                memory_id = item[memory_key]
                if not isinstance(memory_id, str) or not memory_id:
                    errors.append(f"{file_name} {label}: {memory_key} must be a memory id string")
                elif memory_id not in memory_ids:
                    errors.append(f"{file_name} {label}: unknown memory id '{memory_id}'")
        if "cg" in item:
            cg = item["cg"]
            if not cg_exists(cg):
                errors.append(f"{file_name} {label}: missing CG resource '{cg}'")
        if "portrait" in item:
            portrait = item["portrait"]
            if isinstance(portrait, str) and portrait.startswith("res://"):
                if not resource_path_exists(portrait):
                    errors.append(f"{file_name} {label}: missing portrait resource '{portrait}'")
            elif not isinstance(portrait, str) or portrait not in portrait_ids:
                errors.append(f"{file_name} {label}: unknown portrait id '{portrait}'")

    for scene_id, data in sorted(documents.items()):
        file_name = f"{scene_id}.json"
        declared_id = data.get("id")
        if declared_id not in (None, scene_id):
            warnings.append(f"{file_name}: declared id '{declared_id}' differs from filename")
        steps = data["steps"]
        for index, step in enumerate(steps):
            label = f"step {index}"
            if not isinstance(step, dict):
                errors.append(f"{file_name} {label}: step must be an object")
                continue
            validate_common_fields(file_name, label, step)

            speaker = step.get("speaker")
            spoken_text = step.get("text")
            if speaker == "Arrel" and isinstance(spoken_text, str):
                word_count = len(re.findall(r"[A-Za-z0-9']+", spoken_text))
                if word_count > 8:
                    errors.append(f"{file_name} {label}: Arrel line exceeds 8 words ({word_count})")
            if speaker in ("Kairos", "Kairós") and isinstance(spoken_text, str):
                if re.search(r"\b[A-Za-z]+['’][A-Za-z]+\b", spoken_text):
                    errors.append(f"{file_name} {label}: Kairós line uses a contraction")
            if speaker in ("Han", "Singer") and isinstance(spoken_text, str) and spoken_text.strip():
                errors.append(f"{file_name} {label}: Han/Singer must communicate without direct speech")

            if step.get("action") == "goto_scene":
                target = step.get("id")
                if not isinstance(target, str) or target not in documents:
                    errors.append(f"{file_name} {label}: goto_scene target '{target}' does not exist")
                elif "start_index" in step:
                    start_index = step["start_index"]
                    target_count = len(documents[target]["steps"])
                    if not isinstance(start_index, int) or isinstance(start_index, bool) or not 0 <= start_index < target_count:
                        errors.append(
                            f"{file_name} {label}: start_index '{start_index}' is outside {target}.steps"
                        )

            if "choice" not in step:
                continue
            choices = step["choice"]
            if not isinstance(choices, list):
                errors.append(f"{file_name} {label}: choice must be an array")
                continue
            for choice_index, choice in enumerate(choices):
                choice_label = f"{label} choice {choice_index}"
                if not isinstance(choice, dict):
                    errors.append(f"{file_name} {choice_label}: choice must be an object")
                    continue
                validate_common_fields(file_name, choice_label, choice)
                if "goto" in choice:
                    goto = choice["goto"]
                    if not isinstance(goto, int) or isinstance(goto, bool) or not 0 <= goto < len(steps):
                        errors.append(f"{file_name} {choice_label}: goto '{goto}' is outside steps")

    for message in warnings:
        print(f"[WARN] {message}")
    for message in errors:
        print(f"[ERROR] {message}")

    print(
        f"VN validation: {len(documents)} files, "
        f"{sum(len(data['steps']) for data in documents.values())} steps, "
        f"{len(errors)} errors, {len(warnings)} warnings"
    )
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
