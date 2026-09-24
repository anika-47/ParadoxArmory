// =====================================================================
// PLAYER HEALTH FILES  (playerhealth.cpp)
// =====================================================================
//   g++ -std=c++03 -fpermissive -w -I. -o test_playerhealth test_playerhealth.cpp \
//       playerhealth.cpp iGraphicsGlobals.cpp player.cpp gameState.cpp variable.cpp
//
// Writes only to /tmp-style scratch files (PlayerHealthSetPaths), never to the
// game's real playerhealth.txt / playerhealth.dat.
// =====================================================================
#include "defines.h"
#include "variable.h"
#include "gameState.h"
#include "player.h"
#include "playerhealth.h"
#include <stdio.h>
#include <string.h>

static int failures = 0, checks = 0;
static void check(bool c, const char* what) {
	checks++;
	printf("  [%s] %s\n", c ? " ok " : "FAIL", what);
	if (!c) failures++;
}

static const char* TXT = "ph_test.txt";
static const char* BIN = "ph_test.dat";

static void wipe() { remove(TXT); remove(BIN); }
static void restart() { PlayerHealthInit(); }          // what a fresh launch does: read the files back
static bool same(const PlayerHealthRecord& a, const PlayerHealthRecord& b) {
	return a.magic == b.magic && a.level == b.level && a.part == b.part && a.health == b.health &&
	       a.maxHealth == b.maxHealth && a.result == b.result;
}
static PlayerHealthRecord mk(int level, int part, int hp, int max, int result) {
	PlayerHealthRecord r; r.magic = PH_MAGIC; r.level = level; r.part = part; r.health = hp; r.maxHealth = max; r.result = result; return r;
}
static long fileSize(const char* p) { FILE* f = fopen(p, "rb"); if (!f) return -1; fseek(f, 0, SEEK_END); long n = ftell(f); fclose(f); return n; }
static void writeRaw(const char* p, const char* data, size_t n) { FILE* f = fopen(p, "wb"); fwrite(data, 1, n, f); fclose(f); }

// put the game into "playing level L with hp/max" like the real game would be
static void inLevel(int level, int part, int hp, int max) {
	selectedLevel = level; currentLevel = level; currentPart = part; playerHealth = hp; playerMaxHealth = max; gameState = STATE_PLAY;
}

