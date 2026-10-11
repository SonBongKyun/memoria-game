# S352 — Readable field combat and memory choices

The user asked Codex to continue independently, using the approved Notion manuscripts and worldbuilding, with emphasis on graphics and gameplay. This task changes the current Unreal game in the Codex checkout. Claude's checkout and handoff are not part of the work.

Base: c8870f19186de773306d7a9cbe5d2e5c15c396b8.
Validation: PASS. Source/test commit f6e7411ba10ea2478fb641cc47926d59a1a66595; all nine source/test working files match the executed SHA-256 snapshot. Documentation is recorded separately.

## Source authority

Read on 2026-10-11:

- [Approved KR final hub](https://app.notion.com/p/3e5fcba683478136adbee4854f09d05f), edited 2026-10-10. Its explicit approval table covers final-1 chapters1–56; the latest55–56 were approved2026-10-10.
- Approved [final1](https://app.notion.com/p/3e5fcba68347816a959beab959ed5747), [final2](https://app.notion.com/p/3e5fcba6834781fdadc8f2a973eb818f), [final3](https://app.notion.com/p/3e5fcba6834781e68279e5725c0cbafa), [final4](https://app.notion.com/p/3e6fcba68347817dbb76eef0f724e438).
- [Worldbuilding v10 integrated review](https://app.notion.com/p/3ebfcba6834781b79800d0ceac49aa64), edited2026-10-10, sections01,05,8.1,8.2. This review is subordinate to approved final manuscripts and explicit human decisions. Historical 'current29' character summaries were not treated as the latest approval boundary.

The manuscripts make memory loss a lasting cost; residue and anchoring do not restore a burned memory. The picker therefore keeps the chosen memory, irreversible warning and cancellation controls visible. Restrained blue fire and existing character identities are preserved. Enemy warning geometry, damage and controls are game adaptations; this task does not declare novel canon or revise narrative content.

Only links, timestamps and implementation rationale are recorded. Automatic approval rejected an attempted full-body local reference copy as outside reference-only authorization; no manuscript/worldbuilding body was copied to the checkout.

## Changes

- Replaced opaque enemy warning cylinders with projected outlines and low-opacity fill. Melee uses the existing strike radius including player contact allowance; charge shows a fixed-width capsule; caster shows an open arrow that follows its existing live aim until release. A timing ring fills during preparation and disappears after release/interruption/death.
- Charge preview sweeps the actual collision box with pawns ignored, shortens the lane at cover and clips the capsule at the wall impact plane. Long final frames are limited to remaining charge time, retaining speed/damage and the nominal520cm route.
- Enemy projectiles compare first player-circle contact against first visibility-blocking sphere sweep, including the whole frame segment. Cover absorbs an orb with brief existing impact sparks. Lifetime is bounded and destroyed/dead owners cannot deal damage; dodge immunity is preserved.
- The burn picker reserves header/footer space before assigning rows, shows at most six rows at any UI scale, follows selection through the full inventory, shows overflow/position/scrollbar and fits authored text to available width. Confirmation keeps its cancellation controls.
- Existing native FoeVariety and FieldBurn tests were expanded without changing the registered test count. FieldBurn acquires real imported chapter definitions, reaching 13 owned memories including one already burned (12 available choices). It reads the live owning HUD geometry before checking keyboard/wheel/wrap/digit navigation, Korean/English authored titles, both cancellation inputs, unchanged ownership/history and the existing actual burn. No synthetic story memories were added.

## Verification

Fresh UE5.8.2 serial build and **58/58 tests passed**: Foundation 6/6 plus related 52/52 (audio 3, campaign 4, shop 15 and all 30 rendered visual tests). Registered identities match exactly; failed/notRun/inProcess are zero. All pipeline commands exited zero with no fatal diagnostics. The full 274-test registry was not rerun.

The native picker reported 13 owned / 12 available, local widget size 2011.3 x 1081.1, six visible rows and middle index 8. This verifies actual DPI-scaled overflow, in addition to geometry checks at 1280x720, 960x540 and 640x360. Keyboard input in Foundation moved Arrel from (137.250,83.500) to (288.361,83.500), pressed=1, ignored=0.

25 fresh combat/picker captures are 1280x720, with exact hashes in S352_RESULTS.json. The closed charge lane, melee circle, caster arrow, middle-window selection and Korean/English last-page confirmation were inspected. A pixel comparison of six damage columns, chips and numeric shortcuts confirmed both language captures contain the same numeric content when the image preview was ambiguous.

Existing diagnostics remain disclosed: each process logs 15 startup Condition-failed lines before the first test; the related process logs 46 memory-budget lines. Three tests pass with warnings: Foundation.MapInputAndModal and Campaign.CanonicalPaidRoute report r.MotionVectorSimulation render-thread safety; MemoriaVisual.Chapter1Presentation reports a missing-world context. Exact messages and raw report locations are versioned in S352_RESULTS.json.

An initial 58-test pass preceded the final outline closure and live-DPI overflow improvement. The second run then exposed a same-container TArray::Add assertion in the newly closed Stroke helper. Copying its first point to a local value before Add fixed the fault. Both earlier attempts are retained under Saved/Validation/s352/attempt1 and attempt2; the final fresh 58-test run followed the repair. Independent review rescanned all nine files for analogous alias and rendering hazards.

Preservation checks confirm 875 unrelated tracked own source/content/tool files remain byte-identical. Claude remains clean and unchanged at 159ae961f844e3492cd9857b6d472e8aa79b5d02. No own engine/compiler process remains after validation.

Post-fix read-only independent review found no remaining actionable correctness defects. Two evidence limits remain: smaller picker viewports are geometry assertions, while native captures use1280×720; blocked-charge tests assert the shortened centerline and actual blocked movement, while polygon half-plane clipping is inspected code rather than a dedicated vertex assertion.

## Remaining scope

This is one bounded field-combat polish task, not completion of all graphics work. Shared NPC faces, primitive market props and original pass-through stalls remain. No new model import, save schema, story/dialogue, enemy numerical rebalance, remote push or peer merge.

Raw generated evidence: Unreal/Memoria/Saved/Validation/s352/.
