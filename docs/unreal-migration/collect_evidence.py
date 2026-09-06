"""Read-only source audit; writes derived evidence only beside this script.

Run from any directory with Python 3. Uses the working tree, not only HEAD.
Lexical references are candidate dependencies, not proof of runtime reachability.
"""
from pathlib import Path
from collections import Counter
import csv
import hashlib
import json
import re
import subprocess
import struct

OUT = Path(__file__).resolve().parent
ROOT = OUT.parents[1]
EVIDENCE = OUT / "evidence"
EVIDENCE.mkdir(exist_ok=True)

def read(path):
    return path.read_text(encoding="utf-8-sig", errors="replace")

def rel(path):
    return path.relative_to(ROOT).as_posix()

def write_csv(name, rows, fields):
    with (EVIDENCE / name).open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)

def git(*args):
    return subprocess.check_output(["git", "-C", str(ROOT), *args]).decode("utf-8", "replace").strip()

project = read(ROOT / "project.godot")
autoload = dict(re.findall(r'^(\w+)="\*([^"]+)"', project, re.M))
scripts = sorted([*ROOT.glob("scripts/**/*.gd"), *ROOT.glob("scenes/**/*.gd")])
sources = {rel(p): read(p) for p in scripts}
index, edges, signals, connections, flags = [], [], [], [], []
for path, body in sources.items():
    lines = body.splitlines()
    methods = [{"name": m.group(1), "line": body[:m.start()].count("\n") + 1}
               for m in re.finditer(r"^(?:static )?func (\w+)\(", body, re.M)]
    index.append({"path": path, "lines": len(lines),
                  "class": re.findall(r"^class_name (\w+)", body, re.M),
                  "extends": re.findall(r"^extends (.+)", body, re.M),
                  "sha256": hashlib.sha256(body.encode()).hexdigest(),
                  "methods": methods,
                  "fields": [f"{i}: {s}" for i, s in enumerate(lines, 1)
                             if re.match(r"^(?:@export.* )?(?:var|const|enum|signal) ", s)],
                  "comments": [f"{i}: {s}" for i, s in enumerate(lines, 1)
                               if s.startswith("##")]})
    for name in autoload:
        refs = [str(i) for i, s in enumerate(lines, 1)
                if not s.lstrip().startswith("#") and re.search(r"\b" + name + r"\.", s)]
        if refs:
            edges.append({"source": path, "target": name, "lines": ";".join(refs)})
    for i, s in enumerate(lines, 1):
        if re.match(r"^signal ", s):
            signals.append({"source": path, "line": i, "declaration": s})
        if ".connect(" in s or ".emit(" in s or "connect(" in s and '"' in s:
            connections.append({"source": path, "line": i, "expression": s.strip()})
        for m in re.finditer(r'(?:set_flag|has_flag|get_flag)\(\s*"([^"]+)"', s):
            flags.append({"source": path, "line": i, "flag": m.group(1), "expression": s.strip()})

(EVIDENCE / "source_index.json").write_text(json.dumps(index, indent=2, ensure_ascii=False), encoding="utf-8")
write_csv("dependencies.csv", edges, ["source", "target", "lines"])
write_csv("signals.csv", signals, ["source", "line", "declaration"])
write_csv("event_sites.csv", connections, ["source", "line", "expression"])
write_csv("story_flag_sites.csv", flags, ["source", "line", "flag", "expression"])

scene_rows = []
for p in sorted(ROOT.glob("scenes/**/*.tscn")):
    body = read(p)
    scene_rows.append({"path": rel(p), "nodes": len(re.findall(r"^\[node ", body, re.M)),
                       "scripts": ";".join(re.findall(r'path="(res://[^"]+\.gd)"', body)),
                       "scene_refs": ";".join(re.findall(r'path="(res://[^"]+\.tscn)"', body)),
                       "node_types": ";".join(sorted(set(re.findall(r'\[node .*?type="([^"]+)"', body))))})
write_csv("scenes.csv", scene_rows, ["path", "nodes", "scripts", "scene_refs", "node_types"])

