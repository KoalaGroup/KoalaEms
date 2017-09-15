/*
 * objects/pi/readout_em_cluster/di_stream.h
 * created 2011-04-21 PW
 * 
 * $ZEL: di_stream.h,v 1.1 2011/08/13 20:17:26 wuestner Exp $
 */

#ifndef _di_stream_h_
#define _di_stream_h_

#include <sconf.h>
#include <errorcodes.h>
#include <emsctypes.h>

errcode di_stream_sock_init(int, int, ems_u32*);
errcode di_stream_lsock_init(int, int, ems_u32*);

#endif

/*****************************************************************************/
/*****************************************************************************/
