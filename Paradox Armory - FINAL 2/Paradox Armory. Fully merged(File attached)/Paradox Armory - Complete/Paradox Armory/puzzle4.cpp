// =====================================================================
// LEVEL 4 FINAL PUZZLE - "THE NINE-TAILS SEAL"
// =====================================================================
// See puzzle4.h for the design summary.
//
// Solvability guarantee
// ---------------------
// Every random element (plate layout, testimonies, flash sequence, lock
// rules) is generated first and then CHECKED:
//   * stage 1: the three testimonies are enumerated against all 5 plates
//     x 3 possible liars - the generator only accepts a set that leaves
//     exactly one (plate, liar) pair standing.
//   * stage 4: the lock rules are enumerated against all 125 dial
//     settings - the generator only accepts a set with exactly one
//     survivor.
//   * stages 2, 3 and 5 are pure functions of the printed rules and of
//     the results of the earlier stages, all of which stay on screen.
// If a random draw cannot be made unique, a direct fallback set that is
// unique by construction is used, so InitPuzzle4() can never loop
// forever and can never produce an ambiguous puzzle.
// =====================================================================

#include "puzzle4.h"
#include "defines.h"
#include "variable.h"
#include "gameState.h"
#include "player.h"
#include "audio4.h"
#include "iGraphics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ---------------------------------------------------------------------
// constants and tables
// ---------------------------------------------------------------------
static const int PZ4_MAX_ATTEMPTS = 5;
static const int PZ4_N            = 5;      // the five chakra natures
static const int PZ4_SEQ_LEN      = 6;      // flashes in stage 3
static const int PZ4_MAX_REPLAYS  = 2;
static const int PZ4_LIT_TICKS    = 42;     // how long one flash stays lit
static const int PZ4_SLOT_TICKS   = 60;     // lit + gap
static const float PZ4_PI         = 3.14159265f;

static const char* const PZ4_NAME[5] = { "FIRE", "WIND", "LIGHTNING", "EARTH", "WATER" };

// nature colours (also used, shuffled, as DECORATIVE plate frames)
static const int PZ4_COL[5][3] = {
	{ 240, 100,  40 },   // fire
	{ 120, 220, 150 },   // wind
	{ 250, 220,  70 },   // lightning
	{ 185, 132,  82 },   // earth
	{  80, 150, 240 }    // water
};

// "A overpowers B":  FIRE > WIND > LIGHTNING > EARTH > WATER > FIRE
static int PZ4Succ(int n) { return (n + 1) % 5; }      // the nature n overpowers
static int PZ4Pred(int n) { return (n + 4) % 5; }      // the nature that overpowers n

// ---------------------------------------------------------------------
// state
// ---------------------------------------------------------------------
struct PZ4Stmt { int type; int a; };                   // one witness testimony
struct PZ4Clue { int type; int x; int y; int k; };     // one logic-lock rule
struct PZ4Rect { int x, y, w, h; };

static int  pz4Stage;                 // 1..5, 6 = solved
static int  pz4Attempts;
static int  pz4Tick;                  // animation clock (one per update tick)
static int  pz4Flash;                 // feedback flash countdown
static bool pz4FlashGood;
static int  pz4Burst;                 // stage-clear chakra burst countdown
static char pz4Msg[160];

// plates (stages 1 and 2)
static int  pz4PlateNat[5];           // nature shown on each plate (a permutation)
static int  pz4PlateLvl[5];           // chakra orbs on each plate (a permutation of 1..5)
static int  pz4PlateFrame[5];         // DECORATIVE frame colour index
static int  pz4PosOfNat[5];           // plate position holding each nature

// stage 1
static PZ4Stmt pz4Stmt[3];
static int  pz4Liar;                  // which testimony is the false one
static int  pz4Warden;                // the correct plate

// stage 2
static int  pz4Chain[4];              // required plate order
static int  pz4ChainLen;
static bool pz4Picked[5];
static int  pz4Step;                  // progress within the current stage

// stage 3
static int  pz4Flashes[6];            // what the seal shows
static int  pz4Answer[6];             // what must be pressed
static int  pz4Dir;                   // +1 along the arrows, -1 against
static bool pz4Shown;                 // has the sequence been shown since the last reset?
static bool pz4ShowActive;
static int  pz4ShowTick;
static int  pz4Replays;

// stage 4
static PZ4Clue pz4Clue[8];
static int  pz4ClueCount;
static int  pz4LockSol[3];
static int  pz4Dial[3];

// stage 5
static int  pz4Code[4];
static int  pz4Entry[4];
static int  pz4EntryLen;

// art (existing Naruto assets, loaded once)
static bool pz4ArtLoaded = false;
static int  pz4NarutoImg = 0;
static int  pz4BurstImg  = 0;

// ---------------------------------------------------------------------
// layout
// ---------------------------------------------------------------------
static PZ4Rect pz4Plate[5];
static PZ4Rect pz4NatBtn[5];
static PZ4Rect pz4ShowBtn;
static PZ4Rect pz4DialRc[3];
static PZ4Rect pz4ConfirmBtn;
static PZ4Rect pz4Slot[4];
static PZ4Rect pz4Digit[10];
static PZ4Rect pz4ClearBtn;
static PZ4Rect pz4EnterBtn;

static bool PZ4Hit(const PZ4Rect& r, int mx, int my) {
	return (mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h);
}

static void PZ4BuildLayout(void) {
	int i;

	// stages 1 & 2: five plates
	for (i = 0; i < 5; i++) {
		pz4Plate[i].x = 45 + i * 170;
		pz4Plate[i].y = 190;
		pz4Plate[i].w = 150;
		pz4Plate[i].h = 170;
	}

	// stage 3: five nature buttons + show button
	for (i = 0; i < 5; i++) {
		pz4NatBtn[i].x = 72 + i * 164;
		pz4NatBtn[i].y = 205;
		pz4NatBtn[i].w = 140;
		pz4NatBtn[i].h = 150;
	}
	pz4ShowBtn.x = 320; pz4ShowBtn.y = 125; pz4ShowBtn.w = 300; pz4ShowBtn.h = 50;

	// stage 4: three dials + confirm
	for (i = 0; i < 3; i++) {
		pz4DialRc[i].x = 60 + i * 200;
		pz4DialRc[i].y = 205;
		pz4DialRc[i].w = 170;
		pz4DialRc[i].h = 120;
	}
	pz4ConfirmBtn.x = 700; pz4ConfirmBtn.y = 225; pz4ConfirmBtn.w = 150; pz4ConfirmBtn.h = 70;

	// stage 5: four slots, digit pad, clear / enter
	for (i = 0; i < 4; i++) {
		pz4Slot[i].x = 254 + i * 114;
		pz4Slot[i].y = 186;
		pz4Slot[i].w = 90;
		pz4Slot[i].h = 90;
	}
	for (i = 0; i < 10; i++) {
		pz4Digit[i].x = 45 + i * 86;
		pz4Digit[i].y = 118;
		pz4Digit[i].w = 76;
		pz4Digit[i].h = 56;
	}
	pz4ClearBtn.x = 300; pz4ClearBtn.y = 40; pz4ClearBtn.w = 160; pz4ClearBtn.h = 48;
	pz4EnterBtn.x = 580; pz4EnterBtn.y = 40; pz4EnterBtn.w = 160; pz4EnterBtn.h = 48;
}

// ---------------------------------------------------------------------
// small helpers
// ---------------------------------------------------------------------
static int PZ4Rnd(int n) { return rand() % n; }

static void PZ4Shuffle5(int* a) {
	int i, j, t;
	for (i = 4; i > 0; i--) {
		j = PZ4Rnd(i + 1);
		t = a[i]; a[i] = a[j]; a[j] = t;
	}
}

static int PZ4WardenNat(void) { return pz4PlateNat[pz4Warden]; }
static int PZ4LastChainPlate(void) { return pz4Chain[pz4ChainLen - 1]; }
static int PZ4LastChainNat(void) { return pz4PlateNat[PZ4LastChainPlate()]; }

