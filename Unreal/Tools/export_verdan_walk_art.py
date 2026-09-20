"""Execute the original Godot procedural gait in an isolated, disposable project."""
from pathlib import Path
import argparse, hashlib, json, re, subprocess
ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "scripts/utils/pixel_sprite.gd"
OUTPUT = ROOT / "Unreal/ArtSource/Verdan"
EVIDENCE = ROOT / "docs/unreal-migration/evidence/presentation2"

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--godot", type=Path, required=True)
    args = ap.parse_args()
    assert not OUTPUT.exists() or not any(OUTPUT.iterdir()), "Refuse to replace previously exported art"
    raw = SOURCE.read_bytes()
    source = raw.decode("utf-8").replace("\r\n", "\n")
    names = ["_detect_leg_top", "_build_walk_cycle", "_make_walk_frame", "get_texture_source"]
    methods = []
    for name in names:
        match = re.search(r"^static func " + name + r"\(.*?(?=^(?:static func |static var |func |##)|\Z)", source, re.M | re.S)
        assert match, name
        methods.append(match.group(0).rstrip() + "\n")
    constants = [line for line in source.splitlines() if line.startswith("const WALK_")]
    assert len(constants) == 5
    work = ROOT / "Unreal/Memoria/Saved/Validation/Presentation2SourceWalk02"
    work.mkdir(parents=True, exist_ok=False)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    (work / "project.godot").write_text('config_version=5\n[application]\nconfig/name="IsolatedSourceGait"\n', encoding="utf-8")
    header = "extends SceneTree\n" + "\n".join(constants) + "\nstatic var _walk_cycle_cache: Dictionary = {}\n"
    body = "\nfunc _initialize() -> void:\n"
    body += '\tfor direction in ["down", "up", "left", "right"]:\n'
    body += '\t\tvar image := Image.load_from_file("' + ROOT.as_posix() + '/assets/sprites/field/arrel/" + direction + ".png")\n'
    body += '\t\tassert(image != null)\n\t\tvar texture := ImageTexture.create_from_image(image)\n\t\ttexture.resource_name = direction\n'
    body += '\t\tvar frames := _build_walk_cycle(texture, direction)\n\t\tassert(frames.size() == 4)\n'
    body += '\t\tvar atlas := Image.create(512, 160, false, Image.FORMAT_RGBA8)\n\t\tatlas.fill(Color.TRANSPARENT)\n'
    body += '\t\tfor i in range(4):\n\t\t\tatlas.blit_rect(frames[i].get_image(), Rect2i(0, 0, 128, 160), Vector2i(i * 128, 0))\n'
    body += '\t\tassert(atlas.save_png("' + OUTPUT.as_posix() + '/ArrelWalk_" + direction + ".png") == OK)\n'
    body += '\tprint("SOURCE_GAIT_EXPORT_PASS four directions, four frames, 9 fps source cadence")\n\tquit(0)\n'
    harness = header + "\n".join(methods) + body
    (work / "export.gd").write_text(harness, encoding="utf-8")
    command = [str(args.godot), "--headless", "--path", str(work), "--script", "export.gd"]
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=90)
    log = result.stdout
    (EVIDENCE / "source_walk02_execution.log").write_bytes(log)
    (EVIDENCE / "source_walk02_harness.gd").write_bytes((work / "export.gd").read_bytes())
    assert result.returncode == 0 and b"SOURCE_GAIT_EXPORT_PASS" in log and not re.search(rb"SCRIPT ERROR|Parse Error|ERROR:|FATAL", log)
    sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    report = {"status": "PASS", "command": command, "exit_code": result.returncode, "source": SOURCE.relative_to(ROOT).as_posix(), "source_sha256": hashlib.sha256(raw).hexdigest(), "source_methods": names, "method_sha256": {name: hashlib.sha256(method.encode()).hexdigest() for name, method in zip(names, methods)}, "constants": constants, "outputs": {p.relative_to(ROOT).as_posix(): sha(p) for p in OUTPUT.glob("*.png")}}
    assert len(report["outputs"]) == 4
    (EVIDENCE / "source_walk_export.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("SOURCE_GAIT_EXPORT_PASS")
if __name__ == "__main__":
    main()
