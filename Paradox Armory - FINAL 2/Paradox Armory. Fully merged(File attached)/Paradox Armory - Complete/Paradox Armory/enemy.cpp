#include "enemy.h"
#include "gameState.h"
#include "player.h"
#include "iGraphics.h"
#include "defines.h"    // needed for STATE_PUZZLE
#include "puzzle.h"     // needed for InitPuzzle()
#include "variable.h"   // needed for gameState

// Enemy Asset Handlers
int enemyPic[6];      // facing left (default art)
int enemyPicRight[6]; // facing right (e1_r..e5_r) -- used when chasing left-to-right
int tankImg = -1;
int enemyBulletImage = -1;

// Enemy Coordinates & Attributes (Definitions)
int enemyCorX = 1400;
int enemyCorY = GROUND_Y;
int enemyIndex = 0;
int enemyLife = 10000;
int enemyMaxLife = 10000;
bool enemyFacingLeft = true;
bool isEnemyMoving = false;

// Enemy Bullets
int e_bx[MAX_ENEMY_BULLETS];
int e_by[MAX_ENEMY_BULLETS];
int e_bulletVY[MAX_ENEMY_BULLETS]; // vertical launch speed for arc'd shots; 0 = flies flat like before
bool e_bulletDirRight[MAX_ENEMY_BULLETS];
bool e_bulletActive[MAX_ENEMY_BULLETS];
static const int ENEMY_BULLET_ARC_GRAVITY = 1; // same feel as the player's own jump gravity (player.cpp)

int enemyBulletSpawnTimer = 0;
int enemyAnimTimer = 0;

static int l3p1RespawnTimer = 0;
static const int L3P1_RESPAWN_DELAY = 90; // 1s pause before the next enemy appears
static int l3p1PendingSlot = -1; // which slot is waiting to respawn, -1 = none

static bool CheckCollision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
	return (x1 < x2 + w2 &&
		x1 + w1 > x2 &&
		y1 < y2 + h2 &&
		y1 + h1 > y2);
}

// The tank sprite's source art faces left. iShowImage always draws it as-is,
// so previously the tank visually always faced left regardless of which way
// it was actually firing -- if the player circled around to its other side,
// it kept "looking" left while shooting right. This draws it normally when
// facingLeft is true, and horizontally mirrored (by swapping the texture's
// U coordinates) when it should be facing right.
static void DrawTankFacing(int x, int y, int width, int height, unsigned int texture, bool facingLeft) {
	if (facingLeft) {
		iShowImage(x, y, width, height, texture);
		return;
	}

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glBegin(GL_QUADS);
	glTexCoord2f(1, 0);  glVertex2f((GLfloat)x, (GLfloat)y);
	glTexCoord2f(0, 0);  glVertex2f((GLfloat)(x + width), (GLfloat)y);
	glTexCoord2f(0, -1); glVertex2f((GLfloat)(x + width), (GLfloat)(y + height));
	glTexCoord2f(1, -1); glVertex2f((GLfloat)x, (GLfloat)(y + height));
	glEnd();

	glDisable(GL_TEXTURE_2D);
}

// ---------------- Level 3 Part 1: multi-enemy swarm (placeholder) ----------------
// Up to 5 on screen at once, 10 total to clear the part. Spawns off-screen at
// random x/height, chases the player, and "climbs" toward whatever platform
// height is under its current x. Alternates melee (contact damage) and
// shooter (spawns a bullet) types. Rectangles for now -- swap the
// iFilledRectangle calls for iShowImage once you have sprites.

struct L3P1Enemy {
	bool alive;
	float x, y;
	int width, height;
	int health;
	int maxHealth;
	bool facingLeft;
	bool isMelee;
	int shootTimer;
	int meleeCooldown;
	int spawnInvuln;
	bool spawnedFromLeft; // new: tracks which side this enemy came from
	int frameIndex;   // new: current walk-cycle frame
	int animTimer;    // new: counts frames between animation steps
};

static const int MAX_L3P1_ENEMIES = 3;
static const int L3P1_TOTAL_TO_KILL = 10;
static L3P1Enemy l3p1Enemies[MAX_L3P1_ENEMIES];
static const int L3P1_ENEMY_FRAMES = 8; // 8 forward + 8 backward frames
static int l3p1WalkRight[L3P1_ENEMY_FRAMES]; // moving toward the right
static int l3p1WalkLeft[L3P1_ENEMY_FRAMES];  // moving toward the left
static int l3p1SpawnedCount = 0;
static int l3p1KilledCount = 0;
static const int L3P1_ENEMY_W = 90;
static const int L3P1_ENEMY_H = 130;

