# MEMORIA Canon v2 Stabilization

This document is the implementation contract for the Steam-demo stabilization pass.

## Product focus

MEMORIA is not differentiated by the number of JRPG subsystems. Its identity is the irreversible relationship between power and memory loss.

For the current demo pass:

1. Do not add new combat subsystems unless they directly deepen Memory Burn.
2. Prefer consequences, callbacks, and replay variation over additional menus or currencies.
3. Every major burn should eventually have three layers when practical:
   - immediate combat benefit,
   - a later visible absence or altered interaction,
   - a downstream story or ending consequence.
4. The intended demo question is: **"Would that scene have been different if I had kept that memory?"**

## Canon authority

The current Notion Master Bible v9.0 and completed manuscript are the narrative source of truth. Older GDD numbering is legacy implementation data where it conflicts with current canon.

### Memory grade canon

Narrative canon rises from 1 to 5:

| Canon grade | Name | Meaning |
| --- | --- | --- |
| 1 | Ember | small sensory fragment |
| 2 | Flame | daily / ordinary memory |
| 3 | Fire | relational memory |
| 4 | Sun | identity-defining memory |
| 5 | Zero | core / existential memory |

The current runtime `MemoryManager.MemoryGrade` enum is **not safe to reorder directly**. Existing save data persists the enum as a raw integer using the legacy inverse labels (`GRADE_5` at rank 0 through `GRADE_1` at rank 4).

Until a dedicated migration is shipped:

- treat the enum value as a storage/power rank, not canonical display numbering;
- do not reorder enum members;
- do not reinterpret old save integers in place;
- migrate user-facing labels separately through an explicit compatibility layer;
- bump `SAVE_VERSION` when persisted grade semantics change;
- add a migration test before accepting old saves under new grade semantics.

`scripts/validation/repo_contract.py` guards the raw enum order so an agent cannot casually break existing saves.

## Vertical slice target

The first public demo should prove the core loop before it proves content volume.

Recommended slice:

1. Rim Forest introduction.
2. First dangerous encounter and first meaningful Memory Burn.
3. Elia interaction that establishes emotional attachment.
4. Verdan / memory-trade context.
5. A second combat encounter that tempts a more valuable burn.
6. A visible World Rewrite caused by an earlier choice.
7. Authority / Kairos pressure.
8. BL-07 tease and demo end.

The slice should be judged by:

- first 10-minute clarity,
- first 30-minute retention,
- whether the first burn feels costly rather than merely optimal,
- whether Elia is emotionally legible,
- whether a previous burn is recognized later without tutorial explanation,
- whether the player wants to replay to preserve a different memory.

## Systems to protect

High-value identity systems:

- Memory Burn
- World Rewrite
- Residue / Elia anchor consequences
- burn-dependent callbacks
- memory-dependent dialogue
- strength gained through irreversible loss

Systems that may be simplified before adding more complexity:

- combo layers
- redundant status effects
- excess stances
- low-value recurring encounters
- crafting that turns memories into generic materials
- UI surfaces that expose optimization math instead of emotional tradeoffs

## Save compatibility

Saves are a release boundary. Any change to persisted semantic meaning must include:

1. `SAVE_VERSION` bump,
2. explicit migration from the previous version,
3. missing-scene / malformed-data handling,
4. a test using at least one representative old save,
5. verification that burned / residue / faded states survive the migration.

## Repository policy

- Generated exports (`build/`, `.exe`, `.pck`) stay out of source control.
- Do **not** enable broad Git LFS tracking over the existing media tree until a dedicated migration/renormalization pass is performed. The repository already contains a large body of binary assets committed as normal Git blobs; simply adding LFS attributes can create pointer and dirty-working-tree problems.
- When LFS migration is scheduled, measure the largest file classes first, choose whether to migrate current state only or rewrite history, and test on a disposable clone before force-updating shared refs.
- Version metadata in `project.godot`, `export_presets.cfg`, and demo artifact names must remain synchronized.
- Every pull request should pass the repository contract check and Godot headless import.
- Main-branch builds should produce a Windows demo artifact and publish the current demo prerelease.

## Definition of done for the stabilization pass

- Canon conflicts are documented and guarded from accidental regression.
- Release version metadata is synchronized.
- Generated builds are excluded from Git.
- LFS migration is deferred until it can be performed safely instead of partially.
- Godot imports headlessly in CI.
- Windows demo exports in CI.
- Main builds publish a GitHub demo prerelease.
- No new gameplay subsystem is introduced merely to increase feature count.
- Remaining gameplay work is prioritized around Memory Burn consequence density and vertical-slice polish.
