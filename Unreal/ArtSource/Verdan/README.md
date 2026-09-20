# Source-derived Arrel gait atlases

These four atlases were exported by executing the existing Godot PixelSprite gait
functions in an isolated project, not by drawing a new animation. Each contains
four original 128 x 160 frames, left to right, at the source 9 fps cadence.

Provenance and complete raw harness/logs:
`docs/unreal-migration/evidence/presentation2/source_walk_export.json`
and `source_walk_raw.zip`. The source hash is recorded and protected.
`Unreal/Tools/export_verdan_walk_art.py` refuses to replace existing exports.
It requires the existing Godot 4.6.2 executable and writes only the isolated
Saved harness and these derived art files. No source checkout import is needed.

UE runtime reads the imported textures/sprites under /Game/Memoria/Presentation/Verdan.
These PNGs are editor import inputs, not runtime file dependencies.
