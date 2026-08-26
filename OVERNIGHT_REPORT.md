# OVERNIGHT REPORT — Gameplay & Visual Polish Pass

**Branch:** `overnight-gameplay-graphics` (from `canon-rebuild-season1` @ `84684ae`)
**Date:** 2026-08-26 (overnight session)
**Scope:** Gameplay feel, combat presentation, visual identity, UI readability. No story, lore, or content changes.

---

## 1. Executive Summary

Six focused improvement cycles were completed and committed, each verified with the project's own
smoke-test infrastructure and inspected through real-OpenGL captures before moving on.

The single largest finding came from *observing* the game first: in 4 of 6 representative encounters
(void_beast, threshold_shade, shade_sentinel, kairos) the enemy was nearly invisible — dark void
artwork on dark purple stages. Brightening was already maxed (enemy plates ship at 1.5x modulate);
multiplying near-black art stays near-black. The fix was **backlight separation**: an identity-colored
radial glow behind each enemy, generated in code (zero new assets).

On the field side, the player avatar was hard to spot on dark maps (verdan_market, belt_waystation,
drift_shelter, crumbling_coast). The fix doubles as identity: Arrel now carries a faint warm
**memory light** — readable, atmospheric, and thematically correct for the man who burns memories.

All 20 core smoke suites pass on the final tree. Three pre-existing smoke failures
(`smoke_ancillary_archive`, `smoke_production_save_path_guard`, `smoke_title_grandeur`) were verified
to fail identically on the untouched base commit — environmental (save-root guard config), not
regressions.

---

## 2. Gameplay Improvements

### G1. Carried memory light (player.gd, map_effects.gd) — `06c1418`
- A soft warm `PointLight2D` (memory-gold, energy 0.5, ~110px radius) follows Arrel with a slow
  breathing flicker.
- Solves avatar readability on the four darkest maps while reinforcing the core fantasy: the man
  glows faintly with the memories he carries.
- Respects `clean_gameplay_visuals` (dimmer, static-friendly) and costs one light — no per-frame
  allocations.

### G2. Memory Pulse directional motes (player.gd) — `06c1418`
- When a pulse finds echoes, 7–12 warm motes now drift from the player toward the echo bearing
  before the toast text arrives.
- The world answers the pulse spatially instead of only through UI text; direction is readable
  at a glance mid-exploration.

---

## 3. Combat Improvements

### C1. Enemy presence backlight (battle_scene.gd) — `b81494a`
- Identity-keyed radial backlight behind every enemy plate: cold spectral violet for void/shade
  families, crimson-violet for bosses (double halo), ember for ash/crawler families, pale moonlight
  as default.
- Void enemies additionally flicker (`self_modulate`, composed with the breathing pulse so the two
  tweens never fight) — the VOID visual language: cold, unstable, wrong.
- Slow 1.4s/1.9s breathing pulse; static fade-in under `reduce_motion`; skipped under clean view.
- Verified by before/after survey captures: kairos and shade_sentinel now separate clearly from
  their backgrounds.

### C2. Identity-keyed enemy death dissolve + death SFX (battle_scene.gd, audio_manager.gd) — `8d48d8b`
- Replaced the generic 4-color square burst with identity-keyed streak motes: gold memory sparks
  mixed with the enemy's own presence color. Void enemies dissolve **upward** with wobble
  (released from space); normal enemies burst radially; bosses get a larger, wider burst.
- The presence backlight flares once before the container fades — the light dies with its owner.
- New procedural `enemy_die` SFX (collapsing pitch + dispersing noise, ±9% pitch variation) wired
  into the death moment; previously death was silent.

### C3. Victory fanfare palette (battle_vfx.gd) — `90513df`
- Rainbow party confetti (pink/lime/cyan) replaced with the game's own language: gold memory
  embers, amber, and falling ash, with rare cold pale-blue sparks.
- Asterisk `*` sparkles replaced with small cross-shaped Line2D sparkles matching the mote language
  used by dust/pulse/death effects.

---

