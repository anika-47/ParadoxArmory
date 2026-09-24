# Headless test harness

These files are **not part of the Visual Studio project** and are never compiled by it.
They exist so the Level 4 logic can be run and verified without a display.

`windows.h`, `mmsystem.h` and `iGraphics.h` here are minimal stubs that stand in for the
real platform and rendering headers. The stub `windows.h` exposes `AsyncKeyTable()`, which
is what `GetAsyncKeyState` reads — that is the *only* way the bot touches the game. It
cannot write `playerX`, `playerHealth` or any enemy state, so anything it manages to do a
real player can do with the keyboard.

## Running them

Copy this folder's contents next to the project's `.cpp`/`.h` files in a scratch directory
(the stubs must shadow the real headers), then:

```sh
g++ -std=c++03 -fpermissive -w -I. -o test_level4 \
    test_level4.cpp iGraphicsGlobals.cpp player.cpp gameState.cpp \
    variable.cpp enemy.cpp obstacle.cpp puzzle.cpp

./test_level4       # full-damage difficulty probe
./test_level4 g     # immortal probe: is the level completable end to end?
./test_level4 g v   # ...with a per-250-frame trace

g++ -std=c++03 -fpermissive -w -I. -o test_puzzle4 \
    test_puzzle4.cpp iGraphicsGlobals.cpp player.cpp gameState.cpp \
    variable.cpp enemy.cpp obstacle.cpp puzzle.cpp

./test_puzzle4      # exits non-zero if any assertion fails
```

`test_level4.cpp` and `test_puzzle4.cpp` `#include "level4.cpp"` and `"puzzle4.cpp"`
directly so they can read file-static state (the enemy list, the obstacle list, the boss,
the generated puzzle answer) for assertions.

`botlogic.inc` holds the bot itself: target selection with hysteresis, facing discipline,
bounded kiting, obstacle probing and hazard escape. It is shared by both harnesses.

See `../LEVEL4_NOTES.md` §8 for what these found.

---

## Added in the final merge (Levels 1-3 + Nine-Tails Seal Level 4)

| File | Kind | What it checks |
|---|---|---|
| `test_progression.cpp` | pass/fail | Start -> Level 1 -> 2 -> 3 (all three parts) -> 4 through the **real** `handleMouseInput` / `handleKeyboardInput` / `updateGameLogic` / puzzle click handlers: locks at start, locked buttons inert, a *loss* never unlocks, each win unlocks exactly the next level, Continue (ENTER) goes to the new level, ESC still returns to Level Select |
| `test_puzzle4.cpp` | pass/fail | the five-stage **Nine-Tails Seal** (replaces the old three-stage Grove Seal test) |
| `test_seal4.cpp` | pass/fail | the two final-approach hazards (wisp, pylon) and the altar |
| `test_playerhealth.cpp` | pass/fail | the player-health files (`playerhealth.cpp`): text + binary round trips, the 10-record cap, missing / cut-off / garbage / hand-edited files, what triggers a record, save-on-exit. Uses scratch files only |
| `test_healthflow.cpp` | pass/fail | the same module inside a real Level 1 -> 4 playthrough (with a loss), then reads both files back from disk. `#include`s `enemy.cpp`, `puzzle.cpp`, `level4.cpp`, `puzzle4.cpp` (leave them off the g++ line) |
| `test_layout_dump.cpp` | diff tool | dumps Level 4's obstacles / spawns / altar / player start / a generated puzzle; diff against a build of the original Nine-Tails ZIP |
| `test_regress_l123.cpp` | diff tool | seeded 6000-frame playthroughs of Levels 1, 2, 3.1, 3.2, 3.3; diff against a build of the original Enemy Facing sources |

Build lines are in each file's header. `test_progression.cpp` `#include`s `level4.cpp`, `puzzle4.cpp`,
`enemy.cpp` and `puzzle.cpp` (to read their file-static state), so leave those four **off** its
`g++` command line:

```sh
g++ -std=c++03 -fpermissive -w -I. -o test_progression test_progression.cpp \
    iGraphicsGlobals.cpp player.cpp gameState.cpp variable.cpp obstacle.cpp
./test_progression     # exits non-zero on any failure

g++ -std=c++03 -fpermissive -w -I. -o test_seal4 test_seal4.cpp iGraphicsGlobals.cpp \
    player.cpp gameState.cpp variable.cpp enemy.cpp obstacle.cpp puzzle.cpp
./test_seal4
```

The stub `iGraphics.h` in this folder now also contains **no-op OpenGL stubs**
(`glEnable`, `glBegin`, ...). They are required because the Enemy Facing work in `enemy.cpp`
(`DrawTankFacing`) mirrors a sprite with raw GL calls. They exist only for these harnesses; the
real game gets them from `glut.h`.

Build lines for the two player-health tests (each is also in the file's header):

```sh
g++ -std=c++03 -fpermissive -w -I. -o test_playerhealth test_playerhealth.cpp \
    playerhealth.cpp iGraphicsGlobals.cpp player.cpp gameState.cpp variable.cpp
g++ -std=c++03 -fpermissive -w -I. -o test_healthflow test_healthflow.cpp \
    playerhealth.cpp iGraphicsGlobals.cpp player.cpp gameState.cpp variable.cpp obstacle.cpp
```
