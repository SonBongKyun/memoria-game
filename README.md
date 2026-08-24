# MEMORIA: The Price of Oblivion

Dark-fantasy 2D narrative RPG built with Godot 4.6.2 and GDScript.

> **Canon notice:** active canonical development currently lives on `canon-rebuild-season1`. Read [`docs/CANON_STATUS.md`](docs/CANON_STATUS.md) before changing story, dialogue, progression, endings, or Memory/Knowledge logic. The public `main` branch still reflects an older pre-rebuild line.

## Current development status

- Season 1 canon: 46 chapters
- Canonical rebuild checkpoint: through Chapter 5, `The Classifier`
- Next gameplay migration: Wave 2B, Chapter 6
- Gameplay migration is intentionally paused while Season 2 is being written
- Canonical development branch: `canon-rebuild-season1`
- Current canonical checkpoint: `84684ae` (`feat(story): implement canonical chapter 5 classifier`)

## Canonical references

Use this order for narrative decisions:

1. Latest completed English Season 1 manuscript, Chapters 01–46
2. `docs/SEASON1_GAME_PROGRESSION.md`
3. `docs/SEASON1_MEMORY_MAP.md`
4. Current canonical runtime implementation
5. Legacy project docs only as historical/reference material

The English Season 1 manuscript remains authoritative for canon decisions. Translations may assist localization but do not override it.

## Important project files

- `AGENTS.md` / `CLAUDE.md`: development-agent guidance aligned to the 46-chapter canon
- `SESSION_LOG.md`: historical development log
- `docs/CANON_STATUS.md`: concise canon guardrails and current checkpoint
- `docs/SEASON1_GAME_PROGRESSION.md`: current Season 1 game-progression rebuild map
- `docs/SEASON1_MEMORY_MAP.md`: Memory/Knowledge candidates, ripples, and callbacks

## Legacy warning

Older docs and runtime code may still describe superseded material. Do not restore those assumptions solely because an old file still references them. In particular:

- Tobias does not join in the early Chapter 3 route
- Sable is not a Cleaner
- the 12-person memorial and old Malet → Sable note ripple are non-canon
- old 7-way ending logic is not the current canon ending structure
- Chapter 43 Name Burn is a required canon story event

Repository synchronization and agent-guidance cleanup are tracked in GitHub issue #2.