// ---------------------------------------------------------------------
// STAGE 1 generation: three testimonies, exactly one lies
// ---------------------------------------------------------------------
// type 0  WARDEN overpowers <a>          type 5  WARDEN holds FEWER chakra than <a>
// type 1  WARDEN is overpowered by <a>   type 6  WARDEN's chakra level is EVEN
// type 2  WARDEN stands LEFT of <a>      type 7  WARDEN's chakra level is ODD
// type 3  WARDEN stands RIGHT of <a>     type 8  WARDEN is NOT <a>
// type 4  WARDEN holds MORE chakra than <a>
static bool PZ4StmtTrue(const PZ4Stmt& s, int p) {
	switch (s.type) {
	case 0: return PZ4Succ(pz4PlateNat[p]) == s.a;
	case 1: return PZ4Succ(s.a) == pz4PlateNat[p];
	case 2: return p < pz4PosOfNat[s.a];
	case 3: return p > pz4PosOfNat[s.a];
	case 4: return pz4PlateLvl[p] > pz4PlateLvl[pz4PosOfNat[s.a]];
	case 5: return pz4PlateLvl[p] < pz4PlateLvl[pz4PosOfNat[s.a]];
	case 6: return (pz4PlateLvl[p] % 2) == 0;
	case 7: return (pz4PlateLvl[p] % 2) == 1;
	default: return pz4PlateNat[p] != s.a;
	}
}

// How many (plate, liar) pairs are consistent with the three testimonies?
static int PZ4CountStmtSolutions(int* plateOut, int* liarOut) {
	int p, l, i, count = 0;
	for (p = 0; p < 5; p++) {
		for (l = 0; l < 3; l++) {
			bool ok = true;
			for (i = 0; i < 3; i++) {
				bool t = PZ4StmtTrue(pz4Stmt[i], p);
				if (i == l) { if (t)  ok = false; }
				else        { if (!t) ok = false; }
			}
			if (ok) {
				count++;
				if (plateOut) *plateOut = p;
				if (liarOut)  *liarOut  = l;
			}
		}
	}
	return count;
}

static void PZ4GenPlates(void) {
	int i;
	for (i = 0; i < 5; i++) { pz4PlateNat[i] = i; pz4PlateLvl[i] = i + 1; pz4PlateFrame[i] = i; }
	PZ4Shuffle5(pz4PlateNat);
	PZ4Shuffle5(pz4PlateLvl);
	PZ4Shuffle5(pz4PlateFrame);
	for (i = 0; i < 5; i++) pz4PosOfNat[pz4PlateNat[i]] = i;
}

static void PZ4GenWitnesses(void) {
	int tries, i, k, p, l;

	for (tries = 0; tries < 6000; tries++) {
		pz4Warden = PZ4Rnd(5);
		pz4Liar   = PZ4Rnd(3);
		bool built = true;

		for (i = 0; i < 3 && built; i++) {
			bool placed = false;
			for (k = 0; k < 60 && !placed; k++) {
				PZ4Stmt s;
				s.type = PZ4Rnd(9);
				s.a    = PZ4Rnd(5);
				if (s.type >= 6 && s.type <= 7) s.a = 0;

				bool truth = PZ4StmtTrue(s, pz4Warden);
				if (truth != (i != pz4Liar)) continue;          // liar false, others true

				bool dup = false;
				for (l = 0; l < i; l++)
					if (pz4Stmt[l].type == s.type && pz4Stmt[l].a == s.a) dup = true;
				if (dup) continue;

				pz4Stmt[i] = s;
				placed = true;
			}
			if (!placed) built = false;
		}
		if (!built) continue;

		if (PZ4CountStmtSolutions(&p, &l) == 1 && p == pz4Warden && l == pz4Liar) return;
	}

	// Fallback, unique by construction: two TRUE statements that each pin
	// the Warden's nature from a different side, plus one false statement.
	// If either pinning statement were the lie, the other would still fix
	// the plate and then contradict the "lie", so the liar can only be #3.
	pz4Warden = PZ4Rnd(5);
	pz4Liar   = 2;
	pz4Stmt[0].type = 0; pz4Stmt[0].a = PZ4Succ(pz4PlateNat[pz4Warden]);
	pz4Stmt[1].type = 1; pz4Stmt[1].a = PZ4Pred(pz4PlateNat[pz4Warden]);
	pz4Stmt[2].type = 8; pz4Stmt[2].a = pz4PlateNat[pz4Warden];
}

// ---------------------------------------------------------------------
// STAGE 2 generation: the chain
// ---------------------------------------------------------------------
static void PZ4BuildChain(void) {
	int k;
	int n0 = PZ4WardenNat();
	pz4ChainLen = 0;
	for (k = 0; k < 5; k++) {
		int p = pz4PosOfNat[(n0 + k) % 5];
		if (pz4PlateLvl[p] == 1) continue;        // the drained plate is skipped
		pz4Chain[pz4ChainLen++] = p;
	}
	// exactly one plate has level 1, so the chain is always 4 long
}

// ---------------------------------------------------------------------
// STAGE 3 generation: the echo
// ---------------------------------------------------------------------
static void PZ4BuildMemory(void) {
	int i, j;

	pz4Dir = ((pz4PlateLvl[PZ4LastChainPlate()] % 2) == 0) ? +1 : -1;

	for (;;) {
		bool ok = true;
		for (i = 0; i < PZ4_SEQ_LEN; i++) {
			pz4Flashes[i] = PZ4Rnd(5);
			if (i > 0 && pz4Flashes[i] == pz4Flashes[i - 1]) ok = false;
		}
		if (!ok) continue;
		int distinct = 0;
		for (i = 0; i < 5; i++) {
			bool seen = false;
			for (j = 0; j < PZ4_SEQ_LEN; j++) if (pz4Flashes[j] == i) seen = true;
			if (seen) distinct++;
		}
		if (distinct >= 4) break;
	}

	for (i = 0; i < PZ4_SEQ_LEN; i++)
		pz4Answer[i] = (pz4Flashes[i] + pz4Dir + 5) % 5;
}

// ---------------------------------------------------------------------
// STAGE 4 generation: the logic lock
// ---------------------------------------------------------------------
// type 0  dial x is the nature that overpowers the WARDEN
// type 1  dial x is the nature the WARDEN overpowers
// type 2  dial x is a nature that never appears in the MEMORY ANSWER
// type 3  dial x is a nature that appears MORE THAN ONCE in the MEMORY ANSWER
// type 4  dial x overpowers dial y
// type 5  dial x and dial y are different natures
// type 6  dial x is the nature of the LAST plate sealed in the chain
// type 7  dial x is the nature of the FINAL entry of the MEMORY ANSWER
// type 8  dial x lies k steps AFTER the WARDEN's nature along the arrows (fallback)
static int PZ4AnswerCount(int nat) {
	int i, c = 0;
	for (i = 0; i < PZ4_SEQ_LEN; i++) if (pz4Answer[i] == nat) c++;
	return c;
}

static bool PZ4ClueTrue(const PZ4Clue& c, const int* d) {
	int w = PZ4WardenNat();
	switch (c.type) {
	case 0: return d[c.x] == PZ4Pred(w);
	case 1: return d[c.x] == PZ4Succ(w);
	case 2: return PZ4AnswerCount(d[c.x]) == 0;
	case 3: return PZ4AnswerCount(d[c.x]) >= 2;
	case 4: return d[c.y] == PZ4Succ(d[c.x]);
	case 5: return d[c.x] != d[c.y];
	case 6: return d[c.x] == PZ4LastChainNat();
	case 7: return d[c.x] == pz4Answer[PZ4_SEQ_LEN - 1];
	default: return d[c.x] == (w + c.k) % 5;
	}
}

