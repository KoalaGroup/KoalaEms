/*
 * objects/pi/readout_em_cluster/di_cluster.h
 * created 25.03.97
 * 
 * $ZEL: di_cluster.h,v 1.5 2011/08/13 20:17:25 wuestner Exp $
 */

#ifndef _di_cluster_h_
#define _di_cluster_h_

#include <sconf.h>
#include <errorcodes.h>
#include <emsctypes.h>

errcode di_cluster_sock_init(int, int, ems_u32*);
errcode di_cluster_lsock_init(int, int, ems_u32*);

#endif

/*****************************************************************************/
/*****************************************************************************/
