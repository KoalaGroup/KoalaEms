/*
 * dataout/cluster/do_cluster.c
 * created 10.04.97 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: do_cluster.c,v 1.14 2011/08/13 20:07:34 wuestner Exp $";

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sconf.h>
#include <debug.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
#include "../dataout.h"
#include "do_cluster.h"
#include "../../main/scheduler.h"
#include "../../objects/domain/dom_dataout.h"
#include "../../commu/commu.h"
#include "../../objects/pi/readout.h"
#include "../../objects/pi/readout_em_cluster/datains.h"
#include "../../objects/ved/ved.h"
#include "../../trigger/trigger.h"

extern ems_u32* outptr;
extern struct Cluster* last_deposited_cluster;

RCS_REGISTER(cvsid, "dataout/cluster")

/*****************************************************************************/
static void
do_cluster_write_error(int do_idx, int error)
{
    DoRunStatus old_status=dataout_cl[do_idx].do_status;
    T(dataout/cluster/do_cluster.c:do_cluster_write_error)

    dataout_cl[do_idx].errorcode=error;
    dataout_cl[do_idx].do_status=Do_error;
    sched_delete_select_task(dataout_cl[do_idx].seltask);
    dataout_cl[do_idx].seltask=0;
    dataout_cl[do_idx].procs.patherror(do_idx, -1);

    if (old_status==Do_flushing) {
        notify_do_done(do_idx);
    }

    if (dataout[do_idx].wieviel==0) {
        send_unsol_var(Unsol_RuntimeError, 4, rtErr_OutDev, do_idx, 0, error);
        fatal_readout_error();
    }
}
/*****************************************************************************/
static int
xreceive(int path, char* data, int size)
{
    int da=0;
    T(dataout/cluster/do_cluster.c:xreceive)
    do {
        int res;
        res=read(path, data+da, size-da);
        if (res>0) {
            da+=res;
        } else {
            if (res==0)
                errno=EPIPE;
            if ((errno!=EINTR) && (errno!=EAGAIN)) {
                printf("dataout/cluster/do_cluster.c:xreceive: %s\n",
                        strerror(errno));
                return -1;
            }
        }
    }
    while (da<size);
    return 0;
}
/*****************************************************************************/
static void do_cluster_read(int path, int do_idx)
{
    int i;
    int req[4]; /* mehr ist noch nicht sinnvoll */
    T(dataout/cluster/do_cluster.c:do_cluster_read)

    if (xreceive(path, (char*)req, sizeof(int))<0) {
#if 0
        if (dataout_cl[do_idx].seltask) {
            enum select_types types;
            types=sched_select_task_get(dataout_cl[do_idx].seltask);
            sched_select_task_set(dataout_cl[do_idx].seltask,
            types&~select_read
#ifdef SELECT_STATIST
                , 0
#endif
            );
        }
        return;
#else
        do_cluster_write_error(do_idx, errno);
        return;
#endif
    }
    req[0]=ntohl(req[0]);
    if (req[0]>3) {
        printf("do_cluster_read: size=%d\n", req[0]);
        return;
    }
    if (xreceive(path, (char*)(req+1), sizeof(int)*req[0])<0) {
        do_cluster_write_error(do_idx, errno);
        return;
    }
    for (i=1; i<=req[0]; i++)
        req[i]=ntohl(req[i]);

    switch (req[1]) {
    case 1: {
        int ved=req[2];
        int evnum=req[3];
#ifdef READOUT_CC
        printf("empfing request: evnum=%d; local evnum=%d, ved=%d, local ved=%d\n",
            evnum, trigger.eventcnt, ved, ved_globals.ved_id);
        flush_databuf(0);
#else
        /*printf("empfing request: evnum=%d; ved=%d, local ved=%d\n", evnum, ved, get_ved_id());*/
        datain_request_event(ved, evnum);
#endif
        }
        break;
    default:
        printf("do_cluster_read: unknown code %d\n", req[1]);
    }
}
/*****************************************************************************/
void do_cluster_write(int path, enum select_types selected, union callbackdata data)
{
    int do_idx=data.i;
    int res;
    struct do_cluster_data* dat=&dataout_cl[do_idx].d.c_data;
    char* cdat;

    T(dataout/cluster/do_cluster.c:do_cluster_write)

    if (selected & (select_except | select_read)) {
        if (selected & select_except) {
            printf("Exception in do_cluster_write(path=%d; do_idx=%d)\n", path, data.i);
            do_cluster_write_error(do_idx, -1);
            return;
        }
        if (selected & select_read) {
            do_cluster_read(path, do_idx);
        }
    }
    if (selected & select_write) {
        if (dat->active_cluster==0) { /* noch kein Cluster in Arbeit */
            if ((dat->active_cluster=dataout_cl[do_idx].advertised_cluster)==0) {
                /* printf("do_cluster_write: kein cluster (--> suspend)\n"); */
                sched_select_task_set(dataout_cl[do_idx].seltask,
                select_read|select_except
#ifdef SELECT_STATIST
                    , 0
#endif
                );
                dataout_cl[do_idx].suspended=1;
                do_statist[do_idx].suspensions++;
                return;
            }
            dat->bytes=dat->active_cluster->size*sizeof(int);
            dat->idx=0;
        }

        cdat=(char*)dat->active_cluster->data;
        res=write(path, cdat+dat->idx, dat->bytes-dat->idx);
        if (res<=0) { /* Fehler? */
            printf("do_cluster_write(path=%d, do_idx=%d, idx=%d, bytes=%d): %s\n", path,
                    data.i, dat->idx, dat->bytes, strerror(errno));
            if ((errno!=EINTR) && (errno!=EWOULDBLOCK)) {
                do_cluster_write_error(do_idx, errno);
            }
            return;
        }
        dat->idx+=res;
        if (dat->idx==dat->bytes) { /* Cluster vollstaendig geschrieben */
            struct Cluster* help=dat->active_cluster;
            if (dat->active_cluster->type!=clusterty_events) {
                if (dat->active_cluster->type==clusterty_no_more_data) {
                    dataout_cl[do_idx].do_status=Do_done;
                    printf("do_cluster::do_cluster_write(do_idx=%d): finished; call reset(%d)\n",
                            data.i, data.i);
                    dataout_cl[do_idx].procs.reset(data.i);
                    notify_do_done(data.i);
                } else if (dat->active_cluster->type==clusterty_ved_info) {
                    dataout_cl[do_idx].vedinfo_sent=1;
                }
            }
            do_statist[do_idx].clusters++;
            do_statist[do_idx].words+=dat->active_cluster->size;
            dat->active_cluster=help->next;
            help->costumers[do_idx]=0;

            if (!--help->numcostumers) {
                clusters_recycle(help);
            }
            while (dat->active_cluster && (dat->active_cluster->costumers[do_idx]==0)) {
                dat->active_cluster=dat->active_cluster->next;
            }

            if (dat->active_cluster==0) {
                /* sched_select_task_suspend(dataout_cl[do_idx].seltask); */
                if (dataout_cl[do_idx].seltask)
                    sched_select_task_set(dataout_cl[do_idx].seltask,
                            select_read|select_except
#ifdef SELECT_STATIST
                            , 0
#endif
                    );
                dataout_cl[do_idx].suspended=1;
                do_statist[do_idx].suspensions++;
                dataout_cl[do_idx].advertised_cluster=0;
            } else {
                dat->bytes=dat->active_cluster->size*sizeof(int);
                dat->idx=0;
            }
        }
    }
}
/*****************************************************************************/
void do_cluster_advertise(int do_idx, struct Cluster* cl)
{
    T(dataout/cluster/do_cluster.c:do_cluster_advertise)

    if (dataout_cl[do_idx].doswitch==Do_disabled) return;
    if (cl->predefined_costumers && !cl->costumers[do_idx]) return;
    if (dataout_cl[do_idx].do_status!=Do_running) return;
/*
 * grober Unfug
 * if ((dataout[do_idx].wieviel<0) && !dataout_cl[do_idx].suspended) return;
 */

#ifdef READOUT_CC
    if (dataout[do_idx].inout_arg_num>=2) { /* triggermask given */
        if (!(cl->triggers & dataout[do_idx].inout_args[1]))
            return;
    }
#endif

    cl->costumers[do_idx]=1;
    cl->numcostumers++;

    if (dataout_cl[do_idx].suspended && dataout_cl[do_idx].seltask) {
        /* sched_select_task_reactivate(dataout_cl[do_idx].seltask); */
        sched_select_task_set(dataout_cl[do_idx].seltask,
                select_read|select_write|select_except
#ifdef SELECT_STATIST
                , 1
#endif
        );
        dataout_cl[do_idx].suspended=0;
    }
    if (!dataout_cl[do_idx].advertised_cluster)
        dataout_cl[do_idx].advertised_cluster=cl;
}
/*****************************************************************************/
errcode do_NotImp_err()
{
    printf("Error in dataout: do_NotImp_err() called\n");
    return Err_Other;
}
/*****************************************************************************/
errcode do_NotImp_err_ii(int a, int b)
{
    printf("Error in dataout: do_NotImp_err_ii(%d, %d) called\n", a, b);
    return Err_Other;
}
/*****************************************************************************/
void do_NotImp_void()
{
}
/*****************************************************************************/
void do_NotImp_void_ii(int a, int b)
{
    printf("Error in dataout: do_NotImp_void_ii(%d, %d) called\n", a, b);
}
/*****************************************************************************/
void do_NotImp_void_i(int a)
{
    printf("Error in dataout: do_NotImp_void_i(%d) called\n", a);
}
/*****************************************************************************/
/*****************************************************************************/
