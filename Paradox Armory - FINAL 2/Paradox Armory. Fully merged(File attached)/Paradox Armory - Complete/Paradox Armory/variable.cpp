#include "variable.h"

int gameState = STATE_MAIN_MENU;
int selectedLevel = 0;

Button playBtn = { UI_CENTER_X, 500, 200, 50, "PLAY" };
Button instructionBtn = { UI_CENTER_X, 430, 200, 50, "INSTRUCTIONS" };
Button aboutBtn = { UI_CENTER_X, 360, 200, 50, "ABOUT US" };
Button exitBtn = { UI_CENTER_X, 290, 200, 50, "EXIT" };

Button lvl1Btn = { UI_CENTER_X, 500, 200, 50, "LEVEL 1" };
Button lvl2Btn = { UI_CENTER_X, 420, 200, 50, "LEVEL 2 (LOCKED)" };
Button lvl3Btn = { UI_CENTER_X, 340, 200, 50, "LEVEL 3 (LOCKED)" };
// Level 4 merge: new stage button; BACK moved down to make room for it.
Button lvl4Btn = { UI_CENTER_X, 260, 200, 50, "LEVEL 4 (LOCKED)" };
Button backBtn = { UI_CENTER_X, 190, 200, 40, "BACK TO MENU" };

Button yesBtn = { SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 50, 120, 50, "YES" };
Button noBtn = { SCREEN_WIDTH / 2 + 30, SCREEN_HEIGHT / 2 - 50, 120, 50, "NO" };

int curMouseX = 0;
int curMouseY = 0;

bool musicOn = true;
bool sfxOn = true;

int bgMenuImg = 0;
int bgLevelImg = 0;

bool puzzleShown = false;
bool puzzleSolved = false;

// All levels open from the menu regardless of progress. (Previously Levels 2,
// 3 and 4 started LOCKED and were unlocked, in order, only by completing the
// level before them -- see the still-intact 'selectedLevel == N' unlock lines
// in puzzle.cpp. That completion tracking is left in place; only the initial
// state below was changed so every level is accessible from the start.)
bool level2Unlocked = true;
bool level3Unlocked = true;
bool level4Unlocked = true;