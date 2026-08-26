#!/usr/bin/env python3
"""Fast repository contract checks for MEMORIA.

This script deliberately avoids importing Godot. It catches cheap, high-value
regressions before the engine boots: release version drift, save-grade enum
reordering, generated build outputs leaking into source control, and loss of
the first-hour Memory Burn consequence loop.
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def read_json(path: str) -> dict:
    return json.loads(read(path))


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


def _collect_values(value, key: str) -> list:
    found = []
    if isinstance(value, dict):
        if key in value:
            found.append(value[key])
        for child in value.values():
            found.extend(_collect_values(child, key))
    elif isinstance(value, list):
        for child in value:
            found.extend(_collect_values(child, key))
    return found


def check_first_hour_consequence_contract() -> None:
    """Protect the vertical slice's core promise: sacrifice now, absence later."""
    battle = read_json("data/vn_scenes/ch1_void_beast.json")
    aftermath = read_json("data/vn_scenes/ch1_after_forest.json")

    burn_costs = set(_collect_values(battle, "cost_memory"))
    expected_burns = {"sense_warm_light", "sense_forest_smell"}
    missing_burns = sorted(expected_burns - burn_costs)
    if missing_burns:
        fail(
            "Chapter 1 no longer offers the expected real Memory Burn choices: "
            + ", ".join(missing_burns)
        )

    callbacks = set(_collect_values(aftermath, "distort_if_burned"))
    missing_callbacks = sorted(expected_burns - callbacks)
    if missing_callbacks:
        fail(
            "Chapter 1 burns lost their visible aftermath callbacks: "
            + ", ".join(missing_callbacks)
        )

    # The aftermath used to claim no additional memory had been burned even
    # when the player paid a real memory cost in the Void Beast scene.
    stale_claim = "You fought through that without burning anything else."
    if stale_claim in read("data/vn_scenes/ch1_after_forest.json"):
        fail("Chapter 1 aftermath still contains the burn-state-agnostic stale claim")

    print("first-hour Memory Burn consequence contract OK")


def check_build_hygiene() -> None:
    ignore = read(".gitignore")
    required = ["build/", "*.exe", "*.pck"]
    missing = [rule for rule in required if rule not in ignore.splitlines()]
    if missing:
        fail(".gitignore is missing generated build-output rules: " + ", ".join(missing))

    # Do not enable broad Git LFS tracking over existing media without a real
    # migration/renormalization pass. A blanket .gitattributes addition can
    # leave previously committed binaries marked as LFS but stored as Git blobs.
    attrs = ROOT / ".gitattributes"
    if attrs.exists():
        text = attrs.read_text(encoding="utf-8")
        broad_media_rules = ["*.png filter=lfs", "*.wav filter=lfs", "*.mp4 filter=lfs"]
        if any(rule in text for rule in broad_media_rules):
            warn(
                "Broad LFS media tracking is enabled. Verify existing assets were migrated/renormalized "
                "before merging to avoid pointer/dirty-working-tree problems."
            )
    print("build-output hygiene OK")


def main() -> int:
    check_versions()
    check_memory_storage_contract()
    check_first_hour_consequence_contract()
    check_build_hygiene()
    print("MEMORIA repository contracts passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
