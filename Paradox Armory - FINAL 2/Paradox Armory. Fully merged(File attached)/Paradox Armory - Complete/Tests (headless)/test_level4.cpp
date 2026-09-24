// =====================================================================
// Headless playtest harness for Level 4.
// =====================================================================
// Includes the level/puzzle sources directly so the test can inspect
// internal state (enemy list, obstacle list, boss) and drive the game
// exactly the way a human does: by holding A/D/W/S and tapping F.
//
// The bot never writes playerX/playerHealth/enemy state directly - it
// only pushes keys into the stub GetAsyncKeyState table, so whatever it
// achieves, a real player can achieve too.
// =====================================================================
#include "level4.cpp"
#include "puzzle4.cpp"
#include <stdio.h>

static void key(int k, bool down) { AsyncKeyTable()[k & 511] = down ? (short)0x8000 : 0; }

static float worldX() { return l4CamX + (float)playerX; }

#include "botlogic.inc"

// ---------------------------------------------------------------------
int main(int argc, char** argv) {
    bool godMode = (argc > 1 && argv[1][0] == 'g');
    bool verbose = (argc > 2);

    srand(12345);
    InitLevel4();
    gameState = STATE_PLAY;

    int frames = 0, deaths = 0, damageTaken = 0, lastHP = playerHealth;
    int sectionFirstFrame[10]; for (int i = 0; i < 10; i++) sectionFirstFrame[i] = -1;
    int gateFrame[10];         for (int i = 0; i < 10; i++) gateFrame[i] = -1;
    int bossSpawnFrame = -1, bossPhase2Frame = -1, puzzleFrame = -1;
    int maxEnemiesAtOnce = 0, totalKills = 0, prevAlive = 0;

    for (frames = 0; frames < 62 * 60 * 20; frames++) {     // 20-minute ceiling
        if (gameState != STATE_PLAY) break;

        botFrame();
        updatePlayer();
        UpdateLevel4();
        RenderLevel4();                 // exercises every draw path / sprintf

        if (playerHealth < lastHP) damageTaken += (lastHP - playerHealth);
        if (playerHealth <= 0) { deaths++; if (godMode) playerHealth = playerMaxHealth; }
        lastHP = playerHealth;

        int alive = 0;
        for (int i = 0; i < L4_MAX_ENEMIES; i++) if (l4Enemies[i].active) alive++;
        if (alive > maxEnemiesAtOnce) maxEnemiesAtOnce = alive;
        if (alive < prevAlive) totalKills += (prevAlive - alive);
        prevAlive = alive;

        if (sectionFirstFrame[l4Section] < 0) sectionFirstFrame[l4Section] = frames;
        for (int s = 1; s <= L4_NUM_SECTIONS; s++)
            if (l4SectionCleared[s] && gateFrame[s] < 0) gateFrame[s] = frames;
        if (l4Boss.active     && bossSpawnFrame  < 0) bossSpawnFrame  = frames;
        if (l4Boss.phase == 2 && bossPhase2Frame < 0) bossPhase2Frame = frames;
        if (gameState == STATE_PUZZLE4 && puzzleFrame < 0) puzzleFrame = frames;

        if (verbose && frames % 250 == 0) {
            printf("f=%5d %5.1fs wx=%6.0f sec=%d wave=%d alive=%d hp=%4d boss=%d/%d\n",
                   frames, frames / 62.5f, worldX(), l4Section, l4Wave, alive,
                   playerHealth, l4Boss.hp, l4Boss.maxHp);
        }
    }

    printf("=============== LEVEL 4 PLAYTEST (%s) ===============\n",
           godMode ? "immortal probe" : "full damage");
    printf("end state          : %d  (%s)\n", gameState,
           gameState == STATE_PUZZLE4 ? "reached FINAL PUZZLE" :
           gameState == STATE_GAME_OVER ? "GAME OVER" : "ran out of time");
    printf("frames             : %d   (%d:%02d at 62.5fps)\n",
           frames, (int)(frames / 62.5) / 60, (int)(frames / 62.5) % 60);
    printf("final world x      : %.0f / %d\n", worldX(), L4_WORLD_WIDTH);
    printf("section reached    : %d / %d\n", l4Section, L4_NUM_SECTIONS);
    printf("kills              : %d   (max on screen at once: %d)\n", totalKills, maxEnemiesAtOnce);
    printf("shots fired        : %d   (hazard escapes: %d)\n", shotsFired, escFires);
    printf("boss               : spawn f=%d  phase2 f=%d  hp=%d/%d defeated=%d\n",
           bossSpawnFrame, bossPhase2Frame, l4Boss.hp, l4Boss.maxHp, (int)l4Boss.defeated);
    printf("damage taken       : %d  (= %.1f health bars)  deaths: %d\n",
           damageTaken, damageTaken / (float)L4_PLAYER_MAX_HP, deaths);
    printf("score              : %d\n", l4Score);
    printf("--- section / gate timeline ---\n");
    for (int s = 1; s <= L4_NUM_SECTIONS; s++) {
        char a[32], b[32];
        if (sectionFirstFrame[s] < 0) sprintf(a, "never");
        else sprintf(a, "%5.1fs", sectionFirstFrame[s] / 62.5f);
        if (gateFrame[s] < 0) sprintf(b, "closed");
        else sprintf(b, "%5.1fs", gateFrame[s] / 62.5f);
        printf("  S%d %-14s entered %-8s gate %s\n", s, L4_SECTION_NAMES[s], a, b);
    }
    if (puzzleFrame >= 0)
        printf("PUZZLE reached at  : %.1fs (%d:%02d)\n", puzzleFrame / 62.5f,
               (int)(puzzleFrame / 62.5) / 60, (int)(puzzleFrame / 62.5) % 60);
    else
        printf("STALLED at wx=%.0f section=%d wave=%d (stall counter %d)\n",
               worldX(), l4Section, l4Wave, stall);
    return 0;
}
