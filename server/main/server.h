#ifndef _server_h_
#define _server_h_

#include <errorcodes.h>

errcode ResetVED(int* p, int size);
void panic(void);
void die(const char* text);

#endif
