#ifndef ENEMY_H
#define ENEMY_H

#include "gameConfig.h"
#include "player.h"

// Shared Enemy State
extern int enemyLife;
extern int enemyMaxLife;

// Enemy Function Declarations
void InitEnemyLevel(int level);
void LoadEnemyAssets(void);
void UpdateEnemyLogic(void);
void RenderEnemies(void);

void InitEnemyLevel(int level);
void InitEnemyPart(int part); // Level 3 merge: mid-level enemy refresh between parts

void InitLevel3Part1Enemies(void);
bool IsLevel3Part1Complete(void);

#endif // ENEMY_H