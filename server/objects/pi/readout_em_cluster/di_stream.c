/*
 * readout_em_cluster/di_stream.c
 * created 2011-04-21 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: di_stream.c,v 1.1 2011/08/13 20:17:26 wuestner Exp $";

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sconf.h>
#include <debug.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
#include "di_stream.h"
#include "../../../main/scheduler.h"
#include "datains.h"
#include "../../domain/dom_datain.h"
#include "../../../dataout/cluster/do_cluster.h"
#include "../readout.h"
#include "../../../commu/commu.h"

#ifdef DMALLOC
#include dmalloc.h
#endif

#define swap_int(a) ( ((a) << 24) | \
		      (((a) << 8) & 0x00ff0000) | \
		      (((a) >> 8) & 0x0000ff00) | \
	((unsigned int)(a) >>24) )

#define set_max(a, b) ((a)=(a)<(b)?(b):(a))

typedef struct {
    int dummy;
} di_stream_data;

RCS_REGISTER(cvsid, "objects/pi/readout_em_cluster")

extern ems_u32* outptr;

/*****************************************************************************/
errcode di_stream_sock_init(int idx, int qlen, ems_u32* q)
{
T(readout_em_cluster/di_cluster.c:di_stream_sock_init)
/*
 * datain_cl[idx].start=di_stream_sock_start;
 * datain_cl[idx].stop=di_stream_sock_stop;
 * datain_cl[idx].resume=di_stream_sock_resume;
 * datain_cl[idx].reset=di_stream_sock_reset;
 */
return Err_AddrTypNotImpl;
}
/*****************************************************************************/
errcode
di_stream_lsock_init(int idx, int qlen, ems_u32* q)
{
T(readout_em_cluster/di_cluster.c:di_stream_lsock_init)
/*
 * datain_cl[idx].stop=di_stream_lsock_stop;
 * datain_cl[idx].resume=di_stream_lsock_resume;
 * datain_cl[idx].reset=di_stream_lsock_reset;
 * datain_cl[idx].advertise=di_stream_advertise;
 */
return Err_AddrTypNotImpl;
}
/*****************************************************************************/
/*****************************************************************************/
