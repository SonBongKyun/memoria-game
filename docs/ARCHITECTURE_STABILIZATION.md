# Architecture Stabilization Plan

MEMORIA has reached the point where protecting working behavior is more valuable than rewriting for theoretical cleanliness. This document defines extraction seams for future work without asking for a big-bang refactor.

## BattleManager

`BattleManager` currently coordinates state, enemies, damage, status effects, bosses, allies, combo, break, witness, tactical objectives, momentum, stance, memory echoes, pacing, and presentation hooks.

Do **not** rewrite it wholesale.

When a future change naturally touches one of these responsibilities, prefer extracting toward the following boundaries.

### 1. CombatResolver
Pure calculations and rules:
- damage
- weakness/resistance
- status resolution
- break/momentum arithmetic

Goal: deterministic functions that can be checked without scene/UI state.

### 2. EncounterData
Static/serializable encounter configuration:
- enemy stats
- boss flags/phases
- abilities
- environment modifiers
- art identifiers

Goal: stop growing hard-coded condition tables inside the manager.

### 3. BattleStateMachine
Turn/state transitions only:
- start/end
- player turn
- enemy turn
- victory/defeat/flee
- phase transition coordination

Goal: make illegal transitions difficult.

### 4. MemoryCombatBridge
The MEMORIA-specific seam:
- available burn options
- burn power/elements
- residue rules needed by combat
- immediate burn consequence events
- handoff to `WorldRewriteDirector`

Goal: make the signature mechanic explicit instead of burying it among generic combat rules.

### 5. BattlePresentation
Presentation only:
- VFX
- camera shake
- cut-ins
- UI text
- pacing delays
- audio cues

Goal: gameplay state must not depend on whether an animation completed successfully.

## Autoload discipline

Do not convert every new helper into an Autoload. Prefer:
- `class_name` utility/data classes
- scene-local controllers
- resources for data
- injected references where practical

An Autoload is appropriate only when lifecycle truly is global and persistent.

## Save boundaries

Anything written into save data is an API.

Rules:
- bump `SAVE_VERSION` for semantic schema changes
- preserve an explicit schema marker when stored values can change meaning
- migrate before importing mutable runtime state
- validate destination resources before applying the save
- never allow developer smoke/capture scenes to overwrite player autosaves

## Refactor trigger

Extract only when at least one is true:
- the same logic must be reused in two places
- a bug is caused by mixed responsibilities
- a unit/regression check cannot be written without the extraction
- a new change would make `BattleManager` materially larger

Do not refactor merely to create more files.

## Release priority

Before the first Steam demo, prioritize:
1. stable save/load
2. deterministic first hour
3. controller/keyboard navigation
4. clear Memory Burn UX
5. world-rewrite callbacks
6. export reproducibility
7. crash/softlock prevention

Large architecture work that does not improve one of these should wait.
