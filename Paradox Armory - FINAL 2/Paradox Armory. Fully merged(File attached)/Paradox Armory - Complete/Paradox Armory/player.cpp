#include "player.h"
#include "iGraphics.h"
#include "gameState.h" // Level 3 merge: currentLevel, currentPart


// Player Position & Dimensions
int playerX = 200;
int playerY = GROUND_Y;
int playerWidth = 100;
int playerHeight = 150;
int playerHealth = 10000;
int playerMaxHealth = 10000;
bool playerFacingRight = true;
bool isCrouching = false;

// Mouse State
int mouseX = 0;
int mouseY = 0;

// Jump Mechanics
bool isJumping = false;
int jumpSpeed = 25;
int gravity = 1;
int currentJumpSpeed = 0;

// Movement tuning
static const int PLAYER_SPEED = 15;
static int fireCooldownTimer = 0;
static const int FIRE_COOLDOWN_FRAMES = 15; // ~0.25s between shots at 60fps -> max 4 shots/sec

int bulletImg = -1;
int bulletImg2 = -1;
int bulletWidth = 15;
int bulletHeight = 6;
float gunHeightRatio = 0.5f;

int p_bx[MAX_PLAYER_BULLETS] = { 0 };
int p_by[MAX_PLAYER_BULLETS] = { 0 };
bool p_bulletDirRight[MAX_PLAYER_BULLETS] = { true };
bool p_bulletActive[MAX_PLAYER_BULLETS] = { false };

// Player Frame Animations
int walkFramesRightSet1[7];   // Level 1 character
int walkFramesLeftSet1[7];
int walkFramesRightSet2[7];   // Level 2 character
int walkFramesLeftSet2[7];

int walkFramesRightSet3[10];
int walkFramesLeftSet3[9];

int* walkFramesRight = walkFramesRightSet1;   // active set (starts on level 1)
int* walkFramesLeft = walkFramesLeftSet1;

int playerFrameIndex = 0;
bool isPlayerMoving = false;
int playerAnimTimer = 0;

// ---------------------------------------------------------------------
// Level 4 player: Naruto. VISUAL ONLY -- position, speed, jump, crouch,
// collision box, health, gun height and the bullet pool are all the same
// variables/logic as before; only the picture drawn for the player changes.
// Frames live in images\naruto_*.png (right-facing "_r", left-facing "_l").
// ---------------------------------------------------------------------
#define NARUTO_IDLE_FRAMES 4
#define NARUTO_WALK_FRAMES 7      // same 7-step cycle that playerFrameIndex already runs
#define NARUTO_IDLE_TICKS  14     // ticks per idle (breathing) frame
#define NARUTO_SHOOT_TICKS 10     // how long the firing pose / recoil / muzzle flash lasts
static bool narutoActive = false;                 // true only while Level 4 is the active character
static int  narutoIdleR[NARUTO_IDLE_FRAMES];
static int  narutoIdleL[NARUTO_IDLE_FRAMES];
static int  narutoWalkR[NARUTO_WALK_FRAMES];
static int  narutoWalkL[NARUTO_WALK_FRAMES];
static int  narutoShootR = 0, narutoShootL = 0;   // standing firing pose (recoil)
static int  narutoCrouchR = 0, narutoCrouchL = 0;
static int  narutoFlash = 0;                      // muzzle flash overlay
static int  narutoIdleTimer = 0;
static int  narutoIdleFrame = 0;
static int  narutoShootTimer = 0;                 // >0 while the shooting animation is playing

static void LoadNarutoAssets(void) {
	char path[64];
	for (int i = 0; i < NARUTO_IDLE_FRAMES; i++) {
		sprintf(path, "images\\naruto_idle_%d_r.png", i);
		narutoIdleR[i] = iLoadImage(path);
		sprintf(path, "images\\naruto_idle_%d_l.png", i);
		narutoIdleL[i] = iLoadImage(path);
	}
	for (int i = 0; i < NARUTO_WALK_FRAMES; i++) {
		sprintf(path, "images\\naruto_walk_%d_r.png", i);
		narutoWalkR[i] = iLoadImage(path);
		sprintf(path, "images\\naruto_walk_%d_l.png", i);
		narutoWalkL[i] = iLoadImage(path);
	}
	narutoShootR  = iLoadImage("images\\naruto_shoot_r.png");
	narutoShootL  = iLoadImage("images\\naruto_shoot_l.png");
	narutoCrouchR = iLoadImage("images\\naruto_crouch_r.png");
	narutoCrouchL = iLoadImage("images\\naruto_crouch_l.png");
	narutoFlash   = iLoadImage("images\\naruto_flash.png");
}

