/*
 * dataout/cluster/cl_interface.c
 * created 1997-Mar-23 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: cl_interface.c,v 1.38 2011/04/06 20:30:21 wuestner Exp $";

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <unsoltypes.h>
#include <sconf.h>
#include <debug.h>
#include <xdrstring.h>
#include <objecttypes.h>
#include <rcs_ids.h>
#include "do_cluster.h"
#include "clusterpool.h"
#include "vedinfo.h"
#include "../dataout.h"
#include "../../objects/objectcommon.h"
#include "../../objects/do/doobj.h"
#include "../../objects/domain/dom_dataout.h"
#include "../../objects/is/is.h"
#include "../../objects/pi/readout.h"
#include "../../objects/ved/ved.h"
#include "../../commu/commu.h"
#include "../../trigger/trigger.h"
#include "../../lowlevel/devices.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#undef USE_RESERVE

struct Dataout_cl dataout_cl[MAX_DATAOUT];
struct Dataout_statist do_statist[MAX_DATAOUT];
struct VEDinfos vedinfos;
static struct Cluster* finalcluster;
#ifdef READOUT_CC
ems_u32 *next_databuf, *last_databuf;
int buffer_free;
#ifdef USE_RESERVE
struct Cluster *reservecluster;
static int reserve_used;
static int copycount;
#endif
static int assumed_event_size;
static struct Cluster *currentcluster;
#endif

extern ems_u32* outptr;
extern int cluster_max, cluster_num; /* in cluster.c definiert */
extern struct Cluster* last_deposited_cluster;

extern int cl_write_count;
int max_ev_per_cluster=0;
int max_time_per_cluster=0;
extern int suspended;
extern int suspensions;

#ifdef READOUT_CC
extern int wirhaben; /* defined in objects/pi/readout_cc/readout.c */
#endif

#if defined (CLUSTERCHECKSUM)
#   if CLUSTERCHECKSUM==MD5
#       define CHECKSUMSIZE CHECKSUMSIZE_MD5
#       define CHECKSUMTYPE CHECKSUMTYPE_MD5
#   elif CLUSTERCHECKSUM==MD4
#       define CHECKSUMSIZE CHECKSUMSIZE_MD4
#       define CHECKSUMTYPE CHECKSUMTYPE_MD4
#   else
#       error CLUSTERCHECKSUM unknown
#   endif
#endif

#ifdef READOUT_CC
static void flush_current_cluster(void);
#endif

RCS_REGISTER(cvsid, "dataout/cluster")

