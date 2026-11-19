#!/usr/bin/env python3
"""Verify and summarize owned S362 cooked-smoke evidence. Python3.10+, stdlib only.

Usage: python analyze_s362_packaged_performance.py EVIDENCE --output summary.json
EVIDENCE is a run directory containing RuntimeArtifacts/status.json, that artifact
directory itself, or status.json. CSV is read only beside that preserved JSON;
recorded original paths are checked, never followed to unrelated filesystem data.
"""
from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from datetime import datetime
import hashlib
import io
import json
import math
import ntpath
from pathlib import Path
import posixpath
import re
import statistics
import sys
from typing import Any, Iterable

MAX_CSV_BYTES = 64 * 1024 * 1024
MAX_STATUS_BYTES = 8 * 1024 * 1024
REGIONS = {1: "verdan_market", 2: "belt_waystation", 3: "drift_shelter"}
TAG_NAMES = ("Region", "Phase", "CapturePending", "ActualCombat")
TAGS = {name: "MemoriaS362Smoke/" + name for name in TAG_NAMES}
FOOTER = re.compile(r"(?m)^\[HasHeaderRowAtEnd\],")
META_KEY = re.compile(r"(?:^|,)\[([^\]\r\n]{1,100})\],")


class EvidenceError(ValueError):
    """Invalid or incomplete evidence, rather than a performance failure."""


@dataclass(frozen=True)
class Metric:
    unit: str
    scope: str
    require_positive: bool = False
    positive_only: bool = False


METRICS = {
    "FrameTime": Metric("ms", "Engine whole-frame interval; includes profiling, capture, UI and frame limiting."),
    "GameThreadTime": Metric("ms", "Published engine game-thread processing time.", True),
    "RenderThreadTime": Metric("ms", "Published engine render-thread processing time.", True),
    "RHIThreadTime": Metric("ms", "Published engine RHI-thread processing time when available.", True),
    "GPUTime": Metric("ms", "Engine RHIGetGPUFrameCycles timing; positive samples only, never CPU/wall substitution.", True, True),
    "RHI/DrawCalls": Metric("draw_calls", "Engine whole-frame RHI draw counter, including other passes/UI."),
    "RHI/Primitives": Metric("primitives", "Engine RHI primitive count, not unique visible scene triangles."),
    "RHI/PrimitivesDrawn": Metric("primitives", "Engine RHI primitive count, not unique visible scene triangles."),
    "RHI/Prims": Metric("primitives", "Engine RHI primitive count if exposed under this engine column name."),
    "CPUUsage_Process": Metric("percent", "Engine per-process CPU usage counter; distinct from thread milliseconds."),
    "GPUMem/LocalUsedMB": Metric("MiB", "Engine process LOCAL graphics-memory usage; engine MB label uses bytes/1024^2.", True, True),
    "GPUMem/LocalBudgetMB": Metric("MiB", "Engine process LOCAL graphics-memory budget; not physical GPU capacity.", True, True),
    "GPUMem/SystemUsedMB": Metric("MiB", "Engine process SYSTEM graphics-memory usage when reported.", True, True),
    "GPUMem/SystemBudgetMB": Metric("MiB", "Engine process SYSTEM graphics-memory budget when reported.", True, True),
    "AnimationParallelEvaluation/TotalTaskTime": Metric("ms", "Sum of animation task durations; parallel tasks can overlap wall/game-thread time.", True),
    "AnimationParallelEvaluation/AverageTaskTime": Metric("ms", "Engine frame's average animation task duration.", True),
    "AnimationParallelEvaluation/MinTaskTime": Metric("ms", "Engine frame's minimum animation task duration.", True),
    "AnimationParallelEvaluation/MaxTaskTime": Metric("ms", "Engine frame's maximum animation task duration.", True),
    "AnimationParallelEvaluation/NumberOfTasks": Metric("tasks", "Number of evaluated animation tasks reported in a frame."),
    "Exclusive/GameThread/Animation": Metric("ms", "Engine exclusive game-thread Animation scope; distinct from parallel task sum.", True),
}
for _name in ("NativeUpdate", "BlueprintUpdate", "InitAnim", "Cloth", "WorkerThreadTickTime"):
    METRICS["Animation/" + _name] = Metric("ms", "Installed engine Animation scoped-timing column; scopes are not additive.", True)

