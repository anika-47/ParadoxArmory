#ifndef PLAYER_H
#define PLAYER_H

#include "gameConfig.h"

// Shared Player State
extern int playerX;
extern int playerY;
extern int playerWidth;
extern int playerHeight;
extern int playerHealth;
extern int playerMaxHealth;
extern bool playerFacingRight;
extern bool isCrouching;
extern bool isJumping;


// Mouse State
extern int mouseX;
extern int mouseY;

// Shared Player Bullet State
extern int bulletImg;
extern int bulletWidth;
extern int bulletHeight;
extern int p_bx[MAX_PLAYER_BULLETS];
extern int p_by[MAX_PLAYER_BULLETS];
extern bool p_bulletActive[MAX_PLAYER_BULLETS];

// Player Function Declarations
void LoadPlayerAssets(void);
void SetPlayerCharacter(int level);  // switches active sprite/bullet set for the given level; called from InitEnemyLevel()
void updatePlayer(void);          // per-frame: realtime input + bullets + animation
void RenderPlayer(void);
void RenderLevel3Part1Platforms(void); // Level 3 merge: floating brick platforms
void HandlePlayerKeyboardInput(unsigned char key);   // one-time key actions (ESC, F)
void HandlePlayerMouseInput(int button, int state, int mx, int my);
void HandlePlayerMouseMove(int mx, int my);



#endif // PLAYER_H