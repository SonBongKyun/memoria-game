# S369 Korean patch (Claude, 2026-11-20)

Branch `claude/s366-upgrade`, on top of S368 (`9951d0ae`). The user asked for a 100% Korean patch. Korean is the default language (`UMemoriaSettingsSubsystem::Locale = "ko"`). The story data (the IR generated from the Godot source) already carried Korean. What remained English in Korean mode was fixed text in the C++ presentation, plus a few data lines without a Korean twin.

## Approach
- New `MemoriaKorean` (`Public/Localization/MemoriaKorean.h`, `Private/Localization/MemoriaKorean.cpp`):
  - `IsKorean(context)`: the active run's locale, else the settings.
  - `Text(english)`: an exact-match table, then patterns (prompts `Name  |  E / A: verb`, `+N Grains`, `+N Hi-Potion`, `Quest Complete: X`, the development-boundary notice, scene headers), then `MemoriaChapterMaps::Korean`. Unknown text passes through.
  - `MemoryText(id)`: Korean memory titles and descriptions from the catalog's `LocalizedText` and the chapter memories.
  - `Grains(n)`: the currency label in either language.
- Applied **where text is drawn**: the field HUD's prompt, quest line and notices; the dialogue view (header, place, choices, stop screen) and its frame's own words; combat popups; the reward line; HUD rows; the shop, archive and Codex; the achievement list and toast; the title kicker; the journal's compass readings.
- The English strings underneath are unchanged. The Godot-oracle and English tests still compare English; the English locale is untouched.

## Newly Korean
- **HUD:** `Ch.N` → `N장`, `Grains` → `그레인` (HUD, pause info, reward line, popups, shop prices).
- **Achievements:** all 38 titles and descriptions, the header and the "unlocked" toast. The Godot source has none; translated here.
- **Saves:** all ~50 checkpoint, save and load status lines as shown on the stop screen.
- **Dialogue frame:** `NEXT`, the controls lines, `YOUR CHOICE`, `PAUSED`, the scene header and place titles (`CHAPTER II / VERDAN` → `2장 / 베르단`, the Sump, Malet, the market beats).
- **Prompts:** Malet, Elia, the nervous trader, the loose stone, the memory stalls, the old man, Malet's table, the Sump stairs.
- **Notices:** the Sump ledger quest and the development-boundary notice.
- **Codex:** memory titles and descriptions (from the catalog's Korean; previously the English source titles were shown).
- **Chapter ledger:** burned memory names.
- **Shop and archive widgets:** frames, buttons, hints and counts.
- **Journal:** the 12 loss compass readings and their `Lost:` / `Eroding:` / `Erosion warning:` prefixes. The source has no Korean; translated here.
- Terms follow the game's own Korean: 그레인, 웅덩이, 관리국, 분류자, 벨트 중계소, 드리프트, 앵커링, 기억 나침반.

## Verification (2026-11-20, native UE 5.8.2 / D3D 1280x720)
- New `MemoriaVisual.S369KoreanSweep`: the Verdan slice in Korean. It captures the arrival scene, HUD, pause, options, journal (6 tabs), Codex (2 tabs), achievement / quit steps, the elite, the reward and game over.
  - Before the patch (`s369/sweep_before`, `sweep_before2`): English in the HUD, the toast, the dialogue frame and the Codex memories.
  - After (`s369/sweep_after1`): those screens inspected and Korean.
  - Its achievement-list, archive and burn-picker steps don't open their screens (key timing in the fixture); those screens were checked in the regression's own Korean captures instead.
- Wide regression: every `MemoriaVisual.*` test + `Memoria.S36*` + `Memoria.Foundation.MapInputAndModal` -> `s369/wide2`: **PASS 58/58**, exit 0, fatal 0, inputs unchanged.
  - Korean captures inspected: title, achievements, Chapter 3/4/5 fields, dialogue and completion cards, revisit, Korean burn picker, Korean demo completion.
  - `wide1` failed 1/58: `MemoriaVisual.Achievements` asserted the English header and "First Blood" in its Korean run, which was the untranslated state. The test now expects `업적 (n / 38)` and `첫 승리`.

## Limits
- The existing Korean story text (from the Godot IR) was not re-translated; some lines read like machine translation.
- Debug and development-only screens (validation smoke fixtures, the Malet placeholder box, recorded-observation overlays) stay English.
- An English run with Korean settings (a test-only state) shows a mix; in play the run takes its language from the settings.
- No package run.
