//
//  Original Author: S. M. Shahriar Nirjon
//
//  Last Modified by: Mr. Mohammad Imrul Jubair [Assistant Professor (AUST CSE)]
//  Last Updated: 16 December 2017 
//
//  Version: 4.0
//

#ifndef IGRAPHICS_H
#define IGRAPHICS_H

# include <stdio.h>
# include <stdlib.h>
#pragma comment(lib, "glut32.lib")
#pragma comment(lib, "glaux.lib")
#include "glut.h"
#include <time.h>
#include <math.h>
#include <windows.h>
#include "glaux.h"

// STB image header included without definition macro to prevent duplicate symbol linker errors
# include "stb_image.h"

// NOTE: Changed from C++17 "inline" variables to classic extern-declaration +
// single-definition (in iGraphicsGlobals.cpp) so this header works with older
// toolsets (e.g. VS2013/v120) that predate C++17 inline variables.
extern int iScreenHeight, iScreenWidth;
extern int iMouseX, iMouseY;
extern int ifft;
extern void (*iAnimFunction[10])(void);
extern int iAnimCount;
extern int iAnimDelays[10];
extern int iAnimPause[10];

extern unsigned int keyPressed[512];
extern unsigned int specialKeyPressed[512];

void iDraw();
void fixedUpdate();
void iMouseMove(int, int);
void iPassiveMouseMove(int, int);
void iMouse(int button, int state, int x, int y);
void iKeyboard(unsigned char key);
void iSpecialKeyboard(unsigned char key);

static void __stdcall iA0(HWND, unsigned int, unsigned int, unsigned long) { if (!iAnimPause[0])iAnimFunction[0](); }
static void __stdcall iA1(HWND, unsigned int, unsigned int, unsigned long) { if (!iAnimPause[1])iAnimFunction[1](); }
static void __stdcall iA2(HWND, unsigned int, unsigned int, unsigned long) { if (!iAnimPause[2])iAnimFunction[2](); }
static void __stdcall iA3(HWND, unsigned int, unsigned int, unsigned long) { if (!iAnimPause[3])iAnimFunction[3](); }
static void __stdcall iA4(HWND, unsigned int, unsigned int, unsigned long) { if (!iAnimPause[4])iAnimFunction[4](); }
static void __stdcall iA5(HWND, unsigned int, unsigned int, unsigned long) { if (!iAnimPause[5])iAnimFunction[5](); }
static void __stdcall iA6(HWND, unsigned int, unsigned int, unsigned long) { if (!iAnimPause[6])iAnimFunction[6](); }
static void __stdcall iA7(HWND, unsigned int, unsigned int, unsigned long) { if (!iAnimPause[7])iAnimFunction[7](); }
static void __stdcall iA8(HWND, unsigned int, unsigned int, unsigned long) { if (!iAnimPause[8])iAnimFunction[8](); }
static void __stdcall iA9(HWND, unsigned int, unsigned int, unsigned long) { if (!iAnimPause[9])iAnimFunction[9](); }
static void __stdcall keypressHandler(HWND, unsigned int, unsigned int, unsigned long) { fixedUpdate(); }

inline int isKeyPressed(unsigned char key) {
	return keyPressed[key];
}

inline int isSpecialKeyPressed(unsigned char key) {
	return specialKeyPressed[key];
}

inline int iSetTimer(int msec, void (*f)(void))
{
	int i = iAnimCount;

	if (iAnimCount >= 10) { printf("Error: Maximum number of already timer used.\n"); return -1; }

	iAnimFunction[i] = f;
	iAnimDelays[i] = msec;
	iAnimPause[i] = 0;

	if (iAnimCount == 0) SetTimer(0, 0, msec, iA0);
	if (iAnimCount == 1) SetTimer(0, 0, msec, iA1);
	if (iAnimCount == 2) SetTimer(0, 0, msec, iA2);
	if (iAnimCount == 3) SetTimer(0, 0, msec, iA3);
	if (iAnimCount == 4) SetTimer(0, 0, msec, iA4);

	if (iAnimCount == 5) SetTimer(0, 0, msec, iA5);
	if (iAnimCount == 6) SetTimer(0, 0, msec, iA6);
	if (iAnimCount == 7) SetTimer(0, 0, msec, iA7);
	if (iAnimCount == 8) SetTimer(0, 0, msec, iA8);
	if (iAnimCount == 9) SetTimer(0, 0, msec, iA9);
	iAnimCount++;

	return iAnimCount - 1;
}

inline void iPauseTimer(int index) {
	if (index >= 0 && index < iAnimCount) {
		iAnimPause[index] = 1;
	}
}

