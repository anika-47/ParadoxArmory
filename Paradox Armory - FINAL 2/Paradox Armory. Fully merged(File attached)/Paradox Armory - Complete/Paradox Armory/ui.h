#ifndef UI_H
#define UI_H

#include "defines.h"
#include "gameState.h"  // backgroundImg (Level 1's loaded background texture)
#include "player.h"     // RenderPlayer()
#include "enemy.h"      // RenderEnemies()
#include "obstacle.h"   // Level 2 merge: RenderObstacle() (no-op outside Level 2)
#include "level4.h"     // Level 4 merge: RenderLevel4()

// External references for variables defined in variables.h
extern int gameState;
extern int selectedLevel;
extern Button playBtn, instructionBtn, aboutBtn, exitBtn;
extern Button lvl1Btn, lvl2Btn, lvl3Btn, lvl4Btn, backBtn;
extern Button lvl1Btn, lvl2Btn, lvl3Btn, lvl4Btn, backBtn;
extern Button yesBtn, noBtn; // Level 3 merge
extern int curMouseX, curMouseY;
extern bool level2Unlocked;
extern bool level4Unlocked; // Level 4 merge

inline int isClicked(Button btn, int mx, int my) {
	return (mx >= btn.x && mx <= btn.x + btn.width &&
		my >= btn.y && my <= btn.y + btn.height);
}

// Returns the pixel width a string would render at in the given GLUT bitmap
// font, so text can be centered exactly instead of eyeballing an x value.
inline int textWidth(const char* str, void* font) {
	int width = 0;
	for (int i = 0; str[i]; i++) {
		width += glutBitmapWidth(font, str[i]);
	}
	return width;
}

// Draws str centered horizontally within the full screen width, at height y.
inline void iTextCenteredX(int y, const char* str, void* font) {
	int x = (SCREEN_WIDTH - textWidth(str, font)) / 2;
	iText(x, y, (char*)str, font);
}

inline void drawButton(Button btn) {
	bool hovered = (isClicked(btn, curMouseX, curMouseY) != 0);

	iSetColor(255, 255, 255);
	iRectangle(btn.x, btn.y, btn.width, btn.height);

	if (hovered) {
		iSetColor(60, 60, 90); // Hover fill
	}
	else {
		iSetColor(20, 20, 20);
	}
	iFilledRectangle(btn.x + 2, btn.y + 2, btn.width - 4, btn.height - 4);

	if (hovered) {
		iSetColor(0, 255, 200);
	}
	else {
		iSetColor(255, 255, 255);
	}
	iText(btn.x + 15, btn.y + 18, btn.text, GLUT_BITMAP_HELVETICA_18);
}

inline void renderMainMenu() {
	if (bgMenuImg > 0) iShowImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, bgMenuImg);
	else iShowBMP(0, 0, "bg.bmp");
	iSetColor(255, 50, 50);
	iTextCenteredX(615, "PARADOX ARMORY", GLUT_BITMAP_TIMES_ROMAN_24);

	iSetColor(0, 255, 128);
	iTextCenteredX(575, "Welcome, Player!", GLUT_BITMAP_HELVETICA_18);

	drawButton(playBtn);
	drawButton(instructionBtn);
	drawButton(aboutBtn);
	drawButton(exitBtn);
}

inline void renderLevelSelect() {
	if (bgLevelImg > 0) iShowImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, bgLevelImg);
	else iShowBMP(0, 0, "img.bmp");
	iSetColor(220, 20, 20);
	iTextCenteredX(615, "SELECT LEVEL", GLUT_BITMAP_TIMES_ROMAN_24);

	iSetColor(220, 220, 220);
	iTextCenteredX(575, "Choose your difficulty stage:", GLUT_BITMAP_HELVETICA_12);

	drawButton(lvl1Btn);

	
	Button lvl2Display = lvl2Btn;
	if (level2Unlocked) {
		strcpy(lvl2Display.text, "LEVEL 2");
	}
	drawButton(lvl2Display);

	Button lvl3Display = lvl3Btn;
	if (level3Unlocked) {
		strcpy(lvl3Display.text, "LEVEL 3");
	}
	drawButton(lvl3Display);

	// Level 4 merge: same pattern as the stages above.
	Button lvl4Display = lvl4Btn;
	if (level4Unlocked) {
		strcpy(lvl4Display.text, "LEVEL 4");
	}
	drawButton(lvl4Display);

	iSetColor(220, 50, 50);
	iRectangle(backBtn.x, backBtn.y, backBtn.width, backBtn.height);
	iSetColor(40, 10, 10);
	iFilledRectangle(backBtn.x + 2, backBtn.y + 2, backBtn.width - 4, backBtn.height - 4);
	iSetColor(255, 255, 255);
	iText(backBtn.x + 30, backBtn.y + 13, backBtn.text, GLUT_BITMAP_HELVETICA_12);
}