LIMITS = [
    "Bounded seeded cooked Development smoke, not a steady-state or Shipping benchmark, human playthrough or before/after optimization comparison.",
    "No FPS/performance pass thresholds are imposed. Evidence integrity and measurement availability are separate from performance quality.",
    "No warmup was added. Captures, UI, streaming, seeded encounters, file work and profiling overhead can affect observations.",
    "Region/phase/capture/combat tags come from the game thread. Published render/RHI/GPU counters may lag those tags; no exact cross-thread alignment is claimed.",
    "Phase17 is artifact finalization and is excluded from gameplay summaries. Capture/combat groups are separated, not silently pooled into an idle rendering benchmark.",
    "Nearest-rank p50/p95/p99 are computed from actual valid raw samples. Small sample counts remain visible; no percentile is invented from manual min/mean/max aggregates.",
    "GPUTime requires positive raw engine samples. Missing/nonpositive GPU timing is unavailable; CPU/wall timings are never substituted.",
    "Manual scoped CPU, engine thread timing and parallel-task sums measure different scopes and may overlap. Manual scopes exclude separate engine bone refresh and GPU skinning.",
    "RHI texture allocation accounting, one-shot DXGI process LOCAL usage/budget and engine GPUMem counters have different scopes. No texture dimensions are converted to invented VRAM bytes.",
    "The status exit_code is the harness's recorded intent. External process exit/log/fatal validation remains a separate runner responsibility.",
]


def number(value: Any) -> float | None:
    if isinstance(value, bool):
        return None
    try:
        out = float(value)
    except (TypeError, ValueError, OverflowError):
        return None
    return out if math.isfinite(out) else None


def integer(value: Any) -> int | None:
    out = number(value)
    return int(out) if out is not None and out.is_integer() else None


def distribution(values: Iterable[Any], spec: Metric) -> dict[str, Any]:
    observed = list(values)
    valid = [v for value in observed if (v := number(value)) is not None and v >= 0]
    positive = [v for v in valid if v > 0]
    out: dict[str, Any] = {
        "status": "unavailable", "unit": spec.unit, "scope": spec.scope,
        "observations": len(observed), "valid_nonnegative_samples": len(valid),
        "positive_samples": len(positive), "zero_samples": sum(v == 0 for v in valid),
        "missing_or_invalid_samples": len(observed) - len(valid),
        "sample_policy": "positive_only" if spec.positive_only else "all_valid_nonnegative",
    }
    if not valid or (spec.require_positive and not positive):
        out["reason"] = "No valid positive observations; support/activity cannot be established." if spec.require_positive else "No valid observations."
        return out
    measured = sorted(positive if spec.positive_only else valid)
    rank = lambda p: measured[max(0, math.ceil(p * len(measured)) - 1)]
    out.update(status="measured", samples=len(measured), minimum=measured[0],
               mean=statistics.fmean(measured), p50=rank(.50), p95=rank(.95),
               p99=rank(.99), maximum=measured[-1], percentile_method="nearest_rank")
    return out


def recorded_path(value: Any) -> tuple[str, str]:
    if not isinstance(value, str) or not value:
        raise EvidenceError("Recorded capture path is missing.")
    if re.match(r"^[A-Za-z]:[/\\]", value):
        return "windows", ntpath.normcase(ntpath.normpath(value)).replace("\\", "/")
    if value.startswith("/") and not value.startswith("//"):
        return "posix", posixpath.normpath(value)
    raise EvidenceError("Recorded capture path is not an absolute local path.")


def timestamp(value: Any) -> datetime:
    if not isinstance(value, str):
        raise EvidenceError("Capture timestamp is missing.")
    try:
        result = datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError as error:
        raise EvidenceError("Capture timestamp is invalid.") from error
    if result.tzinfo is None:
        raise EvidenceError("Capture timestamp has no timezone.")
    return result


def metadata(footer: str) -> dict[str, str]:
    # UE writes metadata raw, including nested quotes/commas in final Commandline.
    # Scan [key], markers and stop before Commandline instead of RFC-parsing it.
    matches = list(META_KEY.finditer(footer))
    out = {}
    for i, match in enumerate(matches):
        key = match.group(1).strip().lower()
        if key == "commandline":
            break
        end = matches[i + 1].start() if i + 1 < len(matches) else len(footer)
        out[key] = footer[match.end():end].rstrip(",\r\n").strip().strip('"')
    return out


