# Migration handoff — UE 5.8.2 foundation

Status: **Phase 1B COMPLETE on UE 5.8.2**. On 2026-09-06 the user intentionally changed the target **UE 5.7 -> UE 5.8.2**: the former baseline was not a product constraint, 5.8.2 is already installed, and no engine-bound migration assets had yet been produced. Historical Phase 0/1A/1B reports retain the original 5.7 target.

- Branch: `unreal-migration/ue58-foundation`.
- Worktree: `C:\Users\jc\MemoriaMigration\foundation`.
- Starting clean HEAD: `a58b1e15fe2fb6c86568b8df45d279c4f0844ca1`.
- Previous Phase 1B checkpoint: `d77c566a141441ac8f935871a7756467eb9373de`.
- Engine root read directly: `C:\Program Files\Epic Games\UE_5.8`.
- Verified Build.version: **5.8.2 / CL 56702186**, compatible CL 55116800.
- New technical checkpoint: **`44d08dde96b352c3f86b770ed11948bcc18f6bb6`** — `feat(unreal): validate foundation on UE 5.8.2`; followed by a documentation-only commit recording this SHA.
- Real UBT/UHT/compile/link and rendered Editor/PIE pass. Exact legacy suite **54/54**, total **57/57**, runtime GC ownership, nondefault SaveGame schema 1/slots 0-3, map/input/modal, XY/Z/camera/sprite/foot/coordinate restore and Back single consumption all pass.
- Godot official **15/15**, native **51/51**, host **12/12**, static **49/49**, original **4,217 files preserved**. Final acceptance: [acceptance.json](evidence/phase1b-ue58/acceptance.json).
- Actual test map: `/Game/Tests/Foundation/L_FoundationTest`. The same directory holds four Input Actions, two Mapping Contexts, texture/sprite and `WBP_FoundationModal`. Runtime input uses simulated keyboard/gamepad events through real Enhanced Input; hardware and campaign certification are not claimed.
- Current report: [PHASE_1B_UE58_REPORT](PHASE_1B_UE58_REPORT.md). All new evidence is under `evidence/phase1b-ue58`; historical evidence is immutable. Failed attempts and the one nonblocking engine render-thread warning are retained.
- No engine installation and no push. Original Godot checkout remains untouched.

## Exact next task — Phase 1C, not started

Deterministically export the actual starting-memory catalog into versioned IR, import it into a typed Unreal catalog asset, and validate repeated-import equivalence for IDs, owned order and semantic content against source hashes. This prepares the established Verdan arrival/trade slice. No catalog content asset, Chapter 1, campaign dialogue, Verdan, BattleManager or bulk art/audio migration was implemented in Phase 1B.

Build and regression commands are in the [current report](PHASE_1B_UE58_REPORT.md#commands-and-next-scope) and [Unreal README](../../Unreal/Memoria/README.md). Normal engine runs load committed test assets; `--create-foundation-assets` is only for an empty test-content directory. No central Phase 1B blocker remains.