static int PZ4CountLockSolutions(int* solOut) {
	int a, b, c, i, count = 0;
	int d[3];
	for (a = 0; a < 5; a++)
	for (b = 0; b < 5; b++)
	for (c = 0; c < 5; c++) {
		bool ok = true;
		d[0] = a; d[1] = b; d[2] = c;
		for (i = 0; i < pz4ClueCount && ok; i++)
			if (!PZ4ClueTrue(pz4Clue[i], d)) ok = false;
		if (ok) {
			count++;
			if (solOut) { solOut[0] = a; solOut[1] = b; solOut[2] = c; }
		}
	}
	return count;
}

static bool PZ4ClueDuplicate(const PZ4Clue& c, int upTo) {
	int i;
	for (i = 0; i < upTo; i++) {
		if (pz4Clue[i].type == c.type && pz4Clue[i].x == c.x && pz4Clue[i].y == c.y && pz4Clue[i].k == c.k)
			return true;
	}
	return false;
}

static void PZ4BuildLock(void) {
	int attempt, i, k;
	int sol[3];

	for (attempt = 0; attempt < 500; attempt++) {
		for (i = 0; i < 3; i++) sol[i] = PZ4Rnd(5);
		pz4ClueCount = 0;

		bool failed = false;
		while (pz4ClueCount < 6) {
			bool placed = false;
			for (k = 0; k < 80 && !placed; k++) {
				PZ4Clue c;
				c.type = PZ4Rnd(8);          // 0..7 ; type 8 is fallback only
				c.x = PZ4Rnd(3);
				c.y = (c.x + 1 + PZ4Rnd(2)) % 3;
				c.k = 0;
				if (c.type != 4 && c.type != 5) c.y = 0;
				if (c.type == 5 && c.y < c.x) { int t = c.x; c.x = c.y; c.y = t; }  // "A and B differ" is symmetric
				if (!PZ4ClueTrue(c, sol)) continue;
				if (PZ4ClueDuplicate(c, pz4ClueCount)) continue;
				pz4Clue[pz4ClueCount++] = c;
				placed = true;
			}
			if (!placed) { failed = true; break; }
			if (pz4ClueCount >= 4 && PZ4CountLockSolutions(0) == 1) break;
		}
		if (failed) continue;

		if (pz4ClueCount >= 4 && PZ4CountLockSolutions(0) == 1) {
			for (i = 0; i < 3; i++) pz4LockSol[i] = sol[i];
			return;
		}
	}

	// Fallback, unique by construction: each dial is pinned directly.
	for (i = 0; i < 3; i++) sol[i] = PZ4Rnd(5);
	pz4ClueCount = 3;
	for (i = 0; i < 3; i++) {
		pz4Clue[i].type = 8;
		pz4Clue[i].x = i;
		pz4Clue[i].y = 0;
		pz4Clue[i].k = (sol[i] - PZ4WardenNat() + 5) % 5;
		pz4LockSol[i] = sol[i];
	}
}

static void PZ4ResetDials(void) {
	int i;
	for (i = 0; i < 3; i++) pz4Dial[i] = (pz4LockSol[i] == 0) ? 1 : 0;   // never starts on the answer
}

// ---------------------------------------------------------------------
// STAGE 5 generation: the code
// ---------------------------------------------------------------------
static int PZ4DistinctAnswers(void) {
	int i, c = 0;
	for (i = 0; i < 5; i++) if (PZ4AnswerCount(i) > 0) c++;
	return c;
}

static void PZ4BuildCode(void) {
	pz4Code[0] = pz4PlateLvl[pz4Warden];                                              // WARDEN's chakra level
	pz4Code[1] = (pz4LockSol[0] + pz4LockSol[1] + pz4LockSol[2]) % 10;                // dial numbers added, last digit
	pz4Code[2] = PZ4DistinctAnswers();                                                // different natures in the echo
	pz4Code[3] = (pz4Answer[0] + PZ4LastChainNat()) % 10;                             // first echo + last chain, last digit
}

// ---------------------------------------------------------------------
// art (existing Naruto assets - nothing new is shipped for the puzzle)
// ---------------------------------------------------------------------
static void PZ4LoadArt(void) {
	char path[64];
	if (pz4ArtLoaded) return;
	pz4ArtLoaded = true;
	sprintf(path, "images\\naruto_idle_0_r.png");
	pz4NarutoImg = iLoadImage(path);
	sprintf(path, "images\\naruto_flash.png");
	pz4BurstImg = iLoadImage(path);
}

// ---------------------------------------------------------------------
// setup
// ---------------------------------------------------------------------
void InitPuzzle4(void) {
	int i;

	PZ4BuildLayout();
	PZ4LoadArt();

	pz4Stage     = 1;
	pz4Attempts  = PZ4_MAX_ATTEMPTS;
	pz4Tick      = 0;
	pz4Step      = 0;
	pz4Flash     = 0;
	pz4FlashGood = false;
	pz4Burst     = 0;
	pz4Msg[0]    = '\0';

	for (i = 0; i < 5; i++) pz4Picked[i] = false;
	for (i = 0; i < 4; i++) pz4Entry[i] = -1;
	pz4EntryLen   = 0;

	pz4Shown      = false;
	pz4ShowActive = false;
	pz4ShowTick   = 0;
	pz4Replays    = PZ4_MAX_REPLAYS;

	PZ4GenPlates();
	PZ4GenWitnesses();
	PZ4BuildChain();
	PZ4BuildMemory();
	PZ4BuildLock();
	PZ4ResetDials();
	PZ4BuildCode();

	sprintf(pz4Msg, "A seal of five natures bars the way. Read the witnesses carefully.");
}

// ---------------------------------------------------------------------
// failure / success - costs one attempt, resets the current stage only
// ---------------------------------------------------------------------
static void PZ4ResetStage(void) {
	int i;
	pz4Step = 0;
	for (i = 0; i < 5; i++) pz4Picked[i] = false;
	for (i = 0; i < 4; i++) pz4Entry[i] = -1;
	pz4EntryLen   = 0;
	pz4Shown      = false;
	pz4ShowActive = false;
	pz4ShowTick   = 0;
	pz4Replays    = PZ4_MAX_REPLAYS;
	PZ4ResetDials();
}

static void PZ4Fail(const char* why) {
	pz4Attempts--;
	pz4Flash = 55;
	pz4FlashGood = false;
	L4SfxPuzzleFail();

	if (pz4Attempts <= 0) {
		sprintf(pz4Msg, "THE SEAL LOCKS DOWN.");
		gameState = STATE_GAME_OVER;
		PlayGameOverMusic();
		return;
	}

	sprintf(pz4Msg, "%s  -  stage reset, %d attempt%s left",
		why, pz4Attempts, (pz4Attempts == 1) ? "" : "s");
	PZ4ResetStage();
}

static void PZ4Advance(const char* msg) {
	pz4Flash = 55;
	pz4FlashGood = true;
	pz4Burst = 46;
	sprintf(pz4Msg, "%s", msg);
	pz4Step = 0;
	PZ4ResetStage();
	L4SfxPuzzleStage();
}

// ---------------------------------------------------------------------
// text helpers for the on-screen rules
// ---------------------------------------------------------------------
static void PZ4StmtText(const PZ4Stmt& s, char* out) {
	switch (s.type) {
	case 0: sprintf(out, "The WARDEN overpowers the %s plate.", PZ4_NAME[s.a]); break;
	case 1: sprintf(out, "The WARDEN is overpowered by the %s plate.", PZ4_NAME[s.a]); break;
	case 2: sprintf(out, "The WARDEN stands somewhere to the LEFT of the %s plate.", PZ4_NAME[s.a]); break;
	case 3: sprintf(out, "The WARDEN stands somewhere to the RIGHT of the %s plate.", PZ4_NAME[s.a]); break;
	case 4: sprintf(out, "The WARDEN holds MORE chakra than the %s plate.", PZ4_NAME[s.a]); break;
	case 5: sprintf(out, "The WARDEN holds FEWER chakra than the %s plate.", PZ4_NAME[s.a]); break;
	case 6: sprintf(out, "The WARDEN's chakra level is EVEN."); break;
	case 7: sprintf(out, "The WARDEN's chakra level is ODD."); break;
	default: sprintf(out, "The WARDEN is NOT the %s plate.", PZ4_NAME[s.a]); break;
	}
}