/*****************************************************************************/
int printuse_output(FILE* outfilepath)
{
    T(dataout/cluster/cl_interface.c:printuse_output)
    return printuse_clusters(outfilepath);
}
/*****************************************************************************/
errcode dataout_init(char* arg)
{
    T(dataout/cluster/cl_interface.c:dataout_init)
    vedinfos.num_veds=-1;
    vedinfos.version=-1;
    vedinfos.info=0;
    return clusters_init(arg);
}
/*****************************************************************************/
errcode dataout_done(void)
{
    int i;
    T(dataout/cluster/cl_interface.c:dataout_done)

    if (vedinfos.info) {
        printf("dataout_done: vedinfos.info=%p, num_veds=%d\n", vedinfos.info, vedinfos.num_veds);
        for (i=0; i<vedinfos.num_veds; i++) {
            printf("dataout_done: vedinfos.info[i].is_info=%p\n", vedinfos.info[i].is_info);
            free(vedinfos.info[i].is_info);
        }
        free(vedinfos.info);
        vedinfos.info=0;
    }
    clusters_reset();
    return OK;
}
/*****************************************************************************/
#ifdef READOUT_CC
void
add_used_trigger(ems_u32 trig)
{
    currentcluster->triggers|=trig;
}
#endif
/*****************************************************************************/
void freeze_dataouts(void)
{
/*
called from readout_em_cluster/piops.c:fatal_readout_error()
all datain and dataout activities have to be immediately stopped
*/
    int do_idx;
    T(dataout/cluster/cl_interface.c:freeze_dataouts)
    for (do_idx=0; do_idx<MAX_DATAOUT; do_idx++) {
        if (dataout[do_idx].bufftyp!=-1)
                dataout_cl[do_idx].procs.freeze(do_idx);
    }
}
/*****************************************************************************/
#ifdef READOUT_CC
static struct IS*
find_is_idx(int is_ID)
{
    int idx;

    for (idx=1; idx<MAX_IS; idx++) {
        if (is_list[idx] && is_list[idx]->id==is_ID)
            return is_list[idx];
    }
    return 0;
}
#endif
/*****************************************************************************/
#ifdef READOUT_CC
static void
put_last_databuf(ems_u32 *data)
{
    int nr_sev, sev;

    data++;            /* skip event length */
    *outptr++=*data++; /* event number */
    *outptr++=*data++; /* trigger id */
    nr_sev=*data++;    /* number of subevents */
    *outptr++=nr_sev;

    for (sev=0; sev<nr_sev; sev++) { /* iterate over single subevents */
        struct IS* IS;
        enum decoding_hints hints;

        int sev_id=*data++; /* subevent ID */
        *outptr++=sev_id;

        IS=find_is_idx(sev_id);
        hints=IS?IS->decoding_hints:0;
        if (hints&decohint_lvd_async) { /* contains MANY real subevents*/
                                        /* we use only the first one */
            unsigned int sev_len;
            data++; /* skip original subevent length */
            sev_len=(*data&0xffff)/4; /* length of LVDS subevent */
            *outptr++=sev_len+1; /* new subevent length */
            *outptr++=sev_len; /* simulate the 'normal' readout procedure */
            while (sev_len--)
                *outptr++=*data++;
        } else {
            int sev_len=*data++;
            *outptr++=sev_len;
            while (sev_len--)
                *outptr++=*data++;
        }
    }
}
#endif
/*****************************************************************************/
#ifdef READOUT_CC
errcode get_last_databuf(void)
{
    ems_u32* data;
    int evnum;

    T(dataout/cluster/cl_interface.c:get_last_databuf)
#ifdef READOUT_CC
    if (currentcluster) {
        data=&ClEVNUM(currentcluster->data);
        evnum=*data++;
        if (evnum>=1) {
            for (; evnum>1; evnum--)
                data+=*data+1;
            put_last_databuf(data);
            return OK;
        }
    }
#endif
    if (last_deposited_cluster==0)
        return Err_NoDomain;
    if (!clusterpool_valid(last_deposited_cluster))
        return Err_NoDomain;
    if (last_deposited_cluster->swapped) {
        printf("get_last_databuf: cluster swapped.\n");
        return Err_NoDomain;
    }
    data=&ClEVNUM(last_deposited_cluster->data);
    evnum=*data++;
    if (evnum<1)
        return Err_NoDomain;
    for (; evnum>1; evnum--)
        data+=*data+1;
    put_last_databuf(data);
    return OK;
}
#endif
/*****************************************************************************/
errcode remove_dataout(unsigned int do_idx)
{
    T(dataout/cluster/cl_interface.c:remove_dataout)

    dataout_cl[do_idx].procs.reset(do_idx);
    dataout_cl[do_idx].procs.cleanup(do_idx);
    {
        struct Cluster *cl, *next;
        cl=clusters;
        while (cl) {
            /* XXX TEST */
            if (cl==cl->next) {
                printf("cl_interface.c:remove_dataout: cl==cl->next==%p\n", cl);
                *(int*)0=0;
                break;
            }
            next=cl->next;
            if (cl->costumers[do_idx]) {
                cl->costumers[do_idx]=0;
                if (!--cl->numcostumers)
                    clusters_recycle(cl);
            }
            
            cl=next;
        }
    }
    return OK;
}
/*****************************************************************************/
#ifdef READOUT_CC
static void
prepare_vedinfo(void)
{
    int is, n, decohints=0;

    for (is=0, n=0; is<MAX_IS; is++) {
        if (is_list[is] && is_list[is]->enabled) {
            n++;
            if (is_list[is]->decoding_hints)
                decohints=1;
        }
    }

    vedinfos.num_veds=1;
    vedinfos.version=decohints?3:1;
    vedinfos.info=malloc(sizeof(struct VEDinfo));
    printf("prepare_vedinfo: vedinfos.info=%p\n", vedinfos.info);
    vedinfos.info->orig_id=ved_globals.ved_id;
    vedinfos.info->ved_id=vedinfos.info->orig_id;
#if 0
    vedinfos.info->events=0;
#endif
    vedinfos.info->num_is=n;
    vedinfos.info->is_info=malloc(n*sizeof(struct ISinfo));
    printf("prepare_vedinfo: vedinfos.info->is_info=%p\n", vedinfos.info->is_info);
    for (is=0, n=0; is<MAX_IS; is++) {
        if (is_list[is] && is_list[is]->enabled) {
            vedinfos.info->is_info[n].is_id=is_list[is]->id;
            vedinfos.info->is_info[n].importance=is_list[is]->importance;
            vedinfos.info->is_info[n].decoding_hints=is_list[is]->decoding_hints;
            n++;
        }
    }
}
#endif
/*****************************************************************************/
void
send_ved_cluster(int do_idx)
{
    struct Cluster* cl;
    int i;

    printf("send_ved_cluster(do_idx=%d)\n", do_idx);

    if (vedinfos.num_veds==0) {
        printf("send_ved_cluster(do_idx=%d): no info available\n", do_idx);
        return;
    }

    cl=create_ved_cluster();
    if (cl==0) {
        printf("send_ved_cluster(do_idx=%d): ved_cluster==0\n", do_idx);
        return;
    }

    cl->predefined_costumers=1;
    if (do_idx<0) {
        for (i=0; i<MAX_DATAOUT; i++) {
            if (dataout[i].bufftyp!=-1) {
                cl->costumers[i]=1;
                dataout_cl[i].vedinfo_sent=1;
            }
        }
    } else {
        cl->costumers[do_idx]=1;
        dataout_cl[do_idx].vedinfo_sent=1;
    }
    clusters_deposit(cl);
}
/*****************************************************************************/
errcode insert_dataout(unsigned int do_idx)
{
    errcode res;
    T(dataout/cluster/cl_interface.c:insert_dataout)
    printf("insert_dataout(do_idx=%d)\n", do_idx);

    switch (dataout[do_idx].bufftyp) {
/*  case InOut_Ringbuffer:
 *
 *      switch (dataout[do_idx].addrtyp) {
 *      default: res=Err_AddrTypNotImpl;
 *      }
 *      break;
 *  case InOut_Stream:
 *     switch (dataout[do_idx].addrtyp) {
 *     case Addr_Socket:
 *         res=do_stream_sock_init(do_idx);
 *         break;
 *     default: res=Err_AddrTypNotImpl;
 *     }
 *     break;
 */
    case InOut_Filebuffer:
        switch (dataout[do_idx].addrtyp) {
#ifdef DO_TAPE
        case Addr_Tape:
            res=do_cluster_tape_init(do_idx);
            break;
#endif
#ifdef DO_NULL
        case Addr_Null:
            res=do_dummy_init(do_idx);
            break;
#endif
    }
    case InOut_Cluster:
        switch (dataout[do_idx].addrtyp) {
#ifdef DO_SOCKET
        case Addr_Socket:
            res=do_cluster_sock_init(do_idx);
            break;
#endif
#ifdef DO_FILE
        case Addr_File:
            res=do_cluster_file_init(do_idx);
            break;
#endif
#ifdef DO_AUTOSOCKET
        case Addr_Autosocket:
            res=do_cluster_autosock_init(do_idx);
            break;
#endif
#ifdef DO_TAPE
        case Addr_Tape:
            res=do_cluster_tape_init(do_idx);
            break;
#endif
#ifdef DO_NULL
        case Addr_Null:
            res=do_dummy_init(do_idx);
            break;
#endif
        default: res=Err_AddrTypNotImpl;
        }
        break;
    default: res=Err_BufTypNotImpl;
    }
    if ((res==OK) && (readout_active)) {
        if ((res=dataout_cl[do_idx].procs.start(do_idx))!=OK) {
            printf("dataout/cluster/cl_interface::insert_dataout():\n"
                    "  cannot start dataout %d\n", do_idx);
                    dataout_cl[do_idx].procs.cleanup(do_idx);
        } else {
            if (dataout_cl[do_idx].doswitch!=Do_disabled)
                send_ved_cluster(do_idx);
        }
    }
    return res;
}
/*****************************************************************************/
errcode init_dataout_handler(void)
{return OK;}
/*****************************************************************************/
errcode done_dataout_handler(void)
{return OK;}
/*****************************************************************************/
/*
 * idx   : dataout_idx
 * offset: wieviel
 */
