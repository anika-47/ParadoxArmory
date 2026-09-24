#include "puzzle.h"
#include "defines.h"
#include "gameState.h"
#include "player.h"
#include "variable.h"
#include "Audio.h"
#include <math.h>
#include <string.h>

// ---------------- Shared dispatch ----------------
// activePuzzleType: 0 = memory match (Level 1/2), 1 = code lock (Part 2, 1st puzzle), 2 = circuit (Part 2, 2nd puzzle)
static int activePuzzleType = 0;
static float puzzleTimer = 0.0f;

static bool ClickedButton(Button btn, int mx, int my) {
	return (mx >= btn.x && mx <= btn.x + btn.width && my >= btn.y && my <= btn.y + btn.height);
}
static bool ClickedCircle(int cx, int cy, int r, int mx, int my) {
	int dx = mx - cx, dy = my - cy;
	return (dx * dx + dy * dy) <= r * r;
}

// ---------------- Memory match puzzle (Level 1 / Level 2) ----------------

struct Card {
	int symbolID;
	bool isFlipped;
	bool isMatched;
	float x, y, w, h;
};

struct MemoryPuzzle {
	Card cards[12];
	int flippedIndices[2];
	int numFlipped;
	float delayTimer;
	bool isSolved;
};

static MemoryPuzzle puzzle;

static void InitMemoryPuzzle() {
	puzzle.numFlipped = 0;
	puzzle.delayTimer = 0;
	puzzle.isSolved = false;
	puzzleTimer = 25.0f;
	puzzleSolved = false;

	int symbols[12] = { 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5 };
	for (int i = 0; i < 12; i++) {
		int r = rand() % 12;
		int temp = symbols[i];
		symbols[i] = symbols[r];
		symbols[r] = temp;
	}

	float startX = (SCREEN_WIDTH - (4 * 120 + 3 * 20)) / 2;
	float startY = (SCREEN_HEIGHT - (3 * 160 + 2 * 20)) / 2;

	for (int i = 0; i < 12; i++) {
		puzzle.cards[i].symbolID = symbols[i];
		puzzle.cards[i].isFlipped = false;
		puzzle.cards[i].isMatched = false;
		puzzle.cards[i].w = 120;
		puzzle.cards[i].h = 160;
		int col = i % 4;
		int row = i / 4;
		puzzle.cards[i].x = startX + col * (120 + 20);
		puzzle.cards[i].y = startY + row * (160 + 20);
	}
}

static void DrawCard(int i) {
	Card& c = puzzle.cards[i];
	if (c.isMatched) return;

	float cx = c.x + c.w / 2;
	float cy = c.y + c.h / 2;
	float halfW = c.w / 2;

	if (c.isFlipped) iSetColor(180, 185, 190);
	else iSetColor(40, 45, 50);
	iFilledRectangle(cx - halfW, c.y, c.w, c.h);

	iSetColor(0, 200, 255);
	iRectangle(cx - halfW, c.y, c.w, c.h);

	if (c.isFlipped) {
		float r = 30;
		iSetColor(20, 20, 20);

		switch (c.symbolID) {
		case 0:
			iLine(cx, cy + r, cx - r, cy - r);
			iLine(cx - r, cy - r, cx + r, cy - r);
			iLine(cx + r, cy - r, cx, cy + r);
			break;
		case 1:
			iCircle(cx, cy, r);
			iCircle(cx, cy, r * 0.6f);
			iCircle(cx, cy, r * 0.3f);
			break;
		case 2:
			iRectangle(cx - r, cy - r, r * 2, r * 2);
			iRectangle(cx - r + 5, cy - r + 5, r * 2 - 10, r * 2 - 10);
			break;
		case 3:
			iCircle(cx, cy, r);
			iCircle(cx + r * 0.4f, cy, r * 0.8f);
			break;
		case 4:
			iLine(cx, cy + r, cx + r, cy);
			iLine(cx + r, cy, cx, cy - r);
			iLine(cx, cy - r, cx - r, cy);
			iLine(cx - r, cy, cx, cy + r);
			iCircle(cx, cy, r * 0.4f);
			break;
		case 5:
			iCircle(cx, cy, r * 0.5f);
			for (int a = 0; a < 12; a++) {
				float ang = a * 30 * 3.14159f / 180.0f;
				float r1 = r * 0.5f;
				float r2 = (a % 2 == 0) ? r : r * 0.7f;
				iLine(cx + cos(ang) * r1, cy + sin(ang) * r1, cx + cos(ang) * r2, cy + sin(ang) * r2);
			}
			break;
		}
	}
}

