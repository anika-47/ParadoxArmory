// =====================================================================
// LEVEL 4 - "PARADOX GROVE"
// =====================================================================
// See level4.h and LEVEL4_NOTES.md for the design overview.
//
// HARD RULE OBSERVED THROUGHOUT THIS FILE:
//   player.cpp / player.h are NOT modified. A/D/W/S/F/left-mouse/ESC and
//   the GetAsyncKeyState realtime polling, PLAYER_SPEED, gravity, jump,
//   crouch, animation and the bullet pool are all untouched. This module
//   runs AFTER updatePlayer() each frame and only:
//     - reads  playerX/playerY/isCrouching/playerFacingRight and the
//              player bullet pool (p_bx/p_by/p_bulletActive)
//     - writes playerX  (scrolling + push-out of solid objects)
//     - writes playerHealth, and ONLY through L4DamagePlayer() and the
//              checkpoint heal in L4OpenGate()
// =====================================================================

#include "level4.h"
#include "gameState.h"
#include "player.h"
#include "variable.h"
#include "defines.h"
#include "puzzle4.h"
#include "audio4.h"
#include "iGraphics.h"

#include <math.h>
#include <stdio.h>

// Sprite handles owned by enemy.cpp (loaded once by LoadEnemyAssets()).
// Declared extern here so Level 4 can reuse the art that already ships
// with the project instead of requiring new asset files.
extern int enemyPic[6];
extern int tankImg;
extern int enemyBulletImage;

int backgroundImg4 = 0;

// =====================================================================
// 1. TUNABLES
// =====================================================================

// --- world layout ---
static const int   L4_WORLD_WIDTH  = 9600;   // 8 sections x 1200px
static const int   L4_SECTION_W    = 1200;
static const int   L4_NUM_SECTIONS = 8;

// Screen x positions that drag the camera. The player sprite is parked
// between these two anchors while the world scrolls underneath.
static const int   L4_ANCHOR_RIGHT = 700;
static const int   L4_ANCHOR_LEFT  = 380;

// --- player ---
static const int   L4_PLAYER_MAX_HP = 20000;   // Naruto mod: player health (max and starting) = 20000
static const int   L4_IFRAMES       = 60;    // ~0.96s of invulnerability
static const int   L4_BULLET_DAMAGE = 20;

// --- enemy type ids ---
#define L4_CHASER 0
#define L4_RANGED 1
#define L4_FAST   2
#define L4_HEAVY  3

// --- enemy stats (health values follow the level brief) ---
static const int L4_HP_CHASER = 130;   // brief: 100-150
static const int L4_HP_RANGED = 160;   // brief: 120-180
static const int L4_HP_FAST   = 100;   // brief:  80-120
static const int L4_HP_HEAVY  = 800;   // brief: 500-800

static const int L4_DMG_CHASER_TOUCH = 55;
static const int L4_DMG_FAST_TOUCH   = 45;
static const int L4_DMG_RANGED_SHOT  = 60;
static const int L4_DMG_BOSS_TOUCH   = 120;
static const int L4_DMG_BOSS_SHOT    = 80;
static const int L4_DMG_SHOCKWAVE    = 100;

// --- obstacle types ---
#define L4_OB_SOLID   0   // stone block: blocks the path, jump over it
#define L4_OB_LOWBAR  1   // overhead beam: blocks a STANDING player, crouch under it
#define L4_OB_MOVER   2   // moving solid slab: blocks and hurts
#define L4_OB_SPIKE   3   // ground hazard: hurts, not solid
#define L4_OB_BLADE   4   // swinging hazard: hurts, not solid
#define L4_OB_GATE    5   // section gate: solid until the section is cleared
#define L4_OB_WISP    6   // final approach: chakra wisp flying a figure-eight patrol (non-solid)
#define L4_OB_PYLON   7   // final approach: seal pylon whose energy barrier cycles on and off (non-solid)

static const int L4_DMG_MOVER = 90;
static const int L4_DMG_SPIKE = 60;
static const int L4_DMG_BLADE = 75;
static const int L4_DMG_WISP  = 80;
static const int L4_DMG_PYLON = 120;

// --- pool sizes (fixed arrays, nothing is allocated at runtime) ---
#define L4_MAX_ENEMIES   14
#define L4_MAX_EBULLETS  48
#define L4_MAX_OBSTACLES 64
#define L4_MAX_SPAWNS    64

// Furthest a scripted spawn may sit from its section's left edge.
static const int L4_SPAWN_MAX_OFFSET = 1040;

// =====================================================================
// 2. DATA
// =====================================================================

struct L4Enemy {
	bool  active;
	int   type;
	float x, y;          // world coords; y is the sprite's BOTTOM edge
	float vy;
	int   w, h;
	int   hp, maxHp;
	bool  facingLeft;
	int   aiState;       // meaning depends on type
	int   aiTimer;
	int   shootTimer;
	int   contactCd;
	int   animTimer, animFrame;
	int   hitFlash;
	int   spawnGrace;    // brief window where it cannot be shot yet
	int   stuckTimer;    // failsafe: frames spent unable to advance
	int   section, wave; // which wave it belongs to
};

struct L4Boss {
	bool  active;
	bool  defeated;
	bool  introPlayed;
	float x, y, vy;
	int   w, h;
	int   hp, maxHp;
	bool  facingLeft;
	int   phase;         // 1 = guard, 2 = enraged
	int   state;         // 0 advance, 1 volley, 2 charge (shielded), 3 slam
	int   stateTimer;
	int   volleyLeft;
	int   volleyGap;
	int   contactCd;
	int   hitFlash;
	int   stomp;         // frames of landing shake left
	bool  regrouped;     // has it already fallen back into section 8?
};

struct L4Bullet {
	bool  active;
	float x, y, vx, vy;
	int   w, h;
	int   damage;
	int   kind;          // 0 enemy shot, 1 boss shot, 2 ground shockwave
	int   life;
};

struct L4Obstacle {
	bool  active;
	int   type;
	float x, y;          // CURRENT world position, bottom-left corner
	int   w, h;
	float baseX, baseY;  // anchor for movers / pivot for blades
	float range;         // travel amplitude (or chain length for blades)
	float speed;         // radians per frame
	float phase;         // current oscillation angle
	int   axis;          // 1 = horizontal, 2 = vertical
	bool  solid;
	int   damage;
	int   section;       // owning section (used by gates)
	float rangeY;        // wisp: vertical amplitude of the figure-eight
	int   timer;         // pylon: position inside its on/off cycle (ticks)
};

// A single scripted spawn: "at <delay> frames into <section>/<wave>,
// place a <type> enemy at <offsetX> from the section's left edge."
struct L4SpawnDef {
	int section;
	int wave;
	int type;
	int offsetX;
	int delay;
};

static L4Enemy    l4Enemies[L4_MAX_ENEMIES];
static L4Boss     l4Boss;
static L4Bullet   l4Bullets[L4_MAX_EBULLETS];
static L4Obstacle l4Obstacles[L4_MAX_OBSTACLES];
static int        l4ObstacleCount = 0;

// --- world / progression state ---
static float l4CamX          = 0.0f;
static int   l4Section       = 1;   // 1..8
static int   l4Wave          = 1;
static int   l4WaveStarted   = 0;
static int   l4WaveTimer     = 0;
static int   l4SectionCleared[L4_NUM_SECTIONS + 1];
static bool  l4SpawnFired[L4_MAX_SPAWNS];
static int   l4Reinforcements = 0;
static int   l4ReinforceTimer = 0;

// --- player-side state owned by this level ---
static int   l4PlayerInvuln  = 0;
static int   l4HurtFlash     = 0;
static int   l4Score         = 0;
static int   l4Frames        = 0;   // elapsed level time, in ticks
static int   l4ShootSfxCd    = 0;
static int   l4PrevActiveBullets = 0;
static bool  l4PuzzleStarted = false;
static bool  l4SealAwake     = false;   // the Warden has fallen: the final approach is live
static int   l4SealTimer     = 0;       // ticks since the seal awoke
static int   l4BannerTimer   = 0;
static char  l4BannerText[64];

// =====================================================================
// 3. THE SPAWN SCRIPT
// =====================================================================
// Controlled waves, never "everything at once". Each section holds one or
// two waves; the gate at the end of a section only opens once every enemy
// belonging to that section's waves is dead.
//
//   section 1 : no enemies  - obstacle tutorial
//   section 2 : chasers (3 waves)
//   section 3 : ranged (+chasers) (3 waves)
//   section 4 : fast (+ranged) (3 waves)
//   section 5 : all three mixed (3 waves)
//   section 6 : the gauntlet - obstacles carry the pressure (3 waves)
//   section 7 : mini-boss phase 1 + chaser/ranged escorts (3 waves)
//   section 8 : mini-boss phase 2 + fast/ranged escorts (3 waves)
//
static const L4SpawnDef L4_SPAWNS[] = {
	// sec wave type       offsetX delay
	// NOTE: offsetX must stay below L4_SPAWN_MAX_OFFSET - the section gate
	// sits at +1140 and blocks enemies too, so anything spawned past it
	// could never reach the player and the wave could never complete.

	// --- SECTION 2: THE AMBUSH - pure chasers, learn the lunge/recoil beat
	{ 2, 1, L4_CHASER,      880,    30 },
	{ 2, 1, L4_CHASER,     1020,   110 },
	{ 2, 2, L4_CHASER,      960,    30 },
	{ 2, 2, L4_CHASER,      300,   130 },   // flanks from behind
	{ 2, 3, L4_CHASER,      900,    40 },
	{ 2, 3, L4_CHASER,     1030,   120 },
	{ 2, 3, L4_CHASER,      340,   210 },

	// --- SECTION 3: CROSSFIRE - ranged lanes, one chaser to break cover
	{ 3, 1, L4_RANGED,      900,    30 },
	{ 3, 1, L4_RANGED,     1030,   120 },
	{ 3, 2, L4_RANGED,      980,    30 },
	{ 3, 2, L4_CHASER,      320,    90 },
	{ 3, 3, L4_RANGED,      940,    40 },
	{ 3, 3, L4_RANGED,      300,   110 },
	{ 3, 3, L4_CHASER,     1020,   180 },

	// --- SECTION 4: SWIFT HUNT - dashers, plus a ranged anchor
	{ 4, 1, L4_FAST,        880,    20 },
	{ 4, 1, L4_FAST,       1010,    80 },
	{ 4, 1, L4_FAST,        280,   150 },
	{ 4, 2, L4_FAST,        940,    20 },
	{ 4, 2, L4_FAST,       1030,    90 },
	{ 4, 2, L4_RANGED,      330,    40 },
	{ 4, 3, L4_FAST,        900,    30 },
	{ 4, 3, L4_FAST,        320,   110 },
	{ 4, 3, L4_FAST,       1000,   180 },
	{ 4, 3, L4_RANGED,     1040,    40 },

	// --- SECTION 5: MIXED ASSAULT - all three types together
	{ 5, 1, L4_CHASER,      900,    20 },
	{ 5, 1, L4_RANGED,     1020,    70 },
	{ 5, 1, L4_FAST,        300,   140 },
	{ 5, 2, L4_CHASER,      860,    20 },
	{ 5, 2, L4_CHASER,     1010,    70 },
	{ 5, 2, L4_RANGED,      340,    40 },
	{ 5, 2, L4_FAST,        960,   170 },
	{ 5, 3, L4_RANGED,     1030,    30 },
	{ 5, 3, L4_FAST,        900,    90 },
	{ 5, 3, L4_FAST,        320,   150 },
	{ 5, 3, L4_CHASER,      980,   220 },

	// --- SECTION 6: THE GAUNTLET - the corridor carries the pressure
	{ 6, 1, L4_FAST,        980,    60 },
	{ 6, 1, L4_FAST,       1030,   200 },
	{ 6, 2, L4_FAST,        900,    40 },
	{ 6, 2, L4_CHASER,     1020,   130 },
	{ 6, 3, L4_FAST,        960,    40 },
	{ 6, 3, L4_CHASER,      320,   120 },

	// --- SECTION 7: HEAVY GUARD - mini-boss phase 1 + escorts
	{ 7, 1, L4_CHASER,      980,   120 },
	{ 7, 1, L4_CHASER,      400,   260 },
	{ 7, 2, L4_RANGED,     1020,    40 },
	{ 7, 2, L4_RANGED,      420,   120 },
	{ 7, 3, L4_CHASER,      960,    40 },
	{ 7, 3, L4_RANGED,      380,   140 },

	// --- SECTION 8: FINAL STAND - mini-boss phase 2 + reinforcements
	// Offsets here sit around the Warden's regroup point so FINAL STAND is
	// fought beside the boss instead of at the far wall.
	{ 8, 1, L4_FAST,        520,    60 },
	{ 8, 1, L4_FAST,        700,   170 },
	{ 8, 2, L4_RANGED,      640,    40 },
	{ 8, 2, L4_FAST,        300,   120 },
	{ 8, 2, L4_CHASER,      560,   210 },
	{ 8, 3, L4_FAST,        480,    40 },
	{ 8, 3, L4_RANGED,      720,   130 }
};
static const int L4_SPAWN_COUNT = (int)(sizeof(L4_SPAWNS) / sizeof(L4_SPAWNS[0]));

