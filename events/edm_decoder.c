/*
 * clusterdecoder.c
 * created 08.Apr.2004 PW
 * $ZEL: emsdecoder.c,v 1.1 2006/02/17 15:15:23 wuestner Exp $
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <utime.h>
#include <unistd.h>
#include <libgen.h>

//#include <emsctypes.h>
//#include <xdrstring.h>
#include <clusterformat.h>
//#include <swap.h>

#include "read_procfile.h"

#define GPX_RES 92.59

static int ip;   /* input path; normally 0 */
static FILE* op; /* output path; normally stdin */
static FILE* ep; /* path for messages and statistics; normally stderr */

static ems_u32* cldata;
static int clsize;
static int cldatasize;

/*
static int clnum_events;
static int clnum_ved_info;
static int clnum_text;
static int clnum_wendy_setup;
static int clnum_file;
static int clnum_no_more_data;
*/
struct statist {
    int t_events;
    int u_events;
};
static struct statist statist;

#define MAX_CLUSTERSIZE 16384 /* 64 KByte */

#define SWAP_32(n) (((n & 0xff000000) >> 24) | \
    ((n & 0x00ff0000) >>  8) | \
    ((n & 0x0000ff00) <<  8) | \
    ((n & 0x000000ff) << 24))

/******************************************************************************/
static void
printusage(char* argv0)
{
    fprintf(stderr, "usage: %s [-h]\n",
        basename(argv0));
    fprintf(stderr, "  -h: this help\n");
}
/*****************************************************************************/
static int
readargs(int argc, char* argv[])
{
    const char* args="hf:p:";
    int c, err;

    err=0;
    while (!err && ((c=getopt(argc, argv, args)) != -1)) {
        switch (c) {
        case 'h':
            printusage(argv[0]);
            return 1;
        default:
            err=1;
        }
    }
    if (err) {
        printusage(argv[0]);
        return -1;
    }
    return 0;
}
/******************************************************************************/
static int
xread(int p, ems_u32* buf, int len)
{
    int restlen, da;

    restlen=len*sizeof(ems_u32);
    while (restlen) {
        da=read(p, buf, restlen);
        if (da>0) {
            buf+=da;
            restlen-=da;
        } else if (errno==EINTR) {
            continue;
        } else {
            printf("xread: %s\n", strerror(errno));
            return -1;
        }
    }
    return 0;
}
/******************************************************************************/
#if 0
static void
dump_data(ems_u32* d, int num, char* text)
{
    int i;

    if (text) {
        fprintf(ep, "dump_data: %s\n", text);
    }
    for (i=1; num; num--, i++) {
        fprintf(ep, "0x%08x ", *d++);
        if (!(i&3)) fprintf(ep, "\n");
    }
    fprintf(ep, "\n");
}
#endif
/******************************************************************************/
#if 0
static void
dump_cluster(int maxwords, char* text, int code)
{
    fprintf(ep, "dump_cluster%s (%d)", maxwords>0?"head":"", code);
    if (text && *text) fprintf(ep, " %s", text);
    fprintf(ep, "\n");

    maxwords=(maxwords<=0)||(maxwords>clsize)?clsize:maxwords;

    dump_data(cldata, maxwords, 0);
}
#endif
/******************************************************************************/
static int
decode_timestamp(ems_u32* data, int size)
{
    struct timeval tv;
    char s[1024];
    struct tm* time;
    time_t tt;

    if (size<2) {
        fprintf(ep, "timestamp: too short");
        return -1;
    }
    tv.tv_sec=*data++;
    tv.tv_usec=*data;
    tt=tv.tv_sec;
    time=localtime(&tt);
    strftime(s, 1024, "%c", time);
    /*fprintf(op, "timestamp: %d:%06d (%s)\n", tv.tv_sec, tv.tv_usec, s);*/
    return 2;
}
/******************************************************************************/
static int
decode_checksum(ems_u32* data, int size)
{
    fprintf(ep, "checksum (not implemented)\n");
    return -1;
}
/******************************************************************************/
static int
decode_options(ems_u32* data, int size)
{
    int idx=0, optsize, flags, res;

    if (size<1) {
        fprintf(ep, "options: too short\n");
        return -1;
    }
    optsize=data[idx++];
    if (size<optsize+1) {
        fprintf(ep, "options: size=%d, optsize=%d\n", size, optsize);
        return -1;
    }
    if (!optsize) return idx;
    flags=data[idx++];
    if (flags&1) {
        res=decode_timestamp(data+idx, size-idx);
        if (res<0) return -1;
        idx+=res;
    }
    if (flags&2) {
        res=decode_checksum(data+idx, size-idx);
        if (res<0) return -1;
        idx+=res;
    }
    if (flags&~3) {
        fprintf(ep, "options: unknown flags 0x%08x\n", flags);
        return -1;
    }

    return idx;
}
/******************************************************************************/
static int
decode_proc_Timestamp(ems_u32* data, int size, struct proc* proc)
{
    struct tm* tm;
    time_t t;
    char str[1024];

    if (size<2) {
        fprintf(ep, "  Timestamp: size=%d (not 2)\n", size);
        return -1;
    }
    t=data[0];
    tm=localtime(&t);
    strftime(str, 1024, "%c", tm);
    fprintf(op, "  Timestamp: %s\n", str);
    return 2;
}
/******************************************************************************/
#if 0
static int
decode_proc_GetIntVar(ems_u32* data, int size, struct proc* proc)
{
    int length;

    length=proc->args[0];
    if (size<length) { /* number of words */
        fprintf(ep, "GetIntVar: not enough data\n");
        return -1;
    }
    return length;
}
#endif
/******************************************************************************/
#if 0
static int
decode_proc_GetPCITrigData(ems_u32* data, int size, struct proc* proc)
{
    int length;

    length=16;
    if (size<length) { /* number of words */
        fprintf(ep, "GetPCITrigData: not enough data\n");
        return -1;
    }
    return length;
}
#endif
/******************************************************************************/
static int
decode_proc_Scaler_read(ems_u32* data, int size, struct proc* proc)
{
    u_int32_t length;

    if (size<1) {
        fprintf(ep, "Scaler_read: not enough data for word counter\n");
        return -1;
    }
    length=data[0];
    if (size<1+length*2) {
        fprintf(ep, "Scaler_read: not enough data\n");
        return -1;
    }

    fprintf(op, "Scaler_read: %d double words\n", length);

    return 1+2*length;
}
/******************************************************************************/
static int
decode_proc_BCT(ems_u32* data, int size, struct proc* proc)
{
    u_int32_t length;
    int i;

    if (size<1) {
        fprintf(ep, "BCT: not enough data for word counter\n");
        return -1;
    }
    length=data[0];
    if (size<1+length) {
        fprintf(ep, "BCT: not enough data\n");
        return -1;
    }

    fprintf(op, "BCT: %d words\n", length);
    for (i=1; i<=length; i++) {
        fprintf(op, "  [%2d] %08x\n", i, data[i]);
    }

    return 1+length;
}
/******************************************************************************/
static int
decode_proc_RunNr(ems_u32* data, int size, struct proc* proc)
{
    u_int32_t runnr;

    if (size<1) {
        fprintf(ep, "RunNr: size=%d\n", size);
        return -1;
    }

    runnr=data[0];

    fprintf(op, "RunNr: %u\n", runnr);

    return 1;
}
/******************************************************************************/
static int
decode_proc_LEMO_In(ems_u32* data, int size, struct proc* proc)
{
    u_int32_t pattern;

    if (size<1) {
        fprintf(ep, "LEMO_In: size=%d\n", size);
        return -1;
    }

    pattern=data[0];

    fprintf(op, "LEMO_In: %08x\n", pattern);

    return 1;
}
/******************************************************************************/
#if 0
static int
dump_subevent(ems_u32* data, int size, struct proc* proc)
{
    int i;

    fprintf(op, "dump_subevent: size=%d\n", size);
    for (i=0; i<size; i++) {
        fprintf(op, "  [%3d] %10u %08x\n", i, data[i], data[i]);
    }
    return size;
}
#endif
/******************************************************************************/
static int
decode_lvd_head_triggered(ems_u32* data, int idx)
{
    static double last_stamp=0;
    static double correction=0;
    int ovfl=0;
    double stamp; /* timestamp in ns */

    fprintf(op, "%3d [ht  %d] %08x bytes=%d\n",
            idx, 0, data[0], data[0]&0x3ffff);
    fprintf(op, "%3d [ht  %d] %08x timestamp:  %d*92.59ps +%d*6.4us\n",
            idx+1, 1, data[1],
            data[1]&0x1ffff,
            data[1]>>17);
    fprintf(op, "%3d [ht  %d] %08x evnr=%d\n",
            idx+2, 2, data[2], data[2]);

    /* 1st part of overflow counter: bit 31..17 of data[1] */
    /* 2nd part of overflow counter: bit 28..18 of data[0] */
    ovfl=(data[1]>>17)&0x7ffff;
    ovfl+=((data[0]>>18)&0x7ff)<<15;
    stamp=ovfl*6400.; 
    stamp+=((data[1]&0x1ffff)*GPX_RES)/1000.;
    if (stamp+correction<last_stamp)
        correction+=209715200.; /* 2^15*6.4us */
    
    stamp+=correction;
    fprintf(op, "#HEAD       %13.3f ns  %13.3f\n", stamp, stamp-last_stamp);
    last_stamp=stamp;

    return 3;
}
/******************************************************************************/
static int
decode_lvd_head_async(ems_u32* data, int idx)
{
    fprintf(op, "%3d [ha  %d] %08x bytes=%d\n",
            idx, 0, data[0], data[0]&0x3ffff);
    fprintf(op, "%3d [ha  %d] %08x timestamp: \n", idx+1, 1, data[1]);
    fprintf(op, "%3d [ha  %d] %08x evnr=%d\n", idx+2, 2, data[2], data[2]);
    return 3;
}
/******************************************************************************/
static int
decode_lvd_sm(ems_u32* data, int idx)
{
    /* sync master */
    fprintf(op, "%3d [sm  %d] %08x\n", idx, 0, data[0]);
    return 1;
}
/******************************************************************************/
static int
decode_lvd_sc(ems_u32* data, int idx)
{
    /* sync communication */
    fprintf(op, "%3d [sc  %d] %08x\n", idx, 0, data[0]);
    return 1;
}
/******************************************************************************/
static int
decode_lvd_tdc(ems_u32* data, int idx)
{
    static double last_stamp_all=0;
    static double last_stamp[64]={
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };
    static int correction=0;
    int chan, ovfl=0, wrapped=0;
    double stamp; /* timestamp in ns */

    fprintf(op, "%3d [tdc %d] %08x chan %d edge %d %d*92.59ps +%d*6.4us\n",
            idx, 0, data[0],
            (data[0]>>22)%0x3f,
            (data[0]>>17)%1,
            data[0]&0x1ffff,
            (data[0]>>18)&0xf
            );
    fprintf(op, "%3d [tdc %d] %08x         +%d*6.4us\n",
            idx+1, 1, data[1],
            (data[1]&0xffff)*16);

    chan=(data[0]>>22)%0x3f;
    if (chan!=16)
        return 2;

    ovfl=(data[0]>>18)&0xf;
    ovfl+=(data[1]&0xffff)<<4;
    stamp=ovfl*6400.; 
    stamp+=((data[0]&0x1ffff)*GPX_RES)/1000.;
    stamp+=correction*6710886400.; /* 2^20*6.4us */
    if (stamp<last_stamp_all) {
        correction++;
        stamp+=6710886400;
        wrapped=1;
        /* fprintf(op, "#TDC wrapped\n"); */
    }

    fprintf(op, "#TDC %2d ", chan);
    if (wrapped)
        fprintf(op, "%4d", correction);
    else
        fprintf(op, "    ");
    fprintf(op, "    %18.3f ns  %15.3f\n",
            stamp, stamp-last_stamp[chan]);
    last_stamp[chan]=stamp;
    last_stamp_all=stamp;

    return 2;
}
/******************************************************************************/
#define ADDR_TDC 0 /* TDC */
#define ADDR_SM 1 /* sync master */
#define ADDR_SC 2 /* sync communicator */
#define ADDR_CONTR 15 /* controller */

