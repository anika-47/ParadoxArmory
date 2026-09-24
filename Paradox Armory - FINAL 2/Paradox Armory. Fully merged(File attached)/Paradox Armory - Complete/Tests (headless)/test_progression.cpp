// =====================================================================
// END-TO-END PROGRESSION TEST  (Level 1 -> 2 -> 3 -> 4)
// =====================================================================
// Build like the other harnesses (see README.md):
//   g++ -std=c++03 -fpermissive -w -I. -o test_progression test_progression.cpp \
//       iGraphicsGlobals.cpp player.cpp gameState.cpp variable.cpp obstacle.cpp
//
// (level4.cpp, puzzle4.cpp, enemy.cpp and puzzle.cpp are #included below so the
//  test can read their file-static state -- Level 4's obstacle list, Level 3
//  Part 1's kill counter, the memory-match cards. They must NOT also be on the
//  g++ command line.)
//
// What is real: handleMouseInput(), handleKeyboardInput(), updateGameLogic(),
// handlePuzzleClick(), StartLevel(), every InitXxx() and every unlock line.
// What is scripted: "defeating the enemy" is done by setting enemyLife = 0 (or
// Part 1's kill counter to its target) exactly where a bullet would have; the
// Level 4 fight itself is covered by the Level 4 suites (test_level4/puzzle4/seal4).
// =====================================================================
#include "defines.h"      // same include order as iMain.cpp
#include "variable.h"
#include "ui.h"
#include "controls.h"
#include "game.h"
#include "gameover.h"
#include "level4.cpp"
#include "puzzle4.cpp"
#include "enemy.cpp"
#include "puzzle.cpp"
#include <stdio.h>

static int failures = 0;
static int checks = 0;
static void check(bool c, const char* what) {
	checks++;
	printf("  [%s] %s\n", c ? " ok " : "FAIL", what);
	if (!c) failures++;
}

// ---- drive the game like a player ------------------------------------
static void click(const Button& b) {
	handleMouseInput(GLUT_LEFT_BUTTON, GLUT_DOWN, b.x + b.width / 2, b.y + b.height / 2);
}
static void pressKey(unsigned char k) { handleKeyboardInput(k); }
static void tick(int n) { for (int i = 0; i < n; i++) { handleRealtimeInput(); updateGameLogic(); } }

// Solve the memory-match puzzle through the real click handler.
static void solveMemoryPuzzle() {
	for (int s = 0; s < 6; s++) {
		int first = -1, second = -1;
		for (int i = 0; i < 12; i++) {
			if (puzzle.cards[i].symbolID == s) { if (first < 0) first = i; else second = i; }
		}
		Card& a = puzzle.cards[first];
		Card& b = puzzle.cards[second];
		handlePuzzleClick((int)(a.x + a.w / 2), (int)(a.y + a.h / 2));
		handlePuzzleClick((int)(b.x + b.w / 2), (int)(b.y + b.h / 2));
	}
}

// "Defeat the enemy" at the point a bullet would: the game loop takes it from there.
static void defeatEnemyAndTick() {
	enemyLife = 0;
	playerHealth = playerMaxHealth;
	tick(2);
}

static void renderEverythingOnce() {
	renderMainMenu(); renderLevelSelect(); renderInstruction(); renderAbout();
	renderGameOver(); renderLevelWin();
}