void LoadPlayerAssets(void) {
	bulletImg = iLoadImage("images\\bullet.png");
	bulletImg2 = iLoadImage("images\\bullet2.png");

	// --- Level 1 character (unchanged paths/frames) ---
	walkFramesRightSet1[0] = iLoadImage("images\\ch1.png");
	walkFramesRightSet1[1] = iLoadImage("images\\ch2.png");
	walkFramesRightSet1[2] = iLoadImage("images\\ch3.png");
	walkFramesRightSet1[3] = iLoadImage("images\\ch4.png");
	walkFramesRightSet1[4] = iLoadImage("images\\ch5.png");
	walkFramesRightSet1[5] = iLoadImage("images\\ch6.png");
	walkFramesRightSet1[6] = iLoadImage("images\\ch7.png");

	walkFramesLeftSet1[0] = iLoadImage("images\\ch_b1.png");
	walkFramesLeftSet1[1] = iLoadImage("images\\ch_b2.png");
	walkFramesLeftSet1[2] = iLoadImage("images\\ch_b3.png");
	walkFramesLeftSet1[3] = iLoadImage("images\\ch_b4.png");
	walkFramesLeftSet1[4] = iLoadImage("images\\ch_b5.png");
	walkFramesLeftSet1[5] = iLoadImage("images\\ch_b6.png");
	walkFramesLeftSet1[6] = iLoadImage("images\\ch_b7.png");

	// --- Level 2 character ---
	walkFramesRightSet2[0] = iLoadImage("images\\ch2_1.png");
	walkFramesRightSet2[1] = iLoadImage("images\\ch2_2.png");
	walkFramesRightSet2[2] = iLoadImage("images\\ch2_3.png");
	walkFramesRightSet2[3] = iLoadImage("images\\ch2_4.png");
	walkFramesRightSet2[4] = iLoadImage("images\\ch2_5.png");
	walkFramesRightSet2[5] = iLoadImage("images\\ch2_6.png");
	walkFramesRightSet2[6] = iLoadImage("images\\ch2_7.png");

	walkFramesLeftSet2[0] = iLoadImage("images\\ch2_b1.png");
	walkFramesLeftSet2[1] = iLoadImage("images\\ch2_b2.png");
	walkFramesLeftSet2[2] = iLoadImage("images\\ch2_b3.png");
	walkFramesLeftSet2[3] = iLoadImage("images\\ch2_b4.png");
	walkFramesLeftSet2[4] = iLoadImage("images\\ch2_b5.png");
	walkFramesLeftSet2[5] = iLoadImage("images\\ch2_b6.png");
	walkFramesLeftSet2[6] = iLoadImage("images\\ch2_b7.png");


	walkFramesRightSet3[0] = iLoadImage("images\\ch3_1.png");
	walkFramesRightSet3[1] = iLoadImage("images\\ch3_2.png");
	walkFramesRightSet3[2] = iLoadImage("images\\ch3_3.png");
	walkFramesRightSet3[3] = iLoadImage("images\\ch3_4.png");
	walkFramesRightSet3[4] = iLoadImage("images\\ch3_5.png");
	walkFramesRightSet3[5] = iLoadImage("images\\ch3_6.png");
	walkFramesRightSet3[6] = iLoadImage("images\\ch3_7.png");
	walkFramesRightSet3[7] = iLoadImage("images\\ch3_8.png");
	walkFramesRightSet3[8] = iLoadImage("images\\ch3_9.png");
	walkFramesRightSet3[9] = iLoadImage("images\\ch3_10.png");
	

	walkFramesLeftSet3[0] = iLoadImage("images\\ch3_b1.png");
	walkFramesLeftSet3[1] = iLoadImage("images\\ch3_b2.png");
	walkFramesLeftSet3[2] = iLoadImage("images\\ch3_b3.png");
	walkFramesLeftSet3[3] = iLoadImage("images\\ch3_b4.png");
	walkFramesLeftSet3[4] = iLoadImage("images\\ch3_b5.png");
	walkFramesLeftSet3[5] = iLoadImage("images\\ch3_b6.png");
	walkFramesLeftSet3[6] = iLoadImage("images\\ch3_b7.png");
	walkFramesLeftSet3[7] = iLoadImage("images\\ch3_b8.png");
	walkFramesLeftSet3[8] = iLoadImage("images\\ch3_b9.png");

	// --- Level 4 character (Naruto) ---
	LoadNarutoAssets();
	

	
 }