static void PZ4ClueText(const PZ4Clue& c, char* out) {
	char X = (char)('A' + c.x);
	char Y = (char)('A' + c.y);
	switch (c.type) {
	case 0: sprintf(out, "Dial %c is the nature that OVERPOWERS the WARDEN's nature.", X); break;
	case 1: sprintf(out, "Dial %c is the nature the WARDEN's nature OVERPOWERS.", X); break;
	case 2: sprintf(out, "Dial %c is a nature that NEVER appears in the ECHO ANSWER.", X); break;
	case 3: sprintf(out, "Dial %c is a nature that appears MORE THAN ONCE in the ECHO ANSWER.", X); break;
	case 4: sprintf(out, "Dial %c OVERPOWERS Dial %c.", X, Y); break;
	case 5: sprintf(out, "Dial %c and Dial %c are DIFFERENT natures.", X, Y); break;
	case 6: sprintf(out, "Dial %c is the nature of the LAST plate sealed in the chain.", X); break;
	case 7: sprintf(out, "Dial %c is the nature of the FINAL entry of the ECHO ANSWER.", X); break;
	default:
		if (c.k == 0) sprintf(out, "Dial %c is the WARDEN's own nature.", X);
		else sprintf(out, "Dial %c lies %d step%s AFTER the WARDEN's nature along the arrows.", X, c.k, (c.k == 1) ? "" : "s");
		break;
	}
}

// ---------------------------------------------------------------------
// input
// ---------------------------------------------------------------------
static void PZ4HandleStage1(int mx, int my) {
	int i;
	for (i = 0; i < 5; i++) {
		if (!PZ4Hit(pz4Plate[i], mx, my)) continue;
		L4SfxPuzzleClick();
		if (i == pz4Warden) {
			pz4Stage = 2;
			PZ4Advance("THE WARDEN IS REVEALED - now seal the chain it would consume");
		}
		else {
			PZ4Fail("THAT PLATE IS NOT THE WARDEN");
		}
		return;
	}
}

static void PZ4HandleStage2(int mx, int my) {
	int i;
	for (i = 0; i < 5; i++) {
		if (!PZ4Hit(pz4Plate[i], mx, my)) continue;
		if (pz4Picked[i]) return;                       // already sealed - ignore, costs nothing

		L4SfxPuzzleClick();

		if (pz4PlateLvl[i] == 1) {
			PZ4Fail("THAT PLATE IS DRAINED");
			return;
		}
		if (i == pz4Chain[pz4Step]) {
			pz4Picked[i] = true;
			pz4Step++;
			if (pz4Step >= pz4ChainLen) {
				pz4Stage = 3;
				PZ4Advance("CHAIN SEALED - the seal answers with an echo");
			}
		}
		else {
			PZ4Fail("OUT OF ORDER");
		}
		return;
	}
}

static void PZ4HandleStage3(int mx, int my) {
	int i;

	if (PZ4Hit(pz4ShowBtn, mx, my)) {
		if (pz4ShowActive) return;
		if (pz4Shown) {
			if (pz4Replays <= 0) return;
			pz4Replays--;
		}
		L4SfxPuzzleClick();
		pz4Shown = true;
		pz4ShowActive = true;
		pz4ShowTick = 0;
		pz4Step = 0;                                    // a replay restarts the answer, for free
		sprintf(pz4Msg, "Watch closely. Answer each flash - not the flash itself.");
		pz4Flash = 0;
		return;
	}

	for (i = 0; i < 5; i++) {
		if (!PZ4Hit(pz4NatBtn[i], mx, my)) continue;
		if (!pz4Shown || pz4ShowActive) return;         // nothing to answer yet - costs nothing

		L4SfxPuzzleClick();
		if (i == pz4Answer[pz4Step]) {
			pz4Step++;
			if (pz4Step >= PZ4_SEQ_LEN) {
				pz4Stage = 4;
				PZ4Advance("ECHO ANSWERED - the lock now asks for a combination");
			}
		}
		else {
			PZ4Fail("WRONG ANSWER TO THE ECHO");
		}
		return;
	}
}

static void PZ4HandleStage4(int mx, int my) {
	int i;

	for (i = 0; i < 3; i++) {
		if (!PZ4Hit(pz4DialRc[i], mx, my)) continue;
		L4SfxPuzzleClick();
		pz4Dial[i] = (pz4Dial[i] + 1) % 5;
		return;
	}

	if (PZ4Hit(pz4ConfirmBtn, mx, my)) {
		L4SfxPuzzleClick();
		if (pz4Dial[0] == pz4LockSol[0] && pz4Dial[1] == pz4LockSol[1] && pz4Dial[2] == pz4LockSol[2]) {
			pz4Stage = 5;
			PZ4Advance("THE LOCK TURNS - the final code is all that remains");
		}
		else {
			PZ4Fail("THE LOCK REFUSES THAT COMBINATION");
		}
	}
}

static void PZ4HandleStage5(int mx, int my) {
	int i;

	for (i = 0; i < 10; i++) {
		if (!PZ4Hit(pz4Digit[i], mx, my)) continue;
		if (pz4EntryLen >= 4) return;
		L4SfxPuzzleClick();
		pz4Entry[pz4EntryLen++] = i;
		return;
	}

	if (PZ4Hit(pz4ClearBtn, mx, my)) {
		L4SfxPuzzleClick();
		for (i = 0; i < 4; i++) pz4Entry[i] = -1;
		pz4EntryLen = 0;
		return;
	}

	if (PZ4Hit(pz4EnterBtn, mx, my)) {
		L4SfxPuzzleClick();

		if (pz4EntryLen < 4) {
			sprintf(pz4Msg, "THE SEAL NEEDS ALL FOUR DIGITS");
			pz4Flash = 30;
			pz4FlashGood = false;
			return;
		}

		bool ok = true;
		for (i = 0; i < 4; i++) {
			if (pz4Entry[i] != pz4Code[i]) { ok = false; break; }
		}

		if (ok) {
			pz4Stage = 6;
			puzzleSolved = true;
			sprintf(pz4Msg, "THE NINE-TAILS SEAL OPENS");
			L4SfxLevelComplete();
			gameState = STATE_LEVEL_WIN;
		}
		else {
			PZ4Fail("CODE REJECTED");
		}
	}
}

void HandlePuzzle4Click(int mx, int my) {
	if (pz4Stage > 5) return;
	switch (pz4Stage) {
	case 1: PZ4HandleStage1(mx, my); break;
	case 2: PZ4HandleStage2(mx, my); break;
	case 3: PZ4HandleStage3(mx, my); break;
	case 4: PZ4HandleStage4(mx, my); break;
	default: PZ4HandleStage5(mx, my); break;
	}
}

void UpdatePuzzle4(float dt) {
	(void)dt;
	pz4Tick++;
	if (pz4Flash > 0) pz4Flash--;
	if (pz4Burst > 0) pz4Burst--;

	if (pz4ShowActive) {
		if ((pz4ShowTick % PZ4_SLOT_TICKS) == 0 && pz4ShowTick < PZ4_SEQ_LEN * PZ4_SLOT_TICKS)
			L4SfxPuzzleClick();
		pz4ShowTick++;
		if (pz4ShowTick >= PZ4_SEQ_LEN * PZ4_SLOT_TICKS + 20) {
			pz4ShowActive = false;
			sprintf(pz4Msg, "Now press your answers, in order.");
			pz4Flash = 0;
		}
	}
}

