# MEMORIA Unreal: Opus + Gemini + Grok team (from 2026-11-22)

Codex is paused. This team replaces the Claude + Codex pair (`CLAUDE_CODEX.md` stays as history).
The game is the Unreal 5.8.2 project under `Unreal/Memoria`. The Godot notes in AGENTS.md / CLAUDE.md are legacy reference only.
Talk to the user in Korean.

## Lanes (one Orca worktree each)
| Agent | Worktree | Branch | Role |
|---|---|---|---|
| **Opus** (Claude) | `C:/Users/jc/orca/workspaces/Game/memoria-unreal-claude` | `claude/s366-upgrade` | Coordinator, main implementer, reviewer, integrator. The only one who pushes. |
| **Gemini** (in the Antigravity app) | `C:/Users/jc/orca/workspaces/Game/memoria-unreal-gemini` | `gemini/memoria-review` | Canon and manuscript review: compares ported scenes, quests and text against the final manuscript; writes review notes; edits text or docs only when assigned. |
| **Grok** | `C:/Users/jc/orca/workspaces/Game/memoria-unreal-grok` | `grok/memoria-lane` | Implements tasks Opus assigns that do not overlap Opus's files (combat, environment, tools, tests), on its own branch. |

Gemini runs in Google's Antigravity app, not as an Orca terminal agent: Google no longer serves Gemini CLI to personal accounts (2026-11-22). Antigravity opens the Gemini worktree folder and reads `.agent/rules/memoria-team.md`.

`claude/s366-upgrade` (pushed to `origin`) is the integration line. Never force-push it. The old `SonBongKyun/memoria-unreal-claude` / `-codex` branches and the Codex worktree are not ours to change.

## Shared coordination
Folder: `C:/Users/jc/Documents/Codex/MEMORIA-Unreal-Collaboration/`
- `TEAM-BOARD.md`: Opus writes it. Holds the current assignments (task, owner, allowed paths, acceptance criteria, base commit) and the **build slot**.
- `claude-handoff.md` (Opus), `gemini-handoff.md` (Gemini), `grok-handoff.md` (Grok). Each agent writes only its own file, newest entry on top, with a one-line Korean summary for the user.
- `BOARD.md`, `codex-review.md` belong to Codex. Read only.

## Rules
1. **Wait for an assignment.** Work only on what `TEAM-BOARD.md` or the user gives you. Acknowledge it at the top of your handoff before you start.
2. **Stay in your lane.** Write and run git only in your own worktree and branch. You may read the other lanes. Never reset, clean, stash, rebase or delete another agent's work. Never touch the Godot checkout (`C:/Users/jc/OneDrive/바탕 화면/메모리아/Game`) or the Codex worktree.
3. **No overlapping files.** Each assigned task names its allowed paths. Shared headers, project settings, `.uasset` / `.umap` files, `Unreal/Tools/export_chapter_maps.py` and `MemoriaManuscriptScenes.cpp` have one named owner per task.
4. **One Unreal build at a time.** The PC runs short of RAM. Before `Build.bat`, an editor, a commandlet or an automation run, take the build slot in your handoff (`BUILD SLOT: taken by <agent> <time>`), check `TEAM-BOARD.md` that nobody else holds it, and release it when done. Never kill another agent's editor or compiler.
5. **Commit small, push never.** Commit on your branch with a clear message. Only Opus pushes, after review.
6. **Report what actually ran.** Each handoff entry has: SHA, changed paths, exact commands, results (pass counts, evidence folder), open issues, next step. No old results presented as new.
7. **Integration.** Opus reviews with `git show` and cherry-picks accepted commits onto `claude/s366-upgrade`, reruns the checks, and pushes. Lanes then catch up with `git merge --ff-only claude/s366-upgrade`, or a plain merge when Opus asks.

## Canon (the final manuscript wins)
- Source of truth: Notion "MEMORIA | 최종본 (KR)" (65화), with local copies in `C:/Users/jc/OneDrive/바탕 화면/메모리아/최종본/MEMORIA_KR_ChNNN_final.md`.
- Ported so far: game chapters 6-16 = 6-26화 (see `docs/unreal-migration/S371_REVIEW.md` … `S393_REVIEW.md`).
- **Sable** is a blind old woman. No seeing verbs for her (hear, touch, know). Use only art that shows her blind.
- **Tobias** appears from 23화 only. Reject pictures that show extra companions.
- The brown-coat man is unnamed, with no portrait.
- Elia gives the echo shell (10화). Sable gives the small spiralwave shell.

## Build and test (Opus and Grok)
- Build: `"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" MemoriaEditor Win64 Development "-Project=<your worktree>/Unreal/Memoria/Memoria.uproject" -WaitMutex -NoHotReloadFromIDE -MaxParallelActions=1`. A new test `.cpp` may need a second build. The first build of a new worktree takes about 10 min.
- Tests (they do not build): `py -3 Unreal/Tools/run_s365_native.py --tests "<filters joined by +>" --evidence Unreal/Memoria/Saved/Validation/<fresh dir> --timeout N`.
- Wide regression: `"MemoriaVisual.+Memoria.S36+Memoria.S37+Memoria.S38+Memoria.S39+Memoria.Journal+MemoriaSave.S362+Memoria.Foundation.MapInputAndModal"` (97 tests at S393).
  - Each `Memoria.S3x` prefix catches only its own decade: `S38` does not catch `S39x`. Add the next decade when its first test lands, and add any test outside these prefixes by name.
- Use `py -3`. `python3` is a Store stub here.

## Gemini's review checklist
For each assigned chapter: for every `ms_*` scene line, check speaker, Korean wording against the manuscript, scene order, and canon. Also check the quest/goal text and the chosen art. Report as `file:line — what the game says — what the manuscript says (화, paragraph) — proposed fix`.
