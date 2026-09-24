#ifndef LEVEL4_H
#define LEVEL4_H

// =====================================================================
// LEVEL 4 - "PARADOX GROVE"
// =====================================================================
//
// A new, self-contained level module. NOTHING in player.cpp / player.h is
// modified by this level: the realtime GetAsyncKeyState input system,
// PLAYER_SPEED, jump/gravity, crouch, the animation system and the bullet
// pool all stay exactly as they were. This module only:
//
//   * reads  playerX / playerY / isCrouching / p_bx / p_by / p_bulletActive
//   * writes playerX (scroll + solid-collision push-back only) and
//     playerHealth (damage, through ONE choke-point function)
//
// The level is a horizontally scrolling world 9600px wide, divided into 8
// playable sections plus a final puzzle stage. Scrolling is done by moving
// a camera and pushing playerX back to a fixed screen anchor AFTER
// updatePlayer() has run - so player.cpp never has to know the world is
// bigger than the screen.
//
// See LEVEL4_NOTES.md for the full design/architecture write-up.
// =====================================================================

#include "gameConfig.h"

// Level id used by currentLevel / selectedLevel.
#define LEVEL4_ID 4

// Loaded once from main() - see iMain.cpp.
extern int backgroundImg4;

// ---- lifecycle -------------------------------------------------------
void LoadLevel4Assets(void);   // load textures once, at startup
void InitLevel4(void);         // reset the whole level (called on level select)
void UpdateLevel4(void);       // per-frame logic; called from updateGameLogic()
void RenderLevel4(void);       // draws background, world, player and HUD

// ---- queries used by the integration layer ---------------------------
int  Level4CameraX(void);      // current world-scroll offset, in pixels

#endif // LEVEL4_H