static const char* L4_SECTION_NAMES[L4_NUM_SECTIONS + 1] = {
	"",
	"OUTER PATH",
	"THE AMBUSH",
	"CROSSFIRE",
	"SWIFT HUNT",
	"MIXED ASSAULT",
	"THE GAUNTLET",
	"HEAVY GUARD",
	"FINAL STAND"
};

// =====================================================================
// 4. SMALL HELPERS
// =====================================================================

static bool L4Rect(float ax, float ay, float aw, float ah,
	float bx, float by, float bw, float bh) {
	// Standard AABB overlap - the only test used for every collision in
	// this level. Nothing here ever reacts to mere proximity.
	return (ax < bx + bw &&
		ax + aw > bx &&
		ay < by + bh &&
		ay + ah > by);
}

static int   L4PlayerH(void)      { return isCrouching ? (playerHeight / 2) : playerHeight; }
static float L4PlayerWorldX(void) { return l4CamX + (float)playerX; }
static int   L4SectionStart(int s){ return (s - 1) * L4_SECTION_W; }

int Level4CameraX(void) { return (int)l4CamX; }

// THE single place player health is ever reduced in this level. The
// i-frame gate here is what guarantees one contact can't drain the whole
// bar in a handful of frames.
static void L4DamagePlayer(int amount, bool trap) {
	if (amount <= 0) return;
	if (l4PlayerInvuln > 0) return;
	if (playerHealth <= 0) return;

	playerHealth -= amount;
	if (playerHealth < 0) playerHealth = 0;

	l4PlayerInvuln = L4_IFRAMES;
	l4HurtFlash = 14;

	if (trap) L4SfxTrapHit();
	else      L4SfxPlayerHurt();
}

static void L4Banner(const char* text) {
	int i = 0;
	while (text[i] != '\0' && i < 63) { l4BannerText[i] = text[i]; i++; }
	l4BannerText[i] = '\0';
	l4BannerTimer = 150;
}

// =====================================================================
// 5. WORLD CONSTRUCTION
// =====================================================================

static void L4AddObstacle(int type, float x, float y, int w, int h,
	int axis, float range, float speed, float phase, int section) {
	if (l4ObstacleCount >= L4_MAX_OBSTACLES) return;

	L4Obstacle& o = l4Obstacles[l4ObstacleCount++];
	o.active  = true;
	o.type    = type;
	o.x = o.baseX = x;
	o.y = o.baseY = y;
	o.w = w;
	o.h = h;
	o.axis    = axis;
	o.range   = range;
	o.speed   = speed;
	o.phase   = phase;
	o.section = section;
	o.rangeY  = 0.0f;
	o.timer   = 0;

	switch (type) {
	case L4_OB_SOLID:  o.solid = true;  o.damage = 0;            break;
	case L4_OB_LOWBAR: o.solid = true;  o.damage = 0;            break;
	case L4_OB_MOVER:  o.solid = true;  o.damage = L4_DMG_MOVER; break;
	case L4_OB_SPIKE:  o.solid = false; o.damage = L4_DMG_SPIKE; break;
	case L4_OB_BLADE:  o.solid = false; o.damage = L4_DMG_BLADE; break;
	case L4_OB_GATE:   o.solid = true;  o.damage = 0;            break;
	default:           o.solid = false; o.damage = 0;            break;
	}
}

// Gate geometry. Height 420 puts the top of the barrier well above the
// player's jump apex (GROUND_Y + 325), so a closed gate genuinely cannot
// be jumped over - required sections can't be skipped.
static void L4AddGate(int section) {
	L4AddObstacle(L4_OB_GATE, (float)(L4SectionStart(section) + L4_SECTION_W - 60),
		(float)GROUND_Y, 44, 420, 0, 0.0f, 0.0f, 0.0f, section);
}

// LOWBAR geometry note: the beam's BOTTOM edge sits at GROUND_Y + 85 = 185.
// A standing player spans 100..250 (overlaps -> blocked); a crouching
// player spans 100..175 (clears it -> passes). That is what forces the
// S key to actually matter.
static const float L4_LOWBAR_Y = (float)(GROUND_Y + 85);

// =====================================================================
// 5b. THE SEAL APPROACH  (the final area, between the Warden and the seal)
// =====================================================================
// Two NEW obstacle types live here and nowhere else:
//
//   L4_OB_WISP   a chakra wisp on a FIGURE-EIGHT patrol. It moves in two
//                dimensions and its speed swells and eases along the path
//                (fastest at the middle of the eight, slowest at the ends).
//                The whole path is drawn as a dotted guide so it can be
//                read and timed. Hits the player through a slightly SMALLER
//                box than it is drawn, never a larger one.
//
//   L4_OB_PYLON  a seal pylon whose energy barrier CYCLES:
//                DORMANT (harmless) -> CHARGING (harmless, warning) ->
//                ACTIVE (hurts, full height) -> COOLING (harmless).
//                The barrier's extent is outlined while it charges.
//
// Neither is solid, so neither can ever pin the player or block a path -
// the worst they can do is one hit per L4_IFRAMES through the same
// L4DamagePlayer() funnel as every other hazard. They stay asleep (not
// drawn, not updated, not colliding) until the Warden has fallen, so the
// mini-boss fights play exactly as before. When the seal awakens they get
// L4_SEAL_GRACE ticks during which they are visible but harmless.
//
// The puzzle no longer starts the instant the Warden dies: it starts when
// the player walks onto the seal altar at the far end of the arena.

static const float L4_ALTAR_X         = 9480.0f;   // world x of the altar's left edge
static const float L4_ALTAR_TRIGGER_X = 9470.0f;   // player's left edge must reach this
static const int   L4_SEAL_GRACE      = 100;       // ticks the hazards are visible but harmless

#define L4_PYLON_CYCLE     240
#define L4_PYLON_CHARGE_AT 110
#define L4_PYLON_ACTIVE_AT 160
#define L4_PYLON_COOL_AT   220

static void L4BuildSealApproach(void) {
	int before;

	// beat 1: a pylon in the gap between the stone block and the spikes
	before = l4ObstacleCount;
	L4AddObstacle(L4_OB_PYLON, 8845.0f, (float)GROUND_Y, 34, 430, 0, (float)L4_PYLON_CYCLE, 0.0f, 0.0f, 8);
	if (l4ObstacleCount > before) l4Obstacles[l4ObstacleCount - 1].timer = 0;

	// beat 2: the wisp patrols the open stretch after the spikes
	before = l4ObstacleCount;
	L4AddObstacle(L4_OB_WISP, 9225.0f, 195.0f, 44, 44, 0, 100.0f, 0.030f, 0.0f, 8);
	if (l4ObstacleCount > before) l4Obstacles[l4ObstacleCount - 1].rangeY = 55.0f;

	// beat 3: a second pylon, half a cycle out of step, guards the altar
	before = l4ObstacleCount;
	L4AddObstacle(L4_OB_PYLON, 9445.0f, (float)GROUND_Y, 34, 430, 0, (float)L4_PYLON_CYCLE, 0.0f, 0.0f, 8);
	if (l4ObstacleCount > before) l4Obstacles[l4ObstacleCount - 1].timer = L4_PYLON_CYCLE / 2;
}

static bool L4PylonActive(const L4Obstacle& o) {
	return (o.timer >= L4_PYLON_ACTIVE_AT && o.timer < L4_PYLON_COOL_AT);
}

static void L4UpdateSealApproach(void) {
	int i;
	if (!l4SealAwake) return;

	l4SealTimer++;

	for (i = 0; i < l4ObstacleCount; i++) {
		L4Obstacle& o = l4Obstacles[i];
		if (!o.active) continue;
		if (o.type == L4_OB_WISP) {
			// figure-eight: x follows sin(p), y follows sin(2p); the phase
			// speed varies by +-40 % so the wisp surges and eases.
			o.phase += o.speed * (1.0f + 0.40f * (float)cos(o.phase));
			o.x = o.baseX + (float)sin(o.phase) * o.range - o.w * 0.5f;
			o.y = o.baseY + (float)sin(2.0f * o.phase) * o.rangeY - o.h * 0.5f;
		}
		else if (o.type == L4_OB_PYLON) {
			o.timer = (o.timer + 1) % L4_PYLON_CYCLE;
		}
	}

	if (l4SealTimer >= L4_SEAL_GRACE) {
		float px = L4PlayerWorldX();
		float py = (float)playerY;
		float pw = (float)playerWidth;
		float ph = (float)L4PlayerH();

		for (i = 0; i < l4ObstacleCount; i++) {
			L4Obstacle& o = l4Obstacles[i];
			if (!o.active) continue;
			if (o.type == L4_OB_WISP) {
				// hit box is 6px INSIDE the drawn orb - never bigger than it looks
				if (L4Rect(px, py, pw, ph, o.x + 6.0f, o.y + 6.0f, (float)(o.w - 12), (float)(o.h - 12)))
					L4DamagePlayer(L4_DMG_WISP, true);
			}
			else if (o.type == L4_OB_PYLON && L4PylonActive(o)) {
				if (L4Rect(px, py, pw, ph, o.x + 2.0f, o.y, (float)(o.w - 4), (float)o.h))
					L4DamagePlayer(L4_DMG_PYLON, true);
			}
		}
	}

	// the altar: reaching it opens the final puzzle
	if (!l4PuzzleStarted && playerHealth > 0 && L4PlayerWorldX() >= L4_ALTAR_TRIGGER_X) {
		l4PuzzleStarted = true;
		L4MusicLevel();
		InitPuzzle4();
		gameState = STATE_PUZZLE4;
	}
}

