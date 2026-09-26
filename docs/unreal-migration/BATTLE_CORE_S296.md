# S296 — playable battle core and Git recovery (2026-09-26)

## Scope and ownership

User authorized items 1–4: repair Git object access, implement the bounded source-derived battle core, connect presentation/audio, and validate the integrated Verdan journey. Codex owns this change in `codex/unreal-battle-core-20260926`, based on `4adbbfacbefc007747a4d4c6e2392a17018b5f3b`. Claude and foundation checkouts were not modified; the original Godot content remains unchanged. No push, release or foundation integration.

## Git recovery

The shared OneDrive Git store contained 1,342 recall-on-data-access loose-object placeholders. Reading a sampled object returned Windows cloud timeout 426; Git reported `mmap failed: Invalid argument`. All five present pack files passed `verify-pack`. A separate bare remote recovery fetch did not contain these loose-object IDs.

The placeholders were moved, without deletion or hydration, from `.git/objects/<prefix>/<suffix>` to `.git/memoria-repair-20260926/cloud-objects/<prefix>/<suffix>` on the same OneDrive volume. The orphan `pack-335650b8da73230e9b21e96d4136bf76f1474b21.idx` and `.rev`, whose pack was absent, were preserved outside the object database. After quarantine, both `git fsck --connectivity-only --no-reflogs` and `git fsck --full --no-dangling` returned 0. All pre-existing refs remained identical; only the task branch was added. No Git configuration was changed.

Local evidence and orphan-index backup: `C:/Users/jc/Documents/Codex/MEMORIA-Unreal-Collaboration/git-repair-20260926/`. It includes `refs-before.txt`, `quarantined-cloud-objects.json`, `fsck-full.txt` and the isolated `remote.git` fetch. Rollback consists of moving those preserved objects and two orphan files back to their original paths after checking for collisions; this would restore the original failing cloud placeholders. Nothing was deleted.

Automatic approval review rejected the optional geometric repack because rewriting the shared pack/index database broadly increases recovery risk. It was not retried. Read-only full integrity verification already passed. The task checkpoint disables automatic maintenance for that command only to avoid triggering the rejected repack.

## Runtime

`FMemoriaBattleModel` is a synchronous value model with injected RNG and no world, UObject, timer or audio dependency. It still uses Unreal core containers and the existing run value type. The battle subsystem computes a copied result and commits only while the same run/world/revision owns it. Run replacement, including same-ID restore, and world cleanup cancel pending actions. The existing Run and Memory domains remain authoritative; a burn goes through `BurnMemory` exactly once.

Implemented: physical combo, all five burn grades and chain burn, effective erosion power and unlocked burn passives, guard focus, potion/antidote/firebomb, seven enemy abilities, poison/weaken/burn, shield/reflect/charge, BREAK, momentum, Limit accumulation, aftershock, Last Stand, bounded corruption modifiers, five supported objectives, win/loss, source healing/grains/drop rewards. Actual source battle-grade and directive-streak bonuses are included even though the draft omitted them. Unsupported objectives are explicitly shown without rewards.

Presentation: keyboard/mouse action selection, source archive titles/colors/states in the burn list, unavailable faded/collateral/burned rows, item quantities, damage/healing numbers, target squash, visual hit pause, damage-scaled shake/flash and a 200+ zoom punch. Burn announces its lasting cost. Source combat cues and low-HP heartbeat are connected; battle/field music follows the existing audio owner. The entry-only development note has been removed.

Source/presentation differences: enemy responses preserve the 0.8-second wait plus 0.2-second normal / 0.5-second ability anticipation. A private value result is committed after anticipation; burn aftershock hides enemy intent. Visual timing does not pause or drive domain progression. There is no party action, Witness action, Limit spending, echo execution, stance system, boss system or auto-battle in this slice. The defeat panel provides checkpoint restore or explicit full-HP Verdan recovery that retains burned memories; this is the approved slice fallback, not a port of the entire Godot Game Over menu. Victory confirmation returns to the real Verdan revisit route.

