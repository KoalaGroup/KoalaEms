/*
 * clusterdecoder.c
 * created 07.Feb.2005 PW
 * $ZEL: cl2perfspect.c,v 1.1 2006/02/17 15:15:17 wuestner Exp $
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <utime.h>
#include <unistd.h>
#include <libgen.h>

#include <emsctypes.h>
#include <xdrstring.h>
#include <clusterformat.h>
#include <swap.h>

static int perfIS, isID, procID, valID;
static ems_u32* cldata;
static int clsize;
static int cldatasize;

int spect[2000];
int overflow;

#define MAX_CLUSTERSIZE 16384 /* 64 KByte */

#define SWAP_32(n) (((n & 0xff000000) >> 24) | \
    ((n & 0x00ff0000) >>  8) | \
    ((n & 0x0000ff00) <<  8) | \
    ((n & 0x000000ff) << 24))

/******************************************************************************/
static void
printusage(char* argv0)
{
    fprintf(stderr, "usage: %s [-h ]-i IS -s IS -p Proc\n",
        basename(argv0));
    fprintf(stderr, "  -h: this help\n");
    fprintf(stderr, "  -i: ID of IS containing perfspect data\n");
    fprintf(stderr, "  -s: ID of IS to be converted\n");
    fprintf(stderr, "  -p: ID of procedure to be converted\n");
    fprintf(stderr, "  -v: Value to be converted\n");
}
/*****************************************************************************/
static int
readargs(int argc, char* argv[])
{
    const char* args="hi:s:p:v:";
    int c, err;

    perfIS=isID=procID=-1;

    err=0;
    while (!err && ((c=getopt(argc, argv, args)) != -1)) {
        switch (c) {
        case 'h':
            printusage(argv[0]);
            return 1;
        case 'i':
            perfIS=atoi(optarg);
            break;
        case 's':
            isID=atoi(optarg);
            break;
        case 'p':
            procID=atoi(optarg);
            break;
        case 'v':
            valID=atoi(optarg);
            break;
        default:
            err=1;
        }
    }
    if (err || (perfIS==-1) || (isID==-1) || (procID==-1) || (valID==-1)) {
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
static int
dump_data(ems_u32* d, int num, char* text)
{
    int i;

    if (text) {
        fprintf(stderr, "dump_data: %s\n", text);
    }
    for (i=1; num; num--, i++) {
        fprintf(stderr, "%08x ", *d++);
        if (!(i&7)) fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
}
/******************************************************************************/
static int
dump_cluster(int maxwords, char* text, int code)
{
    fprintf(stderr, "dump_cluster%s (%d)", maxwords>0?"head":"", code);
    if (text && *text) fprintf(stderr, " %s", text);
    fprintf(stderr, "\n");

    maxwords=(maxwords<=0)||(maxwords>clsize)?clsize:maxwords;

    dump_data(cldata, maxwords, 0);
}
/******************************************************************************/
static int
decode_perf_is_debug(ems_u32* data, int size)
{
    static int count=0;
    int code, trig, is, proc, num, val, i;
    if (count++>=4) exit(0);

dump_data(data, size, "mist");
    while (size>0) {
        code=*data++; size--;
        switch (code) {
        case 1:
            trig=*data++; size--;
            is=*data++; size--;
            printf("trig %d is %d\n", trig, is);
            break;
        case 2:
            proc=*data++; size--;
            num=*data++; size--;
            printf("proc %d num %d\n", proc, num);
            for (i=0; (i<num) && (size>=0); i++) {
                val=*data++; size--;
                printf("val %d %08x\n", i, val);
            }
            break;
        default:
            fprintf(stderr, "unknown code %d\n", code);
            return -1;
        }
    }
    return 0;
}
/******************************************************************************/
static void
add_val(ems_u32 val)
{
    val/=1000;
    if (val>=200)
        overflow++;
    else
        spect[val]++;
}
/******************************************************************************/
#if 0
static int
decode_perf_is(ems_u32* data, int size, int index)
{
    static int count=0;
    int vval=-1;
    int code, trig, is, proc, num, val, i;
    /*if (count++>=400) exit(0);*/

    while (size>0) {
        code=*data++; size--;
        switch (code) {
        case 1:
            trig=*data++; size--;
            is=*data++; size--;
            break;
        case 2:
            proc=*data++; size--;
            num=*data++; size--;
            if ((is==isID) && (proc==procID)) {
                for (i=0; (i<num) && (size>=0); i++) {
                    val=*data++; size--;
                    if (i==valID) {
                        vval=val;
                    }
                }
                if (val>10000) {
                    printf("%08x (%d)\n", vval, index);
                } else {
                    /*printf("         %08x\n", vval);*/
                }
            } else {
                data+=num;
                size-=num;
            }
            break;
        default:
            fprintf(stderr, "unknown code %d\n", code);
            return -1;
        }
    }
    return 0;
}
#else
static int
decode_perf_is(ems_u32* data, int size, int index)
{
    static int cc=0;
    ems_u32 time, pid, count;
    time=data[0];
    pid=data[1];
    count=data[2];
/*
 *     if (cc++<50) {
 *         printf("%d %d %d\n", time, pid, count);
 *     }
 */
    if (count!=2) return 0;
    if (pid) {
        printf("%d %d %d\n", time, pid, count);
        add_val(time);
    }
}
#endif
/******************************************************************************/
static int
decode_subevent(ems_u32* data, int size, int ved, ems_u32 evno, ems_u32 trigno,
    int index)
{
    int ssize, idx=0;
    ems_u32 is_id;

/*dump_data(data, size, "sub");*/
    if (size<2) {
        fprintf(stderr, "subevent too short\n");
        return -1;
    }
    is_id=data[idx++];
    ssize=data[idx++];
    if (size-idx<ssize) {
        fprintf(stderr, "subevent: subeventsize=%d size=%d\n", ssize, size);
        return -1;
    }

    if (is_id==perfIS)
        decode_perf_is(data+idx, ssize, index);

    return ssize+2;
}
/******************************************************************************/
static int
decode_event(ems_u32* data, int size, int ved, int index)
{
    int eventsize, num_subevents, idx=0, res, i;
    ems_u32 evno, trigno;

    if (size<4) {
        fprintf(stderr, "event too short\n");
        return -1;
    }
    eventsize=data[idx++];
    if (size-idx<eventsize) {
        fprintf(stderr, "event: eventsize=%d size=%d\n", eventsize, size);
        return -1;
    }
    evno=data[idx++];
    trigno=data[idx++];
    num_subevents=data[idx++];
/*
    fprintf(op, "event: evno=%d trigger=0x%x %d subevents\n",
            evno, trigno, num_subevents);
*/
    for (i=0; i<num_subevents; i++) {
        res=decode_subevent(data+idx, size-idx, ved, evno, trigno, index);
        if (res<0) return -1;
        idx+=res;
    }

    return idx;
}
/******************************************************************************/
static int
decode_cluster_events(ems_u32* data, int size)
{
    int num_events, flags, ved, fragment, idx=0, res, i;
    int optsize;

    if (size<1) {
        fprintf(stderr, "options: too short\n");
        return -1;
    }
    optsize=data[idx++];
    idx+=optsize;

    if (size-idx<4) {
        fprintf(stderr, "cluster_events too short\n");
        return -1;
    }
    flags=data[idx++];
    if (flags) {
        fprintf(stderr, "cluster_events: unknown flags=0x%08x\n", flags);
        return -1;
    }
    ved=data[idx++];
    /*fprintf(op, "events: ved=%d\n");*/

    fragment=data[idx++];
    if (fragment) {
        fprintf(stderr, "cluster_events: fragmented\n");
        return -1;
    }
    num_events=data[idx++];
    for (i=0; i<num_events; i++) {
        res=decode_event(data+idx, size-idx, ved, i);
        if (res<0) return -1;
        idx+=res;
    }
   
    return idx;
}
/******************************************************************************/
static int
read_cluster(int p)
{
    ems_u32 head[2], v;
    ems_u32* data;
    int wenden, evnum, i;

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
        fprintf(stderr, "clustersize=%d\n", clsize);
    }
    if (clsize>cldatasize) {
        if (cldata) free(cldata);
        cldata=malloc(clsize*sizeof(ems_u32));
        if (!cldata) {
            perror("malloc");
            return -1;
        }
    }
    if (xread(p, cldata+2, clsize-2)) return -1;
    if (wenden) {
        for (i=0; i<clsize; i++) cldata[i]=SWAP_32(cldata[i]);
    }
    cldata[0]=head[0];
    cldata[1]=head[1];
    return clsize;
}
/*****************************************************************************/
static int
decode_cluster(ems_u32* cl, int size)
{
    int res=0;

    /*dump_cluster(128, "decode_cluster", ClTYPE(cldata));*/
    switch (ClTYPE(cldata)) {
    case clusterty_events:
        res=decode_cluster_events(cl+3, size-3);
        break;
    case clusterty_ved_info:
        break;
    case clusterty_text:
        break;
    case clusterty_file:
        break;
    case clusterty_no_more_data:
        fprintf(stderr, "no more data\n");
        return 1;
    }
    if (res<0) return -1;
    if (res && (res!=size-3)) {
        fprintf(stderr, "ERROR: decode_cluster: cluster not exhausted: size=%d, used=%d\n",
                size, res);
        return -1;
    }

    return 0;
}
/*****************************************************************************/
static int
do_cluster(void)
{
    int size;
    if ((size=read_cluster(0))<0) return -1;
    if (decode_cluster(cldata, size)<0) return -1;
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
    int i;

    if (readargs(argc, argv)<0) return 1;

    cldata=0;
    cldatasize=0;

    for (i=0; i<2000; i++) spect[i]=0;
    overflow=0;

    do_scan();

    printf("overflow=%d\n", overflow);
    for (i=0; i<200; i++) {
        printf("%4d %8d\n", i, spect[i]);
        spect[i]=0;
    }

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
