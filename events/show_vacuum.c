/*
 * show_vacuum.c
 * 
 * created 2014-07-04 PW
 * $ZEL: show_vacuum.c,v 1.1 2014/07/04 15:42:59 wuestner Exp $
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include "swap.h"

static int quiet=0;
static int no_v6=0;
static int no_v4=0;
static const char *inname=0;

static int p;

/******************************************************************************/
static void
printusage(char* argv0)
{
    printf("usage: %s [-h] [-q] [-4|-6] source\n", argv0);
    printf("  -h: this help\n");
    printf("  -q: quiet (no informational output)\n");
    printf("  -4: use IPv4 only\n");
    printf("  -6: use IPv6 only\n");
    printf("  -4 and -6 are mutually exclusive\n");
    printf("  source: host:port or [host]:port\n");
}
/******************************************************************************/
static int
readargs(int argc, char* argv[])
{
    int c, err;
    err=0;

    while (!err && ((c=getopt(argc, argv, "hq46")) != -1)) {
        switch (c) {
        case 'h': printusage(argv[0]); return 1;
        case 'q': quiet=1; break;
        case '4': no_v6=1; break;
        case '6': no_v4=1; break;
        default: err=1;
        }
    }
    err=err||(no_v6&&no_v4);
    if (err || argc-optind!=1) {
        printusage(argv[0]);
        return -1;
    }

    inname=argv[optind];

    return 0;
}
/******************************************************************************/
static int
connect_socket(const char *name)
{
    char host[NI_MAXHOST], service[NI_MAXSERV], *pp;
    struct addrinfo *addr, *a;
    struct addrinfo hints;
    int res;

    /* separate host and port */
    if (name[0]=='[') {
        pp=strstr(name, "]:");
        if (!pp) {
            printf("cannot parse %s\n", name);
            return -1;
        }
        if (pp-name>NI_MAXHOST-1) {
            printf("host part of %s too long\n", name);
            return -1;
        }
        strncpy(host, name, pp-name);
        host[NI_MAXHOST-1]='\0';
        strncpy(service, pp+2, NI_MAXSERV);
        service[NI_MAXSERV-1]='\0';
    } else {
        pp=strrchr(name, ':');
#if 0
        printf("name=>%s<\n", name);
        printf("name=%p pp=%p\n", name, pp);
        printf("pp-name=%lld\n", (long long)(pp-name));
#endif
        if (!pp) {
            printf("cannot parse %s\n", name);
            return -1;
        }
        if (pp-name>NI_MAXHOST-1) {
            printf("host part of %s too long\n", name);
            return -1;
        }
        strncpy(host, name, pp-name);
        host[pp-name]='\0';
        strncpy(service, pp+1, NI_MAXSERV);
        service[NI_MAXSERV-1]='\0';
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    if (no_v4)
        hints.ai_family=AF_INET6;
    else if (no_v6)
        hints.ai_family=AF_INET;
    else
         hints.ai_family=AF_UNSPEC;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_protocol=0;
    hints.ai_flags=0;
    if ((res=getaddrinfo(host, service, &hints, &addr))) {
        printf("request addrinfo for \"%s:%s\": %s\n",
            host, service, gai_strerror(res));
        return -1;
    }

    for (a=addr; a; a=a->ai_next) {
        if ((p=socket(a->ai_family, a->ai_socktype, a->ai_protocol))<0) {
            printf("create socket: %s\n", strerror(errno));
            continue;
        }

        if (connect(p, a->ai_addr, a->ai_addrlen)<0) {
            printf("connect to port %s:%s: %s\n",
                    host, service, strerror(errno));
            close(p);
            p=-1;
            continue;
        }
        printf("successfully connected to %s:%s\n", host, service);
        break;
    }

    freeaddrinfo(addr);

    return p;
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

    if (connect_socket(inname)<0)
        return 2;

    while (!do_block());

    return 0;
}
/******************************************************************************/
/******************************************************************************/
