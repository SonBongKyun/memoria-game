# S348 integration review: townsfolk in Verdan and Drift

Claude's S348 **f1bb1337ee851aa81acdac6fb4f13854de67fa7a** is accepted into Codex as **ba3c555793976d02f567688fc639942b2eea99f6**, over **030bf0579a7cd2f6989a14997b71003496adf51e**. Additional imported-asset and live-scale checks are committed as **560b7cccd9ed93f0f2edafa4503b5b779a4beb7e**, the exact freshly tested game/test snapshot. No product repair was required; no new push, game packaging or deployment occurred.

## Integration and review decisions

Actual checkout: `C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex`, branch `codex/unreal-s342-integration-env-s343-20261003`. Peer `memoria-unreal-claude` remains clean/read-only at **eac9e0efb1ffe72a3c99a92d5243fa3dfdc09351**. The original Godot project, foundation, shared models and Claude handoff were not modified.

The incoming change has70 paths:12 source FBX/PNG files,49 Unreal packages (48 new and the expected regenerated `IK_Mannequin_Ambient`),8 source/test files and migration history. All12 raw models match the delivered S347 files; all49 packages are hydrated actual binary assets matching their source-commit LFS SHA256 and byte sizes and the peer's bytes. The importer was not rerun.

Two conflicts were resolved deliberately:

- `MIGRATION_STATE.md`: both the new Claude S348 entry and the prior Codex S347/S345/S344/S343 histories are retained.
- `MemoriaChapterDressingTests.cpp`: the new visible-NPC count/capture stage is followed by every existing S345 standing/battle/win/pause/title audio stage. No previous lifecycle assertions were dropped. The replay runs through step8.

The five unchanged production blobs (`FieldCharacterComponent.{h,cpp}`, `VerdanPresentation.{h,cpp}`, `AmbientModelsCommandlet.cpp`) match the source commit exactly. `ChapterPresentation.cpp` incorporates the one S348 height change while preserving Codex's earlier painted `M_Surface`, Tint, roughness and ember fixes. Added visual assertions are the only further source changes.

The original Godot `scenes/maps/verdan_market.gd` S55 lists woman(7,6), fisherman(12,5), man(16,7), elder(4,9), child(19,6). The smaller Unreal square uses west shopping(-470,105), northern plaza(-160,300), central/eastern plaza(110,140), west elder(-640,30) and eastern child(290,215). This retains their relative roles/placement while keeping the old stalls, story places and Malet's southern table clear. Accepted as a presentation adjustment; it does not redefine source story/interaction points. Original seven level collision surfaces and all other existing source/art/content remain preserved. Live NPC skeletal components have NoCollision and no generated overlaps.

Verdan has five rigged idle figures; Drift's revisit has three existing ambient actors on the new woman/scholar/man models. The six imported physical heights are165/175/172/165/120/172cm. Their77 bone names and parent indices match the actual Arrel skeleton; each has its own lit skeletal material and2048x2048 atlas and matching idle/walk animation skeletons. The game preserves these ratios against Arrel's180cm source: at Arrel worldheight150, the child is100 units and the woman/elder137.5. Belt's traveler/agent/guard become145/146.67/148.33 instead of all150, as intended.

Fresh Market/NearMalet/West and Belt/Drift NPC captures were visually inspected. All five market figures are present with the child visibly smaller; Malet's interaction remains available. The Drift capture shows its woman and scholar, with the man partly at the right edge; it is not represented as an unobstructed group portrait. Runtime visibility assertions confirm all three. No new movement or dialogue was added. Verdan still idles without strolling/turning, the shared facial template remains, and S346's pass-through stalls/apparent west-canopy standing remain existing documented limits.

## Fresh validation on560b7ccc