static void SpawnOneL3P1Enemy(int slot) {
	L3P1Enemy& e = l3p1Enemies[slot];
	e.alive = true;
	e.width = L3P1_ENEMY_W;
	e.height = L3P1_ENEMY_H;

	e.frameIndex = 0;
	e.animTimer = 0;

	// Count how many currently alive enemies came from each side, and cap
	// either side at 2 -- so no more than 2 ever approach from the same
	// direction at once.
	int leftCount = 0, rightCount = 0;
	for (int i = 0; i < MAX_L3P1_ENEMIES; i++) {
		if (l3p1Enemies[i].alive) {
			if (l3p1Enemies[i].spawnedFromLeft) leftCount++;
			else rightCount++;
		}
	}

	bool fromLeft;
	if (leftCount >= 2) fromLeft = false;
	else if (rightCount >= 2) fromLeft = true;
	else fromLeft = (rand() % 2 == 0);

	e.spawnedFromLeft = fromLeft;
	e.x = fromLeft ? (float)(-e.width - rand() % 200) : (float)(SCREEN_WIDTH + rand() % 200);
	e.facingLeft = !fromLeft;

	// Random starting height: ground, or one of the floating platforms.
	int heightRoll = rand() % (NUM_L3P1_PLATFORMS + 1);
	e.y = (heightRoll == NUM_L3P1_PLATFORMS) ? (float)GROUND_Y : (float)level3Part1Platforms[heightRoll].y;

	// Alternate type; health tuned low so a fast player clears each in 2-3 hits.
	e.isMelee = (l3p1SpawnedCount % 2 == 0);
	e.health = e.isMelee ? 1200 : 900; // raised so clearing all 10 takes longer
	e.maxHealth = e.health; // store the starting value so the HP bar can show percentage
	e.spawnInvuln = 30; // ~0.5s grace period after appearing, can't be hit yet

	e.shootTimer = 30 + rand() % 30;
	e.meleeCooldown = 0;

	l3p1SpawnedCount++;
}

void InitLevel3Part1Enemies(void) {
	l3p1SpawnedCount = 0;
	l3p1KilledCount = 0;
	for (int i = 0; i < MAX_L3P1_ENEMIES; i++) l3p1Enemies[i].alive = false;
	for (int i = 0; i < MAX_L3P1_ENEMIES && l3p1SpawnedCount < L3P1_TOTAL_TO_KILL; i++) {
		SpawnOneL3P1Enemy(i);
	}
}

bool IsLevel3Part1Complete(void) {
	return l3p1KilledCount >= L3P1_TOTAL_TO_KILL;
}

// Level 3 Part 1: highest platform under this x-position (or ground if none).
// Unlike the player's landing check, this doesn't require the enemy to
// already be near that height -- it's the CLIMB target, computed purely
// from horizontal position.
static int GetFloorYAt(float x) {
	int floorY = GROUND_Y;
	for (int i = 0; i < NUM_L3P1_PLATFORMS; i++) {
		const Platform& p = level3Part1Platforms[i];
		if (x >= p.x && x <= p.x + p.width) {
			if (p.y > floorY) floorY = p.y;
		}
	}
	return floorY;
}

// Returns the index of the platform the player is currently standing on
// (within NUM_L3P1_PLATFORMS), or -1 if the player is on the ground/airborne.
static int GetPlayerCurrentPlatformIndex() {
	int centerX = playerX + playerWidth / 2;
	for (int i = 0; i < NUM_L3P1_PLATFORMS; i++) {
		const Platform& p = level3Part1Platforms[i];
		if (centerX >= p.x && centerX <= p.x + p.width && fabs((float)playerY - (float)p.y) <= 20.0f) {
			return i;
		}
	}
	return -1;
}
// Level 3 Part 1: pushes any two overlapping enemies apart horizontally so
// they don't stack on top of each other while chasing the same player spot.
static void SeparateL3P1Enemies(void) {
	for (int i = 0; i < MAX_L3P1_ENEMIES; i++) {
		if (!l3p1Enemies[i].alive) continue;
		for (int j = i + 1; j < MAX_L3P1_ENEMIES; j++) {
			if (!l3p1Enemies[j].alive) continue;

			L3P1Enemy& a = l3p1Enemies[i];
			L3P1Enemy& b = l3p1Enemies[j];

			// Only separate if they're roughly at the same height (same
			// platform/ground level) -- enemies on different platforms
			// shouldn't push each other.
			if (fabs(a.y - b.y) > 20.0f) continue;

			if (CheckCollision((int)a.x, (int)a.y, a.width, a.height,
				(int)b.x, (int)b.y, b.width, b.height)) {

				float overlap = (a.x < b.x)
					? (a.x + a.width - b.x)
					: (b.x + b.width - a.x);
				float push = overlap / 2.0f + 1.0f;

				if (a.x < b.x) {
					a.x -= push;
					b.x += push;
				}
				else {
					a.x += push;
					b.x -= push;
				}
			}
		}
	}
}

