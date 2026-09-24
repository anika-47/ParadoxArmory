// =====================================================================
// Unit test for LEVEL 4's final puzzle, "THE NINE-TAILS SEAL".
// =====================================================================
// Includes puzzle4.cpp directly so the test can read the generated
// state and click the real hit rectangles through the real
// HandlePuzzle4Click() entry point.
//
// What it proves, over thousands of seeds:
//   1. STAGE 1 is uniquely solvable: the testimonies are re-read from
//      the PRINTED TEXT by an independent parser and enumerated against
//      all 5 plates x 3 possible liars - exactly one pair survives and it
//      is the plate the game expects.
//   2. STAGE 4 is uniquely solvable: the printed lock rules are re-read
//      from their TEXT and enumerated against all 125 dial settings using
//      only the values the screen shows - exactly one survives.
//   3. STAGES 2, 3 and 5 follow their printed rules (re-derived here
//      independently from the plate data / flash list).
//   4. The intended path wins and reaches STATE_LEVEL_WIN, costing no
//      attempts.
//   5. A wrong input costs exactly one attempt and resets ONLY the
//      current stage; running out of attempts hands over to
//      STATE_GAME_OVER; nothing crashes; rendering is safe everywhere.
// =====================================================================
#include "level4.cpp"
#include "puzzle4.cpp"
#include <stdio.h>
#include <string.h>

static int failures = 0;
static int checks   = 0;

static void check(bool cond, const char* what) {
    checks++;
    if (!cond) { printf("  [FAIL] %s\n", what); failures++; }
}
static void say(bool cond, const char* what) {
    printf("  [%s] %s\n", cond ? " ok " : "FAIL", what);
    checks++;
    if (!cond) failures++;
}

static void clickRect(const PZ4Rect& r) { HandlePuzzle4Click(r.x + r.w / 2, r.y + r.h / 2); }

// ---------------------------------------------------------------------
// independent reading of the printed sentences
// ---------------------------------------------------------------------
static int natOf(const char* text, const char* after) {
    const char* p = strstr(text, after);
    if (!p) return -1;
    p += strlen(after);
    for (int n = 0; n < 5; n++) {
        if (strncmp(p, PZ4_NAME[n], strlen(PZ4_NAME[n])) == 0) return n;
    }
    return -1;
}

// Truth of a printed testimony if plate p were the WARDEN.
static bool sentenceTrue(const char* t, int p, const int* nat, const int* lvl, const int* posOf, bool* recognised) {
    *recognised = true;
    int n;
    if ((n = natOf(t, "overpowers the ")) >= 0 && strstr(t, "is overpowered") == 0)
        return ((nat[p] + 1) % 5) == n;
    if ((n = natOf(t, "is overpowered by the ")) >= 0)
        return ((n + 1) % 5) == nat[p];
    if ((n = natOf(t, "LEFT of the ")) >= 0)   return p < posOf[n];
    if ((n = natOf(t, "RIGHT of the ")) >= 0)  return p > posOf[n];
    if ((n = natOf(t, "MORE chakra than the ")) >= 0)  return lvl[p] > lvl[posOf[n]];
    if ((n = natOf(t, "FEWER chakra than the ")) >= 0) return lvl[p] < lvl[posOf[n]];
    if (strstr(t, "level is EVEN")) return lvl[p] % 2 == 0;
    if (strstr(t, "level is ODD"))  return lvl[p] % 2 == 1;
    if ((n = natOf(t, "is NOT the ")) >= 0)    return nat[p] != n;
    *recognised = false;
    return false;
}

static bool verifyStage1() {
    char text[3][200];
    for (int i = 0; i < 3; i++) PZ4StmtText(pz4Stmt[i], text[i]);

    int count = 0, foundPlate = -1, foundLiar = -1;
    for (int p = 0; p < 5; p++)
        for (int liar = 0; liar < 3; liar++) {
            bool ok = true;
            for (int i = 0; i < 3; i++) {
                bool rec;
                bool t = sentenceTrue(text[i], p, pz4PlateNat, pz4PlateLvl, pz4PosOfNat, &rec);
                if (!rec) return false;
                if (i == liar ? t : !t) ok = false;
            }
            if (ok) { count++; foundPlate = p; foundLiar = liar; }
        }
    return count == 1 && foundPlate == pz4Warden && foundLiar == pz4Liar;
}

