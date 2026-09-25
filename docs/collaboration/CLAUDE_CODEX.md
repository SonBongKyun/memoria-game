# MEMORIA Unreal: Claude + Codex collaboration

## Current project
This pair works on the Unreal migration, UE 5.8.2. The Godot description later in AGENTS.md / CLAUDE.md is legacy reference, not a request to resume Godot development.
Read docs/unreal-migration/MIGRATION_STATE.md, PLAYABLE_SLICE.md and the latest SESSION_LOG.md entries before implementation. Historical validation results are not validation of this new snapshot.

## Locations and default responsibilities
- Claude: C:/Users/jc/orca/workspaces/Game/memoria-unreal-claude. Implement one user-assigned, bounded task in this worktree and branch only.
- Codex: C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex. Review Claude commits, validate, and integrate accepted commits in this worktree. Independent implementation is allowed only for explicitly disjoint files.
- Shared coordination: C:/Users/jc/Documents/Codex/MEMORIA-Unreal-Collaboration/BOARD.md.
- Handoffs: claude-handoff.md is written by Claude; codex-review.md by Codex in that same shared directory. Read the other file but do not overwrite it.
- Original Unreal checkout: C:/Users/jc/MemoriaMigration/foundation. Read-only reference for this collaboration. Its untracked evidence archives stay there.
- Original Godot checkout: C:/Users/jc/OneDrive/바탕 화면/메모리아/Game. Read-only behavioral reference. Manuscripts and images live in its parent directory; old ../ relative links do not resolve from the new worktrees.

## Start and ownership
No development task is assigned by this setup. Wait for the user's task rather than inventing features.
Before editing, the coordinator (Codex by default) records task, owner, allowed paths, acceptance criteria and base commit in BOARD.md. Claude reads that assignment and acknowledges in its own handoff. Do not edit the same files concurrently. Shared headers, project settings, migration fixtures, SESSION_LOG.md and binary .uasset/.umap files require one named owner for the current task.
Both agents may read the peer worktree, but only its owner writes or runs Git mutations there. Never reset, clean, stash, force-push or delete a peer's work. Do not touch the original checkouts or old ORCA worktrees.

## Handoff and integration
Use small, task-owned commits on your assigned branch. Record SHA, changed paths, actual commands/results, unresolved issues and next action in your handoff. Do not claim old test results as fresh.
Codex reviews with git show / git diff before integrating. With a clean own worktree, cherry-pick accepted SHA(s) into the Codex branch and rerun relevant checks. Stop on conflicts and resolve deliberately; never use reset --hard to bypass them.
For the next task, record the accepted Codex SHA in BOARD.md. Claude may fast-forward from the Codex branch if possible; if histories diverge after cherry-picks, merge the reviewed Codex branch into its own clean branch, resolving intentionally. Do not rebase shared work without agreement.
No automatic merge into unreal-migration/ue58-foundation and no remote push, release or deployment as part of setup. Follow later explicit user shipping instructions.

## Unreal and validation
Open Unreal/Memoria/Memoria.uproject from your own worktree. Binaries, Intermediate, Saved and DDC must stay per-worktree; do not symlink these mutable directories between agents. Serialize expensive editor/build/automation sessions using the coordinator's BOARD.md build slot. Never terminate another agent's editor or compiler.
Inspect Unreal/Tools/validate_unreal.py --help for the current supported checks. Source-derived migration must preserve executable Godot contracts, original content and existing save behavior; uncertain drafts are not approved canon.
This setup verifies worktree isolation, snapshot content and CLI startup only. It does not certify gameplay or a successful Unreal build.
