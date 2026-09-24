# Player health files

File handling for the player's health, written in the same style as the two example programs (a text file with
`fprintf` / `fscanf`, and a binary file of structs with `fwrite` / `fread`).

## What it does

Whenever a level is **completed**, the game is **lost**, or the player **leaves a level** (Back / menu / NO at the
Level 3 part prompt / closing the game), the player's health is saved to two files in the folder the game runs from
(next to `images`, i.e. the project folder when started from Visual Studio):

| File | Format | Written with | Read with |
|---|---|---|---|
| `playerhealth.txt` | readable text | `fprintf` | `fscanf` |
| `playerhealth.dat` | binary, one 24-byte `PlayerHealthRecord` per record | `fwrite` | `fread` |

Both hold the same **last 10 records**, oldest first. When the game starts, `playerhealth.dat` is read; if it is missing
or damaged, `playerhealth.txt` is used instead. The **Level Select** screen shows the newest record in a small panel under
the buttons (nothing is shown until a record exists), e.g.

```
Last run: Level 2 - level complete
Health: 14200 / 20000   (5 saved)
```

Example `playerhealth.txt`:

```
PLAYER HEALTH FILE
Records: 2

Level: 1
Part: 0
Health: 7400
MaxHealth: 10000
Result: COMPLETE

Level: 3
Part: 2
Health: 0
MaxHealth: 30000
Result: GAMEOVER
```

`Result` is `COMPLETE`, `GAMEOVER` or `LEFT`. `Part` is only used by Level 3 (otherwise 0).
To start fresh, delete the two files.

## It records health; it does not change it

Gameplay is untouched: damage, the full-health start of every level, controls, progression and all level content are
exactly as before. (Each level deliberately starts at full health, so *restoring* saved health at launch would change
the game; this only keeps the record. That could be added if you want it.)

## Files and hooks

| Change | Where |
|---|---|
| new `playerhealth.h`, `playerhealth.cpp` | project folder; added to `Paradox Armory.vcxproj` and `.filters` |
| `#include "playerhealth.h"` | `iMain.cpp` |
| `PlayerHealthInit();` (load saved records, arrange the save-on-exit) | `main()` in `iMain.cpp` |
| `PlayerHealthUpdate();` (once per tick) | `fixedUpdate()` in `iMain.cpp` |
| `PlayerHealthDrawLast();` | `iDraw()`, Level Select case, in `iMain.cpp` |

The per-tick call **watches `gameState`** in one place, so no level code (Levels 1-4, the puzzles) had to be touched.
Leaving through `exit()` (ESC while playing, the END key, the EXIT button, closing the window) is caught with `atexit`.

## Robustness

A file read from disk is only trusted if every field is in range (level 1-4, part 0-3, `0 <= health <= maxHealth`, a valid
`Result`, and a magic number in the binary version). A cut-off or hand-edited file keeps the valid records before the first
bad one; a missing or unwritable file never crashes the game (a write failure is only logged to the console).
Health is clamped to `0..maxHealth` when recorded.

## Verification (headless; not built with MSVC, not played on screen)

* all 11 source files compile as C++03 and link; the new files produce **no** warnings under `-Wall -Wextra -pedantic`
  (`fopen`/`fscanf`/`sprintf` are fine with the project's SDL checks because `playerhealth.cpp` includes `gameConfig.h`
  first, which defines `_CRT_SECURE_NO_WARNINGS`, like the other source files)
* `test_playerhealth`: 48 checks, 0 failures (six deliberately broken variants each fail it)
* `test_healthflow`: 24 checks, 0 failures - a real Level 1 -> 2 -> 3 -> 4 playthrough with one loss, then both files are
  read back from disk and compared
* re-run unchanged and passing: `test_progression` (69), `test_puzzle4` (29), `test_seal4`; Level 4's playthrough and layout
  are still identical to the Nine-Tails ZIP; Levels 1-3 state hashes are identical to before
