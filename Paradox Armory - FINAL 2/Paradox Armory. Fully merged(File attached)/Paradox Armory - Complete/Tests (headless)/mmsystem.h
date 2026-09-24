#ifndef MMSYSTEM_STUB_H
#define MMSYSTEM_STUB_H
#include <stddef.h>
inline unsigned long mciSendStringA(const char*, char*, unsigned int, void*) { return 0; }
#define mciSendString mciSendStringA
#endif
