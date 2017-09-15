/*
 * lowlevel/camac/sis5100/sis5100camaddr.h
 * created: 2007-Apr-18 PW
 * $ZEL: sis5100camaddr.h,v 1.1 2007/04/24 20:19:38 wuestner Exp $
 */

#ifndef _sis5100camadd_h_
#define _sis5100camadd_h_

#include <sconf.h>
#include <emsctypes.h>

struct sis5100_camadr_t {
#ifdef SIS5100_MMAPPED
    ems_u32 naf;
#else
    int n, a, f;
#endif
};

#endif