## 4. Visual Improvements

| # | Change | Commit |
|---|--------|--------|
| V1 | Enemy presence backlight system (see C1) — the biggest visible change in the game | `b81494a` |
| V2 | Carried memory light on the field (see G1) | `06c1418` |
| V3 | Pulse direction motes (see G2) | `06c1418` |
| V4 | Identity death dissolve (see C2) | `8d48d8b` |
| V5 | Victory ember/ash palette (see C3) | `90513df` |

All new visuals are code-generated (GradientTexture2D radials, Line2D motes, PointLight2D) — no
placeholder art, no downloaded assets, no existing artwork replaced.

---

## 5. UI / UX Improvements

### U1. Turn banner readability (battle_scene.gd) — `b81494a`
- The "당신의 턴 / YOUR TURN" banner was nearly invisible in captures: dim gold text, 62%-alpha
  border on a dark stage.
- Text brightened to (1.0, 0.92, 0.70) with a 4px black outline; border raised to 85% alpha in a
  brighter gold. Turn transitions now read instantly.

### U2. Grade-colored burn list (battle_scene.gd) — `578fc66`
- Every entry in the battle burn list used identical gray styling — memory weight was invisible at
  decision time.
- Entries now carry a 3px left border accent and title tint in the established
  `UITheme.GRADE_COLORS` convention (same indexing as the Memory Archive), with matching hover
  states. Residue entries keep a distinct ghost-violet border so "already gone" reads at a glance.

---

## 6. Files Changed

| File | Change |
|------|--------|
| `scenes/battle/battle_scene.gd` | Presence backlight system, turn banner, death dissolve, burn-list grade colors |
| `scripts/core/player.gd` | Carried memory light, breathing update, pulse motes |
| `scripts/utils/map_effects.gd` | `add_carried_light()` helper |
| `scripts/systems/battle_vfx.gd` | Victory ember/ash palette, cross sparkles |
| `scripts/systems/audio_manager.gd` | `enemy_die` SFX + pitch variation entry |

Commits (oldest → newest):
1. `b81494a` polish battle: enemy presence backlight and turn banner readability
2. `06c1418` polish field: carried memory light and pulse direction motes
3. `8d48d8b` polish battle: identity-keyed enemy death dissolve and death sfx
4. `578fc66` polish battle: grade-colored burn list entries
5. `90513df` polish battle: victory fanfare uses memory ember and ash palette

The pre-existing user modification to `assets/fonts/theme.tres` and the untracked `.uid` files were
preserved untouched throughout.

---

## 7. Tests Performed

- **Headless import (full project parse):** exit 0, zero script/parse errors, run twice (start and end).
- **Full smoke sweep:** all 51 smoke scenes executed; 48 pass. 3 failures
  (`smoke_ancillary_archive`, `smoke_production_save_path_guard`, `smoke_title_grandeur`) verified
  **identical on the base commit** via a detached worktree at `84684ae` — pre-existing environmental
  issue (SaveManager smoke path guard expects a configured test save root), not a regression.
- **Focused regression subset (final tree):** 20/20 pass, including canon story regression
  (`smoke_canon_wave2a`), battle interface/command deck/cinematic stage, story combat, movement
  naturalism, field flow/focus, gameplay QoL, ambient life, visual clarity, tactical directives,
  memory cascade, burn directive stabilization, RPG systems/depth, early loop, crash guards,
  resonance choice, sable memory gameplay.
- **Real-OpenGL captures inspected at 1280x720:**
  - `battle_survey.png` (6 encounters) — before/after for backlight, banner, and final state.
  - `verdan_malet_field.png` — carried light on the darkest map.
  - `burn_preview_stakes_ko.png` — burn ritual panel unaffected.
  - `map_survey.png`, `endpoint_survey.png`, `memory_archive_ko.png` — context surveys.

---

## 8. Bugs Discovered and Fixed

- **Enemy parse error during development:** `_spawn_enemy_presence_backlight` initially referenced
  an undeclared local (`enemy`); caught immediately by `smoke_battle_interface` and fixed before
  commit.