static int
decode_lvd_read_single(ems_u32* data, int size, struct proc* proc)
{
    int idx, res, addr;

    idx=0;
    fprintf(op, "lvd_read_single: size=%d\n", size);
    fprintf(op, "%3d [%5d] %08x size=%d\n", idx, idx, data[idx], data[idx]);
    idx++;

    if (data[idx]&0x20000000) {
        res=decode_lvd_head_async(data+idx, idx);
    } else {
        res=decode_lvd_head_triggered(data+idx, idx);
    }
    if (res<0)
        return -1;
    idx+=3;

    for (; idx<size;) {
        addr=(data[idx]>>28)&0xf;
        switch (addr) {
        case ADDR_SM:
            res=decode_lvd_sm(data+idx, idx);
            break;
        case ADDR_SC:
            res=decode_lvd_sc(data+idx, idx);
            break;
        case ADDR_TDC:
            res=decode_lvd_tdc(data+idx, idx);
            break;
        case ADDR_CONTR:
            /* data from controller not used by EDM DAQ */
        default:
            fprintf(ep, "lvd_read_single: unexpected addr %d\n", addr);
            return -1;
        }
        if (res<0)
            return -1;
        idx+=res;
    }

    return size;
}
/**##########################################################################**/
/**##########################################################################**/
static struct proc proc_2000_1 = {
    "lvd_read_single",
    decode_lvd_read_single,
    0,
    0,
};

