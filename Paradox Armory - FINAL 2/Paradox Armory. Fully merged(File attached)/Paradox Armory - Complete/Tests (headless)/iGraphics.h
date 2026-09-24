#ifndef IGRAPHICS_STUB_H
#define IGRAPHICS_STUB_H
#include <stdio.h>
#include <math.h>
#include <time.h>
typedef double GLdouble;
#define GLUT_BITMAP_8_BY_13        ((void*)1)
#define GLUT_BITMAP_HELVETICA_12   ((void*)2)
#define GLUT_BITMAP_HELVETICA_18   ((void*)3)
#define GLUT_BITMAP_TIMES_ROMAN_24 ((void*)4)
#define GLUT_LEFT_BUTTON  0
#define GLUT_RIGHT_BUTTON 2
#define GLUT_DOWN 0
#define GLUT_UP   1
#define GLUT_KEY_END 107
inline int  iSetTimer(int, void(*)(void)) { return 0; }
inline void iShowBMP(int, int, char[]) {}
inline unsigned int iLoadImage(char[]) { return 1; }
inline void iShowImage(int,int,int,int,unsigned int) {}
inline void iText(GLdouble, GLdouble, char *str, void* font = GLUT_BITMAP_8_BY_13) { (void)str;(void)font; }
inline void iLine(double,double,double,double) {}
inline void iRectangle(double,double,double,double) {}
inline void iFilledRectangle(double,double,double,double) {}
inline void iFilledPolygon(double[], double[], int) {}
inline void iFilledCircle(double,double,double,int slices=100) { (void)slices; }
inline void iCircle(double,double,double,int slices=100) { (void)slices; }
inline void iSetColor(double,double,double) {}
inline void iClear() {}
inline void iInitialize(int w=500,int h=500,char *t=0,int k=16) {(void)w;(void)h;(void)t;(void)k;}
inline void iStart() {}
void iDraw();

// ---- minimal no-op OpenGL stubs (test harness only) ------------------------
// enemy.cpp's Enemy Facing code (DrawTankFacing) issues raw GL calls to mirror
// a sprite. The real build gets these from glut.h/gl.h; headless they do nothing.
typedef float GLfloat;
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_LINEAR 0x2601
#define GL_REPEAT 0x2901
#define GL_TEXTURE_ENV 0x2300
#define GL_TEXTURE_ENV_MODE 0x2200
#define GL_REPLACE 0x1E01
#define GL_QUADS 0x0007
inline void glEnable(int) {}
inline void glDisable(int) {}
inline void glBindTexture(int, unsigned int) {}
inline void glTexParameterf(int, int, float) {}
inline void glTexEnvf(int, int, float) {}
inline void glBegin(int) {}
inline void glEnd() {}
inline void glTexCoord2f(float, float) {}
inline void glVertex2f(float, float) {}
#endif