static void RenderMemoryPuzzle() {
	iSetColor(5, 10, 20);
	iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	iSetColor(255, 255, 255);
	iText(SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT - 60, "ENEMY DEFEATED - SOLVE THE PUZZLE", GLUT_BITMAP_TIMES_ROMAN_24);
	iSetColor(0, 200, 255);
	iText(SCREEN_WIDTH / 2 - 190, SCREEN_HEIGHT - 90, "Match the pairs of symbols before time runs out", GLUT_BITMAP_HELVETICA_18);

	char timeStr[32];
	sprintf(timeStr, "TIME: %.1fs", puzzleTimer);
	if (puzzleTimer < 8.0f) iSetColor(255, 60, 60);
	else iSetColor(200, 255, 255);
	iText(SCREEN_WIDTH - 220, SCREEN_HEIGHT - 50, timeStr, GLUT_BITMAP_TIMES_ROMAN_24);

	for (int i = 0; i < 12; i++) {
		DrawCard(i);
	}
}

static void UpdateMemoryPuzzle(float dt) {
	if (puzzle.delayTimer > 0) {
		puzzle.delayTimer -= dt;
		if (puzzle.delayTimer <= 0) {
			puzzle.delayTimer = 0;
			puzzle.cards[puzzle.flippedIndices[0]].isFlipped = false;
			puzzle.cards[puzzle.flippedIndices[1]].isFlipped = false;
			puzzle.numFlipped = 0;
		}
	}

	if (puzzle.isSolved) return;

	puzzleTimer -= dt;
	if (puzzleTimer <= 0) {
		puzzleTimer = 0;

		if (inSegmentPuzzle) {
			inSegmentPuzzle = false;
			gameState = STATE_PLAY;
		}
		else {
			gameState = STATE_GAME_OVER;
			PlayGameOverMusic();
		}
	}
}

static void HandleMemoryPuzzleClick(int mx, int my) {
	if (puzzle.delayTimer > 0 || puzzle.isSolved) return;

	for (int i = 0; i < 12; i++) {
		Card& c = puzzle.cards[i];
		if (!c.isMatched && !c.isFlipped &&
			mx >= c.x && mx <= c.x + c.w && my >= c.y && my <= c.y + c.h) {

			c.isFlipped = true;
			puzzle.flippedIndices[puzzle.numFlipped++] = i;

			if (puzzle.numFlipped == 2) {
				int a = puzzle.flippedIndices[0];
				int b = puzzle.flippedIndices[1];
				if (puzzle.cards[a].symbolID == puzzle.cards[b].symbolID) {
					puzzle.cards[a].isMatched = true;
					puzzle.cards[b].isMatched = true;
					puzzle.numFlipped = 0;

					bool allMatched = true;
					for (int j = 0; j < 12; j++) {
						if (!puzzle.cards[j].isMatched) allMatched = false;
					}
					if (allMatched) {
						puzzle.isSolved = true;
						puzzleSolved = true;
						PlayCollectSound();

						if (inSegmentPuzzle) {
							const int SEGMENT_PUZZLE_HP_REWARD_PERCENT = 15;
							int reward = (playerMaxHealth * SEGMENT_PUZZLE_HP_REWARD_PERCENT) / 100;
							playerHealth += reward;
							if (playerHealth > playerMaxHealth) playerHealth = playerMaxHealth;

							inSegmentPuzzle = false;
							gameState = STATE_PLAY;
						}
						else {
							if (selectedLevel == 1) level2Unlocked = true;
							else if (selectedLevel == 2) level3Unlocked = true;
							else if (selectedLevel == 3) level4Unlocked = true; // Level 4 unlock: completing Level 3
							gameState = STATE_LEVEL_WIN;
						}
					}
				}
				else {
					puzzle.delayTimer = 1.0f;
				}
			}
			break;
		}
	}
}

