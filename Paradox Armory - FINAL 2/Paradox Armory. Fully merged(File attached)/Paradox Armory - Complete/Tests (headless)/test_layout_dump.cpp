// =====================================================================
// LEVEL 4 LAYOUT DUMP - a diff tool, not a pass/fail test.
// =====================================================================
// Prints every Level 4 obstacle (type, position, size, motion, damage), the
// altar, all 54 scripted spawns, the player start and one generated Nine-Tails
// puzzle. Build it once against the original "Nine-Tails Seal" ZIP sources and
// once against this project, run both, and diff the outputs: they must be identical.
//   g++ -std=c++03 -fpermissive -w -I. -o dump test_layout_dump.cpp iGraphicsGlobals.cpp \
//       player.cpp gameState.cpp variable.cpp enemy.cpp obstacle.cpp puzzle.cpp
// =====================================================================
#include "level4.cpp"
#include "puzzle4.cpp"
#include <stdio.h>
int main() {
	srand(12345);
	InitLevel4();
	printf("obstacles=%d spawns=%d sections=%d\n", l4ObstacleCount, (int)(sizeof(L4_SPAWNS) / sizeof(L4_SPAWNS[0])), L4_NUM_SECTIONS);
	for (int i = 0; i < l4ObstacleCount; i++) {
		const L4Obstacle& o = l4Obstacles[i];
		printf("OB %2d type=%d x=%.1f y=%.1f w=%d h=%d baseX=%.1f baseY=%.1f range=%.1f rangeY=%.1f speed=%.4f axis=%d solid=%d dmg=%d sec=%d timer=%d\n",
		       i, o.type, o.x, o.y, o.w, o.h, o.baseX, o.baseY, o.range, o.rangeY, o.speed, o.axis, (int)o.solid, o.damage, o.section, o.timer);
	}
	printf("ALTAR x=%.1f trigger=%.1f\n", L4_ALTAR_X, L4_ALTAR_TRIGGER_X);
	int n = (int)(sizeof(L4_SPAWNS) / sizeof(L4_SPAWNS[0]));
	for (int i = 0; i < n; i++) printf("SPAWN %2d sec=%d wave=%d type=%d\n", i, L4_SPAWNS[i].section, L4_SPAWNS[i].wave, L4_SPAWNS[i].type);
	printf("player start: x=%d y=%d hp=%d/%d cam=%d\n", playerX, playerY, playerHealth, playerMaxHealth, Level4CameraX());
	// the final puzzle: generate with a fixed seed and dump what the player would see/answer
	srand(4242); InitPuzzle4();
	printf("puzzle4 stage=%d attempts=%d warden=%d code=%d%d%d%d\n", pz4Stage, pz4Attempts, pz4Warden, pz4Code[0], pz4Code[1], pz4Code[2], pz4Code[3]);
	return 0;
}