def parse_csv(data: bytes) -> dict[str, Any]:
    if len(data) > MAX_CSV_BYTES:
        raise EvidenceError("CSV exceeds the bounded64MiB evidence limit.")
    try:
        text = data.decode("utf-8-sig")
    except UnicodeError as error:
        raise EvidenceError("Owned CSV is not UTF-8 text; compressed/other input is unsupported.") from error
    footers = list(FOOTER.finditer(text))
    if not footers:
        raise EvidenceError("CSV lacks its completed HasHeaderRowAtEnd footer.")
    footer_at = footers[-1].start()
    meta = metadata(text[footer_at:])
    if meta.get("hasheaderrowatend") != "1":
        raise EvidenceError("CSV final-header metadata is not1.")
    csv.field_size_limit(1024 * 1024)
    try:
        rows = list(csv.reader(io.StringIO(text[:footer_at], newline=""), strict=False))
    except csv.Error as error:
        raise EvidenceError("CSV data/header could not be parsed: " + str(error)) from error
    headers = [(i, row) for i, row in enumerate(rows) if row and row[0].strip() == "EVENTS" and "FrameTime" in row]
    if len(headers) < 2:
        raise EvidenceError("CSV lacks initial and final EVENTS headers.")
    last_index, header = headers[-1]
    if any(any(cell.strip() for cell in row) for row in rows[last_index + 1:]):
        raise EvidenceError("Unexpected data after the final EVENTS header.")
    positions: dict[str, list[int]] = {}
    for i, name in enumerate(header):
        positions.setdefault(name, []).append(i)
    for _, earlier in headers[:-1]:
        if header[:len(earlier)] != earlier:
            raise EvidenceError("Earlier CSV header is not a prefix of the final schema.")
    header_rows = {i for i, _ in headers}
    data_rows = [row for i, row in enumerate(rows[:last_index]) if i not in header_rows and any(cell.strip() for cell in row)]
    if any(len(row) > len(header) for row in data_rows):
        raise EvidenceError("CSV data row is wider than its final schema.")
    widths = sorted({len(row) for row in data_rows})
    return {"header": header, "positions": positions, "rows": data_rows, "metadata": meta,
            "format": {"initial_header_columns": len(headers[0][1]), "final_header_columns": len(header),
                       "data_rows": len(data_rows), "data_row_widths": widths,
                       "duplicate_column_positions": {name: at for name, at in positions.items() if len(at) > 1},
                       "short_rows_padded_as": "missing values, never synthetic zero", "final_footer_verified": True}}


def editor_csv(meta: dict[str, str]) -> bool:
    return any("editor" in meta.get(key, "").lower() for key in ("deviceprofile", "targettype", "executable", "exe")) or meta.get("is_editor", "").lower() in ("1", "true")


