# Paradox Armory — Merge Notes

Base project: **Source C** (`Paradox Armory`). Level 1 gameplay merged in from
**Source A** (`parad1_fixed_repaired`). Audio, name entry, hover, and puzzle
concepts merged in from **Source B** (`Game-Development--main`).

## Final flow implemented

Name Entry → Continue → Main Menu → Level Select → Level 1 → Enemy Defeated →
Puzzle → Success (Continue) or Failure (Game Over) → ESC → Return to Level
Select.

## What changed, file by file

- **defines.h** — added new state constants (`STATE_NAME_INPUT`,
  `STATE_PUZZLE`, `STATE_GAME_OVER`, `STATE_LEVEL_WIN`); pulled in
  `gameConfig.h` (Level 1's screen-size constants) and `mmsystem.h` (for
  audio). Nothing existing was removed.
- **variable.h** — re-centered the existing buttons for the new window size
  (constants only, same `Button` struct, same click logic); added name-entry
  state, mouse-position tracking, audio toggles, and the `puzzleShown` flag.
- **ui.h** — `drawButton()` now highlights on hover (STEP 8); `renderGameplay()`
  now calls Level 1's real `RenderPlayer()`/`RenderEnemies()` instead of the
  placeholder text; menu backgrounds now draw via a scaled `iShowImage()`
  instead of the native-size `iShowBMP()` (see resolution note below); all
  other menu/instructions/about text shifted by a constant offset to stay
  centered on the larger canvas — a translation, not a redesign.
- **controls.h** — rewritten to route input per state. Every branch that
  existed before (`playBtn`/`instructionBtn`/`aboutBtn`/`exitBtn`,
  `lvl1Btn`/`backBtn`, `'B'`-to-go-back, ESC-to-exit) is preserved exactly;
  new branches were added for name input, Level 1 forwarding, puzzle clicks,
  and the game-over/level-win ESC handling.
- **game.h** — was an empty stub; now calls Level 1's own `updatePlayer()` /
  `UpdateEnemyLogic()` each tick during `STATE_PLAY`, and hooks the
  enemy-defeated → puzzle transition (STEP 11), guarded by `puzzleShown` so
  it fires once.
