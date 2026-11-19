# S367 Void-touched elites (Claude, 2026-11-19)

Branch `claude/s366-upgrade`, on top of S366 (`01f85751`). Codex is paused for now (user, 2026-11-19); Claude builds, validates and pushes alone.

## What it does
- **When.** A roaming fight brings one elite in its pack when `EliteWave(N, burns)` holds for the run's Nth fight (`TotalBattles + 1`). The rule:
  - never before the 3rd fight;
  - otherwise every `clamp(5 - burns/2, 2, 5)`th fight: every 5th unburned, every 3rd at 4 burns, every 2nd at 6+.
  The more Arrel burns, the sooner the void notices him; the change is shown only indirectly, with no gauge. It is deterministic, takes no RNG draw, and seeded encounter/drop sequences are unchanged. One-time battle areas never bring elites.
- **What.**
  - Health x2.2, blows x1.3, and 1.18x the height.
  - Named as it rises ("Void-touched" / "공허에 잠식된 자").
  - A violet edge-lit shell: S366's `M_Afterimage` used as the skeletal mesh's overlay material.
  - A breathing violet point light.
- **The shell.** A plain cut or a firebomb does 35% and neither staggers nor shoves the elite. An Echo Counter, a heavy cut, a memory burn wave, Elia's Remembered Strike or a parry breaks it. On a break:
  - full harm for that blow, and the foe reels 1.2 s;
  - a "Shell broken" popup, violet burst and ring, a heavy hit stop;
  - the light dims.
  Popups now show the damage actually dealt (`GetLastHitDamage`).
- **Reward.** The elite pays x3 grains when it falls. Its wave always drops an item, from the richer (void) table; the 30% roll is still drawn, so seeded runs keep their sequence.
- **Debug.** F11 in the Verdan slice spawns an elite thief, next to the existing F9/F10.

## Files
- `MemoriaFieldCombatTypes.h`: S367 constants plus `EliteInterval` and `EliteWave`.
- `MemoriaFieldMonster.{h,cpp}`: elite setup, the shell, `TakeHit(..., bBreaksShell)`, `BreakShell`, the aura light.
- `MemoriaFieldCombatSubsystem.{h,cpp}`: `SpawnWave(..., Elites)`, `EncounterElites`, `NotifyShellBroken`, damage x1.3, which hits break the shell, rewards, resets.
- `MemoriaChapterPresentation.cpp` and `MemoriaSliceHost.cpp`: roaming encounters ask `EncounterElites()`; F11.
- `MemoriaElitesS367Tests.cpp`: `MemoriaVisual.S367Elite`.

## Verification
Native UE 5.8.2 / D3D, 1280x720, fresh editor build after the final source edit (`Result: Succeeded`):
- `MemoriaVisual.S367Elite` -> `Saved/Validation/s367/focus3`: **PASS**. The schedule table, the elite's health, height, overlay and light, a plain J cut grazing at exactly 35% with no stagger, a held-J heavy cut breaking the shell with full harm and a stagger, x3 grains, and a guaranteed item. Captures `EliteShelled.png` and `EliteShellBroken.png` inspected.
- Wide regression: every `MemoriaVisual.*` test (chapter journeys and revisits included) + `Memoria.S36*` + `Memoria.Foundation.MapInputAndModal` -> `s367/wide1`: **PASS 56/56**, exit 0, fatal 0, inputs unchanged.
- Failed runs kept: `focus1` and `focus2`. The test expected 30 from the heavy cut, but the thief's Weaken (x0.7) and the ordinary cut that opens a held J (14 x 0.35) were also in play. The test now measures from the heavy cut's start and accounts for Weaken. No game value was changed.
- Not run: the full registry (memory parity, campaign and checkpoint suites) and the package/cooked smoke.

## Limits
- No physical play or package run.
- The elite reuses its kind's model and moves; there are no new attacks.
- The multipliers are first-pass, tuned by feel.