// Truth of a printed lock rule for a dial setting, using ONLY what the
// screen shows: warden nature, last chain nature, the echo answer.
static bool ruleTrue(const char* t, const int* d, int w, int lastChain, const int* ans) {
    char X = 0, Y = 0;
    const char* p = strstr(t, "Dial ");
    X = p[5];
    int x = X - 'A';
    int cnt = 0;
    if (strstr(t, "OVERPOWERS the WARDEN's nature")) return d[x] == (w + 4) % 5;
    if (strstr(t, "the WARDEN's nature OVERPOWERS"))  return d[x] == (w + 1) % 5;
    if (strstr(t, "NEVER appears")) { for (int i = 0; i < 6; i++) if (ans[i] == d[x]) cnt++; return cnt == 0; }
    if (strstr(t, "MORE THAN ONCE")) { for (int i = 0; i < 6; i++) if (ans[i] == d[x]) cnt++; return cnt >= 2; }
    if (strstr(t, "OVERPOWERS Dial ")) { Y = strstr(t, "OVERPOWERS Dial ")[16]; return d[Y - 'A'] == (d[x] + 1) % 5; }
    if (strstr(t, "are DIFFERENT")) { Y = strstr(t, "and Dial ")[9]; return d[x] != d[Y - 'A']; }
    if (strstr(t, "LAST plate sealed")) return d[x] == lastChain;
    if (strstr(t, "FINAL entry")) return d[x] == ans[5];
    if (strstr(t, "own nature")) return d[x] == w;
    int k = 0;
    if (sscanf(strstr(t, "lies ") + 5, "%d", &k) == 1) return d[x] == (w + k) % 5;
    return false;
}

static bool verifyStage4() {
    int w = pz4PlateNat[pz4Warden];
    int last = pz4PlateNat[pz4Chain[3]];
    int count = 0, sol[3] = { -1, -1, -1 };
    char text[8][200];
    for (int i = 0; i < pz4ClueCount; i++) PZ4ClueText(pz4Clue[i], text[i]);
    for (int a = 0; a < 5; a++) for (int b = 0; b < 5; b++) for (int c = 0; c < 5; c++) {
        int d[3] = { a, b, c };
        bool ok = true;
        for (int i = 0; i < pz4ClueCount; i++)
            if (!ruleTrue(text[i], d, w, last, pz4Answer)) ok = false;
        if (ok) { count++; sol[0] = a; sol[1] = b; sol[2] = c; }
    }
    return count == 1 && sol[0] == pz4LockSol[0] && sol[1] == pz4LockSol[1] && sol[2] == pz4LockSol[2];
}

// Re-derive stages 2, 3, 5 from the printed rules.
static bool verifyDerived() {
    // stage 2: walk the cycle from the warden's nature, skip the level-1 plate
    int seq[5], n = 0;
    for (int k = 0; k < 5; k++) {
        int p = pz4PosOfNat[(pz4PlateNat[pz4Warden] + k) % 5];
        if (pz4PlateLvl[p] != 1) seq[n++] = p;
    }
    if (n != 4 || pz4ChainLen != 4) return false;
    for (int i = 0; i < 4; i++) if (seq[i] != pz4Chain[i]) return false;

    // stage 3: parity of the last sealed plate's chakra picks the direction
    int lastLvl = pz4PlateLvl[seq[3]];
    int dir = (lastLvl % 2 == 0) ? 1 : 4;              // +1 or -1 mod 5
    for (int i = 0; i < 6; i++) if (pz4Answer[i] != (pz4Flashes[i] + dir) % 5) return false;
    for (int i = 1; i < 6; i++) if (pz4Flashes[i] == pz4Flashes[i - 1]) return false;   // no immediate repeats
    int distinct = 0;
    for (int nn = 0; nn < 5; nn++) { bool s = false; for (int i = 0; i < 6; i++) if (pz4Flashes[i] == nn) s = true; if (s) distinct++; }
    if (distinct < 4) return false;

    // stage 5: the four printed digit rules
    int d1 = pz4PlateLvl[pz4Warden];
    int d2 = (pz4LockSol[0] + pz4LockSol[1] + pz4LockSol[2]) % 10;
    int ansDistinct = 0;
    for (int nn = 0; nn < 5; nn++) { bool s = false; for (int i = 0; i < 6; i++) if (pz4Answer[i] == nn) s = true; if (s) ansDistinct++; }
    int d4 = (pz4Answer[0] + pz4PlateNat[seq[3]]) % 10;
    return pz4Code[0] == d1 && pz4Code[1] == d2 && pz4Code[2] == ansDistinct && pz4Code[3] == d4;
}

