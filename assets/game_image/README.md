# Game Image Asset Intake

Source folder:

`../이미지/game image/`

These files were copied into the Godot project so they are included in editor imports and future exports.

## Runtime CG

`assets/cg/game_image/`

- `tobias_fullbody.png`
- `nera_fullbody.png`
- `kairos_fullbody.png`
- `veil_fullbody.png`
- `env_frost_city.png`
- `env_memory_hall.png`
- `env_wasteland_city.png`
- `env_void_cathedral.png`
- `env_bureau_spires.png`
- `env_frozen_archive.png`
- `chapter_sealed_zone.png`
- `nera_void_cavern.png`
- `malet_bureau_overlook.png`
- `tobias_memory_corridor.png`
- `sheet_arrel_profile.png`
- `sheet_arrel_battle_ready.png`
- `sheet_arrel_memory_fading.png`
- `sheet_elia_profile.png`
- `sheet_elia_healing.png`
- `sheet_elia_memory_restoration.png`
- `sheet_arrel_elia_duo.png`
- `memory_crystal_choice.png`
- `void_beast_confrontation.png`
- `kairos_sealed_city.png`
- `sealed_city_ruins.png`
- `sealed_gate_plaza.png`
- `memory_crystal_item.png`
- `memory_loss_warning.png`
- `world_map_memoria.png`

These are safe to reference directly from VN JSON using full `res://` paths.

## Reference Sheets

`assets/game_image/reference/`

Character turnaround, expression, sprite-sheet, UI, VFX, world, and icon reference sheets live here. Most sheets are intentionally not sliced yet. The sheets are valuable as art direction and future sprite extraction sources, but using the full sheet in gameplay would look like a design document rather than an in-world scene.

S82 exception: `malet_expression_sheet.png` has been sliced into runtime dialogue portraits in `assets/portraits/malet_*_hd.png`.

S83 exception: Arrel and Elia now use only the new sheet pipeline for runtime portrait/CG work:
- `arrel_expression_sheet.png` -> `assets/portraits/arrel_sheet_*.png`
- `elia_expression_sheet.png` -> `assets/portraits/elia_sheet_*.png`
- sheet-derived story plates live in `assets/cg/game_image/sheet_arrel_*.png`, `sheet_elia_*.png`, and `sheet_arrel_elia_duo.png`

S84 exception: Arrel and Elia sprite sheets are now sliced into actual gameplay frames:
- `arrel_sprite_sheet_reference.png` -> `assets/sprites/characters/arrel_sheet/*.png`
- `elia_sprite_sheet_reference.png` -> `assets/sprites/characters/elia_sheet/*.png`
- exploration and battle sprites load these frames through `PixelSprite.create_sheet_frames()` and `PixelSprite.create_battle_sprite_frames()`.

Legacy CG files that lived directly under `assets/cg/` were removed after all runtime references were moved to `assets/cg/game_image/`. The `assets/cg/` folder should now be treated as a container for curated subfolders, not as a place for loose story art.

## Current Usage

- The title screen uses `game_start.png`.
- `ch1_prologue.json` uses sheet-derived Arrel runtime plates.
- Act I VN scenes now use `chapter_sealed_zone.png`, `sheet_arrel_elia_duo.png`, `void_beast_confrontation.png`, `sheet_arrel_battle_ready.png`, and `sheet_arrel_memory_fading.png`.
- Ch2 and later dialogue scenes now use the new Malet, Kairos, sealed-city, memory-crystal, and warning plates where they replace older lower-quality placeholders.
- Artbook now exposes the new character sheets, enemy sheets, UI references, VFX sheet, world map, and runtime CG plates.
- Player exploration, Elia companion, and Arrel/Elia battle bodies now use sheet-derived animated sprite frames instead of generated placeholder blocks or portrait stand-ins.