def validate_manifest(report: dict[str, Any], data: bytes) -> dict[str, Any]:
    if report.get("status") != "PASS" or integer(report.get("exit_code")) != 0:
        raise EvidenceError("Runtime status is not a completed PASS with recorded exit_code0.")
    if report.get("mode") != "cooked_development_render_smoke" or report.get("is_editor") is not False or report.get("cooked_data_required") is not True:
        raise EvidenceError("Status does not establish a cooked non-editor smoke process.")
    if report.get("isolated_smoke_until_process_exit") is not True or report.get("engine_automation_testing") is not False:
        raise EvidenceError("Status does not establish the current own-mode isolation with a quiet engine test worker.")
    if integer(report.get("phase")) != 17:
        raise EvidenceError("Runtime did not reach the final artifact-completion phase17.")
    owned = report.get("engine_csv")
    if not isinstance(owned, dict) or owned.get("status") != "complete" or owned.get("completion_future_ready") is not True or owned.get("final_header_metadata_present") is not True:
        raise EvidenceError("Owned CSV capture is incomplete or its completion future/header was not verified.")
    expected, actual = recorded_path(owned.get("expected_path")), recorded_path(owned.get("actual_path"))
    if expected != actual or actual[1].rsplit("/", 1)[-1].lower() != "enginecookedsmoke.csv":
        raise EvidenceError("Recorded expected/actual owned CSV paths do not match exactly.")
    user = recorded_path(report.get("user_dir"))
    if user[0] != actual[0] or not actual[1].startswith(user[1].rstrip("/") + "/saved/validation/upgradesmoke/"):
        raise EvidenceError("Recorded owned CSV is outside the verified isolated UserDir evidence subtree.")
    if "/saved/validation/upgradesmokeruntime/" not in user[1].lower():
        raise EvidenceError("Recorded UserDir lacks the isolated smoke-runtime marker.")
    parent = actual[1].rsplit("/", 2)[-2]
    if not re.fullmatch(r"upgrade-smoke-[a-f0-9]{32}", parent, re.I):
        raise EvidenceError("Recorded CSV folder is not a fresh smoke GUID leaf.")
    if integer(owned.get("bytes")) != len(data) or not 128 < len(data) <= MAX_CSV_BYTES:
        raise EvidenceError("Preserved CSV byte count does not match the completed manifest or size bound.")
    digest = owned.get("sha1")
    if not isinstance(digest, str) or not re.fullmatch(r"[a-fA-F0-9]{40}", digest) or hashlib.sha1(data).hexdigest() != digest.lower():
        raise EvidenceError("Preserved CSV SHA1 does not match the completed manifest.")
    if timestamp(owned.get("completed_utc")) < timestamp(report.get("started_utc")):
        raise EvidenceError("CSV completion predates the run start.")
    times = [number(owned.get(k)) for k in ("capture_started_elapsed_seconds", "stop_requested_elapsed_seconds", "completion_elapsed_seconds")]
    if any(v is None or v < 0 for v in times) or times != sorted(times):
        raise EvidenceError("Capture start/stop/completion elapsed timestamps are missing or nonmonotonic.")
    if (elapsed := number(report.get("elapsed_seconds"))) is None or elapsed < times[-1]:
        raise EvidenceError("Final runtime elapsed time predates CSV completion.")
    return {"status": "verified", "recorded_expected_path": owned["expected_path"], "recorded_actual_path": owned["actual_path"],
            "bytes": len(data), "sha1": digest.lower(), "completion_future_ready": True,
            "path_policy": "Preserved adjacent CSV is hash-verified against original owned paths; recorded paths were not followed.",
            "process_exit_verified": False, "process_exit_limit": "Separate runner exit/log review is required; JSON exit_code is harness intent."}


def summarize_group(records: list[dict[str, Any]], metrics: list[str]) -> dict[str, Any]:
    counters = {name: distribution((row["values"].get(name) for row in records), METRICS[name]) for name in metrics}
    frame = counters["FrameTime"]
    result: dict[str, Any] = {"frames": len(records), "engine_csv_counters": counters}
    if frame.get("status") == "measured" and frame["mean"] > 0:
        result["observed_interval_fps"] = 1000. / frame["mean"]
        result["fps_scope"] = "Reciprocal of mean observed engine frame interval; may be capped or affected by capture/profiling."
    return result


def retained_manual(report: dict[str, Any], verified: bool) -> dict[str, Any]:
    perf = report.get("performance", {})
    if not isinstance(perf, dict):
        perf = {}
    return {"status": "retained_recorded_summary" if verified else "unverified_input_summary",
            "limits": "Manual min/mean/max, counts, texture accounting and DXGI snapshots are retained verbatim; raw per-frame manual samples do not exist, so no manual p50/p95/p99 are invented.",
            "environment": report.get("performance_environment", {}), "cpu_animation_ms": perf.get("cpu_animation_ms", {"status": "unavailable"}),
            "inline_gpu_frame_ms": perf.get("gpu_frame_ms", {"status": "unavailable"}),
            "regions": perf.get("regions", []), "phase_windows": perf.get("phase_windows", []),
            "sources_and_limits": {key: value for key, value in perf.items() if isinstance(value, (str, bool))},
            "asset_inspections": report.get("regions", [])}


