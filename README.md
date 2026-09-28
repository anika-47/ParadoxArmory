# Paradox Armory

## Game Description

**Paradox Armory** is a 2D side-scrolling action-shooter with puzzle gates, created using the **iGraphics** library in C/C++. The player fights through four levels, defeating enemies and bosses, dodging hazards, and cracking a puzzle at the end of every level to unlock the next one. The project demonstrates graphics programming concepts like sprite animation, scrolling cameras, collision detection, enemy AI, file handling, and audio.

```
Main Menu -> Level Select -> LEVEL 1 -> LEVEL 2 -> LEVEL 3 (3 parts) -> LEVEL 4 : Nine-Tails Seal
```

## Features
- Four levels with increasing difficulty, each ending in a puzzle.
- Mouse-aimed shooting (left click) or keyboard shooting (`F`) with a bullet pool.
- Walking, jumping, crouching, and shooting animations for the player.
- Enemy AI with chasers, ranged shooters, fast dashers, and mini-bosses.
- Environmental hazards: swinging balls, a spiked saw, spikes, blades, crushers, energy pylons, and chakra wisps.
- Multiple puzzle types: memory match, 4-digit code lock, sliding-tile puzzle, and a 5-stage logic puzzle.
- Level 3 is split into three parts with a "proceed to next part?" prompt.
- Level 4 is a long scrolling stage with 8 sections, gates, checkpoints, and a two-phase mini-boss.
- Health bars for the player and the enemy/boss.
- Level progression (unlock system) and a level-complete / game-over flow.
- Player health is saved to a text file and a binary file (`playerhealth.txt` / `playerhealth.dat`) and shown on the Level Select screen.
- Background music, menu music, and sound effects.


## Project Details
IDE: Visual Studio 2013 (toolset v120)

Language: C, C++.

Platform : Windows PC.

Genre : 2D action adventure / shooter with puzzles

Screen : 1200 x 650


## How to Run the Project

Make sure you have the following installed:
- **Visual Studio 2013**
- **iGraphics Library** (included in this repository: `iGraphics.h`, `glut.h`, `glut32.lib`, `GLUT32.DLL`, `glaux.h`, `Glaux.lib`)


Open the project in Visual Studio 2013
- Open Visual Studio 2013.
- Go to File → Open → Project/Solution.
- Locate and select `Paradox Armory.sln` from the cloned repository.
- Click Build → Rebuild Solution
- Run the program by clicking Debug → Start Without Debugging (`Ctrl + F5`)

> **Note:** Run the game from Visual Studio (or from the `Paradox Armory` project folder) so it can find the `images` and `Audios` folders, `bg.bmp`, and `img.bmp`.


## How to Play

### **Controls**
| Action | Key / Mouse |
|--------|-------------|
| **Move Left** | `A` |
| **Move Right** | `D` |
| **Jump** | `W` |
| **Crouch** | `S` (hold) |
| **Shoot** | `F` or `Left Mouse Button` |
| **Aim** | Move the mouse cursor |
| **Back to Level Select** | `B` or `Right Click` (while playing) |
| **Continue** (Level Complete screen) | `Enter` |
| **Go back / Exit** | `Esc` |

All puzzles are **mouse-only** (click the buttons, cards, tiles, and plates on screen).


### **Levels**

| Level | Player HP | What you face | Level ends with |
|-------|-----------|---------------|-----------------|
| **Level 1** | 10,000 | A single enemy (10,000 HP) | Memory-match puzzle (12 cards, 25 seconds) |
| **Level 2** | 20,000 | A stronger enemy (20,000 HP) and a swinging ball hazard (400 damage per hit) | Memory-match puzzle |
| **Level 3** | 30,000 | Three parts (see below) | Memory-match puzzle after Part 3 |
| **Level 4** | 20,000 | 8 scrolling sections of enemies, traps, and a two-phase mini-boss | The Nine-Tails Seal (5-stage puzzle) |

