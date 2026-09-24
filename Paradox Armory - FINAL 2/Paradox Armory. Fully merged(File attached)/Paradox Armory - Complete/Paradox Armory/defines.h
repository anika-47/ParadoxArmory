#ifndef DEFINES_H
#define DEFINES_H

#pragma comment(lib, "glut32.lib")
#pragma comment(lib, "glaux.lib")
#pragma comment(lib, "winmm.lib")

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <mmsystem.h>
#include "iGraphics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gameConfig.h"

#define STATE_MAIN_MENU    1
#define STATE_LEVEL_SELECT 2
#define STATE_PLAY         3
#define STATE_INSTRUCTION  4
#define STATE_ABOUT        5
#define STATE_PUZZLE       6
#define STATE_GAME_OVER    7
#define STATE_LEVEL_WIN    8
#define STATE_PART_CONFIRM  9   // Level 3 merge: "proceed to next part?" prompt
#define STATE_PUZZLE4      10   // Level 4: the three-stage final seal (see puzzle4.h)

// Button Structure
typedef struct {
	int x, y, width, height;
	char text[40];
} Button;

#define MAX_NAME_LEN 20

#endif