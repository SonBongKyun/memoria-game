# MEMORIA Steam Demo Release Checklist

Use this as the gate before any public demo upload.

## Build

- [ ] `project.godot` version matches export metadata and artifact filename.
- [ ] Godot 4.6.2 headless import succeeds.
- [ ] Windows release export succeeds in CI.
- [ ] Exported `.exe` and `.pck` are both non-empty.
- [ ] Debug-only tools, capture scenes, and local session notes are excluded from the public package where appropriate.

## Save safety

- [ ] New game starts from a clean state.
- [ ] Autosave works on map checkpoint.
- [ ] Autosave before boss battle works.
- [ ] Manual save/load restores player position and story state.
- [ ] VN resume state restores safely.
- [ ] Backup recovery works with a deliberately corrupted primary save.
- [ ] Any save schema change includes migration coverage.

## First-session gameplay

- [ ] The player understands basic movement/interact controls without reading a manual.
- [ ] The first meaningful Memory Burn happens early enough to establish the game's identity.
- [ ] The first burn is a choice with emotional context, not a generic consumable tutorial.
- [ ] At least one later scene visibly changes because of that burn.
- [ ] Elia is introduced before the demo asks the player to sacrifice a relationship memory of meaningful weight.
- [ ] The first 30 minutes contain no long stretch of low-agency exposition.
- [ ] The demo ends with a clear unresolved hook rather than a content wall.

## Memory system

- [ ] User-facing grade labels follow current canon, independent of legacy save rank.
- [ ] The legacy raw enum order has not been changed without migration.
- [ ] Burned, residue, faded, and intact states display consistently.
- [ ] Important burns have an immediate consequence and at least one later callback.
- [ ] Secret/alternate path conditions are not exposed as obvious optimization counters.
- [ ] Memory synthesis, if present, has a narrative cost and does not read like generic crafting.

## Combat

- [ ] Ordinary combat does not bury Memory Burn under redundant systems.
- [ ] Major enemy patterns are readable without relying on hidden numbers.
- [ ] Battle speed options do not change rules or outcomes.
- [ ] Auto battle cannot spend irreversible memories without explicit player approval.
- [ ] Boss battles have safe retry points.
- [ ] Repeated encounters do not dominate demo pacing.

## Presentation

- [ ] Character faces/outfits are consistent across key art, portraits, CGs, and battle plates.
- [ ] UI typography remains readable at 1280×720 and common larger resolutions.
- [ ] English and Korean layouts do not clip important text.
- [ ] AI-generated visual assets read as one art direction rather than mixed model outputs.
- [ ] Audio levels are normalized enough that dialogue/UI/impact cues are not buried.

## Controller and accessibility

- [ ] Keyboard-only playthrough works.
- [ ] Gamepad-only playthrough works.
- [ ] Focus never gets trapped in menus.
- [ ] Text speed setting works.
- [ ] Battle speed setting works.
- [ ] Fullscreen/window settings persist.
- [ ] Important information is not communicated only by color.

## Steam page readiness

- [ ] Short description communicates the hook in one sentence.
- [ ] Capsule art is readable at thumbnail size.
- [ ] Trailer shows Memory Burn and a later consequence, not only cinematics.
- [ ] Screenshots show gameplay UI as well as key art.
- [ ] Store tags match the actual game: narrative RPG / dark fantasy / choices matter / turn-based as appropriate.
- [ ] Demo CTA is Wishlist, not a promise of release scope that is not locked.

## Distribution

- [ ] CI artifact has been downloaded and run on a clean Windows machine.
- [ ] Steamworks App ID / depot / branch are configured before Steam upload automation is enabled.
- [ ] Steam credentials are stored only as repository/environment secrets, never committed.
- [ ] Public build is uploaded to a beta branch first.
- [ ] Crash/save blockers are resolved before moving the build to the default demo branch.

## Final release question

Before shipping, ask one thing:

> Does the demo make the player care about a memory *before* asking them to burn it, and later make them notice what is missing?

If the answer is no, add consequence density and emotional setup before adding more systems.