void SetPlayerCharacter(int level) {
	narutoActive = (level == 4);   // Level 4 draws Naruto; every other level is unchanged
	narutoIdleTimer = 0;
	narutoIdleFrame = 0;
	narutoShootTimer = 0;
	if (level == 2) {
		walkFramesRight = walkFramesRightSet2;
		walkFramesLeft = walkFramesLeftSet2;
		bulletImg = bulletImg2;
		gunHeightRatio = 0.63f;
		bulletWidth = 20;
		bulletHeight = 8;
	}
	else if (level == 3) {
		walkFramesRight = walkFramesRightSet3;
		walkFramesLeft = walkFramesLeftSet3;
		bulletImg = bulletImg2; // reuse Level 2's bullet, or load a new one if you have one
		gunHeightRatio = 0.63f; // tweak if your sprite's gun sits at a different height
		bulletWidth = 20;
		bulletHeight = 8;
	}
	else if (level == 4) {
		// Naruto is drawn by DrawNarutoPlayer(); these keep the old pointers valid.
		// Gun/bullet values are the SAME ones Level 4 got before (level-2 set).
		walkFramesRight = walkFramesRightSet2;
		walkFramesLeft = walkFramesLeftSet2;
		bulletImg = bulletImg2;
		gunHeightRatio = 0.63f;
		bulletWidth = 20;
		bulletHeight = 8;
	}
	else {
		walkFramesRight = walkFramesRightSet1;
		walkFramesLeft = walkFramesLeftSet1;
		bulletImg = iLoadImage("images\\bullet.png");
		gunHeightRatio = 0.5f;
		bulletWidth = 15;
		bulletHeight = 6;
	}
}

// Fires a bullet from the current gun position, respecting the bullet pool.
static void FirePlayerBullet(void) {
	for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
		if (!p_bulletActive[i]) {
			p_bulletActive[i] = true;
			narutoShootTimer = NARUTO_SHOOT_TICKS;   // visual only: starts Naruto's firing animation
			p_bulletDirRight[i] = playerFacingRight;
			p_bx[i] = playerFacingRight ? (playerX + playerWidth) : playerX;
			int currentHeight = isCrouching ? (playerHeight / 2) : playerHeight;
			p_by[i] = playerY + (int)(currentHeight * gunHeightRatio);
			break;
		}
	}
}

// Level 3 Part 1 merge: floating brick platforms. Edit x/y/width freely to
// place them — y is the platform's TOP surface height (same axis as GROUND_Y,
// bigger y = higher up).



// Returns the floor height directly under the player right now: a platform's
// top if they're standing over one, otherwise the normal GROUND_Y.
static int GetFloorYUnderPlayer(void) {
	int floorY = GROUND_Y;

	if (currentLevel == 3 && currentPart == 1) {
		int centerX = playerX + playerWidth / 2;
		for (int i = 0; i < NUM_L3P1_PLATFORMS; i++) {
			const Platform& p = level3Part1Platforms[i];
			if (centerX >= p.x && centerX <= p.x + p.width) {
				// Only counts as ground if the player is at/above it, so
				// jumping up from underneath isn't blocked.
				if (playerY >= p.y && p.y > floorY) {
					floorY = p.y;
				}
			}
		}
	}
	return floorY;
}

// Level 3 Part 1 merge: draws the brick platforms. Call this from renderGameplay
// (ui.h), BEFORE RenderPlayer(), so the player draws on top of them.
void RenderLevel3Part1Platforms(void) {
	if (!(currentLevel == 3 && currentPart == 1)) return;

	for (int i = 0; i < NUM_L3P1_PLATFORMS; i++) {
		const Platform& p = level3Part1Platforms[i];
		const int brickH = 40;

		iSetColor(150, 75, 30);
		iFilledRectangle(p.x, p.y - brickH, p.width, brickH);

		iSetColor(90, 40, 15);
		iRectangle(p.x, p.y - brickH, p.width, brickH);
		for (int seg = brickH / 2; seg < p.width; seg += brickH) {
			iLine(p.x + seg, p.y - brickH, p.x + seg, p.y);
		}
	}
}

