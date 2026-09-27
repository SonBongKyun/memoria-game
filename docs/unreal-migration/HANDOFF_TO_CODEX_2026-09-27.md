# Handoff to Codex — Claude pauses after S309 (2026-09-27)

The user asked Claude to stop after S309 and hand the work to Codex, so the two do not edit at once. Claude does not write in either lane after this handoff until the user says so. Everything below is the state at the S309 commit (full rendered 413/413, visual 7/7) on `SonBongKyun/memoria-unreal-claude` (see `git log`).

## 1. Take over the code (one command)

- The Codex lane `C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex` is at `3130ec5a` and clean.
- `3130ec5a` is an ancestor of the Claude branch, so a fast-forward brings in everything:

  ```
  git -C C:/Users/jc/orca/workspaces/Game/memoria-unreal-codex merge --ff-only SonBongKyun/memoria-unreal-claude
  ```

- **Nothing is pushed.** The Claude branch is 9 local commits ahead of `origin` (S303–S309 plus two earlier ones). Push only when the user asks. `foundation` stays read-only.
- **Git commits:**
  - Use `git -c gc.auto=0 -c maintenance.auto=false commit ...`. The repository sits on OneDrive, and automatic repacks have failed on it before.
  - Write commit messages to a file first.
- **Line endings:** `git stash` and `pop` turn some files CRLF; that is harmless.

## 2. The user's decisions that now govern the work

- **Combat pivot (2026-09-27).** The Unreal game drops the Godot turn-based battle and becomes **Diablo-style quarter-view action combat on the field map**:
  - WASD moves; the mouse aims attacks; number keys fire skills; there is a dodge.
  - Characters are **rigged 3D models with animations**.
  - **Memory burn is a powerful skill.** Burning a memory fires a skill tied to it, and the memory is lost permanently. Higher grades hit harder: the source's "power for a permanent cost".
  - The illustrated `field_hd` cards stay as dialogue and UI portraits.
  - **S309 (turn-based presentation) was committed only as a stopgap**, so the game still plays until action combat replaces the encounter flow. Do not polish the turn-based battle further. Its rules data (enemy stats, damage, witness, break) can be reused.
- **No paid subscriptions or new accounts for tools.** Free, local tools only.
- **Canon:** Sable is a blind old woman.
- **Language:** communicate with the user in Korean.

## 3. Open work, in order

**A. 3D models (Codex; the request is in the shared `claude-handoff.md`).**
- Arrel, Elia and Malet go through free tools: Stable Fast 3D or TripoSR, then Blender.
- **Not Hunyuan3D:** its license excludes South Korea.
- The steps are an A-pose sheet, then shape, cleanup and projection texture, then an unrigged FBX, then QA renders.
- The user rigs on Mixamo (free) following a Korean guide, or falls back to Blender Rigify.
- Deliver to the shared `models/<id>/`.

**B. Action combat foundation (next implementation story; Claude had planned it as S310).**
1. **Mannequin stand-in.**
   - Run `python Unreal/Tools/install_mannequin.py`. It copies Epic's UE 5.8 mannequin from the engine templates into `Unreal/Memoria/Content/Characters/Mannequins` (about 126 MB, already git-ignored in `.gitignore`).
   - The copy includes `SKM_Manny_Simple`, `SKM_Quinn_Simple` and `SK_Mannequin`, plus the `Unarmed` animations: `BS_Idle_Walk_Run`, `MM_Attack_01..03`, `MM_ChargedAttack`, `MM_Dash`, `MM_HitReact_Front_Lgt_01` and `MM_Death_Front_01`.
   - Check with `--check`.
   - It keeps the template mount path `/Game/Characters/Mannequins`.
2. **3D field figures.** Arrel, Elia and Malet become mannequins (tinted placeholders) instead of the `UMemoriaFieldCharacterComponent` cards.
   - A single-node blend space by speed is enough to start; add a C++ `UAnimInstance` for montage slots when attacks need blending.
   - Where to hook in:
     - `AMemoriaVerdanPresentation::BeginPlay`: `ArrelFigure`, `MaletCard`, fill lights;
     - `AMemoriaEliaCompanion`: its `Figure`;
     - movement: `AMemoriaFieldPawn` (FloatingPawnMovement) and `AMemoriaVerdanPresentation::Tick`.
3. **Combat core.**
   - A combat component: HP, faction, damage, hit-stun and death.
   - The player: a 3-hit combo toward the cursor, with a sweep during each swing's active window; a dash with i-frames.
   - `AMemoriaMonster`: idle, aggro radius, chase, telegraphed attack, hit reaction, death.
   - Damage numbers, hitstop and camera shake. A field HP HUD.
4. **Encounters.** On a Verdan revisit, `AMemoriaSliceController::Tick` currently calls `Battle->BeginEncounter(...)`, which opens the turn-based widget. Spawn a monster wave in the field instead. Keep the BattleCore subsystem only as the stopgap until the wave works end to end.
5. **Then** burn skills, with the grade-scaled power and permanent loss shown in the archive. Then the real models from A: import them, then IK-retarget the mannequin animations (UE auto-retargets Mixamo skeletons).