inline void renderGameplay() {

	// Level 4 merge: the new level owns its entire frame (scrolling
	// background, world, player and HUD), so it renders and returns
	// before any of the Level 1/2/3 drawing below can run.
	if (currentLevel == LEVEL4_ID) {
		RenderLevel4();
		return;
	}

	if (currentLevel == 3) {
		// Level 3 merge: pick background by part; same character (RenderPlayer())
		// is drawn below regardless of which part we're in.
		int bgToShow = backgroundImg3a;
		if (currentPart == 2) bgToShow = backgroundImg3b;
		else if (currentPart == 3) bgToShow = backgroundImg3c;

		if (bgToShow > 0) {
			iShowImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, bgToShow);
		}
	}
	else if (currentLevel == 2 && backgroundImg2 > 0) {
		const int BG_LEVEL2_NATIVE_WIDTH = 1002;
		const int BG_LEVEL2_NATIVE_HEIGHT = 393;

		float scaleX = (float)SCREEN_WIDTH / BG_LEVEL2_NATIVE_WIDTH;
		float scaleY = (float)SCREEN_HEIGHT / BG_LEVEL2_NATIVE_HEIGHT;
		float scale = (scaleX < scaleY) ? scaleX : scaleY;

		int drawW = (int)(BG_LEVEL2_NATIVE_WIDTH * scale);
		int drawH = (int)(BG_LEVEL2_NATIVE_HEIGHT * scale);
		int drawX = (SCREEN_WIDTH - drawW) / 2;
		int drawY = (SCREEN_HEIGHT - drawH) / 2;

		iShowImage(drawX, drawY, drawW, drawH, backgroundImg2);
	}
	else if (backgroundImg > 0) {
		iShowImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, backgroundImg);
	}

	RenderLevel3Part1Platforms(); // Level 3 merge: draw floating bricks behind the player
	RenderPlayer();
	RenderEnemies();
	RenderObstacle();

	iSetColor(255, 255, 255);
	iText(SCREEN_WIDTH / 2 - 220, 40, "Press 'B' Key or Right-Click to Go Back to Level Select", GLUT_BITMAP_HELVETICA_12);
}

inline void renderPartConfirm() {
	renderGameplay(); // keep the current scene visible behind the prompt

	iSetColor(0, 0, 0);
	iFilledRectangle(SCREEN_WIDTH / 2 - 260, SCREEN_HEIGHT / 2 - 80, 520, 140);
	iSetColor(255, 255, 255);
	iRectangle(SCREEN_WIDTH / 2 - 260, SCREEN_HEIGHT / 2 - 80, 520, 140);

	char confirmText[60];
	sprintf(confirmText, "Do you want to proceed to Part %d?", currentPart + 1);
	iText(SCREEN_WIDTH / 2 - 230, SCREEN_HEIGHT / 2 + 30,
		confirmText, GLUT_BITMAP_HELVETICA_18);

	drawButton(yesBtn);
	drawButton(noBtn);
}
inline void renderInstruction() {
	if (bgMenuImg > 0) iShowImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, bgMenuImg);
	else iShowBMP(0, 0, "bg.bmp");
	iSetColor(255, 50, 50);
	iText(820, 630, "INSTRUCTIONS", GLUT_BITMAP_TIMES_ROMAN_24);

	iSetColor(255, 255, 255);
	iText(680, 540, "1. AIM & SHOOT:", GLUT_BITMAP_HELVETICA_18);
	iSetColor(200, 200, 200);
	iText(700, 510, "- Move mouse cursor to aim at enemies.", GLUT_BITMAP_HELVETICA_12);
	iText(700, 485, "- Click LEFT MOUSE BUTTON to fire bullets.", GLUT_BITMAP_HELVETICA_12);

	iSetColor(255, 255, 255);
	iText(680, 440, "2. GAME RULES:", GLUT_BITMAP_HELVETICA_18);
	iSetColor(200, 200, 200);
	iText(700, 410, "- Bullets bounce off walls to hit tricky targets.", GLUT_BITMAP_HELVETICA_12);
	iText(700, 385, "- Defeat all waves of enemies to complete each level.", GLUT_BITMAP_HELVETICA_12);

	iSetColor(255, 255, 255);
	iText(680, 340, "3. NAVIGATION CONTROLS:", GLUT_BITMAP_HELVETICA_18);
	iSetColor(200, 200, 200);
	iText(700, 310, "- Press 'B' or RIGHT-CLICK to return to main menu.", GLUT_BITMAP_HELVETICA_12);

	iSetColor(255, 50, 50);
	iText(730, 230, "Press 'B' or Right-Click to Go Back", GLUT_BITMAP_HELVETICA_12);
}

inline void renderAbout() {
	if (bgMenuImg > 0) iShowImage(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, bgMenuImg);
	else iShowBMP(0, 0, "bg.bmp");
	iSetColor(255, 50, 50);
	iText(830, 630, "ABOUT US", GLUT_BITMAP_TIMES_ROMAN_24);

	iSetColor(255, 255, 255);
	iText(800, 580, "PARADOX ARMORY", GLUT_BITMAP_HELVETICA_18);
	iText(780, 550, "Developed By - Group Team", GLUT_BITMAP_HELVETICA_12);

	iSetColor(0, 255, 128);
	iText(730, 480, "1. SAMIRA ISLAM", GLUT_BITMAP_HELVETICA_18);
	iSetColor(220, 220, 220);
	iText(750, 455, "ID NO - 00725105101131", GLUT_BITMAP_HELVETICA_12);

	iSetColor(0, 255, 128);
	iText(730, 400, "2. ANIKA TASNIM", GLUT_BITMAP_HELVETICA_18);
	iSetColor(220, 220, 220);
	iText(750, 375, "ID NO - 00725105101133", GLUT_BITMAP_HELVETICA_12);

	iSetColor(0, 255, 128);
	iText(730, 320, "3. ARHAM BIN ZAHEED", GLUT_BITMAP_HELVETICA_18);
	iSetColor(220, 220, 220);
	iText(750, 295, "ID NO - 0072510510145", GLUT_BITMAP_HELVETICA_12);

	iSetColor(255, 50, 50);
	iText(730, 220, "Press 'B' or Right-Click to Go Back", GLUT_BITMAP_HELVETICA_12);
}

#endif