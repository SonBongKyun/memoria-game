"""Run the real source in an isolated Godot harness; export or check only."""
import argparse
import datetime
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
from starting_memory_ir import ROOT, IR, ORACLE, SOURCES, VERSION, KIND, canonical, sha, source_bytes, validate

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--godot', required=True, type=Path)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--evidence-dir', type=Path)
    args = parser.parse_args()
    work = ROOT/'Unreal/Memoria/Saved/Validation'/('catalog-export-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%f'))
    work.mkdir(parents=True)
    evidence = args.evidence_dir.resolve() if args.evidence_dir else work
    if any(evidence.is_relative_to(ROOT/'docs/unreal-migration/evidence'/p) for p in ('phase0','phase1a','phase1b','phase1b-ue58')):
        parser.error('Historical evidence is immutable')
    evidence.mkdir(parents=True, exist_ok=True)
    report = {'commands': [], 'check_only': args.check, 'status': 'RUNNING'}
    env = os.environ.copy()
    for key, name in [('APPDATA','UserData'),('LOCALAPPDATA','LocalUserData')]:
        env[key] = str(work/name); (work/name).mkdir()
    def run(command):
        result = subprocess.run(command, capture_output=True, text=True, encoding='utf-8', errors='replace', env=env, timeout=120)
        output = result.stdout+result.stderr
        report['commands'].append({'command': [str(c) for c in command], 'exit_code': result.returncode, 'output': output})
        if result.returncode or re.search(r'SCRIPT ERROR|Parse Error|FATAL|CRASH|Failed to load script|ERROR:', output, re.I):
            raise ValueError('Godot source extraction failed; see full evidence')
        return output
    try:
        version = run([str(args.godot), '--version']).strip()
        if not version.startswith('4.6.2.'):
            raise ValueError('Requires Godot 4.6.2')
        for path, name in [(SOURCES[0], 'memory_manager.gd'), (SOURCES[1], 'journey_oath.gd')]:
            shutil.copyfile(ROOT/path, work/name)
        for p in (ROOT/'Unreal/Tools/GodotOracle').glob('*.gd'):
            shutil.copyfile(p, work/p.name)
        (work/'project.godot').write_text('''config_version=5
[application]
config/name="MEMORIA_Offline_Catalog"
run/main_scene="res://export.tscn"
config/use_custom_user_dir=true
config/custom_user_dir_name="MEMORIA_Offline_Catalog"
[autoload]
GameManager="*res://game_manager_stub.gd"
AudioManager="*res://audio_manager_stub.gd"
NotificationToast="*res://notification_stub.gd"
SystemLog="*res://system_log_stub.gd"
MemoryManager="*res://memory_manager.gd"
[rendering]
renderer/rendering_method="gl_compatibility"
''',encoding='utf-8')
        (work/'export.tscn').write_text('[gd_scene load_steps=2 format=3]\n[ext_resource type="Script" path="res://starting_catalog.gd" id="1"]\n[node name="Export" type="Node"]\nscript = ExtResource("1")\n',encoding='utf-8')
        run([str(args.godot),'--headless','--path',str(work),'--editor','--import','--quit'])
        output = run([str(args.godot),'--headless','--path',str(work),'--',str(work/'raw.json')])
        if 'MEMORIA_STARTING_CATALOG_EXTRACTED' not in output:
            raise ValueError('Missing completion marker')
        raw = json.loads((work/'raw.json').read_text(encoding='utf-8'))
        # Godot JSON encodes integral Variant values as JSON numbers. No coercion
        # beyond proven integral numeric fields is allowed.
        for row in raw['entries']:
            for field in ('grade','burn_power'):
                if type(row[field]) not in (int,float) or int(row[field]) != row[field]:
                    raise ValueError('Source numeric field is not integral')
                row[field] = int(row[field])
        revision = subprocess.check_output(['git','log','-1','--format=%H','--',*SOURCES],cwd=ROOT,text=True).strip()
        value = {'schema_version':1,'content_kind':KIND,'extractor_version':VERSION,'source_repository_revision':revision,
                 'sources':[{'path':p,'sha256_utf8_lf':sha(source_bytes((ROOT/p).read_bytes()))} for p in SOURCES],
                 'entries':raw['entries']}
        from starting_memory_ir import fingerprint
        value['semantic_sha256'] = fingerprint(value['entries'])
        validate(value)
        data = canonical(value)
        oracle = canonical({'schema_version':1,'catalog_ir_sha256':sha(data),'initial':raw['initial'],'behavior_input':raw['behavior_input'],'behavior':raw['behavior']})
        for path, content in ((IR,data),(ORACLE,oracle)):
            if args.check:
                if not path.is_file() or path.read_bytes() != content:
                    raise ValueError('Source-derived artifact differs: '+str(path.relative_to(ROOT)))
            else:
                path.parent.mkdir(parents=True,exist_ok=True); path.write_bytes(content)
        report.update(status='PASS',source_repository_revision=revision,sources=value['sources'],
                      source_byte_sha256={p:sha((ROOT/p).read_bytes()) for p in SOURCES},
                      ir_sha256=sha(data),oracle_sha256=sha(oracle),semantic_sha256=value['semantic_sha256'],
                      ordered_ids=[r['id'] for r in value['entries']],godot_version=version)
    except Exception as error:
        report.update(status='FAIL',error=str(error))
    (evidence/'source_export.json').write_bytes(canonical(report))
    print('MEMORIA_CATALOG_EXPORT_'+report['status']+' '+report.get('error',report.get('ir_sha256','')))
    return 0 if report['status']=='PASS' else 1

if __name__ == '__main__':
    raise SystemExit(main())
