// playerhealth.cpp -- see playerhealth.h for the overview.
#include "gameConfig.h"      // first: it defines _CRT_SECURE_NO_WARNINGS (the project builds with SDL checks on)
#include "playerhealth.h"
#include "defines.h"         // STATE_* constants
#include "variable.h"        // gameState, selectedLevel, lvl1Btn
#include "gameState.h"       // currentPart
#include "player.h"          // playerHealth, playerMaxHealth
#include "iGraphics.h"       // iText / iSetColor (Level Select read-back)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------
// Records kept in memory (oldest first) and where they are stored
// ---------------------------------------------------------------------
static struct PlayerHealthRecord g_records[PH_MAX_RECORDS];
static int  g_count = 0;
static int  g_prevState = 0;
static bool g_exitHookSet = false;
static char g_textPath[260] = PH_TEXT_FILE;
static char g_binaryPath[260] = PH_BINARY_FILE;

void PlayerHealthSetPaths(const char* textPath, const char* binaryPath) {
	strncpy(g_textPath, textPath, sizeof(g_textPath) - 1);     g_textPath[sizeof(g_textPath) - 1] = 0;
	strncpy(g_binaryPath, binaryPath, sizeof(g_binaryPath) - 1); g_binaryPath[sizeof(g_binaryPath) - 1] = 0;
}

const char* PlayerHealthResultName(int result) {
	if (result == PH_RESULT_COMPLETE) return "COMPLETE";
	if (result == PH_RESULT_GAMEOVER) return "GAMEOVER";
	return "LEFT";
}

static int ResultFromName(const char* name) {
	if (strcmp(name, "LEFT") == 0)     return PH_RESULT_LEFT;
	if (strcmp(name, "COMPLETE") == 0) return PH_RESULT_COMPLETE;
	if (strcmp(name, "GAMEOVER") == 0) return PH_RESULT_GAMEOVER;
	return -1;
}

// A record read from disk is only trusted if every field is in range, so a
// damaged or hand-edited file can never inject nonsense into the game.
static int RecordIsValid(const struct PlayerHealthRecord* r) {
	if (r->magic != PH_MAGIC) return 0;
	if (r->level < 1 || r->level > 4) return 0;
	if (r->part < 0 || r->part > 3) return 0;
	if (r->maxHealth < 1 || r->maxHealth > PH_HEALTH_LIMIT) return 0;
	if (r->health < 0 || r->health > r->maxHealth) return 0;
	if (r->result < PH_RESULT_LEFT || r->result > PH_RESULT_GAMEOVER) return 0;
	return 1;
}

// Adds r to records[0..*n), dropping the oldest when the array is already full.
static void PushRecord(struct PlayerHealthRecord* records, int* n, int maxCount, const struct PlayerHealthRecord* r) {
	if (*n >= maxCount) {
		memmove(records, records + 1, (maxCount - 1) * sizeof(struct PlayerHealthRecord));
		*n = maxCount - 1;
	}
	records[*n] = *r;
	(*n)++;
}

// ---------------------------------------------------------------------
// TEXT file: fprintf to write, fscanf to read
// ---------------------------------------------------------------------
int PlayerHealthSaveText(const char* path, const struct PlayerHealthRecord* records, int count) {
	FILE* fptr1 = fopen(path, "w");
	if (fptr1 == NULL) return 0;

	fprintf(fptr1, "PLAYER HEALTH FILE\nRecords: %d\n", count);
	for (int i = 0; i < count; i++) {
		fprintf(fptr1, "\nLevel: %d\nPart: %d\nHealth: %d\nMaxHealth: %d\nResult: %s\n",
			records[i].level, records[i].part, records[i].health, records[i].maxHealth,
			PlayerHealthResultName(records[i].result));
	}

	int failed = ferror(fptr1);
	fclose(fptr1);
	return failed ? 0 : 1;
}

int PlayerHealthLoadText(const char* path, struct PlayerHealthRecord* records, int maxCount) {
	FILE* fptr2 = fopen(path, "r");
	if (fptr2 == NULL) return 0;

	int declared = 0, n = 0;
	if (fscanf(fptr2, " PLAYER HEALTH FILE Records: %d", &declared) != 1 || declared < 0) {
		fclose(fptr2);
		return 0;
	}

	for (int i = 0; i < declared; i++) {
		struct PlayerHealthRecord r;
		char resultName[16];
		if (fscanf(fptr2, " Level: %d Part: %d Health: %d MaxHealth: %d Result: %15s",
			&r.level, &r.part, &r.health, &r.maxHealth, resultName) != 5) break;
		r.magic = PH_MAGIC;
		r.result = ResultFromName(resultName);
		if (r.result < 0 || !RecordIsValid(&r)) break;
		PushRecord(records, &n, maxCount, &r);
	}

	fclose(fptr2);
	return n;
}

// ---------------------------------------------------------------------
// BINARY file: fwrite / fread of whole structs
// ---------------------------------------------------------------------
int PlayerHealthSaveBinary(const char* path, const struct PlayerHealthRecord* records, int count) {
	FILE* fptr1 = fopen(path, "wb");
	if (fptr1 == NULL) return 0;

	size_t written = fwrite(records, sizeof(struct PlayerHealthRecord), (size_t)count, fptr1);
	int failed = ferror(fptr1);
	fclose(fptr1);
	return (!failed && written == (size_t)count) ? 1 : 0;
}

