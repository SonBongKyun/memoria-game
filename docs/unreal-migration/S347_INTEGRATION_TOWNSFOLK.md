# S347: S346 integration and six townsfolk model delivery

S346 fffdef2d was reviewed and adopted as **45fb1a0cc1ed34a9c609b00f710a9212da5f23a6** over Codex883008621edd8f13dc484afc50114ce7bb96c8f4. A fresh UE5.8.2 build and52 relevant rendered tests pass. Six townsfolk models are delivered in the shared models directory, independently verified and packaged. Game application remains Claude's next step; no push, game packaging or deployment occurred.

## Integration and market review

Source: **fffdef2d50fc03b67032d432c03cb21a5d2ab27e**, Claude lane. Current peer **357d1a742459979b0bb78f6bc324401a335d7374**, clean/read-only. All3 production/test blobs match the accepted source commit exactly. The sole cherry-pick conflict was MIGRATION_STATE history, resolved retaining both S346 and the prior Codex S345/S344/S343 records. No additional product repair was required.

Ten stalls,17 warm nonshadowed/flickering lights, four imported lantern posts, two imported crate stacks and three hanging lanterns are accepted. Bottles share three materials; the actual replay records30 geometry batches and18 pitched roofs. The original7 physical surfaces, pawn collider, camera, story/interaction locations and run/memory state are retained. Fresh market, north, west/corner and Malet captures were inspected at1280x720.

The two review requests remain explicit:

- **New stall collision:** the presentation contract and test require its components to have NoCollision. The original level has7 physical surfaces, including two100x100x10cm ground landmarks under the old story stalls; it has no full-height counter/canopy bodies. The ten new stalls add visuals only, and can be walked through. Recommendation: keep this integration's original navigation; if blocking stalls is chosen, introduce footprint blockers in the level's navigation/interaction design and verify source/story routes together with NPC placement. A full canopy box would close too much ground.
- **Apparent standing on a canopy:** reproduced in the fresh `PlayFeel1/West.png`. The pawn remains on the ground, but its projected feet overlap the roof while the existing view-ray opening passes above that roof. This accepted visual limitation is preserved. A focused future canopy fade based on projected feet/roof overlap can address it while retaining collision, camera and story locations. It was not silently reported fixed.

## Fresh validation on45fb1a0c

Command: `python -B Unreal/Tools/validate_unreal.py --build-and-test --rendered --test-prefix Memoria.Audio.+Memoria.Campaign.+MemoriaVisual.+Memoria.BattleEntry.+Memoria.ShopTransactions. --automation-timeout2400 --evidence-dir Unreal/Memoria/Saved/Validation/s347-integration/s346-regression`

- UE5.8.2 editor build, fixture/audio-source/Verdan-story checks and automation: exits0, fatal diagnostics0.
- **52/52 unique registered IDs:** audio3, campaign4, all30 visual, shop transactions15. Raw50 successes plus2 successes with warning events; failed/not-run/in-process0; automation duration216.034s. The requested BattleEntry prefix resolves to0 current registered IDs; no separate BattleEntry test count is claimed. Existing field/battle transitions are covered by the selected campaign and visual cases.
- Diagnostics are retained:15 existing startup `Condition failed` lines, one layout compatibility warning, MotionVector warning in CanonicalPaidRoute and missing-world warning in Chapter1Presentation. These did not fail the selected tests; this report does not claim warning-free logs.
- This task did **not** rerun the complete274 registry. Claude's274/274 for S346 and older Codex results are separate historical evidence.
- Persisted exact52 IDs, command/log paths, warning events, source blobs and7 native capture hashes: [s346_results.json](evidence/s347/s346_results.json). Raw automation report: `C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe` invocation's `Saved/Validation/unreal-run-20261004T023241622289/Automation-20261004T023739810951/index.json`.

## Six models delivered

| ID | Actual height cm | Triangles | Vertices |
|---|---:|---:|---:|
| `npc_villager_f` | 165.00 | 24,153 | 12,378 |
| `npc_villager_m` | 175.00 | 15,361 | 7,990 |
| `npc_fisherman` | 172.00 | 18,113 | 9,406 |
| `npc_elder` | 165.00 | 24,449 | 12,532 |
| `npc_child` | 120.00 | 15,329 | 7,978 |
| `npc_scholar` | 172.00 | 23,889 | 12,244 |

Shared root: `C:/Users/jc/Documents/Codex/MEMORIA-Unreal-Collaboration/models`. Each `<id>/` contains `<id>_rigged.fbx` and `<id>_basecolor.png`:12 primary files, exact77 Arrel bone names/parents, one skinned mesh/material, max2 normalized influences, <=25k triangles,2048 RGB basecolor, cm/Zup/front-Y, root at0 and soles at ground. The child has nonuniform fitted child proportions and120cm authoring height.

