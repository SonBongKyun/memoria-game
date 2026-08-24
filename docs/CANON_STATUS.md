# MEMORIA Canon Status

This file is the short-form guardrail for story and Memory-system work during the Season 1 rebuild.

## Narrative source of truth

Use this priority order:

1. Latest completed English Season 1 manuscript, Chapters 01–46
2. `docs/SEASON1_GAME_PROGRESSION.md`
3. `docs/SEASON1_MEMORY_MAP.md`
4. Current canonical runtime implementation on `canon-rebuild-season1`
5. Legacy documentation only as historical/reference material

The Korean manuscript/translation is incomplete and must not override the completed English Season 1 manuscript for canon decisions.

## Canonical development branch

- Branch: `canon-rebuild-season1`
- Current checkpoint: `84684aefa5721f97ee2318d2f8fa64d92dd077bc`
- Commit: `feat(story): implement canonical chapter 5 classifier`
- Playable canonical rebuild currently reaches the Chapter 6 boundary

Current route:

`New Game → Ch1 → Ch2 Malet → Ch3 Blank Book / Class Seven → Ch4 Drift / Anchoring → Ch5 The Classifier → Ch6 boundary`

## Confirmed Season 1 guardrails

- Season 1 contains 46 chapters.
- Sable is **not** a Cleaner.
- Sable's canonical state separates persistent knowledge from personal relationship memory:
  - knowledge: `fact.bl07.seventeen_seekers_never_returned`
  - memory: `memory.sable.vessa_daughter_bond`
- The old Cleaner backstory, child/parent erasure order, 12-person memorial, and old Malet → Sable note ripple are non-canon.
- Malet's BL-07 state separates event knowledge from requester identity:
  - knowledge: `fact.bl07.route_request_received`
  - source/identity memory: `memory.malet.bl07_request_source`
- Chapter 5 snapshots Malet's state into a historical Kairós report outcome. Later restoration of Malet's memory does not retroactively rewrite the already-delivered report.
- Kairós report outcomes are mutually exclusive after the report resolves:
  - `fact.kairos.malet_report_identified_arrel`
  - `fact.kairos.malet_report_requester_unknown`
- Tobias does not join the party in early Chapter 3. His canonical placement is later in the rebuilt progression.
- Chapter 43 Name Burn is a required canon story event, not the old optional ending branch.
- Legacy 7-way ending logic must not be used to infer the current canon ending structure.

## Canonical checkpoints

- `2eb5d33` — Sable/Vessa canon correction
- `d3c9992` — Season 1 progression map
- `7abd36d` — canonical Chapters 3–4 rebuild
- `84684ae` — canonical Chapter 5 `The Classifier`

## Development pause

Do not continue the canonical gameplay migration beyond Chapter 5 while Season 2 is still being written.

Next gameplay task after the writing pause:

**Canon Migration Wave 2B — Chapter 6**

## Legacy-document rule

Older Part I atlas material, Steam/demo copy, tester guides, old GDD assumptions, session notes, and runtime branches can still contain valid assets or historical implementation detail. They do not override the source-of-truth order above.

When an old file conflicts with the current 46-chapter manuscript or the two Season 1 mapping docs, treat the old narrative assumption as legacy until explicitly revalidated.