static bool verifyPlates() {
    bool nat[5] = { false, false, false, false, false }, lvl[6] = { false, false, false, false, false, false };
    for (int i = 0; i < 5; i++) {
        if (pz4PlateNat[i] < 0 || pz4PlateNat[i] > 4 || nat[pz4PlateNat[i]]) return false;
        nat[pz4PlateNat[i]] = true;
        if (pz4PlateLvl[i] < 1 || pz4PlateLvl[i] > 5 || lvl[pz4PlateLvl[i]]) return false;
        lvl[pz4PlateLvl[i]] = true;
        if (pz4PosOfNat[pz4PlateNat[i]] != i) return false;
    }
    for (int i = 0; i < 4; i++) if (pz4Code[i] < 0 || pz4Code[i] > 9) return false;
    return true;
}

// ---------------------------------------------------------------------
// driving the real UI
// ---------------------------------------------------------------------
static void watchSequence() {
    clickRect(pz4ShowBtn);
    for (int i = 0; i < 6 * PZ4_SLOT_TICKS + 40; i++) UpdatePuzzle4(0.016f);
}

// Start a fresh puzzle and solve it cleanly up to (not including) the given stage.
static void freshAt(int seed, int stage) {
    srand(seed);
    gameState = STATE_PUZZLE4;
    puzzleSolved = false;
    InitPuzzle4();
    int i;
    if (stage > 1) clickRect(pz4Plate[pz4Warden]);
    if (stage > 2) for (i = 0; i < pz4ChainLen; i++) clickRect(pz4Plate[pz4Chain[i]]);
    if (stage > 3) { watchSequence(); for (i = 0; i < 6; i++) clickRect(pz4NatBtn[pz4Answer[i]]); }
    if (stage > 4) {
        for (i = 0; i < 3; i++) while (pz4Dial[i] != pz4LockSol[i]) clickRect(pz4DialRc[i]);
        clickRect(pz4ConfirmBtn);
    }
}

static void solveAll(bool renderEach) {
    int i;
    clickRect(pz4Plate[pz4Warden]);
    if (renderEach) RenderPuzzle4();
    for (i = 0; i < pz4ChainLen; i++) clickRect(pz4Plate[pz4Chain[i]]);
    if (renderEach) RenderPuzzle4();
    watchSequence();
    if (renderEach) RenderPuzzle4();
    for (i = 0; i < 6; i++) clickRect(pz4NatBtn[pz4Answer[i]]);
    if (renderEach) RenderPuzzle4();
    while (pz4Dial[0] != pz4LockSol[0]) clickRect(pz4DialRc[0]);
    while (pz4Dial[1] != pz4LockSol[1]) clickRect(pz4DialRc[1]);
    while (pz4Dial[2] != pz4LockSol[2]) clickRect(pz4DialRc[2]);
    if (renderEach) RenderPuzzle4();
    clickRect(pz4ConfirmBtn);
    if (renderEach) RenderPuzzle4();
    for (i = 0; i < 4; i++) clickRect(pz4Digit[pz4Code[i]]);
    clickRect(pz4EnterBtn);
}

