#ifndef AUDIO_H
#define AUDIO_H

#include "defines.h"
#include "variable.h"

// Reused as-is from Game-Development--main/Project1/Audio.hpp: same MCI
// (Windows Media Control Interface) calls, same alias-based open/play/stop
// pattern. Only the asset paths and function names touching this project's
// states were adjusted; the audio approach itself is not reinvented.

inline void StopMusic() {
	mciSendString("stop bgm", NULL, 0, NULL);
	mciSendString("close bgm", NULL, 0, NULL);
	mciSendString("stop mm", NULL, 0, NULL);
	mciSendString("close mm", NULL, 0, NULL);
	mciSendString("stop gom", NULL, 0, NULL);
	mciSendString("close gom", NULL, 0, NULL);
}

inline void PlayMenuMusic() {
	StopMusic();
	if (musicOn) {
		mciSendString("open \"Audios/menu.mp3\" type mpegvideo alias mm", NULL, 0, NULL);
		mciSendString("play mm repeat", NULL, 0, NULL);
	}
}

inline void PlayBackgroundMusic() {
	StopMusic();
	if (musicOn) {
		mciSendString("open \"Audios/background.mp3\" type mpegvideo alias bgm", NULL, 0, NULL);
		mciSendString("play bgm repeat", NULL, 0, NULL);
	}
}

inline void PlayGameOverMusic() {
	StopMusic();
	if (musicOn) {
		mciSendString("open \"Audios/gameover.mp3\" type mpegvideo alias gom", NULL, 0, NULL);
		mciSendString("play gom", NULL, 0, NULL);
	}
}

inline void PlayCollectSound() {
	if (sfxOn) {
		mciSendString("close csfx", NULL, 0, NULL);
		mciSendString("open \"Audios/collect.mp3\" type mpegvideo alias csfx", NULL, 0, NULL);
		mciSendString("play csfx", NULL, 0, NULL);
	}
}

#endif // AUDIO_H
