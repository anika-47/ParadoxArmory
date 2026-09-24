#include "obstacle.h"
#include "gameConfig.h"
#include "gameState.h"
#include "player.h"
#include "iGraphics.h"
#include <mmsystem.h>  
#include <math.h>

#pragma comment(lib, "winmm.lib")


static void PlayObstacleHitSound(void) {
	mciSendString("close hitsfx", NULL, 0, NULL);
	mciSendString("open \"Audios/bubble.mp3\" type mpegvideo alias hitsfx", NULL, 0, NULL);
	mciSendString("play hitsfx", NULL, 0, NULL);
}


static const float ANCHOR_X = 900.0f;
static const float ANCHOR_Y = 500.0f;
static const float CHAIN_LENGTH = 300.0f;
static const float BALL_RADIUS = 45.0f;
static const float SWING_RANGE = 1.05f;   
static const float SWING_SPEED = 1.6f;   
static const float STEP = 0.02f;          


static const int HIT_DAMAGE = 400;
static const int HIT_COOLDOWN_FRAMES = 45;

static float swingTimer = 0.0f;
static float ballAngle = 0.0f;
static int hitCooldown = 0;

void LoadObstacleAssets(void) {
	
}

// ---------------- Level 3 Part 3: multiple swinging balls + a horizontal spikey saw ----------------
// Same swinging-ball mechanic as the Level 2 hazard above (anchored chain,
// sinusoidal swing, radius-based impact), just several of them at once with
// different anchors/speeds/phases so they don't all swing in sync. Plus a
// new hazard: a spiked saw blade that sweeps across the screen near ground
// level -- jump to clear it.

struct L3P3Ball {
	float anchorX, anchorY;
	float chainLength;
	float swingRange;
	float swingSpeed;
	float swingTimer; // seeded with a phase offset so the balls desync
	float ballAngle;
	int hitCooldown;
};

static const int L3P3_NUM_BALLS = 3;
static L3P3Ball l3p3Balls[L3P3_NUM_BALLS];
static const float L3P3_BALL_RADIUS = 40.0f;
static const int L3P3_BALL_HIT_DAMAGE = 400; // same weight as the Level 2 ball
static const int L3P3_BALL_HIT_COOLDOWN_FRAMES = 45;

static void InitLevel3Part3Balls(void) {
	static const float anchorXs[L3P3_NUM_BALLS] = { 300.0f, 650.0f, 1000.0f };
	static const float speeds[L3P3_NUM_BALLS] = { 1.4f, 1.7f, 1.3f };
	static const float phases[L3P3_NUM_BALLS] = { 0.0f, 1.4f, 2.8f };

	for (int i = 0; i < L3P3_NUM_BALLS; i++) {
		l3p3Balls[i].anchorX = anchorXs[i];
		l3p3Balls[i].anchorY = 550.0f;
		l3p3Balls[i].chainLength = 250.0f;
		l3p3Balls[i].swingRange = 1.0f;
		l3p3Balls[i].swingSpeed = speeds[i];
		l3p3Balls[i].swingTimer = phases[i];
		l3p3Balls[i].ballAngle = 0.0f;
		l3p3Balls[i].hitCooldown = 0;
	}
}

static void GetL3P3BallCenter(int i, float& bx, float& by) {
	bx = l3p3Balls[i].anchorX + (float)sin(l3p3Balls[i].ballAngle) * l3p3Balls[i].chainLength;
	by = l3p3Balls[i].anchorY - (float)cos(l3p3Balls[i].ballAngle) * l3p3Balls[i].chainLength;
}

// Same closest-point-on-rectangle distance check as the Level 2 ball.
static bool L3P3BallHitsPlayer(float bx, float by) {
	int currentHeight = isCrouching ? (playerHeight / 2) : playerHeight;

	float closestX = bx;
	if (closestX < playerX) closestX = (float)playerX;
	if (closestX > playerX + playerWidth) closestX = (float)(playerX + playerWidth);

	float closestY = by;
	if (closestY < playerY) closestY = (float)playerY;
	if (closestY > playerY + currentHeight) closestY = (float)(playerY + currentHeight);

	float dx = bx - closestX;
	float dy = by - closestY;
	return (dx * dx + dy * dy) < (L3P3_BALL_RADIUS * L3P3_BALL_RADIUS);
}

