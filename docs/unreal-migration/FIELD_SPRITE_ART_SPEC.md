# Field character art: high-resolution illustrated sprites (request to Codex)

Requested by the user on 2026-09-27, after playing the S305 build. In the Verdan field, Arrel is a 3D prototype mesh, while Elia and Malet are 128×160 pixel chibi sprites. The user rejected both looks and chose **high-resolution illustrated sprites**, with the art produced by Codex.

The Unreal side is built to consume exactly this layout (S306, Claude lane). Until the art lands, the field falls back to the existing pixel sprites. Dropping files into the paths below and running the import commandlet switches a character over, with no code change.

## Status

- **2026-09-27, S307:** priority 1 was delivered by Codex, imported and accepted: `arrel`, `elia` and `malet` each have down, up and right. `FIELD_CHARACTER <id> hd` is reported, and the close review is at full resolution.
- **Open:** walk contacts and priority 2 and 3 characters.
- **Import notes:** the commandlet keeps these textures resident (`NeverStream`) and stretches them to a power of two for mips. `-Force` replaces existing packages.

## Characters and priority

| Priority | Id | Reference art (canon costume, face, palette) |
|---|---|---|
| 1 | `arrel` | `assets/portraits/character_shots/arrel_story_v2.png`, `arrel_battle_v3.png`, pixel `assets/sprites/field/arrel/*.png`, `assets/game_image/reference/arrel_sprite_sheet_reference.png` |
| 1 | `elia` | `assets/portraits/character_shots/elia_story_v2.png`, `elia_anchor_v3.png`, pixel `assets/sprites/field/elia/*.png` |
| 1 | `malet` | `assets/cg/game_image/malet_fullbody_stage.png`, `assets/portraits/malet_face_*.png`, pixel `assets/sprites/field/malet/down.png` |
| 2 | `sable`, `tobias`, `nera`, `kairos`, `veil` | `character_shots/<id>_story_v2.png` / `_v3.png`, `kairos_fullbody.png`, pixel `assets/sprites/field/<id>/` |

Canon notes:
- **Sable** is a blind old woman who lost her sight to the Void (canon fixed 2026-07-03). Do not draw her young or sighted.
- **Elia** keeps her white-and-gold robe and her staff (the lantern appears in Chapter 1 CGs).
- **Arrel** keeps his silver hair, plate armour, deep-blue cloak and the star emblem.

## Views per character (required)

- `down.png`: facing the camera, a slight 3/4 is fine.
- `up.png`: back view.
- `right.png`: facing screen-right, in profile or a strong 3/4. The left view is mirrored by the engine, so draw no left view. Asymmetric items (a sword on the hip) may flip; that is accepted.

Optional, but used when present:
- `walk_down_0.png`, `walk_down_1.png`, `walk_up_0.png`, `walk_up_1.png`, `walk_right_0.png`, `walk_right_1.png`: two contact poses per view, left foot forward and right foot forward, on the same canvas and pivot.
- Without them the engine animates walking with a bob and sway.

## Canvas and framing (all files identical)

- **Canvas:** PNG, RGBA, **1024 × 1536**, with a fully transparent background. No ground plane, cast shadow, vignette, text or frame.
- **Pivot:** the feet stand on **y = 1480** (the bottom of the soles), centred on **x = 512**. The engine anchors this point to the floor.
- **Height:** the silhouette's top (hair) sits at y ≈ 180 for Arrel, a figure about 1300 px tall. Other characters keep the same scale for their heights: Elia about 0.92 of Arrel, Malet about 1.0, Sable about 0.85 (stooped). Scale must be consistent across a character's views and between characters.
- **Edges:** clean alpha with no white or dark halo. Hair strands may be soft, but must not be matted onto a colour.

## Style

- Match the existing illustrated set (`character_shots` v2/v3 and the chapter CGs): painterly anime illustration, dark-fantasy palette, desaturated midtones, controlled rim light.
- Proportions: realistic to slightly stylised, about 7 heads tall. Not chibi: the field camera frames them at roughly 180–260 px on a 1080p screen, so silhouettes and costume colour blocks must read at that size.
- **Lighting:** neutral soft key from the upper front, a faint cool rim from behind, no strong coloured light. The engine adds scene lights (lanterns, moonlight) on top.
- **Camera:** eye level to a slightly high angle (at most 20° down). The engine tilts the card to face its 48° field camera, so do not bake a top-down angle.

## Delivery

Codex does not write in the Claude lane. Claude imports the art.
1. Codex writes the PNGs to the shared folder `C:/Users/jc/Documents/Codex/MEMORIA-Unreal-Collaboration/field_hd/<id>/`:
   - `down.png`, `up.png` and `right.png`;
   - optionally `walk_<view>_<0|1>.png`;
   - `field_hd/MANIFEST.md`: prompts, reference images used, model settings and a self-check per file.
2. Codex reports in `codex-review.md`: what was delivered, known issues, and anything regenerated.
3. Claude copies the files into `assets/sprites/field_hd/<id>/` in its lane and runs
   `UnrealEditor-Cmd Memoria.uproject -run=MemoriaFieldCharacterAssets -unattended -nop4 -NullRHI` (additive; it creates `/Game/Memoria/Presentation/FieldHD/T_<Id>_<View>` and `SPR_<Id>_<View>`).
   Claude then tunes the in-engine size, anchor and lighting, and commits.
4. Acceptance: `MemoriaVisual.FieldCharacters` reports `FIELD_CHARACTER <id> hd`, and the `Saved/Validation/PlayFeel1/` captures (`CharacterFront/Side/Back`, `Walk*`, `NearMalet`) are reviewed with the user.
