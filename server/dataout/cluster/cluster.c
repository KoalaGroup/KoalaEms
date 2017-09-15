/*
 * dataout/cluster/cluster.c
 * created 10.04.97 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: cluster.c,v 1.24 2011/08/18 22:30:42 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#ifndef HAVE_CGETCAP
#include <getcap.h>
#endif
#include <rcs_ids.h>
#include "do_cluster.h"
#include "cluster.h"
#include "clusterpool.h"
#include "../../lowlevel/oscompat/oscompat.h"
#include "../../lowlevel/lowbuf/lowbuf.h"
#include "../../objects/domain/dom_datain.h"
#include "../../objects/pi/readout.h"
#include <actiontypes.h>
#include "../../objects/notifystatus.h"
#include "../../main/server.h"
#include "../../objects/domain/dom_dataout.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#define swap_int(a) ( ((a) << 24) | \
		      (((a) << 8) & 0x00ff0000) | \
		      (((a) >> 8) & 0x0000ff00) | \
	((unsigned int)(a) >>24) )

#ifdef READOUT_CC
#  ifndef CLUSTER_NUM
#    define CLUSTER_NUM 100
#  endif
#  ifndef CLUSTER_MAX
#    define CLUSTER_MAX 2*EVENT_MAX
#  endif
#else /* READOUT_CC */
#  ifndef CLUSTER_NUM
#    define CLUSTER_NUM 5
#  endif
#  ifndef CLUSTER_MAX
#    define CLUSTER_MAX 16380
#  endif
#endif /* READOUT_CC */

#define DAQ_LOCK_LINK "daq_lock_link"
#define TSM_LOCK_LINK "tsm_lock_link"

struct Cluster* clusters;

int cluster_num;  /* maximum number of allowed clusters */
int cluster_max;  /* maximum data words allowed in clusters */

static int _initialized=0;

extern ems_u32* outptr;
struct Cluster* last_deposited_cluster;

char *daq_lock_link=0;
char *tsm_lock_link=0;

/* definitions for the memory used to store clusters */
/*
If LOWLEVELBUFFER is defined a (fixed) lowlevelbuffer specified and managed
    in lowlevel/lowbuf/lowbuf.[ch] is used. CLUSTER_MALLOC and commandline
    options are ignored.
If LOWLEVELBUFFER is not defined shared memory and allocation via malloc can
    be used.

The allocation method defaults to shared memory unless CLUSTER_MALLOC is
    defined.

Lowlevelbuffer is used if the hardware to be read out needs it. One reason
may be DMA without scatter/gather capability.

Shared memory has to be used if the data have to be written to tape.
A combination of both methods is not implemented.
*/

RCS_REGISTER(cvsid, "dataout/cluster")