static void L4DrawWisp(const L4Obstacle& o, bool live) {
	const float PI2 = 6.2831853f;
	int k;
	float cx = o.x + o.w * 0.5f - l4CamX;
	float cy = o.y + o.h * 0.5f;
	float t  = (float)l4SealTimer;

	// the patrol path, as a dotted guide (drawn even while harmless)
	iSetColor(live ? 150 : 90, live ? 70 : 50, live ? 20 : 20);
	for (k = 0; k < 36; k++) {
		float p  = PI2 * k / 36.0f;
		float gx = o.baseX + (float)sin(p) * o.range - l4CamX;
		float gy = o.baseY + (float)sin(2.0f * p) * o.rangeY;
		iFilledCircle(gx, gy, 2.0f);
	}
	if (cx < -80.0f || cx > (float)SCREEN_WIDTH + 80.0f) return;

	if (!live) {
		iSetColor(170, 90, 40);
		iCircle(cx, cy, 22.0f);
		iCircle(cx, cy, 13.0f);
		return;
	}

	// afterglow trail
	for (k = 4; k >= 1; k--) {
		float pp = o.phase - k * 0.085f;
		float tx = o.baseX + (float)sin(pp) * o.range - l4CamX;
		float ty = o.baseY + (float)sin(2.0f * pp) * o.rangeY;
		iSetColor(230 - k * 30, 90 - k * 12, 20);
		iFilledCircle(tx, ty, 19.0f - k * 3.0f);
	}

	// rotating flame tongues
	for (k = 0; k < 6; k++) {
		float a   = t * 0.11f + k * (PI2 / 6.0f);
		float tip = 37.0f + 5.0f * (float)sin(t * 0.3f + k * 1.7f);
		double xs[3], ys[3];
		xs[0] = cx + (float)cos(a - 0.36f) * 15.0f;  ys[0] = cy + (float)sin(a - 0.36f) * 15.0f;
		xs[1] = cx + (float)cos(a) * tip;            ys[1] = cy + (float)sin(a) * tip;
		xs[2] = cx + (float)cos(a + 0.36f) * 15.0f;  ys[2] = cy + (float)sin(a + 0.36f) * 15.0f;
		iSetColor(255, 140, 30);
		iFilledPolygon(xs, ys, 3);
	}

	// core
	iSetColor(255, 110, 30);
	iFilledCircle(cx, cy, 20.0f + 2.0f * (float)sin(t * 0.4f));
	iSetColor(255, 205, 90);
	iFilledCircle(cx, cy, 12.0f);
	iSetColor(255, 255, 235);
	iFilledCircle(cx, cy, 5.0f);

	// orbiting sparks
	iSetColor(255, 235, 140);
	for (k = 0; k < 3; k++) {
		float a = -t * 0.20f + k * (PI2 / 3.0f);
		iFilledCircle(cx + (float)cos(a) * 30.0f, cy + (float)sin(a) * 30.0f, 3.0f);
	}
}

static void L4DrawPylon(const L4Obstacle& o, bool live) {
	float sx   = o.x - l4CamX;
	float cxp  = sx + o.w * 0.5f;
	float top  = o.y + o.h;
	float low  = o.y + 22.0f;
	float high = top - 16.0f;
	float t    = (float)l4SealTimer;
	int   tm   = live ? o.timer : 0;
	int   k;
	double xs[4], ys[4];

	if (sx + o.w < -60.0f || sx > (float)SCREEN_WIDTH + 60.0f) return;

	// base block and floating emitter cap
	iSetColor(62, 72, 90);
	iFilledRectangle(cxp - 24.0f, o.y, 48.0f, 22.0f);
	xs[0] = cxp;         ys[0] = top + 16.0f;
	xs[1] = cxp + 18.0f; ys[1] = top;
	xs[2] = cxp;         ys[2] = top - 16.0f;
	xs[3] = cxp - 18.0f; ys[3] = top;
	iFilledPolygon(xs, ys, 4);
	iSetColor(0, 190, 230);
	iRectangle(cxp - 24.0f, o.y, 48.0f, 22.0f);

	if (tm < L4_PYLON_CHARGE_AT) {
		// DORMANT: a thin dim thread with slow beads - harmless
		iSetColor(20, 70, 100);
		iLine(cxp, low, cxp, high);
		iSetColor(60, 150, 200);
		for (k = 0; k < 5; k++) {
			float by = low + (float)(((int)(t * 1.5f) + k * 76) % (int)(high - low));
			iFilledCircle(cxp, by, 2.5f);
		}
	}
	else if (tm < L4_PYLON_ACTIVE_AT) {
		// CHARGING: warning - the beam will fill exactly this outline
		float f = (tm - L4_PYLON_CHARGE_AT) / (float)(L4_PYLON_ACTIVE_AT - L4_PYLON_CHARGE_AT);
		float wdt = 2.0f + f * 10.0f;
		iSetColor(90, 130, 170);
		iRectangle(cxp - o.w * 0.5f + 2.0f, o.y, (float)(o.w - 4), (float)o.h);
		iSetColor(40 + (int)(f * 180.0f), 120 + (int)(f * 100.0f), 220);
		iFilledRectangle(cxp - wdt * 0.5f, low, wdt, high - low);
		if (((int)(t / 5.0f)) % 2 == 0) {
			iSetColor(255, 230, 90);
			iText(cxp - 4.0f, top + 22.0f, (char*)"!", GLUT_BITMAP_HELVETICA_18);
		}
	}
	else if (tm < L4_PYLON_COOL_AT) {
		// ACTIVE: full-height barrier - hurts
		iSetColor(40, 110, 200);
		iFilledRectangle(cxp - o.w * 0.5f - 7.0f, low, o.w + 14.0f, high - low);
		iSetColor(120, 220, 255);
		iFilledRectangle(cxp - o.w * 0.5f + 2.0f, low, (float)(o.w - 4), high - low);
		iSetColor(245, 255, 255);
		iFilledRectangle(cxp - 6.0f, low, 12.0f, high - low);
		iSetColor(200, 240, 255);
		for (k = 0; k < 8; k++) {
			float ry = low + (float)(((int)(t * 4.0f) + k * 50) % (int)(high - low));
			iLine(cxp - o.w * 0.5f - 9.0f, ry, cxp + o.w * 0.5f + 9.0f, ry);
		}
	}
	else {
		// COOLING: the beam collapses - harmless
		float f = (tm - L4_PYLON_COOL_AT) / (float)(L4_PYLON_CYCLE - L4_PYLON_COOL_AT);
		float wdt = (float)(o.w - 4) * (1.0f - f) + 2.0f;
		iSetColor(70, 150, 210);
		iFilledRectangle(cxp - wdt * 0.5f, low, wdt, high - low);
	}
}

static void L4DrawSealApproach(void) {
	int i;
	const float PI2 = 6.2831853f;
	bool live;

	if (!l4SealAwake) return;
	live = (l4SealTimer >= L4_SEAL_GRACE);

	// the altar
	{
		float sx = L4_ALTAR_X - l4CamX;
		float t  = (float)l4SealTimer;
		if (sx < (float)SCREEN_WIDTH + 140.0f && sx > -200.0f) {
			float cx = sx + 60.0f, cy = (float)GROUND_Y + 92.0f;
			float r  = 40.0f + 3.0f * (float)sin(t * 0.08f);
			iSetColor(56, 50, 44);
			iFilledRectangle(sx, (float)GROUND_Y, 120.0f, 26.0f);
			iSetColor(0, 220, 200);
			iRectangle(sx, (float)GROUND_Y, 120.0f, 26.0f);
			iCircle(cx, cy, r);
			iCircle(cx, cy, r - 8.0f);
			for (i = 0; i < 5; i++) {
				float a = t * 0.02f + i * (PI2 / 5.0f);
				float b = t * 0.02f + ((i + 2) % 5) * (PI2 / 5.0f);
				iLine(cx + (float)cos(a) * r, cy + (float)sin(a) * r, cx + (float)cos(b) * r, cy + (float)sin(b) * r);
			}
			iSetColor(0, 255, 220);
			iText(sx - 2.0f, (float)GROUND_Y + 150.0f, (char*)"THE GROVE SEAL", GLUT_BITMAP_HELVETICA_12);
		}
		else if (sx >= (float)SCREEN_WIDTH + 140.0f && ((int)(t / 30.0f)) % 2 == 0) {
			iSetColor(0, 255, 220);
			iText((float)SCREEN_WIDTH - 250.0f, 430.0f, (char*)"THE GROVE SEAL  >>>", GLUT_BITMAP_HELVETICA_18);
		}
	}

	for (i = 0; i < l4ObstacleCount; i++) {
		const L4Obstacle& o = l4Obstacles[i];
		if (!o.active) continue;
		if (o.type == L4_OB_PYLON) L4DrawPylon(o, live);
		else if (o.type == L4_OB_WISP) L4DrawWisp(o, live);
	}
}

static void L4BuildWorld(void) {
	l4ObstacleCount = 0;

	// ---------------- SECTION 1 : OUTER PATH (teach the verbs) --------
	L4AddObstacle(L4_OB_SOLID,   520.0f, (float)GROUND_Y,  70,  95, 0, 0, 0, 0, 1);
	L4AddObstacle(L4_OB_LOWBAR,  850.0f, L4_LOWBAR_Y,     170, 130, 0, 0, 0, 0, 1);
	L4AddObstacle(L4_OB_SPIKE,  1060.0f, (float)GROUND_Y,  90,  30, 0, 0, 0, 0, 1);

	// ---------------- SECTION 2 : THE AMBUSH (chasers) ----------------
	L4AddObstacle(L4_OB_SOLID,  1500.0f, (float)GROUND_Y,  80, 105, 0, 0, 0, 0, 2);
	L4AddObstacle(L4_OB_SPIKE,  1770.0f, (float)GROUND_Y, 110,  30, 0, 0, 0, 0, 2);
	L4AddObstacle(L4_OB_SOLID,  2060.0f, (float)GROUND_Y,  80, 125, 0, 0, 0, 0, 2);
	L4AddGate(2);

	// ---------------- SECTION 3 : CROSSFIRE (ranged + movers) ---------
	L4AddObstacle(L4_OB_MOVER,  2650.0f, (float)GROUND_Y,  70, 110, 1, 190.0f, 0.030f, 0.0f, 3);
	L4AddObstacle(L4_OB_LOWBAR, 2960.0f, L4_LOWBAR_Y,     200, 130, 0, 0, 0, 0, 3);
	L4AddObstacle(L4_OB_SPIKE,  3190.0f, (float)GROUND_Y, 120,  30, 0, 0, 0, 0, 3);
	// vertical crusher: baseY is its TOP-most rest height, range pushes it down
	L4AddObstacle(L4_OB_MOVER,  3390.0f, 310.0f,           90, 150, 2, 105.0f, 0.036f, 1.6f, 3);
	L4AddGate(3);

	// ---------------- SECTION 4 : SWIFT HUNT (moving hazards) ---------
	L4AddObstacle(L4_OB_BLADE,  3860.0f, 520.0f,           84,  84, 0, 250.0f, 0.032f, 0.0f, 4);
	L4AddObstacle(L4_OB_MOVER,  4150.0f, (float)GROUND_Y,  70, 115, 1, 230.0f, 0.042f, 0.9f, 4);
	L4AddObstacle(L4_OB_SPIKE,  4420.0f, (float)GROUND_Y, 140,  30, 0, 0, 0, 0, 4);
	L4AddObstacle(L4_OB_BLADE,  4630.0f, 520.0f,           84,  84, 0, 250.0f, 0.038f, 2.1f, 4);
	L4AddGate(4);

	// ---------------- SECTION 5 : MIXED ASSAULT -----------------------
	L4AddObstacle(L4_OB_SOLID,  5060.0f, (float)GROUND_Y,  80, 115, 0, 0, 0, 0, 5);
	L4AddObstacle(L4_OB_MOVER,  5300.0f, 310.0f,           90, 150, 2, 105.0f, 0.040f, 0.4f, 5);
	L4AddObstacle(L4_OB_SPIKE,  5530.0f, (float)GROUND_Y, 130,  30, 0, 0, 0, 0, 5);
	L4AddObstacle(L4_OB_LOWBAR, 5730.0f, L4_LOWBAR_Y,     180, 130, 0, 0, 0, 0, 5);
	L4AddGate(5);

	// ---------------- SECTION 6 : THE GAUNTLET (narrow passage) -------
	// A long crouch-corridor with two sliding thorn hazards inside it.
	// The hazards are deliberately NON-solid: inside a corridor you cannot
	// jump out of, a solid mover could pin the player against a wall, so
	// these cost health instead of ever trapping anyone.
	L4AddObstacle(L4_OB_SOLID,  6150.0f, (float)GROUND_Y,  70, 130, 0, 0, 0, 0, 6);
	L4AddObstacle(L4_OB_LOWBAR, 6260.0f, L4_LOWBAR_Y,     430, 150, 0, 0, 0, 0, 6);
	L4AddObstacle(L4_OB_BLADE,  6390.0f, 150.0f,           62,  62, 1, 110.0f, 0.045f, 0.0f, 6);
	L4AddObstacle(L4_OB_BLADE,  6580.0f, 150.0f,           62,  62, 1, 110.0f, 0.052f, 1.9f, 6);
	L4AddObstacle(L4_OB_SPIKE,  6760.0f, (float)GROUND_Y, 120,  30, 0, 0, 0, 0, 6);
	L4AddObstacle(L4_OB_SOLID,  6900.0f, (float)GROUND_Y,  80, 140, 0, 0, 0, 0, 6);
	L4AddObstacle(L4_OB_MOVER,  7050.0f, 310.0f,           90, 150, 2, 105.0f, 0.048f, 2.6f, 6);
	L4AddGate(6);

	// ---------------- SECTION 7 : HEAVY GUARD (mini-boss, phase 1) ----
	L4AddObstacle(L4_OB_SOLID,  7520.0f, (float)GROUND_Y,  60, 120, 0, 0, 0, 0, 7);
	L4AddObstacle(L4_OB_SPIKE,  7820.0f, (float)GROUND_Y, 140,  30, 0, 0, 0, 0, 7);
	L4AddObstacle(L4_OB_SOLID,  8120.0f, (float)GROUND_Y,  60, 120, 0, 0, 0, 0, 7);
	L4AddGate(7);

	// ---------------- SECTION 8 : FINAL STAND (mini-boss, phase 2) ----
	L4AddObstacle(L4_OB_SOLID,  8720.0f, (float)GROUND_Y,  60, 120, 0, 0, 0, 0, 8);
	L4AddObstacle(L4_OB_SPIKE,  8960.0f, (float)GROUND_Y, 130,  30, 0, 0, 0, 0, 8);
	L4AddObstacle(L4_OB_BLADE,  9160.0f, 520.0f,           84,  84, 0, 250.0f, 0.040f, 1.1f, 8);
	L4AddObstacle(L4_OB_SOLID,  9360.0f, (float)GROUND_Y,  60, 120, 0, 0, 0, 0, 8);
	// no gate - the seal approach below wakes up when the mini-boss falls

	L4BuildSealApproach();
}

