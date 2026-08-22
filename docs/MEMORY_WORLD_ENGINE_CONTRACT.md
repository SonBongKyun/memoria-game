# Memory World Engine data contract

This contract covers the deterministic engine layer and one development-only
Malet dialogue vertical slice. Story scenes, SceneFlow, quests, and live NPC
content do not consume it yet.

## Persistent IDs

- Actor: `player.<slug>` or `npc.<slug>`
- Memory: `memory.<actor_slug>.<memory_slug>`
- Fact: `fact.<domain_slug>.<fact_slug>`
- Slugs begin with `a-z` and contain lowercase ASCII letters, digits, and
  single underscores between non-empty segments.
- Display names are metadata and never persistent identity.
- Actor slugs are unique across both actor namespaces because Memory IDs omit
  the actor namespace.

Current actors are `player.arrel` (`Arrel`), `npc.malet` (`Malet`), and
`npc.sable` (`Sable`). Their definitions live in
`data/world_state/actors.json` with catalog schema version `1`. The catalog is
the only bootstrap source; load or validation failures do not silently fall
back to actors embedded in code.

ActorRegistry owns identity syntax, registration, display metadata, and
collision rejection. WorldState owns only runtime state containers for actors
already registered in ActorRegistry.

Catalog replacement is atomic. Duplicate actor IDs, invalid IDs, empty display
names, and actor-slug collisions across namespaces reject the entire candidate
catalog and leave the previously loaded registry unchanged. Display names may
be duplicated because only `actor_id` is persistent identity.

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

## Structured dialogue conditions

DialogueManager evaluates Memory World Engine state only when a line or choice
contains a Dictionary-valued `condition` field. The existing
`requires_memory_intact`, `requires_memory_gone`, `requires_flag`,
`requires_not_flag`, `requires_weave`, and legacy
`requires_memory + burned_text` behavior remains unchanged. If structured and
legacy conditions coexist, both must pass.

Structured conditions support `all`, `any`, `not`, `knowledge`, and `memory`.
The optional `restored` Boolean on a memory condition reads the existing
deterministic `restored_revision > 0` audit metadata; it creates no new status
or state. Unsupported or malformed structured conditions fail closed.

The only current consumer data is
`data/development/malet_memory_world_dialogue.json`. Its three mutually
exclusive development branches are:

- `active`: knowledge true, memory active, `restored=false`;
- `removed`: knowledge true, memory tombstone removed;
- `restored`: knowledge true, memory active, `restored=true`.

No gameplay progression file references the development scene or dialogue.

## Committed event payload schema

The current committed event schema is **version 1**. Every
`world_event_committed` event contains exactly these common fields:

| Field | Type | Contract |
| --- | --- | --- |
| `schema_version` | int | Exactly `1` for the current envelope |
| `event_id` | String | `world.%08d` derived only from `event_sequence` |
| `event_type` | String | One of the five supported mutation types |
| `event_sequence` | int | Positive, monotonic WorldState sequence |
| `revision` | int | Positive WorldState revision created by the mutation |
| `actor_id` | String | Actor whose state changed |
| `target_id` | String | Memory ID or Fact ID changed by the event |
| `payload` | Dictionary | Exact type-specific payload described below |

Type-specific payloads:

| `event_type` | `target_id` | Exact payload |
| --- | --- | --- |
| `memory.added` | Memory ID | `status`, `fact_ids`, `source_actor_id` |
| `memory.removed` | Memory ID | `previous_status="active"`, `status="removed"` |
| `memory.restored` | Memory ID | `previous_status="removed"`, `status="active"`, `last_removed_revision` |
| `knowledge.learned` | Fact ID | `value=true` |
| `knowledge.forgotten` | Fact ID | `value=false` |

The schema rejects extra common or payload fields. Events contain no wall-clock
time, random values, or nonce. A successful mutation emits once; a no-op emits
nothing. Importing or restoring WorldState never re-emits historical events.

Consumers must validate `schema_version` before reading type-specific payloads.
A v1 consumer rejects unsupported future versions without changing gameplay
state. Additive or breaking envelope/payload changes require a new schema
version and either parallel validation or an explicit adapter; producers must
not silently reinterpret a v1 payload under newer semantics.

## Save compatibility

SaveManager remains at save version `0.4.0`; WorldState schema version is `1`.
A valid current snapshot is preserved. A legacy save with no WorldState, a
missing payload, a non-dictionary payload, or an unsupported schema recovers to
deterministic WorldState defaults. This recovery does not migrate or modify
legacy `story_flags` or the existing player-card `MemoryManager` data.

Migration fixtures live under `data/test_fixtures/save_migrations` and are read
from `res://`; tests never write to `user://saves/`.

## Save path isolation for smoke processes

Production continues to use `user://saves`. A smoke process is detected by the
explicit `--smoke-test` user argument or a direct `scripts/tools/smoke_*` scene
launch. In that mode SaveManager starts with no usable save root, disables
autosave, and does not create or probe the production directory.

A save-writing smoke must inject a process-specific descendant of
`user://test_tmp/smoke_saves` through `configure_test_save_root`. All
SaveManager slot operations then validate their normalized absolute target.
The shared `smoke_save_sandbox.gd` helper applies the same validation before a
test performs direct fixture writes. Missing injection, path traversal, the
production root, or any target outside the active test root logs the attempted
target and terminates the process with exit code `1` before filesystem access.

Test directories are intentionally safe to leave behind after a crash or
timeout; no cleanup path ever targets production saves.

The Malet development scene writes a normal save payload only after an isolated
root is configured. `reload_test_world_state` is smoke-only and restores just
the WorldState portion from that guarded slot; it does not import GameManager,
MemoryManager, SceneFlow, or change scenes.

The focused PowerShell suite also scans every `smoke_*.gd` source before launch
and rejects production save literals or direct FileAccess writes outside the
shared sandbox helper.

## Actor catalog export availability

The Windows export preset uses `export_filter="all_resources"` and also names
`data/world_state/actors.json` in `include_filter`. The explicit include keeps
the catalog packaged even though ActorRegistry opens it through a runtime
string path rather than a preloaded Resource dependency.

The official smoke entrypoint exports a temporary PCK, launches that PCK with
`--smoke-test`, and runs the actor catalog smoke against the real
`res://data/world_state/actors.json` path. The gate fails if export fails, the
file is missing from the pack, ActorRegistry cannot parse it, or the runtime
catalog differs from the three expected actors. The temporary PCK is removed only
after its normalized path is verified beneath the OS temp directory.

## Read-only consumer probe

`world_event_consumer_probe.gd` is development-only and is not an autoload or a
gameplay consumer. It subscribes to `world_event_committed`, validates schema
v1, checks event sequence order and actor/target identity, reads an immutable
payload copy, and records diagnostics. It has no mutation API.

Its smoke compares an identical five-mutation run with and without the probe.
The final WorldState snapshots must match, while MemoryManager and legacy
story_flags remain unchanged. Each of the five v1 event types must be observed
exactly once.

## Smoke contract

Memory World Engine smoke scenes use `smoke_test_runner.gd`. Each failure logs
the suite name, test name, and reason, then the process exits with code `1`.
Only a failure-free suite prints its PASS marker.

Run the focused suite with:

```powershell
./scripts/tools/run_memory_world_engine_smoke_suite.ps1 -GodotPath <godot-console-exe>
```

This is the single official Memory World Engine smoke entrypoint. It always
enables `--smoke-test`, production save guards, the isolated write sandbox,
fatal-output scanning, actor catalog export/runtime verification, schema v1
coverage, the read-only consumer probe, and the development-only Malet dialogue
vertical slice.