// Level 3 Part 1: gently nudges the player and an overlapping enemy apart,
// a little each frame, instead of snapping the full overlap distance
// instantly (which felt like the enemy was yanking the player around).
static void SeparatePlayerFromL3P1Enemies(void) {
	int currentPlayerH = isCrouching ? (playerHeight / 2) : playerHeight;
	const float MAX_PUSH_PER_FRAME = 1.0f; // lower = slower/softer push

	for (int i = 0; i < MAX_L3P1_ENEMIES; i++) {
		L3P1Enemy& e = l3p1Enemies[i];
		if (!e.alive) continue;

		if (fabs(e.y - (float)playerY) > 20.0f) continue; // different height, ignore

		if (CheckCollision(playerX, playerY, playerWidth, currentPlayerH,
			(int)e.x, (int)e.y, e.width, e.height)) {

			float push = MAX_PUSH_PER_FRAME; // fixed small step, regardless of overlap size

			if (playerX < e.x) {
				playerX -= (int)push;
				e.x += push;
			}
			else {
				playerX += (int)push;
				e.x -= push;
			}

			if (playerX < 0) playerX = 0;
			if (playerX + playerWidth > SCREEN_WIDTH) playerX = SCREEN_WIDTH - playerWidth;
		}
	}
}

void UpdateLevel3Part1Enemies(void) {
	int playerCenterX = playerX + playerWidth / 2;


	if (l3p1PendingSlot != -1) {
		l3p1RespawnTimer--;
		if (l3p1RespawnTimer <= 0) {
			SpawnOneL3P1Enemy(l3p1PendingSlot);
			l3p1PendingSlot = -1;
		}
	}

	int playerPlatformIndex = GetPlayerCurrentPlatformIndex();
	bool playerPlatformClaimed = false; // tracks whether an enemy has already taken the player's platform this frame

	for (int i = 0; i < MAX_L3P1_ENEMIES; i++) {
		L3P1Enemy& e = l3p1Enemies[i];
		if (!e.alive) continue;

		int enemyCenterX = (int)e.x + e.width / 2;
		e.facingLeft = (playerCenterX <= enemyCenterX);

		float oldX = e.x;

		int speed = 3;
		int stopDistance = e.isMelee ? 40 : 320;
		if (e.facingLeft && enemyCenterX > playerCenterX + stopDistance) e.x -= speed;
		else if (!e.facingLeft && enemyCenterX < playerCenterX - stopDistance) e.x += speed;

		bool isMoving = (e.x != oldX);
		e.animTimer++;
		if (isMoving) {
			if (e.animTimer % 6 == 0) {
				e.frameIndex = (e.frameIndex + 1) % L3P1_ENEMY_FRAMES;
			}
		}
		else {
			e.frameIndex = 0;
			e.animTimer = 0;
		}

		int targetFloor = GetFloorYAt(e.x + e.width / 2.0f);

		// Only climb up onto a platform if the player is currently elevated too.
		// If the player is on the ground, enemies stay grounded even if a
		// platform happens to be above their x position.
		if (playerY <= GROUND_Y) {
			targetFloor = GROUND_Y;
		}

		if (e.y < targetFloor) e.y += 4;
		else if (e.y > targetFloor) e.y -= 4;

		int currentPlayerH = isCrouching ? (playerHeight / 2) : playerHeight;

		if (e.isMelee) {
			if (e.meleeCooldown > 0) e.meleeCooldown--;
			bool overlapping = CheckCollision((int)e.x, (int)e.y, e.width, e.height,
				playerX, playerY, playerWidth, currentPlayerH);
			if (overlapping && e.meleeCooldown == 0) {
				playerHealth -= 150;
				if (playerHealth < 0) playerHealth = 0;
				e.meleeCooldown = 40;
			}
		}
		else {
			// Only shoot when close enough in height that the bullet's flat,
			// horizontal path could actually reach the player -- otherwise
			// it's just wasted shots at the wrong altitude.
			bool playerInLineOfFire = fabs(e.y - (float)playerY) <= 30.0f;

			if (playerInLineOfFire) {
				e.shootTimer--;
			}

			if (playerInLineOfFire && e.shootTimer <= 0) {
				e.shootTimer = 60;
				for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
					if (!e_bulletActive[j]) {
						e_bulletActive[j] = true;
						e_bulletDirRight[j] = !e.facingLeft;
						e_bx[j] = e.facingLeft ? (int)e.x : (int)(e.x + e.width);
						e_by[j] = (int)e.y + e.height / 2;
						break;
					}
				}
			}
		}

		if (e.spawnInvuln > 0) e.spawnInvuln--;

		for (int j = 0; j < MAX_PLAYER_BULLETS; j++) {
			if (e.spawnInvuln <= 0 && p_bulletActive[j] &&
				CheckCollision(p_bx[j], p_by[j], bulletWidth, bulletHeight,
				(int)e.x, (int)e.y, e.width, e.height)) {
				p_bulletActive[j] = false;
				e.health -= 150;
				if (e.health <= 0) {
					e.alive = false;
					l3p1KilledCount++;
					if (l3p1SpawnedCount < L3P1_TOTAL_TO_KILL && l3p1PendingSlot == -1) {
						l3p1PendingSlot = i;
						l3p1RespawnTimer = L3P1_RESPAWN_DELAY;
					}
				}
			}
		}
	}

	SeparateL3P1Enemies();
	SeparatePlayerFromL3P1Enemies();



	for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
		if (e_bulletActive[j]) {
			int bulletSpeed = 5;
			if (e_bulletDirRight[j]) {
				e_bx[j] += bulletSpeed;
				if (e_bx[j] > SCREEN_WIDTH) e_bulletActive[j] = false;
			}
			else {
				e_bx[j] -= bulletSpeed;
				if (e_bx[j] < 0) e_bulletActive[j] = false;
			}
			int currentH = isCrouching ? (playerHeight / 2) : playerHeight;
			if (e_bulletActive[j] && CheckCollision(e_bx[j], e_by[j], 20, 20, playerX, playerY, playerWidth, currentH)) {
				e_bulletActive[j] = false;
				playerHealth -= 200;
				if (playerHealth < 0) playerHealth = 0;
			}
		}
	}
}