errcode windtape(unsigned int idx, int offset)
{
    T(dataout/cluster/cl_interface.c:windtape)
    if (dataout[idx].bufftyp==-1) return Err_NoDo;
    return dataout_cl[idx].procs.wind(idx, offset);
}
/*****************************************************************************/
/*
 * idx   : dataout_idx
 *
 * result:
 * if (format>1): -format
 * error (errcode?)
 * fertig (Do_running, Do_neverstarted, Do_done, Do_error)
 * disabled (Do_noswitch, Do_enabled, Do_disabled)
 * Anzahl der folgenden Worte
 * transferred clusters          32/64 bit
 * transferred words             32/64 bit
 * transferred events            32/64 bit
 * number of dataout_suspensions 32/64 bit
 * number of clusters currently in use
 * maximum number of clusters
 *
 *   if (dataout_cl[do_idx].procs.status)
 * num
 * ... (was auch immer procs.status tut)
 *   else
 * num==0
 */
errcode getoutputstatus(unsigned int do_idx, int format)
{
    ems_u32 *help;
    int c=0;

    T(dataout/cluster/cl_interface.c:getoutputstatus)
    if (dataout[do_idx].bufftyp==-1) return Err_NoDo;
    if (format>1) *outptr++=-format;
    *outptr++=dataout_cl[do_idx].do_status==Do_error?
        dataout_cl[do_idx].errorcode:0;
    *outptr++=dataout_cl[do_idx].do_status;
    *outptr++=dataout_cl[do_idx].doswitch;

    help=outptr++;

    switch (format) {
    case 0: /* old and simple */
        if (dataout_cl[do_idx].procs.status)
        dataout_cl[do_idx].procs.status(do_idx);
    case 1: /* newer; but with 32 bit counters */
        *outptr++=do_statist[do_idx].clusters;
        *outptr++=do_statist[do_idx].words;
        *outptr++=do_statist[do_idx].events;
        *outptr++=do_statist[do_idx].suspensions;
        break;
    case 2: /* 64 bit counters */
        *outptr++=(do_statist[do_idx].clusters>>32)&0xffffffffU;
        *outptr++=(do_statist[do_idx].clusters)&0xffffffffU;
        *outptr++=(do_statist[do_idx].words>>32)&0xffffffffU;
        *outptr++=(do_statist[do_idx].words)&0xffffffffU;
        *outptr++=(do_statist[do_idx].events>>32)&0xffffffffU;
        *outptr++=(do_statist[do_idx].events)&0xffffffffU;
        *outptr++=(do_statist[do_idx].suspensions>>32)&0xffffffffU;
        *outptr++=(do_statist[do_idx].suspensions)&0xffffffffU;
        break;
    default:
        return Err_ArgRange;
    }

    if (dataout_cl[do_idx].do_status==Do_running) {
        struct Cluster* cl=clusters;
        while (cl) {
            if (cl->costumers[do_idx])
                c++;
            cl=cl->next;
        }
        *outptr++=c;
    } else {
        *outptr++=0;
    }
    *outptr++=cluster_num;
    if (dataout_cl[do_idx].procs.status) {
        ems_u32 *help;
        help=outptr++;
        dataout_cl[do_idx].procs.status(do_idx);
        *help=outptr-help-1;
    } else {
        *outptr++=0;
    }

    if (format!=0)
        *help=outptr-help-1;

    return OK;
}
/*****************************************************************************/
errcode enableoutput(unsigned int do_idx)
{
    T(dataout/cluster/cl_interface.c:enableoutput)

    if (dataout[do_idx].bufftyp==-1)
        return Err_NoDo;
    dataout_cl[do_idx].doswitch=Do_enabled;
    if (!dataout_cl[do_idx].vedinfo_sent)
        send_ved_cluster(do_idx);
    return OK;
}
/*****************************************************************************/
errcode disableoutput(unsigned int do_idx)
{
    T(dataout/cluster/cl_interface.c:disableoutput)

    if (dataout[do_idx].bufftyp!=-1) {
        dataout_cl[do_idx].doswitch=Do_disabled;
        return OK;
    } else {
        return Err_NoDo;
    }
}
/*****************************************************************************/
static objectcommon* lookup_dataout(ems_u32* id, unsigned int idlen,
    unsigned int* remlen)
{
    T(dataout/cluster/cl_interface.c:lookup_dataout)
    if (idlen>0) {
        if (id[0]!=0) return 0;
        *remlen=idlen;
        return &do_obj;
    } else {
        *remlen=0;
        return &do_obj;
    }
}
/*****************************************************************************/
static ems_u32 *dir_dataout(ems_u32* ptr)
{
    int i;
    T(dataout/cluster/cl_interface.c:dir_dataout)
    for (i=0; i<MAX_DATAOUT; i++) {
        if (dataout[i].bufftyp!=-1)
            *ptr++=i;
    }
    return ptr;
}
/*****************************************************************************/
#if 0
static int setprot_dataout(ems_u32* idx, unsigned int idxlen,
    char* newowner, int* newperm)
{
    T(dataout/cluster/cl_interface.c:setprot_dataout)
    printf("interface::setprot_dataout(...)\n"); exit(0);
    return -1;
/*
 * if (newperm) return -1;
 * if (newowner)
 *   {
 *   if (owner) free(owner);
 *   owner=strdup(newowner);
 *   }
 * return 0;
 */
}
#endif
/*****************************************************************************/
#if 0
static int getprot_dataout(ems_u32* idx, unsigned int idxlen,
    char** ownerp, int** permp)
{
    T(dataout/cluster/cl_interface.c:getprot_dataout)
    printf("interface::getprot_dataout(...)\n"); exit(0);
    return -1;
/*
 * if (ownerp) *ownerp=owner;
 * if (permp) *permp=perms;
 * return 0;
 */
}
#endif
/*****************************************************************************/

