/*
 * lowlevel/camac/pcicamac/pcicamaddr.h
 * created: 2007-Jun-25 PW
 * $ZEL: pcicamaddr.h,v 1.1 2007/06/24 23:41:19 wuestner Exp $
 */

#ifndef _pcicamaddr_h_
#define _pcicamaddr_h_

#include <sconf.h>
#include <emsctypes.h>

struct pcicamac_camadr_t {
#ifdef PCICAM_MMAPPED
    ems_u32 naf;
#else
    int n, a, f;
#endif
};

#endif