// Level 3 Part 1: keeps the player and any overlapping enemy from visually
// stacking/passing through each other. Runs independently of the melee
// damage check above -- contact still hurts the player, this just stops
// the sprites from occupying the same space.


void RenderLevel3Part1Enemies(void) {
	for (int i = 0; i < MAX_L3P1_ENEMIES; i++) {
		L3P1Enemy& e = l3p1Enemies[i];
		if (!e.alive) continue;

		int spriteImg = e.facingLeft ? l3p1WalkLeft[e.frameIndex] : l3p1WalkRight[e.frameIndex];

		if (spriteImg > 0) {
			iShowImage((int)e.x, (int)e.y, e.width, e.height, spriteImg);
		}
		else {
			// Fallback placeholder if a sprite failed to load, so you can
			// still tell melee vs shooter apart while testing.
			if (e.isMelee) iSetColor(200, 40, 40);
			else iSetColor(140, 40, 200);
			iFilledRectangle(e.x, e.y, e.width, e.height);
			iSetColor(0, 0, 0);
			iRectangle(e.x, e.y, e.width, e.height);
		}
		// Floating HP bar above this enemy's head.
		int barW = 60, barH = 8;
		int barX = (int)e.x + (e.width - barW) / 2;
		int barY = (int)e.y + e.height + 10;

		iSetColor(80, 80, 80);
		iRectangle(barX, barY, barW, barH);

		int fillW = (int)(((float)e.health / e.maxHealth) * barW);
		if (fillW < 0) fillW = 0;
		iSetColor(220, 0, 0);
		iFilledRectangle(barX, barY, fillW, barH);
	} // <-- this was the missing brace: closes the "for (int i...)" enemy loop



	for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
		if (e_bulletActive[j] && enemyBulletImage > 0) {
			iShowImage(e_bx[j], e_by[j], 20, 20, enemyBulletImage);
		}
	}

	char progressText[50];
	sprintf(progressText, "Enemies Defeated: %d / %d", l3p1KilledCount, L3P1_TOTAL_TO_KILL);
	iSetColor(255, 255, 255);
	iText(30, SCREEN_HEIGHT - 75, progressText, GLUT_BITMAP_HELVETICA_18);
}

// ---------------- Level 3 Part 3: fast single enemy, single weak-point hitbox ----------------
// Same overall shape as Part 2's boss (one tank-style enemy, ranged bullets,
// enemyLife counts down to clear the part) but with two differences:
//   1) It's a bit quicker than the other parts (though not as relentless as
//      an earlier tuning of this had it).
//   2) Player bullets only damage it when they land inside one specific
//      "weak point" box drawn on its body -- everywhere else on the enemy is
//      immune. This is a single fixed box, not a multi-zone/body-part system.
static const int L3P3_ENEMY_W = 150;
static const int L3P3_ENEMY_H = 120;
static const int L3P3_WEAKPOINT_W = 36;
static const int L3P3_WEAKPOINT_H = 36;
static const int L3P3_WEAKPOINT_OFFSET_X = 55; // from the edge of the enemy it's facing toward
static const int L3P3_WEAKPOINT_OFFSET_Y = 40; // from the top of the enemy
static int l3p3WeakHitFlash = 0; // brief white flash so a successful hit reads clearly