// ---------------- Code lock puzzle (Level 3 Part 2, 1st puzzle) ----------------

struct CodeLockGuess {
	int digits[4];
	int exactMatches;
	int partialMatches;
};

static const int CODE_MAX_GUESSES = 15;
static int codeSecret[4];
static int codeCurrentGuess[4];
static int codeGuessLength = 0;
static CodeLockGuess codeGuessHistory[CODE_MAX_GUESSES];
static int codeGuessCount = 0;
static bool codeSolved = false;

static Button codeDigitBtns[10];
static Button codeBackspaceBtn;
static Button codeSubmitBtn;

static void SetupCodeLockButtons() {
	int btnW = 70, btnH = 60, gap = 15;
	int cols = 5;
	int totalW = cols * btnW + (cols - 1) * gap;
	int startX = (SCREEN_WIDTH - totalW) / 2;

	const char* labels[10] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0" };

	for (int i = 0; i < 10; i++) {
		int col = i % cols;
		int row = i / cols;
		codeDigitBtns[i].x = startX + col * (btnW + gap);
		codeDigitBtns[i].y = (row == 0) ? 250 : 175;
		codeDigitBtns[i].width = btnW;
		codeDigitBtns[i].height = btnH;
		strcpy(codeDigitBtns[i].text, labels[i]);
	}

	codeBackspaceBtn.x = startX;
	codeBackspaceBtn.y = 90;
	codeBackspaceBtn.width = btnW * 2 + gap;
	codeBackspaceBtn.height = btnH;
	strcpy(codeBackspaceBtn.text, "DEL");

	codeSubmitBtn.x = startX + totalW - (btnW * 2 + gap);
	codeSubmitBtn.y = 90;
	codeSubmitBtn.width = btnW * 2 + gap;
	codeSubmitBtn.height = btnH;
	strcpy(codeSubmitBtn.text, "SUBMIT");
}

static void InitCodeLockPuzzle() {
	for (int i = 0; i < 4; i++) codeSecret[i] = rand() % 10;
	for (int i = 0; i < 4; i++) codeCurrentGuess[i] = -1;
	codeGuessLength = 0;
	codeGuessCount = 0;
	codeSolved = false;
	puzzleSolved = false;
	puzzleTimer = 150.0f;
	SetupCodeLockButtons();
}

static void DrawUIButton(Button btn, int r, int g, int b) {
	iSetColor(255, 255, 255);
	iRectangle(btn.x, btn.y, btn.width, btn.height);
	iSetColor(r, g, b);
	iFilledRectangle(btn.x + 2, btn.y + 2, btn.width - 4, btn.height - 4);
	iSetColor(255, 255, 255);
	iText(btn.x + 15, btn.y + 20, btn.text, GLUT_BITMAP_HELVETICA_18);
}

static void RenderCodeLockPuzzle() {
	iSetColor(5, 10, 20);
	iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	iSetColor(255, 255, 255);
	iText(SCREEN_WIDTH / 2 - 180, SCREEN_HEIGHT - 60, "CRACK THE 4-DIGIT CODE", GLUT_BITMAP_TIMES_ROMAN_24);
	iSetColor(0, 200, 255);
	iText(SCREEN_WIDTH / 2 - 280, SCREEN_HEIGHT - 90,
		"Exact = right digit, right spot | Partial = right digit, wrong spot", GLUT_BITMAP_HELVETICA_12);

	char timeStr[32];
	sprintf(timeStr, "TIME: %.1fs", puzzleTimer);
	if (puzzleTimer < 20.0f) iSetColor(255, 60, 60);
	else iSetColor(200, 255, 255);
	iText(SCREEN_WIDTH - 220, SCREEN_HEIGHT - 50, timeStr, GLUT_BITMAP_TIMES_ROMAN_24);

	int boxW = 60, boxH = 70, boxGap = 15;
	int boxTotalW = 4 * boxW + 3 * boxGap;
	int boxStartX = (SCREEN_WIDTH - boxTotalW) / 2;
	int boxY = 520;
	for (int i = 0; i < 4; i++) {
		iSetColor(255, 255, 255);
		iRectangle(boxStartX + i * (boxW + boxGap), boxY, boxW, boxH);
		if (i < codeGuessLength) {
			char d[4];
			sprintf(d, "%d", codeCurrentGuess[i]);
			iSetColor(0, 255, 128);
			iText(boxStartX + i * (boxW + boxGap) + 22, boxY + 25, d, GLUT_BITMAP_TIMES_ROMAN_24);
		}
	}

	int shownCount = (codeGuessCount < 5) ? codeGuessCount : 5;
	int startIdx = codeGuessCount - shownCount;
	int histY = 470;
	for (int i = 0; i < shownCount; i++) {
		CodeLockGuess& g = codeGuessHistory[startIdx + i];
		char line[80];
		sprintf(line, "%d %d %d %d   ->   Exact: %d   Partial: %d",
			g.digits[0], g.digits[1], g.digits[2], g.digits[3],
			g.exactMatches, g.partialMatches);
		iSetColor(200, 200, 200);
		iText(SCREEN_WIDTH / 2 - 200, histY - i * 28, line, GLUT_BITMAP_HELVETICA_18);
	}

	for (int i = 0; i < 10; i++) DrawUIButton(codeDigitBtns[i], 20, 20, 20);
	DrawUIButton(codeBackspaceBtn, 90, 20, 20);
	DrawUIButton(codeSubmitBtn, 10, 90, 40);
}