static struct proc *procs_2000_A[] = {
    &proc_2000_1,
    0,
};

static struct proclist list_2000_A = {
    -1, /* all triggers */
    procs_2000_A,
};

static struct proclist *lists_2000[] = {
    &list_2000_A,
    0,
};

static struct is is_2000 = {
    2000, /* ID */
    0,    /* LVDS (for DDMA mode only) */
    lists_2000,
};
static struct is is_40 = {
    40, /* ID */
    0,    /* LVDS (for DDMA mode only) */
    lists_2000,
};

/*---*/

static struct proc proc_Scaler_read = {
    "ScalerXXXX_update_read",
    decode_proc_Scaler_read,
    0,
    0,
};

static struct proc *procs_30[] = {
/*
Scaler4434_update_read "$var 1 0xffffffff"
Scaler4434_update_read "$var 2 0xffffffff"
Scaler2551_update_read "$var 3 0xfff"
*/
    &proc_Scaler_read,
    &proc_Scaler_read,
    &proc_Scaler_read,
    0,
};

static struct proclist list_30 = {
    -1,
    procs_30,
};

static struct proclist *lists_30[] = {
    &list_30,
    0,
};

static struct is is_30 = {
    30, /* ID */
    0,
    lists_30,
};


/*---*/

static struct proc proc_RunNr = {
    "RunNr",
    decode_proc_RunNr,
    0,
    0,
};

