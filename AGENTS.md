# MEMORIA: The Price of Oblivion — Agent Contract

This repository is in **stabilization / vertical-slice mode**. The goal is not to increase feature count. The goal is to make the existing game feel unmistakably like MEMORIA and prepare it for a reliable Steam demo.

## Source-of-truth order

When sources disagree, follow this order:

1. The user's explicit current instruction.
2. Current Notion canon, especially **MEMORIA Master Bible v9.0** and the completed Ch01–108 manuscript.
3. `docs/CANON_GAMEPLAY_V2.md` in this repository.
4. Current implemented behavior plus regression checks.
5. Old GDDs, session notes, and archived design documents only as historical reference.

Never silently revive an older rule because it already exists in code.

## Product thesis

MEMORIA is a dark-fantasy narrative RPG where **memories are the price of power**.

The signature loop is:

`threat → temptation to burn a memory → immediate power → visible absence → delayed callback → relationship/world/ending consequence`

A player should remember the memory they chose to lose, not the number of combat subsystems available.

## Stabilization rules

### 1. Do not add new generic RPG systems

During stabilization, do not add new:

- crafting systems
- currencies
- status families
- stance families
- skill trees
- minigames
- collection layers
- combat meters
- menus

unless the user explicitly asks for one.

Prefer deleting, combining, clarifying, or deepening existing systems.

### 2. Strengthen memory consequences first

For important burnable memories, prefer three layers of consequence:

1. **Immediate cue**: animation, line, changed interaction, or readable world effect.
2. **Revisit**: the absence returns later in the same route/chapter.
3. **Long consequence**: dialogue, relationship, route, boss, or ending changes.

Do not turn these into checklist UI. The player should discover the cost through play.

### 3. Preserve save compatibility

The historical implementation stores memory strength as an **internal rank 0..4**. Existing saves persist that raw integer.

Current canon presents grades as:

- 1 — Ember / 잔불
- 2 — Flame / 불꽃
- 3 — Blaze / 화염
- 4 — Sun / 태양
- 5 — Zero / 영점

Therefore:

- internal `0` maps to canonical `1`
- internal `1` maps to canonical `2`
- internal `2` maps to canonical `3`
- internal `3` maps to canonical `4`
- internal `4` maps to canonical `5`

**Never reverse the persisted enum values in-place.** Use an explicit mapping/codec at UI and serialization boundaries. Any future persisted schema change requires a save-version bump and migration.

See `docs/CANON_GAMEPLAY_V2.md`.

### 4. BattleManager is frozen against growth

`BattleManager` is already a large integration point. Do not make it larger unless unavoidable.

When touching combat code, prefer small extractions toward the seams documented in `docs/ARCHITECTURE_STABILIZATION.md`. Do not perform a wholesale rewrite during stabilization.

### 5. Steam-demo vertical slice is the quality bar

The first 30–60 minutes must answer these questions:

- Is the first memory burn understandable without a lore lecture?
- Does the player feel tempted to sacrifice a stronger memory?
- Does a burned memory visibly matter later?
- Does Elia feel like a person worth protecting/remembering?
- Does the player want to see what BL-07 is?
- Is any stretch of play boring for more than several minutes?

If a change does not improve one of these, it is probably not a stabilization priority.

## Technical guardrails

- Godot 4.6.x / GDScript.
- Preserve current save/load recovery behavior.
- Never commit credentials, Steam secrets, signing keys, or private API keys.
- Keep release credentials in GitHub/Steam secrets, never repository files.
- New large binary assets must use Git LFS rules in `.gitattributes`.
- Avoid history rewrites for existing binary assets during ordinary feature work.
- Keep developer validation from overwriting player saves.
- Validate scene paths before loading mutable game state.
- Prefer deterministic content/data over hidden runtime magic.
- Reuse existing art and systems before adding variants.

## Before merging

At minimum:

1. Godot headless import/parse succeeds.
2. Windows demo export succeeds when CI is available.
3. Save/load behavior affected by the change is checked.
4. First-burn and world-rewrite paths are checked when touched.
5. No new secret or credential is committed.
6. Canon terminology matches `docs/CANON_GAMEPLAY_V2.md`.

## Current focus

The repository already contains many systems. **Polish, consequence, readability, stability, and release discipline now beat feature count.**

For historical session-by-session implementation notes, consult `SESSION_LOG.md`; do not treat it as current canon.
