# Verdan memory shop transactions — S288

Worktree: C:/Users/jc/MemoriaMigration/foundation.
Branch: unreal-migration/ue58-foundation; entry HEAD:
969d7aad682b14d03b91885a1134688cbb91ad66. Local working changes only.

The 108-chapter manuscript rebuild remains provisional. No manuscripts, authored
Godot content, artwork, narrative IR or existing packages are edited. S287 movement,
stride and camera work is preserved. No commit, push, deployment or package resave.

## Implemented behavior

- Sell/buy tabs, mouse rows/action buttons, keyboard/controller navigation and
  explicit close. Completed trades clear selection.
- Exact source prices and two offers reuse the prior mechanically generated stock.
  Sale executes the existing domain burn, including residue, cascade and passives.
  The source Ash oath flag/toast follows the burn before crediting grains.
  Core, burned, faded and collateral memories cannot enter a sale.
- Purchase checks current stock and balance, debits before the grains signal,
  acquires the source definition, then marks the offer sold. Insufficient funds
  and repeat purchases have no effect. Purchased definitions survive existing
  complete save serialization/restore.
- Monotonic view revisions reject stale tab/selection/duplicate confirmation.
  Busy guards stop nested transactions during synchronous notifications.
  Run replacement, same-ID restore and owner-world cleanup invalidate the session.
  Integer overflow is rejected before a sale burns a memory.
- Close records ch2_complete and current chapter3, then observes source autosave
  and achievement requests in order. The UI states the pending boundary
  before:chapter_transition_delay. Cleanup never invokes close effects.

## Source evidence and limits

export_shop_transactions_oracle.py executes exact original memory shop handlers
and Malet stock/close callback, with actual MemoryManager and JourneyOath in an
isolated Godot4.6.2 project. UI construction collects rows. Toast/audio/profile/save/
travel endpoints observe requests; no real save or scene transition is performed.

Twelve fixtures cover four sale grades, residue/cascade connections, EN/KO oath
notifications, insufficient/exact funds, two purchases, purchase/resale, exchange
and close. Close is observed immediately and after the source1.5second timer.
The native implementation explicitly stops before scheduling that travel; no
stale travel timer is created.

Source02 generated the fixtures; source_check03 and source_check04 independently
reproduced them byte-for-byte. Source_check04 includes shop/map/save hashes and
the exact executed harness ZIP. Notification text is mechanically generated from
executed results. No new dialogue, lore, price or offer text was authored.

Native comparisons cover grains, chapter/flags, sold IDs, every memory's burned/
residue/faded/erosion/connections, burned history and toast text/type after each
transaction. NPC world cognition remains unchanged. Audio/profile/stat handlers
are not claimed as executed in Unreal. No sound, persistent achievement, disk
autosave or Chapter3 map is implemented by this step.

## Fresh validation

- Source12/12, followed by two deterministic check executions.
- UE5.8.2 / CL56702186 build and native03:14/14.
- Source state tests12; guard suite1; rendered actual-input campaign1.
- Physical replay: arrival/payment/reward → two5G sales → 8G purchase → 2G →
  ESC close, chapter3 and visible truthful development boundary.
- Final host100/100 (host03.log); static structure62. Original byte verification
  passed in the separate preservation gate.
- Related rendered regressions40/40: native03 transactions14 + shop06 shop12 +
  full04 completed Campaign4/Malet4/Foundation6. Exact identities and source equality
  are recorded in validated_coverage.json.
- Full217 regression is INCOMPLETE: full04 hit the runner900second timeout after
  78 successes, with no completed test failures and no final automation index.
  Full05 NullRHI was stopped after79 completed (27 failed render-dependent checks).
  It cannot certify tests that require a real viewport/texture resource. Neither
  attempt is represented as a successful whole-suite run.
- Final preservation02 PASS: original4217, protected worktree22405, oldUE95,
  prior fixtures/evidence and unchanged HEAD. No new packages. Entry captures22424
  files including prior uncommitted work; before_source.zip retains exact sources.
  Final_review01 confirms S287 movement/presentation files and generated stock
  remain byte-identical; attributes retain their entering bytes as a prefix.
- Raw native screenshots visually inspected: balance, transaction feedback and
  close limitations are readable and unclipped.

## Retained failures

Source01 rejected mixed indentation in the synthetic harness; source02 corrected
it without changing original files. Native01 failed UE5.8 checked format API;
runtime formatting of generated text was corrected and source/logs retained.
Native02 passed source12+guards1; actual-input test timed out because its new stage
reused existing stage13. Only the new stage changed to14; native03 passed14/14.
The full04 archive helper initially expected a final index after timeout. Its partial
archive remains; the helper now skips absent indexes and bounds artifacts to that
execution timestamp. Full05 mode-related failures and explicit cancellation are
recorded in its partial_results.json. No game or assertion was changed for them.
No assertion, timeout or source expectation was relaxed.

The old first-screen replay no longer presses ESC as an inert key because ESC
now closes the shop. The new replay asserts close effects; previous pre-action
state and trace checks remain. Four host tests subtract the14 new test identities
when comparing historical lists, preserving historical totals and exact identities.

## Remaining work

Actual disk save/load and continuation, persistent achievements, next-map ownership,
representative revisit battle/memory burn, archive feedback, and human timing/feel
review. The integrated15–20minute segment is not complete. Items/equipment/loans and
duplicate-ID source purchases outside this single-entry bounded stock are excluded.

## Byte storage rules

The new fixtures and generated notification include need LF on Windows for
deterministic re-checkout. Three narrowly scoped .gitattributes rules are appended;
the complete entering bytes are in before_gitattributes.bin. Restore that file
to roll back just this task's rules while retaining S287's existing rules.
Raw evidence is marked non-text; no existing file is renormalized.


## Screens and evidence

![After buying the copper memory](evidence/shop_transactions1/final_captures/Shop_Bought.png)
![Explicit close boundary](evidence/shop_transactions1/final_captures/Shop_Closed.png)

These are byte-identical PNGs extracted from the successful native03 archive;
final_captures/manifest.json records their origin and SHA256.