## Source contract and verification

`export_battle_core_oracle.py` runs the lane's pinned Godot battle methods in an isolated harness, redirects RNG, and emits source catalog/expected states. The original Godot checkout is older and is not substituted for these source scripts. Fixtures include complete recorded RNG bounds/results, statuses, damage events, inventory, burned history, gauges, objective outcomes and rewards. Generated text/catalog output is checked for reproducibility. A scoped `.gitattributes` LF rule keeps the three new JSON fixtures byte-stable across Windows checkouts; staged bytes match all three validated files. The prior attributes file is backed up at `Saved/Validation/s296-full/gitattributes.before`; removing that single rule rolls it back. Excluded companion/echo/profile/presentation hooks are inert harness adapters and are documented in the exporter; their behavior is not claimed as migrated.

Fresh UE 5.8.2 build and rendered `Memoria.BattleCore.`: **76/76 PASS** (74 paired source cases + ownership cancellation + integrated journey). Source oracle rerun with `--check`: **74/74 PASS**. Python audio-source tests: **5/5 PASS**; battle-entry source invariants: **6/6 PASS**. Full UE 5.8.2 build + rendered `Memoria.` registry: **366/366 PASS**, 0 failures/fatal diagnostics, one existing engine render-thread console-variable warning. Automation duration: 1,820.52 seconds. Full report: `Unreal/Memoria/Saved/Validation/unreal-run-20260926T014420955867/Automation-20260926T014425179278/index.json`. Separate-process checkpoint tests, packaged builds and human listening were not rerun in this task.

Exact commands:

```powershell
python Unreal/Tools/export_battle_core_oracle.py --godot C:/Users/jc/Downloads/Godot_v4.6.2-stable_win64.exe/Godot_v4.6.2-stable_win64_console.exe --evidence-dir Unreal/Memoria/Saved/Validation/s296-oracle-check --check
python Unreal/Tools/validate_unreal.py --build-and-test --rendered --test-prefix Memoria.BattleCore. --evidence-dir Unreal/Memoria/Saved/Validation/s296-core-05
python Unreal/Tools/validate_unreal.py --build-and-test --rendered --test-prefix Memoria. --automation-timeout 5400 --evidence-dir Unreal/Memoria/Saved/Validation/s296-full
```

Focused report: `Unreal/Memoria/Saved/Validation/unreal-run-20260926T013949622825/Automation-20260926T014138164234/index.json`.

Earlier failed attempts remain local: initial compile fixes, the JSON fixture owner-lifetime crash, a validation run started before the extra 16 cases were registered, and the turn-limit/Last Stand mismatch. No expected values or numerical tolerances were relaxed. The mismatch was corrected in native rules to preserve the executed source. The existing engine render-thread console-variable warning remains visible.

The rendered journey uses real new-game dialogue, Malet trade, closed checkpoint, native travel, walking, battle input, victory, field return and Tab archive. It then explicitly restores a near-death fixture for defeat/Last Stand and Verdan recovery. The large-impact screenshot is also explicitly a presentation-only 200-damage fixture and never mutates the actual fight. That second segment is a declared boundary fixture, not a claim that normal play starts with 1 HP. Captures live under `Unreal/Memoria/Saved/Validation/Phase1O/ShopBattleCombat_*` and are local-only. Numerical source cases use clearly declared synthetic enemy HP when needed to reach high-grade, multi-turn and boundary branches.

The draft suggests 3–5 turns against Alley Rat, but the executed chapter-3 source fixture wins with two attacks (enemy HP 35 → 9 → 0). These source values were preserved rather than silently rebalanced; the 3–5-turn target is not claimed as met.

Human feel/listening remains unverified. Automated physical-key play and screenshot review establish routing and presentation, not subjective balance or audio quality. The existing 15 unrelated historical Python host-suite failures from S295 remain outside this task; they are not being reported as fixed.