- **Tween property conflict risk:** void flicker initially animated the same `modulate:a` property
  as the breathing pulse; redesigned to animate `self_modulate:a` (multiplicative composition)
  before it could ship jitter.
- No pre-existing gameplay bugs were discovered beyond the three pre-existing smoke failures noted
  above (environmental, documented for the next session).

---

## 9. Remaining Known Issues

1. **Pre-existing smoke failures (3):** `smoke_ancillary_archive`, `smoke_production_save_path_guard`,
   `smoke_title_grandeur` require the SaveManager smoke save-root environment configuration that
   earlier sessions used. They fail identically on the base commit. Next session should restore the
   sandbox save-root env (see `smoke_production_save_path_guard.gd:10-14`) or mark them as
   env-gated.
2. **Battle stage mid-field remains sparse** by design (S211 deliberately keeps the 2D floor strip
   minimal so the 3D perspective grid reads). The backlight now gives it a focal point, but a
   future pass could add per-biome floor accents.
3. **Enemy HP "HP ? / ?" gating** is intentional design (Ash Sight/scan reveals); untouched.

---

## 10. Suggested Next Five Improvements

1. **Restore the smoke save-root environment** so the three env-gated suites run in CI again
   (small, unblocks full-suite gating).
2. **Per-biome battle floor accents** — extend `HybridDepthStage` profiles with a faint floor
   gradient/prop silhouettes so the mid-field reads as place, not void (pairs with the new
   backlights).
3. **Enemy attack wind-up telegraphs in-world** — a brief scale/lean anticipation on the enemy
   plate 0.2s before enemy strikes (the cue panel exists; the body should also promise it).
4. **Memory Pulse echo pings on the minimap** — the pulse already reports direction/distance in
   text and motes; a temporary minimap blip would close the loop for players who navigate by map.
5. **Burn afterglow on the field HUD** — after a battle where memories were burned, briefly tint
   the ExplorationHUD memory counter ash-gray for a few seconds (the cost follows you out of the
   fight).

---

## 11. Art Assets That Would Provide the Largest Visual Upgrade

Per the art-asset limitation rule, nothing below was placeholder-faked; all were improved in code
instead. These professional assets would still elevate the game most:

| Asset | Scene | Spec | Direction | Why | Priority |
|-------|-------|------|-----------|-----|----------|
| Enemy aura plates (void family) | Battle, behind enemy | 512x512, transparent | Cold violet radial mist with slow tendrils, painterly | The code backlight separates silhouettes but a painted void-mist would give the void family a signature presence no other enemy shares | High |
| Victory banner calligraphy | Battle victory moment | 640x200, transparent | Ink-brush "살아남았다 / SURVIVED" with ash texture | Replaces UI-panel-only victory with an authored moment; current fanfare is particles only | Medium |
| Field dusk gradient overlays (5 biomes) | Maps, full-screen | 1280x720, screen-blend | Subtle horizon-anchored dusk gradients per biome | Would deepen time-of-day feel beyond CanvasModulate tints | Medium |
| Boss intro sigils (Kairós, Shade Sentinel) | Battle intro overlay | 480x480, transparent | Occult seal line-art matching each boss's chapter motifs | The intro shows name + art; a rotating sigil behind it would make boss entrances ceremonial | Medium |
| Ash-snow ground frost overlay for Colorless Waste | Field tiles | 256x256 tileable | Desaturated crystalline frost with faint memory-color glints | The most distinctive map currently relies on geometry alone | Low |

---

## Verdict

Movement, exploration, and battle were already deeply iterated (S57–S246); this pass targeted the
genuine remaining gaps found by observation rather than assumption: enemies you could not see, an
avatar that vanished into dark maps, a silent death, party-confetti victory, unreadable turn
banner, and a burn list that hid memory weight. Every change was verified in-engine and through
real captures, and the game's identity — memory as warm, fragile, costly light; void as cold,
unstable wrongness — is now expressed consistently across field, battle, and death.
