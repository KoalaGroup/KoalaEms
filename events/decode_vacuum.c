/*
 * decode_vacuum.c
 * 
 * created 2014-07-04 PW
 * $ZEL: decode_vacuum.c,v 1.2 2014/07/08 12:30:25 wuestner Exp $
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include "swap.h"

int p=0; /* stdin */

/******************************************************************************/
static void
printusage(char* argv0)
{
    printf("usage: %s [-h]\n", argv0);
    printf("  -h: this help\n");
    printf("  (data will read from stdin)\n");
}
/******************************************************************************/
static int
readargs(int argc, char* argv[])
{
    int c, err;
    err=0;

    while (!err && ((c=getopt(argc, argv, "h")) != -1)) {
        switch (c) {
        case 'h': printusage(argv[0]); return 1;
        default: err=1;
        }
    }

    return 0;
}
/*****************************************************************************/
static int
xreceive(int path, u_int32_t* d, int size)
{
    u_int8_t *data=(u_int8_t*)d;
    int da=0;

    size*=4;
    do {
        int res;
        res=read(path, data+da, size-da);
        if (res>0) {
            da+=res;
        } else {
            if (res==0)
                errno=EPIPE;
            if ((errno!=EINTR) && (errno!=EAGAIN)) {
                printf("receive: %s\n", strerror(errno));
                return -1;
            }
        }
    }
    while (da<size);
    return 0;
}
/******************************************************************************/
static void
decode_et200s(u_int32_t *data)
{
    printf("%2d et200s   %02x %d %08x\n",
            data[0], data[3], data[4], data[5]);
}
/******************************************************************************/
static void
decode_channel(u_int32_t *data)
{
    union u {
        u_int32_t u;
        float f;
    } u;

    u.u=data[1];
    switch (data[0]) {
    case 0:
        printf(" %8g", u.f);
        break;
    case 1:
        printf(" %8s", "under");
        break;
    case 2:
        printf(" %8s", "over");
        break;
    case 3:
        printf(" %8s", "error");
        break;
    case 4:
        printf(" %8s", "off");
        break;
    case 5:
        printf(" %8s", "nix");
        break;
    default:
        printf(" %8s", "unknown");
    };
}
/******************************************************************************/
static void
decode_tpg300(u_int32_t *data)
{
    int i;

    printf("%2d tpg300 %04x   ", data[0], data[3]);
    data+=4;
    for (i=0; i<4; i++) {
        decode_channel(data);
        data+=2;
    }
    printf("\n");
}
/******************************************************************************/
static void
decode_slave(u_int32_t *data)
{
    int type=data[2];

    if (type==1)
        decode_tpg300(data);
    else if (type==2)
        decode_et200s(data);
    else
        printf("%2d: unknown slave type %d\n", data[0], type);
}
/******************************************************************************/
static int
do_block(void)
{
    u_int32_t head[5], *data, *d;
    int len, n_slaves, i;

    if (xreceive(p, head, 5)<0)
        return -1;

    swap_falls_noetig(head, 5);

    printf("head: %d %d %u %u %d\n",
            head[0], head[1], head[2], head[3], head[4]);

    len=head[0]-4;
    data=malloc(len*4);
    if (!data) {
        printf("alloc %d words: %s\n", len, strerror(errno));
        return -1;
    }
    if (xreceive(p, data, len)<0)
        return -1;

    swap_falls_noetig(data, len);

#if 0
    for (i=0; i<5; i++)
        printf("%08x ", head[i]);
    printf("\n");

    for (i=0; i<len; i++)
        printf("%08x ", data[i]);
    printf("\n");
#endif

    n_slaves=head[4];
    d=data;
    for (i=0; i<n_slaves; i++){
        int l=d[1];
        decode_slave(d);
        d+=l+2;
    }

    free(data);

    return 0;
}
/******************************************************************************/
int
main(int argc, char *argv[])
{
    int res;

    if ((res=readargs(argc, argv)))
        return res<0?1:0;

    while (!do_block());

    return 0;
}
/******************************************************************************/
/******************************************************************************/