/*****************************************************************************/
int
printuse_clusters(FILE* outfilepath)
{
    T(dataout/clusters/cluster.c:printuse_clusters)
    fprintf(outfilepath,
        "  [:clnum#<number of clusters>][:clsize#<words per cluster>]"
        "[:evsize#<words per event>]"

#if !defined(LOWLEVELBUFFER)
        "[:alloc=malloc|shm]"
#endif
        "[:lockdir=<dir for TSM lock>]"
        "\n    defaults: clnum#%d clsize#%d evsize#%d"

#if !defined(LOWLEVELBUFFER)
#ifdef CLUSTER_MALLOC
        " alloc=malloc"
#else
        " alloc=shm"
#endif
#endif /* LOWLEVELBUFFER */

        "\n",
        CLUSTER_NUM, CLUSTER_MAX, event_max);
    return 1;
}
/*****************************************************************************/
/*
    Arguments:
    alloc: malloc|shm|lowbuf (shm is default (needed for tape output))
    clnum: <num>
    clsize: <num words> (default: 16383 (==64 KByte-4))
    evsize: <num words> (default EVENT_MAX)
    lockdir: <dirname> (no default --> no locking with TSM)
*/
errcode
clusters_init(char* args)
{
    enum clpooltype clpooltype;
    long int argval;
    errcode res;

    T(dataout/clusters/cluster.c:clusters_init)
printf("clusters_init\n");
//printf("~server/dataout/cluster: confirmation of cluster initialization \n");

    if (_initialized) {
        printf("clusters_init: already initialized\n");
        panic();
    }

/* initialize defaults */
    cluster_num=CLUSTER_NUM;
    cluster_max=CLUSTER_MAX;
    /* event_max=EVENT_MAX; statically initialized in /objects/pi/pi.c */

#ifdef CLUSTER_ALLOC
    clpooltype=clpool##CLUSTER_ALLOC;
#else
    clpooltype=clpool_malloc;
#endif
#ifdef LOWLEVELBUFFER
    clpooltype=clpool_lowbuf;
#endif

/* process args */
    if (args) {
        char* alloc_str;

        if (cgetnum(args, "clnum", &argval)==0)
            cluster_num=argval;
        if (cgetnum(args, "clsize", &argval)==0)
            cluster_max=argval;
        if (cgetnum(args, "evsize", &argval)==0)
            event_max=argval;

        if (cgetstr(args, "alloc", &alloc_str)>0) {
            if (strcasecmp(alloc_str, "malloc")==0) {
                clpooltype=clpool_malloc;
            } else if (strcasecmp(alloc_str, "shm")==0) {
                clpooltype=clpool_shm;
#ifdef LOWLEVELBUFFER
            } else if (strcasecmp(alloc_str, "lowbuf")==0) {
                clpooltype=clpool_lowbuf;
#endif
            } else {
                printf("clusters_init: '%s' not known; available types:",
                    alloc_str);
                printf(" 'malloc', 'shm'");
#ifdef LOWLEVELBUFFER
                printf(" 'lowbuf'");
#endif
                free(alloc_str);
                return Err_ArgRange;
            }
            free(alloc_str);
        }
        if (cgetstr(args, "lockdir", &alloc_str)>0) {
            daq_lock_link=(char*)malloc(strlen(alloc_str)+
                    strlen(DAQ_LOCK_LINK)+2);
            tsm_lock_link=(char*)malloc(strlen(alloc_str)+
                    strlen(TSM_LOCK_LINK)+2);
            if (!daq_lock_link || !tsm_lock_link) {
                printf("malloc: daq_lock_link=%p tsm_lock_link=%p\n",
                    daq_lock_link, tsm_lock_link);
                free(daq_lock_link);
                free(tsm_lock_link);
                free(alloc_str);
                return Err_NoMem;
            }
            sprintf(daq_lock_link, "%s/%s", alloc_str, DAQ_LOCK_LINK);
            sprintf(tsm_lock_link, "%s/%s", alloc_str, TSM_LOCK_LINK);
            free(alloc_str);
printf("daq_lock_link=%s tsm_lock_link=%s\n", daq_lock_link, tsm_lock_link);
        } else {
            daq_lock_link=0;
            tsm_lock_link=0;
        }
    }

    if (cluster_num<4) {
        printf("clusters_init: minimum of 4 cluster expected (not %d)\n",
            cluster_num);
        return Err_ArgRange;
    }

#ifdef READOUT_CC
    if (cluster_max<event_max) {
        printf("clusters_init: minimum size for clusters: "
            "%d (==event_max) (not %d)\n",
            event_max, cluster_max);
        return Err_ArgRange;
    }
#endif

    res=clusterpool_alloc(clpooltype, cluster_max+HEADSIZE, cluster_num);
    if (res!=OK) {
        return res;
    }

    clusters=0;
    last_deposited_cluster=0;

printf("clusters_init: set _initialized to 1\n");
    _initialized=1;

    clusters=0;
    return OK;
}
/*****************************************************************************/
int
cluster_using_shm(void)
{
    enum clpooltype pooltype;

    pooltype=clusterpool_type();
    return pooltype==clpool_shm;
}
/*****************************************************************************/
errcode
clusters_reset(void)
{
    T(dataout/clusters/cluster.c:clusters_reset)
printf("clusters_reset\n");

    if (!_initialized) {
        printf("clusters_reset: not initialized\n");
        /*panic();*/
    }

    if (daq_lock_link && daq_lock_link[0]) {
        unlink(daq_lock_link);
    }
    if (daq_lock_link) {
        free(daq_lock_link);
        daq_lock_link=0;
    }
    if (tsm_lock_link) {
        free(tsm_lock_link);
        tsm_lock_link=0;
    }

    clusters=0;
    last_deposited_cluster=0;
printf("clusters_reset: set _initialized to 0\n");
    _initialized=0;
    return clusterpool_release();
}
/*****************************************************************************/
#ifdef READOUT_CC
struct Cluster* clusters_create(size_t size, const char* text)
#else
struct Cluster* clusters_create(size_t size, int di_idx, const char* text)
#endif
{
    struct Cluster* cluster;
    int i;

    T(dataout/clusters/cluster.c:clusters_create)

#ifdef CLUSTERCHECK
    if (size+HEADSIZE>clustersize) {
        printf("clusters_create: requested size: %d; available size: %d\n",
                size, clustersize);
        panic();
    }
#endif /* CLUSTERCHECK */

    cluster=(struct Cluster*)alloc_cluster((size+HEADSIZE)*sizeof(ems_u32), text);
    if (cluster==0) {
        /*printf("clusters_create: no space\n");*/
        return 0;
    }

#ifdef READOUT_CC
    cluster->triggers=0;
#else
    /*cluster->di_idx=di_idx;*/
#endif
    cluster->prev=0;
    cluster->next=0;
    for (i=0; i<MAX_DATAOUT; i++)
        cluster->costumers[i]=0;
    cluster->predefined_costumers=0;
    cluster->numcostumers=0;
    return cluster;
}
/*****************************************************************************/
void
clusters_recycle(struct Cluster* cluster)
{
    T(dataout/clusters/cluster.c:clusters_recycle)

    if (last_deposited_cluster==cluster) {
        /*printf("clusters_recycle: RECYCLE last_deposited_cluster!!\n");*/
        last_deposited_cluster=0;
    }

    if (cluster->next)
        cluster->next->prev=cluster->prev;
    if (cluster->prev)
        cluster->prev->next=cluster->next;
    if (clusters==cluster)
        clusters=cluster->next;

    free_cluster(cluster);
    outputbuffer_freed();
}
/*****************************************************************************/
void clusters_conv_to_network(struct Cluster* cluster)
{
int cl_swapped;
T(dataout/clusters/cluster.c:clusters_conv_to_network)
switch (ClENDIEN(cluster->data))
  {
  case 0x12345678: cl_swapped=0; break;
  case 0x78563412: cl_swapped=1; break;
  default:
    printf("clusters_conv_to_network: endientest=0x%x\n",
    ClENDIEN(cluster->data));
    return;
  }
if (cl_swapped!=cluster->swapped)
  {
  printf("clusters_conv_to_network: cluster->swapped is wrong\n");
  cluster->swapped=cl_swapped;
  }
#ifdef WORDS_BIGENDIAN
if (cluster->swapped==1)
#else
if (cluster->swapped==0)
#endif
  {
  int i;
  for (i=0; i<cluster->size; i++) cluster->data[i]=swap_int(cluster->data[i]);
  cluster->swapped=!cluster->swapped;
  }
}
/*****************************************************************************/
void
clusters_link(struct Cluster* cluster)
{
    T(dataout/clusters/cluster.c:clusters_link)
    if (clusters==0) {
        clusters=cluster;
    } else {
        struct Cluster* help=clusters;
        while (help->next!=0)
            help=help->next;
        cluster->prev=help;
        help->next=cluster;
    }
    /*cluster->linked=1;*/
}
/*****************************************************************************/
void
clusters_deposit(struct Cluster* cluster)
{
    static int ev_clusters=0;
    static int is_clusters=0;

    T(dataout/clusters/cluster.c:clusters_deposit)
    if (cluster->size==0) {
        printf("clusters_deposit: size=0\n");
        *(int*)0=0;
    }

#ifdef CLUSTERCHECKSUM
    calculate_checksum(cluster);
#endif

    switch (cluster->type) {
    case clusterty_events:
        ev_clusters++;
        /*if (is_clusters==0) printf("deposit cluster event (%d)\n", ev_clusters);*/
        break;
    case clusterty_ved_info:
        /*printf("deposit cluster ved_info, %d eventclusters; is_clusters=%d\n",
            ev_clusters, is_clusters);*/
        is_clusters++;
        break;
    case clusterty_text:
        /*printf("deposit cluster text\n");*/
        ev_clusters=0;
        break;
    case clusterty_file:
        /*printf("deposit cluster file\n");*/
        ev_clusters=0;
        break;
    case clusterty_async_data:
        /*printf("deposit cluster async\n");*/
        break;
    case clusterty_no_more_data:
        /*printf("deposit cluster no_more_data, %d eventclusters; called from %s\n",
            ev_clusters, caller);*/
        break;
    }

    /* replace last deposited cluster */
    if (cluster->type==clusterty_events) {

        if (last_deposited_cluster && !--last_deposited_cluster->numcostumers)
            clusters_recycle(last_deposited_cluster);

        last_deposited_cluster=cluster;
        cluster->numcostumers++;
    }

    {
        int i;
        for (i=0; i<MAX_DATAOUT; i++) {
            if (dataout[i].bufftyp!=-1) {
                /*printf("advertise %p to DO %d\n", cluster, i);*/
                dataout_cl[i].procs.advertise(i, cluster);
            }
        }
    }
    if (cluster->numcostumers==0)
        clusters_recycle(cluster);
    else
        clusters_link(cluster);
}
/*****************************************************************************/
/*
    state machine eines Dataouts:
    Stati:
        Do_running
        Do_neverstarted
        Do_done
        Do_error
        Do_flushing

    Events:
        start
        reset
        stop

    Initialzustand: Do_neverstarted (==Do_done oder idle?)
    event: start
        (start_dataout)
        (dataout_clX(do_idx).procs.start(idx))
        Do_neverstarted --> Do_running

    event: reset
        ()
        (dataout_clX(j).procs.reset(idx)
        !=Do_done --> Do_neverstarted
            close(file), kein flush, kein nix

    event: stop
        (stop_dataouts)
        !=Do_error && !=Do_done:
            clusters_deposit_final(-1);
            status --> Do_flushing fuer alle Dataouts

idle, running, error

idle+start      --> running
error+start      --> running
running+ fehler --> error
runnging+final cluster --> idle + notifystatus
error+final cluster --> error+ notifystatus


*/
/*
    notify_do_done sends 'unsol_StatusChanged' for each dataout which is no
    longer running (ready or error)
    if no dataout is running any more a 'unsol_StatusChanged(-1)' is sent
*/
void
notify_do_done(int idx)
{
    ems_u32 id;
    int i, r=0;

    T(dataout/clusters/cluster.c:notify_do_done)

    printf("notify_do_done: dataout %d finished.\n", idx);

    if ((idx<0) || (idx>=MAX_DATAOUT)) {
        printf("notify_do_done: illegal index %d\n", idx);
        *(int*)0=0;
    }
    if (dataout[idx].bufftyp==-1) {
        printf("notify_do_done: invalid index %d\n", idx);
        *(int*)0=0;
    }

    id=(ems_u32)idx;
    notifystatus(status_action_finish, Object_do, 1, &id);

    for (i=0; i<MAX_DATAOUT; i++) {
        if ((dataout[i].bufftyp!=-1) &&
                (dataout_cl[i].do_status==Do_running)) {
            r++;
        }
    }
    if (r) {
        printf("notify_do_done: dataouts ");
        for (i=0; i<MAX_DATAOUT; i++) {
            if ((dataout[i].bufftyp!=-1) &&
                    (dataout_cl[i].do_status==Do_running)) {
                printf("%d ", i);
            }
        }
        printf("still running\n");
    } else {
        printf("notify_do_done: all dataouts finished\n");
        id=(ems_u32)-1;
        notifystatus(status_action_finish, Object_do, 1, &id);
    }
}
/*****************************************************************************/
/*****************************************************************************/
