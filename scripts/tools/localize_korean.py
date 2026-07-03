#!/usr/bin/env python3
"""Populate Korean companion fields in MEMORIA dialogue JSON.

The game keeps English source text and reads ``<field>_ko`` when Korean is
selected.  This utility translates only player-facing prose fields, preserves
direction tags/placeholders, and never touches flags, resource paths or IDs.
"""

from __future__ import annotations

import argparse
import json
import re
import time
import urllib.parse
import urllib.request
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
DATA_ROOT = ROOT / "data"
TRANSLATABLE_FIELDS = {
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
TOKEN_RE = re.compile(
    r"\[[^\]\n]+\]|\{[^}\n]+\}|%(?:\d+\$)?[sdif]|res://[^\s\]]+",
    re.IGNORECASE,
)
MARKER_RE = re.compile(r"<<<MEMORIA_(\d{4})>>>")
ENDPOINT = "https://translate.googleapis.com/translate_a/single"
GLOSSARY_REPLACEMENTS = {
    "Malet": "말렛",
    "Mallet": "말렛",
    "말레의": "말렛의",
    "말레는": "말렛은",
    "말레가": "말렛이",
    "말레를": "말렛을",
    "말레": "말렛",
    "Arrel": "아렐",
    "아르렐": "아렐",
    "에럴": "아렐",
    "Elia": "엘리아",
    "Tobias": "토비아스",
    "Kairós": "카이로스",
    "Kairos": "카이로스",
    "Han": "한",
    "한씨": "한",
    "Nera": "네라",
    "Verdan": "베르단",
    "베르당": "베르단",
    "Belt": "벨트",
    "허리띠": "벨트",
    "Bureau": "관리국",
    "Authority": "관리국",
    "사무국": "관리국",
    "당국": "관리국",
    "권위 체인": "관리국 봉쇄선",
    "권위 검문소": "관리국 검문소",
    "Seam": "심",
    "Erasers": "소거관들",
    "Executor": "집행관",
    "Handler": "관리관",
    "Echo Shell": "에코 셸",
    "에코 쉘": "에코 셸",
    "Mneme": "므네메",
    "음네메": "므네메",
    "Arkein": "아르케인",
    "Sump": "섬프",
    "BL-07": "BL-07",
}


def protect_tokens(text: str) -> tuple[str, dict[str, str]]:
    saved: dict[str, str] = {}

    def replace(match: re.Match[str]) -> str:
        key = f"MEMORIATOKEN{len(saved):03d}QXZ"
        saved[key] = match.group(0)
        return key

    return TOKEN_RE.sub(replace, text), saved


def restore_tokens(text: str, saved: dict[str, str]) -> str:
    for key, value in saved.items():
        text = text.replace(key, value)
        text = text.replace(key.lower(), value)
    return text


def polish_korean(text: str) -> str:
    for source, target in GLOSSARY_REPLACEMENTS.items():
        text = text.replace(source, target)
    return text


def translate_batch(texts: list[str], retries: int = 4) -> list[str]:
    protected: list[str] = []
    token_maps: list[dict[str, str]] = []
    for text in texts:
        clean, mapping = protect_tokens(text)
        protected.append(clean)
        token_maps.append(mapping)

    payload = "\n".join(
        f"<<<MEMORIA_{index:04d}>>>\n{text}"
        for index, text in enumerate(protected)
    )
    query = urllib.parse.urlencode(
        {"client": "gtx", "sl": "en", "tl": "ko", "dt": "t", "q": payload}
    )
    request = urllib.request.Request(
        f"{ENDPOINT}?{query}",
        headers={"User-Agent": "MEMORIA-localization-tool/1.0"},
    )

    last_error: Exception | None = None
    for attempt in range(retries):
        try:
            with urllib.request.urlopen(request, timeout=30) as response:
                data = json.loads(response.read().decode("utf-8"))
            translated = "".join(part[0] for part in data[0] if part and part[0])
            matches = list(MARKER_RE.finditer(translated))
            if len(matches) != len(texts):
                raise RuntimeError(
                    f"marker mismatch: expected {len(texts)}, received {len(matches)}"
                )
            results: list[str] = []
            for index, match in enumerate(matches):
                start = match.end()
                end = matches[index + 1].start() if index + 1 < len(matches) else len(translated)
                value = translated[start:end].strip()
                results.append(polish_korean(restore_tokens(value, token_maps[index])))
            return results
        except Exception as exc:  # network retry boundary
            last_error = exc
            time.sleep(1.5 * (attempt + 1))
    raise RuntimeError(f"translation request failed: {last_error}")


def collect_targets(node: Any, targets: list[tuple[dict[str, Any], str, str]]) -> None:
    if isinstance(node, dict):
        for key, value in list(node.items()):
            if (
                key in TRANSLATABLE_FIELDS
                and isinstance(value, str)
                and value.strip()
                and f"{key}_ko" not in node
                and not (key in {"text", "narrate"} and isinstance(node.get("ko"), str))
            ):
                targets.append((node, key, value))
            collect_targets(value, targets)
    elif isinstance(node, list):
        for value in node:
            collect_targets(value, targets)


def polish_document(node: Any) -> None:
    if isinstance(node, dict):
        for key, value in list(node.items()):
            if (key.endswith("_ko") or key == "ko") and isinstance(value, str):
                node[key] = polish_korean(value)
            else:
                polish_document(value)
    elif isinstance(node, list):
        for value in node:
            polish_document(value)


def batch_targets(targets: list[tuple[dict[str, Any], str, str]]) -> list[list[tuple[dict[str, Any], str, str]]]:
    batches: list[list[tuple[dict[str, Any], str, str]]] = []
    current: list[tuple[dict[str, Any], str, str]] = []
    current_size = 0
    for target in targets:
        text_size = len(target[2]) + 24
        if current and (len(current) >= 20 or current_size + text_size > 4200):
            batches.append(current)
            current = []
            current_size = 0
        current.append(target)
        current_size += text_size
    if current:
        batches.append(current)
    return batches


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    files = sorted(DATA_ROOT.glob("*_dialogue.json")) + sorted((DATA_ROOT / "vn_scenes").glob("*.json"))
    documents: dict[Path, Any] = {}
    targets: list[tuple[dict[str, Any], str, str]] = []
    for path in files:
        document = json.loads(path.read_text(encoding="utf-8"))
        polish_document(document)
        documents[path] = document
        collect_targets(document, targets)

    print(f"[KO] {len(files)} files, {len(targets)} missing Korean fields")
    if args.dry_run:
        return 0

    batches = batch_targets(targets)
    for index, batch in enumerate(batches, 1):
        translations = translate_batch([target[2] for target in batch])
        for (container, key, _), translated in zip(batch, translations):
            container[f"{key}_ko"] = translated
        print(f"[KO] batch {index}/{len(batches)} ({len(batch)} lines)")
        time.sleep(0.25)

    for path, document in documents.items():
        path.write_text(
            json.dumps(document, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )
    print(f"[KO] wrote {len(documents)} files")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
