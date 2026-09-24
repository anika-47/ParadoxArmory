
#ifndef CONTROLS_H
#define CONTROLS_H

#include "defines.h"
#include "ui.h"        // also pulls in player.h / enemy.h (Level 1, unchanged) and obstacle.h
#include "variable.h"
#include "puzzle.h"
#include "puzzle4.h"  // Level 4 merge: HandlePuzzle4Click()
#include "level4.h"   // Level 4 merge: InitLevel4()
#include "Audio.h"

// External references for variables
extern int gameState;
extern int selectedLevel;
extern Button playBtn, instructionBtn, aboutBtn, exitBtn;
extern Button lvl1Btn, lvl2Btn, lvl3Btn, lvl4Btn, backBtn;
extern bool level2Unlocked;
extern bool level4Unlocked;

// Sequential progression: the ONE place a level is started. The Level Select
// buttons and "Continue" on the level-complete screen both call this, so both
// routes always run exactly the same set-up the Level Select buttons always
// ran (same calls, same order) -- nothing about how a level starts changed.
inline void StartLevel(int level) {
	selectedLevel = level;
	puzzleShown = false;

	if (level == 1) {
		InitEnemyLevel(1); // parad1's own reset logic, reused as-is
	}
	else if (level == 2) {
		InitEnemyLevel(2);
		InitObstacle();
	}
	else if (level == 3) {
		// Level 3 merge: same pattern as Level 2, plus resetting currentPart to 1
		currentPart = 1;
		InitEnemyLevel(3);
	}
	else if (level == 4) {
		// Level 4 merge: the level runs entirely through its own module,
		// so it only needs InitLevel4().
		InitLevel4();
	}

	gameState = STATE_PLAY;
	PlayBackgroundMusic();
}

inline void handleMouseInput(int button, int state, int mx, int my) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		if (gameState == STATE_MAIN_MENU) {
			if (isClicked(playBtn, mx, my)) gameState = STATE_LEVEL_SELECT;
			else if (isClicked(instructionBtn, mx, my)) gameState = STATE_INSTRUCTION;
			else if (isClicked(aboutBtn, mx, my)) gameState = STATE_ABOUT;
			else if (isClicked(exitBtn, mx, my)) exit(0);
		}
		else if (gameState == STATE_LEVEL_SELECT) {
			
			// Levels 2-4 are gated by their unlock flags, which start false and are
			// set only by completing the level before (see puzzle.cpp).
			if (isClicked(lvl1Btn, mx, my)) {
				StartLevel(1);
			}
			else if (level2Unlocked && isClicked(lvl2Btn, mx, my)) {
				StartLevel(2);
			}
			else if (level3Unlocked && isClicked(lvl3Btn, mx, my)) {
				StartLevel(3);
			}
			else if (level4Unlocked && isClicked(lvl4Btn, mx, my)) {
				StartLevel(4);
			}

			else if (isClicked(backBtn, mx, my)) {
				gameState = STATE_MAIN_MENU;
				PlayMenuMusic();
			}
		}
		else if (gameState == STATE_PART_CONFIRM) {
			if (isClicked(yesBtn, mx, my)) {
				currentPart++;               // was hardcoded to 2 -- now works for any part
				InitEnemyPart(currentPart);
				if (currentPart == 3) InitObstacle(); // Part 3's hazards (balls + saw) reset fresh
				gameState = STATE_PLAY;
			}
			else if (isClicked(noBtn, mx, my)) {
				gameState = STATE_LEVEL_SELECT;
				puzzleShown = false;
				PlayMenuMusic();
			}
		}
		else if (gameState == STATE_PLAY) {
			// STEP 10: Level 1 shoot-on-click, reused unmodified.
			HandlePlayerMouseInput(button, state, mx, my);
		}
		else if (gameState == STATE_PUZZLE) {
			handlePuzzleClick(mx, my);
		}
		else if (gameState == STATE_PUZZLE4) {
			// Level 4 merge: the final seal is mouse-only, so it can never
			// clash with the A/D/W/S/F gameplay controls.
			HandlePuzzle4Click(mx, my);
		}
	}

	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
		// Original "right-click to go back" behavior, preserved for the
		// same screens it already applied to.
		if (gameState == STATE_PLAY) {
			gameState = STATE_LEVEL_SELECT;
			PlayMenuMusic();
		}
		else if (gameState == STATE_INSTRUCTION || gameState == STATE_ABOUT) {
			gameState = STATE_MAIN_MENU;
			PlayMenuMusic();
		}
	}
}

inline void handleKeyboardInput(unsigned char key) {
	printf("Key Pressed: %c (%d)\n", key, key);


	if (gameState == STATE_PLAY) {
		// STEP 10: Level 1's own one-time key actions (ESC = exit, F =
		// shoot), reused unmodified.
		HandlePlayerKeyboardInput(key);

		// Debug/testing helper: 'N' jumps straight to the next Level 3 part
		// (or, once already on Part 3, resets that fight) -- so a part can
		// be tested directly instead of having to clear the ones before it.
		if ((key == 'n' || key == 'N') && currentLevel == 3) {
			if (currentPart < 3) currentPart++;
			InitEnemyPart(currentPart);
			if (currentPart == 3) InitObstacle(); // Part 3's hazards (balls + saw) reset fresh
		}

		if (key == 'b' || key == 'B') {
			gameState = STATE_LEVEL_SELECT;
			PlayMenuMusic();
		}
		return;
	}

	if (gameState == STATE_GAME_OVER) {
		if (key == 27) { // ESC -> Return (STEP 12)
			gameState = STATE_LEVEL_SELECT;
			puzzleShown = false;
			PlayMenuMusic();
		}
		return;
	}

	if (gameState == STATE_LEVEL_WIN) {
		if (key == 13) { // Enter (Continue)
			// Sequential progression: Continue goes straight on to the level this
			// win just unlocked (1 -> 2 -> 3 -> 4). After the last level -- or if
			// the next level is somehow not unlocked -- it does what it always did
			// and returns to Level Select.
			if (selectedLevel == 1 && level2Unlocked)      StartLevel(2);
			else if (selectedLevel == 2 && level3Unlocked) StartLevel(3);
			else if (selectedLevel == 3 && level4Unlocked) StartLevel(4);
			else {
				gameState = STATE_LEVEL_SELECT;
				puzzleShown = false;
				PlayMenuMusic();
			}
		}
		else if (key == 27) { // ESC -> Return (unchanged)
			gameState = STATE_LEVEL_SELECT;
			puzzleShown = false;
			PlayMenuMusic();
		}
		return;
	}

	if (key == 'b' || key == 'B') {
		if (gameState != STATE_MAIN_MENU) {
			gameState = STATE_MAIN_MENU;
			PlayMenuMusic();
		}
	}
	else if (key == 27) {
		exit(0);
	}
}

inline void handleRealtimeInput() {
	if (gameState == STATE_PLAY) {
		
		return;
	}

	if (gameState == STATE_GAME_OVER || gameState == STATE_LEVEL_WIN) {
		// STEP 12: immediate ESC response, no mouse click required.
		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
			gameState = STATE_LEVEL_SELECT;
			puzzleShown = false;
			PlayMenuMusic();
			Sleep(150);
		}
		return;
	}
	if (gameState != STATE_PUZZLE && gameState != STATE_PUZZLE4) {
		if (GetAsyncKeyState('B') & 0x8000) {
			if (gameState != STATE_MAIN_MENU) {
				gameState = STATE_MAIN_MENU;
				PlayMenuMusic();
			}
			Sleep(150);
		}

		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
			exit(0);
		}
	}
}

#endif