// ---------------------------------------------------------------------
// drawing helpers
// ---------------------------------------------------------------------
static int PZ4TextW(const char* s, void* font) {
	int per = 7;
	if (font == GLUT_BITMAP_HELVETICA_18) per = 10;
	else if (font == GLUT_BITMAP_TIMES_ROMAN_24) per = 13;
	return per * (int)strlen(s);
}

static void PZ4Text(int x, int y, const char* s, void* font) {
	char buf[240];
	strncpy(buf, s, 239);
	buf[239] = '\0';
	iText(x, y, buf, font);
}

static void PZ4TextCentered(int cx, int y, const char* s, void* font) {
	PZ4Text(cx - PZ4TextW(s, font) / 2, y, s, font);
}

static void PZ4DrawPanel(int x, int y, int w, int h, int r, int g, int b) {
	iSetColor(8, 14, 20);
	iFilledRectangle(x, y, w, h);
	iSetColor(r, g, b);
	iRectangle(x, y, w, h);
}

static void PZ4SetNatColor(int n, int scalePct) {
	iSetColor((PZ4_COL[n][0] * scalePct) / 100, (PZ4_COL[n][1] * scalePct) / 100, (PZ4_COL[n][2] * scalePct) / 100);
}

// One chakra-nature emblem, drawn with plain iGraphics shapes.
static void PZ4DrawEmblem(int nat, float cx, float cy, float s, int scalePct) {
	double xs[6], ys[6];
	int k;

	PZ4SetNatColor(nat, scalePct);
	switch (nat) {
	case 0:   // FIRE : a flame
		xs[0] = cx;            ys[0] = cy + s;
		xs[1] = cx + 0.62f * s; ys[1] = cy - 0.15f * s;
		xs[2] = cx + 0.36f * s; ys[2] = cy - 0.80f * s;
		xs[3] = cx - 0.36f * s; ys[3] = cy - 0.80f * s;
		xs[4] = cx - 0.62f * s; ys[4] = cy - 0.15f * s;
		iFilledPolygon(xs, ys, 5);
		iSetColor(255, 225, 130);
		xs[0] = cx;            ys[0] = cy + 0.35f * s;
		xs[1] = cx + 0.30f * s; ys[1] = cy - 0.25f * s;
		xs[2] = cx + 0.18f * s; ys[2] = cy - 0.62f * s;
		xs[3] = cx - 0.18f * s; ys[3] = cy - 0.62f * s;
		xs[4] = cx - 0.30f * s; ys[4] = cy - 0.25f * s;
		iFilledPolygon(xs, ys, 5);
		break;
	case 1:   // WIND : three streaming lines
		for (k = -1; k <= 1; k++) {
			int seg;
			float py = cy + k * 0.55f * s;
			for (seg = 0; seg < 8; seg++) {
				float t0 = seg / 8.0f, t1 = (seg + 1) / 8.0f;
				float x0 = cx - s + 2.0f * s * t0, x1 = cx - s + 2.0f * s * t1;
				float y0 = py + 0.20f * s * (float)sin(t0 * 6.28f + k);
				float y1 = py + 0.20f * s * (float)sin(t1 * 6.28f + k);
				iLine(x0, y0, x1, y1);
				iLine(x0, y0 + 1, x1, y1 + 1);
			}
		}
		break;
	case 2:   // LIGHTNING : a bolt
		xs[0] = cx + 0.30f * s; ys[0] = cy + s;
		xs[1] = cx - 0.55f * s; ys[1] = cy - 0.10f * s;
		xs[2] = cx + 0.02f * s; ys[2] = cy - 0.05f * s;
		iFilledPolygon(xs, ys, 3);
		xs[0] = cx - 0.30f * s; ys[0] = cy - s;
		xs[1] = cx + 0.55f * s; ys[1] = cy + 0.10f * s;
		xs[2] = cx - 0.02f * s; ys[2] = cy + 0.05f * s;
		iFilledPolygon(xs, ys, 3);
		break;
	case 3:   // EARTH : two peaks on a base
		xs[0] = cx - s;         ys[0] = cy - 0.60f * s;
		xs[1] = cx - 0.15f * s; ys[1] = cy + 0.85f * s;
		xs[2] = cx + 0.55f * s; ys[2] = cy - 0.60f * s;
		iFilledPolygon(xs, ys, 3);
		xs[0] = cx + 0.05f * s; ys[0] = cy - 0.60f * s;
		xs[1] = cx + 0.55f * s; ys[1] = cy + 0.25f * s;
		xs[2] = cx + s;         ys[2] = cy - 0.60f * s;
		iFilledPolygon(xs, ys, 3);
		iFilledRectangle(cx - s, cy - 0.85f * s, 2.0f * s, 0.25f * s);
		break;
	default:  // WATER : a droplet
		xs[0] = cx;             ys[0] = cy + s;
		xs[1] = cx - 0.55f * s; ys[1] = cy - 0.20f * s;
		xs[2] = cx + 0.55f * s; ys[2] = cy - 0.20f * s;
		iFilledPolygon(xs, ys, 3);
		iFilledCircle(cx, cy - 0.28f * s, 0.58f * s);
		iSetColor(210, 235, 255);
		iFilledCircle(cx - 0.18f * s, cy - 0.20f * s, 0.14f * s);
		break;
	}
}

static void PZ4DrawButton(const PZ4Rect& rc, const char* label, bool enabled) {
	if (enabled) iSetColor(26, 38, 48); else iSetColor(30, 22, 22);
	iFilledRectangle(rc.x, rc.y, rc.w, rc.h);
	if (enabled) iSetColor(0, 190, 210); else iSetColor(110, 60, 60);
	iRectangle(rc.x, rc.y, rc.w, rc.h);
	if (enabled) iSetColor(230, 245, 250); else iSetColor(130, 90, 90);
	PZ4TextCentered(rc.x + rc.w / 2, rc.y + rc.h / 2 - 7, label, GLUT_BITMAP_HELVETICA_18);
}

// A plate: emblem + name + chakra orbs. The frame colour is decoration only.
static void PZ4DrawPlate(int i, bool picked, int orderBadge) {
	const PZ4Rect& rc = pz4Plate[i];
	int nat = pz4PlateNat[i];
	int frame = pz4PlateFrame[i];
	int k;
	char buf[16];

	if (picked) iSetColor(16, 58, 44); else iSetColor(20, 28, 38);
	iFilledRectangle(rc.x, rc.y, rc.w, rc.h);

	if (picked) iSetColor(0, 235, 165);
	else        iSetColor(PZ4_COL[frame][0] * 7 / 10, PZ4_COL[frame][1] * 7 / 10, PZ4_COL[frame][2] * 7 / 10);
	iRectangle(rc.x, rc.y, rc.w, rc.h);
	iRectangle(rc.x + 4, rc.y + 4, rc.w - 8, rc.h - 8);

	PZ4DrawEmblem(nat, rc.x + rc.w * 0.5f, rc.y + 112, 34.0f, 100);

	iSetColor(225, 238, 245);
	PZ4TextCentered(rc.x + rc.w / 2, rc.y + 58, PZ4_NAME[nat], GLUT_BITMAP_HELVETICA_12);

	// chakra orbs: filled = held chakra
	for (k = 0; k < 5; k++) {
		float ox = rc.x + rc.w * 0.5f + (k - 2) * 22.0f;
		float oy = rc.y + 28.0f;
		if (k < pz4PlateLvl[i]) { iSetColor(120, 220, 255); iFilledCircle(ox, oy, 8); }
		else                    { iSetColor(60, 80, 96);   iCircle(ox, oy, 8); }
	}

	if (orderBadge > 0) {
		sprintf(buf, "#%d", orderBadge);
		iSetColor(0, 255, 180);
		PZ4Text(rc.x + rc.w - 30, rc.y + rc.h - 22, buf, GLUT_BITMAP_HELVETICA_12);
	}
}

