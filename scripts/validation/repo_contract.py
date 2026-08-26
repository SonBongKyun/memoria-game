#!/usr/bin/env python3
"""Fast repository contract checks for MEMORIA.

This script deliberately avoids importing Godot. It catches cheap, high-value
regressions before the engine boots: release version drift, save-grade enum
reordering, and missing large-asset policy.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def fail(message: str) -> None:
    print(f"::error::{message}")
    raise SystemExit(1)


def warn(message: str) -> None:
    print(f"::warning::{message}")


def project_version() -> str:
    text = read("project.godot")
    match = re.search(r'^config/version="([^"]+)"$', text, re.MULTILINE)
    if not match:
        fail("project.godot is missing config/version")
    return match.group(1)


def export_metadata() -> tuple[str, str, str]:
    text = read("export_presets.cfg")
    path_match = re.search(r'^export_path="[^"]*v([^"/]+)\.exe"$', text, re.MULTILINE)
    file_match = re.search(r'^application/file_version="([^"]+)"$', text, re.MULTILINE)
    product_match = re.search(r'^application/product_version="([^"]+)"$', text, re.MULTILINE)
    if not path_match or not file_match or not product_match:
        fail("export_presets.cfg is missing versioned Windows demo metadata")
    return path_match.group(1), file_match.group(1), product_match.group(1)


def check_versions() -> None:
    project = project_version()
    path_version, file_version, product_version = export_metadata()
    win_version = f"{project}.0"
    if path_version != project:
        fail(f"demo filename version {path_version} != project version {project}")
    if file_version != win_version:
        fail(f"Windows file version {file_version} != {win_version}")
    if product_version != win_version:
        fail(f"Windows product version {product_version} != {win_version}")
    print(f"version contract OK: {project}")


def check_memory_storage_contract() -> None:
    text = read("scripts/systems/memory_manager.gd")
    enum_match = re.search(r'enum MemoryGrade\s*\{([^}]+)\}', text)
    if not enum_match:
        fail("MemoryGrade enum not found")
    members = [part.strip() for part in enum_match.group(1).split(",") if part.strip()]
    expected = ["GRADE_5", "GRADE_4", "GRADE_3", "GRADE_2", "GRADE_1"]
    if members != expected:
        fail(
            "MemoryGrade storage enum order changed. Existing saves persist raw integer grades; "
            "add an explicit save migration before changing enum order."
        )

    # Canon v9.0 uses 1→5 as rising narrative power, while the current save
    # schema stores the older inverse display labels as rank 0→4. Keep the raw
    # order frozen until a dedicated migration is implemented.
    ui_text = read("scripts/ui/memory_ui.gd")
    if '"Grade 5, Sensory"' in ui_text and '"Grade 1, Core"' in ui_text:
        warn(
            "Legacy grade labels are still user-facing. Canon v9.0 uses 1 Ember → 5 Zero. "
            "Do not reorder the enum; migrate UI labels and saves through a compatibility layer."
        )
    print("memory save-storage contract OK")


def check_lfs_policy() -> None:
    attrs = read(".gitattributes")
    required = ["*.png filter=lfs", "*.wav filter=lfs", "*.mp4 filter=lfs"]
    missing = [rule for rule in required if rule not in attrs]
    if missing:
        fail("Git LFS policy missing required media rules: " + ", ".join(missing))
    print("large-asset policy OK")


def main() -> int:
    check_versions()
    check_memory_storage_contract()
    check_lfs_policy()
    print("MEMORIA repository contracts passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