**Level 3 parts**
1. **Part 1** - Defeat a swarm of 10 melee and ranged enemies while using three floating platforms.
2. **Part 2** - A tough enemy whose health is broken into segments; every time a segment falls, a puzzle interrupts the fight (a 4-digit code lock, then a sliding-tile puzzle).
3. **Part 3** - The toughest enemy (30,000 HP) plus a fast enemy, swinging balls, and a spiked saw.

After Part 1 and Part 2 you are asked whether you want to proceed to the next part.

**Level 4 - Nine-Tails Seal**
- **Enemies:** Chasers (melee), Ranged shooters, Fast dashers, and a Heavy mini-boss with four attack states.
- **Obstacles:** stone blocks (jump), low bars (crouch), moving crushers, spikes, pendulum blades, and section gates.
- **Gates & checkpoints:** clearing a section opens its gate and repairs 300 HP.
- **Final area:** chakra wisps and energy pylons guard the altar after the mini-boss falls.
- **The Nine-Tails Seal puzzle:** five stages (Witnesses, Chain, Echo, Logic lock, Final seal) with five attempts shared across all stages. A wrong input costs one attempt and resets only the current stage.


### **Game Rules**

- Each level starts with the player at full health (see the table above).
- Enemies and hazards reduce the player's health when they hit; Level 4 gives about one second of invulnerability after every hit.
- Defeat the enemy (or clear all sections in Level 4) to trigger the level's puzzle.
- Solve the puzzle to win the level. Running out of time (or attempts in Level 4) ends the game.
- In Levels 2, 3, and 4 the game is over when the player's health reaches 0.
- Winning a level unlocks the next one.
- On the **LEVEL COMPLETE** screen, press `Enter` to continue to the next level or `Esc` to return to Level Select.

### **Player Health Files**

Whenever a level is completed, the game is lost, or the player leaves a level, the health is saved to:

| File | Format |
|------|--------|
| `playerhealth.txt` | Readable text (`fprintf` / `fscanf`) |
| `playerhealth.dat` | Binary records (`fwrite` / `fread`) |

Both keep the **last 10 records**. The newest record is shown on the Level Select screen. Delete both files to start fresh.


## Project Structure

```
Paradox Armory - Complete/
├── Paradox Armory.sln          # Visual Studio 2013 solution
├── Paradox Armory/             # Project folder
│   ├── iMain.cpp               # Entry point, draw loop, input hooks
│   ├── player.cpp / .h         # Player movement, shooting, animation
│   ├── enemy.cpp / .h          # Enemy logic for Levels 1-3
│   ├── obstacle.cpp / .h       # Swinging balls and saw (Levels 2-3)
│   ├── level4.cpp / .h         # Level 4: world, enemies, boss, obstacles
│   ├── puzzle.cpp / .h         # Memory match, code lock, sliding puzzle
│   ├── puzzle4.cpp / .h        # The Nine-Tails Seal (5-stage puzzle)
│   ├── playerhealth.cpp / .h   # Health save/load (text + binary files)
│   ├── controls.h, game.h, ui.h, gameover.h, defines.h, variable.*
│   ├── Audio.h, audio4.h       # Music and sound effects
│   ├── iGraphics.h, glut.h, glaux.h, stb_image.h
│   ├── images/                 # Sprites, backgrounds, bullets
│   └── Audios/                 # Music and sound effects (.mp3)
├── Tests (headless)/           # Optional logic tests (not part of the VS project)
└── *_NOTES.md                  # Design and merge notes
```


## Project Contributors

1. Samira Islam(00725105101131)
2. Anika Tasnim(00725105101133)
3. Arham Bin Zaheed(00725105101145)


## Screenshots

### **Menu**

<img src="ADD_MENU_SCREENSHOT_LINK" width="200" height="200">

### **Gameplay**

<img src="ADD_GAMEPLAY_SCREENSHOT_LINK" width="200" height="200">

## Youtube Link
[Paradox Armory -]()

## Project Report
[Project Report: Paradox Armory]()