int main() {
    printf("=============== PUZZLE 4 UNIT TEST (Nine-Tails Seal) ===============\n");

    int badStage1 = 0, badStage4 = 0, badDerived = 0, badPlates = 0, badWin = 0;
    int fallbackWitness = 0, fallbackLock = 0;
    int cluesMin = 99, cluesMax = 0;
    int dirPlus = 0, dirMinus = 0;
    int wardenLvlCount[6] = { 0, 0, 0, 0, 0, 0 };
    const int SEEDS = 20000;

    for (int seed = 1; seed <= SEEDS; seed++) {
        srand(seed);
        gameState = STATE_PUZZLE4;
        puzzleSolved = false;
        InitPuzzle4();

        if (!verifyPlates())  badPlates++;
        if (!verifyStage1())  badStage1++;
        if (!verifyStage4())  badStage4++;
        if (!verifyDerived()) badDerived++;

        if (pz4Stmt[0].type == 0 && pz4Stmt[1].type == 1 && pz4Stmt[2].type == 8 && pz4Liar == 2) fallbackWitness++;
        if (pz4Clue[0].type == 8) fallbackLock++;
        if (pz4ClueCount < cluesMin) cluesMin = pz4ClueCount;
        if (pz4ClueCount > cluesMax) cluesMax = pz4ClueCount;
        if (pz4Dir > 0) dirPlus++; else dirMinus++;
        wardenLvlCount[pz4PlateLvl[pz4Warden]]++;

        if (seed <= 3000) {
            solveAll(seed <= 20);
            if (!(gameState == STATE_LEVEL_WIN && puzzleSolved && pz4Attempts == PZ4_MAX_ATTEMPTS)) badWin++;
        }
    }

    printf("\n-- %d generated puzzles --\n", SEEDS);
    say(badPlates == 0,  "plates: natures and chakra levels are permutations, code digits are enterable");
    say(badStage1 == 0,  "stage 1: EXACTLY one (plate, liar) pair survives the printed testimonies, and it is the Warden");
    say(badStage4 == 0,  "stage 4: EXACTLY one of 125 dial settings survives the printed rules, and it is the lock");
    say(badDerived == 0, "stages 2/3/5: chain, echo answer and code match independently re-derived values");
    say(badWin == 0,     "intended path wins (3000 seeds): STATE_LEVEL_WIN, puzzleSolved, zero attempts spent");
    printf("  info: fallback witness sets used %d / %d, fallback lock sets used %d / %d\n", fallbackWitness, SEEDS, fallbackLock, SEEDS);
    printf("  info: lock rules per puzzle %d..%d; echo direction +1: %d  -1: %d\n", cluesMin, cluesMax, dirPlus, dirMinus);
    printf("  info: warden chakra level spread: 1:%d 2:%d 3:%d 4:%d 5:%d\n",
           wardenLvlCount[1], wardenLvlCount[2], wardenLvlCount[3], wardenLvlCount[4], wardenLvlCount[5]);
    say(dirPlus > 0 && dirMinus > 0, "both echo directions (EVEN / ODD) occur");

    // ---------- wrong input: one attempt, current stage resets only ----------
    printf("\n-- failure handling --\n");
    {
        int before;

        // ---- stage 1 ----
        freshAt(5, 1);
        clickRect(pz4Plate[(pz4Warden + 1) % 5]);
        say(pz4Attempts == PZ4_MAX_ATTEMPTS - 1 && pz4Stage == 1, "stage 1: wrong plate costs exactly one attempt, stays on stage 1");
        clickRect(pz4Plate[pz4Warden]);
        say(pz4Stage == 2, "stage 1 can still be solved after a failure");

        // ---- stage 2 ----
        freshAt(6, 2);
        int drained = -1; for (int i = 0; i < 5; i++) if (pz4PlateLvl[i] == 1) drained = i;
        clickRect(pz4Plate[pz4Chain[0]]);
        say(pz4Step == 1, "stage 2: first correct plate accepted");
        before = pz4Attempts;
        clickRect(pz4Plate[drained]);
        say(pz4Attempts == before - 1 && pz4Step == 0 && !pz4Picked[pz4Chain[0]], "stage 2: drained plate costs one attempt and resets ONLY stage 2");
        say(pz4Stage == 2, "stage 2: earlier stage stays solved");
        before = pz4Attempts;
        clickRect(pz4Plate[pz4Chain[1]]);
        say(pz4Attempts == before - 1 && pz4Step == 0, "stage 2: out-of-order plate costs one attempt");
        for (int i = 0; i < pz4ChainLen; i++) clickRect(pz4Plate[pz4Chain[i]]);
        say(pz4Stage == 3, "stage 2 solved after failures");

        // ---- stage 3 ----
        freshAt(7, 3);
        before = pz4Attempts;
        clickRect(pz4NatBtn[pz4Answer[0]]);
        say(pz4Attempts == before && pz4Step == 0, "stage 3: presses before viewing are ignored and cost nothing");
        watchSequence();
        before = pz4Attempts;
        clickRect(pz4NatBtn[pz4Flashes[0]]);      // the flash itself, not its answer: never correct
        say(pz4Attempts == before - 1 && pz4Step == 0 && !pz4Shown, "stage 3: pressing the flash instead of its answer costs one attempt and resets the stage");
        clickRect(pz4ShowBtn); for (int i = 0; i < 6 * PZ4_SLOT_TICKS + 40; i++) UpdatePuzzle4(0.016f);
        say(pz4Replays == 2, "stage 3: first viewing does not use a replay");
        clickRect(pz4ShowBtn); for (int i = 0; i < 6 * PZ4_SLOT_TICKS + 40; i++) UpdatePuzzle4(0.016f);
        clickRect(pz4ShowBtn); for (int i = 0; i < 6 * PZ4_SLOT_TICKS + 40; i++) UpdatePuzzle4(0.016f);
        say(pz4Replays == 0, "stage 3: two replays available");
        clickRect(pz4ShowBtn);
        say(!pz4ShowActive, "stage 3: no third replay");
        for (int i = 0; i < 6; i++) clickRect(pz4NatBtn[pz4Answer[i]]);
        say(pz4Stage == 4, "stage 3 solved after failure");

        // ---- stage 4 ----
        freshAt(8, 4);
        before = pz4Attempts;
        clickRect(pz4ConfirmBtn);                 // dials start on non-solutions
        say(pz4Attempts == before - 1 && pz4Stage == 4, "stage 4: wrong combination costs one attempt");
        bool startsOff = true;
        for (int i = 0; i < 3; i++) if (pz4Dial[i] == pz4LockSol[i]) startsOff = false;
        say(startsOff, "stage 4: dials never start on the answer");
        for (int i = 0; i < 3; i++) while (pz4Dial[i] != pz4LockSol[i]) clickRect(pz4DialRc[i]);
        clickRect(pz4ConfirmBtn);
        say(pz4Stage == 5, "stage 4 solved after failure");

        // ---- stage 5 ----
        freshAt(9, 5);
        before = pz4Attempts;
        clickRect(pz4EnterBtn);
        say(pz4Stage == 5 && pz4Attempts == before, "stage 5: ENTER with no digits costs nothing");
        for (int i = 0; i < 4; i++) clickRect(pz4Digit[(pz4Code[i] + 1) % 10]);
        clickRect(pz4EnterBtn);
        say(pz4Attempts == before - 1 && pz4EntryLen == 0 && pz4Stage == 5, "stage 5: wrong code costs one attempt and clears the slots");
        for (int i = 0; i < 4; i++) clickRect(pz4Digit[pz4Code[i]]);
        clickRect(pz4EnterBtn);
        say(gameState == STATE_LEVEL_WIN && puzzleSolved, "stage 5: correct code -> STATE_LEVEL_WIN");
    }

    // ---------- exhaustion ----------
    printf("\n-- attempt exhaustion --\n");
    {
        srand(9); gameState = STATE_PUZZLE4; puzzleSolved = false; InitPuzzle4();
        for (int i = 0; i < PZ4_MAX_ATTEMPTS + 2; i++) {
            if (gameState != STATE_PUZZLE4) break;
            clickRect(pz4Plate[(pz4Warden + 1) % 5]);
            RenderPuzzle4();
        }
        say(gameState == STATE_GAME_OVER, "running out of attempts hands over to STATE_GAME_OVER");
        say(!puzzleSolved, "puzzleSolved stays false");
        RenderPuzzle4();
        HandlePuzzle4Click(10, 10);
        say(true, "render and clicks after game over do not crash");
    }

    // ---------- every stage renders, incl. mid-demonstration ----------
    printf("\n-- rendering --\n");
    {
        srand(21); gameState = STATE_PUZZLE4; puzzleSolved = false; InitPuzzle4();
        for (int i = 0; i < 300; i++) UpdatePuzzle4(0.016f);
        RenderPuzzle4();                                 // stage 1
        clickRect(pz4Plate[pz4Warden]); RenderPuzzle4(); // stage 2
        clickRect(pz4Plate[pz4Chain[0]]); RenderPuzzle4();
        for (int i = 1; i < pz4ChainLen; i++) clickRect(pz4Plate[pz4Chain[i]]);
        clickRect(pz4ShowBtn);
        for (int i = 0; i < 6 * PZ4_SLOT_TICKS + 30; i++) { UpdatePuzzle4(0.016f); RenderPuzzle4(); }   // stage 3 incl. demo
        for (int i = 0; i < 6; i++) clickRect(pz4NatBtn[pz4Answer[i]]);
        RenderPuzzle4();                                 // stage 4
        for (int i = 0; i < 3; i++) while (pz4Dial[i] != pz4LockSol[i]) clickRect(pz4DialRc[i]);
        clickRect(pz4ConfirmBtn);
        for (int i = 0; i < 2; i++) clickRect(pz4Digit[3]);
        RenderPuzzle4();                                 // stage 5
        say(pz4Stage == 5, "all five stages render without incident");
    }

    printf("\n%d checks, %d failure%s\n", checks, failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