// =====================================================================
// 6. LIFECYCLE
// =====================================================================

void LoadLevel4Assets(void) {
	backgroundImg4 = iLoadImage((char*)"images\\level4_background.png");
}

static void L4ResetEnemy(L4Enemy& e) {
	e.active = false;
	e.hitFlash = 0;
	e.contactCd = 0;
}

void InitLevel4(void) {
	int i;

	currentLevel = LEVEL4_ID;
	currentPart  = 1;

	// Reuse the existing character-swap entry point. Passing LEVEL4_ID (4)
	// selects the Naruto player (visual only); the bullet/gun values it
	// sets are the same ones the level-2 set used here before.
	SetPlayerCharacter(LEVEL4_ID);

	// Player reset. playerMaxHealth/playerHealth are the project's own
	// globals - the level just gives them a value suited to its damage
	// numbers instead of the huge level-1/2/3 figures.
	playerMaxHealth = L4_PLAYER_MAX_HP;
	playerHealth    = L4_PLAYER_MAX_HP;
	playerX         = 120;
	playerY         = GROUND_Y;
	isJumping       = false;
	isCrouching     = false;
	playerFacingRight = true;

	for (i = 0; i < MAX_PLAYER_BULLETS; i++) p_bulletActive[i] = false;

	l4CamX          = 0.0f;
	l4Section       = 1;
	l4Wave          = 1;
	l4WaveStarted   = 0;
	l4WaveTimer     = 0;
	l4PlayerInvuln  = 0;
	l4HurtFlash     = 0;
	l4Score         = 0;
	l4Frames        = 0;
	l4ShootSfxCd    = 0;
	l4PrevActiveBullets = 0;
	l4PuzzleStarted = false;
	l4SealAwake     = false;
	l4SealTimer     = 0;
	l4Reinforcements = 0;
	l4ReinforceTimer = 0;
	l4BannerTimer   = 0;
	l4BannerText[0] = '\0';

	for (i = 0; i <= L4_NUM_SECTIONS; i++) l4SectionCleared[i] = 0;
	l4SectionCleared[1] = 1; // section 1 is a traversal tutorial, no gate

	for (i = 0; i < L4_MAX_ENEMIES; i++)  L4ResetEnemy(l4Enemies[i]);
	for (i = 0; i < L4_MAX_EBULLETS; i++) l4Bullets[i].active = false;
	for (i = 0; i < L4_MAX_SPAWNS; i++)   l4SpawnFired[i] = false;

	l4Boss.active     = false;
	l4Boss.defeated   = false;
	l4Boss.introPlayed = false;
	l4Boss.hp         = L4_HP_HEAVY;
	l4Boss.maxHp      = L4_HP_HEAVY;
	l4Boss.phase      = 1;
	l4Boss.hitFlash   = 0;

	L4BuildWorld();
	L4Banner("SECTION 1 - OUTER PATH");
}

// =====================================================================
// 7. ENEMY SPAWNING
// =====================================================================

static int L4WaveCount(int section) {
	int maxWave = 0, i;
	for (i = 0; i < L4_SPAWN_COUNT; i++) {
		if (L4_SPAWNS[i].section == section && L4_SPAWNS[i].wave > maxWave) {
			maxWave = L4_SPAWNS[i].wave;
		}
	}
	return maxWave;
}

static void L4SpawnEnemy(int type, float worldX, int section, int wave) {
	int i;
	for (i = 0; i < L4_MAX_ENEMIES; i++) {
		if (l4Enemies[i].active) continue;

		L4Enemy& e = l4Enemies[i];
		e.active     = true;
		e.type       = type;
		e.x          = worldX;
		e.y          = (float)GROUND_Y;
		e.vy         = 0.0f;
		e.facingLeft = true;
		e.aiState    = 0;
		e.aiTimer    = 0;
		e.contactCd  = 0;
		e.animTimer  = 0;
		e.animFrame  = 0;
		e.hitFlash   = 0;
		e.spawnGrace = 24;
		e.stuckTimer = 0;
		e.section    = section;
		e.wave       = wave;

		switch (type) {
		case L4_CHASER:
			e.w = 90;  e.h = 145; e.hp = L4_HP_CHASER; e.shootTimer = 0;
			break;
		case L4_RANGED:
			e.w = 86;  e.h = 140; e.hp = L4_HP_RANGED; e.shootTimer = 70;
			break;
		case L4_FAST:
		default:
			e.w = 70;  e.h = 112; e.hp = L4_HP_FAST;   e.shootTimer = 0;
			break;
		}
		e.maxHp = e.hp;
		return;
	}
}

static void L4SpawnBoss(void) {
	l4Boss.active     = true;
	l4Boss.defeated   = false;
	l4Boss.x          = (float)(L4SectionStart(7) + 880);
	l4Boss.y          = (float)GROUND_Y;
	l4Boss.vy         = 0.0f;
	l4Boss.w          = 190;
	l4Boss.h          = 165;
	l4Boss.hp         = L4_HP_HEAVY;
	l4Boss.maxHp      = L4_HP_HEAVY;
	l4Boss.facingLeft = true;
	l4Boss.phase      = 1;
	l4Boss.state      = 0;
	l4Boss.stateTimer = 110;
	l4Boss.volleyLeft = 0;
	l4Boss.volleyGap  = 0;
	l4Boss.contactCd  = 0;
	l4Boss.hitFlash   = 0;
	l4Boss.stomp      = 0;
	l4Boss.regrouped  = false;

	if (!l4Boss.introPlayed) {
		l4Boss.introPlayed = true;
		L4MusicBoss();
		L4Banner("MINI-BOSS: THE GROVE WARDEN");
	}
}

static void L4FireBullet(float x, float y, float vx, float vy,
	int w, int h, int damage, int kind) {
	int i;
	for (i = 0; i < L4_MAX_EBULLETS; i++) {
		if (l4Bullets[i].active) continue;
		L4Bullet& b = l4Bullets[i];
		b.active = true;
		b.x = x; b.y = y;
		b.vx = vx; b.vy = vy;
		b.w = w; b.h = h;
		b.damage = damage;
		b.kind = kind;
		b.life = 420;
		return;
	}
}

// =====================================================================
// 8. OBSTACLES
// =====================================================================

static void L4UpdateObstacles(void) {
	int i;
	for (i = 0; i < l4ObstacleCount; i++) {
		L4Obstacle& o = l4Obstacles[i];
		if (!o.active) continue;

		if (o.type == L4_OB_BLADE) {
			// Pendulum/slider hazard. axis 1 = slides sideways along the
			// ground; otherwise it swings from a fixed anchor overhead.
			o.phase += o.speed;
			if (o.axis == 1) {
				o.x = o.baseX + (float)sin(o.phase) * o.range;
			}
			else {
				float angle = (float)sin(o.phase) * 1.05f;
				o.x = o.baseX + (float)sin(angle) * o.range;
				o.y = o.baseY - (float)cos(angle) * o.range;
			}
		}
		else if (o.type == L4_OB_MOVER) {
			o.phase += o.speed;
			if (o.axis == 1) {
				o.x = o.baseX + (float)sin(o.phase) * o.range;
			}
			else {
				// Crusher: rests high, slams down toward the ground.
				float t = ((float)sin(o.phase) + 1.0f) * 0.5f;  // 0..1
				o.y = o.baseY - t * o.range;
			}
		}
	}
}

// Push the player out of anything solid. X-axis only: the level is played
// on a single ground plane, so a horizontal push is always a safe, legible
// resolution and never fights the player's own jump/gravity code.
static void L4ResolveSolids(void) {
	int i;
	float pw = (float)playerWidth;
	float ph = (float)L4PlayerH();

	for (i = 0; i < l4ObstacleCount; i++) {
		L4Obstacle& o = l4Obstacles[i];
		if (!o.active || !o.solid) continue;
		if (o.type == L4_OB_GATE && l4SectionCleared[o.section]) continue;

		float px = L4PlayerWorldX();
		float py = (float)playerY;

		if (!L4Rect(px, py, pw, ph, o.x, o.y, (float)o.w, (float)o.h)) continue;

		float pcx = px + pw * 0.5f;
		float ocx = o.x + o.w * 0.5f;
		float targetWorldX = (pcx < ocx) ? (o.x - pw - 1.0f) : (o.x + o.w + 1.0f);

		int newScreenX = (int)(targetWorldX - l4CamX);
		if (newScreenX < 0) newScreenX = 0;
		if (newScreenX > SCREEN_WIDTH - playerWidth) newScreenX = SCREEN_WIDTH - playerWidth;
		playerX = newScreenX;

		if (o.damage > 0) L4DamagePlayer(o.damage, true);
	}
}

// Non-solid hazards: spikes and blades. Contact hurts, nothing else.
static void L4CheckHazards(void) {
	int i;
	float px = L4PlayerWorldX();
	float py = (float)playerY;
	float pw = (float)playerWidth;
	float ph = (float)L4PlayerH();

	for (i = 0; i < l4ObstacleCount; i++) {
		L4Obstacle& o = l4Obstacles[i];
		if (!o.active || o.solid || o.damage <= 0) continue;

		if (L4Rect(px, py, pw, ph, o.x, o.y, (float)o.w, (float)o.h)) {
			L4DamagePlayer(o.damage, true);
		}
	}
}

// Does a solid block sit immediately in front of this enemy at its current
// height? Used to decide when an enemy should hop instead of walking into
// a wall. Gates count only while still closed.
static bool L4SolidAhead(const L4Enemy& e, float dx) {
	int i;
	float nx = e.x + dx;
	for (i = 0; i < l4ObstacleCount; i++) {
		const L4Obstacle& o = l4Obstacles[i];
		if (!o.active || !o.solid) continue;
		if (o.type == L4_OB_GATE && l4SectionCleared[o.section]) continue;
		if (L4Rect(nx, e.y, (float)e.w, (float)e.h, o.x, o.y, (float)o.w, (float)o.h)) return true;
	}
	return false;
}