// The always-visible reference: the nature cycle and the nature numbers.
static void PZ4DrawCycleRef(void) {
	const int px = 925, py = 300, pw = 250, ph = 235;
	const float cx = px + pw * 0.5f, cy = py + 92.0f, R = 60.0f;
	float nx[5], ny[5];
	int k;

	PZ4DrawPanel(px, py, pw, ph, 0, 170, 190);
	iSetColor(255, 225, 140);
	PZ4Text(px + 12, py + ph - 24, "THE NATURE CYCLE", GLUT_BITMAP_HELVETICA_12);
	iSetColor(150, 170, 180);
	PZ4Text(px + 12, py + ph - 40, "A > B  means  A overpowers B", GLUT_BITMAP_HELVETICA_12);

	for (k = 0; k < 5; k++) {
		float ang = (90.0f - 72.0f * k) * PZ4_PI / 180.0f;
		nx[k] = cx + R * (float)cos(ang);
		ny[k] = cy + R * (float)sin(ang);
	}

	// arrows k -> k+1
	for (k = 0; k < 5; k++) {
		int j = (k + 1) % 5;
		float dx = nx[j] - nx[k], dy = ny[j] - ny[k];
		float len = (float)sqrt(dx * dx + dy * dy);
		float ux = dx / len, uy = dy / len;
		float sx = nx[k] + ux * 19.0f, sy = ny[k] + uy * 19.0f;
		float ex = nx[j] - ux * 19.0f, ey = ny[j] - uy * 19.0f;
		double ax[3], ay[3];
		iSetColor(120, 140, 155);
		iLine(sx, sy, ex, ey);
		ax[0] = ex;                          ay[0] = ey;
		ax[1] = ex - ux * 9.0f - uy * 5.0f;  ay[1] = ey - uy * 9.0f + ux * 5.0f;
		ax[2] = ex - ux * 9.0f + uy * 5.0f;  ay[2] = ey - uy * 9.0f - ux * 5.0f;
		iFilledPolygon(ax, ay, 3);
	}

	// nodes
	for (k = 0; k < 5; k++) {
		char lbl[24];
		iSetColor(10, 20, 28);
		iFilledCircle(nx[k], ny[k], 18);
		PZ4SetNatColor(k, 100);
		iCircle(nx[k], ny[k], 18);
		PZ4DrawEmblem(k, nx[k], ny[k], 10.0f, 100);

		sprintf(lbl, "%s %d", PZ4_NAME[k], k);
		iSetColor(200, 215, 225);
		{
			int lx = (int)nx[k] - PZ4TextW(lbl, GLUT_BITMAP_HELVETICA_12) / 2;
			int ly = (k == 0) ? (int)ny[k] + 24 : (int)ny[k] - 32;
			if (k == 1) { lx = (int)nx[k] - 12; ly = (int)ny[k] + 22; }
			if (k == 4) { lx = (int)nx[k] - 34; ly = (int)ny[k] + 22; }
			PZ4Text(lx, ly, lbl, GLUT_BITMAP_HELVETICA_12);
		}
	}
}

// The shinobi and a chakra aura. Uses the existing Naruto sprite frame.
static void PZ4DrawShinobi(void) {
	const int px = 925, py = 30, pw = 250, ph = 255;
	float pulse = 0.5f + 0.5f * (float)sin(pz4Tick * 0.07f);
	int r = (int)(74 + 10 * pulse);

	PZ4DrawPanel(px, py, pw, ph, 0, 120, 140);

	iSetColor(30 + (int)(30 * pulse), 60 + (int)(40 * pulse), 70 + (int)(50 * pulse));
	iCircle(px + pw * 0.5f, py + 118, r);
	iCircle(px + pw * 0.5f, py + 118, r + 10);

	if (pz4NarutoImg > 0) iShowImage(px + 60, py + 30, 130, 195, pz4NarutoImg);

	if (pz4Burst > 0 && pz4BurstImg > 0) {
		int size = 40 + (46 - pz4Burst) * 4;
		iShowImage(px + pw / 2 - size / 2, py + 118 - size / 2, size, size, pz4BurstImg);
	}

	iSetColor(150, 200, 215);
	PZ4TextCentered(px + pw / 2, py + 10, "the seal tests the shinobi", GLUT_BITMAP_HELVETICA_12);
}

static void PZ4DrawBreadcrumb(void) {
	static const char* const labels[5] = { "1 WITNESSES", "2 CHAIN", "3 ECHO", "4 LOGIC LOCK", "5 FINAL SEAL" };
	int i;
	int x = 40;
	for (i = 0; i < 5; i++) {
		bool done = (pz4Stage > i + 1);
		bool here = (pz4Stage == i + 1);
		char buf[40];

		if (done)      iSetColor(0, 200, 140);
		else if (here) iSetColor(255, 220, 120);
		else           iSetColor(90, 100, 110);

		sprintf(buf, "%s%s", labels[i], done ? "  [OK]" : "");
		PZ4Text(x, SCREEN_HEIGHT - 84, buf, GLUT_BITMAP_HELVETICA_12);
		x += 176;
	}
}

static void PZ4NatNameList(char* out, const int* list, int n, const char* sep) {
	int i, pos = 0;
	out[0] = '\0';
	for (i = 0; i < n; i++) {
		pos += sprintf(out + pos, "%s%s", PZ4_NAME[list[i]], (i + 1 < n) ? sep : "");
	}
}

