# Asset Storage Policy

The repository already contains a large amount of binary art/audio history. This stabilization pass does **not** rewrite existing Git history because that is disruptive and can break clones, branches, and collaborators.

## Rule for new heavy source assets

Prefer storing editable/master media in dedicated source directories and track them with Git LFS before the files are first committed.

Recommended paths for future source media:

- `assets/source/` — PSD/Blend/high-resolution source art
- `assets/audio/master/` — lossless/master audio
- `assets/video/` — trailers and generated video masters

Runtime-optimized assets should stay as small as practical.

## Existing repository history

Do not run `git lfs migrate`, `git filter-repo`, or another history rewrite casually. If repository size becomes a blocking issue, perform a dedicated migration with:

1. a full backup/mirror
2. branch/tag inventory
3. explicit migration plan
4. fresh-clone verification
5. coordination for any other working copies

## Before adding media

- Check usage rights and generation/source provenance.
- Keep licenses/attribution where required.
- Avoid duplicate near-identical exports.
- Prefer one canonical source plus generated runtime derivatives.
- Never store Steam credentials, signing certificates, or API keys next to media.