- **enemy.cpp** — one targeted edit: removed the auto-advance-to-next-level
  call on enemy death (since Level 2/3 aren't part of this build); the
  enemy's health/state logic itself is untouched.
- **player.cpp, player.h, gameConfig.h, gameState.cpp/h, bitmap_loader.h,
  images/** — copied from Source A unmodified.
- **Audio.h, nameinput.h, puzzle.h, gameover.h** — new files, each ported
  from the corresponding Source B system (`Audio.hpp`, the name-entry screen
  in `Leaderboard.hpp`, the memory-match puzzle in `Puzzles.hpp`) but
  rewritten against Source C's own `Button`/`drawButton` system instead of
  duplicating a second UI system. The memory-match puzzle was chosen because
  it was the one puzzle in Source B that didn't depend on that project's
  other Level 2/3 systems.
- **iMain.cpp** — rewritten to dispatch draw/input by state, track mouse
  position for hover, and load all assets (Level 1 + audio + menu textures)
  at startup.
- **.vcxproj / .vcxproj.filters** — updated to include the newly-added and
  copied source/header files so the project picks them up in Visual Studio.

## Key decision: window resolution

Source C's window was 800×600, with two backgrounds baked to that exact
size. Source A's Level 1 gameplay has 1800×900 (`SCREEN_WIDTH`/`SCREEN_HEIGHT`
from `gameConfig.h`) baked into player movement, bullet, and collision math
throughout `player.cpp`/`enemy.cpp`.

Rather than rewrite that gameplay math (explicitly against the brief), the
window is now sized to Level 1's native 1800×900. The two Source C
backgrounds are scaled up via `iShowImage()` to fill the new canvas, and the
existing menu button/text coordinates were re-centered by constant
offsets. No gameplay logic changed to accommodate this.

## Level 2 / Level 3

Kept visible in the Level Select screen, labeled "(LOCKED)", but their
buttons have no click handler — effectively disabled rather than deleted, per
the brief's preference.

## Fix: LNK2005 duplicate-symbol errors (first build attempt)

The first real build (done outside this environment, no MSVC available here)
turned up ~185 `LNK2005 "... already defined in iMain.obj"` errors across
`player.obj`/`enemy.obj` — every single `iGraphics.h` function (`iCircle`,
`iRectangle`, `iShowBMP`, ...) plus every `stb_image` internal symbol.

Root cause: Source C's `iGraphics.h` was written assuming only **one** `.cpp`
file would ever include it (true in the original Paradox Armory project,
where `iMain.cpp` was the only compiled source file). Its functions aren't
`inline` and its globals aren't `extern` — fine for one translation unit,
but once `player.cpp`/`enemy.cpp`/`gameState.cpp` were added as their own
compiled files (each pulling in `iGraphics.h`), every one of those symbols
got compiled into multiple `.obj` files, and the linker rejected the
duplicates. The `STB_IMAGE_IMPLEMENTATION` macro was also being defined
unconditionally inside Source C's `iGraphics.h`, so every TU that included
it also got its own full copy of the stb_image implementation — same
problem, much bigger blast radius (100+ of the errors).

Source A's own copy of `iGraphics.h` had already been fixed for exactly
this: it declares its globals `extern` (with the one real definition split
out into `iGraphicsGlobals.cpp`) and marks every function `inline`, and its
`iMain.cpp` is the only place that defines `STB_IMAGE_IMPLEMENTATION`. That
project already compiles `player.cpp`/`enemy.cpp`/`gameState.cpp`/`iMain.cpp`
as four separate files successfully, so it's the correct header for this
merged project's multi-file structure too.

Fix applied:
- Replaced this project's `iGraphics.h` with Source A's multi-TU-safe
  version (verified line-by-line: functionally identical, just
  `inline`/`extern` added and a header guard).
- Added `iGraphicsGlobals.cpp` (Source A's file providing the one real
  definition of the `extern` globals) to the project.
- Moved `#define STB_IMAGE_IMPLEMENTATION` to the top of `iMain.cpp` only
  (matching Source A's convention), removed from the header.
- Updated `.vcxproj`/`.vcxproj.filters` to compile `iGraphicsGlobals.cpp`.
- Also silenced a `C4800` warning in `ui.h`'s new hover code (`int` result
  from `isClicked()` assigned to a `bool` — added an explicit `!= 0`).

## ⚠️ Compile check — not run

This environment has no Windows, MSVC, or GLUT/GLUT32 available, so **I was
not able to actually compile or run this project**. I've done a careful
manual review instead:

- Verified every function call I added against its actual declared
  signature in `player.h`/`enemy.h`/`gameState.h`.
- Verified `updatePlayer()` internally calls Level 1's own
  `HandlePlayerRealtimeInput()` (which owns ESC-to-exit and WASD movement)
  so `controls.h` doesn't double-handle those.
- Verified include-guard/single-translation-unit structure: all the new
  headers define real (non-`extern`) globals, matching this project's
  existing pattern — safe because only `iMain.cpp` includes them, while
  `player.cpp`/`enemy.cpp`/`gameState.cpp` remain separate translation units
  that never see those headers.
- Checked asset paths (`images\...`, `Audios\...`) match where the files
  actually are on disk.

I'd still recommend opening it in Visual Studio and doing an actual build
before considering this done — a static read-through can miss things a
compiler catches immediately (a typo, an off-by-one on a struct field), and
I want to be upfront that step wasn't possible here.

## Suggested test order (from the original brief)

1. Game launches → name input shows
2. Type a name → Continue button appears → click it → Main Menu
3. Menu music plays; hover highlights buttons
4. Level Select → Level 1 (2/3 show as locked)
5. Level 1 starts, background music switches, WASD + mouse-aim + click/F to
   shoot all work, ESC exits (Level 1's original behavior, unchanged)
6. Kill the enemy → puzzle appears automatically, once
7. Solve it → "LEVEL COMPLETE" screen → Enter or ESC returns to Level Select
8. Let the puzzle timer run out instead → "GAME OVER" screen → ESC returns
   immediately (no click needed)

---

# Level 2 Merge (Paradox Armory 1.0 + Level 2 Final)

This section documents merging the separate "Paradox Armory Level 2 Final"
build into the project above. Level 1 was not redesigned or rewritten; see
"What changed, file by file" below for the exact, minimal touch points.

## Final flow after this merge

Main Menu → Level Select → Level 1 (unlocked from the start) → Level 1
victory → Level 2 unlocks on the Level Select screen → Level 2 → Level 2
victory/defeat, same as Level 1's own win/lose screens.

Level 2 is **locked** on a fresh run. There is no existing save/progression
system in this project (progression was always in-memory only, reset on
relaunch), so the unlock flag (`level2Unlocked` in `variable.h`) follows
that same existing pattern rather than introducing a new persistence layer.

## Key decision: reuse Level 1's puzzle/progression system instead of Level 2's own

The standalone Level 2 build had its own separate systems for the
end-of-level puzzle (`puzzle.h`/`puzzle.cpp`, "Guardian's Alignment" —
rotate three statues) and its own audio header. Both use the **same
function names** as this project's own, already-wired puzzle system
(`InitPuzzle()`, an `AUDIO_H` include guard, etc.) — including both would
be either a straight naming collision or two silently-different
same-named functions depending on inclusion order, neither of which is
safe.

Rather than rename around the collision and maintain two parallel puzzle
systems, this merge reuses Paradox Armory 1.0's own memory-match puzzle
for **both** levels, since it was already level-agnostic: `game.h`'s
`updateGameLogic()` triggers it purely off `enemyLife <= 0`, with no
level-specific logic. This matches the brief's own instruction to reuse
the existing progression system "rather than creating a separate
duplicate system." Level 2's own puzzle/audio files were therefore **not**
copied into this project.

## What changed, file by file

- **player.h / player.cpp** — replaced with the Level 2 build's own
  version of these files, which is a proper superset: it adds a second
  character sprite set (`walkFramesRightSet2`/`walkFramesLeftSet2`) and a
  `SetPlayerCharacter(level)` function that swaps the active sprite set,
  bullet image, and bullet size/offset. Its Level 1 branch (the `else` in
  `SetPlayerCharacter()`) was adjusted to reproduce Level 1's original,
  unconditional values exactly (`gunHeightRatio = 0.5f`, 15×6 bullet,
  `bullet.png`) — calling it for Level 1 is a no-op change in behavior.
- **enemy.cpp** — two targeted edits: `InitEnemyLevel()` now calls
  `SetPlayerCharacter(level)`, and the player-bullet-vs-enemy collision
  check now uses the active `bulletWidth`/`bulletHeight` (from player.h)
  instead of the old hardcoded `15, 6` — for Level 1 this evaluates to the
  same `15, 6` as before, so Level 1's collision is unchanged; Level 2's
  larger bullet now registers hits correctly. Level 2's max enemy health
  branch (`20000`) was already present in this file from the original
  Level 1 merge and needed no change.
- **obstacle.h / obstacle.cpp** (new) — the Level 2 black-ball hazard,
  carried over essentially unmodified. One change: it no longer includes
  a second `audio.h` (which would collide with this project's own
  `Audio.h`/`variable.h` if pulled into a second translation unit — see
  the LNK2005 section above for exactly this failure mode); instead it
  plays its one sound with a small, self-contained, `static` MCI call of
  its own, so it stays safe as an independently-compiled `.cpp` file like
  `player.cpp`/`enemy.cpp`/`gameState.cpp` already are. Gated on
  `currentLevel == 2` throughout, so it is a no-op during Level 1.
- **variable.h** — added `bool level2Unlocked = false;`, the single
  source of truth for whether Level 2 can be entered.
- **gameState.h / gameState.cpp** — added `backgroundImg2`, Level 2's own
  background texture, alongside the existing `backgroundImg`.
- **ui.h** — `renderGameplay()` now draws `backgroundImg2` "contain"-fit
  (uniformly scaled, centered, letterboxed) when `currentLevel == 2`,
  since that art's aspect ratio doesn't match the canvas; Level 1's own
  stretch-to-fill `backgroundImg` draw is untouched. Also calls
  `RenderObstacle()` (no-op outside Level 2). `renderLevelSelect()` now
  draws a local copy of `lvl2Btn` with its label swapped to "LEVEL 2"
  once `level2Unlocked` is true — the underlying `lvl2Btn.text` stays
  "LEVEL 2 (LOCKED)" as the single source of truth for `controls.h`'s
  click check.
- **controls.h** — added an `else if (level2Unlocked && ...)` branch
  alongside the existing Level 1 branch in the `STATE_LEVEL_SELECT` mouse
  handler: sets `selectedLevel = 2`, resets `puzzleShown`, calls
  `InitEnemyLevel(2)` and `InitObstacle()`, and starts play — mirroring
  the existing Level 1 branch exactly. Level 2's button has no effect at
  all while locked (same "kept visible, no handler" treatment Level 3
  already had).
- **puzzle.h** — one line added where the memory-match puzzle is solved:
  `if (selectedLevel == 1) level2Unlocked = true;`. This is the **only**
  place Level 2 gets unlocked, and it fires only on an actual Level 1
  win, never on a loss (puzzle timeout) or from Level 2's own puzzle
  completion.
- **game.h** — `updateGameLogic()` now also calls `UpdateObstacle()`
  (no-op outside Level 2), and checks `currentLevel == 2 && playerHealth
  <= 0` to send the player to `STATE_GAME_OVER` — Level 2's own loss
  condition. This check is scoped to Level 2 only: Level 1 never checked
  `playerHealth` for a loss before this merge (a pre-existing property of
  the base project, left untouched), so Level 1's behavior is unchanged.
- **gameover.h** — `renderGameOver()`'s message now depends on
  `currentLevel == 2 && playerHealth <= 0`: a new "defeated before
  reaching the seal" line for a Level 2 health-loss, otherwise the
  original "puzzle went unsolved in time" text Level 1 has always shown
  (and still always shows, since the new condition can't be true there).
- **iMain.cpp** — loads `backgroundImg2` and calls
  `LoadObstacleAssets()` (currently a no-op — the hazard is drawn
  procedurally) alongside the existing Level 1 asset loads.
- **images/**, **Audios/** — added Level 2's character frames
  (`ch2_1..7.png`, `ch2_b1..7.png`), its bullet (`bullet2.png`), its
  background (`level2_background.bmp`), and its obstacle-hit sound
  (`bubble.mp3`). No existing Level 1 asset was replaced or renamed.
- **.vcxproj / .vcxproj.filters** — added `obstacle.h`/`obstacle.cpp` as
  project items, same pattern as the other source/header pairs.

## What was deliberately *not* carried over from the Level 2 build

- Its own `puzzle.h`/`puzzle.cpp` ("Guardian's Alignment") and `audio.h`
  — see "Key decision" above.
- Its standalone `iMain.cpp`, `controls.h`-equivalent input routing, and
  win/lose screens — this project's own state machine, input routing,
  and `gameover.h` screens are reused instead, exactly as the brief asked
  ("reuse existing managers ... rather than creating a separate duplicate
  system").

## ⚠️ Compile check — not run

Same caveat as the Level 1 merge above: this environment has no Windows,
MSVC, or GLUT available, so this could not actually be compiled or run.
This was a careful manual, file-by-file review instead — every new call
site was checked against its actual declared signature, and the
multi-translation-unit safety of every new/changed `.cpp` file
(`obstacle.cpp` in particular) was checked against the same rules that
caused the LNK2005 errors documented above for the original Level 1
merge. I'd still recommend an actual build in Visual Studio before
considering this done.

## Suggested test order

1. Fresh launch → Level Select → Level 1 available, Level 2 shows
   "LEVEL 2 (LOCKED)" and does nothing when clicked.
2. Play Level 1, let the puzzle timer run out (lose) → back at Level
   Select → Level 2 is still locked.
3. Play Level 1 again, solve the puzzle (win) → Level 2's button now
   reads "LEVEL 2" and is clickable.
4. Click Level 2 → Level 2's background/character/enemy load, the
   black-ball hazard swings and can be dodged (crouch or reposition) or
   can damage the player.
5. Let the hazard/enemy bring player health to 0 → "GAME OVER" screen
   with the Level 2-specific message → ESC returns to Level Select
   (Level 2 remains unlocked).
6. Defeat the Level 2 enemy → the same memory-match puzzle appears →
   solve it → "LEVEL COMPLETE" screen.
7. Return to Level 1 → confirm it still looks, sounds, and plays exactly
   as before this merge (background stretch, bullet size/offset, no
   health-based game over, same win/lose screens and text).
