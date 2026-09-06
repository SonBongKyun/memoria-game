"""Execute unmodified Godot memory rules offline in a separate minimal project."""
from pathlib import Path
import argparse, hashlib, json, os, re, shutil, subprocess

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--godot', required=True)
parser.add_argument('--reference', type=Path, default=ROOT)
args = parser.parse_args()
godot = Path(args.godot).resolve()
reference = args.reference.resolve()
workspace = ROOT/'Unreal/Memoria/Intermediate/GodotOracle'
workspace.mkdir(parents=True, exist_ok=True)
fixtures = ROOT/'docs/unreal-migration/fixtures'
inputs = fixtures/'player_memory_inputs.json'
outputs = fixtures/'player_memory_expected.json'
version = subprocess.check_output([str(godot), '--version'], text=True).strip()
if not version.startswith('4.6.2.'):
    raise SystemExit(f'Oracle requires Godot 4.6.2, detected {version}')
hashes = {}
for source, target in [('scripts/systems/memory_manager.gd','memory_manager.gd'), ('scripts/utils/journey_oath.gd','journey_oath.gd')]:
    data = (reference/source).read_bytes()
    hashes[source] = hashlib.sha256(data).hexdigest()
    (workspace/target).write_bytes(data)
for p in (ROOT/'Unreal/Tools/GodotOracle').glob('*.gd'):
    shutil.copyfile(p, workspace/p.name)
(workspace/'project.godot').write_text('''config_version=5
[application]
config/name="MEMORIA_Phase1A_Offline_Oracle"
run/main_scene="res://oracle.tscn"
config/use_custom_user_dir=true
config/custom_user_dir_name="MEMORIA_Phase1A_Offline_Oracle"
[autoload]
GameManager="*res://game_manager_stub.gd"
AudioManager="*res://audio_manager_stub.gd"
NotificationToast="*res://notification_stub.gd"
SystemLog="*res://system_log_stub.gd"
MemoryManager="*res://memory_manager.gd"
[rendering]
renderer/rendering_method="gl_compatibility"
''', encoding='utf-8')
(workspace/'oracle.tscn').write_text('[gd_scene load_steps=2 format=3]\n[ext_resource type="Script" path="res://oracle.gd" id="1"]\n[node name="Oracle" type="Node"]\nscript = ExtResource("1")\n', encoding='utf-8')
env = os.environ.copy()
env['APPDATA'] = str(workspace/'UserData')
env['LOCALAPPDATA'] = str(workspace/'LocalUserData')
Path(env['APPDATA']).mkdir(exist_ok=True)
Path(env['LOCALAPPDATA']).mkdir(exist_ok=True)
commands = [
    [str(godot), '--headless', '--path', str(workspace), '--editor', '--import', '--quit'],
    [str(godot), '--headless', '--path', str(workspace), '--', str(inputs), str(outputs)],
]
logs = []
for command in commands:
    result = subprocess.run(command, capture_output=True, text=True, encoding='utf-8', errors='replace', env=env, timeout=90)
    log = result.stdout + result.stderr
    logs.append({'command': command, 'exit_code': result.returncode, 'output': log})
    (workspace/'execution.json').write_text(json.dumps(logs, ensure_ascii=False, indent=2), encoding='utf-8')
    if result.returncode != 0 or re.search(r'SCRIPT ERROR|Parse Error|FATAL|CRASH|Failed to load script|ERROR:', log, re.I):
        print(log)
        raise SystemExit('Godot oracle failed; expected results must not be accepted')
if 'MEMORIA_MEMORY_ORACLE_PASS' not in logs[-1]['output']:
    raise SystemExit('Missing oracle completion marker')
metadata = dict(schema_version=1, engine_version=version, reference_head=subprocess.check_output(['git','-C',str(reference),'rev-parse','HEAD'], text=True).strip(),
    source_sha256=hashes, input_sha256=hashlib.sha256(inputs.read_bytes()).hexdigest(),
    expected_sha256=hashlib.sha256(outputs.read_bytes()).hexdigest(),
    harness='unmodified memory_manager.gd and journey_oath.gd; fixture instance outside tree; no UI/profile listeners',
    scope='player-memory domain only; no GameManager story effects, battle, audio, UI or full Godot-save import certified')
(fixtures/'player_memory_provenance.json').write_text(json.dumps(metadata, indent=2) + '\n', encoding='utf-8')
print(next(line for line in logs[-1]['output'].splitlines() if 'MEMORIA_MEMORY_ORACLE_PASS' in line))
