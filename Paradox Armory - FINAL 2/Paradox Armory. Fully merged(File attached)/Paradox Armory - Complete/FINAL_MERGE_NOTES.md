# Paradox Armory - final merge (Levels 1-3 + Nine-Tails Seal Level 4)

One project, one continuous game:

```
Main Menu -> Level Select -> LEVEL 1 -> LEVEL 2 -> LEVEL 3 (3 parts) -> LEVEL 4 : NINE-TAILS SEAL
```

* **Levels 1-3** are the *Paradox Armory - Enemy Facing* project, unchanged.
* **Level 4** is the *Paradox Armory - Level 4 (Nine-Tails Seal)* implementation, unchanged
  (`level4.cpp/.h`, `puzzle4.cpp/.h`, `audio4.h` are byte-for-byte the ZIP's files, plus its 27 Naruto sprites).
* Open `Paradox Armory.sln` in Visual Studio 2013 (toolset v120, unchanged). Do a clean **Rebuild**:
  the old `Debug/` build output was removed because it contained the previous Paradox Grove build.

## Progression

| | at first launch | unlocked by |
|---|---|---|
| Level 1 | open | - |
| Level 2 | LOCKED | winning Level 1 |
| Level 3 | LOCKED | winning Level 2 |
| Level 4 | LOCKED | winning Level 3 (its **final** puzzle, after Part 3) |

* "Winning" = solving the level's final puzzle. A lost puzzle or a game over never unlocks anything, and
  finishing a *part* of Level 3 is not finishing Level 3.
* On the **LEVEL COMPLETE** screen, **ENTER (Continue)** now starts the level that was just unlocked;
  **ESC** returns to Level Select exactly as before. After Level 4 (the last level) ENTER returns to Level Select.
* Progress is held in memory only (as in the original project): closing the game re-locks Levels 2-4.
* It reuses the project's existing system - the `level2Unlocked` / `level3Unlocked` / `level4Unlocked` flags,
  `STATE_LEVEL_WIN`, the Level Select screen - rather than adding a second one.

## What changed (relative to the Enemy Facing project)

| File | Change |
|---|---|
| `level4.cpp`, `level4.h`, `puzzle4.cpp`, `puzzle4.h`, `audio4.h` | replaced by the Nine-Tails ZIP's versions (byte-identical) |
| `images/naruto_*.png` (27 files) | added (Naruto sprites + `naruto_flash.png`, used by Level 4 and its puzzle screen) |
| `player.cpp` | **Naruto additions only**: sprite loading, animation timers, `DrawNarutoPlayer()`, a `level == 4` branch in `SetPlayerCharacter()`, and a `narutoActive` branch in `RenderPlayer()`. Every branch for Levels 1-3 is untouched (`narutoActive` is false for them). |
| `variable.cpp` | `level2Unlocked`, `level3Unlocked`, `level4Unlocked` now start `false` (they were all `true`, a testing state) |
| `puzzle.cpp` | one added line at each of the three level-completion sites: `else if (selectedLevel == 3) level4Unlocked = true;` (Level 3 -> Level 4 was missing) |
| `controls.h` | the four "start a level" sequences moved, call-for-call, into one `StartLevel(n)` helper used by the Level Select buttons **and** by Continue; ENTER on LEVEL COMPLETE continues to the next level |
| `Debug/` (both) | stale build output removed |
| `Tests (headless)/`, `FINAL_MERGE_NOTES.md`, `FINAL_PUZZLE_NOTES.md`, `NARUTO_MOD_NOTES.md` | tests / docs (not part of the VS project) |

Nothing else differs from the Enemy Facing project. `enemy.cpp`, `obstacle.cpp`, `ui.h`, `game.h`,
`gameover.h`, `iMain.cpp`, `player.h`, the `.vcxproj`/`.filters`/`.sln` and all Level 1-3 art are as shipped.

### Deliberately NOT taken from the Level 4 ZIP
The ZIP was forked from an older point in the codebase. Copying these would have removed Level 1-3 work:

* `enemy.cpp`, `obstacle.cpp`, `puzzle.cpp`, `controls.h` - older versions without the enemy-facing, Level 3
  (parts, hazards, puzzles) and debug-helper work. Level 4 needs only `enemyPic[]`, `tankImg` and
  `enemyBulletImage` from `enemy.cpp`, which the Enemy Facing version still provides.
* `images/ch3_b1..b7.png`, `level3_part2_background.jpg` - older Level 3 art.
* Two lines in the ZIP's `player.cpp`: the 20000 default HP (Levels 1-3 set their own HP at start, and Level 4
  sets 20000 itself in `InitLevel4()`, so Level 4 is unaffected) and the older `ch3_b7/ch3_b8` left-facing
  frame names (Enemy Facing uses `ch3_b8/ch3_b9`).

## Verification that was run

All headless (a stub `windows.h` / `iGraphics.h`, `g++ -std=c++03` as a stand-in for VS2013's compiler):

* all 10 translation units compile, and link together with no duplicate or missing symbols
* `test_progression` - **69 checks, 0 failures** (also confirmed to *fail* when the Level 3->4 unlock line is removed,
  when the flags are left `true`, and when a Level 1 win also unlocks Level 3)
* `test_puzzle4` (29 checks) and `test_seal4` - pass, unchanged from the Level 4 ZIP
* full Level 4 playthrough bot (`test_level4`, immortal and full-damage): the merged build's output is
  **identical** to the original Nine-Tails ZIP's (kills, boss frames, section timings, damage, score, frame the
  puzzle opens)
* Level 4 layout dump: all 41 obstacles, 54 spawns, the altar (world x 9480), player start and a generated
  Nine-Tails puzzle are **identical** to the ZIP's
* Levels 1, 2 and 3 (all three parts): seeded 6000-frame playthroughs give **identical** state hashes on the
  original Enemy Facing sources and on the merged sources
* Level Select -> Level 1/2/3 produces identical game state before and after the `StartLevel` refactor
* every image / audio path referenced by the code exists; every file in the `.vcxproj` exists

## Not verified / known items

* **Not compiled with MSVC / VS2013 and not played on screen.** No Windows or GLUT was available. The build
  and playthrough checks above are compile-and-logic checks only; please do a real build and a quick visual pass.
* **Level 4 win / game-over text is stale.** `gameover.h` still says "You felled the Grove Warden and broke the
  three-stage seal" / "The grove swallowed you whole". The Nine-Tails ZIP has this same text; it was left as-is
  because Level 4 was to be preserved. It is two `iText` lines in `gameover.h` if you want it updated.
* **Level 3 `N` debug key** (jumps to the next Level 3 part) is existing Enemy Facing behaviour and was kept.
  It cannot unlock Level 4 (only the final puzzle can), but you may want to remove it for release
  (`controls.h`, `handleKeyboardInput`).
* To test Level 4 without playing through, temporarily set the flags in `variable.cpp` to `true`.

---

## Update after the merge: player health files

Two new project files (`playerhealth.h`, `playerhealth.cpp`) plus four one-line hooks in `iMain.cpp` and the two
`.vcxproj` / `.filters` entries. Nothing else in the game changed (this version has no Batman change).
See `PLAYER_HEALTH_NOTES.md`.