objectcommon do_obj=  {
    .init    = 0,
    .done    = dataout_done,
    .lookup  = lookup_dataout,
    .dir     = dir_dataout,
    .refcnt  = 0,
#if 0
    .setprot = setprot_dataout,
    .getprot = getprot_dataout,
#endif
};
/*****************************************************************************/
static void dump_vedinfo(void)
{
    int nv, j;
    printf("num_veds=%d\n", vedinfos.num_veds);
    printf("      ved_ids, is_nums:\n");
    for (nv=0; (nv<vedinfos.num_veds) && (nv<50); nv++) {
        struct VEDinfo* info=vedinfos.info+nv;
        printf("  [%d]: %d, %d; is:\n", nv, info->ved_id, info->num_is);
    for (j=0; j<info->num_is; j++)
        printf(" (%d %d 0x%x)", info->is_info[j].is_id,
                info->is_info[j].importance, info->is_info[j].decoding_hints);
    printf("\n");
    }
}
/*****************************************************************************/
struct Cluster*
create_ved_cluster()
{
    struct Cluster* is_cluster;
    ems_u32 *p;
    int clsize, nv, ni;
    int a=0, b=0, c=0;

    printf("create_ved_cluster:\n");
    dump_vedinfo();

/*size=a+b*numved+c*numis;*/
    switch (vedinfos.version) {
    case 1: a=6; b=2; c=1; break;
    case 2: a=6; b=3; c=1; break;
    case 3: a=6; b=2; c=3; break;
    default:
        printf("create_ved_cluster: version %d invalid\n", vedinfos.version);
        send_unsol_var(Unsol_RuntimeError, 3, rtErr_Other, -1, 1);
        return 0;
    }
    clsize=a+b*vedinfos.num_veds;
    for (nv=0; nv<vedinfos.num_veds; nv++)
        clsize+=c*vedinfos.info[nv].num_is;

#ifdef READOUT_CC
    is_cluster=clusters_create(clsize, "ved");
#else
    is_cluster=clusters_create(clsize, MAX_DATAIN, "ved");
#endif

    if (is_cluster==0) {
        printf("dataout/cluster/cl_interface.c: no cluster for Num-VEDs.\n");
        send_unsol_var(Unsol_RuntimeError, 3, rtErr_Other, -1, 1);
        return 0;
    }

    ClLEN(is_cluster->data)=clsize-1;
    ClENDIEN(is_cluster->data)=0x12345678;
    ClTYPE(is_cluster->data)=clusterty_ved_info;
    ClOPTSIZE(is_cluster->data)=0;
    p=is_cluster->data+4;
    *p++=0x80000000|vedinfos.version;       /* Version + Bit 31 */
    *p++=vedinfos.num_veds;

    for (nv=0; nv<vedinfos.num_veds; nv++) {
        struct VEDinfo* info=vedinfos.info+nv;
        *p++=info->ved_id;
        if (vedinfos.version==2) {
            if (info->num_is>0)
                *p++=info->is_info[0].importance;
            else
                *p++=0;
        }
        *p++=info->num_is;
        for (ni=0; ni<info->num_is; ni++) {
            *p++=info->is_info[ni].is_id;
            if (vedinfos.version>=3) {
                *p++=info->is_info[ni].importance;
                *p++=info->is_info[ni].decoding_hints;
            }
        }
    }

    is_cluster->size=clsize;
    is_cluster->swapped=0;
    is_cluster->type=clusterty_ved_info;
    is_cluster->flags=0;
    is_cluster->optsize=0;
    return is_cluster;
}
/*****************************************************************************/
static errcode
start_one_dataout(int idx)
{
    errcode res;

    if (dataout_cl[idx].do_status==Do_running) {
        printf("start_one_dataout: dataout %d already running\n", idx);
        return OK;
    }
    if (dataout[idx].loginfo) {
        free(dataout[idx].loginfo);
        dataout[idx].loginfo=0;
        printf("loginfo for dataout %d cleared\n", idx);
    }
    res=dataout_cl[idx].procs.start(idx);
    return res;
}
/*****************************************************************************/
static errcode
start_all_dataouts(void)
{
    int i;

    for (i=0; i<MAX_DATAOUT; i++) {
        if (dataout[i].bufftyp!=-1) {
            errcode res;
            res=start_one_dataout(i);
            if (res!=OK) {
                int j;
                for (j=0; j<i; j++) {
                    if (dataout[j].bufftyp!=-1)
                        dataout_cl[j].procs.reset(j);
                }
                return res;
            }
        }
    }
    return OK;
}
/*****************************************************************************/
#ifdef READOUT_CC
errcode
start_dataout(void)
{
    errcode res;

    T(dataout/cluster/cl_interface.c:start_dataout)

    clusterpool_clear();

    finalcluster=clusters_create(7, "final");
    if (finalcluster==0) {
        printf("start_dataout: cannot allocate finalcluster\n");
        return Err_NoMem;
    }

#ifdef USE_RESERVE
    reservecluster=clusters_create(cluster_max+event_max, "reserve");
    if (reservecluster==0) {
        printf("start_dataout: cannot allocate reservecluster\n");
        return Err_NoMem;
    }
#endif

    clusters=0;
    last_deposited_cluster=0;
#ifdef USE_RESERVE
    reserve_used=0;
    copycount=0;
#endif
    assumed_event_size=event_max;
    currentcluster=0;
    buffer_free=0;
    next_databuf=0;
    last_databuf=0;
    res=start_all_dataouts();
    if (res!=OK)
        return res;

    prepare_vedinfo();
    send_ved_cluster(-1);

    return OK;
}
#else /* READOUT_CC */
errcode start_dataout(void)
{
    T(dataout/cluster/cl_interface::start_dataout)
    printf("start_dataout(); called from %s\n", caller);

    clusterpool_clear();

    finalcluster=clusters_create(7, -1, "final");
    if (finalcluster==0) {
        printf("start_dataout: cannot allocate finalcluster\n");
        return Err_NoMem;
    }

    clusters=0;
    last_deposited_cluster=0;
    vedinfos.num_veds=-1;
    printf("start_dataout: vedinfos.info=%p, num_veds set to %d\n", vedinfos.info, vedinfos.num_veds);
    start_all_dataouts();

    return OK;
}
#endif /* READOUT_CC */
/*****************************************************************************/
plerrcode current_filename(unsigned int idx, const char **name)
{
    *name=0;
    if (idx>=MAX_DATAOUT || dataout[idx].bufftyp==-1)
        return plErr_ArgRange;
    if (dataout_cl[idx].procs.filename)
        return dataout_cl[idx].procs.filename(idx, name);
    else
        return plErr_NotImpl;
}
/*****************************************************************************/
static void
clusters_deposit_final(int do_idx)
{
    struct timeval tv;
    int i;

    T(dataout/clusters/cluster.c:clusters_deposit_final)

/*
  wenn Dataout einzeln removed werden, wird immer der gleiche Cluster
  deponiert, auch, wenn er noch nicht wieder frei ist.
  Schaeden? Ja: time wird corrupt sein
*/
    finalcluster->size=7;
    finalcluster->type=clusterty_no_more_data;
    ClLEN(finalcluster->data)=6;
    ClENDIEN(finalcluster->data)=0x12345678;
    ClTYPE(finalcluster->data)=clusterty_no_more_data;
    ClOPTSIZE(finalcluster->data)=3;
    ClOPTFLAGS(finalcluster->data)=ClOPTFL_TIME;
    gettimeofday(&tv, 0);
    finalcluster->data[5]=tv.tv_sec;
    finalcluster->data[6]=tv.tv_usec;


    for (i=0; i<MAX_DATAOUT; i++)
        finalcluster->costumers[i]=0;

    if (do_idx>=0) {
        finalcluster->predefined_costumers=1;
        finalcluster->costumers[do_idx]=1;
    } else {
        finalcluster->predefined_costumers=0;
    }

    clusters_deposit(finalcluster);
}
/*****************************************************************************/
/*
 * called from resetreadout in readout_cc/readout.c and
 * readout_em_cluster/piops.c
 */
