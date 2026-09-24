#include "gameState.h"

// Actual memory allocation/definition for the extern variables
int currentLevel = 1;
int backgroundImg = 0;
int backgroundImg2 = 0; // Level 2 merge: Level 2's own background texture

int backgroundImg3a = 0;
int backgroundImg3b = 0;
int backgroundImg3c = 0;

int currentPart = 1;

int enemySegmentsCleared = 0;
bool inSegmentPuzzle = false;

const int NUM_L3P1_PLATFORMS = 3;
Platform level3Part1Platforms[NUM_L3P1_PLATFORMS] = {
	{ 250, 250, 200 },
	{ 550, 380, 200 },
	{ 850, 250, 200 }
};
int showPart2Prompt = 0;
int proceedToPart2 = 0;