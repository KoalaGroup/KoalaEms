/*
 * objects/pi/readout_em_cluster/datains.h
 * created 14.04.97 PW
 * 13.Mar.2001 PW: VEDinfos used
 */
/*
 * static char *rcsid="$ZEL: datains.h,v 1.7 2011/08/13 20:17:25 wuestner Exp $";
 */

#ifndef _datains_h_
#define _datains_h_

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <objecttypes.h>
#include "../../../dataout/cluster/cluster.h"
#include "../../../dataout/cluster/vedinfo.h"
#include "../../../main/scheduler.h"
#include "../datain.h"

#if 0
typedef struct {
    int server; /* dataout is the passive part */
    int connected;
    int s_path; /* server socket */
    int n_path; /* socket for data */
    struct seltaskdescr* accepttask; /* task for accept (if server) */
} di_special_sock;
#endif

typedef struct {
    errcode (*start) (int);
    errcode (*stop) (int);
    errcode (*resume) (int);
    errcode (*reset) (int);
    void (*cleanup) (int);
    void (*reactivate) (int);
    void (*request_) (int, int, int);
} di_procs;

typedef struct Datain_cl {
    di_procs procs;
    struct seltaskdescr* seltask;
    int suspended;
    InvocStatus status;
    int error;
    struct VEDinfos vedinfos;
    /*int cluster_num;*/
    /*int cluster_max;*/
    int max_displacement;
    int clusters;    /* fuer statistic */
    int words;       /* fuer statistic */
    int suspensions; /* fuer statistic */
#if 0
    union {
        di_cluster_data c_data;
        di_stream_data s_data;
    } d;
#else
    void *private;
#endif
#if 0
    union {
        di_special_sock sock_data;
    } s;
#endif
} Datain_cl;

extern Datain_cl datain_cl[];
extern int ved_info_sent;
/*
 * errcode datain_pre_init(int idx, int qlen, ems_u32* q);
 * errcode remove_datain(int idx);
 */
void freeze_datains(void);
void datain_request_event(int ved, int evnum);

#endif

/*****************************************************************************/
/*****************************************************************************/