// Continuous (held-key) input, polled every frame via GetAsyncKeyState.
// This is what makes movement smooth while a key is held, instead of
// relying on the OS key-repeat behavior of the iKeyboard() callback.
static void HandlePlayerRealtimeInput(void) {
	if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
		exit(0);
	}

	bool movingNow = false;

	if (GetAsyncKeyState('A') & 0x8000) {
		playerX -= PLAYER_SPEED;
		playerFacingRight = false;
		movingNow = true;
	}
	if (GetAsyncKeyState('D') & 0x8000) {
		playerX += PLAYER_SPEED;
		playerFacingRight = true;
		movingNow = true;
	}

	if (playerX < 0) playerX = 0;
	if (playerX + playerWidth > SCREEN_WIDTH) playerX = SCREEN_WIDTH - playerWidth;

	isPlayerMoving = movingNow;

	int floorY = GetFloorYUnderPlayer(); // Level 3 merge: platform top or GROUND_Y

	// W = jump (only start a new jump if standing on solid ground/platform)
	if ((GetAsyncKeyState('W') & 0x8000) && !isJumping && playerY <= floorY) {
		isJumping = true;
		currentJumpSpeed = jumpSpeed;
	}

	// S = crouch while held
	isCrouching = (GetAsyncKeyState('S') & 0x8000) != 0;

	// Level 3 merge: walked off a platform edge while not jumping -> fall.
	if (!isJumping && playerY > floorY) {
		isJumping = true;
		currentJumpSpeed = 0; // fall from rest, no upward pop
	}

	// Jump Physics
	if (isJumping) {
		playerY += currentJumpSpeed;
		currentJumpSpeed -= gravity;

		if (playerY <= floorY) {
			playerY = floorY;
			isJumping = false;
			currentJumpSpeed = 0;
		}
	}
}

static void UpdatePlayerBullets(void) {
	for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
		if (p_bulletActive[i]) {
			int speed = 12;
			if (p_bulletDirRight[i]) {
				p_bx[i] += speed;
				if (p_bx[i] > SCREEN_WIDTH) p_bulletActive[i] = false;
			}
			else {
				p_bx[i] -= speed;
				if (p_bx[i] < 0) p_bulletActive[i] = false;
			}
		}
	}
}

static void UpdatePlayerAnimation(void) {
	if (isPlayerMoving) {
		playerAnimTimer++;
		if (playerAnimTimer >= 3) {
			playerAnimTimer = 0;
			playerFrameIndex = (playerFrameIndex + 1) % 7;
		}
	}
	else {
		playerFrameIndex = 0;
		playerAnimTimer = 0;
	}
}

// Naruto idle-breathing and shooting-animation timers (visual only).
static void UpdateNarutoAnimation(void) {
	if (!narutoActive) return;
	if (narutoShootTimer > 0) narutoShootTimer--;
	if (isPlayerMoving) {
		narutoIdleTimer = 0;
		narutoIdleFrame = 0;
	}
	else {
		narutoIdleTimer++;
		if (narutoIdleTimer >= NARUTO_IDLE_TICKS) {
			narutoIdleTimer = 0;
			narutoIdleFrame = (narutoIdleFrame + 1) % NARUTO_IDLE_FRAMES;
		}
	}
}

// Per-frame player update, called from fixedUpdate().
void updatePlayer(void) {
	HandlePlayerRealtimeInput();
	UpdatePlayerBullets();
	UpdatePlayerAnimation();
	UpdateNarutoAnimation();
	if (fireCooldownTimer > 0) fireCooldownTimer--;
}

