#ifndef GAMEOVER_H
#define GAMEOVER_H

#include "defines.h"
#include "variable.h"
#include "player.h"     // Level 2 merge: playerHealth, to pick the right message below
#include "gameState.h"  // Level 2 merge: currentLevel, to scope that message to Level 2 only

// STEP 12 (Game Over): failure screen shown when the puzzle timer runs out.
// "Press ESC to go back" per the spec; ESC is polled with GetAsyncKeyState
// in controls.h's handleRealtimeInput() for an immediate response, same
// approach already used elsewhere in this project for VK_ESCAPE/'B'.
inline void renderGameOver() {
	iSetColor(20, 0, 0);
	iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	iSetColor(220, 30, 30);
	iText(SCREEN_WIDTH / 2 - 160, SCREEN_HEIGHT / 2 + 40, "GAME OVER", GLUT_BITMAP_TIMES_ROMAN_24);

	iSetColor(220, 220, 220);
	// Level 2 merge: gated on currentLevel == 2 (not just playerHealth <= 0)
	// so Level 1's game-over text is byte-for-byte unchanged in every case,
	// including the pre-existing edge case where a Level 1 enemy bullet has
	// already brought playerHealth to 0 by the time the puzzle timer runs out.
	if (currentLevel == 4) {
		// Level 4 merge: the new level's only loss condition is running
		// out of health, or running out of attempts at the final seal.
		iText(SCREEN_WIDTH / 2 - 190, SCREEN_HEIGHT / 2 - 10, "The grove swallowed you whole.", GLUT_BITMAP_HELVETICA_18);
	}
	else if (currentLevel == 2 && playerHealth <= 0) {
		iText(SCREEN_WIDTH / 2 - 190, SCREEN_HEIGHT / 2 - 10, "You were defeated before reaching the seal.", GLUT_BITMAP_HELVETICA_18);
	}
	else {
		iText(SCREEN_WIDTH / 2 - 190, SCREEN_HEIGHT / 2 - 10, "The puzzle went unsolved in time.", GLUT_BITMAP_HELVETICA_18);
	}

	iSetColor(255, 255, 255);
	iText(SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 60, "Press ESC to go back", GLUT_BITMAP_HELVETICA_18);
}

// Success screen shown once the puzzle is solved.
inline void renderLevelWin() {
	iSetColor(0, 15, 10);
	iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	iSetColor(0, 255, 150);
	iText(SCREEN_WIDTH / 2 - 220, SCREEN_HEIGHT / 2 + 40, "PUZZLE SOLVED - LEVEL COMPLETE!", GLUT_BITMAP_TIMES_ROMAN_24);

	iSetColor(220, 220, 220);
	if (currentLevel == 4) {
		iText(SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 10, "You felled the Grove Warden and", GLUT_BITMAP_HELVETICA_18);
		iText(SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 35, "broke the three-stage seal.", GLUT_BITMAP_HELVETICA_18);
	}
	else {
		iText(SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 10, "You defeated the enemy and", GLUT_BITMAP_HELVETICA_18);
		iText(SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 35, "cracked the ruins' seal.", GLUT_BITMAP_HELVETICA_18);
	}

	iSetColor(255, 255, 255);
	iText(SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 - 90, "Press ENTER to Continue, or ESC to go back", GLUT_BITMAP_HELVETICA_18);
}

#endif // GAMEOVER_H
