#ifndef WINDOWS_STUB_H
#define WINDOWS_STUB_H
#include <stdlib.h>
typedef unsigned char BYTE;
typedef const char* LPCSTR;
#define VK_ESCAPE 0x1B
// Test-controllable key table shared by every translation unit.
inline short* AsyncKeyTable() { static short t[512] = {0}; return t; }
inline short GetAsyncKeyState(int k) { return AsyncKeyTable()[k & 511]; }
inline void Sleep(unsigned long) {}
#endif