Independent proof reads exported FBXs from relocated folders and resolves every relative texture from that folder. Rest geometry/UV/weights/height/units/material and Arrel hierarchy pass6/6. All6 are rendered front/side/back and through idle, both walking directions and reach; finite deformation, edge-stretch p99<1.8/max<4. Separate rest audit finds zero zero-area triangles, finite unit normals, positive object bases, root origins0 and A-pose arm alignment within15degrees of the delivered Arrel reference. This is manual Blender deformation QA, not actual Epic clip retarget or Unreal game application validation.

Visible fixes are included: the first women's dress prototype failed a walking check because center vertices switched abruptly between the two leg chains. The accepted skirt has continuous weighting and fixed waistband. Hidden trousers under closed skirts were omitted after real renders exposed intersections. Work outfits have a connected trouser seat, including the child. Long hair has a continuous backing under sculpted locks. The fisherman carries a bag/coiled line; elder/scholar have brooches.

Provenance: locally authored garment/body/accessory geometry and deterministic cloth/leather atlas, with fitted existing S310 project head/rig and original NPC palette values. No new AI inference, downloaded weights or model service was used. These are ambient NPC variants using a shared facial template, not six bespoke portrait sculpts. Elder uses upright A-pose; slight stoop belongs in animation. No cloth simulation or extra normal/roughness maps are supplied. Exact source rig/head atlas/helper and all7 reproduction-tool hashes are in the manifest.

- Inventory/limits/import instructions: shared `models/S347_NPC_MANIFEST.md` and JSON; local JSON snapshot [S347_NPC_MANIFEST.json](evidence/s347/S347_NPC_MANIFEST.json).
- Real common-scale studio preview: `models/_qa/s347/townsfolk_gallery.png`; individual real renders/reports under `_qa/s347/<id>/`. The gallery is not a game screenshot.
- Editable authored/roundtrip sources: `_raw/s347_townsfolk/<id>/`; seven reproducible tools: own `Unreal/Tools/*s347.py` plus shared `_raw/s347_townsfolk/scripts/`.
- Bundle `_handoff/s347_townsfolk.zip`: **26,386,713bytes**, SHA256 **55e8dde2d47e7b2c3fdc84089ad76a981f53cd685b38f3589fd3b3d58ebceed3**.42members, independent extraction CRC/all-file SHA matches and relative texture resolution6/6. [Exact delivery audit](evidence/s347/model_delivery_results.json).

## Claude's application step

Copy the two primary files per ID into `Unreal/ArtSource/FieldCharacters/<id>/`, verify the12 manifest hashes, and add the six Npcs entries to the existing AmbientModels commandlet while preserving the previous entries. Names: Villagerf, Villagerm, Fisherman, Elder, Child, Scholar. Retain S310/S334 skeletal import settings, separate sRGB basecolor and the existing neutral fill material. Retarget the existing Epic MM_Idle/MF_Unarmed_Walk_Fwd through the existing kit. Native game verification must check imported bounds/bones/facing/feet and both actual clips.

**Preserve the child proportions on application.** `CreateSkeletal` scales every model to the requested WorldHeight. Passing150 for every new NPC would make the child adult-sized. With180cm source Arrel rendered at ArrelHeight150, consistent authored ratios use `WorldHeight=ArrelHeight*(model_cm/180)`: female/elder137.5, male145.833, fisherman/scholar143.333 and child100 world units. Raw authoring height remains120cm for the child. Keep that visual-scale choice separate from NPC collider/story positions.

Verdan source tiles: female(7,6), fisherman(12,5), male(16,7), elder(4,9), child(19,6). Drift revisit uses female/scholar/male. Import, placement and final map lighting/animation/collision verification stay Claude-owned. No map NPCs were placed by this delivery task.

## Preservation and final boundaries

Post-run integrity: **845 existing own source/art/content files and70 earlier shared model files** retain baseline byte sizes/SHA256. Existing S337/S343/S344/S345 work is preserved. Peer remains clean/read-only at357d1a742459979b0bb78f6bc324401a335d7374; original Godot/foundation and Claude handoff were not modified. [Integrity evidence](evidence/s347/integrity_after.json).

Only the adopted S3463-source-file change/history,7 new model tools and owned review/session documents are committed here. Generated JSON snapshots under evidence/s347 follow the existing ignored-evidence policy and remain local; the complete manifest and QA reports are also inside the shared ZIP. The shared six primary folders/newS347raw-QA-bundle/manifest and Codex coordination records carry the delivery. Other existing model directories/tools are untouched. Final shared BOARD/codex-review/SESSION identify the concrete local commits and mark the build slot released after process checks. No remote push or release is authorized by this task.

## Versioned exact delivery inventory