static void UpdateCodeLockPuzzle(float dt) {
	if (codeSolved) return;

	puzzleTimer -= dt;
	if (puzzleTimer <= 0) {
		puzzleTimer = 0;

		if (inSegmentPuzzle) {
			inSegmentPuzzle = false;
			gameState = STATE_PLAY;
		}
		else {
			gameState = STATE_GAME_OVER;
			PlayGameOverMusic();
		}
	}
}

static void HandleCodeLockClick(int mx, int my) {
	if (codeSolved) return;

	for (int i = 0; i < 10; i++) {
		if (ClickedButton(codeDigitBtns[i], mx, my)) {
			if (codeGuessLength < 4) {
				int digitValue = (i == 9) ? 0 : (i + 1);
				codeCurrentGuess[codeGuessLength] = digitValue;
				codeGuessLength++;
			}
			return;
		}
	}

	if (ClickedButton(codeBackspaceBtn, mx, my)) {
		if (codeGuessLength > 0) codeGuessLength--;
		return;
	}

	if (ClickedButton(codeSubmitBtn, mx, my)) {
		if (codeGuessLength < 4) return;
		if (codeGuessCount >= CODE_MAX_GUESSES) return;

		CodeLockGuess& g = codeGuessHistory[codeGuessCount];
		bool secretUsed[4] = { false, false, false, false };
		bool guessUsed[4] = { false, false, false, false };
		g.exactMatches = 0;
		g.partialMatches = 0;

		for (int i = 0; i < 4; i++) {
			g.digits[i] = codeCurrentGuess[i];
			if (codeCurrentGuess[i] == codeSecret[i]) {
				g.exactMatches++;
				secretUsed[i] = true;
				guessUsed[i] = true;
			}
		}
		for (int i = 0; i < 4; i++) {
			if (guessUsed[i]) continue;
			for (int j = 0; j < 4; j++) {
				if (secretUsed[j]) continue;
				if (codeCurrentGuess[i] == codeSecret[j]) {
					g.partialMatches++;
					secretUsed[j] = true;
					break;
				}
			}
		}

		codeGuessCount++;
		codeGuessLength = 0;

		if (g.exactMatches == 4) {
			codeSolved = true;
			puzzleSolved = true;
			PlayCollectSound();

			if (inSegmentPuzzle) {
				const int SEGMENT_PUZZLE_HP_REWARD_PERCENT = 15;
				int reward = (playerMaxHealth * SEGMENT_PUZZLE_HP_REWARD_PERCENT) / 100;
				playerHealth += reward;
				if (playerHealth > playerMaxHealth) playerHealth = playerMaxHealth;

				inSegmentPuzzle = false;
				gameState = STATE_PLAY;
			}
			else {
				if (selectedLevel == 1) level2Unlocked = true;
				else if (selectedLevel == 2) level3Unlocked = true;
				else if (selectedLevel == 3) level4Unlocked = true; // Level 4 unlock: completing Level 3
				gameState = STATE_LEVEL_WIN;
			}
		}
	}
}