static struct proc *procs_0[] = {
    &proc_RunNr,
    0,
};

static struct proclist list_0 = {
    -1,
    procs_0,
};

static struct proclist *lists_0[] = {
    &list_0,
    0,
};

static struct is is_0 = {
    0, /* ID */
    0,
    lists_0,
};

/*---*/

static struct proc proc_Timestamp = {
    "Timestamp",
    decode_proc_Timestamp,
    0,
    0,
};

static struct proc *procs_1[] = {
    &proc_Timestamp,
    0,
};

static struct proclist list_1 = {
    -1,
    procs_1,
};

static struct proclist *lists_1[] = {
    &list_1,
    0,
};

static struct is is_1 = {
    1, /* ID */
    0,
    lists_1,
};

/*---*/

static struct proc proc_LEMO_In = {
    "LEMO_In",
    decode_proc_LEMO_In,
    0,
    0,
};

static struct proc *procs_2[] = {
    &proc_LEMO_In,
    0,
};

static struct proclist list_2 = {
    -1,
    procs_2,
};

static struct proclist *lists_2[] = {
    &list_2,
    0,
};

static struct is is_2 = {
    2, /* ID */
    0,
    lists_2,
};

/*---*/

static struct proc *procs_5[] = {
/*
Scaler4434_update_read "$var 1 0xffffffff"
Scaler2551_update_read "$var 2 0x00000fff"
*/
    &proc_Scaler_read,
    &proc_Scaler_read,
    0,
};

