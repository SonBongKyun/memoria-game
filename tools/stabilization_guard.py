#!/usr/bin/env python3
"""MEMORIA stabilization guard.

`--fix` performs only narrow, asserted migrations that are intentionally safe for
existing saves. `--check` fails CI if the repository drifts back to legacy canon
labels or mismatched release metadata.
"""

from __future__ import annotations

import argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def _write(path: str, text: str) -> None:
    (ROOT / path).write_text(text, encoding="utf-8")


def replace_guarded(path: str, old: str, new: str, *, fix: bool) -> None:
    text = _read(path)
    if new in text:
        return
    if old not in text:
        raise RuntimeError(f"{path}: expected legacy or stabilized text was not found")
    if not fix:
        raise RuntimeError(f"{path}: stabilization migration has not been applied")
    if text.count(old) != 1:
        raise RuntimeError(f"{path}: expected exactly one migration target, found {text.count(old)}")
    _write(path, text.replace(old, new, 1))


def require(path: str, needle: str) -> None:
    if needle not in _read(path):
        raise RuntimeError(f"{path}: required invariant missing: {needle!r}")


def migrate(*, fix: bool) -> None:
    # Product version: align Godot metadata with the existing 0.9.0 Windows product version.
    replace_guarded(
        "project.godot",
        'config/version="0.1.0"',
        'config/version="0.9.0"',
        fix=fix,
    )

    # Player-facing memory grades follow Master Bible v9.0 while indices remain legacy 0..4.
    replace_guarded(
        "scripts/ui/memory_ui.gd",
        'const GRADE_NAMES = ["Grade 5, Sensory", "Grade 4, Daily", "Grade 3, Relational", "Grade 2, Identity", "Grade 1, Core"]',
        'const GRADE_NAMES = ["1 Ember, Sensory", "2 Flame, Daily", "3 Blaze, Relational", "4 Sun, Identity", "5 Zero, Core"]',
        fix=fix,
    )

    replace_guarded(
        "scripts/systems/memory_manager.gd",
        '# 값이 클수록 높은 등급: GRADE_5=0(최하) ~ GRADE_1=4(최상). 비교 시 >= 는 "같거나 높은 등급".',
        '# 레거시 enum 이름은 세이브 호환 때문에 유지한다. 값 0..4는 내부 rank, 플레이어 표시 캐논은 1..5다.\n# UI/직렬화 경계에서는 MemoryGradeCodec을 사용하며 raw 값을 뒤집지 않는다.',
        fix=fix,
    )

    replace_guarded(
        "scripts/systems/memory_manager.gd",
        '\t\t"burn_passives": burn_passives.duplicate(),\n\t}',
        '\t\t"burn_passives": burn_passives.duplicate(),\n\t\t"grade_schema": MemoryGradeCodec.INTERNAL_SCHEMA,\n\t}',
        fix=fix,
    )

    replace_guarded(
        "scripts/systems/memory_manager.gd",
        '\tburn_passives = data.get("burn_passives", {})\n\n\tvar burned_ids = data.get("burned", [])',
        '\tburn_passives = data.get("burn_passives", {})\n\tvar grade_schema := String(data.get("grade_schema", MemoryGradeCodec.INTERNAL_SCHEMA))\n\n\tvar burned_ids = data.get("burned", [])',
        fix=fix,
    )

    replace_guarded(
        "scripts/systems/memory_manager.gd",
        '\t\t\tint(m_data.get("grade", MemoryGrade.GRADE_5)),',
        '\t\t\tMemoryGradeCodec.normalize_saved_grade(int(m_data.get("grade", MemoryGrade.GRADE_5)), grade_schema),',
        fix=fix,
    )

    # The schema marker is additive, but bump the save version so release diagnostics can
    # distinguish saves written before and after the canonical grade boundary was made explicit.
    replace_guarded(
        "scripts/systems/save_manager.gd",
        'const SAVE_VERSION: String = "0.3.0"',
        'const SAVE_VERSION: String = "0.4.0"',
        fix=fix,
    )

    # Release metadata invariants.
    require("export_presets.cfg", 'export_path="build/MEMORIA-Demo-v0.9.0.exe"')
    require("export_presets.cfg", 'debug/export_console_wrapper=0')
    require("export_presets.cfg", 'application/file_version="0.9.0.0"')
    require("export_presets.cfg", 'application/product_version="0.9.0.0"')
    require("scripts/systems/memory_grade_codec.gd", 'const INTERNAL_SCHEMA: String = "internal_rank_0_4_v1"')


def main() -> int:
    parser = argparse.ArgumentParser()
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--check", action="store_true")
    mode.add_argument("--fix", action="store_true")
    args = parser.parse_args()

    migrate(fix=args.fix)
    print("MEMORIA stabilization guard: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