// ---------------- Sliding tile puzzle (Level 3 Part 2, 2nd puzzle) ----------------
// Classic 15-puzzle: a 4x4 grid holding tiles 1-15 plus one empty slot.
// Click any tile adjacent to the empty slot to slide it in. Win by
// arranging 1-15 in reading order with the empty slot last.

static const int TILE_GRID = 4;
static const int TILE_COUNT = TILE_GRID * TILE_GRID; // 16 (15 numbered + 1 empty)
static int tileBoard[TILE_COUNT]; // 0 = empty, 1-15 = numbered tiles
static bool tileSolved = false;
static int tileSize = 90;
static int tileGap = 8;
static int tileStartX, tileStartY;

static int FindEmptyIndex() {
	for (int i = 0; i < TILE_COUNT; i++) if (tileBoard[i] == 0) return i;
	return -1;
}

static void InitCircuitPuzzle() {
	for (int i = 0; i < TILE_COUNT - 1; i++) tileBoard[i] = i + 1;
	tileBoard[TILE_COUNT - 1] = 0; // empty slot, bottom-right

	// Shuffle via random VALID slides from the solved state -- this
	// guarantees the result is always actually solvable (unlike a pure
	// random shuffle, which is only solvable half the time).
	for (int shuffleStep = 0; shuffleStep < 200; shuffleStep++) {
		int emptyIdx = FindEmptyIndex();
		int emptyRow = emptyIdx / TILE_GRID;
		int emptyCol = emptyIdx % TILE_GRID;

		int neighbors[4];
		int neighborCount = 0;
		if (emptyRow > 0) neighbors[neighborCount++] = emptyIdx - TILE_GRID;
		if (emptyRow < TILE_GRID - 1) neighbors[neighborCount++] = emptyIdx + TILE_GRID;
		if (emptyCol > 0) neighbors[neighborCount++] = emptyIdx - 1;
		if (emptyCol < TILE_GRID - 1) neighbors[neighborCount++] = emptyIdx + 1;

		int pick = neighbors[rand() % neighborCount];
		int temp = tileBoard[emptyIdx];
		tileBoard[emptyIdx] = tileBoard[pick];
		tileBoard[pick] = temp;
	}

	tileSolved = false;
	puzzleSolved = false;
	puzzleTimer = 150.0f;

	int gridPixelSize = TILE_GRID * tileSize + (TILE_GRID - 1) * tileGap;
	tileStartX = (SCREEN_WIDTH - gridPixelSize) / 2;
	tileStartY = (SCREEN_HEIGHT - gridPixelSize) / 2;
}

static void RenderCircuitPuzzle() {
	iSetColor(5, 10, 20);
	iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	iSetColor(255, 255, 255);
	iText(SCREEN_WIDTH / 2 - 220, SCREEN_HEIGHT - 60, "SLIDING PUZZLE: ARRANGE 1-15 IN ORDER", GLUT_BITMAP_TIMES_ROMAN_24);
	iSetColor(0, 200, 255);
	iText(SCREEN_WIDTH / 2 - 230, SCREEN_HEIGHT - 90, "Click a tile next to the empty slot to slide it in", GLUT_BITMAP_HELVETICA_18);

	char timeStr[32];
	sprintf(timeStr, "TIME: %.1fs", puzzleTimer);
	if (puzzleTimer < 20.0f) iSetColor(255, 60, 60); else iSetColor(200, 255, 255);
	iText(SCREEN_WIDTH - 220, SCREEN_HEIGHT - 50, timeStr, GLUT_BITMAP_TIMES_ROMAN_24);

	for (int i = 0; i < TILE_COUNT; i++) {
		int row = i / TILE_GRID;
		int col = i % TILE_GRID;
		int x = tileStartX + col * (tileSize + tileGap);
		int y = tileStartY + row * (tileSize + tileGap);

		if (tileBoard[i] == 0) continue; // empty slot -- draw nothing

		bool inCorrectSpot = (tileBoard[i] == i + 1);
		if (inCorrectSpot) iSetColor(30, 120, 60);
		else iSetColor(50, 55, 70);
		iFilledRectangle(x, y, tileSize, tileSize);

		iSetColor(0, 200, 255);
		iRectangle(x, y, tileSize, tileSize);

		char label[8];
		sprintf(label, "%d", tileBoard[i]);
		iSetColor(255, 255, 255);
		int textOffsetX = (tileBoard[i] < 10) ? 35 : 25;
		iText(x + textOffsetX, y + 40, label, GLUT_BITMAP_TIMES_ROMAN_24);
	}
}