datasets = []
def walk(value, keys, types):
    if isinstance(value, dict):
        keys.update(value.keys())
        if "type" in value and isinstance(value["type"], str):
            types.update([value["type"]])
        for v in value.values():
            walk(v, keys, types)
    elif isinstance(value, list):
        for v in value:
            walk(v, keys, types)

for p in sorted(ROOT.glob("data/**/*.json")):
    body = read(p)
    data = json.loads(body)
    keys, types = Counter(), Counter()
    walk(data, keys, types)
    datasets.append({"path": rel(p), "root_type": type(data).__name__, "root_count": len(data),
                     "root_keys": list(data) if isinstance(data, dict) else [],
                     "keys": dict(sorted(keys.items())), "type_values": dict(sorted(types.items())),
                     "bytes": p.stat().st_size,
                     "sha256": hashlib.sha256(p.read_bytes()).hexdigest()})
(EVIDENCE / "data_catalog.json").write_text(json.dumps(datasets, indent=2, ensure_ascii=False), encoding="utf-8")

# Scan text for candidate direct and filename references, including generated path
# fragments. Unreferenced is explicitly NOT an unused-asset verdict.
texts = [project, *sources.values()]
for area in ["data", "scenes", "assets"]:
    for p in (ROOT / area).rglob("*"):
        if p.is_file() and p.suffix in {".json", ".tscn", ".tres", ".gdshader"}:
            texts.append(read(p))
all_text = "\n".join(texts)
assets = []
for area in ["assets", "mugic"]:
    for p in sorted((ROOT / area).rglob("*")):
        if not p.is_file():
            continue
        path, ext = rel(p), p.suffix.lower()
        width = height = ""
        if ext == ".png":
            with p.open("rb") as stream:
                header = stream.read(24)
            if header[:8] == b"\x89PNG\r\n\x1a\n":
                width, height = struct.unpack(">II", header[16:24])
        classification = ("Godot-specific and nonportable" if ext in {".import", ".uid", ".tres"}
                          else "requires recreation" if ext == ".gdshader"
                          else "requires conversion" if ext in {".ogg", ".mp3", ".svg"}
                          else "requires reimport" if ext in {".png", ".jpg", ".jpeg", ".webp", ".ttf", ".otf", ".wav"}
                          else "directly reusable source / inspect format")
        assets.append({"path": path, "extension": ext, "bytes": p.stat().st_size,
                       "width": width, "height": height, "classification": classification,
                       "reference_evidence": "exact path" if "res://" + path in all_text
                       else "filename/fragment candidate" if p.name in all_text
                       else "no literal reference (dynamic/unreachable status not proven)"})
write_csv("assets.csv", assets, ["path", "extension", "bytes", "width", "height", "classification", "reference_evidence"])
tracked = [path for path in git("ls-files", "-z").split("\0") if path]
stats = {"head": git("rev-parse", "HEAD"), "branch": git("branch", "--show-current"),
         "autoloads": autoload, "script_count": len(index), "script_lines": sum(x["lines"] for x in index),
         "runtime_script_count": sum(not x["path"].startswith("scripts/tools/") for x in index),
         "scene_count": len(scene_rows), "json_count": len(datasets),
         "asset_files_including_metadata": len(assets), "asset_bytes_including_metadata": sum(x["bytes"] for x in assets),
         "assets_by_extension": dict(Counter(x["extension"] for x in assets)),
         "asset_bytes_by_extension": {ext: sum(x["bytes"] for x in assets if x["extension"] == ext) for ext in sorted(set(x["extension"] for x in assets))},
         "tracked_files": len(tracked), "top_level_tracked": dict(Counter(x.split("/")[0] if "/" in x else "(root)" for x in tracked)),
         "methodology": "Lexical whole-source scan; manually reviewed findings are in the eight markdown reports. No execution or whole-game certification implied."}
(EVIDENCE / "summary.json").write_text(json.dumps(stats, indent=2, ensure_ascii=False), encoding="utf-8")
print(json.dumps(stats, indent=2, ensure_ascii=False))