static void UpdateLevel3Part3Balls(void) {
	for (int i = 0; i < L3P3_NUM_BALLS; i++) {
		L3P3Ball& b = l3p3Balls[i];
		b.swingTimer += STEP * b.swingSpeed; // reuses the same STEP as the Level 2 ball
		b.ballAngle = (float)sin(b.swingTimer) * b.swingRange;

		if (b.hitCooldown > 0) b.hitCooldown--;

		float bx, by;
		GetL3P3BallCenter(i, bx, by);

		if (b.hitCooldown == 0 && L3P3BallHitsPlayer(bx, by)) {
			playerHealth -= L3P3_BALL_HIT_DAMAGE;
			if (playerHealth < 0) playerHealth = 0;
			b.hitCooldown = L3P3_BALL_HIT_COOLDOWN_FRAMES;
			PlayObstacleHitSound();
		}
	}
}

static void RenderLevel3Part3Balls(void) {
	for (int i = 0; i < L3P3_NUM_BALLS; i++) {
		float bx, by;
		GetL3P3BallCenter(i, bx, by);

		iSetColor(70, 70, 70);
		iLine(l3p3Balls[i].anchorX, l3p3Balls[i].anchorY, bx, by);

		iSetColor(90, 90, 90);
		iFilledRectangle(l3p3Balls[i].anchorX - 16, l3p3Balls[i].anchorY - 6, 32, 12);

		iSetColor(0, 0, 0);
		iFilledCircle(bx, by, L3P3_BALL_RADIUS);

		iSetColor(230, 230, 230);
		iCircle(bx, by, L3P3_BALL_RADIUS);
	}
}

// ---- Spikey saw: sweeps horizontally near the ground; jump to clear it ----
static const int SAW_DAMAGE = 10;
static const int SAW_HIT_COOLDOWN_FRAMES = 30;
static const float SAW_SPEED = 7.0f;
static const float SAW_SIZE = 70.0f; // width and height of its square hitbox
static const int SAW_PAUSE_FRAMES = 220; // gap between passes, so it's not constant

static float sawX = 0.0f;
static bool sawActive = false;
static int sawPauseTimer = SAW_PAUSE_FRAMES;
static int sawHitCooldown = 0;
static float sawSpinAngle = 0.0f; // purely visual spin

