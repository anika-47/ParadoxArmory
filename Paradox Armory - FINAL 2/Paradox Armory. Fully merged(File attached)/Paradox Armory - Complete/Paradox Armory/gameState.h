#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "gameConfig.h"

// Global Game State Declarations
extern int currentLevel;
extern int backgroundImg;
extern int backgroundImg2; // Level 2 merge: Level 2's own background texture (see gameState.cpp)
extern int backgroundImg3a;
extern int backgroundImg3b;
extern int backgroundImg3c;
extern int currentPart; // Level 3 merge: which of the 3 parts is active (1/2/3)

extern int enemySegmentsCleared;
extern bool inSegmentPuzzle;

struct Platform { int x, y, width; };
extern Platform level3Part1Platforms[];
extern const int NUM_L3P1_PLATFORMS;

extern int showPart2Prompt;
extern int proceedToPart2;

#endif // GAME_STATE_H