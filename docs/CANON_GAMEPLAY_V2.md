# MEMORIA Gameplay Canon v2

This file translates the current MEMORIA canon into implementation rules for the game. It exists to prevent old GDD terminology from silently overriding the current story bible.

## Memory burn grades

Current player-facing canon is:

| Canon grade | English | Korean | Internal legacy rank |
|---:|---|---|---:|
| 1 | Ember | 잔불 | 0 |
| 2 | Flame | 불꽃 | 1 |
| 3 | Blaze | 화염 | 2 |
| 4 | Sun | 태양 | 3 |
| 5 | Zero | 영점 | 4 |

The internal rank is intentionally retained because historical save data serializes raw grade integers. A UI may never infer display text from old enum names such as `GRADE_5`/`GRADE_1`.

### Compatibility rule

`canonical_grade = internal_rank + 1`

Do not invert historical integer values in an existing save. If a future save format stores canonical 1..5 values directly, stamp that schema explicitly and migrate it at load time.

## What each grade should feel like

### 1 — Ember / 잔불
A sensory trace or low-stakes fragment. Useful enough to tempt the player, but its absence is initially subtle.

### 2 — Flame / 불꽃
Routine and lived habit. Losing it should make familiar behavior or a place feel wrong.

### 3 — Blaze / 화염
Relationship memory. Losing it should alter trust, recognition, instinctive care, or dialogue.

### 4 — Sun / 태양
Identity-bearing memory. Losing it should alter how Arrel acts, fights, interprets himself, or is read by others.

### 5 — Zero / 영점
Core identity/name-level sacrifice. This is not a routine consumable. It must feel irreversible and structurally important.

## Signature design rule

Power is immediate. Cost is delayed and human.

A strong memory burn should normally produce:

1. **Immediate power payoff** in the battle or crisis that forced the choice.
2. **Immediate absence cue** so the player understands that something real was lost.
3. **Delayed revisit** after enough time has passed for the player to remember the original event.
4. **Long-tail consequence** when appropriate: changed relationship, navigation, boss state, route, or ending.

The game should avoid reducing memory loss to a visible optimization puzzle. Hidden exact ending thresholds and consequence discovery are preferred over checklist-style min/maxing.

## Vertical-slice spine

The Steam demo should concentrate on a compact sequence:

1. Rim forest establishes Arrel, ash, danger, and the physical feel of memory loss.
2. A first battle makes Memory Burn necessary or deeply tempting.
3. Elia creates an emotional anchor.
4. A later scene visibly answers the first burn.
5. Verdan demonstrates that memory extraction/trade is a social system, not only a combat mechanic.
6. A second difficult choice makes the player understand that stronger power asks for a more personal cost.
7. Authority/Kairos pressure reframes Arrel as something being observed.
8. BL-07 is teased strongly enough to drive a wishlist/full-game desire.

The demo does not need to explain the whole cosmology.

## Narrative priorities

The player only needs enough lore to understand the immediate stakes. Prefer playable evidence, environmental detail, consequences, and character behavior over philosophical exposition.

When adapting novel material, preserve the meaning but shorten explanations. If a theme can be expressed through a player choice or changed interaction, prefer that over dialogue explaining the theme.

## Character voice guardrail

Not every character should speak in the vocabulary of the setting's philosophy. Words such as memory, record, witness, completion, name, absence, and unfinished are powerful because they are not used by everyone in every scene.

Arrel, Elia, Tobias, Nera, Kairos, Veil, Sable, and ordinary NPCs should differ in rhythm, priorities, and what they notice.

## System priority

Protect these systems first:

1. Memory Burn
2. World Rewrite
3. Residue / Elia anchoring
4. Burn-dependent dialogue and revisits
5. Ending/route consequences

Generic RPG systems are supporting material. They may be simplified if they obscure the memory-cost loop.
