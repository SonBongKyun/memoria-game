"""Byte-preservation gate for the 2.5D depth pass, independent of its runtime."""
from pathlib import Path
import argparse, hashlib, json
ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / "docs/unreal-migration/evidence/playfeel1"
ALLOWED = set(json.loads((BASE / 'entering_baseline.json').read_text(encoding='utf-8'))['allowed_modified'])

def sha(p):
    h = hashlib.sha256()
    with p.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()
def main():
    ap = argparse.ArgumentParser(); ap.add_argument("--output", type=Path, required=True); args = ap.parse_args()
    out = args.output.resolve(); assert out.is_relative_to(BASE) and not out.exists()
    baseline = json.loads((BASE / "entering_baseline.json").read_text(encoding="utf-8"))
    tracked = baseline["tracked_sha256"]
    changed = [p for p, h in tracked.items() if not (ROOT / p).is_file() or sha(ROOT / p) != h]
    bad = sorted(set(changed) - ALLOWED)
    manifest_path = Path(baseline["original_manifest"])
    original = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
    original_bad = [p for p, h in original["hashes"].items() if sha(Path(original["root"]) / p) != h.lower()]
    old_packages = [p for p in tracked if p.startswith("Unreal/Memoria/Content/") and Path(p).suffix in (".uasset", ".umap")]
    packages = {p.relative_to(ROOT).as_posix(): sha(p) for p in (ROOT / "Unreal/Memoria/Content").rglob("*") if p.suffix in (".uasset", ".umap")}
    new = sorted(set(packages) - set(old_packages))
    expected = []
    checks = {"original_manifest_unchanged": sha(manifest_path) == baseline["original_manifest_sha256"], "original_4217_preserved": len(original["hashes"]) == 4217 and not original_bad, "protected_worktree_bytes_preserved": not bad, "previous_95_packages_preserved": len(old_packages) == 95 and all(packages.get(p) == tracked[p] for p in old_packages), "no_new_packages": new == expected and len(new) == 0, "new_narrative_ir_zero": not any(p.is_file() and p.relative_to(ROOT).as_posix() not in tracked for p in (ROOT / "docs/unreal-migration/ir").rglob("*")), "historical_evidence_and_reports_preserved": not any(p.startswith("docs/unreal-migration/") and p not in ALLOWED for p in changed)}
    result = {"status": "PASS" if all(checks.values()) else "FAIL", "checks": checks, "original_count": len(original["hashes"]), "original_mismatches": original_bad, "protected_worktree_count": len(set(tracked) - ALLOWED), "worktree_mismatches": bad, "authorized_modified": changed, "previous_package_count": len(old_packages), "prior_ir_fixture_count": sum(p.startswith(("docs/unreal-migration/ir/", "docs/unreal-migration/fixtures/")) for p in tracked), "new_packages": {p: packages[p] for p in new}}
    out.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({k: result[k] for k in ["status", "checks", "protected_worktree_count", "prior_ir_fixture_count"]}))
    return result["status"] != "PASS"
if __name__ == "__main__":
    raise SystemExit(main())
