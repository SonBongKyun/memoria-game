# Verdan play feel 1 — walking, stride and playable camera

Status: verified bounded control-tuning pass. Local working changes; no new commit, push or deployment.
Entry checkpoint: `969d7aad682b14d03b91885a1134688cbb91ad66`.
Worktree: `C:/Users/jc/MemoriaMigration/foundation`, branch `unreal-migration/ue58-foundation`.

## Scope

This is the first control-tuning step toward a 15–20 minute Verdan slice. It does
not complete that slice. The 108-chapter manuscript is being rebuilt in parallel;
no draft chapter, illustration or gameplay tuning value is promoted to final canon.
No dialogue, event, memory consequence, original Godot source or source artwork is edited.

The Verdan host explicitly opts its pawn into a walking profile. The generic
Foundation pawn keeps its existing movement defaults and orthographic view.
The presentation actor continues to own only rendering and its existing camera.
Existing maps, seven physical surfaces, 16x16 collider, Malet position/interaction
range, inventory/story contracts and the `before:shop_actions` frontier remain.
No new or resaved Unreal content packages are needed. No push or deployment.

## Tuning

| Quantity | Previous | This pass |
| --- | --- | --- |
| Verdan maximum speed | 1200 units/s | 120 units/s |
| Acceleration / deceleration | 4000 / 8000 units/s² | 1200 / 1200 units/s² |
| Fore/aft foot reach, source skeleton | 28 cm | 32 cm |
| Stance portion of stride | 60% | 55% |
| World stride at mesh scale 0.68 | 63.47 units | 79.13 units |
| Walk cadence at maximum speed | saturated at 3 cycles/s | 1.52 cycles/s |
| Camera offset | (0,-1150,1450) | (0,-820,1000) |
| Horizontal FOV / downward pitch | 65° / 42° | 55° / 48° |

Values live in `Unreal/Memoria/Source/Memoria/Public/Framework/MemoriaVerdanTuning.h`.
Both distance-driven stance and swing use the same stride parameters. The visual
safety cap remains above the supported walking speed. Partial stick input keeps
its proportionate speed; diagonal input is clamped by the real movement component.
The camera retains exact central following and the smooth spatial edge limits;
there is no extra time lag, head bob or camera rotation tied to walking direction.

These values change pace for the character's current scale. They do not manufacture
15–20 minutes of content by slowing down an otherwise complete route. Even the
courtyard width takes only about 15 seconds of unobstructed walking to cross.

## Validation

- UE 5.8.2 / CL56702186 build passed. Final `visual02`: 3/3 passed.
- Initial `visual01`: 2/3 passed. The half-stick response reached 95% speed in
  167–183ms, missing the 160ms response target. Original source, logs, measurements
  and captures are retained in that execution. Only acceleration was raised from
  the draft 900 to 1200; no assertion or tolerance was relaxed.
- Final axial/diagonal speed: 120 units/s; half-stick speed: 60 units/s at all three
  tested frame rates. Full-input cruise: 100ms; half-stick cruise: 117–133ms.
  Full-input stopping takes 100ms and 4–5.5 units. Two-second axial and diagonal
  distances match, ranging from 234.5 to 236 units across 30/60/120Hz.
- Final 12 skeletal probes cover 60/120/180/1200 units/s at 30/60/120Hz.
  The nine supported walk-range probes measure maximum planted-foot slip 0.009234
  world units/sample, below the unchanged 0.04 limit. The three 1200-speed stress
  probes only check finite pose/clearance/settling and do not claim no-slip walking.
- All eight rendered edges/corners retain the head and feet inside the playable view;
  center character height is 11.0967% of the actual 1274x682 PIE viewport.
  The wall still stops X at 881.9 and the gait settles when blocked. Seven original
  physical surfaces, collider, Malet reachability, run/memory state and narrative
  trace checks pass. Final field and corner captures were inspected.
- Host tools: 96/96; static/source integrity: 71 checks, original 4,217 files passed.
  These ran before the last numeric acceleration adjustment; host tooling and
  protected files were not changed by that adjustment.
- Fresh rendered gameplay regressions passed: campaign 4/4, shop 12/12,
  Malet 4/4 and Foundation 6/6. Together with final visual 3/3, this is 29/29
  relevant UE checks on the final runtime, with zero fatal diagnostics.
- Byte-preservation gate passed: original 4,217, protected tracked worktree 22,375,
  previous UE packages 95 and prior IR/fixtures 90. No new packages or narrative IR.
  Historical reports/evidence and the original manifest remain byte-identical.

Evidence is written only to `evidence/playfeel1`; prior reports and raw executions
remain unchanged. Both visual executions contain exact tested-source ZIPs and raw
logs/captures. Final selected PNGs are copied unchanged from the passing raw archive.
No full 203-test suite, source Godot gameplay/native oracle or packaged run is claimed.

The added response probe advances the possessed production movement component at
30/60/120 Hz using axial, diagonal and half-stick input, measuring actual position,
velocity, time to cruise and stopping distance. Existing Enhanced Input replays
exercise real key events, walls, narrative handoffs and Malet interaction.
Foot-contact checks inspect actual transformed skeletal feet, not a second IK model.

## Remaining work

Straight walking is the scope of planted-foot validation. Sharp turns and the
start/stop blend still lack full world-space foot locking. Character/environment
art remain prototypes; this is not final animation, all-aspect-ratio certification
or a packaged build. Human control-feel approval and a timed 15–20 minute run remain.

The next bounded gameplay step is to inspect the executable reference contracts
for shop purchases, sales and closing, then extend beyond `before:shop_actions`.
Revisit combat/memory-burn, archive feedback and save/load still need their own
bounded implementation and evidence before the representative loop is complete.

## Preservation / rollback

`entering_baseline.json` records all 22,385 tracked file hashes and the unchanged
4,217-file original manifest. `before_changes.zip` holds exact bytes of all prior
files authorized for this pass. Rollback can restore only the task-owned changed
files from that archive, preserving any subsequent user edits; remove new task files
only after checking their diff. No package reimport or Git reset is needed.

![Final playable camera](evidence/playfeel1/final_captures/MarketFront.png)

Final review: `evidence/playfeel1/final_review02.json` confirms tested-source identity,
raw archive/capture hashes, unchanged session-log prefix, untouched index/HEAD and
repository-configured `git diff --check`. The first review's temporary
`core.autocrlf=false` comparison exposed old mixed CRLF as whitespace changes;
that failed review is retained. Existing bytes were preserved, not renormalized.
