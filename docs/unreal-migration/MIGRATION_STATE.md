# Migration handoff — Phase 1B

Status: **PARTIAL — UE 5.7 remains unavailable.** Phase 0/1A source work has a local checkpoint; no UBT/UHT/link/editor/Unreal Automation or visual success has been demonstrated. Do not advance to content migration yet.

- Branch/worktree: `unreal-migration/foundation`, `C:\Users\jc\MemoriaMigration\foundation`.
- Project: `Unreal/Memoria/Memoria.uproject`, target **5.7**.
- Phase 1A checkpoint: `2b2a2607296faa1de2f4e8bf94f4eec847bfc8f9` — `chore(unreal): checkpoint Phase 1A foundation`.
- Phase 1B checkpoint: `d77c566a141441ac8f935871a7756467eb9373de` — `chore(unreal): harden validation and record UE5.7 blocker`; see [PHASE_1B_REPORT](PHASE_1B_REPORT.md). A documentation-only follow-up records the SHA. No push.
- Original dirty Godot checkout remains at `C:\Users\jc\OneDrive\바탕 화면\메모리아\Game`; migration stays in this worktree.

Completed: reviewed the unchanged 67-file Phase 1A inventory and all 88 staged paths; reran 47 static/preservation checks and 51 native memory cases before checkpointing. Fixed the UE validator's count-only acceptance (now requires all 54 distinct expected identities), added fixture freshness verification before UBT, and isolated new evidence destinations from historical Phase 1A records. Pinned the attested input JSON to CRLF and Godot output JSON to LF so Git checkout preserves their exact hashes; expected values and production C++ were not changed. Host-side validator tests pass 10/10, including both Git autocrlf modes.

Godot regression: repository contract, VN 21 files/526 steps, Korean 32 files/1,581 fields, editor import, and official 15-case memory/world suite pass. Exported actor catalog passes with `export_log_errors=0` in this run. Full subprocess app-data is isolated; editor-rewritten metadata is restored. See [Phase 1B evidence](evidence/phase1b/).

Engine inspection: only `C:\Program Files\Epic Games\UE_5.8\Engine\Build\Build.version` reports an installed engine, **5.8.2 / CL 56702186**. Exact `UE_5.7` path, Launcher, registry, environment roots and additional common installation parents did not reveal 5.7. [engine_locations.json](evidence/phase1b/engine_locations.json) lists every checked path; [ue57_validation.json](evidence/phase1b/ue57_validation.json) records the blocked state. No engine was installed or substituted.

Unfinished Phase 1B: real UE5.7 build/UHT/link; all 54 Automation cases; explicit GC/lifetime and complete nondefault SaveGame round trips; actual `L_FoundationTest` map; real Enhanced Input assets; one UMG modal; numeric plane/camera/sprite-foot-pivot/coordinate restore checks; press/hold/release Back consumption and focus return. No `.umap`, input `.uasset`, modal asset or visual proof has been fabricated.

Exact next action: use a real **UE 5.7** root whose `Build.version` matches, then run the command below. Resolve compiler/test failures and extend the existing foundation tests for GC and full save-field coverage before creating the isolated test map/input/modal assets through Unreal Editor tooling. Preserve the existing 51 oracle expectations and the two narrative dialects.

```powershell
python Unreal/Tools/validate_ue57.py --engine-root '<verified UE_5.7-root>' --build-and-test --evidence-dir docs/unreal-migration/evidence/phase1b
python Unreal/Tools/validate_native_memory.py --evidence-dir docs/unreal-migration/evidence/phase1b
python -m unittest discover -s Unreal/Tools -p test_validation_tools.py -v
python Unreal/Tools/validate_foundation.py --original-manifest 'C:\Users\jc\MemoriaMigration\phase1a-original-manifest.json' --evidence-dir docs/unreal-migration/evidence/phase1b
python Unreal/Tools/validate_godot_baseline.py --godot 'C:\Users\jc\Downloads\Godot_v4.6.2-stable_win64.exe\Godot_v4.6.2-stable_win64_console.exe' --evidence-dir docs/unreal-migration/evidence/phase1b
```

Without `--evidence-dir`, validators use a timestamped `Unreal/Memoria/Saved/Validation` directory. They reject historical `evidence/phase1a`. Earlier reports remain immutable.

Phase 1C is gated by full Phase 1B acceptance. Its first bounded target should be deterministic import of the actual starting-memory catalog into versioned IR and a typed Unreal catalog asset, with repeated-import ID/order/content equivalence and original-source hash checks. Use that accepted import as preparation for the contract's Verdan arrival/trade slice; do not begin Chapter 1 reconstruction, BattleManager or bulk assets now.

Keep these contracts: raw grades 0..4 never reverse; residue can precede history; player memory is separate from actor cognition; field and VN effect/payment/index order differs; UE save schema 1 does not imply Godot 0.4.0 compatibility; no live battle serialization; no generic removal for burn/sale/synthesis; `(x,y)` -> `(x,-y,0)` is a coordinate contract pending real-editor verification.