inline void iResumeTimer(int index) {
	if (index >= 0 && index < iAnimCount) {
		iAnimPause[index] = 0;
	}
}

inline void iShowBMP2(int x, int y, char filename[], int ignoreColor)
{
	AUX_RGBImageRec *TextureImage;
	TextureImage = auxDIBImageLoadA(filename);

	int i, j;
	int width = TextureImage->sizeX;
	int height = TextureImage->sizeY;
	int nPixels = width * height;
	int *rgPixels = new int[nPixels];

	for (i = 0, j = 0; i < nPixels; i++, j += 3)
	{
		int rgb = 0;
		for (int k = 2; k >= 0; k--)
		{
			rgb = ((rgb << 8) | TextureImage->data[j + k]);
		}

		rgPixels[i] = (rgb == ignoreColor) ? 0 : 255;
		rgPixels[i] = ((rgPixels[i] << 24) | rgb);
	}

	glRasterPos2f((GLfloat)x, (GLfloat)y);
	glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgPixels);

	delete[]rgPixels;
	free(TextureImage->data);
	free(TextureImage);
}

inline void iShowBMP(int x, int y, char filename[])
{
	iShowBMP2(x, y, filename, -1 /* ignoreColor */);
}

inline unsigned int iLoadImage(char filename[])
{
	int width, height, bpp;

	unsigned int texture;

	BYTE* data(0);
	data = stbi_load(filename, &width, &height, &bpp, 4);

	// stbi_load() returns NULL (and leaves width/height undefined) when the
	// file is missing or fails to decode. Previously this fell through to
	// glGenTextures()/glTexImage2D() anyway, which ALWAYS hands back a
	// nonzero texture id - so every "if (img > 0)" check in the game kept
	// treating a failed load as a success (e.g. the background image path
	// fallback in iMain.cpp never triggered). Bail out and return 0 so
	// callers can actually detect the failure.
	if (data == 0) {
		fprintf(stderr, "iLoadImage: failed to load \"%s\"\n", filename);
		return 0;
	}

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D,
		0,
		GL_RGBA,
		width, height,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		data);

	stbi_image_free(data);

	return texture;
}

inline void iShowImage(int x, int y, int width, int height, unsigned int texture)
{
	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glBegin(GL_QUADS);

	glTexCoord2f(0, 0);
	glVertex2f((GLfloat)x, (GLfloat)y);

	glTexCoord2f(1, 0);
	glVertex2f((GLfloat)(x + width), (GLfloat)y);

	glTexCoord2f(1, -1);
	glVertex2f((GLfloat)(x + width), (GLfloat)(y + height));

	glTexCoord2f(0, -1);
	glVertex2f((GLfloat)x, (GLfloat)(y + height));

	glEnd();

	glDisable(GL_TEXTURE_2D);
}

inline void iGetPixelColor(int cursorX, int cursorY, int rgb[])
{
	GLubyte pixel[3];
	glReadPixels(cursorX, cursorY, 1, 1,
		GL_RGB, GL_UNSIGNED_BYTE, (void *)pixel);

	rgb[0] = pixel[0];
	rgb[1] = pixel[1];
	rgb[2] = pixel[2];
}

inline void iText(GLdouble x, GLdouble y, char *str, void* font = GLUT_BITMAP_8_BY_13)
{
	glRasterPos3d(x, y, 0);
	int i;
	for (i = 0; str[i]; i++) {
		glutBitmapCharacter(font, str[i]);
	}
}

inline void iPoint(double x, double y, int size = 0)
{
	int i, j;
	glBegin(GL_POINTS);
	glVertex2f((GLfloat)x, (GLfloat)y);
	for (i = (int)(x - size); i < x + size; i++)
	{
		for (j = (int)(y - size); j < y + size; j++)
		{
			glVertex2f((GLfloat)i, (GLfloat)j);
		}
	}
	glEnd();
}

inline void iLine(double x1, double y1, double x2, double y2)
{
	glBegin(GL_LINE_STRIP);
	glVertex2f((GLfloat)x1, (GLfloat)y1);
	glVertex2f((GLfloat)x2, (GLfloat)y2);
	glEnd();
}

inline void iFilledPolygon(double x[], double y[], int n)
{
	int i;
	if (n < 3)return;
	glBegin(GL_POLYGON);
	for (i = 0; i < n; i++) {
		glVertex2f((GLfloat)x[i], (GLfloat)y[i]);
	}
	glEnd();
}

