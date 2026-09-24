
#define STB_IMAGE_IMPLEMENTATION

#include "defines.h"
#include "variable.h"
#include "ui.h"
#include "controls.h"
#include "game.h"
#include "puzzle.h"
#include "gameover.h"
#include "Audio.h"
#include "obstacle.h"
#include "level4.h"
#include "puzzle4.h"
#include "playerhealth.h"   // player health files (playerhealth.txt / playerhealth.dat)

void iDraw() {
	iClear();

	switch (gameState) {
	case STATE_MAIN_MENU:
		renderMainMenu();
		break;
	case STATE_LEVEL_SELECT:
		renderLevelSelect();
		PlayerHealthDrawLast();   // most recent saved health record, if any
		break;
	case STATE_PLAY:
		renderGameplay();
		break;
	case STATE_INSTRUCTION:
		renderInstruction();
		break;
	case STATE_ABOUT:
		renderAbout();
		break;
	case STATE_PUZZLE:
		renderPuzzle();
		break;
	case STATE_PUZZLE4:              // Level 4 merge: three-stage final seal
		RenderPuzzle4();
		break;
	case STATE_GAME_OVER:
		renderGameOver();
		break;
	case STATE_LEVEL_WIN:
		renderLevelWin();
		break;
	case STATE_PART_CONFIRM:
		renderPartConfirm();
		break;
	}
	
}


void iMouseMove(int mx, int my) {
	curMouseX = mx;
	curMouseY = my;
	if (gameState == STATE_PLAY) {
		HandlePlayerMouseMove(mx, my); // Level 1, unchanged
	}
}
void iPassiveMouseMove(int mx, int my) {
	curMouseX = mx;
	curMouseY = my;
	if (gameState == STATE_PLAY) {
		HandlePlayerMouseMove(mx, my); // Level 1, unchanged
	}
}

void iMouse(int button, int state, int mx, int my) {
	handleMouseInput(button, state, mx, my);
}

void iKeyboard(unsigned char key) {
	handleKeyboardInput(key);
}

void iSpecialKeyboard(unsigned char key) {
	if (key == GLUT_KEY_END) exit(0);
}

void fixedUpdate() {
	handleRealtimeInput();
	updateGameLogic();
	PlayerHealthUpdate();     // records health on level complete / game over / leaving a level
}

int main() {
	
	iInitialize(SCREEN_WIDTH, SCREEN_HEIGHT, "PARADOX ARMORY");

	bgMenuImg = iLoadImage("bg.bmp");
	bgLevelImg = iLoadImage("img.bmp");

	backgroundImg = iLoadImage("images\\background.jpg");


	backgroundImg2 = iLoadImage("images\\level2_background.bmp");

	backgroundImg3a = iLoadImage("images\\level3_part1_background.jpg");
	backgroundImg3b = iLoadImage("images\\level3_part2_background.jpg");
	backgroundImg3c = iLoadImage("images\\level3_part3_background.jpg"); 

	printf("3a=%d 3b=%d 3c=%d\n", backgroundImg3a, backgroundImg3b, backgroundImg3c);

	LoadPlayerAssets();
	LoadEnemyAssets();
	LoadObstacleAssets(); // Level 2 merge: currently a no-op (ball is drawn procedurally)
	LoadLevel4Assets();   // Level 4 merge: loads images\\level4_background.png once
	InitEnemyLevel(1);
	PlayerHealthInit();   // load playerhealth.dat (or .txt) and arrange the save-on-exit

	// STEP 9: background music starts on the very first screen (main menu).
	PlayMenuMusic();

	iSetTimer(16, fixedUpdate);
	iStart();
	return 0;
}