// ---------------------------------------------------------------------
// render
// ---------------------------------------------------------------------
void RenderPuzzle4(void) {
	char buf[260];
	int i;

	// Its own visual space - nothing of the gameplay scene is drawn behind it.
	iSetColor(4, 9, 14);
	iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	iSetColor(0, 44, 54);
	for (i = 0; i < SCREEN_HEIGHT; i += 40) iLine(0, i, SCREEN_WIDTH, i);

	// slowly turning seal in the background
	{
		float cx = 470.0f, cy = 310.0f, rr = 250.0f;
		float a0 = pz4Tick * 0.004f;
		iSetColor(0, 38, 48);
		iCircle(cx, cy, rr);
		iCircle(cx, cy, rr - 14);
		for (i = 0; i < 5; i++) {
			float aa = a0 + i * 2.0f * PZ4_PI / 5.0f;
			float bb = a0 + ((i + 2) % 5) * 2.0f * PZ4_PI / 5.0f;
			iLine(cx + rr * (float)cos(aa), cy + rr * (float)sin(aa), cx + rr * (float)cos(bb), cy + rr * (float)sin(bb));
		}
	}

	// ---- header ----
	iSetColor(0, 230, 210);
	PZ4Text(40, SCREEN_HEIGHT - 48, "THE NINE-TAILS SEAL  -  FINAL LOCK", GLUT_BITMAP_TIMES_ROMAN_24);

	sprintf(buf, "ATTEMPTS REMAINING: %d / %d", pz4Attempts, PZ4_MAX_ATTEMPTS);
	if (pz4Attempts <= 1) iSetColor(255, 80, 70);
	else                  iSetColor(220, 230, 235);
	PZ4Text(SCREEN_WIDTH - 320, SCREEN_HEIGHT - 48, buf, GLUT_BITMAP_HELVETICA_18);

	PZ4DrawBreadcrumb();

	// ---- feedback line ----
	if (pz4Msg[0] != '\0') {
		if (pz4Flash > 0) iSetColor(pz4FlashGood ? 0 : 255, pz4FlashGood ? 255 : 90, pz4FlashGood ? 170 : 80);
		else              iSetColor(150, 165, 175);
		PZ4Text(40, SCREEN_HEIGHT - 112, pz4Msg, GLUT_BITMAP_HELVETICA_18);
	}

	PZ4DrawCycleRef();
	PZ4DrawShinobi();

	// =================== STAGE 1 : THE WITNESSES ===================
	if (pz4Stage == 1) {
		PZ4DrawPanel(40, 372, 860, 148, 0, 170, 190);

		iSetColor(255, 225, 140);
		PZ4Text(58, 496, "STAGE 1  -  THE THREE WITNESSES", GLUT_BITMAP_HELVETICA_18);
		iSetColor(220, 235, 240);
		PZ4Text(58, 474, "One of these plates is the WARDEN. Exactly ONE witness is lying; the other two tell the truth.", GLUT_BITMAP_HELVETICA_12);

		for (i = 0; i < 3; i++) {
			char t[200];
			PZ4StmtText(pz4Stmt[i], t);
			sprintf(buf, "WITNESS %d:  %s", i + 1, t);
			iSetColor(190, 235, 255);
			PZ4Text(58, 446 - i * 26, buf, GLUT_BITMAP_HELVETICA_18);
		}

		for (i = 0; i < 5; i++) PZ4DrawPlate(i, false, 0);

		iSetColor(0, 220, 200);
		PZ4TextCentered(470, 160, "Orbs = chakra held.   Frame colours are decoration.   Click the WARDEN plate.", GLUT_BITMAP_HELVETICA_12);
		return;
	}

	// =================== STAGE 2 : THE CHAIN ===================
	if (pz4Stage == 2) {
		PZ4DrawPanel(40, 372, 860, 148, 0, 170, 190);

		iSetColor(255, 225, 140);
		PZ4Text(58, 496, "STAGE 2  -  THE CHAIN OF CONSUMPTION", GLUT_BITMAP_HELVETICA_18);

		sprintf(buf, "WARDEN CONFIRMED:  %s   (chakra %d)", PZ4_NAME[PZ4WardenNat()], pz4PlateLvl[pz4Warden]);
		iSetColor(0, 230, 170);
		PZ4Text(58, 472, buf, GLUT_BITMAP_HELVETICA_12);

		iSetColor(220, 235, 240);
		PZ4Text(58, 450, "Seal the plates in the order the WARDEN's chakra would consume them: the WARDEN first,", GLUT_BITMAP_HELVETICA_12);
		PZ4Text(58, 432, "then the plate it OVERPOWERS, then the plate THAT plate overpowers, and so on around the cycle.", GLUT_BITMAP_HELVETICA_12);
		PZ4Text(58, 414, "A plate holding only ONE chakra orb is DRAINED - never touch it. A wrong touch costs an attempt.", GLUT_BITMAP_HELVETICA_12);
		PZ4Text(58, 390, "(Use the NATURE CYCLE on the right. The order is by nature, not by where a plate sits.)", GLUT_BITMAP_HELVETICA_12);

		for (i = 0; i < 5; i++) {
			int badge = 0, k;
			for (k = 0; k < pz4Step; k++) if (pz4Chain[k] == i) badge = k + 1;
			PZ4DrawPlate(i, pz4Picked[i], badge);
		}

		sprintf(buf, "SEALED  %d / %d", pz4Step, pz4ChainLen);
		iSetColor(0, 220, 200);
		PZ4TextCentered(470, 160, buf, GLUT_BITMAP_HELVETICA_18);
		return;
	}

	// =================== STAGE 3 : ECHO ===================
	if (pz4Stage == 3) {
		int chainNat[4];
		int k;
		bool even = (pz4Dir == +1);

		PZ4DrawPanel(40, 372, 860, 148, 0, 170, 190);

		iSetColor(255, 225, 140);
		PZ4Text(58, 496, "STAGE 3  -  ECHO OF THE TAILS", GLUT_BITMAP_HELVETICA_18);

		for (k = 0; k < pz4ChainLen; k++) chainNat[k] = pz4PlateNat[pz4Chain[k]];
		{
			char list[120];
			PZ4NatNameList(list, chainNat, pz4ChainLen, "  ->  ");
			sprintf(buf, "CHAIN SEALED:  %s", list);
		}
		iSetColor(0, 230, 170);
		PZ4Text(58, 472, buf, GLUT_BITMAP_HELVETICA_12);

		sprintf(buf, "LAST PLATE SEALED:  %s   (chakra %d - %s)",
			PZ4_NAME[PZ4LastChainNat()], pz4PlateLvl[PZ4LastChainPlate()], even ? "EVEN" : "ODD");
		iSetColor(255, 214, 110);
		PZ4Text(58, 452, buf, GLUT_BITMAP_HELVETICA_12);

		iSetColor(220, 235, 240);
		if (even) PZ4Text(58, 432, "EVEN -> answer every flash with the nature it OVERPOWERS  (one step ALONG the arrows).", GLUT_BITMAP_HELVETICA_12);
		else      PZ4Text(58, 432, "ODD -> answer every flash with the nature that OVERPOWERS it  (one step AGAINST the arrows).", GLUT_BITMAP_HELVETICA_12);
		PZ4Text(58, 412, "Six natures flash, one at a time. Press your ANSWERS in the same order. Do not press the flashes themselves.", GLUT_BITMAP_HELVETICA_12);
		PZ4Text(58, 392, "The first viewing is free, then two replays. A replay restarts your answer; a wrong press costs an attempt.", GLUT_BITMAP_HELVETICA_12);

		// the five nature buttons; the one currently flashing is lit
		{
			int litNat = -1;
			if (pz4ShowActive) {
				int idx = pz4ShowTick / PZ4_SLOT_TICKS;
				int ph  = pz4ShowTick % PZ4_SLOT_TICKS;
				if (idx < PZ4_SEQ_LEN && ph < PZ4_LIT_TICKS) litNat = pz4Flashes[idx];
			}
			for (i = 0; i < 5; i++) {
				const PZ4Rect& rc = pz4NatBtn[i];
				bool lit = (litNat == i);

				if (lit) { iSetColor(PZ4_COL[i][0] / 3, PZ4_COL[i][1] / 3, PZ4_COL[i][2] / 3); }
				else     { iSetColor(20, 28, 38); }
				iFilledRectangle(rc.x, rc.y, rc.w, rc.h);

				if (lit) { PZ4SetNatColor(i, 100); }
				else     { iSetColor(0, 150, 170); }
				iRectangle(rc.x, rc.y, rc.w, rc.h);
				iRectangle(rc.x + 4, rc.y + 4, rc.w - 8, rc.h - 8);
				if (lit) iRectangle(rc.x - 3, rc.y - 3, rc.w + 6, rc.h + 6);

				PZ4DrawEmblem(i, rc.x + rc.w * 0.5f, rc.y + 88, 34.0f, lit ? 100 : 62);
				iSetColor(lit ? 255 : 200, lit ? 255 : 215, lit ? 255 : 225);
				PZ4TextCentered(rc.x + rc.w / 2, rc.y + 28, PZ4_NAME[i], GLUT_BITMAP_HELVETICA_12);
			}
		}

		// show / replay button
		{
			char lbl[40];
			bool enabled = true;
			if (pz4ShowActive)          { sprintf(lbl, "SHOWING ..."); enabled = false; }
			else if (!pz4Shown)         { sprintf(lbl, "SHOW THE SEQUENCE"); }
			else if (pz4Replays > 0)    { sprintf(lbl, "REPLAY  (%d left)", pz4Replays); }
			else                        { sprintf(lbl, "NO REPLAYS LEFT"); enabled = false; }
			PZ4DrawButton(pz4ShowBtn, lbl, enabled);
		}

		// progress
		sprintf(buf, "ANSWERS ENTERED  %d / %d", pz4Step, PZ4_SEQ_LEN);
		iSetColor(0, 220, 200);
		PZ4TextCentered(470, 96, buf, GLUT_BITMAP_HELVETICA_18);
		for (i = 0; i < PZ4_SEQ_LEN; i++) {
			int bx = 470 - (PZ4_SEQ_LEN * 30) / 2 + i * 30;
			if (i < pz4Step) { iSetColor(0, 230, 170); iFilledRectangle(bx, 56, 22, 22); }
			else             { iSetColor(70, 90, 100); iRectangle(bx, 56, 22, 22); }
		}
		return;
	}

	// =================== STAGE 4 : LOGIC LOCK ===================
	if (pz4Stage == 4) {
		int chainNat[4];
		int k;

		PZ4DrawPanel(40, 338, 860, 182, 0, 170, 190);

		iSetColor(255, 225, 140);
		PZ4Text(58, 496, "STAGE 4  -  THE LOGIC LOCK", GLUT_BITMAP_HELVETICA_18);
		iSetColor(220, 235, 240);
		PZ4Text(58, 474, "Click a dial to turn it to the next nature. Exactly ONE setting obeys every rule. Then press TURN.", GLUT_BITMAP_HELVETICA_12);

		for (i = 0; i < pz4ClueCount; i++) {
			char t[200];
			PZ4ClueText(pz4Clue[i], t);
			sprintf(buf, "RULE %d:  %s", i + 1, t);
			iSetColor(190, 235, 255);
			PZ4Text(58, 450 - i * 19, buf, GLUT_BITMAP_HELVETICA_12);
		}

		// dials
		for (i = 0; i < 3; i++) {
			const PZ4Rect& rc = pz4DialRc[i];
			int nat = pz4Dial[i];
			iSetColor(20, 28, 38);
			iFilledRectangle(rc.x, rc.y, rc.w, rc.h);
			PZ4SetNatColor(nat, 100);
			iRectangle(rc.x, rc.y, rc.w, rc.h);
			iRectangle(rc.x + 4, rc.y + 4, rc.w - 8, rc.h - 8);
			PZ4DrawEmblem(nat, rc.x + rc.w * 0.5f, rc.y + 72, 30.0f, 100);
			iSetColor(225, 238, 245);
			PZ4TextCentered(rc.x + rc.w / 2, rc.y + 22, PZ4_NAME[nat], GLUT_BITMAP_HELVETICA_12);
			sprintf(buf, "DIAL %c", 'A' + i);
			iSetColor(255, 225, 140);
			PZ4Text(rc.x + 4, rc.y + rc.h + 8, buf, GLUT_BITMAP_HELVETICA_12);
		}
		PZ4DrawButton(pz4ConfirmBtn, "TURN", true);

		// what the seal has recorded so far
		PZ4DrawPanel(40, 30, 860, 150, 0, 120, 140);
		iSetColor(255, 225, 140);
		PZ4Text(58, 156, "RECORDED BY THE SEAL", GLUT_BITMAP_HELVETICA_12);

		sprintf(buf, "WARDEN:  %s  (chakra %d)", PZ4_NAME[PZ4WardenNat()], pz4PlateLvl[pz4Warden]);
		iSetColor(0, 230, 170);
		PZ4Text(58, 132, buf, GLUT_BITMAP_HELVETICA_12);

		for (k = 0; k < pz4ChainLen; k++) chainNat[k] = pz4PlateNat[pz4Chain[k]];
		{
			char list[120];
			PZ4NatNameList(list, chainNat, pz4ChainLen, "  ->  ");
			sprintf(buf, "CHAIN (last plate sealed = %s):  %s", PZ4_NAME[PZ4LastChainNat()], list);
		}
		iSetColor(0, 230, 170);
		PZ4Text(58, 108, buf, GLUT_BITMAP_HELVETICA_12);

		{
			char list[160];
			PZ4NatNameList(list, pz4Answer, PZ4_SEQ_LEN, ", ");
			sprintf(buf, "ECHO ANSWER:  %s", list);
		}
		iSetColor(200, 160, 255);
		PZ4Text(58, 84, buf, GLUT_BITMAP_HELVETICA_12);

		iSetColor(150, 165, 175);
		PZ4Text(58, 56, "Rules only ever talk about the results recorded above and about the cycle on the right.", GLUT_BITMAP_HELVETICA_12);
		return;
	}

	// =================== STAGE 5 : FINAL SEAL ===================
	if (pz4Stage >= 5) {
		int chainNat[4];
		int k;

		PZ4DrawPanel(40, 400, 860, 120, 0, 170, 190);
		iSetColor(255, 225, 140);
		PZ4Text(58, 496, "STAGE 5  -  THE FINAL SEAL", GLUT_BITMAP_HELVETICA_18);
		iSetColor(220, 235, 240);
		PZ4Text(58, 472, "DIGIT 1  =  the WARDEN's chakra level", GLUT_BITMAP_HELVETICA_12);
		PZ4Text(58, 452, "DIGIT 2  =  the nature numbers of DIAL A + DIAL B + DIAL C added together (last digit only)", GLUT_BITMAP_HELVETICA_12);
		PZ4Text(58, 432, "DIGIT 3  =  how many DIFFERENT natures appear in the ECHO ANSWER", GLUT_BITMAP_HELVETICA_12);
		PZ4Text(58, 412, "DIGIT 4  =  (number of the FIRST ECHO ANSWER + number of the LAST plate in the CHAIN), last digit only", GLUT_BITMAP_HELVETICA_12);

		PZ4DrawPanel(40, 296, 860, 96, 0, 120, 140);
		iSetColor(255, 225, 140);
		PZ4Text(58, 372, "RECORDED BY THE SEAL   (nature numbers: FIRE 0, WIND 1, LIGHTNING 2, EARTH 3, WATER 4)", GLUT_BITMAP_HELVETICA_12);

		sprintf(buf, "WARDEN:  %s  (chakra %d)      LOCK (dial A, B, C):  %s, %s, %s",
			PZ4_NAME[PZ4WardenNat()], pz4PlateLvl[pz4Warden],
			PZ4_NAME[pz4LockSol[0]], PZ4_NAME[pz4LockSol[1]], PZ4_NAME[pz4LockSol[2]]);
		iSetColor(0, 230, 170);
		PZ4Text(58, 348, buf, GLUT_BITMAP_HELVETICA_12);

		for (k = 0; k < pz4ChainLen; k++) chainNat[k] = pz4PlateNat[pz4Chain[k]];
		{
			char list[120];
			PZ4NatNameList(list, chainNat, pz4ChainLen, "  ->  ");
			sprintf(buf, "CHAIN:  %s", list);
		}
		iSetColor(0, 230, 170);
		PZ4Text(58, 328, buf, GLUT_BITMAP_HELVETICA_12);

		{
			char list[160];
			PZ4NatNameList(list, pz4Answer, PZ4_SEQ_LEN, ", ");
			sprintf(buf, "ECHO ANSWER:  %s", list);
		}
		iSetColor(200, 160, 255);
		PZ4Text(58, 308, buf, GLUT_BITMAP_HELVETICA_12);

		// code slots
		for (i = 0; i < 4; i++) {
			const PZ4Rect& rc = pz4Slot[i];
			iSetColor(18, 26, 34);
			iFilledRectangle(rc.x, rc.y, rc.w, rc.h);
			iSetColor(0, 200, 220);
			iRectangle(rc.x, rc.y, rc.w, rc.h);
			iRectangle(rc.x + 5, rc.y + 5, rc.w - 10, rc.h - 10);

			if (pz4Entry[i] >= 0) {
				char d[4];
				sprintf(d, "%d", pz4Entry[i]);
				iSetColor(255, 240, 180);
				PZ4TextCentered(rc.x + rc.w / 2, rc.y + rc.h / 2 - 10, d, GLUT_BITMAP_TIMES_ROMAN_24);
			}
			else {
				iSetColor(70, 90, 100);
				iLine(rc.x + 22, rc.y + 24, rc.x + rc.w - 22, rc.y + 24);
			}
			sprintf(buf, "D%d", i + 1);
			iSetColor(150, 165, 175);
			PZ4Text(rc.x + 4, rc.y + rc.h + 6, buf, GLUT_BITMAP_HELVETICA_12);
		}

		for (i = 0; i < 10; i++) {
			char lbl[4];
			sprintf(lbl, "%d", i);
			PZ4DrawButton(pz4Digit[i], lbl, true);
		}
		PZ4DrawButton(pz4ClearBtn, "CLEAR", true);
		PZ4DrawButton(pz4EnterBtn, "ENTER", true);
		return;
	}
}
