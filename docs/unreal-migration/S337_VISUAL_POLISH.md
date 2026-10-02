# S337 — Existing assets and chapter environment polish

User request: improve the quality of existing assets and graphics. Base: `f2104f4d`; branch: `codex/unreal-visual-polish-s337`.

## Visible changes

The Belt Waystation and Drift Shelter use weathered paving, earth, masonry and concrete surfaces. World-space patterning crosses tile boundaries, while the visible ground meets a consistent foot plane. Walls and concrete use a 100 cm block with a 3 cm edge bevel (44 triangles). Terrain rubble reuses the delivered stone model in instanced batches instead of three dark cubes per tile. Both terrain and decorative rubble retain their painted atlas and share a darker material tone.

The three ambient NPC materials now distinguish cloth, leather, skin and small metal fittings by the existing atlas regions. Faces and original base colors remain intact. The water tank separates rusted bodywork from its metal hoops, supports and tap, with restrained surface variation. The campfire and rubble receive subtle roughness/grain changes. The small original material fill remains in place for readable shadowed sides.

Chapter lighting retains each location's warm/cool palette with a slightly stronger fill and reduced shadow contrast. No camera or postprocess settings were changed.

## Reproduction and ownership

`MemoriaVisualPolishAssetsCommandlet` authors `/Game/Memoria/Presentation/VisualPolish/M_StoneSurface` and `SM_BeveledBlock`, and updates only six explicitly listed Field3D materials. Run after the existing S335 ambient import:

```text
UnrealEditor-Cmd.exe Memoria.uproject -run=MemoriaVisualPolishAssets -Refresh -AllowCommandletRendering -RenderOffscreen -unattended -nop4 -stdout -FullStdOutLogOutput
```

Use a rendering RHI. The commandlet preflights all original textures/materials, requires `-Refresh` for replacement, checks the generated mesh bounds and triangle count, and refuses to save if any of the seven materials fails shader compilation. No source texture, FBX, skeletal mesh, skeleton or animation is rewritten.

The original material packages and 149 source/package hashes are preserved under `Saved/Validation/s337-visual-polish/`. The source-derived chapter map data, hidden collision layer, gameplay, save behavior and story are unchanged. Two existing rendered chapter tests now verify actual material/model use and compare every hidden collision transform against the source tile map.

## Validation

- Fresh UE5.8.2 C++ build passed. Final rendered visual registry **26/26 PASS** (24 clean, 2 with existing warnings; failed 0; runtime 109.98 s). The full 274 gameplay registry was not rerun for this visual-only change.
- Final asset authoring (`author-20261002T053525.json`): exit 0, all seven owned material resources compiled with complete shader maps and zero compile errors, eight packages saved. Generated LOD: 96 rendered vertices / 44 triangles, inward normals 0 / inward faces 0.
- Asset preservation: 149 original source/Field3D files checked. Only six named material packages changed; other 143 files, including source textures/FBX, skeletal meshes, skeletons and animations, match their original SHA-256.
- Two existing chapter replays validate the actual detailed render assets and every original terrain block's world location, scale and rotation.

The first authoring attempt stopped before saving because UE5.8 `PostEditChange` prepares resources with `PrecompileMode::None`; the commandlet now explicitly requests `CacheShaders(Default)` and checks the actual owner/resource/map. First-render inspection found inward bevel faces even though functional replays passed. The builder now uses Unreal's winding convention and rejects any inward triangle or rendered vertex normal. Both intermediate attempts and captures remain under the task evidence directory.

The first rendering commandlet also compiled missing engine debug shaders and logged Niagara/FXC diagnostics attached to engine debug materials. These are distinct from the seven authored material checks; the final visual run has shader failures 0 and fatal/assertion/unhandled-exception lines 0. It retains the known 15 startup `Condition failed` lines and the prior `r.MotionVectorSimulation` / transient no-world-context warnings; no engine warning cleanup is claimed. Historical S336 results are not reported as fresh full-registry validation.

## Scope limits

This is surface and environment polish. It does not add new character topology, NPC movement, new content or a packaged release. Other agents' worktrees and the original Godot/foundation checkouts remain untouched.

Reference note: the legacy GDD in OneDrive could not be hydrated (OS error 389). The active Unreal collaboration/migration documents, current game captures and recorded MEMORIA art direction guided this bounded polish. No story or canon changes were made.

## Final verification and captures

```text
python -B Unreal/Tools/validate_unreal.py --build-and-test --rendered --test-prefix MemoriaVisual. --automation-timeout 1800 --evidence-dir Unreal/Memoria/Saved/Validation/s337-visual-polish/visual-final
```

Final automation report: `Saved/Validation/unreal-run-20261002T053639355427/Automation-20261002T053642327379/index.json`.

[Validation summary and screenshot hashes](evidence/s337-visual-polish/validation-summary.json). [Original asset preservation audit](evidence/s337-visual-polish/asset-preservation.json).

| View | Before | After |
|---|---|---|
| Belt Waystation | [Original](evidence/s337-visual-polish/before/RevisitBelt.png) | [Polished](evidence/s337-visual-polish/after/RevisitBelt.png) |
| Drift Shelter campfire | [Original](evidence/s337-visual-polish/before/Chapter4Campfire.png) | [Polished](evidence/s337-visual-polish/after/Chapter4Campfire.png) |
| Drift Shelter rubble | [Original](evidence/s337-visual-polish/before/Chapter4Rubble.png) | [Polished](evidence/s337-visual-polish/after/Chapter4Rubble.png) |

Per the repository evidence policy, captures, detailed JSON audits and engine logs are retained locally and excluded from Git; this report and the reproducible source/assets are committed. All six archived comparison images are 1280×720. Captures were inspected: stone patterns and bevel tops render correctly; the road is clear; model rubble is consistent; NPCs and the tank remain readable. Vertical wall faces remain dark under the current directional lighting. No packaged build or long manual performance/play session was performed.