static bool RectOverlap(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
	return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

static void InitLevel3Part3Saw(void) {
	sawActive = false;
	sawPauseTimer = SAW_PAUSE_FRAMES;
	sawHitCooldown = 0;
	sawSpinAngle = 0.0f;
	sawX = -SAW_SIZE;
}

static void UpdateLevel3Part3Saw(void) {
	sawSpinAngle += 12.0f; // spins whether or not it's currently active

	if (!sawActive) {
		if (sawPauseTimer > 0) {
			sawPauseTimer--;
			return;
		}
		sawActive = true;
		sawX = -SAW_SIZE;
		return;
	}

	sawX += SAW_SPEED;
	if (sawHitCooldown > 0) sawHitCooldown--;

	int currentHeight = isCrouching ? (playerHeight / 2) : playerHeight;
	if (sawHitCooldown == 0 &&
		RectOverlap(sawX, (float)GROUND_Y, SAW_SIZE, SAW_SIZE,
			(float)playerX, (float)playerY, (float)playerWidth, (float)currentHeight)) {
		playerHealth -= SAW_DAMAGE;
		if (playerHealth < 0) playerHealth = 0;
		sawHitCooldown = SAW_HIT_COOLDOWN_FRAMES;
	}

	if (sawX > SCREEN_WIDTH) {
		sawActive = false;
		sawPauseTimer = SAW_PAUSE_FRAMES;
	}
}

static void RenderLevel3Part3Saw(void) {
	if (!sawActive) return;

	float cx = sawX + SAW_SIZE / 2.0f;
	float cy = GROUND_Y + SAW_SIZE / 2.0f;
	float radius = SAW_SIZE / 2.0f;

	iSetColor(170, 170, 175);
	iFilledCircle(cx, cy, radius * 0.65f);
	iSetColor(50, 50, 55);
	iCircle(cx, cy, radius * 0.65f);

	// Triangular spikes around the rim, rotating over time so the blade
	// visibly spins as it sweeps across the screen.
	const int TEETH = 8;
	for (int t = 0; t < TEETH; t++) {
		float angleDeg = sawSpinAngle + t * (360.0f / TEETH);
		float angle = angleDeg * 3.14159265f / 180.0f;
		float baseAngle1 = angle - 0.18f;
		float baseAngle2 = angle + 0.18f;

		double px[3], py[3];
		px[0] = cx + cos(baseAngle1) * radius * 0.65;
		py[0] = cy + sin(baseAngle1) * radius * 0.65;
		px[1] = cx + cos(baseAngle2) * radius * 0.65;
		py[1] = cy + sin(baseAngle2) * radius * 0.65;
		px[2] = cx + cos(angle) * radius * 1.3;
		py[2] = cy + sin(angle) * radius * 1.3;

		iSetColor(190, 25, 25);
		iFilledPolygon(px, py, 3);
	}
}

void InitObstacle(void) {
	// Level 2 hazard reset (unchanged)
	swingTimer = 0.0f;
	ballAngle = 0.0f;
	hitCooldown = 0;

	// Level 3 Part 3 hazards reset
	InitLevel3Part3Balls();
	InitLevel3Part3Saw();
}

static void GetBallCenter(float& bx, float& by) {
	bx = ANCHOR_X + (float)sin(ballAngle) * CHAIN_LENGTH;
	by = ANCHOR_Y - (float)cos(ballAngle) * CHAIN_LENGTH;
}


static bool BallHitsPlayer(float bx, float by) {
	int currentHeight = isCrouching ? (playerHeight / 2) : playerHeight;

	float closestX = bx;
	if (closestX < playerX) closestX = (float)playerX;
	if (closestX > playerX + playerWidth) closestX = (float)(playerX + playerWidth);

	float closestY = by;
	if (closestY < playerY) closestY = (float)playerY;
	if (closestY > playerY + currentHeight) closestY = (float)(playerY + currentHeight);

	float dx = bx - closestX;
	float dy = by - closestY;
	return (dx * dx + dy * dy) < (BALL_RADIUS * BALL_RADIUS);
}

void UpdateObstacle(void) {
	if (currentLevel == 2) {
		swingTimer += STEP * SWING_SPEED;
		ballAngle = (float)sin(swingTimer) * SWING_RANGE;

		if (hitCooldown > 0) {
			hitCooldown--;
		}

		float bx, by;
		GetBallCenter(bx, by);

		// Only the player can be damaged here - see the header comment for
		// why that's structurally guaranteed rather than just a convention.
		if (hitCooldown == 0 && BallHitsPlayer(bx, by)) {
			playerHealth -= HIT_DAMAGE;
			if (playerHealth < 0) playerHealth = 0;
			hitCooldown = HIT_COOLDOWN_FRAMES;
			PlayObstacleHitSound();
		}
		return;
	}

	if (currentLevel == 3 && currentPart == 3) {
		UpdateLevel3Part3Balls();
		UpdateLevel3Part3Saw();
		return;
	}
}

void RenderObstacle(void) {
	if (currentLevel == 2) {
		float bx, by;
		GetBallCenter(bx, by);

		// Chain
		iSetColor(70, 70, 70);
		iLine(ANCHOR_X, ANCHOR_Y, bx, by);

		// Anchor bracket
		iSetColor(90, 90, 90);
		iFilledRectangle(ANCHOR_X - 18, ANCHOR_Y - 6, 36, 12);

		// The ball itself - solid black, as requested.
		iSetColor(0, 0, 0);
		iFilledCircle(bx, by, BALL_RADIUS);

		// Thin light outline so it still reads clearly against a dark background.
		iSetColor(230, 230, 230);
		iCircle(bx, by, BALL_RADIUS);
		return;
	}

	if (currentLevel == 3 && currentPart == 3) {
		RenderLevel3Part3Balls();
		RenderLevel3Part3Saw();
		return;
	}
}