errcode stop_dataouts(void)
{
    T(dataout/cluster/cl_interface.c:stop_dataouts)

#ifdef READOUT_CC
    flush_current_cluster();
#endif

    clusters_deposit_final(-1);

    return OK;
}
/*****************************************************************************/
#if 0
errcode stop_dataout(int do_idx)
{
/*
 * called from remove_dataout
 */
    T(dataout/cluster/cl_interface.c:stop_dataout)

    printf("stop_dataout; called from %s\n", caller);
    if (dataout_cl[do_idx].do_status!=Do_error)
            dataout_cl[do_idx].do_status=Do_flushing;

    clusters_deposit_final(do_idx);
    return OK;
}
#endif
/*****************************************************************************/
/*
 * idx    : dataout_idx
 * clustertype: 0: header included in data;
 *              1, 0x10000000: not allowed
 *              2: clusterty_text
 *              3: clusterty_wendy_setup
 *              4: clusterty_file
 * data   : data to be written, first word is size
 */
/*
 * Cluster: (clusterty_text or clusterty_wendy_setup)
 * [0] size
 * [1] endientest=0x12345678
 * [2] typ=clusterty_text or clusterty_wendy_setup
 * [3] options (x=timestamp?2:0+checksum?1:0+(timestamp||checksum)?1:0)
 * [4+x] flags
 * [5+x] fragment_id
 * [6+x] <>string<> or other data
 */