inline void iPolygon(double x[], double y[], int n)
{
	int i;
	if (n < 3)return;
	glBegin(GL_LINE_STRIP);
	for (i = 0; i < n; i++) {
		glVertex2f((GLfloat)x[i], (GLfloat)y[i]);
	}
	glVertex2f((GLfloat)x[0], (GLfloat)y[0]);
	glEnd();
}

inline void iRectangle(double left, double bottom, double dx, double dy)
{
	double x1, y1, x2, y2;

	x1 = left;
	y1 = bottom;
	x2 = x1 + dx;
	y2 = y1 + dy;

	iLine(x1, y1, x2, y1);
	iLine(x2, y1, x2, y2);
	iLine(x2, y2, x1, y2);
	iLine(x1, y2, x1, y1);
}

inline void iFilledRectangle(double left, double bottom, double dx, double dy)
{
	double xx[4], yy[4];
	double x1, y1, x2, y2;

	x1 = left;
	y1 = bottom;
	x2 = x1 + dx;
	y2 = y1 + dy;

	xx[0] = x1;
	yy[0] = y1;
	xx[1] = x2;
	yy[1] = y1;
	xx[2] = x2;
	yy[2] = y2;
	xx[3] = x1;
	yy[3] = y2;

	iFilledPolygon(xx, yy, 4);
}

inline void iFilledCircle(double x, double y, double r, int slices = 100)
{
	double t, PI = acos(-1.0), dt, x1, y1, xp, yp;
	dt = 2 * PI / slices;
	xp = x + r;
	yp = y;
	glBegin(GL_POLYGON);
	for (t = 0; t <= 2 * PI; t += dt)
	{
		x1 = x + r * cos(t);
		y1 = y + r * sin(t);

		glVertex2f((GLfloat)xp, (GLfloat)yp);
		xp = x1;
		yp = y1;
	}
	glEnd();
}

inline void iCircle(double x, double y, double r, int slices = 100)
{
	double t, PI = acos(-1.0), dt, x1, y1, xp, yp;
	dt = 2 * PI / slices;
	xp = x + r;
	yp = y;
	for (t = 0; t <= 2 * PI; t += dt)
	{
		x1 = x + r * cos(t);
		y1 = y + r * sin(t);
		iLine(xp, yp, x1, y1);
		xp = x1;
		yp = y1;
	}
}

inline void iEllipse(double x, double y, double a, double b, int slices = 100)
{
	double t, PI = acos(-1.0), dt, x1, y1, xp, yp;
	dt = 2 * PI / slices;
	xp = x + a;
	yp = y;
	for (t = 0; t <= 2 * PI; t += dt)
	{
		x1 = x + a * cos(t);
		y1 = y + b * sin(t);
		iLine(xp, yp, x1, y1);
		xp = x1;
		yp = y1;
	}
}

inline void iFilledEllipse(double x, double y, double a, double b, int slices = 100)
{
	double t, PI = acos(-1.0), dt, x1, y1, xp, yp;
	dt = 2 * PI / slices;
	xp = x + a;
	yp = y;
	glBegin(GL_POLYGON);
	for (t = 0; t <= 2 * PI; t += dt)
	{
		x1 = x + a * cos(t);
		y1 = y + b * sin(t);
		glVertex2f((GLfloat)xp, (GLfloat)yp);
		xp = x1;
		yp = y1;
	}
	glEnd();
}

inline void iRotate(double x, double y, double degree)
{
	glPushMatrix();
	glTranslatef((GLfloat)x, (GLfloat)y, 0.0f);
	glRotatef((GLfloat)degree, 0, 0, 1.0f);
	glTranslatef((GLfloat)-x, (GLfloat)-y, 0.0f);
}

inline void iUnRotate()
{
	glPopMatrix();
}

inline void iSetColor(double r, double g, double b)
{
	double mmx;
	mmx = r;
	if (g > mmx)mmx = g;
	if (b > mmx)mmx = b;
	mmx = 255;
	if (mmx > 0) {
		r /= mmx;
		g /= mmx;
		b /= mmx;
	}
	glColor3f((GLfloat)r, (GLfloat)g, (GLfloat)b);
}

inline void iDelay(int sec)
{
	int t1, t2;
	t1 = (int)time(0);
	while (1) {
		t2 = (int)time(0);
		if (t2 - t1 >= sec)
			break;
	}
}

inline void iDelayMS(int msec)
{
	clock_t end;
	end = clock() + msec * CLOCKS_PER_SEC / 1000;
	while (end > clock());
}

inline void iClear()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glClearColor(0, 0, 0, 0);
	glFlush();
}

