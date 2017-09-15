#ifndef _drivercamac_h_
#define _drivercamac_h_

#include "vcc2117.h"

typedef int camadr_t;
#define CAMACinc(a) a+=4

#define CAMACreadX(addr) CAMACread(addr)

#define CAMACbase 0

#endif