A separate fresh serial build used UE5.8.2 `Build.bat MemoriaEditor Win64 Development -Project=<own project> -WaitMutex -NoHotReloadFromIDE -MaxParallelActions=1`:28 actions, exit0, fatal0,506.864s wall time. This avoided the peer's previous multi-action memory failure without terminating user applications. The standard validator's subsequent build was explicitly up to date, exit0; it did not use an older binary.

Command: `python -B Unreal/Tools/validate_unreal.py --build-and-test --rendered --engine-root "C:/Program Files/Epic Games/UE_5.8" --test-prefix Memoria.Audio.+Memoria.Campaign.+MemoriaVisual.+Memoria.BattleEntry.+Memoria.ShopTransactions. --automation-timeout2400 --evidence-dir Unreal/Memoria/Saved/Validation/s348-integration/regression`

- **52/52 distinct registered tests PASS:** audio3, campaign4, shop transactions15 and all30 visual. Raw50 successes plus2 successes with warning events; failed/not-run/in-process0. Automation duration199.1796875s.
- Fixture, audio-source, Verdan-story, standard editor build and automation commands all exit0/fatal0. Actual loaded-model checks pass for all six, plus five live world-scale/collision checks. Belt and Drift retained audio lifecycle checks both pass.
- The requested BattleEntry prefix resolves to0 current registered IDs; no separate BattleEntry test count is claimed. Field combat and chapter transitions are covered by the selected actual cases.
- Diagnostics retained:15 existing startup `Condition failed` lines, all before the first selected test; one startup editor-layout compatibility warning; MotionVector warning in CanonicalPaidRoute and missing-world warning in Chapter1Presentation. Serial compilation emits the existing C4996 warning about `UMaterial::bUsedWithSkeletalMesh` at the importer's line73. This is not a warning-free-log claim.
- **45 fresh captures** under PlayFeel1/ChapterDressing all have1280x720 pixels, including both new `_Npcs.png` views. Captures include the editor preview title strip.
- Full274 was **not rerun** by this task. Claude's reported274/274 plus30/30 on its S348 branch are separate historical evidence.
- Post-test integrity:950 existing own source/art/content/tool files and82 primary shared model files retain hashes; incoming12 source files+49 packages retain the accepted hashes/sizes; peer clean/HEAD unchanged. No shared model or existing unrelated package was rewritten.

Exact local evidence: `Unreal/Memoria/Saved/Validation/s348-integration/final_results.json`, `baseline.json`, `serial_build.json/log`, `regression/unreal_validation.json`; raw automation `Unreal/Memoria/Saved/Validation/unreal-run-20261004T065042692898/Automation-20261004T065220698085/index.json` (the exact path is also stored in final_results.json). Captures live in `Saved/Validation/PlayFeel1` and `ChapterDressing`. The saved evidence is local generated output; identities and primary hashes below are retained in this versioned review.

## Imported primary SHA256 inventory

| File | Bytes | SHA256 |
|---|---:|---|
| `npc_child/npc_child_basecolor.png` | 3362126 | `c6e03423d7c889c32be0114aa3a61f01973dc4dae796cb03d4290c348fd04c0a` |
| `npc_child/npc_child_rigged.fbx` | 579340 | `3ae04ad1ef25fb94b851dfb3139b37573ee61cc7be2078b8a48f11808ddaccfb` |
| `npc_elder/npc_elder_basecolor.png` | 3358705 | `bc9654a17c2731561acf01d9131c63d0c9277c0821aba70579726277850591ec` |
| `npc_elder/npc_elder_rigged.fbx` | 820044 | `b45156f7395ac0d0e067c466e639862b6c21d0fd92177d40741da3372f926444` |
| `npc_fisherman/npc_fisherman_basecolor.png` | 3353206 | `4040443b9e094ebdc46d83b97d7094669b616a3fb422c8d191bc6d9e1b9dee24` |
| `npc_fisherman/npc_fisherman_rigged.fbx` | 638620 | `54d0a56bb4f0175a10a4164939def031e1e8ef70071a4f55dfc1561b89f3dcd9` |
| `npc_scholar/npc_scholar_basecolor.png` | 3349324 | `8acdf3b353064f137dd5890638075c472095b3e1db0d6a83d45092608e485cd0` |
| `npc_scholar/npc_scholar_rigged.fbx` | 806124 | `4a872ce3b3b489949e9d26b08b7253adf8d0aed604fce0721b50b01c0b04c6db` |
| `npc_villager_f/npc_villager_f_basecolor.png` | 3353070 | `22ae3f8903d014b0e632d15ec9268adc6dc9b3c08fa41e9ebeb748bcf8e66eba` |
| `npc_villager_f/npc_villager_f_rigged.fbx` | 813292 | `bc86ffc0627aebee12dead8b9130f6f0d7303b866cd592af7e7f62f9691f5628` |
| `npc_villager_m/npc_villager_m_basecolor.png` | 3351758 | `2545503c9b4c3e381b81c72bfd5d1c9d9c4565167cf980348aea2c891df0e089` |
| `npc_villager_m/npc_villager_m_rigged.fbx` | 579452 | `2bbec05f8521e999ebc8f2bd05e3f940b3155edb37158e797266044033267fd4` |

