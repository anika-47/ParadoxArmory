// =====================================================================
// Unit test for LEVEL 4's two new final-area hazards and the seal altar.
// =====================================================================
// Build like the other tests (see README.md):
//   g++ -std=c++03 -fpermissive -w -I. -o test_seal4 test_seal4.cpp iGraphicsGlobals.cpp \
//       player.cpp gameState.cpp variable.cpp enemy.cpp obstacle.cpp puzzle.cpp
//
// Proves: the hazards sleep until the Warden falls; are harmless during
// their grace period; hurt only through real overlap (pylon: only while
// ACTIVE); go through the invulnerability window (no per-frame drain);
// stay inside the arena; animate continuously; and that the altar - not
// the Warden's death - starts the puzzle.
// =====================================================================
#include "level4.cpp"
#include "puzzle4.cpp"
#include <stdio.h>

static int failures = 0;
static void check(bool c, const char* what) { printf("  [%s] %s\n", c ? " ok " : "FAIL", what); if (!c) failures++; }

static void placePlayerAtWorld(float wx, int y) {
    l4CamX = 8400.0f;
    playerX = (int)(wx - l4CamX);
    playerY = y;
    isCrouching = false;
}
static L4Obstacle* find(int type, int nth) {
    int n = 0;
    for (int i = 0; i < l4ObstacleCount; i++)
        if (l4Obstacles[i].type == type) { if (n == nth) return &l4Obstacles[i]; n++; }
    return 0;
}
static void freshLevel() {
    srand(1); InitLevel4(); gameState = STATE_PLAY;
    playerHealth = playerMaxHealth;
}