// =====================================================================
// 9. ENEMY AI  (four genuinely different behaviours)
// =====================================================================

static void L4ApplyEnemyGravity(L4Enemy& e) {
	if (e.y > (float)GROUND_Y || e.vy != 0.0f) {
		e.vy -= 0.50f;
		e.y  += e.vy;
		if (e.y <= (float)GROUND_Y) {
			e.y = (float)GROUND_Y;
			e.vy = 0.0f;
		}
	}
}

static void L4MoveEnemyX(L4Enemy& e, float dx) {
	if (dx == 0.0f) return;

	// Three rules keep an enemy from ever being permanently walled off,
	// which would leave a wave uncompletable and soft-lock the level:
	//   (a) if it is ALREADY inside something solid it may always move out;
	//   (b) if blocked while standing, it hops - the jump arc clears every
	//       solid in the level with room to spare;
	//   (c) if it somehow stays blocked for 5 straight seconds it starts
	//       clambering through, so progression can never stall.
	bool blocked = L4SolidAhead(e, dx);
	bool embedded = L4SolidAhead(e, 0.0f);

	if (blocked && !embedded && e.stuckTimer < 300) {
		if (e.y <= (float)GROUND_Y + 0.5f) e.vy = 17.0f;
		e.stuckTimer++;
		return;
	}

	e.x += dx;
	if (e.stuckTimer > 0) e.stuckTimer--;
}

static void L4EnemyTouchesPlayer(L4Enemy& e, int damage) {
	if (e.contactCd > 0) { e.contactCd--; return; }

	float px = L4PlayerWorldX();
	if (L4Rect(px, (float)playerY, (float)playerWidth, (float)L4PlayerH(),
		e.x, e.y, (float)e.w, (float)e.h)) {
		L4DamagePlayer(damage, false);
		e.contactCd = 42;
	}
}

// How far the player's rectangle and an enemy's rectangle currently are
// apart along x, measured edge to edge. Negative means they overlap.
static float L4EdgeGap(const L4Enemy& e) {
	float px = L4PlayerWorldX();
	float pr = px + (float)playerWidth;
	if (e.x >= pr) return e.x - pr;          // enemy is to the right
	if (e.x + e.w <= px) return px - (e.x + (float)e.w);
	return -1.0f;                            // already overlapping
}

// A melee enemy that has just landed a hit backs out of the player before
// lunging again. This is not decoration: the player's bullets spawn at the
// player's own outer edge, so an enemy parked *inside* the player could
// never be shot. Without the recoil a chaser that reached the player would
// deadlock the wave - and therefore the whole level.
static bool L4ContactRecoil(L4Enemy& e, float playerCX, int holdFrames, float speed, int damage) {
	if (e.contactCd <= holdFrames) return false;

	bool pushLeft = (playerCX <= e.x + e.w * 0.5f);   // player is to our left
	L4MoveEnemyX(e, pushLeft ? speed : -speed);

	e.animTimer++;
	if (e.animTimer >= 5) { e.animTimer = 0; e.animFrame = (e.animFrame + 1) % TOTAL_ENEMY_FRAMES; }

	L4ApplyEnemyGravity(e);
	L4EnemyTouchesPlayer(e, damage);
	return true;
}

static void L4UpdateChaser(L4Enemy& e, float playerCX) {
	// TYPE 1 - CHASER: steady pursuit, moderate health, contact damage.
	float ecx = e.x + e.w * 0.5f;
	e.facingLeft = (playerCX <= ecx);

	// Lunge -> hit -> recoil -> lunge again.
	if (L4ContactRecoil(e, playerCX, 20, 4.2f, L4_DMG_CHASER_TOUCH)) return;

	// Close in until the bodies just barely overlap, not until the centres
	// line up - an enemy standing dead-centre on the player is unshootable.
	const float SPEED = 2.4f;
	if (L4EdgeGap(e) > -14.0f) {
		L4MoveEnemyX(e, e.facingLeft ? -SPEED : SPEED);
		e.animTimer++;
		if (e.animTimer >= 6) { e.animTimer = 0; e.animFrame = (e.animFrame + 1) % TOTAL_ENEMY_FRAMES; }
	}

	L4ApplyEnemyGravity(e);
	L4EnemyTouchesPlayer(e, L4_DMG_CHASER_TOUCH);
}

static void L4UpdateRanged(L4Enemy& e, float playerCX) {
	// TYPE 2 - RANGED: holds a firing lane, telegraphs, then shoots.
	// It never touches the player for damage; only its projectile does,
	// and only on an actual rectangle overlap.
	float ecx = e.x + e.w * 0.5f;
	float dist = playerCX - ecx;
	e.facingLeft = (dist <= 0.0f);

	const float SPEED   = 1.9f;
	const float HOLD_MIN = 330.0f;
	const float HOLD_MAX = 470.0f;
	float adist = (float)fabs(dist);

	if (adist < HOLD_MIN)      L4MoveEnemyX(e, e.facingLeft ?  SPEED : -SPEED); // back off
	else if (adist > HOLD_MAX) L4MoveEnemyX(e, e.facingLeft ? -SPEED :  SPEED); // close in

	e.animTimer++;
	if (e.animTimer >= 7) { e.animTimer = 0; e.animFrame = (e.animFrame + 1) % TOTAL_ENEMY_FRAMES; }

	// Periodic hop so it is never a static turret.
	e.aiTimer++;
	if (e.aiTimer >= 190) {
		e.aiTimer = 0;
		if (e.y <= (float)GROUND_Y + 0.5f) e.vy = 11.0f;
	}

	// Fire cycle: countdown -> 22-frame telegraph -> shot -> reset.
	if (e.aiState == 0) {
		e.shootTimer--;
		if (e.shootTimer <= 0) { e.aiState = 1; e.shootTimer = 22; }
	}
	else {
		e.shootTimer--;
		if (e.shootTimer <= 0) {
			e.aiState = 0;
			e.shootTimer = 110;
			float bx = e.facingLeft ? (e.x - 18.0f) : (e.x + e.w);
			float by = e.y + e.h * 0.52f;
			L4FireBullet(bx, by, e.facingLeft ? -7.0f : 7.0f, 0.0f,
				22, 22, L4_DMG_RANGED_SHOT, 0);
		}
	}

	L4ApplyEnemyGravity(e);
}

static void L4UpdateFast(L4Enemy& e, float playerCX) {
	// TYPE 3 - FAST: dash / recover cycle. It commits to a direction for a
	// whole dash, so it regularly overshoots the player and has to turn
	// around - that's what makes it feel twitchy instead of just quick.
	float ecx = e.x + e.w * 0.5f;

	// Same anti-deadlock recoil as the chaser, but snappier.
	if (L4ContactRecoil(e, playerCX, 18, 5.4f, L4_DMG_FAST_TOUCH)) return;

	e.aiTimer--;
	if (e.aiTimer <= 0) {
		if (e.aiState == 0) { e.aiState = 1; e.aiTimer = 46; e.facingLeft = (playerCX <= ecx); }
		else                { e.aiState = 0; e.aiTimer = 26; }
	}

	if (e.aiState == 1) {
		const float DASH = 6.2f;
		L4MoveEnemyX(e, e.facingLeft ? -DASH : DASH);
		e.animTimer++;
		if (e.animTimer >= 3) { e.animTimer = 0; e.animFrame = (e.animFrame + 1) % TOTAL_ENEMY_FRAMES; }

		// Leaps mid-dash, on a fixed beat so the pattern stays readable.
		if (e.aiTimer == 30 && e.y <= (float)GROUND_Y + 0.5f) e.vy = 13.0f;
	}

	L4ApplyEnemyGravity(e);
	L4EnemyTouchesPlayer(e, L4_DMG_FAST_TOUCH);
}

static void L4UpdateEnemies(void) {
	int i;
	float playerCX = L4PlayerWorldX() + playerWidth * 0.5f;

	for (i = 0; i < L4_MAX_ENEMIES; i++) {
		L4Enemy& e = l4Enemies[i];
		if (!e.active) continue;

		if (e.spawnGrace > 0) e.spawnGrace--;
		if (e.hitFlash  > 0) e.hitFlash--;

		switch (e.type) {
		case L4_CHASER: L4UpdateChaser(e, playerCX); break;
		case L4_RANGED: L4UpdateRanged(e, playerCX); break;
		case L4_FAST:   L4UpdateFast(e, playerCX);   break;
		default: break;
		}

		// Keep enemies inside the playable world.
		if (e.x < -220.0f) e.x = -220.0f;
		if (e.x > (float)(L4_WORLD_WIDTH - 40)) e.x = (float)(L4_WORLD_WIDTH - 40);
	}
}

// =====================================================================
// 10. MINI-BOSS  (TYPE 4 - HEAVY)
// =====================================================================
//
// Four-state cycle, all of it in motion:
//   0 ADVANCE - walks the player down
//   1 VOLLEY  - stops and fires a spread (3 shots in phase 1, 5 in phase 2)
//   2 CHARGE  - raises its guard: damage taken drops to 25% while it winds
//               up, and in phase 2 it also recovers behind the guard after
//               every slam. This is what makes the fight long without
//               inflating HP.
//   3 SLAM    - leaps, and on landing sends a shockwave along the ground in
//               both directions. Jump to clear it.
//   4 RETREAT - at 50% health it raises an unbreakable guard and falls back
//               into section 8. The player has to follow it through FINAL
//               STAND to finish the fight, so the second half of the
//               encounter is a chase, not a longer health bar.
//
static void L4UpdateBoss(void) {
	if (!l4Boss.active) return;

	float playerCX = L4PlayerWorldX() + playerWidth * 0.5f;
	float bcx = l4Boss.x + l4Boss.w * 0.5f;
	l4Boss.facingLeft = (playerCX <= bcx);

	if (l4Boss.hitFlash > 0) l4Boss.hitFlash--;
	if (l4Boss.contactCd > 0) l4Boss.contactCd--;
	if (l4Boss.stomp > 0) l4Boss.stomp--;

	bool enraged = (l4Boss.phase == 2);
	l4Boss.stateTimer--;

	switch (l4Boss.state) {
	case 0: { // ADVANCE
		float speed = enraged ? 2.1f : 1.3f;
		if (fabs(playerCX - bcx) > 70.0f) {
			l4Boss.x += l4Boss.facingLeft ? -speed : speed;
		}
		if (l4Boss.stateTimer <= 0) {
			// Alternate between shooting and the slam wind-up.
			if (((int)(l4Frames / 60)) % 2 == 0) {
				l4Boss.state = 1;
				l4Boss.stateTimer = enraged ? 70 : 80;
				l4Boss.volleyLeft = enraged ? 5 : 3;
				l4Boss.volleyGap = 0;
			}
			else {
				l4Boss.state = 2;
				l4Boss.stateTimer = enraged ? 55 : 85;
			}
		}
		break;
	}
	case 1: { // VOLLEY
		if (l4Boss.volleyLeft > 0) {
			l4Boss.volleyGap--;
			if (l4Boss.volleyGap <= 0) {
				l4Boss.volleyGap = enraged ? 11 : 16;
				l4Boss.volleyLeft--;
				float bx = l4Boss.facingLeft ? (l4Boss.x - 24.0f) : (l4Boss.x + l4Boss.w);
				// Fan the shots at slightly different heights so the player
				// has to move rather than stand in one safe spot.
				float by = l4Boss.y + 40.0f + (float)(l4Boss.volleyLeft % 3) * 34.0f;
				float sp = enraged ? 8.5f : 7.0f;
				L4FireBullet(bx, by, l4Boss.facingLeft ? -sp : sp, 0.0f,
					28, 28, L4_DMG_BOSS_SHOT, 1);
			}
		}
		if (l4Boss.stateTimer <= 0) {
			l4Boss.state = 0;
			l4Boss.stateTimer = enraged ? 80 : 110;
		}
		break;
	}
	case 2: { // CHARGE (shielded wind-up)
		if (l4Boss.stateTimer <= 0) {
			l4Boss.state = 3;
			l4Boss.stateTimer = 90;
			l4Boss.vy = 15.0f;   // leap
		}
		break;
	}
	case 4: { // RETREAT - withdraw into section 8, guard up, untouchable
		float goal = (float)(L4SectionStart(8) + 560);
		l4Boss.x += 4.2f;
		if (l4Boss.x >= goal) {
			l4Boss.x = goal;
			l4Boss.regrouped = true;
			l4Boss.state = 0;
			l4Boss.stateTimer = 70;
			L4Banner("THE WARDEN MAKES ITS FINAL STAND");
		}
		break;
	}
	case 3: { // SLAM
		l4Boss.vy -= 0.85f;
		l4Boss.y += l4Boss.vy;

		// Drift toward the player while airborne so the slam is a threat.
		l4Boss.x += l4Boss.facingLeft ? -2.6f : 2.6f;

		if (l4Boss.y <= (float)GROUND_Y) {
			l4Boss.y = (float)GROUND_Y;
			l4Boss.vy = 0.0f;
			l4Boss.stomp = 18;

			// Two ground shockwaves. They are 38px tall, so a player in the
			// air simply is not overlapping them - jumping is the answer.
			float cy = (float)GROUND_Y;
			L4FireBullet(l4Boss.x - 50.0f, cy, -9.0f, 0.0f, 54, 38, L4_DMG_SHOCKWAVE, 2);
			L4FireBullet(l4Boss.x + l4Boss.w, cy, 9.0f, 0.0f, 54, 38, L4_DMG_SHOCKWAVE, 2);

			if (enraged) {
				// Enraged, it recovers from a slam behind its guard, so the
				// second phase is longer because of what it DOES, not because
				// it was handed a bigger health bar.
				l4Boss.state = 2;
				l4Boss.stateTimer = 75;
			}
			else {
				l4Boss.state = 0;
				l4Boss.stateTimer = 100;
			}
		}
		break;
	}
	default: break;
	}

	// Keep the boss on the field. While section 7's gate is still sealed the
	// boss is held inside that arena too, so it can't wander through a
	// barrier the player is standing behind.
	float minX = (float)(L4SectionStart(7) + 40);
	float maxX = (float)(L4_WORLD_WIDTH - l4Boss.w - 40);
	if (l4Boss.regrouped) {
		// Phase 2 belongs to section 8 - the Warden never walks back.
		minX = (float)(L4SectionStart(8) + 200);
	}
	else if (!l4SectionCleared[7] && l4Boss.state != 4) {
		float gateX = (float)(L4SectionStart(7) + L4_SECTION_W - 60);
		maxX = gateX - l4Boss.w - 10.0f;
	}
	if (l4Boss.x < minX) l4Boss.x = minX;
	if (l4Boss.x > maxX) l4Boss.x = maxX;

	// Body contact.
	if (l4Boss.contactCd <= 0) {
		float px = L4PlayerWorldX();
		if (L4Rect(px, (float)playerY, (float)playerWidth, (float)L4PlayerH(),
			l4Boss.x, l4Boss.y, (float)l4Boss.w, (float)l4Boss.h)) {
			L4DamagePlayer(L4_DMG_BOSS_TOUCH, false);
			l4Boss.contactCd = 55;
		}
	}
}