## Exact52 passing identities

- `Memoria.Audio.CatalogAssets`
- `Memoria.Audio.Routing`
- `Memoria.Audio.RunReplacement`
- `Memoria.Campaign.CanonicalPaidRoute`
- `Memoria.Campaign.FilteredOriginalChoice`
- `Memoria.Campaign.HostContinuation`
- `Memoria.Campaign.UnseenFieldRoute`
- `Memoria.ShopTransactions.Source.buy_both`
- `Memoria.ShopTransactions.Source.buy_exact`
- `Memoria.ShopTransactions.Source.buy_poor`
- `Memoria.ShopTransactions.Source.buy_sell_ko`
- `Memoria.ShopTransactions.Source.close`
- `Memoria.ShopTransactions.Source.exchange`
- `Memoria.ShopTransactions.Source.oath_en`
- `Memoria.ShopTransactions.Source.oath_ko`
- `Memoria.ShopTransactions.Source.sell_daily`
- `Memoria.ShopTransactions.Source.sell_identity`
- `Memoria.ShopTransactions.Source.sell_relation`
- `Memoria.ShopTransactions.Source.sell_sense`
- `Memoria.ShopTransactions.Canonical`
- `Memoria.ShopTransactions.Guards`
- `Memoria.ShopTransactions.RequestCancellation`
- `MemoriaVisual.Achievements`
- `MemoriaVisual.ArtworkCoverage`
- `MemoriaVisual.BeltDressing`
- `MemoriaVisual.Chapter1Journey`
- `MemoriaVisual.Chapter1Presentation`
- `MemoriaVisual.Chapter3`
- `MemoriaVisual.Chapter3Travel`
- `MemoriaVisual.Chapter4`
- `MemoriaVisual.Chapter5`
- `MemoriaVisual.ChapterContinue`
- `MemoriaVisual.ChapterRevisit`
- `MemoriaVisual.Codex`
- `MemoriaVisual.DialogueInteraction`
- `MemoriaVisual.DriftDressing`
- `MemoriaVisual.EliaSkills`
- `MemoriaVisual.FieldBurn`
- `MemoriaVisual.FieldCharacters`
- `MemoriaVisual.FieldCombat`
- `MemoriaVisual.FieldEncounters`
- `MemoriaVisual.FieldFeel`
- `MemoriaVisual.FieldFoes`
- `MemoriaVisual.FieldItems`
- `MemoriaVisual.FieldRewards`
- `MemoriaVisual.FoeVariety`
- `MemoriaVisual.GameOver`
- `MemoriaVisual.Journal`
- `MemoriaVisual.PauseMenu`
- `MemoriaVisual.TitleScreen`
- `MemoriaVisual.TutorialHints`
- `MemoriaVisual.VerdanExploration`