static struct proclist list_5 = {
    -1,
    procs_5,
};

static struct proclist *lists_5[] = {
    &list_5,
    0,
};

static struct is is_5 = {
    5, /* ID */
    0,
    lists_5,
};

/*---*/

static struct proc *procs_20[] = {
/*
Scaler4434_update_read "$var 1 0xffffffff"
*/
    &proc_Scaler_read,
    0,
};

static struct proclist list_20 = {
    -1,
    procs_20,
};

static struct proclist *lists_20[] = {
    &list_20,
    0,
};

static struct is is_20 = {
    20, /* ID */
    0,
    lists_20,
};

/*---*/

static struct proc proc_BCT = {
    "BCT",
    decode_proc_BCT,
    0,
    0,
};

static struct proc *procs_100[] = {
    &proc_BCT,
    0,
};

static struct proclist list_100 = {
    -1,
    procs_100,
};

static struct proclist *lists_100[] = {
    &list_100,
    0,
};

static struct is is_100 = {
    100, /* ID */
    0,
    lists_100,
};

/*---*/

static struct is *iss[] = {
    &is_2000,
    &is_40,   /* 2000 renamed to 40 */
    &is_100,
    &is_30,
    &is_20,
    &is_5,
    &is_2,
    &is_1,
    &is_0,
    0,
};
/**##########################################################################**/
/**##########################################################################**/
static int
decode_subevent(ems_u32* data, int size, int ved, ems_u32 evno, ems_u32 trigno)
{
    int ssize, idx=0, sidx;
    ems_u32 *sdata, is_id;
    struct is **isp;
    struct is *is;
    struct proclist **proclistp;
    struct proclist *proclist;
    struct proc **procp;

    if (size<2) {
        fprintf(ep, "subevent too short\n");
        return -1;
    }
    is_id=data[idx++];
    ssize=data[idx++];
    if (size-idx<ssize) {
        fprintf(ep, "subevent: subeventsize=%d size=%d\n", ssize, size);
        return -1;
    }
#if 1
    fprintf(op, "subevent: id=%d size=%d\n", is_id, ssize);
#endif

    /* find subevent description */
    isp=iss;
    while (*isp && (*isp)->id!=is_id)
        isp++;
    is=*isp;
    if (!is) {
        fprintf(ep, "description for subevent %d not found\n", is_id);
        return -1;
    }
#if 0
    fprintf(ep, "description for subevent %d FOUND\n", is_id);
#endif

    /* find procedurelist */
    proclistp=is->proclists;
    while (*proclistp && (*proclistp)->trigger!=-1 &&
            (*proclistp)->trigger!=trigno)
        proclistp++;
    proclist=*proclistp;
    if (!proclist) {
        fprintf(ep, "procedurelist for IS %d trigger %d not found\n",
                is_id, trigno);
        return -1;
    }
#if 0
    fprintf(ep, "procedurelist for IS %d trigger %d FOUND\n", is_id, trigno);
#endif

    sdata=data+idx;
    sidx=0;
    for (procp=proclist->procs; *procp; procp++) {
        struct proc* proc=*procp;
        int res;

        res=proc->proc(sdata+sidx, ssize-sidx, proc);
        if (res<0)
            return -1;
        sidx+=res;
    }
    if (sidx!=ssize) {
        fprintf(ep, "ERROR: is %d trigger %d: ssize=%d; used=%d\n",
                is_id, trigno, ssize, sidx);
        return -1;
    }

    return idx+sidx;
}
/******************************************************************************/
static int
decode_event(ems_u32* data, int size, int ved)
{
    int eventsize, num_subevents, idx=0, res, i;
    ems_u32 evno, trigno;

    if (size<4) {
        fprintf(ep, "event too short\n");
        return -1;
    }
    eventsize=data[idx++];
    if (size-idx<eventsize) {
        fprintf(ep, "event: eventsize=%d size=%d\n", eventsize, size);
        return -1;
    }
    evno=data[idx++];
    trigno=data[idx++];
    num_subevents=data[idx++];
#if 1
    fprintf(op, "event: evno=%d trigger=0x%x %d subevents\n",
            evno, trigno, num_subevents);
#endif
    for (i=0; i<num_subevents; i++) {
        res=decode_subevent(data+idx, size-idx, ved, evno, trigno);
        if (res<0)
            return -1;
        idx+=res;
    }

    return idx;
}
/******************************************************************************/
static int
decode_cluster_events(ems_u32* data, int size)
{
    int num_events, flags, ved, fragment, idx=0, res, i;

    res=decode_options(data, size);
    if (res<0)
        return -1;
    idx+=res;

    if (size-idx<4) {
        fprintf(ep, "cluster_events too short\n");
        return -1;
    }
    flags=data[idx++];
    if (flags) {
        fprintf(ep, "cluster_events: unknown flags=0x%08x\n", flags);
        return -1;
    }
    ved=data[idx++];
    /*fprintf(op, "events: ved=%d\n");*/

    fragment=data[idx++];
    if (fragment) {
        fprintf(ep, "cluster_events: fragmented\n");
        return -1;
    }
    num_events=data[idx++];
    for (i=0; i<num_events; i++) {
        res=decode_event(data+idx, size-idx, ved);
        if (res<0)
            return -1;
        idx+=res;
    }
   
    return idx;
}
/******************************************************************************/
static int
decode_ved_info1(ems_u32* data, int size)
{
    int idx=0, ved, num_is, i;

    if (size<2) {
        fprintf(ep, "ved_info type 1: too short\n");
        return -1;
    }
    ved=data[idx++];
    num_is=data[idx++];
    if (size-idx<num_is) {
        fprintf(ep, "ved_info type 1: size=%d num_is=%d\n", size, num_is);
        return -1;
    }
    fprintf(op, "ved_info_1: ved=%d; %d is%s", ved, num_is, num_is?":":"");
    for (i=0; i<num_is; i++)
        fprintf(op, " %d", data[idx++]);
    fprintf(op, "\n");

    return idx;
}
/******************************************************************************/
static int
decode_ved_infos1(ems_u32* data, int size)
{
    int num_infos, i, res, idx=0;

    if (size<1) {
        fprintf(ep, "ved_infos type 1: too short\n");
        return -1;
    }
    num_infos=data[idx++];

    for (i=0; i<num_infos; i++) {
        res=decode_ved_info1(data+idx, size-idx);
        if (res<0)
            return -1;
        idx+=res;
    }

    return idx;
}
/******************************************************************************/
static int
decode_ved_infos2(ems_u32* data, int size)
{
    fprintf(ep, "ved_info type 2 (not implemented)\n");
    return -1;
}
/******************************************************************************/
static int
decode_ved_infos3(ems_u32* data, int size)
{
    fprintf(ep, "ved_info type 3 (not implemented)\n");
    return -1;
}
/******************************************************************************/
static int
decode_cluster_ved_info(ems_u32* data, int size)
{
    int res, version, idx=0;

    fprintf(ep, "cluster_ved_info\n");

    res=decode_options(data, size);
    if (res<0) return -1;
    idx+=res;

    if (size-idx<1) {
        fprintf(ep, "cluster_ved_info too short\n");
        return -1;
    }
    version=data[idx++];
    switch (version) { /* version */
    case 0x80000001:
        res=decode_ved_infos1(data+idx, size-idx);
        break;
    case 0x80000002:
        res=decode_ved_infos2(data+idx, size-idx);
        break;
    case 0x80000003:
        res=decode_ved_infos3(data+idx, size-idx);
        break;
    default:
        fprintf(ep, "cluster_ved_info: unknown version 0x%08x\n", version);
        return -1;
    }
    if (res<0)
        return -1;
    idx+=res;
    return idx;
}
/*****************************************************************************/
static int
decode_cluster(ems_u32* cl, int size)
{
    int res=size-3; /* for ignored cluster types */

    /*dump_cluster(128, "decode_cluster", ClTYPE(cldata));*/
    switch (ClTYPE(cldata)) {
    case clusterty_events:
        res=decode_cluster_events(cl+3, size-3);
        break;
    case clusterty_ved_info:
        res=decode_cluster_ved_info(cl+3, size-3);
        break;
    case clusterty_text:
        break;
#if 0
    case clusterty_x_wendy_setup:
        break;
#endif
    case clusterty_file:
        break;
    case clusterty_no_more_data:
        fprintf(ep, "no more data\n");
        return 1;
    }
    if (res<0)
        return -1;
    if (res!=size-3) {
        fprintf(ep, "ERROR: decode_cluster: cluster not exhausted: size=%d, used=%d\n",
                size, res);
        return -1;
    }

    return 0;
}
/******************************************************************************/
static int
read_cluster(int p)
{
    ems_u32 head[2];
    int wenden, i;

    if (xread(p, head, 2)) return -1;
    switch (head[1]) {
    case 0x12345678: wenden=0; break;
    case 0x78563412: wenden=1; break;
    default:
        printf("unkown endien 0x%x\n", head[1]);
        return -1;
    }
    clsize=(wenden?SWAP_32(head[0]):head[0])+1;
    if (clsize>MAX_CLUSTERSIZE) {
        fprintf(ep, "clustersize=%d\n", clsize);
    }
    if (clsize>cldatasize) {
        if (cldata) free(cldata);
        cldata=malloc(clsize*sizeof(ems_u32));
        if (!cldata) {
            perror("malloc");
            return -1;
        }
    }
    if (xread(p, cldata+2, clsize-2))
        return -1;
    if (wenden) {
        for (i=0; i<clsize; i++) cldata[i]=SWAP_32(cldata[i]);
    }
    cldata[0]=head[0];
    cldata[1]=head[1];
    return clsize;
}
/*****************************************************************************/
static int
do_cluster(void)
{
    int size;
    if ((size=read_cluster(ip))<0)
        return -1;
    if (decode_cluster(cldata, size)<0)
        return -1;
    return 0;
}
/*****************************************************************************/
static void
do_scan(void)
{
    int res;
    do {
        res=do_cluster();
    } while (!res);
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
    if (readargs(argc, argv)<0) return 1;

    ip=0; op=stdout; ep=stdout;
    cldata=0;
    cldatasize=0;

    statist.t_events=0;
    statist.u_events=0;

    do_scan();

    fprintf(ep, "t_events      =%d\n", statist.t_events);
    fprintf(ep, "u_events      =%d\n", statist.u_events);

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
