#ifndef GAME_H
#define GAME_H

#include "defines.h"
#include "variable.h"
#include "player.h"   // updatePlayer() -- parad1 Level 1, unchanged
#include "enemy.h"     // UpdateEnemyLogic(), enemyLife -- parad1 Level 1, unchanged
#include "puzzle.h"    // updatePuzzle()
#include "obstacle.h"  // Level 2 merge: UpdateObstacle() (no-op outside Level 2)
#include "Audio.h"     // Level 2 merge: PlayGameOverMusic() for the health-loss game over below
#include "gameState.h" // Level 2 merge: currentLevel, for the Level-2-only loss check below
#include "level4.h"    // Level 4 merge: UpdateLevel4()
#include "puzzle4.h"   // Level 4 merge: UpdatePuzzle4()

// Fixed tick length in seconds, matching iSetTimer(16, fixedUpdate) in
// iMain.cpp -- used as "dt" for the puzzle timer.
#define TICK_DT 0.016f

// STEP 11 (Enemy Death -> Puzzle hook): the Level 1 gameplay logic itself
// (player.cpp/enemy.cpp) is untouched. The transition to the puzzle is
// wired here, in the integration layer, exactly like the spec's
// "if(enemyDead) { showPuzzle(); }" example, guarded by puzzleShown so it
// only fires once per enemy death rather than every frame.
inline void updateGameLogic() {
	if (gameState == STATE_PLAY) {

		// ---------------- Level 4 merge ----------------
		// The new level is fully self-contained: it drives its own
		// enemies, obstacles, waves, mini-boss and puzzle hand-off. It
		// runs updatePlayer() first (unchanged), then returns, so none
		// of the Level 1/2/3 logic below is touched by it.
		if (currentLevel == LEVEL4_ID) {
			updatePlayer();
			UpdateLevel4();

			if (playerHealth <= 0) {
				gameState = STATE_GAME_OVER;
				PlayGameOverMusic();
			}
			return;
		}

		updatePlayer();
		UpdateEnemyLogic();
		UpdateObstacle(); // Level 2 merge: no-op outside Level 2 (see obstacle.cpp)

		// Level 2 merge: Level 2's own loss condition (the black-ball hazard can
		// bring playerHealth to 0). Scoped to currentLevel == 2 only -- Level 1
		// never checked playerHealth for a loss before this merge, and still
		// doesn't, so Level 1's behavior is unchanged.
		if ((currentLevel == 2 || currentLevel == 3) && playerHealth <= 0) {
			gameState = STATE_GAME_OVER;
			PlayGameOverMusic();
			return;
		}
		bool partCleared = (currentLevel == 3 && currentPart == 1)
			? IsLevel3Part1Complete()
			: (enemyLife <= 0);

		if (partCleared) {
			if (currentLevel == 3 && currentPart < 3) {
				gameState = STATE_PART_CONFIRM; // ask before entering the next part
				return;
			}
			else if (!puzzleShown) {
				puzzleShown = true;
				InitPuzzle();
				gameState = STATE_PUZZLE;
			}
		}
	}
	else if (gameState == STATE_PUZZLE) {
		updatePuzzle(TICK_DT);
	}
	else if (gameState == STATE_PUZZLE4) {
		UpdatePuzzle4(TICK_DT);
	}
}

#endif