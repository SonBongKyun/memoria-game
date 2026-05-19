# Game Image Asset Intake

Source folder:

`../이미지/game image/`

These files were copied into the Godot project so they are included in editor imports and future exports.

## Runtime CG

`assets/cg/game_image/`

- `arrel_fullbody.png`
- `elia_fullbody.png`
- `tobias_fullbody.png`
- `nera_fullbody.png`
- `kairos_fullbody.png`
- `veil_fullbody.png`
- `arrel_ruins_rest.png`
- `env_frost_city.png`
- `env_memory_hall.png`
- `env_wasteland_city.png`
- `env_void_cathedral.png`
- `env_bureau_spires.png`
- `env_frozen_archive.png`

These are safe to reference directly from VN JSON using full `res://` paths.

## Reference Sheets

`assets/game_image/reference/`

Character turnaround, expression, sprite-sheet, UI, and skill-icon reference sheets live here. They are intentionally not sliced yet. The sheets are valuable as art direction and future sprite extraction sources, but using the full sheet in gameplay would look like a design document rather than an in-world scene.

## Current Usage

- Title slideshow now includes the new environment CGs and Arrel ruin illustration.
- `ch1_prologue.json` uses `arrel_fullbody.png`.
- `ch1_void_beast.json` uses `arrel_ruins_rest.png` for the post-battle beat.
- `ch1_after_forest.json` uses `env_wasteland_city.png` and `env_bureau_spires.png`.