int main() {
	printf("=============== PLAYER HEALTH FILES TEST ===============\n");
	PlayerHealthSetPaths(TXT, BIN);
	wipe();

	printf("\n-- struct layout --\n");
	check(sizeof(PlayerHealthRecord) == 6 * sizeof(int), "record is 6 ints, no padding (binary file layout is stable)");

	printf("\n-- low-level TEXT file: fprintf out, fscanf back --\n");
	PlayerHealthRecord a[3] = { mk(1, 0, 7400, 10000, PH_RESULT_COMPLETE), mk(3, 2, 0, 30000, PH_RESULT_GAMEOVER), mk(4, 0, 15250, 20000, PH_RESULT_LEFT) };
	check(PlayerHealthSaveText(TXT, a, 3) == 1, "text file is written");
	PlayerHealthRecord back[PH_MAX_RECORDS];
	int n = PlayerHealthLoadText(TXT, back, PH_MAX_RECORDS);
	check(n == 3 && same(back[0], a[0]) && same(back[1], a[1]) && same(back[2], a[2]), "3 records read back exactly");
	{
		FILE* f = fopen(TXT, "r"); char l1[64] = "", l2[64] = ""; fgets(l1, 64, f); fgets(l2, 64, f); fclose(f);
		check(strncmp(l1, "PLAYER HEALTH FILE", 18) == 0 && strncmp(l2, "Records: 3", 10) == 0, "file is human-readable (header + record count)");
	}

	printf("\n-- low-level BINARY file: fwrite out, fread back --\n");
	check(PlayerHealthSaveBinary(BIN, a, 3) == 1, "binary file is written");
	check(fileSize(BIN) == 3 * (long)sizeof(PlayerHealthRecord), "file size = 3 structs");
	n = PlayerHealthLoadBinary(BIN, back, PH_MAX_RECORDS);
	check(n == 3 && same(back[0], a[0]) && same(back[1], a[1]) && same(back[2], a[2]), "3 records read back exactly");

	printf("\n-- missing / damaged files never crash and never inject nonsense --\n");
	wipe();
	check(PlayerHealthLoadBinary(BIN, back, PH_MAX_RECORDS) == 0 && PlayerHealthLoadText(TXT, back, PH_MAX_RECORDS) == 0, "missing files -> 0 records");
	PlayerHealthSaveBinary(BIN, a, 3);
	{ // cut the last record in half
		FILE* f = fopen(BIN, "rb"); char buf[200]; size_t got = fread(buf, 1, 200, f); fclose(f);
		writeRaw(BIN, buf, got - sizeof(PlayerHealthRecord) / 2);
	}
	n = PlayerHealthLoadBinary(BIN, back, PH_MAX_RECORDS);
	check(n == 2 && same(back[0], a[0]) && same(back[1], a[1]), "binary cut mid-record -> the 2 complete records survive");
	writeRaw(BIN, "this is not a health file at all, just some text bytes", 52);
	check(PlayerHealthLoadBinary(BIN, back, PH_MAX_RECORDS) == 0, "binary garbage -> 0 records");
	{
		PlayerHealthRecord bad = mk(2, 0, 500, 1000, PH_RESULT_LEFT); bad.health = 99999;   // health above max
		PlayerHealthRecord mix[3] = { a[0], bad, a[2] };
		PlayerHealthSaveBinary(BIN, mix, 3);
		check(PlayerHealthLoadBinary(BIN, back, PH_MAX_RECORDS) == 1, "out-of-range value -> reading stops at the bad record");
		bad = mk(9, 0, 5, 10, PH_RESULT_LEFT); mix[1] = bad; PlayerHealthSaveBinary(BIN, mix, 3);
		check(PlayerHealthLoadBinary(BIN, back, PH_MAX_RECORDS) == 1, "level 9 -> rejected");
		bad = mk(2, 0, 5, 10, PH_RESULT_LEFT); bad.magic = 0; mix[1] = bad; PlayerHealthSaveBinary(BIN, mix, 3);
		check(PlayerHealthLoadBinary(BIN, back, PH_MAX_RECORDS) == 1, "wrong magic number -> rejected");
	}
	writeRaw(TXT, "hello", 5);
	check(PlayerHealthLoadText(TXT, back, PH_MAX_RECORDS) == 0, "text garbage -> 0 records");
	writeRaw(TXT, "PLAYER HEALTH FILE\nRecords: 2\n\nLevel: 1\nPart: 0\nHealth: 10\nMaxHealth: 100\nResult: COMPLETE\n\nLevel: 2\nPart: 0\nHea", 110);
	check(PlayerHealthLoadText(TXT, back, PH_MAX_RECORDS) == 1, "text cut off mid-record -> the 1 complete record survives");
	writeRaw(TXT, "PLAYER HEALTH FILE\nRecords: 1\n\nLevel: 1\nPart: 0\nHealth: 10\nMaxHealth: 100\nResult: WINNER\n", 84);
	check(PlayerHealthLoadText(TXT, back, PH_MAX_RECORDS) == 0, "unknown result word -> rejected");
	{ // hand-edited text file claiming 15 records: only the newest 10 are kept, oldest first
		FILE* f = fopen(TXT, "w"); fprintf(f, "PLAYER HEALTH FILE\nRecords: 15\n");
		for (int i = 0; i < 15; i++) fprintf(f, "\nLevel: 1\nPart: 0\nHealth: %d\nMaxHealth: 20000\nResult: LEFT\n", 1000 + i);
		fclose(f);
		n = PlayerHealthLoadText(TXT, back, PH_MAX_RECORDS);
		check(n == PH_MAX_RECORDS && back[0].health == 1005 && back[9].health == 1014, "15 records in a file -> the newest 10 kept, oldest first");
	}

	printf("\n-- recording, both files written, restart reads them back --\n");
	wipe();
	gameState = STATE_MAIN_MENU; selectedLevel = 0;
	restart();
	check(PlayerHealthCount() == 0 && PlayerHealthLast() == 0, "fresh install: nothing saved");
	PlayerHealthRecordNow(PH_RESULT_COMPLETE);
	check(PlayerHealthCount() == 0, "nothing recorded before any level has been started");
	inLevel(2, 3, 12345, 20000);                       // currentPart is stale (3) but Level 2 has no parts
	PlayerHealthRecordNow(PH_RESULT_COMPLETE);
	check(PlayerHealthCount() == 1, "one record");
	check(fileSize(TXT) > 0 && fileSize(BIN) == (long)sizeof(PlayerHealthRecord), "BOTH files now exist");
	{
		const PlayerHealthRecord* r = PlayerHealthLast();
		check(r && r->level == 2 && r->part == 0 && r->health == 12345 && r->maxHealth == 20000 && r->result == PH_RESULT_COMPLETE,
			"Level 2: level, part 0 (not the stale currentPart), health, max, result");
	}
	inLevel(3, 2, 8000, 30000);
	PlayerHealthRecordNow(PH_RESULT_LEFT);
	check(PlayerHealthLast()->level == 3 && PlayerHealthLast()->part == 2, "Level 3 keeps its part");
	inLevel(1, 1, 250000, 10000);                        // impossible health is clamped, not stored raw
	PlayerHealthRecordNow(PH_RESULT_LEFT);
	check(PlayerHealthLast()->health == 10000, "health above max is clamped to max");
	inLevel(1, 1, -50, 10000);
	PlayerHealthRecordNow(PH_RESULT_GAMEOVER);
	check(PlayerHealthLast()->health == 0, "negative health is clamped to 0");

	PlayerHealthRecord snapshot[PH_MAX_RECORDS]; int snapN = PlayerHealthCount();
	for (int i = 0; i < snapN; i++) snapshot[i] = *PlayerHealthGet(i);
	restart();                                            // "close and reopen the game"
	bool sameAfter = (PlayerHealthCount() == snapN);
	for (int i = 0; i < snapN && sameAfter; i++) sameAfter = same(*PlayerHealthGet(i), snapshot[i]);
	check(sameAfter, "after a restart the same 4 records are loaded from playerhealth.dat");

	remove(BIN);
	restart();
	sameAfter = (PlayerHealthCount() == snapN);
	for (int i = 0; i < snapN && sameAfter; i++) sameAfter = same(*PlayerHealthGet(i), snapshot[i]);
	check(sameAfter, "binary deleted -> the text file is used instead, same records");
	PlayerHealthRecordNow(PH_RESULT_LEFT);                // writes both files again
	check(fileSize(BIN) > 0, "the next save recreates the binary file");

	printf("\n-- only the newest 10 are kept, in both files --\n");
	wipe(); gameState = STATE_MAIN_MENU; restart();
	for (int i = 0; i < 13; i++) { inLevel(1, 1, 100 + i, 10000); PlayerHealthRecordNow(PH_RESULT_LEFT); }
	check(PlayerHealthCount() == PH_MAX_RECORDS, "13 saves -> 10 kept");
	check(PlayerHealthGet(0)->health == 103 && PlayerHealthLast()->health == 112, "oldest 3 dropped; order preserved (103 ... 112)");
	check(fileSize(BIN) == PH_MAX_RECORDS * (long)sizeof(PlayerHealthRecord), "binary file stays at 10 structs");
	n = PlayerHealthLoadText(TXT, back, PH_MAX_RECORDS);
	check(n == 10 && back[0].health == 103 && back[9].health == 112, "text file holds the same 10");

	printf("\n-- state watcher: what triggers a record --\n");
	wipe(); gameState = STATE_MAIN_MENU; selectedLevel = 0; restart();
	int steps[] = { STATE_LEVEL_SELECT, STATE_INSTRUCTION, STATE_MAIN_MENU, STATE_LEVEL_SELECT };
	for (int i = 0; i < 4; i++) { gameState = steps[i]; PlayerHealthUpdate(); }
	check(PlayerHealthCount() == 0, "browsing menus records nothing");

	inLevel(3, 1, 30000, 30000); PlayerHealthUpdate();                      // start Level 3
	gameState = STATE_PART_CONFIRM; PlayerHealthUpdate();
	gameState = STATE_PLAY;         PlayerHealthUpdate();
	gameState = STATE_PUZZLE;       PlayerHealthUpdate();                    // a segment puzzle
	gameState = STATE_PLAY;         PlayerHealthUpdate();
	check(PlayerHealthCount() == 0, "part prompts and mid-level puzzles record nothing");

	playerHealth = 21000; currentPart = 3;
	gameState = STATE_LEVEL_WIN; PlayerHealthUpdate();
	check(PlayerHealthCount() == 1 && PlayerHealthLast()->result == PH_RESULT_COMPLETE && PlayerHealthLast()->level == 3 &&
	      PlayerHealthLast()->part == 3 && PlayerHealthLast()->health == 21000, "winning -> COMPLETE (level 3, part 3, 21000 HP)");
	PlayerHealthUpdate(); PlayerHealthUpdate();
	check(PlayerHealthCount() == 1, "staying on the win screen does not record again");

	inLevel(4, 1, 20000, 20000); PlayerHealthUpdate();                       // 'Continue' into Level 4
	check(PlayerHealthCount() == 1, "Continue into the next level records nothing");
	playerHealth = 0; gameState = STATE_GAME_OVER; PlayerHealthUpdate();
	check(PlayerHealthCount() == 2 && PlayerHealthLast()->result == PH_RESULT_GAMEOVER && PlayerHealthLast()->level == 4 &&
	      PlayerHealthLast()->health == 0, "losing -> GAMEOVER (level 4, 0 HP)");
	gameState = STATE_LEVEL_SELECT; PlayerHealthUpdate();
	check(PlayerHealthCount() == 2, "leaving the game-over screen records nothing");

	inLevel(1, 1, 6500, 10000); PlayerHealthUpdate();
	gameState = STATE_LEVEL_SELECT; PlayerHealthUpdate();                    // 'B' / back
	check(PlayerHealthCount() == 3 && PlayerHealthLast()->result == PH_RESULT_LEFT && PlayerHealthLast()->health == 6500, "back to Level Select mid-level -> LEFT (6500 HP)");
	inLevel(2, 1, 9000, 20000); PlayerHealthUpdate();
	gameState = STATE_MAIN_MENU; PlayerHealthUpdate();
	check(PlayerHealthCount() == 4 && PlayerHealthLast()->level == 2, "back to the main menu mid-level -> LEFT");
	inLevel(3, 2, 4000, 30000); PlayerHealthUpdate();
	gameState = STATE_PART_CONFIRM; PlayerHealthUpdate(); gameState = STATE_LEVEL_SELECT; PlayerHealthUpdate();   // answered NO
	check(PlayerHealthCount() == 5 && PlayerHealthLast()->part == 2, "answering NO at the part prompt -> LEFT (level 3, part 2)");

	printf("\n-- closing the game --\n");
	inLevel(2, 1, 7777, 20000); PlayerHealthUpdate();
	PlayerHealthSaveOnExit();
	check(PlayerHealthCount() == 6 && PlayerHealthLast()->health == 7777 && PlayerHealthLast()->result == PH_RESULT_LEFT, "exit while playing -> LEFT (7777 HP)");
	gameState = STATE_MAIN_MENU; selectedLevel = 2;
	PlayerHealthSaveOnExit();
	check(PlayerHealthCount() == 6, "exit from a menu records nothing");
	gameState = STATE_GAME_OVER;
	PlayerHealthSaveOnExit();
	check(PlayerHealthCount() == 6, "exit from the game-over screen records nothing (already recorded)");

	printf("\n-- read-back on Level Select --\n");
	PlayerHealthDrawLast();
	check(true, "draws the newest record without crashing");
	wipe(); gameState = STATE_MAIN_MENU; selectedLevel = 0; restart();
	PlayerHealthDrawLast();
	check(true, "draws nothing (and does not crash) when there are no records");

	printf("\n-- unwritable location does not crash the game --\n");
	PlayerHealthSetPaths("/nonexistent_dir/x.txt", "/nonexistent_dir/x.dat");
	inLevel(1, 1, 100, 10000);
	PlayerHealthRecordNow(PH_RESULT_LEFT);
	check(PlayerHealthCount() == 1, "still keeps the record in memory; the failure is only logged");

	wipe();
	printf("\n%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