// Weak point tracks whichever side is facing the player, so it's always on
// the side the player is actually shooting from.
static void GetL3P3WeakPointRect(int* outX, int* outY) {
	*outX = enemyFacingLeft
		? (enemyCorX + L3P3_WEAKPOINT_OFFSET_X)
		: (enemyCorX + L3P3_ENEMY_W - L3P3_WEAKPOINT_OFFSET_X - L3P3_WEAKPOINT_W);
	*outY = enemyCorY + L3P3_WEAKPOINT_OFFSET_Y;
}

void UpdateLevel3Part3Enemy(void) {
	if (enemyLife <= 0) return;

	enemyAnimTimer++;

	int enemyCenterX = enemyCorX + L3P3_ENEMY_W / 2;
	int playerCenterX = playerX + playerWidth / 2;
	enemyFacingLeft = (playerCenterX <= enemyCenterX);

	// Fast, but not as relentless as before -- still noticeably quicker than
	// the other parts, just with a bit more breathing room to react.
	int speed = 6;
	int stopDistance = 110;
	if (enemyFacingLeft && enemyCorX > playerX + stopDistance) enemyCorX -= speed;
	else if (!enemyFacingLeft && enemyCorX + L3P3_ENEMY_W < playerX - stopDistance) enemyCorX += speed;

	if (l3p3WeakHitFlash > 0) l3p3WeakHitFlash--;

	// Burst fire: 3 bullets launched together, fanned out as an arc rather
	// than a single flat shot. Slightly wider spread and a bit less cooldown
	// than Part 2's burst to match this part's faster pace.
	enemyBulletSpawnTimer++;
	if (enemyBulletSpawnTimer >= 35) {
		enemyBulletSpawnTimer = 0;
		static const int BURST_VY[3] = { 12, 0, -12 }; // up / straight / down launch speeds
		int fired = 0;
		for (int j = 0; j < MAX_ENEMY_BULLETS && fired < 3; j++) {
			if (!e_bulletActive[j]) {
				e_bulletActive[j] = true;
				e_bulletDirRight[j] = !enemyFacingLeft;
				e_bx[j] = enemyFacingLeft ? enemyCorX : (enemyCorX + L3P3_ENEMY_W);
				e_by[j] = enemyCorY + 70;
				e_bulletVY[j] = BURST_VY[fired];
				fired++;
			}
		}
	}

	// Enemy bullet movement & collision (same shape as the generic tank's, plus the arc).
	for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
		if (e_bulletActive[j]) {
			int bulletSpeed = 6; // a touch faster too
			if (e_bulletDirRight[j]) {
				e_bx[j] += bulletSpeed;
				if (e_bx[j] > SCREEN_WIDTH) e_bulletActive[j] = false;
			}
			else {
				e_bx[j] -= bulletSpeed;
				if (e_bx[j] < 0) e_bulletActive[j] = false;
			}

			e_by[j] += e_bulletVY[j];
			e_bulletVY[j] -= ENEMY_BULLET_ARC_GRAVITY;
			if (e_by[j] < -50 || e_by[j] > SCREEN_HEIGHT + 50) e_bulletActive[j] = false;

			int currentH = isCrouching ? (playerHeight / 2) : playerHeight;
			if (e_bulletActive[j] && CheckCollision(e_bx[j], e_by[j], 20, 20, playerX, playerY, playerWidth, currentH)) {
				e_bulletActive[j] = false;
				playerHealth -= 200;
				if (playerHealth < 0) playerHealth = 0;
			}
		}
	}

	// Player bullets only register against the weak point box -- a hit
	// anywhere else on the enemy's body simply does nothing.
	int weakX, weakY;
	GetL3P3WeakPointRect(&weakX, &weakY);
	for (int j = 0; j < MAX_PLAYER_BULLETS; j++) {
		if (p_bulletActive[j] &&
			CheckCollision(p_bx[j], p_by[j], bulletWidth, bulletHeight, weakX, weakY, L3P3_WEAKPOINT_W, L3P3_WEAKPOINT_H)) {
			p_bulletActive[j] = false;
			enemyLife -= 200;
			l3p3WeakHitFlash = 10;
			if (enemyLife < 0) enemyLife = 0;
		}
	}
}

