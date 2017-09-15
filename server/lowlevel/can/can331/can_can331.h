#ifndef _can_can331_h_
#define _can_can331_h_

#include <sconf.h>
#include <sys/types.h>
#include <emsctypes.h>
#include "can331.h"

#if 0
struct can_can331_info {
    int hnd;
};
#endif

errcode can_init_can331(char* pathname, struct can_dev* dev);

#endif
