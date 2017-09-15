/*
 * tpg300_txt.c
 * 
 * created 2011-08-29 PW
 * $ZEL: tpg300_txt.c,v 1.1 2011/08/31 20:24:00 wuestner Exp $
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <arpa/inet.h>

static int loglevel=0;
static u_int32_t *data=0;
static int data_space=0;
static int data_num;
static char *logname=0;
static FILE *logfile;
static FILE *log=0;

#define LOG_(fmt, arg...)        \
    do {                         \
        FILE *l=log?log:stderr;  \
        fprintf(l, fmt, ## arg); \
        fflush(l);               \
    } while (0)
#define LOG(level, fmt, arg...)  \
    do {                         \
        if (level>=loglevel)     \
            LOG_(fmt, ## arg);   \
   } while (0)

#define dLOG(fmt, arg...) LOG(-2, fmt, ## arg)
#define vLOG(fmt, arg...) LOG(-1, fmt, ## arg)
#define eLOG(fmt, arg...) LOG(10, fmt, ## arg)

/******************************************************************************/
static void
printusage(char* argv0)
{
    eLOG("usage: %s [-h] [-4] [-s size] [-q] [-v] [-d] [-l <logname>] "
                 "[-n hours] -i <inport> directory\n", argv0);
    eLOG("  -h: this help\n");
    eLOG("  the input comes from stdin, the output is written to stdout\n");
}
/******************************************************************************/
static int
readargs(int argc, char* argv[])
{
    int c, err;
    err=0;

    while (!err && ((c=getopt(argc, argv, "hqvd64i:n:l:s:")) != -1)) {
        switch (c) {
        case 'h': printusage(argv[0]); return 1;
        case 'q':
            if (loglevel>0 && loglevel<10) /* >10 is forbidden */
                loglevel++;
            else
                loglevel=1;
            break;
        case 'v':
            if (loglevel<0)
                loglevel--;
            else
                loglevel=-1;
            break;
        case 'd':
            if (loglevel<0)
                loglevel--;
            else
                loglevel=-2;
            break;
        case 'l':
            logname=optarg;
            logfile=fopen(logname, "a");
            if (!logfile) {
                eLOG("open logfile \"%s\": %s\n",
                        logname, strerror(errno));
                return -1;
            }
            log=logfile;
            break;
        default: err=1;
        }
    }
    if (err) {
        printusage(argv[0]);
        return -1;
    }

    return 0;
}
/*****************************************************************************/
static int
read_record(void)
{
    u_int32_t size;
    int i, res;
    static int reccount=-1;
//int correct_data_num;

    reccount++;
    res=read(0, &size, 4);
    if (res!=4) {
        eLOG("read: %s\n", strerror(errno));
        return -1;
    }
    //eLOG("size A: %08x\n", size);
    data_num=ntohl(size);
    //eLOG("size of record %d: %08x\n", reccount, data_num);
    if (data_num!=0xe0) {
        eLOG("record has wrong size: %d insted of %d\n", data_num, 0xe0);
        return -1;
    }

//correct_data_num=data_num;
//data_num=(correct_data_num+1)*4-1;
    size=data_num*4;
    //eLOG("size C: %08x\n", size);

    if (data_space<data_num) {
        u_int32_t *d;
        d=realloc(data, 4*data_num);
        if (!d) {
            eLOG("alloc %d words for data: %s\n", data_num, strerror(errno));
            return -1;
        }
        data=d;
        data_space=data_num;
    }

    //eLOG("size=%d\n", size);
//eLOG("read: data=%p\n", data);
    res=read(0, data, size);
    if (res!=size) {
        eLOG("read data: res=%d errno=%s\n", res, strerror(errno));
        return -1;
    }

    for (i=0; i<data_num; i++) {
        data[i]=ntohl(data[i]);
        //eLOG("[%3d] %08x\n", i, data[i]);
    }

//data_num=correct_data_num;
    return 0;
}
/******************************************************************************/
static void
put_block(u_int32_t *d)
{
    int i, idx=0;
    u_int32_t bstate;

#if 0
    u_int32_t addr;
    addr=d[idx++];
#else
    idx++;
#endif

    bstate=d[idx++];
    if (bstate&0xff) {
        printf(" %8d %8d %8d %8d", 1, 1, 1, 1);
        idx+=8;
    } else {
        for (i=0; i<4; i++) {
            u_int32_t state;
            state=d[idx++];
            if (state) {
                printf(" %8d", 1);
                idx++;
            } else {
                float val;
                val=*(float*)(d+idx);
                printf(" %.2e", val);
                idx++;
            }
        }
    }
    printf("   ");
}
/******************************************************************************/
static int
put_record(void)
{
    u_int32_t version;
    int idx=0;

    version=data[idx++];
    switch (version) {
    case 0: {
            u_int32_t sec;
            u_int32_t nr;
            int block;

            sec    =data[idx++];
            printf("%d", sec);
#if 0
            u_int32_t usec;
            usec   =data[idx++];
#else
            idx++;
#endif
            nr     =data[idx++];

            for (block=0; block<nr; block++) {
                u_int32_t n=data[idx++];
                put_block(data+idx);
                idx+=n;
            }
        }
        break;
    default:
        eLOG("unknown version %d\n", version);
        return -1;
    }

    printf("\n");
    return 0;
}
/******************************************************************************/
int
main(int argc, char *argv[])
{
    int res;

    if ((res=readargs(argc, argv)))
        return res<0?1:0;

    while (!read_record()) {
        if (put_record()<0)
            break;
    }

    return 0;
}
/******************************************************************************/
