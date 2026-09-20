"""Archive fresh diagnostics/state and selected captures for the Verdan depth pass."""
from pathlib import Path
import argparse, hashlib, json, zipfile
ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / "docs/unreal-migration/evidence/character2"
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--execution", type=Path, required=True)
    ap.add_argument("--since", type=float, required=True)
    args = ap.parse_args()
    dest = args.execution.resolve()
    assert dest.is_relative_to(BASE) and not (dest / "raw_execution.zip").exists()
    report = json.loads((dest / "unreal_validation.json").read_text(encoding="utf-8"))
    files = {c["name"] + ".log": ROOT / c["log"] for c in report["commands"] if "log" in c}
    if "automation_report" in report:
        files["automation_index.json"] = ROOT / report["automation_report"]
    # These original-sized captures are read by the independent full-state validator.
    required_firebomb = {"FirebombCanonical_" + name + ".png" for name in (
        "WorldSeed_Complete", "Potion_Before", "Potion_Granted", "Potion_Toast",
        "Antidote_Before", "Antidote_Granted", "Antidote_Signal", "Antidote_Toast",
        "Firebomb_Before", "Firebomb_Granted", "Reward_Toasts", "Shop_Deferred")}
    omitted = []
    for group in ["Phase1O", "Phase1E", "Character2"]:
        for p in (ROOT / "Unreal/Memoria/Saved/Validation" / group).glob("*"):
            if not p.is_file() or p.stat().st_mtime < args.since:
                continue
            selected = p.suffix == ".json" or group == "Character2" or p.name in required_firebomb or p.name in ["CanonicalPaidRoute_Exploration.png", "CanonicalPaidRoute_Moved.png"]
            if selected:
                files[group + "/" + p.name] = p
            else:
                omitted.append(p.name)
    manifest = {}
    with zipfile.ZipFile(dest / "raw_execution.zip", "x", zipfile.ZIP_DEFLATED) as z:
        for name, p in files.items():
            data = p.read_bytes()
            z.writestr(name, data)
            manifest[name] = {"source": str(p), "size": len(data), "sha256": hashlib.sha256(data).hexdigest()}
    with zipfile.ZipFile(dest / "raw_execution.zip") as z:
        assert all(hashlib.sha256(z.read(name)).hexdigest() == row["sha256"] for name, row in manifest.items())
    (dest / "raw_manifest.json").write_text(json.dumps({"files": manifest, "unarchived_redundant_captures": omitted}, indent=2) + "\n", encoding="utf-8")
    print("ARCHIVED", len(files), "raw files; redundant screenshots omitted", len(omitted))
if __name__ == "__main__":
    main()