int main() {
    printf("=============== SEAL APPROACH TEST ===============\n");

    // ---- counts / asleep --------------------------------------------------
    freshLevel();
    int pylons = 0, wisps = 0;
    for (int i = 0; i < l4ObstacleCount; i++) { if (l4Obstacles[i].type == L4_OB_PYLON) pylons++; if (l4Obstacles[i].type == L4_OB_WISP) wisps++; }
    check(pylons == 2 && wisps == 1, "two pylons and one wisp are built (two NEW obstacle types)");
    check(!l4SealAwake, "the approach starts asleep");

    L4Obstacle* w = find(L4_OB_WISP, 0);
    float wx0 = w->x, wy0 = w->y;
    L4Obstacle* p1 = find(L4_OB_PYLON, 0);
    p1->timer = 170;                                    // would be ACTIVE if awake
    placePlayerAtWorld(p1->x - 30.0f, GROUND_Y);
    int hp0 = playerHealth;
    for (int i = 0; i < 400; i++) { L4UpdateSealApproach(); L4CheckHazards(); }
    check(playerHealth == hp0, "asleep: standing in an 'active' pylon column costs nothing");
    check(w->x == wx0 && w->y == wy0 && p1->timer == 170, "asleep: nothing animates or moves");

    // ---- grace period -----------------------------------------------------
    l4SealAwake = true; l4SealTimer = 0;
    p1->timer = 170;
    l4PlayerInvuln = 0;
    for (int i = 0; i < 90; i++) { L4UpdateSealApproach(); if (l4PlayerInvuln > 0) l4PlayerInvuln--; }
    check(playerHealth == hp0, "grace: hazards are visible but harmless for the first 100 ticks");

    // ---- pylon: only while ACTIVE, once per invulnerability window --------
    freshLevel(); l4SealAwake = true; l4SealTimer = L4_SEAL_GRACE;
    p1 = find(L4_OB_PYLON, 0);
    placePlayerAtWorld(p1->x - 30.0f, GROUND_Y);
    int states[4] = { 30, 130, 240 - 5, 0 };            // dormant, charging, cooling(-ish), dormant
    hp0 = playerHealth;
    bool harmlessOutsideActive = true;
    for (int s = 0; s < 4; s++) {
        p1->timer = states[s];
        int before = playerHealth;
        // step only ONE tick so the state cannot roll into ACTIVE
        L4UpdateSealApproach();
        if (states[s] != 240 - 5 || true) if (playerHealth != before && !(p1->timer >= L4_PYLON_ACTIVE_AT && p1->timer < L4_PYLON_COOL_AT)) harmlessOutsideActive = false;
    }
    check(harmlessOutsideActive, "pylon: dormant / charging / cooling never hurt");
    p1->timer = L4_PYLON_ACTIVE_AT + 5;
    l4PlayerInvuln = 0;
    int before = playerHealth;
    L4UpdateSealApproach();
    check(playerHealth == before - L4_DMG_PYLON, "pylon: ACTIVE barrier hurts by exactly its damage value");
    int afterFirst = playerHealth;
    for (int i = 0; i < L4_IFRAMES - 5; i++) { L4UpdateSealApproach(); if (l4PlayerInvuln > 0) l4PlayerInvuln--; }
    check(playerHealth == afterFirst, "pylon: no repeat damage inside the invulnerability window (no per-frame drain)");

    // player next to (not inside) the column is never hit
    freshLevel(); l4SealAwake = true; l4SealTimer = L4_SEAL_GRACE;
    p1 = find(L4_OB_PYLON, 0);
    p1->timer = L4_PYLON_ACTIVE_AT + 5;
    placePlayerAtWorld(p1->x - playerWidth - 20.0f, GROUND_Y);
    before = playerHealth;
    L4UpdateSealApproach();
    check(playerHealth == before, "pylon: standing beside the barrier (not overlapping) is safe");

    // ---- wisp -------------------------------------------------------------
    freshLevel(); l4SealAwake = true; l4SealTimer = L4_SEAL_GRACE;
    w = find(L4_OB_WISP, 0);
    float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f, minStep = 1e9f, maxStep = 0.0f, prevPhase = w->phase;
    bool moved = true; float lx = w->x, ly = w->y;
    placePlayerAtWorld(8000.0f, GROUND_Y);              // far away: no interference
    for (int i = 0; i < 3000; i++) {
        L4UpdateSealApproach();
        float cx = w->x + w->w * 0.5f, cy = w->y + w->h * 0.5f;
        if (cx < minX) minX = cx; if (cx > maxX) maxX = cx;
        if (cy < minY) minY = cy; if (cy > maxY) maxY = cy;
        float st = w->phase - prevPhase; prevPhase = w->phase;
        if (st < minStep) minStep = st; if (st > maxStep) maxStep = st;
        if (w->x == lx && w->y == ly) moved = false;
        lx = w->x; ly = w->y;
    }
    check(moved, "wisp: moves on every tick (its animation never freezes)");
    check(minX >= 9225.0f - 100.0f - 1.0f && maxX <= 9225.0f + 100.0f + 1.0f, "wisp: stays inside its horizontal patrol lane");
    check(minY - w->h * 0.5f >= (float)GROUND_Y && maxY <= 195.0f + 55.0f + 1.0f, "wisp: stays above the ground and inside its vertical lane");
    check(maxStep / minStep > 2.0f, "wisp: speed genuinely varies along the path");
    check((maxY - minY) > 90.0f && (maxX - minX) > 190.0f, "wisp: covers a two-dimensional figure-eight (not a line)");

    // wisp hurts on overlap, inflicts its damage once per window
    l4PlayerInvuln = 0; playerHealth = playerMaxHealth;
    placePlayerAtWorld(w->x + w->w * 0.5f - playerWidth * 0.5f, (int)(w->y + w->h * 0.5f) - 20);
    before = playerHealth;
    L4UpdateSealApproach();
    check(playerHealth == before - L4_DMG_WISP, "wisp: contact hurts by exactly its damage value");
    // drawn orb radius is 22; hit box is inset: a player just clipping the orb's edge is missed
    l4PlayerInvuln = 0; playerHealth = playerMaxHealth;
    L4UpdateSealApproach();
    float ox = w->x, oy = w->y;
    placePlayerAtWorld(ox + w->w - 3.0f, (int)oy + 2);   // overlaps only the outer 3px of the orb's box
    l4PlayerInvuln = 0;
    before = playerHealth;
    // freeze the wisp so the geometry under test is exact
    float keepSpeed = w->speed; w->speed = 0.0f;
    placePlayerAtWorld(w->x + w->w - 3.0f, (int)w->y - playerHeight + 3);
    L4UpdateSealApproach();
    check(playerHealth == before, "wisp: hit box is smaller than the drawn orb (no invisible hit boxes)");
    w->speed = keepSpeed;

    // ---- altar -----------------------------------------------------------
    freshLevel();
    placePlayerAtWorld(L4_ALTAR_TRIGGER_X + 5.0f, GROUND_Y);
    L4UpdateSealApproach();
    check(gameState == STATE_PLAY && !l4PuzzleStarted, "altar: does nothing while the Warden is alive");
    l4SealAwake = true; l4SealTimer = 0;
    placePlayerAtWorld(L4_ALTAR_TRIGGER_X - 20.0f, GROUND_Y);
    L4UpdateSealApproach();
    check(gameState == STATE_PLAY, "altar: short of the trigger line the puzzle does not open");
    placePlayerAtWorld(L4_ALTAR_TRIGGER_X, GROUND_Y);
    L4UpdateSealApproach();
    check(gameState == STATE_PUZZLE4 && l4PuzzleStarted, "altar: reaching it opens the final puzzle");

    // ---- boss defeat wakes the approach but does not start the puzzle -----
    freshLevel();
    l4Boss.defeated = true;
    L4UpdateProgression();
    check(l4SealAwake && gameState == STATE_PLAY, "Warden defeated: the approach wakes, the puzzle waits for the altar");

    // ---- render every state without incident ------------------------------
    freshLevel(); l4SealAwake = true;
    for (int i = 0; i < 700; i++) {
        l4SealTimer = i;
        L4UpdateSealApproach();
        l4CamX = (float)(8000 + (i % 400));
        RenderLevel4();
    }
    check(true, "700 frames of the awake approach render without incident");

    printf("\n%d failure%s\n", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