void RenderLevel3Part3Enemy(void) {
	if (enemyLife > 0) {
		if (tankImg > 0) DrawTankFacing(enemyCorX, enemyCorY, L3P3_ENEMY_W, L3P3_ENEMY_H, tankImg, enemyFacingLeft);
		else { iSetColor(150, 50, 50); iFilledRectangle(enemyCorX, enemyCorY, L3P3_ENEMY_W, L3P3_ENEMY_H); }

		// The weak point box -- the only spot that actually damages this
		// enemy. Flashes white for a few frames right after a successful hit.
		int weakX, weakY;
		GetL3P3WeakPointRect(&weakX, &weakY);
		if (l3p3WeakHitFlash > 0) iSetColor(255, 255, 255);
		else iSetColor(255, 210, 0);
		iFilledRectangle(weakX, weakY, L3P3_WEAKPOINT_W, L3P3_WEAKPOINT_H);
		iSetColor(0, 0, 0);
		iRectangle(weakX, weakY, L3P3_WEAKPOINT_W, L3P3_WEAKPOINT_H);
	}

	for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
		if (e_bulletActive[j] && enemyBulletImage > 0) {
			iShowImage(e_bx[j], e_by[j], 20, 20, enemyBulletImage);
		}
	}

	int barX = 30;
	int eBarY = SCREEN_HEIGHT - 100;
	int barWidth = 300;
	int barHeight = 20;
	iSetColor(100, 100, 100);
	iRectangle(barX, eBarY, barWidth, barHeight);
	int eFillWidth = (int)(((double)enemyLife / enemyMaxLife) * barWidth);
	if (eFillWidth < 0) eFillWidth = 0;
	iSetColor(220, 0, 0);
	iFilledRectangle(barX, eBarY, eFillWidth, barHeight);

	char eText[50];
	sprintf(eText, "Enemy HP (Lvl 3 Part 3): %d / %d", enemyLife, enemyMaxLife);
	iSetColor(255, 255, 255);
	iText(barX, eBarY + 25, eText, GLUT_BITMAP_HELVETICA_18);
}

void InitEnemyLevel(int level) {
	currentLevel = level;
	SetPlayerCharacter(level); // switch active sprite/bullet set to match the level

	// Scale single enemy health by level
	if (level == 1) {
		enemyMaxLife = 10000;
	}
	else if (level == 2) {
		enemyMaxLife = 20000;
	}
	else {
		enemyMaxLife = 30000; // Level 3 Tank
	}

	enemyLife = enemyMaxLife;

	// Synchronize player health to enemy max health on new level
	playerMaxHealth = enemyMaxLife;
	playerHealth = playerMaxHealth;

	// Reset combat positions
	playerX = 200;
	playerY = GROUND_Y;

	enemyCorX = 1400;
	enemyCorY = GROUND_Y;
	enemyIndex = 0;
	enemyFacingLeft = true;
	isEnemyMoving = false;

	for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
		e_bulletActive[i] = false;
		e_bulletVY[i] = 0;
	}
	for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
		p_bulletActive[i] = false;
	}

	if (level == 3) {
		InitLevel3Part1Enemies();
	}

}

void InitEnemyPart(int part) {
	if (part == 2) {
		enemySegmentsCleared = 0;
		inSegmentPuzzle = false;

		// Level 3 Part 2 merge: player starts this part completely fresh
		// (full HP), and the enemy is scaled relative to that so it's
		// always far tougher than the player, whatever playerMaxHealth is.
		playerHealth = playerMaxHealth;
		enemyMaxLife = playerMaxHealth * 0.75; // tweak the multiplier to taste
	}
	else if (part == 1) {
		enemyMaxLife = 15000;
	}
	else {
		enemyMaxLife = 30000; // Part 3 - toughest

		// This part adds a fast enemy plus extra hazards (swinging balls,
		// the spiked saw), so the player gets a survivability buff here:
		// max/current HP set to just under the enemy's health pool.
		playerMaxHealth = enemyMaxLife - 1000;
		playerHealth = playerMaxHealth;
	}

	enemyLife = enemyMaxLife;

	playerX = 200;
	playerY = GROUND_Y;

	enemyCorX = 1400;
	enemyCorY = GROUND_Y;
	enemyIndex = 0;
	enemyFacingLeft = true;
	isEnemyMoving = false;

	for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
		e_bulletActive[i] = false;
		e_bulletVY[i] = 0;
	}
	for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
		p_bulletActive[i] = false;
	}
}
void LoadEnemyAssets(void) {
	enemyPic[0] = iLoadImage("images\\e1.png");
	enemyPic[1] = iLoadImage("images\\e2.png");
	enemyPic[2] = iLoadImage("images\\e3.png");
	enemyPic[3] = iLoadImage("images\\e4.png");
	enemyPic[4] = iLoadImage("images\\e5.png");
	enemyPic[5] = enemyPic[4];

	// Right-facing counterparts -- used when this enemy is chasing the
	// player toward the right instead of the left.
	enemyPicRight[0] = iLoadImage("images\\e1_r.png");
	enemyPicRight[1] = iLoadImage("images\\e2_r.png");
	enemyPicRight[2] = iLoadImage("images\\e3_r.png");
	enemyPicRight[3] = iLoadImage("images\\e4_r.png");
	enemyPicRight[4] = iLoadImage("images\\e5_r.png");
	enemyPicRight[5] = enemyPicRight[4];

	tankImg = iLoadImage("images\\tank.png");
	enemyBulletImage = iLoadImage("images\\bullet1.png");

	// Level 3 Part 1 swarm enemy walk cycle (shared by melee + shooter types).
	// Rename these to your actual filenames -- forward = moving right,
	// backward = moving left.
	for (int i = 0; i < L3P1_ENEMY_FRAMES; i++) {
		char pathRight[64], pathLeft[64];
		sprintf(pathRight, "images\\l3_enemy_forward%d.png", i + 1);
		sprintf(pathLeft, "images\\l3_enemy_backward%d.png", i + 1);
		l3p1WalkRight[i] = iLoadImage(pathRight);
		l3p1WalkLeft[i] = iLoadImage(pathLeft);
		printf("Loaded %s -> %d | %s -> %d\n", pathRight, l3p1WalkRight[i], pathLeft, l3p1WalkLeft[i]);
	}   // <-- closes the for loop
}       // <-- closes LoadEnemyAssets itself