**C. Deferred.**
- The field exploration HUD after `exploration_hud.gd`: the "Arrel · Journey" header, chapter, HP and story flow. It would replace the dev "VERDAN / THE GRAY BELT" status panel. Tests do not assert that panel's text.
- Field lighting polish.
- Options not ported yet: text speed, difficulty, accessibility.
- The priority-2 field illustrations are on hold (3D replaces them in the field).

## 4. Build, test and assets

- **Serialize.** Never run two UnrealEditor or UBT processes at once, and never edit source while a validation run is building.
- **Build:** `python Unreal/Tools/validate_unreal.py --build-only`.
- **Full rendered registry:** `python Unreal/Tools/validate_unreal.py --build-and-test --rendered --automation-timeout 3600 --evidence-dir <dir>`.
  - It runs `Memoria.` and takes about 30–35 minutes. S309 runs **413** tests.
  - Results are in `<evidence-dir>/unreal_validation.json` → `automation_report` (index.json).
- **Visual suite, separately:** `--test-prefix MemoriaVisual.`, 7 tests. `Memoria.` does not match `MemoriaVisual.`.
- **Useful subsets:** `--test-prefix` with `Memoria.BattleCore.` (101), `Memoria.Title.`, `Memoria.Chapter1.` or `Memoria.Foundation.`.
- **Custom mixes:** bypass the validator's choices by running `UnrealEditor-Cmd` with `-ExecCmds=Automation RunTests A.+B.` and the same flags. The validator's report JSON shows the exact command.
- **Asset commandlets** (`UnrealEditor-Cmd <uproject> -run=<Name> -unattended -nop4 -NullRHI`):
  - `MemoriaDialogueAssets`, `MemoriaAudioAssets`, `MemoriaFontAssets`, `MemoriaFieldCharacterAssets` (`-Force` replaces), `MemoriaBattleEntryAssets` (now additive) and `MemoriaBattleStageAssets` (`M_BattlePlate` UI material; `-Force` replaces).
  - All are additive unless noted.
- **Play:** `UnrealEditor.exe Memoria.uproject -game` opens the title (`LocalMapOptions=?Title`).
- **Layout units:** UMG lays out in 1080p units at 720p, so Godot lengths ×1.5 and font points ×1.12 (see `MemoriaTitleWidget.cpp`).
- **Colours:** Godot 2D colours are sRGB; convert them with `MemoriaUiKit::Srgb`.

## 5. Known issues (read before judging a red run)

- **Memory.** On 2026-09-27 a full run died with `Ran out of memory` (a page-file limit). Other programs held about 25 GB committed: MapleStory 7.3 GB, OneDrive 9.1 GB, and fm, vmmem and others. Ask the user to close heavy programs before long runs.
- **`Memoria.Foundation.MapInputAndModal`** failed at `Keyboard moves +X` in all three S309 full runs, and only in full runs:
  - the injected `D` never registered (`KEYBOARD_TRACE ... pressed=0`);
  - it passes in isolation (2/2), after Firebomb, and after BattleCore+Firebomb (every subset rerun passed);
  - the S308 full run (412/412) was before the user started a game; the S309 runs were during it.
  - Resolved: with the heavy programs closed, the S309 full run passed **413/413**. Treat this failure as host load (a lost input under slow frames); rerun on a quiet machine before suspecting code.
- **`python -m unittest` in `Unreal/Tools`** has 16 pre-existing failures (stale counts, for example `narrative_test_paths` 11 vs 8). They are unrelated to recent work.
- The Claude lane does not ignore `.godot/`. Running Godot captures there would create an import cache; the existing Godot captures are in the main repo's `tmp/visual_audit/`.

## 6. Where things are (S303–S309)

- **Chapter 1 route and New Game:**
  - `MemoriaNarrativeSubsystem` (`StartNewGame`, `ResumeChapterAutosave`, `EnterTitle`);
  - `MemoriaCheckpointSubsystem` (chapter slot, `FindContinue`);
  - oracle fixtures in `docs/unreal-migration/fixtures/chapter1`.
- **VN:** `MemoriaDevelopmentNarrativeWidget` (story mode), `MemoriaFonts`, `MemoriaUiKit`.
- **Field:**
  - `MemoriaFieldCharacterComponent` (HD cards);
  - `MemoriaVerdanPresentation`;
  - `MemoriaEliaCompanion` (formation 90 and talk reach 122; user-approved deviations);
  - HD art imported through `MemoriaFieldCharacterAssets`.
- **Title and settings:** `MemoriaTitleWidget`, `Settings/MemoriaSettingsSubsystem`, and the title branch in `MemoriaSliceHost` (controller).
- **Battle (stopgap):** `MemoriaBattleEntryWidget` (stage plates through `M_BattlePlate`, the source HUD layout), `MemoriaBattleStageAssetsCommandlet`.
- **Progress log:** `docs/unreal-migration/MIGRATION_STATE.md`, newest first. Each session section lists changes, deviations, tests and results.