// Draws the Naruto player into the SAME box the old sprite used
// (playerX, playerY, playerWidth x currentHeight), so collision/size/ground
// position are untouched. Picks a frame from the existing state variables.
static void DrawNarutoPlayer(int currentHeight) {
	int img;
	bool shooting = (narutoShootTimer > 0);
	bool poseHasRecoil = false;

	if (isCrouching) {
		img = playerFacingRight ? narutoCrouchR : narutoCrouchL;
	}
	else if (shooting && !isPlayerMoving) {
		img = playerFacingRight ? narutoShootR : narutoShootL;   // dedicated recoil pose
		poseHasRecoil = true;
	}
	else if (isPlayerMoving) {
		int f = playerFrameIndex % NARUTO_WALK_FRAMES;
		img = playerFacingRight ? narutoWalkR[f] : narutoWalkL[f];
	}
	else {
		img = playerFacingRight ? narutoIdleR[narutoIdleFrame] : narutoIdleL[narutoIdleFrame];
	}

	// Small visual kick backwards while firing on the move / crouched.
	int kick = 0;
	if (shooting && !poseHasRecoil) kick = (narutoShootTimer * 4) / NARUTO_SHOOT_TICKS;
	int drawX = playerFacingRight ? (playerX - kick) : (playerX + kick);

	if (img > 0) {
		iShowImage(drawX, playerY, playerWidth, currentHeight, img);
	}
	else {
		iSetColor(255, 140, 0);
		iFilledRectangle(drawX, playerY, playerWidth, currentHeight);
	}

	// Muzzle flash at the end of the gun for the first few ticks after a shot.
	if (shooting && narutoShootTimer > NARUTO_SHOOT_TICKS - 5 && narutoFlash > 0) {
		int size = (narutoShootTimer > NARUTO_SHOOT_TICKS - 3) ? 34 : 24;
		int muzzle = poseHasRecoil ? (playerWidth * 220 / 240) : (isCrouching ? (playerWidth * 224 / 240) : (playerWidth * 236 / 240));
		int mx = playerFacingRight ? (drawX + muzzle) : (drawX + playerWidth - muzzle);
		int my = playerY + (int)(currentHeight * gunHeightRatio) + bulletHeight / 2;
		iShowImage(mx - size / 2, my - size / 2, size, size, narutoFlash);
	}
}

void RenderPlayer(void) {
	int frame = isPlayerMoving ? (playerFrameIndex % 7) : 0;
	int currentHeight = isCrouching ? (playerHeight / 2) : playerHeight;

	if (narutoActive) {
		DrawNarutoPlayer(currentHeight);   // Level 4: Naruto (same box, same state variables)
	}
	else if (playerFacingRight) {
		if (walkFramesRight[frame] > 0) {
			iShowImage(playerX, playerY, playerWidth, currentHeight, walkFramesRight[frame]);
		}
		else {
			iSetColor(0, 150, 255);
			iFilledRectangle(playerX, playerY, playerWidth, currentHeight);
		}
	}
	else {
		if (walkFramesLeft[frame] > 0) {
			iShowImage(playerX, playerY, playerWidth, currentHeight, walkFramesLeft[frame]);
		}
		else {
			iSetColor(0, 150, 255);
			iFilledRectangle(playerX, playerY, playerWidth, currentHeight);
		}
	}

	// Draw Player Bullets
	for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
		if (p_bulletActive[i]) {
			if (bulletImg > 0) {
				iShowImage(p_bx[i], p_by[i], bulletWidth, bulletHeight, bulletImg);
			}
			else {
				iSetColor(255, 255, 0);
				iFilledRectangle(p_bx[i], p_by[i], bulletWidth, bulletHeight);
			}
		}
	}
}

// One-time (discrete) key actions only. Continuous movement/jump/crouch
// live in HandlePlayerRealtimeInput() via GetAsyncKeyState instead, since
// relying on iKeyboard()'s OS key-repeat for movement is what made the
// player feel unresponsive / not move correctly while a key was held.
void HandlePlayerKeyboardInput(unsigned char key) {
	// ESC = exit
	if (key == 27) {
		exit(0);
	}

	// F Key Shoot (kept alongside left-mouse shooting)
	if (key == 'f' || key == 'F') {
		FirePlayerBullet();
	}
}

void HandlePlayerMouseInput(int button, int state, int mx, int my) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		if (fireCooldownTimer <= 0) {
			FirePlayerBullet();
			fireCooldownTimer = FIRE_COOLDOWN_FRAMES;
		}
	}
}

void HandlePlayerMouseMove(int mx, int my) {
	mouseX = mx;
	mouseY = my;
}
