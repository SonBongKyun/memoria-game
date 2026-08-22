# Memory World Engine data contract

This contract covers the deterministic engine layer only. Story scenes,
DialogueManager, SceneFlow, quests, and live NPC content do not consume it yet.

## Persistent IDs

- Actor: `player.<slug>` or `npc.<slug>`
- Memory: `memory.<actor_slug>.<memory_slug>`
- Fact: `fact.<domain_slug>.<fact_slug>`
- Slugs begin with `a-z` and contain lowercase ASCII letters, digits, and
  single underscores between non-empty segments.
- Display names are metadata and never persistent identity.
- Actor slugs are unique across both actor namespaces because Memory IDs omit
  the actor namespace.

Current actors are `player.arrel` (`Arrel`) and `npc.malet` (`Malet`).

ActorRegistry owns identity syntax, registration, display metadata, and
collision rejection. WorldState owns only runtime state containers for actors
already registered in ActorRegistry.

## Knowledge

`learn_fact(actor_id, fact_id)` stores an explicit `value=true` knowledge
record. `forget_fact(actor_id, fact_id)` stores `value=false`; it does not delete
the record. This distinguishes an actor that explicitly forgot a fact from a
state with no fact record.

Knowledge and memory are independent. Removing or restoring a memory never
learns or forgets any linked fact automatically.

Successful knowledge mutations increment WorldState revision and commit one
`knowledge.learned` or `knowledge.forgotten` event. Repeating an already true or
false operation returns `false`, changes no state, and commits no event.

## Memory removal and restoration

`remove_memory` retains the complete record with `status="removed"` and records
`removed_revision`. It never deletes the memory dictionary entry.

`restore_memory` is valid only for a removed tombstone. For this phase,
"restore" means restoring the most recent valid state held immediately before
the latest removal:

- `id`, `owner_actor_id`, `content`, `fact_ids`, `source_actor_id`, and
  `created_revision` remain unchanged;
- `status` becomes `active`;
- the tombstone's `removed_revision` moves to `last_removed_revision`;
- `removed_revision` becomes `0`;
- `restored_revision` records the new WorldState revision.

A successful restore commits exactly one `memory.restored` event. Restoring an
active or missing memory returns `false`, changes no state, and commits no
event.

## Save compatibility

SaveManager remains at save version `0.4.0`; WorldState schema version is `1`.
A valid current snapshot is preserved. A legacy save with no WorldState, a
missing payload, a non-dictionary payload, or an unsupported schema recovers to
deterministic WorldState defaults. This recovery does not migrate or modify
legacy `story_flags` or the existing player-card `MemoryManager` data.

Migration fixtures live under `data/test_fixtures/save_migrations` and are read
from `res://`; tests never write to `user://saves/`.

## Smoke contract

Memory World Engine smoke scenes use `smoke_test_runner.gd`. Each failure logs
the suite name, test name, and reason, then the process exits with code `1`.
Only a failure-free suite prints its PASS marker.

Run the focused suite with:

```powershell
./scripts/tools/run_memory_world_engine_smoke_suite.ps1 -GodotPath <godot-console-exe>
```