// =====================================================================
// 11. BULLETS
// =====================================================================

// Enemy/boss projectiles -> player. Damage only on a real overlap.
static void L4UpdateEnemyBullets(void) {
	int i;
	float px = L4PlayerWorldX();
	float py = (float)playerY;
	float pw = (float)playerWidth;
	float ph = (float)L4PlayerH();

	for (i = 0; i < L4_MAX_EBULLETS; i++) {
		L4Bullet& b = l4Bullets[i];
		if (!b.active) continue;

		b.x += b.vx;
		b.y += b.vy;
		b.life--;

		if (b.life <= 0 || b.x < -120.0f || b.x > (float)(L4_WORLD_WIDTH + 120)) {
			b.active = false;
			continue;
		}

		if (L4Rect(b.x, b.y, (float)b.w, (float)b.h, px, py, pw, ph)) {
			b.active = false;
			L4DamagePlayer(b.damage, b.kind == 2);
		}
	}
}

// Player bullets -> enemies / boss.
//
// The player bullet pool lives in player.cpp in SCREEN coordinates, and it
// is deliberately left that way. Enemy world positions are converted to
// screen space for this test, which keeps the check exactly consistent
// with what the player can see on screen.
static void L4UpdatePlayerBulletHits(void) {
	int i, j;

	for (j = 0; j < MAX_PLAYER_BULLETS; j++) {
		if (!p_bulletActive[j]) continue;

		float bx = (float)p_bx[j];
		float by = (float)p_by[j];
		float bw = (float)bulletWidth;
		float bh = (float)bulletHeight;

		// --- normal enemies ---
		for (i = 0; i < L4_MAX_ENEMIES; i++) {
			L4Enemy& e = l4Enemies[i];
			if (!e.active || e.spawnGrace > 0) continue;

			float sx = e.x - l4CamX;
			if (!L4Rect(bx, by, bw, bh, sx, e.y, (float)e.w, (float)e.h)) continue;

			p_bulletActive[j] = false;
			e.hp -= L4_BULLET_DAMAGE;
			e.hitFlash = 6;

			if (e.hp <= 0) {
				e.active = false;
				l4Score += (e.type == L4_FAST) ? 150 : (e.type == L4_RANGED ? 130 : 100);
				L4SfxEnemyDown();
			}
			else {
				L4SfxEnemyHit();
			}
			break;
		}
		if (!p_bulletActive[j]) continue;

		// --- mini-boss ---
		if (l4Boss.active) {
			float sx = l4Boss.x - l4CamX;
			if (L4Rect(bx, by, bw, bh, sx, l4Boss.y, (float)l4Boss.w, (float)l4Boss.h)) {
				p_bulletActive[j] = false;

				// Guard stance (state 2) cuts incoming damage rather than
				// the boss simply owning a bigger health pool.
				int dmg = L4_BULLET_DAMAGE;
				if (l4Boss.state == 2) dmg = (L4_BULLET_DAMAGE * 25) / 100;
				if (l4Boss.state == 4) dmg = 0;   // guard is absolute while it withdraws

				l4Boss.hp -= dmg;
				l4Boss.hitFlash = 6;
				L4SfxEnemyHit();

				if (l4Boss.phase == 1 && l4Boss.hp <= l4Boss.maxHp / 2) {
					l4Boss.phase = 2;
					l4Boss.state = 4;      // break off and fall back
					l4Boss.stateTimer = 9999;
					L4Banner("WARDEN ENRAGED - IT FALLS BACK");
				}

				if (l4Boss.hp <= 0) {
					l4Boss.hp = 0;
					l4Boss.active = false;
					l4Boss.defeated = true;
					l4Score += 1500;
					L4SfxBossDown();
					L4Banner("WARDEN DOWN - THE SEAL AWAKENS");
				}
			}
		}
	}
}

// =====================================================================
// 12. WAVE / SECTION PROGRESSION
// =====================================================================

static int L4AliveInWave(int section, int wave) {
	int i, n = 0;
	for (i = 0; i < L4_MAX_ENEMIES; i++) {
		if (l4Enemies[i].active &&
			l4Enemies[i].section == section &&
			l4Enemies[i].wave == wave) n++;
	}
	return n;
}

static int L4AliveInSection(int section) {
	int i, n = 0;
	for (i = 0; i < L4_MAX_ENEMIES; i++) {
		if (l4Enemies[i].active && l4Enemies[i].section == section) n++;
	}
	return n;
}

static bool L4AllSpawnsFired(int section, int wave) {
	int i;
	for (i = 0; i < L4_SPAWN_COUNT; i++) {
		if (L4_SPAWNS[i].section == section && L4_SPAWNS[i].wave == wave && !l4SpawnFired[i]) {
			return false;
		}
	}
	return true;
}

// Each gate doubles as this level's checkpoint: clearing a section repairs
// part of the player's health. It is the only thing in the level that ever
// raises playerHealth, and it can never push it above playerMaxHealth.
static const int L4_CHECKPOINT_HEAL = 300;

static void L4OpenGate(int section) {
	if (l4SectionCleared[section]) return;
	l4SectionCleared[section] = 1;
	L4SfxGateOpen();

	int before = playerHealth;
	playerHealth += L4_CHECKPOINT_HEAL;
	if (playerHealth > playerMaxHealth) playerHealth = playerMaxHealth;

	if (playerHealth > before) {
		char msg[64];
		sprintf(msg, "CHECKPOINT - GATE OPEN  (+%d HP)", playerHealth - before);
		L4Banner(msg);
	}
	else {
		L4Banner("CHECKPOINT - GATE OPEN");
	}
}

static void L4UpdateProgression(void) {
	int i;
	float worldPX = L4PlayerWorldX();

	// Which section is the player physically standing in?
	int here = (int)(worldPX / L4_SECTION_W) + 1;
	if (here < 1) here = 1;
	if (here > L4_NUM_SECTIONS) here = L4_NUM_SECTIONS;

	// Once the Warden has broken off into section 8, FINAL STAND is on
	// wherever the player is standing. Without this the player could snipe
	// the boss from the far side of the open gate and skip the whole
	// section's waves.
	if (l4Boss.regrouped && here == 7) here = 8;

	if (here != l4Section) {
		l4Section = here;
		l4Wave = 1;
		l4WaveStarted = 0;
		l4WaveTimer = 0;

		char msg[64];
		sprintf(msg, "SECTION %d - %s", l4Section, L4_SECTION_NAMES[l4Section]);
		L4Banner(msg);
	}

	// ---- mini-boss appears on entering section 7 --------------------
	if (l4Section >= 7 && !l4Boss.active && !l4Boss.defeated) {
		if (worldPX >= (float)(L4SectionStart(7) + 120)) L4SpawnBoss();
	}

	// ---- wave driver for the current section ------------------------
	int waveCount = L4WaveCount(l4Section);
	if (waveCount > 0 && l4Wave <= waveCount) {

		// Waves arm once the player is properly inside the section, so
		// enemies are never dumped on someone who just walked through a
		// gate.
		if (!l4WaveStarted) {
			if (worldPX >= (float)(L4SectionStart(l4Section) + 180)) {
				l4WaveStarted = 1;
				l4WaveTimer = 0;
			}
		}
		else {
			l4WaveTimer++;

			for (i = 0; i < L4_SPAWN_COUNT && i < L4_MAX_SPAWNS; i++) {
				const L4SpawnDef& s = L4_SPAWNS[i];
				if (l4SpawnFired[i]) continue;
				if (s.section != l4Section || s.wave != l4Wave) continue;
				if (l4WaveTimer < s.delay) continue;

				l4SpawnFired[i] = true;

				int off = s.offsetX;
				if (off > L4_SPAWN_MAX_OFFSET) off = L4_SPAWN_MAX_OFFSET;
				if (off < 60) off = 60;

				L4SpawnEnemy(s.type, (float)(L4SectionStart(s.section) + off),
					s.section, s.wave);
			}

			if (L4AllSpawnsFired(l4Section, l4Wave) &&
				L4AliveInWave(l4Section, l4Wave) == 0 &&
				l4WaveTimer > 60) {

				l4Wave++;
				l4WaveStarted = 0;
				l4WaveTimer = 0;

				if (l4Wave > waveCount) {
					char msg[64];
					sprintf(msg, "SECTION %d CLEARED", l4Section);
					L4Banner(msg);
				}
				else {
					L4Banner("INCOMING - NEXT WAVE");
				}
			}
		}
	}

	// ---- gate rules --------------------------------------------------
	bool wavesDone = (waveCount == 0) || (l4Wave > waveCount);

	if (l4Section >= 2 && l4Section <= 6) {
		if (wavesDone && L4AliveInSection(l4Section) == 0) L4OpenGate(l4Section);
	}
	else if (l4Section == 7) {
		// Section 7's gate needs the escorts cleared AND the mini-boss
		// beaten down into its second phase.
		if (wavesDone && L4AliveInSection(7) == 0 && l4Boss.phase == 2) L4OpenGate(7);
	}

	// Checked every frame, not just while l4Section == 7: once the Warden
	// has withdrawn, the player must always be able to follow it. This is
	// the failsafe that makes a sealed-in-an-empty-arena soft lock
	// impossible.
	if (l4Boss.regrouped && L4AliveInSection(7) == 0) L4OpenGate(7);

	// ---- section 8 reinforcement drip --------------------------------
	// Keeps pressure on during the boss's second phase without ever
	// becoming an endless spawner: hard cap of 4.
	if (l4Section == 8 && l4Boss.active && l4Reinforcements < 4) {
		l4ReinforceTimer++;
		if (l4ReinforceTimer >= 720) {
			l4ReinforceTimer = 0;
			l4Reinforcements++;
			float sx = L4PlayerWorldX() + ((l4Reinforcements % 2 == 0) ? 620.0f : -420.0f);
			float lo = (float)L4SectionStart(8) + 60.0f;
			float hi = (float)(L4_WORLD_WIDTH - 160);
			if (sx < lo) sx = lo;
			if (sx > hi) sx = hi;
			L4SpawnEnemy(L4_FAST, sx, 8, 99);
		}
	}

	// ---- boss dead -> the seal awakens --------------------------------
	// The final approach (two new hazards + the altar) goes live; the
	// puzzle itself starts when the player reaches the altar
	// (see L4UpdateSealApproach).
	if (l4Boss.defeated && !l4SealAwake) {
		l4SealAwake = true;
		l4SealTimer = 0;
	}
}

