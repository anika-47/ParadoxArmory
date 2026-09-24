#ifndef VARIABLES_H
#define VARIABLES_H

#include "defines.h"

#define UI_CENTER_X (SCREEN_WIDTH / 2 - 100)

extern int gameState;
extern int selectedLevel;

extern Button playBtn, instructionBtn, aboutBtn, exitBtn;
extern Button lvl1Btn, lvl2Btn, lvl3Btn, lvl4Btn, backBtn;
extern Button yesBtn, noBtn;

extern int curMouseX;
extern int curMouseY;

extern bool musicOn;
extern bool sfxOn;

extern int bgMenuImg;
extern int bgLevelImg;

extern bool puzzleShown;
extern bool puzzleSolved;

extern bool level2Unlocked;
extern bool level3Unlocked;
extern bool level4Unlocked;

#endif