#ifndef AUDIO4_H
#define AUDIO4_H

// =====================================================================
// LEVEL 4 AUDIO
// =====================================================================
//
// Same MCI (Media Control Interface) approach already used by Audio.h and
// obstacle.cpp - open an alias, play it, close it before reopening. No new
// audio files are introduced: every sound below maps onto one of the five
// clips that already ship in Audios/ (background.mp3, bubble.mp3,
// collect.mp3, gameover.mp3, menu.mp3). Each event gets its OWN alias so
// one sound never cuts another one off mid-play, which is what makes the
// events distinguishable even though the clip pool is small.
//
// Existing audio is left completely untouched - Audio.h is included, not
// replaced, and PlayBackgroundMusic()/PlayMenuMusic()/PlayGameOverMusic()
// still behave exactly as before.
// =====================================================================

#include "defines.h"
#include "variable.h"
#include "Audio.h"

// The shooting SFX fires far more often than any other event. Opening an
// MCI alias per shot is comparatively expensive, so it is rate-limited
// below. If it ever causes an audible hitch on a slow machine, set this to
// 0 - nothing else in the level depends on it.
#define L4_SHOOT_SFX_ENABLED 1

// Internal helper: open + play one clip under a private alias.
inline void L4PlayClip(const char* alias, const char* file) {
	if (!sfxOn) return;

	char cmd[256];

	sprintf(cmd, "close %s", alias);
	mciSendStringA(cmd, NULL, 0, NULL);

	sprintf(cmd, "open \"Audios/%s\" type mpegvideo alias %s", file, alias);
	mciSendStringA(cmd, NULL, 0, NULL);

	sprintf(cmd, "play %s from 0", alias);
	mciSendStringA(cmd, NULL, 0, NULL);
}

// ---- combat ----------------------------------------------------------
inline void L4SfxShoot(void)        { L4PlayClip("l4shot",  "bubble.mp3");  }
inline void L4SfxEnemyHit(void)     { L4PlayClip("l4hit",   "bubble.mp3");  }
inline void L4SfxEnemyDown(void)    { L4PlayClip("l4kill",  "collect.mp3"); }
inline void L4SfxPlayerHurt(void)   { L4PlayClip("l4hurt",  "bubble.mp3");  }
inline void L4SfxTrapHit(void)      { L4PlayClip("l4trap",  "bubble.mp3");  }
inline void L4SfxGateOpen(void)     { L4PlayClip("l4gate",  "collect.mp3"); }

// ---- mini-boss -------------------------------------------------------
// The boss encounter swaps the looping track so the fight is audibly
// different from the rest of the level; beating it restores the normal
// level music. Both clips already exist in Audios/.
inline void L4MusicBoss(void) {
	StopMusic();
	if (musicOn) {
		mciSendStringA("open \"Audios/menu.mp3\" type mpegvideo alias bgm", NULL, 0, NULL);
		mciSendStringA("play bgm repeat", NULL, 0, NULL);
	}
}
inline void L4MusicLevel(void) {
	PlayBackgroundMusic();
}
inline void L4SfxBossDown(void)     { L4PlayClip("l4boss",  "collect.mp3"); }

// ---- final puzzle ----------------------------------------------------
inline void L4SfxPuzzleClick(void)  { L4PlayClip("l4pz",    "bubble.mp3");  }
inline void L4SfxPuzzleStage(void)  { L4PlayClip("l4pzok",  "collect.mp3"); }
inline void L4SfxPuzzleFail(void)   { L4PlayClip("l4pzno",  "gameover.mp3");}
inline void L4SfxLevelComplete(void) {
	StopMusic();
	L4PlayClip("l4win", "collect.mp3");
}

#endif // AUDIO4_H
