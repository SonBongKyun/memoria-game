# MEMORIA Script Agent Rules

These instructions apply to changes under `scripts/` and take precedence over older project notes when they conflict.

Read `../docs/CANON_V2_STABILIZATION.md` before modifying memory, battle, save, story, or release behavior.

## Current phase

This repository is in Steam-demo stabilization, not feature expansion.

Do not add a new gameplay subsystem unless the user explicitly requests one. Prefer fixing, simplifying, connecting, testing, and polishing what already exists.

## Protect the core loop

The product identity is:

Memory Burn → immediate power → irreversible loss → later visible consequence.

Protect:

- `MemoryManager` burn/residue/faded state
- `WorldRewriteDirector` callbacks and absence manifestations
- Elia/relationship consequences
- memory-dependent dialogue
- ending paths based on what the player preserved or lost

Do not turn memories into generic crafting currency or optimization counters without narrative cost.

## Canon grade safety

Current canon is 1 Ember → 5 Zero, increasing in narrative power.

The existing `MemoryManager.MemoryGrade` enum is legacy persisted storage order. Existing saves store the raw integer. **Do not reorder that enum directly.**

If grade persistence changes:

1. bump `SAVE_VERSION`,
2. implement explicit migration,
3. test an old save,
4. verify burned/residue/faded states,
5. migrate player-facing labels separately.

Run `python3 scripts/validation/repo_contract.py` before commit.

## BattleManager

Do not keep adding unrelated responsibilities to `BattleManager`.

If refactoring it, extract one boundary at a time and preserve behavior. No wholesale rewrite.

Preferred future boundaries:

- encounter/static data
- damage/status resolver
- presentation helpers
- reward handling
- memory/combat bridge

## SaveManager

Save regressions are release blockers. Never silently reinterpret persisted values. Preserve scene validation, backup recovery, player position, story flags, memory state, and VN resume state.

## Validation

Before merging script work:

- repository contract check passes
- Godot 4.6.2 headless import passes
- save/load behavior is covered when touched
- no new system was added just to increase feature count
