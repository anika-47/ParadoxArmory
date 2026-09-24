// =====================================================================
// PLAYER HEALTH FILES IN A REAL PLAYTHROUGH
// =====================================================================
//   g++ -std=c++03 -fpermissive -w -I. -o test_healthflow test_healthflow.cpp \
//       playerhealth.cpp iGraphicsGlobals.cpp player.cpp gameState.cpp variable.cpp obstacle.cpp
//
// Same idea as test_progression.cpp: the REAL handleMouseInput / handleKeyboardInput /
// updateGameLogic / puzzle click handlers drive Level 1 -> 2 -> 3 -> 4 (with one loss on the
// way), and PlayerHealthUpdate() runs after every tick and input exactly like fixedUpdate()
// in iMain.cpp. Afterwards both files are read back from disk.
// (enemy.cpp, puzzle.cpp, level4.cpp and puzzle4.cpp are #included -- leave them off the g++ line.)
// =====================================================================
#include "defines.h"      // same include order as iMain.cpp
#include "variable.h"
#include "ui.h"
#include "controls.h"
#include "game.h"
#include "gameover.h"
#include "playerhealth.h"
#include "level4.cpp"
#include "puzzle4.cpp"
#include "enemy.cpp"
#include "puzzle.cpp"
#include <stdio.h>

static int failures = 0, checks = 0;
static void check(bool c, const char* what) {
	checks++;
	printf("  [%s] %s\n", c ? " ok " : "FAIL", what);
	if (!c) failures++;
}

static const char* TXT = "ph_flow.txt";
static const char* BIN = "ph_flow.dat";

// ---- drive the game like a player; the health hook runs like the 16 ms timer does ----
static void click(const Button& b) { handleMouseInput(GLUT_LEFT_BUTTON, GLUT_DOWN, b.x + b.width / 2, b.y + b.height / 2); PlayerHealthUpdate(); }
static void pressKey(unsigned char k) { handleKeyboardInput(k); PlayerHealthUpdate(); }
static void tick(int n) { for (int i = 0; i < n; i++) { handleRealtimeInput(); updateGameLogic(); PlayerHealthUpdate(); } }
static void solveMemoryPuzzle() {
	for (int s = 0; s < 6; s++) {
		int first = -1, second = -1;
		for (int i = 0; i < 12; i++) if (puzzle.cards[i].symbolID == s) { if (first < 0) first = i; else second = i; }
		Card& a = puzzle.cards[first]; Card& b = puzzle.cards[second];
		handlePuzzleClick((int)(a.x + a.w / 2), (int)(a.y + a.h / 2));
		handlePuzzleClick((int)(b.x + b.w / 2), (int)(b.y + b.h / 2));
	}
	PlayerHealthUpdate();
}
static void defeatEnemyAndTick() { enemyLife = 0; playerHealth = playerMaxHealth; tick(2); }

static bool same(const PlayerHealthRecord& a, const PlayerHealthRecord& b) {
	return a.magic == b.magic && a.level == b.level && a.part == b.part && a.health == b.health && a.maxHealth == b.maxHealth && a.result == b.result;
}

