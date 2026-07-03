#!/usr/bin/env python3
"""Fail when any shipped dialogue field lacks a Korean companion value."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
FIELDS = {
    "title",
    "text",
    "narrate",
    "system_log",
    "burned_text",
    "distorted_text",
    "distorted_narrate",
    "choice_title",
    "choice_hint",
    "effect",
}


def main() -> int:
    files = sorted((ROOT / "data").glob("*_dialogue.json")) + sorted(
        (ROOT / "data" / "vn_scenes").glob("*.json")
    )
    errors: list[str] = []
    total = 0
    speakers: set[str] = set()

    def walk(path: Path, node: Any, breadcrumb: str = "root") -> None:
        nonlocal total
        if isinstance(node, dict):
            speaker = node.get("speaker")
            if isinstance(speaker, str) and speaker:
                speakers.add(speaker)
            for key, value in node.items():
                if key in FIELDS and isinstance(value, str) and value.strip():
                    total += 1
                    translated = node.get(f"{key}_ko")
                    legacy = node.get("ko") if key in {"text", "narrate"} else None
                    if not (isinstance(translated, str) and translated.strip()) and not (
                        isinstance(legacy, str) and legacy.strip()
                    ):
                        errors.append(f"{path.name}:{breadcrumb}.{key} has no Korean text")
                walk(path, value, f"{breadcrumb}.{key}")
        elif isinstance(node, list):
            for index, value in enumerate(node):
                walk(path, value, f"{breadcrumb}[{index}]")

    for path in files:
        walk(path, json.loads(path.read_text(encoding="utf-8")))

    manager = (ROOT / "scripts" / "core" / "game_manager.gd").read_text(encoding="utf-8")
    speaker_block = manager[manager.find("const SPEAKER_NAMES_KO"):manager.find("const ENEMY_NAMES_KO")]
    known_speakers = set(re.findall(r'^\s*"([^"]+)"\s*:', speaker_block, flags=re.MULTILINE))
    for speaker in sorted(speakers - known_speakers):
        errors.append(f"speaker '{speaker}' has no Korean display name")

    for message in errors:
        print(f"[ERROR] {message}")
    print(
        f"Korean localization: {len(files)} files, {total} fields, "
        f"{len(speakers)} speakers, {len(errors)} errors"
    )
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