def analyze_payload(report: dict[str, Any], data: bytes) -> dict[str, Any]:
    result: dict[str, Any] = {"schema_version": 1, "status": "unavailable", "classification": "unverified_or_incomplete",
                              "limits": LIMITS, "manual_summary": retained_manual(report, False)}
    try:
        result["evidence_validation"] = validate_manifest(report, data)
        table = parse_csv(data)
        meta = table["metadata"]
        if editor_csv(meta):
            raise EvidenceError("CSV metadata identifies an editor fixture; it cannot be relabeled as cooked game data.")
        if meta.get("config") and meta["config"].lower() != "development":
            raise EvidenceError("CSV configuration does not match the cooked Development smoke.")
        header = table["header"]
        positions = table["positions"]
        if any(len(positions.get(name, [])) != 1 for name in TAGS.values()):
            raise EvidenceError("CSV lacks unique actual MemoriaS362Smoke region/phase/capture/combat tag columns.")
        # Engine CSV can legitimately contain unrelated duplicated printed names
        # with different values. Never silently choose/sum a duplicated metric.
        indices = {name: at[0] for name, at in positions.items() if len(at) == 1}
        present_metrics = [name for name in METRICS if name in indices]
        metrics = list(METRICS)
        if "FrameTime" not in present_metrics:
            raise EvidenceError("CSV lacks engine FrameTime.")
        exclusions = {"missing_or_invalid_frame_time": 0, "missing_or_invalid_tags": 0, "unmapped_region": 0, "phase17_artifact_finalization": 0}
        records = []
        for row in table["rows"]:
            get = lambda name: row[indices[name]] if indices[name] < len(row) else None
            frame_ms = number(get("FrameTime"))
            if frame_ms is None or frame_ms <= 0:
                exclusions["missing_or_invalid_frame_time"] += 1
                continue
            tags = {name: integer(get(column)) for name, column in TAGS.items()}
            if tags["Phase"] not in range(18) or tags["CapturePending"] not in (0, 1) or tags["ActualCombat"] not in (0, 1) or tags["Region"] is None:
                exclusions["missing_or_invalid_tags"] += 1
                continue
            if tags["Region"] not in REGIONS:
                exclusions["unmapped_region"] += 1
                continue
            if tags["Phase"] == 17:
                exclusions["phase17_artifact_finalization"] += 1
                continue
            records.append({"region": tags["Region"], "phase": tags["Phase"], "capture": tags["CapturePending"], "combat": tags["ActualCombat"],
                            "values": {name: get(name) for name in present_metrics}})
        if not records:
            raise EvidenceError("CSV has no actual tagged region frames outside artifact finalization.")
        result.update(status="analyzed", classification="verified_cooked_development_smoke",
                      evidence_validation={**result["evidence_validation"], "csv_final_footer_verified": True, "smoke_tag_columns_verified": True},
                      csv_format=table["format"], csv_metadata={key: meta[key] for key in ("hasheaderrowatend", "deviceprofile", "config", "engineversion") if key in meta},
                      excluded_rows=exclusions, included_gameplay_frames=len(records), percentile_method="nearest_rank: sorted[ceil(p*n)-1]",
                      missing_known_counter_columns=[name for name in METRICS if name not in positions],
                      ambiguous_known_counter_columns=[name for name in METRICS if len(positions.get(name, [])) > 1],
                      manual_summary=retained_manual(report, True))
        if "GPUTime" in result["ambiguous_known_counter_columns"]:
            result["gpu_timing_limit"] = "Duplicate GPUTime columns are ambiguous; neither column is silently selected."
        result["engine_gpu_time"] = distribution((row["values"].get("GPUTime") for row in records), METRICS["GPUTime"])
        result["regions"] = {}
        for region, name in REGIONS.items():
            selected = [row for row in records if row["region"] == region]
            groups = {}
            for capture in (0, 1):
                for combat in (0, 1):
                    key = ("capture_pending" if capture else "capture_free") + "_" + ("actual_combat" if combat else "noncombat")
                    groups[key] = summarize_group([row for row in selected if row["capture"] == capture and row["combat"] == combat], metrics)
            result["regions"][name] = {"status": "measured" if selected else "unavailable", "all_runtime": summarize_group(selected, metrics),
                                       "capture_combat_groups": groups,
                                       "phase_windows": {str(phase): summarize_group([row for row in selected if row["phase"] == phase], metrics)
                                                         for phase in sorted({row["phase"] for row in selected})}}
        result["observed_region_count"] = sum(entry["status"] == "measured" for entry in result["regions"].values())
        result["region_coverage"] = "complete_three_regions" if result["observed_region_count"] == 3 else "partial_region_coverage"
    except EvidenceError as error:
        result["evidence_validation"] = {"status": "rejected", "reason": str(error)}
    return result


