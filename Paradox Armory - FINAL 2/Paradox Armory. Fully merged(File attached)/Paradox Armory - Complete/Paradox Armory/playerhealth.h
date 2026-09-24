// =====================================================================
// playerhealth.h / playerhealth.cpp  --  FILE HANDLING FOR THE PLAYER'S HEALTH
// =====================================================================
// Every time a level is completed, the game is lost, or the player leaves a
// level, the player's health is written to TWO files (in the folder the game
// runs from, next to "images"):
//
//     playerhealth.txt   readable text   (fprintf to write, fscanf to read)
//     playerhealth.dat   binary structs  (fwrite to write, fread to read)
//
// Both hold the same last PH_MAX_RECORDS records, oldest first. On start-up the
// binary file is read (the text file is the fallback if it is missing/damaged),
// and the Level Select screen shows the most recent record.
//
// This only RECORDS health. It never changes it: gameplay, damage and the
// full-health start of every level are exactly as before.
//
// Text file layout:
//     PLAYER HEALTH FILE
//     Records: 2
//
//     Level: 1
//     Part: 0
//     Health: 7400
//     MaxHealth: 10000
//     Result: COMPLETE
//
//     Level: 3
//     Part: 2
//     Health: 0
//     MaxHealth: 30000
//     Result: GAMEOVER
// =====================================================================
#ifndef PLAYERHEALTH_H
#define PLAYERHEALTH_H

#define PH_TEXT_FILE     "playerhealth.txt"
#define PH_BINARY_FILE   "playerhealth.dat"
#define PH_MAX_RECORDS   10            // how many of the most recent records are kept
#define PH_MAGIC         0x50414831    // "PAH1": marks a valid binary record
#define PH_HEALTH_LIMIT  1000000       // anything above this in a file is treated as damaged

// how the visit to the level ended
#define PH_RESULT_LEFT      0   // went back to the menu / closed the game mid-level
#define PH_RESULT_COMPLETE  1   // finished the level
#define PH_RESULT_GAMEOVER  2   // lost

// One saved moment. Six ints, so the binary layout has no padding.
struct PlayerHealthRecord {
	int magic;       // always PH_MAGIC in a valid record
	int level;       // 1..4
	int part;        // Level 3's part 1..3; 0 for every other level
	int health;      // 0..maxHealth
	int maxHealth;   // that level's full health
	int result;      // PH_RESULT_*
};

// ---- low-level file functions (the caller gives the path) -------------
// Save*: write all 'count' records, replacing the file. Return 1 on success, 0 on failure.
// Load*: read up to 'maxCount' records (the NEWEST if the file has more), stop at the first
//        damaged record. Return how many valid records were read (0 if the file is missing).
int PlayerHealthSaveText  (const char* path, const struct PlayerHealthRecord* records, int count);
int PlayerHealthLoadText  (const char* path, struct PlayerHealthRecord* records, int maxCount);
int PlayerHealthSaveBinary(const char* path, const struct PlayerHealthRecord* records, int count);
int PlayerHealthLoadBinary(const char* path, struct PlayerHealthRecord* records, int maxCount);

const char* PlayerHealthResultName(int result);   // "LEFT" / "COMPLETE" / "GAMEOVER"

// ---- used by the game ---------------------------------------------------
void PlayerHealthInit(void);              // once at start-up: load the saved records, arrange the exit-time save
void PlayerHealthUpdate(void);            // once per tick: notices level complete / game over / leaving a level
void PlayerHealthRecordNow(int result);   // add a record from the CURRENT health and write both files
void PlayerHealthSaveOnExit(void);        // called when the program exits; records "left" if a level is in progress
void PlayerHealthDrawLast(void);          // Level Select: draws the most recent record (nothing if there is none)

// ---- read access (used by the tests) -----------------------------------
int  PlayerHealthCount(void);
const struct PlayerHealthRecord* PlayerHealthGet(int index);   // 0 = oldest; NULL if out of range
const struct PlayerHealthRecord* PlayerHealthLast(void);       // newest; NULL if there are none
void PlayerHealthSetPaths(const char* textPath, const char* binaryPath);   // default: the two names above

#endif // PLAYERHEALTH_H