The following inventory is preserved in this review commit as well as the shared manifest/ZIP.

| Primary relative path | Bytes | SHA256 |
|---|---:|---|
| npc_villager_f/npc_villager_f_rigged.fbx | 813292 | `bc86ffc0627aebee12dead8b9130f6f0d7303b866cd592af7e7f62f9691f5628` |
| npc_villager_f/npc_villager_f_basecolor.png | 3353070 | `22ae3f8903d014b0e632d15ec9268adc6dc9b3c08fa41e9ebeb748bcf8e66eba` |
| npc_villager_m/npc_villager_m_rigged.fbx | 579452 | `2bbec05f8521e999ebc8f2bd05e3f940b3155edb37158e797266044033267fd4` |
| npc_villager_m/npc_villager_m_basecolor.png | 3351758 | `2545503c9b4c3e381b81c72bfd5d1c9d9c4565167cf980348aea2c891df0e089` |
| npc_fisherman/npc_fisherman_rigged.fbx | 638620 | `54d0a56bb4f0175a10a4164939def031e1e8ef70071a4f55dfc1561b89f3dcd9` |
| npc_fisherman/npc_fisherman_basecolor.png | 3353206 | `4040443b9e094ebdc46d83b97d7094669b616a3fb422c8d191bc6d9e1b9dee24` |
| npc_elder/npc_elder_rigged.fbx | 820044 | `b45156f7395ac0d0e067c466e639862b6c21d0fd92177d40741da3372f926444` |
| npc_elder/npc_elder_basecolor.png | 3358705 | `bc9654a17c2731561acf01d9131c63d0c9277c0821aba70579726277850591ec` |
| npc_child/npc_child_rigged.fbx | 579340 | `3ae04ad1ef25fb94b851dfb3139b37573ee61cc7be2078b8a48f11808ddaccfb` |
| npc_child/npc_child_basecolor.png | 3362126 | `c6e03423d7c889c32be0114aa3a61f01973dc4dae796cb03d4290c348fd04c0a` |
| npc_scholar/npc_scholar_rigged.fbx | 806124 | `4a872ce3b3b489949e9d26b08b7253adf8d0aed604fce0721b50b01c0b04c6db` |
| npc_scholar/npc_scholar_basecolor.png | 3349324 | `8acdf3b353064f137dd5890638075c472095b3e1db0d6a83d45092608e485cd0` |

## Exact freshly executed test identities

```text
Memoria.Audio.CatalogAssets
Memoria.Audio.Routing
Memoria.Audio.RunReplacement
Memoria.Campaign.CanonicalPaidRoute
Memoria.Campaign.FilteredOriginalChoice
Memoria.Campaign.HostContinuation
Memoria.Campaign.UnseenFieldRoute
Memoria.ShopTransactions.Source.buy_both
Memoria.ShopTransactions.Source.buy_exact
Memoria.ShopTransactions.Source.buy_poor
Memoria.ShopTransactions.Source.buy_sell_ko
Memoria.ShopTransactions.Source.close
Memoria.ShopTransactions.Source.exchange
Memoria.ShopTransactions.Source.oath_en
Memoria.ShopTransactions.Source.oath_ko
Memoria.ShopTransactions.Source.sell_daily
Memoria.ShopTransactions.Source.sell_identity
Memoria.ShopTransactions.Source.sell_relation
Memoria.ShopTransactions.Source.sell_sense
Memoria.ShopTransactions.Canonical
Memoria.ShopTransactions.Guards
Memoria.ShopTransactions.RequestCancellation
MemoriaVisual.Achievements
MemoriaVisual.ArtworkCoverage
MemoriaVisual.BeltDressing
MemoriaVisual.Chapter1Journey
MemoriaVisual.Chapter1Presentation
MemoriaVisual.Chapter3
MemoriaVisual.Chapter3Travel
MemoriaVisual.Chapter4
MemoriaVisual.Chapter5
MemoriaVisual.ChapterContinue
MemoriaVisual.ChapterRevisit
MemoriaVisual.Codex
MemoriaVisual.DialogueInteraction
MemoriaVisual.DriftDressing
MemoriaVisual.EliaSkills
MemoriaVisual.FieldBurn
MemoriaVisual.FieldCharacters
MemoriaVisual.FieldCombat
MemoriaVisual.FieldEncounters
MemoriaVisual.FieldFeel
MemoriaVisual.FieldFoes
MemoriaVisual.FieldItems
MemoriaVisual.FieldRewards
MemoriaVisual.FoeVariety
MemoriaVisual.GameOver
MemoriaVisual.Journal
MemoriaVisual.PauseMenu
MemoriaVisual.TitleScreen
MemoriaVisual.TutorialHints
MemoriaVisual.VerdanExploration
```
