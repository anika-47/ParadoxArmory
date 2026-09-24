#ifndef OBSTACLE_H
#define OBSTACLE_H

// Level 2 "black ball" hazard.
//
// Behavior is adapted from the swinging wrecking-ball trap in the
// Game-Development reference project (Level2.hpp: level2.wreckingBalls /
// DrawWreckingBalls() - anchored chain, sinusoidal swing angle, radius-
// based impact check), redrawn here as a solid black ball on a chain to
// fit this game's simpler single-screen arena (parad1_fixed_repaired has
// no scrolling camera, so the anchor point is fixed instead of following
// the player down a level).
//
// IMPORTANT: this file deliberately never includes enemy.h and never
// reads or writes any enemy state. That is what guarantees the ball can
// only ever damage the player, per the task rule "ONLY THE PLAYER CAN
// TAKE DAMAGE FROM THE BLACK-BALL OBSTACLE" - there is nothing in this
// translation unit it could apply damage to except playerHealth.

void LoadObstacleAssets(void);   // kept for symmetry with LoadPlayerAssets()/LoadEnemyAssets(); the ball is drawn procedurally, so there is currently nothing to load
void InitObstacle(void);         // resets the swing to its starting state; call once when Level 2 starts
void UpdateObstacle(void);       // advances the swing and checks player collision; only acts while currentLevel == 2
void RenderObstacle(void);       // draws the chain + ball; only draws while currentLevel == 2

#endif // OBSTACLE_H