void UpdateEnemyLogic(void) {
	
		if (currentLevel == 3 && currentPart == 1) {
			UpdateLevel3Part1Enemies();
			return;
		}
		if (currentLevel == 3 && currentPart == 3) {
			UpdateLevel3Part3Enemy();
			return;
		}
	if (enemyLife <= 0) return;

	enemyAnimTimer++;

	int enemyWidth = (currentLevel == 3) ? 150 : 100;
	int enemyCenterX = enemyCorX + (enemyWidth / 2);
	int playerCenterX = playerX + (playerWidth / 2);


	enemyFacingLeft = (playerCenterX <= enemyCenterX);

	int oldX = enemyCorX;
	int speed = 2;
	int stopDistance = 200;

	// AI horizontal movement
	if (enemyFacingLeft && (enemyCorX > playerX + stopDistance)) {
		enemyCorX -= speed;
	}
	else if (!enemyFacingLeft && (enemyCorX + enemyWidth < playerX - stopDistance)) {
		enemyCorX += speed;
	}

	// Cycles through all 6 enemy frames (0 to 5)
	if (enemyCorX != oldX) {
		isEnemyMoving = true;
		if (enemyAnimTimer % 6 == 0) {
			enemyIndex = (enemyIndex + 1) % TOTAL_ENEMY_FRAMES;
		}
	}
	else {
		isEnemyMoving = false;
		enemyIndex = 0;
	}

	// Shooting logic. Level 3 Part 2 fires a 3-bullet burst fanned out as an
	// arc (different vertical launch speeds); every other level/part keeps
	// the original single flat bullet, unchanged.
	bool isL3P2Burst = (currentLevel == 3 && currentPart == 2);
	enemyBulletSpawnTimer++;
	int fireDelay = isL3P2Burst ? 55 : 40; // burst hits harder, so a slightly longer cooldown
	if (enemyBulletSpawnTimer >= fireDelay) {
		enemyBulletSpawnTimer = 0;
		if (isL3P2Burst) {
			static const int BURST_VY[3] = { 10, 0, -10 }; // up / straight / down launch speeds
			int fired = 0;
			for (int j = 0; j < MAX_ENEMY_BULLETS && fired < 3; j++) {
				if (!e_bulletActive[j]) {
					e_bulletActive[j] = true;
					e_bulletDirRight[j] = !enemyFacingLeft;
					e_bx[j] = enemyFacingLeft ? enemyCorX : (enemyCorX + enemyWidth);
					e_by[j] = enemyCorY + 70;
					e_bulletVY[j] = BURST_VY[fired];
					fired++;
				}
			}
		}
		else {
			for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
				if (!e_bulletActive[j]) {
					e_bulletActive[j] = true;
					e_bulletDirRight[j] = !enemyFacingLeft;
					e_bx[j] = enemyFacingLeft ? enemyCorX : (enemyCorX + enemyWidth);
					e_by[j] = enemyCorY + 70;
					e_bulletVY[j] = 0;
					break;
				}
			}
		}
	}

	// Enemy bullet movement & collision check
	for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
		if (e_bulletActive[j]) {
			int bulletSpeed = 5;
			if (e_bulletDirRight[j]) {
				e_bx[j] += bulletSpeed;
				if (e_bx[j] > SCREEN_WIDTH) e_bulletActive[j] = false;
			}
			else {
				e_bx[j] -= bulletSpeed;
				if (e_bx[j] < 0) e_bulletActive[j] = false;
			}

			// Arc motion -- only ever nonzero for the Level 3 Part 2 burst
			// above, so every other bullet keeps flying perfectly flat.
			if (e_bulletVY[j] != 0) {
				e_by[j] += e_bulletVY[j];
				e_bulletVY[j] -= ENEMY_BULLET_ARC_GRAVITY;
				if (e_by[j] < -50 || e_by[j] > SCREEN_HEIGHT + 50) e_bulletActive[j] = false;
			}

			int currentH = isCrouching ? (playerHeight / 2) : playerHeight;
			if (e_bulletActive[j] && CheckCollision(e_bx[j], e_by[j], 20, 20, playerX, playerY, playerWidth, currentH)) {
				e_bulletActive[j] = false;
				playerHealth -= 200;
				if (playerHealth < 0) playerHealth = 0;
			}
		}
	}

	// Player bullet hits enemy & progression check
	for (int j = 0; j < MAX_PLAYER_BULLETS; j++) {
		if (p_bulletActive[j]) {
			int enemyHeight = (currentLevel == 3) ? 120 : 150;
			
			if (CheckCollision(p_bx[j], p_by[j], bulletWidth, bulletHeight, enemyCorX, enemyCorY, enemyWidth, enemyHeight)) {
				p_bulletActive[j] = false;

				// Level 3 Part 2 merge: player bullets hit harder here than
				// against Level 2's or Part 3's enemy.
				int damage = (currentLevel == 3 && currentPart == 2) ? 400 : 200;
				enemyLife -= damage;

				if (enemyLife <= 0) {
					enemyLife = 0;
				}
				// Level 3 Part 2 merge: enemy HP split into 4 quarters. Crossing
				// each of the first 3 quarter-boundaries pauses combat for a
				// puzzle; crossing the 4th (full death) does NOT -- that falls
				// through to game.h's normal part-complete handling instead.
				if (currentLevel == 3 && currentPart == 2 && enemySegmentsCleared < 2) {
					int segmentFloor = enemyMaxLife - (enemyMaxLife / 3) * (enemySegmentsCleared + 1);
					if (enemyLife <= segmentFloor) {
						enemySegmentsCleared++;
						inSegmentPuzzle = true;
						InitPuzzle();
						gameState = STATE_PUZZLE;
					}
				}
			}
		}
	}
}