errcode writeoutput(unsigned int idx, int cl_type, ems_u32* data)
{
    int size;
    struct Cluster* cluster;
    int nidx=4;
    int cl_flags, cl_fraq, cl_start, cl_size;

    T(dataout/cluster/cl_interface.c:writeoutput)
    if (dataout[idx].bufftyp==-1) return Err_NoDo;
    if (cl_type!=0) {
        if ((cl_type<clusterty_text) || (cl_type>clusterty_file)) {
            printf("writeoutput: clustertype %d not allowed\n", cl_type);
            return Err_ArgRange;
        }
    }

    if (dataout_cl[idx].do_status!=Do_running) {
        errcode res;
        printf("writeoutput: dataout %d not yet running; will start it\n", idx);
        res=start_one_dataout(idx);
        if (res!=OK)
            return res;
    }

    if (cl_type>0) {
        cl_flags=0;
        cl_fraq=0;
        cl_start=1; cl_size=data[0];
    } else {
        cl_type=data[1];
        /* data[2] (options) ignored */
        cl_flags=data[3];
        cl_fraq=data[4];
        cl_start=5; cl_size=data[0]-4;
    }

    size=5+cl_size;
#if defined(CLUSTERTIMESTAMPS) || defined (CLUSTERCHECKSUM)
    size++;

#if defined(CLUSTERTIMESTAMPS)
    size+=2;
#endif

#if defined(CLUSTERCHECKSUM)
    size+=CHECKSUMSIZE+2; /* type, count, CHECKSUMSIZE*data */
#endif

#endif

    if (size>cluster_max) {
        printf("writeoutput: clustersize=%d, requested size=%d\n",
            cluster_max, size);
        return Err_BufOverfl;
    }

#ifdef READOUT_CC
    cluster=clusters_create(size+HEADSIZE, "write");
#else
    cluster=clusters_create(size+HEADSIZE, MAX_DATAIN, "write");
#endif
    if (cluster==0) return Err_NoMem;

    ClLEN(cluster->data)=size;
    ClENDIEN(cluster->data)=0x12345678;     /* endientest */
    ClTYPE(cluster->data)=cl_type;
    cluster->optsize=0;

#if defined(CLUSTERTIMESTAMPS) || defined (CLUSTERCHECKSUM)

    cluster->data[nidx++]=0;
    cluster->optsize++;

#if defined(CLUSTERTIMESTAMPS)
    {
    struct timeval tv;
    gettimeofday(&tv, 0);
    cluster->data[nidx++]=tv.tv_sec;
    cluster->data[nidx++]=tv.tv_usec;
    cluster->optsize+=2;
    ClOPTFLAGS(cluster->data)|=ClOPTFL_TIME;
    }
#endif

#if defined (CLUSTERCHECKSUM)
    cluster->data[nidx++]=CHECKSUMTYPE;
    cluster->data[nidx++]=CHECKSUMSIZE;

    bzero(cluster->data+nidx, CHECKSUMSIZE*4);
    nidx+=CHECKSUMSIZE;
    cluster->optsize+=CHECKSUMSIZE;
    ClOPTFLAGS(cluster->data)|=ClOPTFL_CHECK;
#endif

#endif

    ClOPTSIZE(cluster->data)=cluster->optsize;
    cluster->data[nidx++]=cl_flags; /* flags */
    cluster->data[nidx++]=cl_fraq; /* fragment_id */
    memcpy(cluster->data+nidx, data+cl_start, cl_size*sizeof(ems_u32));

    cluster->size=size+1;
    cluster->type=cl_type;
    cluster->flags=cl_flags;
    cluster->swapped=0;
    clusters_conv_to_network(cluster);
    cluster->predefined_costumers=1;
    cluster->costumers[idx]=1;
    clusters_deposit(cluster);
    return OK;
}
/*****************************************************************************/
#ifdef READOUT_CC
static void
prepare_eventcluster(struct Cluster* cluster)
{
    int nidx=4;
    T(dataout/cluster/cl_interface.c:prepare_cluster)

    /* ClLEN: in flush_databuf */
    ClENDIEN(cluster->data)=0x12345678;     /* endientest */
    ClTYPE(cluster->data)=clusterty_events; /* type event */
    /* ClOPTSIZE: spaeter */

    /* size: in flush_databuf */
    cluster->type=clusterty_events;
    cluster->flags=0; /* bisher ungenutzt */
    /* firstevent: in flush_databuf */
    /* numevents: in flush_databuf */
    cluster->swapped=0;
    cluster->optsize=0;
    cluster->ved_id=ved_globals.ved_id;

#if defined(CLUSTERTIMESTAMPS) || defined (CLUSTERCHECKSUM)

    cluster->data[nidx++]=0;
    cluster->optsize++;

#if defined(CLUSTERTIMESTAMPS)
    {
    struct timeval tv;
    gettimeofday(&tv, 0);
    cluster->data[nidx++]=tv.tv_sec;
    cluster->data[nidx++]=tv.tv_usec;
    cluster->optsize+=2;
    cluster->data[4]|=ClOPTFL_TIME;
    }
#endif

#if defined (CLUSTERCHECKSUM)
    cluster->data[nidx++]=CHECKSUMTYPE;
    cluster->data[nidx++]=CHECKSUMSIZE;

    bzero(cluster->data+nidx, CHECKSUMSIZE*4);
    nidx+=CHECKSUMSIZE;
    cluster->optsize+=CHECKSUMSIZE;
    cluster->data[4]|=ClOPTFL_CHECK;
#endif

#endif

    cluster->data[nidx++]=0;                  /* flags */
    cluster->data[nidx++]=ved_globals.ved_id; /* ved_id */
    cluster->data[nidx++]=0;                  /* fragment id */
    cluster->data[nidx++]=0;                  /* number of events */
    ClOPTSIZE(cluster->data)=cluster->optsize;
    ClLEN(cluster->data)=7+cluster->optsize;  /* size */
}
#endif
/*****************************************************************************/
#ifdef READOUT_CC
void schau_nach_speicher(void)
{
    T(dataout/cluster/cl_interface.c:schau_nach_speicher)
    if ((currentcluster=clusters_create(cluster_max+event_max, 0))!=0) {
#ifdef USE_RESERVE
        if (reserve_used) {
            struct Cluster* help=reservecluster;
            reservecluster=currentcluster;
            currentcluster=help;
            next_databuf=currentcluster->data+ClLEN(currentcluster->data)+2;
            reserve_used=0;
        } else 
#endif
               {
            prepare_eventcluster(currentcluster);
            next_databuf=&ClEVID1(currentcluster->data);
        }
        if (next_databuf==0) {
            printf("schau_nach_speicher: next_databuf=0\n");
            *(int*)0=0;
        }
        buffer_free=1;
    }
}
#endif
/*****************************************************************************/
#ifdef READOUT_CC
static void
flush_cluster(void)
{
    currentcluster->size=ClLEN(currentcluster->data)+1;
    currentcluster->numevents=ClEVNUM(currentcluster->data);
    if (currentcluster->numevents>0)
        currentcluster->firstevent=ClEVID1(currentcluster->data);
    else
        currentcluster->firstevent=-1;
#if 0
    printf("  flushing cluster: events: %d+%d\n",
            currentcluster->firstevent, currentcluster->numevents);
#endif

    clusters_deposit(currentcluster);

    currentcluster=0;
    next_databuf=0;
    buffer_free=0;
    schau_nach_speicher();
/*
This is done in objects/pi/readout_cc/readout.c:doreadout
    if (!buffer_free) {
        suspend_trigger_task("flush_cluster");
        suspended=1;
        suspensions++;
    }
*/
}
#endif
/*****************************************************************************/
#ifdef READOUT_CC
static void
flush_current_cluster(void)
{
    if (currentcluster==0) {
        printf("flush_current_cluster: no current cluster\n");
        return;
    }

    if (ClEVNUM(currentcluster->data)==0) {
        if (!currentcluster->numcostumers) {
            clusters_recycle(currentcluster);
            return;
        }
        printf("flush_current_cluster: current cluster is empty, "
                "but linkcount is %d\n",
                currentcluster->numcostumers);
    }

    if (read_delayed()<0) {
        fatal_readout_error();
        return;
    }

    flush_cluster();
}
#endif
/*****************************************************************************/
#ifdef READOUT_CC
static int
should_send_cluster(struct Cluster* cluster)
{
/*
 * Old version checks if the number of events inside the cluster
 * is larger than maxev_procluster.
 * This does not work in case of LVD-demand-DMA because many real events
 * are hidden inside an 'event' seen by the cluster logic.
 * Reason is that a trigger now belongs to a DMA block and not a single
 * event.
 * Additionally an arbitrary amount of event numbers may be skipped by
 * the synchronisation system. 
 * New logic:
 * A cluster is to be sent if the real event number (eventcnt) has crossed
 * a multiple of maxev_procluster.
 * A cluster is also be sent if a time more than max_time_per_cluster
 * is elapsed.
 * In both cases an event is to be sent, even if it is empty.
 */

    static ems_u64 q_time=0;
    static ems_u32 q_evt=0;

    if (max_ev_per_cluster) {
        ems_u32 q;
        q=trigger.eventcnt/max_ev_per_cluster;
        if (q!=q_evt) {
#if 0
            printf("cluster: max_ev  , ev=%lu q=%lu\n",
                (unsigned long)trigger.eventcnt, (unsigned long)q);
#endif
            q_evt=q;
            return 1;
        }
    }

    if (max_time_per_cluster) {
        ems_u64 msec; /* ms since 1970-01-01 00:00:00 UTC */
        ems_u64 q;    /* trigger.time/max_time_per_cluster */
#if 0
        int nsec;
#endif

        msec=trigger.time.tv_sec;
        msec*=1000;
        msec+=trigger.time.tv_nsec/1000000;
#if 0
        nsec=trigger.time.tv_nsec%1000000;
#endif
        q=msec/max_time_per_cluster;
        if (q!=q_time) {
#if 0
            printf("cluster: max_time, ev=%lu msec=%llu nsec=%7d q=%llu\n",
                (unsigned long)trigger.eventcnt,
                (unsigned long long)msec,
                nsec,
                (unsigned long long)q);
#endif
            q_time=q;
            return 1;
        }
    }

    return 0;
}
#endif
/*****************************************************************************/
#ifdef READOUT_CC
#ifdef USE_RESERVE
void
flush_databuf(ems_u32* p)
{
    int len;
    T(dataout/cluster/cl_interface.c:flush_databuf)

    /* sanity check */
    if (p==0) {
        printf("flush_databuf: p=0\n");
        *(int*)0=0;
    }

    len=p-next_databuf;
    next_databuf[-1]=len;

    /* cluster overfull? */
    if ((ClLEN(currentcluster->data)+len+1>=cluster_max) ||
            should_send_cluster(currentcluster)){
        /* read delayed data from some hardware */
        if (read_delayed()<0) {
            fatal_readout_error();
            return;
        }
        prepare_eventcluster(reservecluster);
        ClLEN(reservecluster->data)+=len+1;
        ClEVNUM(reservecluster->data)=1;
        bcopy((void*)(next_databuf-1),
            (void*)(&ClEVSIZ1(reservecluster->data)),
            (len+1)*sizeof(ems_u32));
        copycount++;
        last_databuf=&ClEVSIZ1(reservecluster->data);
        assumed_event_size=ClLEN(currentcluster->data)/
                ClEVNUM(currentcluster->data)+1;
        flush_cluster();
        return;
    }

    /* adjust cluster fields for new data */
    last_databuf=next_databuf-1;
    next_databuf+=len+1;
    ClLEN(currentcluster->data)+=len+1;
    ClEVNUM(currentcluster->data)++;

    /* cluster full? */
    if ((ClLEN(currentcluster->data)+assumed_event_size>=cluster_max) ||
            check_maxev_procluster(currentcluster)) {
        if (read_delayed()<0) {
            fatal_readout_error();
            return;
        }
        assumed_event_size=ClLEN(currentcluster->data)/
                ClEVNUM(currentcluster->data)+1;
        flush_cluster();
    }
}
#else
void
flush_databuf(ems_u32* p)
{
    int len;
    T(dataout/cluster/cl_interface.c:flush_databuf)

    /* sanity check */
    if (p==0) {
        printf("flush_databuf: p=0\n");
        *(int*)0=0;
    }

    len=p-next_databuf;
    next_databuf[-1]=len;

    /* adjust cluster fields for new data */
    last_databuf=next_databuf-1;
    next_databuf+=len+1;
    ClLEN(currentcluster->data)+=len+1;
    ClEVNUM(currentcluster->data)++;

    /* cluster full? */
    if ((ClLEN(currentcluster->data)+assumed_event_size>=cluster_max) ||
            should_send_cluster(currentcluster)) {
        /* read delayed data from some hardware */
        if (read_delayed()<0) {
            fatal_readout_error();
            return;
        }
        assumed_event_size=ClLEN(currentcluster->data)/
                ClEVNUM(currentcluster->data)+1;
        flush_cluster();
    }
}
#endif
#endif
/*****************************************************************************/
#ifdef READOUT_CC
void extra_do_data(void)
{
    *outptr++=assumed_event_size;
#ifdef USE_RESERVE
    *outptr++=copycount;
#else
    *outptr++=0;
#endif
}
#else
void extra_do_data(void) {}
#endif
/*****************************************************************************/
/*****************************************************************************/
