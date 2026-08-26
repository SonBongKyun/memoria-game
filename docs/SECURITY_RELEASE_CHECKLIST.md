# Security & Release Checklist

This is a practical release gate for a solo/public Godot repository. It is not a substitute for a dedicated security audit.

## Secrets and publishing

- [ ] No Steam username/password, Steam Guard token, API key, signing key, certificate, or private webhook is committed.
- [ ] SteamPipe credentials live only in protected CI secrets or the publisher workstation.
- [ ] Production signing material is never stored in the public repository.
- [ ] Debug-only tokens/URLs are removed before a public build.

## Save integrity

- [ ] Saves are written only under `user://`.
- [ ] A backup is created before overwriting an existing save.
- [ ] Corrupt primary saves can fall back to backup without executing user-controlled code.
- [ ] Destination scene/resource paths are validated before mutable runtime state is imported.
- [ ] Schema changes bump `SAVE_VERSION` and include migration logic.
- [ ] Developer smoke/capture runs cannot overwrite a real player autosave.

## File and content handling

- [ ] No arbitrary filesystem path from a save/data file is opened without validation.
- [ ] JSON/imported data uses bounded/default values where practical.
- [ ] Missing art/audio/resources fail gracefully rather than softlocking a run.
- [ ] User-visible external links, if added, are explicit and trusted.

## Public build

- [ ] Export uses release mode.
- [ ] Console/debug wrapper is disabled for the public Windows build unless intentionally needed for a test build.
- [ ] Developer menus/cheats/capture harnesses are not exposed accidentally.
- [ ] Version shown in project/export/package metadata is consistent.
- [ ] Windows build launches from a fresh directory without editor-only files.

## Dependencies and assets

- [ ] Godot addons are reviewed before updating and pinned/committed intentionally.
- [ ] Asset licenses and attribution obligations are recorded.
- [ ] AI-generated or externally sourced media has provenance/usage notes where needed.
- [ ] New large source/master binaries follow `docs/ASSET_STORAGE.md`.

## CI

- [ ] CI runs headless import/parse.
- [ ] CI performs a Windows release export.
- [ ] Build artifact is retained for inspection.
- [ ] Actions use maintained versions.
- [ ] CI permissions remain read-only unless a release job specifically requires write access.

## Steam gate

Before Steam deployment is automated, require:

- Steam App ID and depot IDs
- reviewed `app_build_*.vdf` / depot VDFs
- protected publishing credentials
- a staging/default branch policy
- a rollback build
- confirmation that achievements/cloud hooks use the production App ID only in release configuration

Never commit the publisher password or Steam Guard material.