int main() {
	srand(12345);
	remove(TXT); remove(BIN);
	PlayerHealthSetPaths(TXT, BIN);
	printf("=============== PLAYER HEALTH FILES: FULL PLAYTHROUGH ===============\n");

	InitEnemyLevel(1);
	PlayerHealthInit();
	check(PlayerHealthCount() == 0, "first launch: no saved records");
	click(playBtn);
	renderLevelSelect(); PlayerHealthDrawLast();

	printf("\n-- Level 1: lose the final puzzle, then win it --\n");
	click(lvl1Btn);
	tick(30);
	check(PlayerHealthCount() == 0, "playing records nothing");
	defeatEnemyAndTick();
	updatePuzzle(30.0f); PlayerHealthUpdate();
	check(gameState == STATE_GAME_OVER && PlayerHealthCount() == 1, "puzzle timed out -> game over -> 1 record");
	check(PlayerHealthLast()->result == PH_RESULT_GAMEOVER && PlayerHealthLast()->level == 1 && PlayerHealthLast()->health == 10000, "  GAMEOVER, level 1, 10000 HP");
	pressKey(27);
	check(gameState == STATE_LEVEL_SELECT && PlayerHealthCount() == 1, "ESC from game over -> Level Select, no extra record");
	click(lvl1Btn); defeatEnemyAndTick(); solveMemoryPuzzle();
	check(gameState == STATE_LEVEL_WIN && PlayerHealthCount() == 2, "won Level 1 -> 2 records");
	check(PlayerHealthLast()->result == PH_RESULT_COMPLETE && PlayerHealthLast()->level == 1, "  COMPLETE, level 1");

	printf("\n-- Continue into Level 2 and win --\n");
	pressKey(13);
	check(gameState == STATE_PLAY && selectedLevel == 2 && PlayerHealthCount() == 2, "Continue -> Level 2 (nothing recorded)");
	tick(60); defeatEnemyAndTick(); solveMemoryPuzzle();
	check(PlayerHealthCount() == 3 && PlayerHealthLast()->level == 2 && PlayerHealthLast()->result == PH_RESULT_COMPLETE && PlayerHealthLast()->health == 20000, "won Level 2 -> COMPLETE, 20000 HP");

	printf("\n-- Continue into Level 3: parts and puzzles record nothing until the level is won --\n");
	pressKey(13);
	check(selectedLevel == 3 && currentPart == 1, "Level 3 Part 1");
	l3p1KilledCount = L3P1_TOTAL_TO_KILL; tick(2);
	check(gameState == STATE_PART_CONFIRM, "Part 1 cleared -> prompt");
	click(yesBtn); tick(30);
	enemyLife = 0; playerHealth = playerMaxHealth; tick(2);
	check(gameState == STATE_PART_CONFIRM, "Part 2 cleared -> prompt");
	click(yesBtn); tick(30);
	check(currentPart == 3 && PlayerHealthCount() == 3, "in Part 3: still only 3 records");
	defeatEnemyAndTick();
	int hpAtWin = playerHealth, maxAtWin = playerMaxHealth;
	solveMemoryPuzzle();
	check(gameState == STATE_LEVEL_WIN && PlayerHealthCount() == 4, "won Level 3 -> 4 records");
	check(PlayerHealthLast()->level == 3 && PlayerHealthLast()->part == 3 && PlayerHealthLast()->health == hpAtWin &&
	      PlayerHealthLast()->maxHealth == maxAtWin && PlayerHealthLast()->result == PH_RESULT_COMPLETE, "  COMPLETE, level 3, part 3, the exact HP at the win");

	printf("\n-- Level 4: leave it with 'B' --\n");
	pressKey(13);
	check(gameState == STATE_PLAY && selectedLevel == 4 && PlayerHealthCount() == 4, "Continue -> Level 4 (nothing recorded)");
	for (int f = 0; f < 300; f++) { handleRealtimeInput(); updateGameLogic(); renderGameplay(); PlayerHealthUpdate(); }
	pressKey('b');
	check(gameState == STATE_LEVEL_SELECT && PlayerHealthCount() == 5, "'B' mid-level -> 5 records");
	check(PlayerHealthLast()->level == 4 && PlayerHealthLast()->result == PH_RESULT_LEFT && PlayerHealthLast()->health == 20000, "  LEFT, level 4, 20000 HP");

	printf("\n-- what is on disk --\n");
	PlayerHealthRecord fromBin[PH_MAX_RECORDS], fromTxt[PH_MAX_RECORDS];
	int nb = PlayerHealthLoadBinary(BIN, fromBin, PH_MAX_RECORDS);
	int nt = PlayerHealthLoadText(TXT, fromTxt, PH_MAX_RECORDS);
	check(nb == 5 && nt == 5, "both files contain 5 records");
	bool ok = true;
	for (int i = 0; i < 5; i++) if (!same(fromBin[i], *PlayerHealthGet(i)) || !same(fromTxt[i], *PlayerHealthGet(i))) ok = false;
	check(ok, "binary file == text file == what the game holds in memory");
	int wantLevel[5] = { 1, 1, 2, 3, 4 };
	int wantResult[5] = { PH_RESULT_GAMEOVER, PH_RESULT_COMPLETE, PH_RESULT_COMPLETE, PH_RESULT_COMPLETE, PH_RESULT_LEFT };
	ok = true;
	for (int i = 0; i < 5; i++) if (fromBin[i].level != wantLevel[i] || fromBin[i].result != wantResult[i]) ok = false;
	check(ok, "in order: L1 GAMEOVER, L1 COMPLETE, L2 COMPLETE, L3 COMPLETE, L4 LEFT");
	printf("\n  ---- %s ----\n", TXT);
	{ FILE* f = fopen(TXT, "r"); char line[80]; int shown = 0; while (fgets(line, 80, f) && shown++ < 16) printf("  | %s", line); fclose(f); }

	printf("\n-- close and reopen the game --\n");
	gameState = STATE_MAIN_MENU;
	PlayerHealthInit();                       // what main() does on the next launch
	check(PlayerHealthCount() == 5 && PlayerHealthLast()->level == 4, "the 5 records are loaded again");
	gameState = STATE_LEVEL_SELECT;
	renderLevelSelect(); PlayerHealthDrawLast();
	click(lvl2Btn);
	check(gameState == STATE_PLAY && selectedLevel == 2, "(Level 2 -- unlocked this session -- started)");
	playerHealth = 15500;
	PlayerHealthSaveOnExit();                 // the atexit hook: window closed / ESC during play
	check(PlayerHealthCount() == 6 && PlayerHealthLast()->result == PH_RESULT_LEFT && PlayerHealthLast()->health == 15500, "closing the game mid-level -> 6th record (LEFT, 15500 HP)");

	remove(TXT); remove(BIN);
	printf("\n%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
