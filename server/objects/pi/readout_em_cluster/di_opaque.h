/*
 * objects/pi/readout_em_opaque/di_opaque.h
 * created 2011-04-14 PW
 * 
 * $ZEL: di_opaque.h,v 1.2 2011/08/16 19:19:51 wuestner Exp $
 */

#ifndef _di_opaque_h_
#define _di_opaque_h_

#include <sconf.h>
#include <errorcodes.h>
#include <emsctypes.h>

errcode di_opaque_sock_init(int, int, ems_u32*);
//errcode di_opaque_lsock_init(int, int, ems_u32*);

#endif

/*****************************************************************************/
/*****************************************************************************/
