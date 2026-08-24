# MEMORIA Canon Status

> **Important:** this repository's current `main` branch predates the latest local Season 1 canon rebuild. Do not treat old game chapter numbers or legacy story documents on `main` as narrative source of truth.

## Narrative source of truth

Until the canonical rebuild is fully pushed, use this priority order:

1. Latest completed English Season 1 manuscript, Chapters 01–46
2. `docs/SEASON1_GAME_PROGRESSION.md` on the canonical rebuild branch once pushed
3. `docs/SEASON1_MEMORY_MAP.md` on the canonical rebuild branch once pushed
4. Canonical runtime implementation on that branch
5. Legacy project documentation only as historical/reference material

The Korean manuscript/translation is still incomplete and must not override the completed English Season 1 manuscript for canon decisions.

## Confirmed Season 1 canon guardrails

- Season 1 consists of **46 chapters**.
- **Sable is not a Cleaner.**
- Sable retains the fact that 17 people who went after BL-07 never returned, while the personal memory that **Vessa was her daughter** may be removed/restored by Memory gameplay.
- The old Cleaner backstory, child/parent erasure order, 12-person memorial, and the old Malet → Sable note ripple are non-canon.
- Malet's BL-07 state separates event knowledge from requester identity:
  - knowledge: `fact.bl07.route_request_received`
  - identity/source memory: `memory.malet.bl07_request_source`
- Tobias does **not** join the party in the early Chapter 3 route. His canonical placement is later in the rebuilt Season 1 progression.
- Chapter 43 Name Burn is a required canon story event, not the old optional multi-ending branch.
- Legacy 7-way ending logic on old `main` must not be used to infer the final canon ending structure.

## Canonical rebuild checkpoint status

Local checkpoints reported before repository sync:

- `2eb5d33` — Sable/Vessa canon correction
- `d3c9992` — Season 1 progression map
- `7abd36d` — canonical Chapters 3–4 rebuild
- Wave 2A — Chapter 5 `The Classifier`, awaiting final manual F5 review/checkpoint before remote sync

The canonical playable route currently reaches:

`New Game → Ch1 → Ch2 Malet → Ch3 Blank Book/Class Seven → Ch4 Drift/Anchoring → Ch5 The Classifier → Ch6 boundary`

## Development freeze while Season 2 is being written

Do not continue canonical gameplay migration beyond Chapter 5 yet.

The next gameplay task after Season 2 writing is complete is:

**Canon Migration Wave 2B — Chapter 6**

Before that, finish the repository sync tracked in GitHub issue #2.

## Legacy documents

Files such as the old Part I atlas, Steam demo/store copy, tester guide, and older session assumptions may describe pre-rebuild chapter order, endings, demo scope, or character roles. Keep them for historical/reference purposes, but do not use them to override the 46-chapter canon.