def locate(evidence: Path) -> tuple[Path, Path]:
    if evidence.is_file():
        status = evidence
    elif (evidence / "RuntimeArtifacts/status.json").is_file():
        status = evidence / "RuntimeArtifacts/status.json"
    else:
        status = evidence / "status.json"
    return status, status.parent / "EngineCookedSmoke.csv"


def analyze_evidence(evidence: Path) -> dict[str, Any]:
    status, artifact = locate(evidence.resolve())
    report: Any = None
    try:
        if status.stat().st_size > MAX_STATUS_BYTES:
            raise EvidenceError("Status JSON exceeds the bounded8MiB size.")
        def reject_constant(value: str) -> None:
            raise EvidenceError("Status JSON contains a nonfinite constant: " + value)
        report = json.loads(status.read_text(encoding="utf-8-sig"), parse_constant=reject_constant)
        if not isinstance(report, dict):
            raise EvidenceError("Status JSON is not an object.")
        if artifact.stat().st_size > MAX_CSV_BYTES:
            raise EvidenceError("CSV exceeds the bounded64MiB size.")
        result = analyze_payload(report, artifact.read_bytes())
    except (OSError, ValueError, EvidenceError) as error:
        result = {"schema_version": 1, "status": "unavailable", "classification": "unverified_or_incomplete",
                  "evidence_validation": {"status": "rejected", "reason": str(error)}, "limits": LIMITS}
        if isinstance(report, dict):
            result["manual_summary"] = retained_manual(report, False)
    result["preserved_artifacts"] = {"status_json": str(status), "engine_csv": str(artifact)}
    return result