// =====================================================================
// 13. PER-FRAME UPDATE
// =====================================================================

void UpdateLevel4(void) {
	if (playerHealth <= 0) return;   // game.h switches to GAME OVER

	l4Frames++;
	if (l4PlayerInvuln > 0) l4PlayerInvuln--;
	if (l4HurtFlash > 0) l4HurtFlash--;
	if (l4BannerTimer > 0) l4BannerTimer--;
	if (l4ShootSfxCd > 0) l4ShootSfxCd--;

#if L4_SHOOT_SFX_ENABLED
	// Detect "a new bullet appeared in the pool" without touching
	// player.cpp's firing code at all.
	{
		int j, n = 0;
		for (j = 0; j < MAX_PLAYER_BULLETS; j++) if (p_bulletActive[j]) n++;
		if (n > l4PrevActiveBullets && l4ShootSfxCd <= 0) {
			L4SfxShoot();
			l4ShootSfxCd = 20;
		}
		l4PrevActiveBullets = n;
	}
#endif

	L4UpdateObstacles();
	L4ResolveSolids();

	// Camera: move the world, then park the player back on the anchor.
	{
		float maxCam = (float)(L4_WORLD_WIDTH - SCREEN_WIDTH);
		if (playerX > L4_ANCHOR_RIGHT && l4CamX < maxCam) {
			float d = (float)(playerX - L4_ANCHOR_RIGHT);
			float room = maxCam - l4CamX;
			if (d > room) d = room;
			l4CamX += d;
			playerX -= (int)d;
		}
		else if (playerX < L4_ANCHOR_LEFT && l4CamX > 0.0f) {
			float d = (float)(L4_ANCHOR_LEFT - playerX);
			if (d > l4CamX) d = l4CamX;
			l4CamX -= d;
			playerX += (int)d;
		}
	}

	L4CheckHazards();
	L4UpdateSealApproach();
	L4UpdateEnemies();
	L4UpdateBoss();
	L4UpdateEnemyBullets();
	L4UpdatePlayerBulletHits();
	L4UpdateProgression();
}

// =====================================================================
// 14. RENDERING
// =====================================================================

static void L4DrawBackground(void) {
	if (backgroundImg4 > 0) {
		// The supplied jungle art is the level's environment. It is drawn
		// at exactly SCREEN_WIDTH x SCREEN_HEIGHT and repeated horizontally
		// with a parallax factor, so it always covers the play area, stays
		// visually stable and never leaves a gap while scrolling.
		float parallax = l4CamX * 0.45f;
		// Both arguments must be the SAME type here. fmod(float, double)
		// is ambiguous under MSVC - it sees float,float / double,double /
		// long double,long double overloads and cannot rank the two
		// possible conversions, which is exactly VS2013's C2666. GCC
		// silently promotes and never complains, which is why this only
		// showed up on the real compiler.
		float off = (float)fmod((double)parallax, (double)SCREEN_WIDTH);
		int startX = -(int)off;

		iShowImage(startX, 0, SCREEN_WIDTH, SCREEN_HEIGHT, backgroundImg4);
		iShowImage(startX + SCREEN_WIDTH, 0, SCREEN_WIDTH, SCREEN_HEIGHT, backgroundImg4);
	}
	else {
		iSetColor(28, 54, 38);
		iFilledRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	}

	// A readable standing plane, so the ground line is never ambiguous
	// against the painted background.
	iSetColor(38, 26, 14);
	iFilledRectangle(0, GROUND_Y - 5, SCREEN_WIDTH, 5);
	iSetColor(120, 92, 48);
	iLine(0, GROUND_Y, SCREEN_WIDTH, GROUND_Y);
}

static void L4DrawObstacle(const L4Obstacle& o) {
	float sx = o.x - l4CamX;
	if (sx + o.w < -60.0f || sx > (float)SCREEN_WIDTH + 60.0f) return;

	switch (o.type) {
	case L4_OB_SOLID:
		iSetColor(104, 84, 58);
		iFilledRectangle(sx, o.y, o.w, o.h);
		iSetColor(58, 44, 28);
		iRectangle(sx, o.y, o.w, o.h);
		iLine(sx, o.y + o.h * 0.5f, sx + o.w, o.y + o.h * 0.5f);
		iSetColor(146, 122, 84);
		iFilledRectangle(sx + 4, o.y + o.h - 12, o.w - 8, 8);
		break;

	case L4_OB_LOWBAR:
		// Hanging thorn beam - crouch under it.
		iSetColor(72, 54, 30);
		iFilledRectangle(sx, o.y, o.w, o.h);
		iSetColor(34, 24, 12);
		iRectangle(sx, o.y, o.w, o.h);
		iSetColor(180, 200, 120);
		{
			int t;
			for (t = 10; t < o.w - 6; t += 22) {
				iLine(sx + t, o.y, sx + t + 8, o.y - 14);
				iLine(sx + t + 8, o.y - 14, sx + t + 16, o.y);
			}
		}
		iSetColor(230, 210, 90);
		iText(sx + o.w * 0.5f - 26, o.y + 14, (char*)"CROUCH", GLUT_BITMAP_HELVETICA_12);
		break;

	case L4_OB_MOVER:
		iSetColor(96, 96, 108);
		iFilledRectangle(sx, o.y, o.w, o.h);
		iSetColor(220, 90, 60);
		iRectangle(sx, o.y, o.w, o.h);
		iRectangle(sx + 5, o.y + 5, o.w - 10, o.h - 10);
		if (o.axis == 2) {
			// crusher rail
			iSetColor(70, 70, 78);
			iLine(sx + o.w * 0.5f, o.y + o.h, sx + o.w * 0.5f, SCREEN_HEIGHT);
			iSetColor(230, 120, 70);
			iFilledRectangle(sx, o.y, o.w, 8);
		}
		break;

	case L4_OB_SPIKE: {
		int t;
		iSetColor(58, 44, 30);
		iFilledRectangle(sx, o.y, o.w, 6);
		iSetColor(214, 224, 236);
		for (t = 0; t < o.w - 12; t += 18) {
			double xs[3], ys[3];
			xs[0] = sx + t;       ys[0] = o.y + 4;
			xs[1] = sx + t + 9;   ys[1] = o.y + o.h;
			xs[2] = sx + t + 18;  ys[2] = o.y + 4;
			iFilledPolygon(xs, ys, 3);
		}
		iSetColor(150, 40, 40);
		iLine(sx, o.y + o.h + 4, sx + o.w, o.y + o.h + 4);
		break;
	}

	case L4_OB_BLADE:
		if (o.axis != 1) {
			iSetColor(60, 60, 60);
			iLine(o.baseX - l4CamX + o.w * 0.5f, o.baseY, sx + o.w * 0.5f, o.y + o.h * 0.5f);
		}
		iSetColor(196, 60, 52);
		iFilledCircle(sx + o.w * 0.5f, o.y + o.h * 0.5f, o.w * 0.5f);
		iSetColor(255, 220, 140);
		iCircle(sx + o.w * 0.5f, o.y + o.h * 0.5f, o.w * 0.5f);
		iSetColor(255, 255, 255);
		iLine(sx, o.y + o.h * 0.5f, sx + o.w, o.y + o.h * 0.5f);
		iLine(sx + o.w * 0.5f, o.y, sx + o.w * 0.5f, o.y + o.h);
		break;

	default: break;
	}
}

static void L4DrawGate(const L4Obstacle& o) {
	if (l4SectionCleared[o.section]) return;

	float sx = o.x - l4CamX;
	if (sx + o.w < -60.0f || sx > (float)SCREEN_WIDTH + 60.0f) return;

	iSetColor(20, 40, 46);
	iFilledRectangle(sx, o.y, o.w, o.h);

	iSetColor(0, 220, 200);
	iRectangle(sx, o.y, o.w, o.h);
	{
		int t;
		for (t = 0; t < o.h; t += 26) {
			iLine(sx, o.y + t, sx + o.w, o.y + t + 13);
		}
	}
	iSetColor(0, 255, 220);
	iText(sx - 56, o.y + o.h + 12, (char*)"SEALED", GLUT_BITMAP_HELVETICA_12);
}

static void L4DrawEnemy(const L4Enemy& e) {
	float sx = e.x - l4CamX;
	if (sx + e.w < -80.0f || sx > (float)SCREEN_WIDTH + 80.0f) return;

	// Colour-coded ground marker: the three light enemy types share the
	// project's existing sprite sheet, so the marker (and their differing
	// silhouette size) is what makes them tell apart at a glance.
	switch (e.type) {
	case L4_CHASER: iSetColor(220, 70, 60);  break;
	case L4_RANGED: iSetColor(170, 90, 230); break;
	case L4_FAST:   iSetColor(60, 220, 230); break;
	default:        iSetColor(200, 200, 200); break;
	}
	iFilledRectangle(sx + 6, e.y - 6, e.w - 12, 5);

	int frame = e.animFrame % TOTAL_ENEMY_FRAMES;
	if (enemyPic[frame] > 0) {
		iShowImage((int)sx, (int)e.y, e.w, e.h, enemyPic[frame]);
	}
	else {
		iFilledRectangle(sx, e.y, e.w, e.h);
	}

	// Type badge + state tells.
	if (e.type == L4_FAST) {
		iSetColor(60, 220, 230);
		iLine(sx - 10, e.y + e.h * 0.6f, sx - 30, e.y + e.h * 0.6f);
		iLine(sx - 10, e.y + e.h * 0.4f, sx - 26, e.y + e.h * 0.4f);
	}
	if (e.type == L4_RANGED && e.aiState == 1) {
		// Telegraph: a charging muzzle glow before the shot lands.
		iSetColor(255, 200, 60);
		float mx = e.facingLeft ? (sx - 12.0f) : (sx + e.w + 12.0f);
		iFilledCircle(mx, e.y + e.h * 0.52f, 9);
	}
	if (e.hitFlash > 0) {
		iSetColor(255, 255, 255);
		iRectangle(sx, e.y, e.w, e.h);
		iRectangle(sx + 2, e.y + 2, e.w - 4, e.h - 4);
	}

	// Small health pip above each enemy.
	{
		float w = (float)e.w - 14.0f;
		float f = w * ((float)e.hp / (float)e.maxHp);
		if (f < 0.0f) f = 0.0f;
		iSetColor(30, 30, 30);
		iFilledRectangle(sx + 7, e.y + e.h + 8, w, 5);
		iSetColor(230, 80, 60);
		iFilledRectangle(sx + 7, e.y + e.h + 8, f, 5);
	}
}