inline void displayFF(void) {
	iDraw();
	glutSwapBuffers();
}

inline void animFF(void)
{
	if (ifft == 0) {
		ifft = 1;
		iClear();
	}
	glutPostRedisplay();
}

inline void keyboardHandlerUp1FF(unsigned char key, int x, int y)
{
	keyPressed[key] = 0;
	glutPostRedisplay();
}

inline void keyboardHandlerUp2FF(int key, int x, int y)
{
	specialKeyPressed[key] = 0;
	glutPostRedisplay();
}

inline void keyboardHandler1FF(unsigned char key, int x, int y)
{
	keyPressed[key] = 1;
	// This is the actual glutKeyboardFunc callback. It used to only set the
	// polling array above and never notify the game -- so one-shot key
	// actions (ESC, 'B' to go back, Enter to confirm, etc., all routed
	// through iKeyboard()/handleKeyboardInput() in controls.h) silently
	// never fired. Held-key movement kept working separately because
	// player.cpp polls GetAsyncKeyState() directly, which is why only
	// "some" keyboard behavior looked broken rather than all of it.
	iKeyboard(key);
	glutPostRedisplay();
}

inline void keyboardHandler2FF(int key, int x, int y)
{
	specialKeyPressed[key] = 1;
	iSpecialKeyboard((unsigned char)key);
	glutPostRedisplay();
}

inline void mouseMoveHandlerFF(int mx, int my)
{
	iMouseX = mx;
	iMouseY = iScreenHeight - my;
	iMouseMove(iMouseX, iMouseY);

	glFlush();
}

inline void mousePassiveMoveHandlerFF(int mx, int my)
{
	iMouseX = mx;
	iMouseY = iScreenHeight - my;
	iPassiveMouseMove(iMouseX, iMouseY);

	glFlush();
}

inline void mouseHandlerFF(int button, int state, int x, int y)
{
	iMouseX = x;
	iMouseY = iScreenHeight - y;

	iMouse(button, state, iMouseX, iMouseY);

	glFlush();
}

// Reshape handler: keeps the OpenGL viewport and projection matrix in sync
// with the actual window size. Without this, if Windows or the window
// manager ever hands the window a different client area than the one it was
// created with (resize, maximize, DPI virtualization on a scaled display,
// etc.), the rendering stays pinned to the old, smaller viewport in a corner
// of the now-larger window -- which is exactly the "content squeezed into
// part of the screen with black everywhere else" symptom.
inline void iReshapeFF(int width, int height)
{
	iScreenWidth = width;
	iScreenHeight = height;

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, width, 0.0, height, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

inline void iInitialize(int width = 500, int height = 500, char *title = "iGraphics", int keyboardSamplingRate = 16)
{
	// REQUIRED: classic GLUT (glut32.dll, bundled with this project) must
	// have glutInit() called before ANY other glut* function. This project
	// never called it (verified: no glutInit() call anywhere in the merged
	// codebase), which left GLUT's internal window/event-routing state
	// uninitialized. That is what caused both reported symptoms at once:
	// the corrupted/undersized window, and the mouse & keyboard callbacks
	// silently never firing. A dummy argc/argv is fine here since this app
	// doesn't use real command-line arguments.
	int dummyArgc = 1;
	char dummyArgv0[] = "Paradox Armory";
	char *dummyArgv[1] = { dummyArgv0 };
	glutInit(&dummyArgc, dummyArgv);

	SetTimer(0, 0, keyboardSamplingRate, keypressHandler);

	iScreenHeight = height;
	iScreenWidth = width;

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA);
	glutInitWindowSize(width, height);
	glutInitWindowPosition(10, 10);
	glutCreateWindow(title);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, width, 0.0, height, -1.0, 1.0);
}

inline void iStart()
{
	iClear();

	glutDisplayFunc(displayFF);
	glutReshapeFunc(iReshapeFF);
	glutKeyboardFunc(keyboardHandler1FF);
	glutSpecialFunc(keyboardHandler2FF);
	glutKeyboardUpFunc(keyboardHandlerUp1FF);
	glutSpecialUpFunc(keyboardHandlerUp2FF);
	glutMouseFunc(mouseHandlerFF);
	glutMotionFunc(mouseMoveHandlerFF);
	glutPassiveMotionFunc(mousePassiveMoveHandlerFF);
	glutIdleFunc(animFF);

	glAlphaFunc(GL_GREATER, 0.0f);
	glEnable(GL_ALPHA_TEST);

	glutMainLoop();
}

#endif // IGRAPHICS_H