def self_check(real_csv: Path | None = None) -> dict[str, Any]:
    header = ["EVENTS", "FrameTime", "GPUTime", "GameThreadTime", *TAGS.values(), "GPUMem/LocalUsedMB"]
    rows = [header[:4], ["", 10, 0, 4], ["event, with comma", 10, 2, 4, 1, 1, 0, 0, 100],
            ["", 20, 0, 6, 1, 2, 1, 1, 0], ["", 12, 3, 5, 2, 10, 0, 0, 110],
            ["", 14, 4, 6, 3, 12, 1, 0, 120], ["", 16, 5, 7, 3, 13, 0, 0, 130],
            ["", 100, 100, 10, 3, 17, 0, 0, 999], ["", 10, 2, 3, 0, 0, 0, 0, 0],
            ["", "nan", 2, 3, 3, 12, 0, 0, 0], header]
    stream = io.StringIO(newline="")
    csv.writer(stream).writerows(rows)
    data = (stream.getvalue() + '[HasHeaderRowAtEnd],1,[deviceprofile],Windows,[config],Development,[commandline],"Engine exe" -ExecCmds="hello,world"').encode()
    user = "C:/Project/Saved/Validation/UpgradeSmokeRuntime/check-01"
    original = user + "/Saved/Validation/UpgradeSmoke/upgrade-smoke-" + "a" * 32 + "/EngineCookedSmoke.csv"
    report = {"status": "PASS", "exit_code": 0, "mode": "cooked_development_render_smoke", "is_editor": False, "cooked_data_required": True,
              "isolated_smoke_until_process_exit": True, "engine_automation_testing": False, "phase": 17, "elapsed_seconds": 10,
              "started_utc": "2026-10-24T00:00:00Z", "user_dir": user, "engine_csv": {"status": "complete", "expected_path": original,
              "actual_path": original.replace("/", "\\"), "bytes": len(data), "sha1": hashlib.sha1(data).hexdigest(), "completed_utc": "2026-10-24T00:00:09Z",
              "capture_started_elapsed_seconds": .1, "stop_requested_elapsed_seconds": 8, "completion_elapsed_seconds": 9,
              "completion_future_ready": True, "final_header_metadata_present": True}}
    parsed = parse_csv(data)
    assert parsed["format"]["data_row_widths"] == [4, 9]
    assert parsed["metadata"]["config"] == "Development"
    result = analyze_payload(report, data)
    assert result["status"] == "analyzed" and result["observed_region_count"] == 3
    assert result["engine_gpu_time"]["p50"] == 3 and result["engine_gpu_time"]["p99"] == 5
    assert result["excluded_rows"]["phase17_artifact_finalization"] == 1
    assert result["excluded_rows"]["missing_or_invalid_tags"] == 1
    assert result["regions"]["verdan_market"]["capture_combat_groups"]["capture_pending_actual_combat"]["engine_csv_counters"]["GPUTime"]["status"] == "unavailable"
    bad = json.loads(json.dumps(report)); bad["engine_csv"]["sha1"] = "0" * 40
    assert analyze_payload(bad, data)["status"] == "unavailable"
    bad = json.loads(json.dumps(report)); bad["engine_csv"]["actual_path"] = original.replace("EngineCookedSmoke", "Other")
    assert analyze_payload(bad, data)["status"] == "unavailable"
    bad = json.loads(json.dumps(report)); bad["status"] = "RUNNING"
    assert analyze_payload(bad, data)["status"] == "unavailable"
    no_footer = data[:FOOTER.search(data.decode()).start()]
    bad = json.loads(json.dumps(report)); bad["engine_csv"].update(bytes=len(no_footer), sha1=hashlib.sha1(no_footer).hexdigest())
    assert analyze_payload(bad, no_footer)["status"] == "unavailable"
    editor_data = data.replace(b"[deviceprofile],Windows,", b"[deviceprofile],WindowsEditor,")
    bad = json.loads(json.dumps(report)); bad["engine_csv"].update(bytes=len(editor_data), sha1=hashlib.sha1(editor_data).hexdigest())
    assert analyze_payload(bad, editor_data)["status"] == "unavailable"
    assert distribution([0, 0, None, "nan"], METRICS["GPUTime"])["status"] == "unavailable"
    assert distribution([1, 2, 3, 4], METRICS["FrameTime"])["p50"] == 2
    duplicate = data.replace(b"GPUMem/LocalUsedMB\r\n", b"GPUMem/LocalUsedMB,Unused,Unused\r\n")
    assert parse_csv(duplicate)["format"]["duplicate_column_positions"]["Unused"] == [9, 10]
    out: dict[str, Any] = {"status": "PASS", "kind": "synthetic_parser_contract_selfcheck", "actual_cooked_runtime_executed": False,
                           "checks": ["late final header and missing early cells", "raw metadata quotes/commas", "all3region tags and four capture/combat groups",
                                      "phase17 excluded", "positive GPU samples only", "hash/path/incomplete evidence rejected", "completed footer required",
                                      "unrelated duplicate names retained without selecting aliases", "editor metadata cannot be relabeled cooked", "nearest-rank percentiles"]}
    if real_csv is not None:
        if real_csv.stat().st_size > MAX_CSV_BYTES:
            raise EvidenceError("Format fixture exceeds64MiB.")
        actual = parse_csv(real_csv.read_bytes())
        out["preserved_csv_format_check"] = {"path": str(real_csv.resolve()), "format": actual["format"],
                                              "classification": "editor_fixture" if editor_csv(actual["metadata"]) else "unclassified_format_only",
                                              "cooked_acceptance": False, "limits": "Parser-format check only; no status/hash manifest was used to accept cooked evidence."}
    return out


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("evidence", nargs="?", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--self-check-csv", type=Path, help="Optional preserved native CSV format fixture; never accepted as cooked by this mode.")
    args = parser.parse_args(argv)
    if not args.self_check and args.evidence is None:
        parser.error("evidence is required unless --self-check is selected")
    if args.self_check_csv and not args.self_check:
        parser.error("--self-check-csv requires --self-check")
    if args.evidence:
        inputs = locate(args.evidence.resolve())
        if args.output.resolve() in {p.resolve() for p in inputs}:
            parser.error("output must not overwrite preserved status/CSV inputs")
    if args.self_check_csv and args.output.resolve() == args.self_check_csv.resolve():
        parser.error("output must not overwrite the CSV format fixture")
    try:
        result = self_check(args.self_check_csv) if args.self_check else analyze_evidence(args.evidence)
    except (OSError, ValueError, AssertionError) as error:
        result = {"status": "FAIL", "kind": "parser_selfcheck", "reason": str(error), "actual_cooked_runtime_executed": False}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2, allow_nan=False) + "\n", encoding="utf-8")
    print(json.dumps({"status": result["status"], "output": str(args.output.resolve()), "classification": result.get("classification", result.get("kind")),
                      "engine_gpu_time": result.get("engine_gpu_time", {}).get("status", "unavailable")}, ensure_ascii=False))
    return 0 if result["status"] in ("analyzed", "PASS") else 2


if __name__ == "__main__":
    sys.exit(main())