static void L4DrawBoss(void) {
	if (!l4Boss.active) return;

	float sx = l4Boss.x - l4CamX;
	if (sx + l4Boss.w < -200.0f || sx > (float)SCREEN_WIDTH + 200.0f) return;

	// Shadow, so the leap reads clearly.
	iSetColor(20, 20, 20);
	iFilledRectangle(sx + 16, GROUND_Y - 6, l4Boss.w - 32, 6);

	if (tankImg > 0) {
		iShowImage((int)sx, (int)l4Boss.y, l4Boss.w, l4Boss.h, tankImg);
	}
	else {
		iSetColor(120, 50, 45);
		iFilledRectangle(sx, l4Boss.y, l4Boss.w, l4Boss.h);
	}

	// Armour plating - visually separates it from every other enemy.
	iSetColor(l4Boss.phase == 2 ? 255 : 200, l4Boss.phase == 2 ? 120 : 170, 60);
	iRectangle(sx, l4Boss.y, l4Boss.w, l4Boss.h);
	iRectangle(sx + 6, l4Boss.y + 6, l4Boss.w - 12, l4Boss.h - 12);

	if (l4Boss.state == 2 || l4Boss.state == 4) {
		// Guard stance: an obvious shield bubble telling the player their
		// shots are being soaked right now.
		iSetColor(90, 200, 255);
		iCircle(sx + l4Boss.w * 0.5f, l4Boss.y + l4Boss.h * 0.5f, l4Boss.w * 0.62f);
		iCircle(sx + l4Boss.w * 0.5f, l4Boss.y + l4Boss.h * 0.5f, l4Boss.w * 0.58f);
		iSetColor(160, 230, 255);
		iText(sx + 40, l4Boss.y + l4Boss.h + 16, (char*)"GUARDING", GLUT_BITMAP_HELVETICA_12);
	}
	if (l4Boss.state == 3) {
		iSetColor(255, 140, 60);
		iText(sx + 50, l4Boss.y + l4Boss.h + 16, (char*)"SLAM!", GLUT_BITMAP_HELVETICA_18);
	}
	if (l4Boss.stomp > 0) {
		iSetColor(255, 180, 90);
		iLine(0, GROUND_Y + 2, SCREEN_WIDTH, GROUND_Y + 2);
	}
	if (l4Boss.hitFlash > 0) {
		iSetColor(255, 255, 255);
		iRectangle(sx + 3, l4Boss.y + 3, l4Boss.w - 6, l4Boss.h - 6);
	}
}

static void L4DrawBullets(void) {
	int i;
	for (i = 0; i < L4_MAX_EBULLETS; i++) {
		const L4Bullet& b = l4Bullets[i];
		if (!b.active) continue;

		float sx = b.x - l4CamX;
		if (sx + b.w < -40.0f || sx > (float)SCREEN_WIDTH + 40.0f) continue;

		if (b.kind == 2) {
			// Ground shockwave.
			iSetColor(255, 160, 60);
			iFilledRectangle(sx, b.y, b.w, b.h);
			iSetColor(255, 240, 180);
			iRectangle(sx, b.y, b.w, b.h);
			iLine(sx, b.y + b.h, sx + b.w * 0.5f, b.y + b.h + 12);
			iLine(sx + b.w * 0.5f, b.y + b.h + 12, sx + b.w, b.y + b.h);
		}
		else if (b.kind == 1) {
			iSetColor(255, 110, 60);
			iFilledCircle(sx + b.w * 0.5f, b.y + b.h * 0.5f, b.w * 0.5f);
			iSetColor(255, 230, 170);
			iCircle(sx + b.w * 0.5f, b.y + b.h * 0.5f, b.w * 0.5f);
		}
		else {
			if (enemyBulletImage > 0) {
				iShowImage((int)sx, (int)b.y, b.w, b.h, enemyBulletImage);
			}
			else {
				iSetColor(200, 80, 220);
				iFilledCircle(sx + b.w * 0.5f, b.y + b.h * 0.5f, b.w * 0.5f);
			}
		}
	}
}

static void L4DrawSignposts(void) {
	int s;
	for (s = 1; s <= L4_NUM_SECTIONS; s++) {
		float sx = (float)L4SectionStart(s) + 30.0f - l4CamX;
		if (sx < -100.0f || sx > (float)SCREEN_WIDTH + 100.0f) continue;

		iSetColor(96, 72, 42);
		iFilledRectangle(sx, GROUND_Y, 8, 86);
		iSetColor(140, 112, 66);
		iFilledRectangle(sx - 26, GROUND_Y + 86, 62, 28);
		iSetColor(40, 30, 18);
		iRectangle(sx - 26, GROUND_Y + 86, 62, 28);

		char lbl[8];
		sprintf(lbl, "S%d", s);
		iSetColor(250, 240, 210);
		iText(sx - 12, GROUND_Y + 95, lbl, GLUT_BITMAP_HELVETICA_12);
	}
}

// ---------------------------------------------------------------------
// HUD
// ---------------------------------------------------------------------
// The panel occupies y = 582..650 only. The player's jump apex puts the
// top of the sprite at y = 575, so the HUD never covers the character or
// any enemy, and the whole play area stays clear.
static void L4DrawHUD(void) {
	char buf[96];

	iSetColor(12, 20, 24);
	iFilledRectangle(0, 582, SCREEN_WIDTH, SCREEN_HEIGHT - 582);
	iSetColor(0, 180, 170);
	iLine(0, 582, SCREEN_WIDTH, 582);

	// ---- player health ----
	{
		int barX = 20, barY = 626, barW = 230, barH = 15;
		float f = (float)barW * ((float)playerHealth / (float)playerMaxHealth);
		if (f < 0.0f) f = 0.0f;

		iSetColor(45, 45, 45);
		iFilledRectangle(barX, barY, barW, barH);
		if (playerHealth > playerMaxHealth / 2)      iSetColor(0, 220, 90);
		else if (playerHealth > playerMaxHealth / 4) iSetColor(235, 200, 40);
		else                                         iSetColor(230, 60, 50);
		iFilledRectangle(barX, barY, f, barH);
		iSetColor(180, 180, 180);
		iRectangle(barX, barY, barW, barH);

		sprintf(buf, "HEALTH  %d / %d", playerHealth, playerMaxHealth);
		iSetColor(230, 240, 240);
		iText(barX + barW + 12, barY + 2, buf, GLUT_BITMAP_HELVETICA_12);

		if (l4PlayerInvuln > 0) {
			iSetColor(120, 200, 255);
			iText(barX + barW + 12, barY - 14, (char*)"SHIELDED", GLUT_BITMAP_HELVETICA_12);
		}
	}

	// ---- time / score ----
	{
		int secs = l4Frames / 62;
		sprintf(buf, "TIME  %02d:%02d      SCORE  %d", secs / 60, secs % 60, l4Score);
		iSetColor(190, 210, 215);
		iText(20, 590, buf, GLUT_BITMAP_HELVETICA_12);
	}

	// ---- section / wave ----
	{
		sprintf(buf, "SECTION %d / %d  -  %s", l4Section, L4_NUM_SECTIONS,
			L4_SECTION_NAMES[l4Section]);
		iSetColor(255, 220, 120);
		iText(330, 628, buf, GLUT_BITMAP_HELVETICA_18);

		int waveCount = L4WaveCount(l4Section);
		int shown = (l4Wave > waveCount) ? waveCount : l4Wave;
		if (waveCount > 0) {
			sprintf(buf, "WAVE %d / %d        ENEMIES REMAINING: %d",
				shown, waveCount, L4AliveInSection(l4Section));
		}
		else {
			sprintf(buf, "NO HOSTILES - CLEAR THE PATH");
		}
		iSetColor(220, 230, 230);
		iText(330, 606, buf, GLUT_BITMAP_HELVETICA_12);

		if (!l4SectionCleared[l4Section] && l4Section >= 2 && l4Section <= 7) {
			iSetColor(0, 220, 200);
			iText(330, 590, (char*)"OBJECTIVE: clear this section to unseal the gate",
				GLUT_BITMAP_HELVETICA_12);
		}
		else if (l4Section == 8) {
			iSetColor(255, 150, 90);
			iText(330, 590, (char*)"OBJECTIVE: bring down the Grove Warden",
				GLUT_BITMAP_HELVETICA_12);
		}
	}

	// ---- mini-boss health (only while it is on the field) ----
	if (l4Boss.active) {
		int barX = 790, barY = 606, barW = 390, barH = 16;
		float f = (float)barW * ((float)l4Boss.hp / (float)l4Boss.maxHp);
		if (f < 0.0f) f = 0.0f;

		sprintf(buf, "GROVE WARDEN   [ PHASE %d ]", l4Boss.phase);
		iSetColor(255, 120, 80);
		iText(barX, barY + 22, buf, GLUT_BITMAP_HELVETICA_18);

		iSetColor(45, 20, 20);
		iFilledRectangle(barX, barY, barW, barH);
		iSetColor(l4Boss.phase == 2 ? 255 : 210, 60, 50);
		iFilledRectangle(barX, barY, f, barH);
		iSetColor(230, 200, 160);
		iRectangle(barX, barY, barW, barH);

		sprintf(buf, "%d / %d", l4Boss.hp, l4Boss.maxHp);
		iSetColor(240, 220, 200);
		iText(barX + barW / 2 - 28, barY + 1, buf, GLUT_BITMAP_HELVETICA_12);
	}
	else {
		iSetColor(140, 200, 190);
		iText(880, 628, (char*)"LEVEL 4", GLUT_BITMAP_HELVETICA_18);
		iSetColor(120, 150, 150);
		iText(880, 606, (char*)"survive the grove, break the seal", GLUT_BITMAP_HELVETICA_12);
	}

	// ---- controls reminder, well clear of the ground plane ----
	iSetColor(215, 225, 225);
	iText(300, 30, (char*)"A / D  move      W  jump      S  crouch      F or LEFT-CLICK  shoot      B  menu      ESC  exit",
		GLUT_BITMAP_HELVETICA_12);
}

static void L4DrawBanner(void) {
	if (l4BannerTimer <= 0) return;

	iSetColor(10, 18, 22);
	iFilledRectangle(SCREEN_WIDTH / 2 - 260, 468, 520, 44);
	iSetColor(0, 220, 200);
	iRectangle(SCREEN_WIDTH / 2 - 260, 468, 520, 44);
	iSetColor(255, 235, 170);
	iText(SCREEN_WIDTH / 2 - 240, 482, l4BannerText, GLUT_BITMAP_HELVETICA_18);
}

void RenderLevel4(void) {
	int i;

	L4DrawBackground();
	L4DrawSignposts();

	for (i = 0; i < l4ObstacleCount; i++) {
		if (l4Obstacles[i].type != L4_OB_GATE) L4DrawObstacle(l4Obstacles[i]);
	}

	L4DrawSealApproach();

	for (i = 0; i < L4_MAX_ENEMIES; i++) {
		if (l4Enemies[i].active) L4DrawEnemy(l4Enemies[i]);
	}

	L4DrawBoss();
	L4DrawBullets();

	// The project's own player renderer, called unmodified.
	RenderPlayer();

	// Damage vignette.
	if (l4HurtFlash > 0) {
		iSetColor(200, 40, 40);
		iRectangle(2, 2, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 4);
		iRectangle(5, 5, SCREEN_WIDTH - 10, SCREEN_HEIGHT - 10);
		iRectangle(8, 8, SCREEN_WIDTH - 16, SCREEN_HEIGHT - 16);
	}

	// Gates last, so the barrier reads on top of everything behind it.
	for (i = 0; i < l4ObstacleCount; i++) {
		if (l4Obstacles[i].type == L4_OB_GATE) L4DrawGate(l4Obstacles[i]);
	}

	L4DrawBanner();
	L4DrawHUD();
}
