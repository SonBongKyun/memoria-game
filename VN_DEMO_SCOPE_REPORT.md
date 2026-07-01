# VN Demo Scope Report

This pass keeps the current visual-novel demo intact and removes no legacy RPG files or autoloads.

## Confirmed current flow

`main.tscn` -> New Game -> `SceneFlow.pending_scene_id = "ch1_prologue"` -> `vn_host.tscn` -> `SceneFlow.play()` -> JSON scenes under `data/vn_scenes/`.

The old map and turn-based battle loop is not the core demo path.

## Required for the current VN demo

- `GameManager`: global flags, chapter, player data, and game state.
- `MemoryManager`: memory costs, burn state, erosion, and save data.
- `SceneFlow`: JSON scene playback and VN resume state.
- `SceneTransition`: title/VN/ending scene changes.
- `SaveManager`: slot persistence and VN resume preparation.
- `AudioManager`: title and VN audio calls.
- `PauseMenu`: in-VN Save/Load access.
- `OptionsMenu`: title and pause-menu settings access.
- `DialogueBox`: currently supplies `PORTRAIT_MAP` to `vn_scene.gd`.
- `NotificationToast`: save, memory, and reward feedback.
- `AchievementManager`: directly referenced by chapter-completion steps.
- `EliaDiary` and `TutorialHints`: currently direct SaveManager dependencies.
- `BattleManager`: legacy in concept, but SaveManager still directly connects its boss autosave signal. Guard that dependency before disabling the autoload.

## Legacy or non-critical for the current VN slice

- Exploration UI/systems: `ExplorationHUD`, `MemoryUI`, `SystemLog`, `MemoryShop`, `MemoryCompass`.
- Legacy game modes: `MemoryPuzzle`, `Codex`, `StoryJournal`, `MemoryConstellation`.
- Map/battle presentation: `CgViewer`, `WorldRewriteDirector`, most map scripts, and battle scenes.
- `Dialogic`: the current JSON SceneFlow/VN UI does not use it as the primary runner.

Some of these remain useful to the larger project. “Non-critical” means only that the current Chapter 1 VN demo does not need them as its main path.

## Recommended later cleanup

1. Guard or invert direct SaveManager references to `BattleManager`, `EliaDiary`, and `TutorialHints`.
2. Move the portrait registry out of the `DialogueBox` autoload into a small shared data/resource file.
3. Disable one non-critical autoload at a time in a dedicated demo export preset, with a headless boot after every change.
4. After the demo preset is stable, consider moving these into a `legacy/` archive without deleting history:
   - `scenes/maps/`
   - `scenes/battle/`
   - map-specific scripts under `scripts/utils/`
   - legacy exploration dialogue and encounter data not referenced by `data/vn_scenes/`

No archive move or autoload removal is part of this stability pass.
