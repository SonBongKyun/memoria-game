"""Copy the UE 5.8 template mannequin (mesh, skeleton, melee and locomotion animations) into the project.

The action combat prototype (S310) uses Epic's mannequin as the stand-in for every 3D combatant
until the real character models arrive. The assets ship with every UE 5.8 install, so they are
copied from the engine instead of being committed; the destination is git-ignored.
The template content was authored at /Game/Characters/Mannequins, so it keeps that mount path.
"""
import argparse
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEST = ROOT / 'Unreal' / 'Memoria' / 'Content' / 'Characters' / 'Mannequins'
DEFAULT_ENGINE = Path(r'C:\Program Files\Epic Games\UE_5.8')
SOURCE = Path('Templates') / 'TemplateResources' / 'High' / 'Characters' / 'Content' / 'Mannequins'
# Everything the prototype reads; the gun sets are left behind.
REQUIRED = [
    'Meshes/SKM_Manny_Simple.uasset', 'Meshes/SKM_Quinn_Simple.uasset', 'Meshes/SK_Mannequin.uasset',
    'Anims/Unarmed/BS_Idle_Walk_Run.uasset', 'Anims/Unarmed/MM_Idle.uasset',
    'Anims/Unarmed/Attack/MM_Attack_01.uasset', 'Anims/Unarmed/Attack/MM_Attack_02.uasset',
    'Anims/Unarmed/Attack/MM_Attack_03.uasset', 'Anims/Unarmed/Attack/MM_ChargedAttack.uasset',
    'Anims/Unarmed/Jump/MM_Dash.uasset', 'Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_01.uasset',
    'Anims/Death/MM_Death_Front_01.uasset',
]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--engine-root', type=Path, default=DEFAULT_ENGINE)
    parser.add_argument('--check', action='store_true', help='report whether the copy is complete')
    args = parser.parse_args()
    if args.check:
        missing = [r for r in REQUIRED if not (DEST / r).exists()]
        print('MANNEQUIN_OK' if not missing else 'MANNEQUIN_MISSING ' + ' '.join(missing))
        return 0 if not missing else 1
    source = args.engine_root / SOURCE
    if not source.is_dir():
        print(f'MANNEQUIN_SOURCE_MISSING {source}')
        return 1
    for part in ('Meshes', 'Materials', 'Textures', 'Rigs', 'Anims/Unarmed', 'Anims/Death', 'Anims/Rifle/HitReact'):
        shutil.copytree(source / part, DEST / part, dirs_exist_ok=True)
    missing = [r for r in REQUIRED if not (DEST / r).exists()]
    print('MANNEQUIN_INSTALLED' if not missing else 'MANNEQUIN_MISSING ' + ' '.join(missing))
    return 0 if not missing else 1


if __name__ == '__main__':
    sys.exit(main())
