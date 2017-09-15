/*
 * async_send.c
 * 
 * created 2011-04-26 PW
 * $ZEL: async_send.c,v 1.1 2011/08/02 18:23:16 wuestner Exp $
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

static int quiet=0;
static int no_v6=0;
static int no_v4=0;
static const char *outname=0;

static int p;

/******************************************************************************/
static void
printusage(char* argv0)
{
    printf("usage: %s [-h] [-q] [-4] destination\n", argv0);
    printf("  -h: this help\n");
    printf("  -q: quiet (no informational output)\n");
    printf("  -4: use IPv4 only\n");
    printf("  -6: use IPv6 only\n");
    printf("  -4 and -6 are mutually exclusive\n");
    printf("  destination: host:port or [host]:port\n");
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

    outname=argv[optind];

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
        printf("name=>%s<\n", name);
        printf("name=%p pp=%p\n", name, pp);
        printf("pp-name=%lld\n", (long long)(pp-name));
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
/******************************************************************************/
static int
do_line(void)
{
    char line[1029];
    int len, size;

    if (!fgets(line+4, 1024, stdin))
        return -1;
    len=strlen(line+4);
    size=((len+3)/4)*4+4;
    line[0]=(len>>24)&0xff;
    line[1]=(len>>16)&0xff;
    line[2]=(len>>8)&0xff;
    line[3]=len&0xff;

    if (write(p, line, size)!=size) {
        printf("send: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}
/******************************************************************************/
int
main(int argc, char *argv[])
{
    int res;

    if ((res=readargs(argc, argv)))
        return res<0?1:0;

    if (connect_socket(outname)<0)
        return 2;

    while (!do_line());

    return 0;
}
/******************************************************************************/
/******************************************************************************/
