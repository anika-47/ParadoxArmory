// Provides the single, real definition of iGraphics.h's global variables.
// iGraphics.h only *declares* them as `extern` now (see the header for why),
// so exactly one .cpp file needs to actually define/allocate them - this one.
#include "iGraphics.h"

int iScreenHeight, iScreenWidth;
int iMouseX, iMouseY;
int ifft = 0;
void (*iAnimFunction[10])(void) = { 0 };
int iAnimCount = 0;
int iAnimDelays[10];
int iAnimPause[10];

unsigned int keyPressed[512];
unsigned int specialKeyPressed[512];
