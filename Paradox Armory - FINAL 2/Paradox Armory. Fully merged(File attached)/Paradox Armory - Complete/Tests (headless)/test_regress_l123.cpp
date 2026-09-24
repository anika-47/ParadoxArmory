// =====================================================================
// LEVEL 1-3 REGRESSION HASH - a diff tool, not a pass/fail test.
// =====================================================================
// Plays Levels 1, 2 and 3 (all three parts) for 6000 frames each with scripted,
// seeded key presses and shooting, and prints a hash of the player/enemy/bullet
// state every frame. Build it against the original "Enemy Facing" sources and against
// this project and compare the printed hashes: they must be identical.
//   g++ -std=c++03 -fpermissive -w -I. -o reg test_regress_l123.cpp iGraphicsGlobals.cpp \
//       player.cpp gameState.cpp variable.cpp enemy.cpp obstacle.cpp puzzle.cpp level4.cpp puzzle4.cpp
// =====================================================================
#include "defines.h"
#include "variable.h"
#include "gameState.h"
#include "player.h"
#include "enemy.h"
#include "obstacle.h"
#include <stdio.h>
extern int enemyCorX, enemyCorY, enemyIndex;
extern bool enemyFacingLeft;

static unsigned int lcg = 1;
static int rnd(int n) { lcg = lcg * 1103515245u + 12345u; return (int)((lcg >> 16) % (unsigned)n); }
static void setKey(int k, bool d) { AsyncKeyTable()[k & 511] = d ? (short)0x8000 : 0; }

static unsigned int hashAcc = 2166136261u;
static void mix(int v) { hashAcc = (hashAcc ^ (unsigned)v) * 16777619u; }

static void runScenario(const char* name, int level, int part, int frames) {
	srand(2024); lcg = 99 + level * 7 + part;
	currentPart = part;
	InitEnemyLevel(level);
	if (level == 3 && part > 1) { InitEnemyPart(part); if (part == 3) InitObstacle(); }
	if (level == 2) InitObstacle();
	playerHealth = playerMaxHealth;
	hashAcc = 2166136261u;
	int sigs = 0;
	for (int f = 0; f < frames; f++) {
		if (f % 20 == 0) {           // change the "held" keys every 20 frames
			setKey('A', rnd(3) == 0); setKey('D', rnd(2) == 0); setKey('W', rnd(6) == 0); setKey('S', rnd(9) == 0);
			int mx = rnd(SCREEN_WIDTH), my = rnd(SCREEN_HEIGHT);
			HandlePlayerMouseMove(mx, my);
		}
		if (f % 7 == 0) HandlePlayerKeyboardInput('f');
		if (f % 11 == 0) HandlePlayerMouseInput(GLUT_LEFT_BUTTON, GLUT_DOWN, rnd(SCREEN_WIDTH), rnd(SCREEN_HEIGHT));
		updatePlayer(); UpdateEnemyLogic(); UpdateObstacle();
		RenderPlayer(); RenderEnemies(); RenderObstacle(); RenderLevel3Part1Platforms();
		if (playerHealth <= 0) playerHealth = playerMaxHealth;    // keep it running
		mix(playerX); mix(playerY); mix(playerHealth); mix(enemyLife); mix(enemyCorX); mix(enemyCorY);
		mix(enemyIndex); mix((int)enemyFacingLeft); mix((int)playerFacingRight); mix((int)isCrouching); mix((int)isJumping);
		for (int i = 0; i < MAX_PLAYER_BULLETS; i++) { mix((int)p_bulletActive[i]); mix(p_bx[i]); mix(p_by[i]); }
		if (f % 500 == 0) { sigs++; printf("  %s f=%5d pX=%4d pY=%3d pHP=%5d eLife=%5d eX=%4d bullets=", name, f, playerX, playerY, playerHealth, enemyLife, enemyCorX);
			int n = 0; for (int i = 0; i < MAX_PLAYER_BULLETS; i++) n += p_bulletActive[i]; printf("%d\n", n); }
	}
	printf("%s hash=%08x\n", name, hashAcc);
	setKey('A', false); setKey('D', false); setKey('W', false); setKey('S', false);
}
int main() {
	runScenario("LEVEL1      ", 1, 1, 6000);
	runScenario("LEVEL2      ", 2, 1, 6000);
	runScenario("LEVEL3-PART1", 3, 1, 6000);
	runScenario("LEVEL3-PART2", 3, 2, 6000);
	runScenario("LEVEL3-PART3", 3, 3, 6000);
	return 0;
}