int main() {
	srand(12345);
	printf("=============== PROGRESSION TEST ===============\n");

	// ------------------------------------------------------------- fresh boot
	printf("\n-- fresh start (mirrors main(): InitEnemyLevel(1), main menu) --\n");
	InitEnemyLevel(1);
	check(gameState == STATE_MAIN_MENU, "game boots into the main menu");
	check(level2Unlocked == false, "Level 2 starts LOCKED");
	check(level3Unlocked == false, "Level 3 starts LOCKED");
	check(level4Unlocked == false, "Level 4 starts LOCKED");
	click(playBtn);
	check(gameState == STATE_LEVEL_SELECT, "PLAY opens Level Select");

	printf("\n-- locked levels cannot be entered from Level Select --\n");
	click(lvl2Btn);  check(gameState == STATE_LEVEL_SELECT && selectedLevel == 0, "clicking locked LEVEL 2 does nothing");
	click(lvl3Btn);  check(gameState == STATE_LEVEL_SELECT && selectedLevel == 0, "clicking locked LEVEL 3 does nothing");
	click(lvl4Btn);  check(gameState == STATE_LEVEL_SELECT && selectedLevel == 0, "clicking locked LEVEL 4 does nothing");
	renderLevelSelect();

	// ---------------------------------------------- Level 1: LOSE first (no unlock)
	printf("\n-- Level 1: a LOSS must not unlock anything --\n");
	click(lvl1Btn);
	check(gameState == STATE_PLAY && selectedLevel == 1 && currentLevel == 1, "LEVEL 1 starts");
	check(playerMaxHealth == 10000 && enemyMaxLife == 10000, "Level 1 stats unchanged (10000 / 10000)");
	defeatEnemyAndTick();
	check(gameState == STATE_PUZZLE, "enemy defeated -> puzzle opens");
	updatePuzzle(30.0f);   // let the 25 s puzzle timer run out
	check(gameState == STATE_GAME_OVER, "puzzle timer expiry -> GAME OVER");
	check(!level2Unlocked, "loss did NOT unlock Level 2");
	pressKey(13);
	check(gameState == STATE_GAME_OVER, "ENTER on GAME OVER does not continue anywhere");
	pressKey(27);
	check(gameState == STATE_LEVEL_SELECT, "ESC returns to Level Select");
	click(lvl2Btn);
	check(gameState == STATE_LEVEL_SELECT, "LEVEL 2 still locked after a loss");

	// ----------------------------------------------------------- Level 1: WIN
	printf("\n-- Level 1: WIN unlocks exactly Level 2 --\n");
	click(lvl1Btn);
	check(gameState == STATE_PLAY && selectedLevel == 1, "LEVEL 1 restarts");
	defeatEnemyAndTick();
	check(gameState == STATE_PUZZLE, "enemy defeated -> puzzle");
	solveMemoryPuzzle();
	check(gameState == STATE_LEVEL_WIN, "puzzle solved -> LEVEL COMPLETE");
	check(level2Unlocked, "Level 2 UNLOCKED");
	check(!level3Unlocked && !level4Unlocked, "Levels 3 and 4 still locked");
	renderLevelWin();

	printf("\n-- ESC path (original behaviour) still returns to Level Select --\n");
	pressKey(27);
	check(gameState == STATE_LEVEL_SELECT, "ESC on LEVEL COMPLETE -> Level Select");
	click(lvl3Btn);
	check(gameState == STATE_LEVEL_SELECT, "cannot skip ahead to Level 3");
	click(lvl4Btn);
	check(gameState == STATE_LEVEL_SELECT, "cannot skip ahead to Level 4");
	click(lvl2Btn);
	check(gameState == STATE_PLAY && selectedLevel == 2 && currentLevel == 2, "Level 2 now enterable from Level Select");
	check(playerMaxHealth == 20000 && enemyMaxLife == 20000, "Level 2 stats unchanged (20000 / 20000)");
	pressKey('b');
	check(gameState == STATE_LEVEL_SELECT, "'B' leaves the level (original behaviour)");

	// Replay Level 1 and use CONTINUE this time.
	click(lvl1Btn);
	defeatEnemyAndTick();
	solveMemoryPuzzle();
	check(gameState == STATE_LEVEL_WIN, "Level 1 won again");

	// ------------------------------------------------------- CONTINUE -> Level 2
	printf("\n-- CONTINUE goes straight to Level 2 --\n");
	pressKey(13);
	check(gameState == STATE_PLAY && selectedLevel == 2 && currentLevel == 2, "ENTER -> Level 2 running");
	check(puzzleShown == false, "puzzleShown reset for the new level");
	check(playerMaxHealth == 20000 && playerHealth == 20000, "Level 2 starts with full 20000 HP");
	check(!level3Unlocked, "Level 3 still locked while playing Level 2");
	tick(120);
	check(gameState == STATE_PLAY, "Level 2 runs 120 ticks without ending");

	printf("\n-- Level 2 WIN unlocks exactly Level 3 --\n");
	defeatEnemyAndTick();
	check(gameState == STATE_PUZZLE, "Level 2 enemy defeated -> puzzle");
	solveMemoryPuzzle();
	check(gameState == STATE_LEVEL_WIN, "Level 2 complete");
	check(level3Unlocked, "Level 3 UNLOCKED");
	check(!level4Unlocked, "Level 4 still locked");

	// ------------------------------------------------------- CONTINUE -> Level 3
	printf("\n-- CONTINUE goes straight to Level 3 (Part 1) --\n");
	pressKey(13);
	check(gameState == STATE_PLAY && selectedLevel == 3 && currentLevel == 3 && currentPart == 1, "ENTER -> Level 3, Part 1");
	check(playerMaxHealth == 30000 && enemyMaxLife == 30000, "Level 3 stats unchanged (30000 / 30000)");
	check(!level4Unlocked, "Level 4 still locked while playing Level 3");
	tick(120);
	check(gameState == STATE_PLAY, "Level 3 Part 1 runs 120 ticks");

	printf("\n-- Level 3 Part 1 -> Part 2 (existing confirm prompt) --\n");
	l3p1KilledCount = L3P1_TOTAL_TO_KILL;        // Part 1's win condition (10 kills)
	tick(2);
	check(gameState == STATE_PART_CONFIRM, "Part 1 cleared -> 'proceed to Part 2?' prompt");
	click(yesBtn);
	check(gameState == STATE_PLAY && currentPart == 2, "YES -> Part 2");
	check(!level4Unlocked, "Level 4 still locked after Part 1");
	tick(60);
	check(gameState == STATE_PLAY, "Part 2 runs 60 ticks");

	printf("\n-- Level 3 Part 2 -> Part 3 --\n");
	enemyLife = 0; playerHealth = playerMaxHealth; tick(2);
	check(gameState == STATE_PART_CONFIRM, "Part 2 cleared -> 'proceed to Part 3?' prompt");
	renderPartConfirm();
	click(yesBtn);
	check(gameState == STATE_PLAY && currentPart == 3, "YES -> Part 3");
	check(!level4Unlocked, "Level 4 still locked after Part 2 (finishing a PART is not finishing the LEVEL)");
	tick(60);
	check(gameState == STATE_PLAY, "Part 3 runs 60 ticks");

	printf("\n-- Level 3 Part 3 final puzzle: a LOSS does not unlock Level 4 --\n");
	defeatEnemyAndTick();
	check(gameState == STATE_PUZZLE, "Part 3 enemy defeated -> final puzzle");
	updatePuzzle(30.0f);
	check(gameState == STATE_GAME_OVER, "final puzzle timeout -> GAME OVER");
	check(!level4Unlocked, "Level 4 still LOCKED after failing Level 3");
	pressKey(27);
	click(lvl4Btn);
	check(gameState == STATE_LEVEL_SELECT, "LEVEL 4 button inert");

	printf("\n-- Level 3 WIN unlocks Level 4 --\n");
	click(lvl3Btn);
	check(gameState == STATE_PLAY && currentLevel == 3 && currentPart == 1, "Level 3 restarts at Part 1");
	pressKey('n'); pressKey('n');                // existing Level 3 debug helper: jump to Part 3
	check(currentPart == 3, "(debug 'N' key: Part 3)");
	defeatEnemyAndTick();
	check(gameState == STATE_PUZZLE, "Part 3 enemy defeated -> final puzzle");
	solveMemoryPuzzle();
	check(gameState == STATE_LEVEL_WIN, "final puzzle solved -> LEVEL COMPLETE");
	check(level4Unlocked, "Level 4 UNLOCKED");

	// ------------------------------------------------------- CONTINUE -> Level 4
	printf("\n-- CONTINUE goes straight to Level 4 (Nine-Tails Seal) --\n");
	pressKey(13);
	check(gameState == STATE_PLAY && selectedLevel == 4 && currentLevel == LEVEL4_ID, "ENTER -> Level 4 running");
	check(playerMaxHealth == 20000 && playerHealth == 20000, "Level 4 player HP = 20000 (Naruto values)");
	check(Level4CameraX() == 0, "Level 4 camera at the start of the map");
	for (int f = 0; f < 900; f++) {
		handleRealtimeInput(); updateGameLogic(); renderGameplay();
		if (gameState != STATE_PLAY) break;
	}
	check(gameState == STATE_PLAY, "Level 4 runs and renders 900 frames");
	check(l4ObstacleCount > 0 && l4SealAwake == false, "Level 4 obstacles built; seal approach asleep until the Warden falls");

	printf("\n-- Level 4 is the last level --\n");
	gameState = STATE_LEVEL_WIN;                  // (Level 4's own win path is covered by test_puzzle4)
	renderLevelWin();
	pressKey(13);
	check(gameState == STATE_LEVEL_SELECT, "ENTER after Level 4 -> Level Select (nothing further to continue to)");
	check(level2Unlocked && level3Unlocked && level4Unlocked, "all four levels available");
	renderEverythingOnce();

	// ------------------------------------------- re-entry from Level Select works
	printf("\n-- every level re-enterable from Level Select --\n");
	int wantLevel[4] = { 1, 2, 3, 4 };
	Button* btn[4] = { &lvl1Btn, &lvl2Btn, &lvl3Btn, &lvl4Btn };
	for (int i = 0; i < 4; i++) {
		gameState = STATE_LEVEL_SELECT;
		click(*btn[i]);
		char msg[64]; sprintf(msg, "Level Select -> LEVEL %d", wantLevel[i]);
		check(gameState == STATE_PLAY && selectedLevel == wantLevel[i] && currentLevel == wantLevel[i], msg);
	}

	printf("\n%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