int PlayerHealthLoadBinary(const char* path, struct PlayerHealthRecord* records, int maxCount) {
	FILE* fptr2 = fopen(path, "rb");
	if (fptr2 == NULL) return 0;

	int n = 0;
	struct PlayerHealthRecord r;
	while (fread(&r, sizeof(struct PlayerHealthRecord), 1, fptr2) == 1) {   // a half-written last record is simply not read
		if (!RecordIsValid(&r)) break;
		PushRecord(records, &n, maxCount, &r);
	}

	fclose(fptr2);
	return n;
}

// ---------------------------------------------------------------------
// Game-facing part
// ---------------------------------------------------------------------
static bool InLevelState(int state) {
	return state == STATE_PLAY || state == STATE_PUZZLE || state == STATE_PUZZLE4 || state == STATE_PART_CONFIRM;
}

int PlayerHealthCount(void) { return g_count; }

const struct PlayerHealthRecord* PlayerHealthGet(int index) {
	return (index >= 0 && index < g_count) ? &g_records[index] : NULL;
}

const struct PlayerHealthRecord* PlayerHealthLast(void) {
	return g_count > 0 ? &g_records[g_count - 1] : NULL;
}

void PlayerHealthRecordNow(int result) {
	if (selectedLevel < 1 || selectedLevel > 4) return;    // no level has been started: nothing to record

	struct PlayerHealthRecord r;
	r.magic = PH_MAGIC;
	r.level = selectedLevel;
	r.part = (selectedLevel == 3) ? currentPart : 0;       // currentPart is only meaningful in Level 3
	r.maxHealth = playerMaxHealth;
	r.health = playerHealth;
	r.result = result;
	if (r.maxHealth < 1) r.maxHealth = 1;
	if (r.health < 0) r.health = 0;
	if (r.health > r.maxHealth) r.health = r.maxHealth;
	if (!RecordIsValid(&r)) return;

	PushRecord(g_records, &g_count, PH_MAX_RECORDS, &r);

	// Both files are rewritten so they always describe the same records.
	if (!PlayerHealthSaveBinary(g_binaryPath, g_records, g_count)) fprintf(stderr, "playerhealth: could not write %s\n", g_binaryPath);
	if (!PlayerHealthSaveText(g_textPath, g_records, g_count))     fprintf(stderr, "playerhealth: could not write %s\n", g_textPath);
}

void PlayerHealthSaveOnExit(void) {
	if (InLevelState(gameState)) PlayerHealthRecordNow(PH_RESULT_LEFT);
}

void PlayerHealthInit(void) {
	g_count = PlayerHealthLoadBinary(g_binaryPath, g_records, PH_MAX_RECORDS);
	if (g_count == 0) g_count = PlayerHealthLoadText(g_textPath, g_records, PH_MAX_RECORDS);   // fallback
	g_prevState = gameState;

	if (!g_exitHookSet) {
		atexit(PlayerHealthSaveOnExit);    // ESC / END / EXIT button / closing the window all end in exit()
		g_exitHookSet = true;
	}
}

// Watches gameState (one place, so no level code has to know about this module).
void PlayerHealthUpdate(void) {
	int now = gameState;
	if (now == g_prevState) return;

	if (now == STATE_LEVEL_WIN)       PlayerHealthRecordNow(PH_RESULT_COMPLETE);
	else if (now == STATE_GAME_OVER)  PlayerHealthRecordNow(PH_RESULT_GAMEOVER);
	else if (InLevelState(g_prevState) && (now == STATE_LEVEL_SELECT || now == STATE_MAIN_MENU))
		PlayerHealthRecordNow(PH_RESULT_LEFT);

	g_prevState = now;
}

// Level Select: a small panel under the buttons showing the newest record.
void PlayerHealthDrawLast(void) {
	const struct PlayerHealthRecord* r = PlayerHealthLast();
	if (r == NULL) return;

	char line1[96], line2[96], where[24] = "";
	if (r->level == 3 && r->part > 0) sprintf(where, " (Part %d)", r->part);
	const char* what = (r->result == PH_RESULT_COMPLETE) ? "level complete"
	                 : (r->result == PH_RESULT_GAMEOVER) ? "game over" : "left the level";
	sprintf(line1, "Last run: Level %d%s - %s", r->level, where, what);
	sprintf(line2, "Health: %d / %d   (%d saved)", r->health, r->maxHealth, g_count);

	int x = lvl1Btn.x - 60, y = 108, w = 320, h = 58;
	iSetColor(220, 50, 50);
	iRectangle(x, y, w, h);
	iSetColor(40, 10, 10);
	iFilledRectangle(x + 2, y + 2, w - 4, h - 4);
	iSetColor(255, 255, 255);
	iText(x + 12, y + 34, line1, GLUT_BITMAP_HELVETICA_12);
	iText(x + 12, y + 12, line2, GLUT_BITMAP_HELVETICA_12);
}