static void UpdateCircuitPuzzle(float dt) {
	if (tileSolved) return;

	puzzleTimer -= dt;
	if (puzzleTimer <= 0) {
		puzzleTimer = 0;

		if (inSegmentPuzzle) {
			inSegmentPuzzle = false;
			gameState = STATE_PLAY;
		}
		else {
			gameState = STATE_GAME_OVER;
			PlayGameOverMusic();
		}
	}
}

static void HandleCircuitClick(int mx, int my) {
	if (tileSolved) return;

	for (int i = 0; i < TILE_COUNT; i++) {
		int row = i / TILE_GRID;
		int col = i % TILE_GRID;
		int x = tileStartX + col * (tileSize + tileGap);
		int y = tileStartY + row * (tileSize + tileGap);

		if (mx >= x && mx <= x + tileSize && my >= y && my <= y + tileSize) {
			if (tileBoard[i] == 0) return; // clicked the empty slot itself

			int emptyIdx = FindEmptyIndex();
			int emptyRow = emptyIdx / TILE_GRID;
			int emptyCol = emptyIdx % TILE_GRID;

			bool isAdjacent = (row == emptyRow && abs(col - emptyCol) == 1) ||
				(col == emptyCol && abs(row - emptyRow) == 1);

			if (isAdjacent) {
				int temp = tileBoard[i];
				tileBoard[i] = tileBoard[emptyIdx];
				tileBoard[emptyIdx] = temp;

				bool allCorrect = true;
				for (int j = 0; j < TILE_COUNT - 1; j++) {
					if (tileBoard[j] != j + 1) { allCorrect = false; break; }
				}
				if (allCorrect && tileBoard[TILE_COUNT - 1] == 0) {
					tileSolved = true;
					puzzleSolved = true;
					PlayCollectSound();

					if (inSegmentPuzzle) {
						const int SEGMENT_PUZZLE_HP_REWARD_PERCENT = 15;
						int reward = (playerMaxHealth * SEGMENT_PUZZLE_HP_REWARD_PERCENT) / 100;
						playerHealth += reward;
						if (playerHealth > playerMaxHealth) playerHealth = playerMaxHealth;
						inSegmentPuzzle = false;
						gameState = STATE_PLAY;
					}
					else {
						if (selectedLevel == 1) level2Unlocked = true;
						else if (selectedLevel == 2) level3Unlocked = true;
						else if (selectedLevel == 3) level4Unlocked = true; // Level 4 unlock: completing Level 3
						gameState = STATE_LEVEL_WIN;
					}
				}
			}
			return;
		}
	}
}
// ---------------- Public dispatch ----------------

void InitPuzzle() {
	if (currentLevel == 3 && currentPart == 2) {
		if (enemySegmentsCleared <= 1) {
			activePuzzleType = 1; // 1st segment cleared -> code lock
			InitCodeLockPuzzle();
		}
		else {
			activePuzzleType = 2; // 2nd segment cleared -> circuit
			InitCircuitPuzzle();
		}
	}
	else {
		activePuzzleType = 0;
		InitMemoryPuzzle();
	}
}

void renderPuzzle() {
	if (activePuzzleType == 1) RenderCodeLockPuzzle();
	else if (activePuzzleType == 2) RenderCircuitPuzzle();
	else RenderMemoryPuzzle();
}

void updatePuzzle(float dt) {
	if (activePuzzleType == 1) UpdateCodeLockPuzzle(dt);
	else if (activePuzzleType == 2) UpdateCircuitPuzzle(dt);
	else UpdateMemoryPuzzle(dt);
}

void handlePuzzleClick(int mx, int my) {
	if (activePuzzleType == 1) HandleCodeLockClick(mx, my);
	else if (activePuzzleType == 2) HandleCircuitClick(mx, my);
	else HandleMemoryPuzzleClick(mx, my);
}