void RenderEnemies(void) {
	int barX = 30;
	int pBarY = SCREEN_HEIGHT - 50;
	int barWidth = 300;
	int barHeight = 20;

	iSetColor(100, 100, 100);
	iRectangle(barX, pBarY, barWidth, barHeight);
	int pFillWidth = (int)(((double)playerHealth / playerMaxHealth) * barWidth);
	if (pFillWidth < 0) pFillWidth = 0;
	iSetColor(0, 220, 0);
	iFilledRectangle(barX, pBarY, pFillWidth, barHeight);

	char pText[50];
	sprintf(pText, "Player HP: %d / %d", playerHealth, playerMaxHealth);
	iSetColor(255, 255, 255);
	iText(barX, pBarY + 25, pText, GLUT_BITMAP_HELVETICA_18);

	if (currentLevel == 3 && currentPart == 1) {
		RenderLevel3Part1Enemies();
		return;
	}
	if (currentLevel == 3 && currentPart == 3) {
		RenderLevel3Part3Enemy();
		return;
	}

	// ---- existing single-enemy (tank) rendering below, unchanged ----
	int eBarY = SCREEN_HEIGHT - 100;
	if (enemyLife > 0) {
		if (currentLevel == 3) {
			if (tankImg > 0) DrawTankFacing(enemyCorX, enemyCorY, 150, 120, tankImg, enemyFacingLeft);
			else { iSetColor(150, 50, 50); iFilledRectangle(enemyCorX, enemyCorY, 150, 120); }
		}
		else {
			int currentFrame = isEnemyMoving ? (enemyIndex % TOTAL_ENEMY_FRAMES) : 0;
			// Faces the player: the default e1-e5 set for facing left, the
			// e1_r-e5_r set for facing right -- real art either way, no
			// mirroring involved.
			int* activeSet = enemyFacingLeft ? enemyPic : enemyPicRight;
			if (activeSet[currentFrame] > 0) iShowImage(enemyCorX, enemyCorY, 100, 150, activeSet[currentFrame]);
		}
	}

	for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
		if (e_bulletActive[j] && enemyBulletImage > 0) {
			iShowImage(e_bx[j], e_by[j], 20, 20, enemyBulletImage);
		}
	}

	iSetColor(100, 100, 100);
	iRectangle(barX, eBarY, barWidth, barHeight);
	int eFillWidth = (int)(((double)enemyLife / enemyMaxLife) * barWidth);
	if (eFillWidth < 0) eFillWidth = 0;
	iSetColor(220, 0, 0);
	iFilledRectangle(barX, eBarY, eFillWidth, barHeight);

	char eText[50];
	sprintf(eText, "Enemy HP (Lvl %d): %d / %d", currentLevel, enemyLife, enemyMaxLife);
	iSetColor(255, 255, 255);
	iText(barX, eBarY + 25, eText, GLUT_BITMAP_HELVETICA_18);
}