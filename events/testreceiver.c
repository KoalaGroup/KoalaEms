/*
 * .c
 * 
 * $ZEL$
 * 
 * created: 2010-Mar-21 PW
 */

#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <netinet/in.h>

struct datasock {
    int p;
    unsigned long bytes;
    unsigned long reads;
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];
};

struct sockets {
    int *ssock;  /* server sockets */
    struct datasock *dsock;  /* data sockets */
    int snum; /* number of server sockets */
    int dnum; /* number of data sockets */
};

static int verbose;
static int v4, v6;
static char *filename;
static char *portname;
static char *defportname="8888";

static struct sockets sockets={0, 0, 0, 0};

#define BUFSIZE (1024*1024)
u_int32_t buffer[BUFSIZE];

#define swap_int(a) ( ((a) << 24) | \
                      (((a) << 8) & 0x00ff0000) | \
                      (((a) >> 8) & 0x0000ff00) | \
        ((unsigned int)(a) >>24) )

/*****************************************************************************/
static void
usage(char* argv0)
{
    printf("usage: %s [-h] [-v] [-f file] [-4|-6] -p port)\n", argv0);
    printf("       -v: verbose\n");
    printf("       -f: filename\n");
    printf("       -p: port (default=%s)\n", defportname);
    printf("       -4: use IP v4 only\n");
    printf("       -6: use IP v6 only\n");
    printf("       -h: this text\n");
}
/*****************************************************************************/
static int
readargs(int argc, char* argv[])
{
    int c, errflg=0;

    verbose=0;
    v4=1;
    v6=1;
    portname=defportname;
    filename=0;
    while (!errflg && ((c=getopt(argc, argv, "hv46f:p:"))!=-1)) {
        switch (c) {
        case 'v':
            verbose++;
            break;
        case '4':
            v4=1; v6=0;
            break;
        case '6':
            v4=0; v6=1;
            break;
        case 'f':
            filename=optarg;
            break;
        case 'p':
            portname=optarg;
            break;
        case 'h':
        default:
            errflg=1;
        }
    }

    if (errflg) {
        usage(argv[0]);
        return -1;
    }

    return 0;
}
/*****************************************************************************/
static int
do_read(struct sockets *sockets, int idx)
{
    struct datasock *dsock=sockets->dsock+idx;
    ssize_t len, res;

    len=BUFSIZE*4;
    res=read(dsock->p, buffer, len);
    if (res<=0) {
        if (res==0) {
            fprintf(stderr, "no more data from %s:%s\n",
                dsock->host, dsock->service);
        } else {
            fprintf(stderr, "error reading %s:%s: %s\n",
                dsock->host, dsock->service, strerror(errno));
        }
        close(dsock->p);
        dsock->p=-1;
        return res;
    }

    dsock->bytes+=res;
    dsock->reads++;

    return 0;
}
/*****************************************************************************/
static int
do_accept(struct sockets *sockets, int idx)
{
    struct sockaddr_storage addr;
    socklen_t addrlen=sizeof(addr);
    struct datasock *dsock;
    int err;

    dsock=realloc(sockets->dsock,
        sizeof(struct datasock)*(sockets->dnum+1));
    if (!dsock) {
        fprintf(stderr, "do_accept: realloc: %s\n", strerror(errno));
        return -1;
    }
    sockets->dsock=dsock;
    dsock+=sockets->dnum;

    dsock->p=-1;
    dsock->bytes=0;
    dsock->reads=0;

    dsock->p=accept(sockets->ssock[idx], (struct sockaddr*)&addr, &addrlen);
    if (dsock->p<0) {
        fprintf(stderr, "accept: %s\n", strerror(errno));
        return -1;
    }
    if ((err=getnameinfo((struct sockaddr*)&addr, addrlen,
            dsock->host, NI_MAXHOST,
            dsock->service, NI_MAXSERV, NI_NUMERICSERV))) {
        fprintf(stderr, "accept: getnameinfo: %s", gai_strerror(err));
    } else {
        fprintf(stderr, "connection from %s:%s accepted\n",
            dsock->host, dsock->service);
    }

    if (fcntl(dsock->p, F_SETFD, FD_CLOEXEC)<0) {
        fprintf(stderr, "do_autosock_accept: fcntl(FD_CLOEXEC): %s\n",
            strerror(errno));
    }
    if (fcntl(dsock->p, F_SETFL, O_NDELAY)<0) {
        fprintf(stderr, "do_autosock_accept: fcntl(O_NDELAY): %s\n",
            strerror(errno));
    }

    sockets->dnum++;

    return 0;
}
/*****************************************************************************/
static int
do_select(struct sockets *sockets)
{
    fd_set readfds;
    int res, i, nfds=-1;

    FD_ZERO(&readfds);
    for (i=0; i<sockets->snum; i++) {
        FD_SET(sockets->ssock[i], &readfds);
        if (sockets->ssock[i]>nfds)
            nfds=sockets->ssock[i];
    }
    for (i=0; i<sockets->dnum; i++) {
        FD_SET(sockets->dsock[i].p, &readfds);
        if (sockets->dsock[i].p>nfds)
            nfds=sockets->dsock[i].p;
    }
    if (nfds<0) {
        fprintf(stderr, "last socket closed\n");
        return -1;
    }

    res=select(nfds+1, &readfds, 0, 0, 0);
    if (res<0) {
        int error=errno;
        fprintf(stderr, "select: %s\n", strerror(errno));
        if (error==EINTR || error==EAGAIN)
            return 0;
        else
            return -1;
    }

    for (i=0; i<sockets->snum; i++) {
        if (FD_ISSET(sockets->ssock[i], &readfds))
            do_accept(sockets, i);
    }

    for (i=0; i<sockets->dnum; i++) {
        if (FD_ISSET(sockets->dsock[i].p, &readfds))
            do_read(sockets, i);
    }

    return 0;
}
/*****************************************************************************/
static int
do_loop(struct sockets *sockets)
{
    int res;
    do {
        int i;

        res=do_select(sockets);

        /* remove closed sockets */
        for (i=sockets->snum-1; i>=0; i--) {
            if (sockets->ssock[i]<0) {
                memcpy(sockets->ssock+i,
                       sockets->ssock+i+1,
                       (sockets->snum-1-i)*sizeof(int));
                sockets->snum--;
            }
        }
        for (i=sockets->dnum-1; i>=0; i--) {
            if (sockets->dsock[i].p<0) {
                memcpy(sockets->dsock+i,
                       sockets->dsock+i+1,
                       (sockets->dnum-1-i)*sizeof(struct datasock));
                sockets->dnum--;
            }
        }
    } while (!res);
    return res;
}
/*****************************************************************************/
static int
create_listensocket(struct addrinfo *rp)
{
    char host[NI_MAXHOST+1], service[NI_MAXSERV+1];
    int s, err;

    err=getnameinfo(rp->ai_addr, rp->ai_addrlen,
                           host, NI_MAXHOST,
                           service, NI_MAXSERV, NI_NUMERICSERV);

    s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (s<0) {
        fprintf(stderr, "socket: %s\n", strerror(errno));
        return -1;
    }

    if (bind(s, rp->ai_addr, rp->ai_addrlen)<0) {
#if 1
        fprintf(stderr, "bind to %s:%s unsuccessfull: %s\n",
            host, service, strerror(errno));
#endif
        close(s);
        return -1;
    }
    if (listen(s, 8)<0) {
        fprintf(stderr, "listen on %s:%s unsuccessfull: %s\n",
            host, service, strerror(errno));
        close(s);
        return -1;
    }

    fprintf(stderr, "cluster_autosock: bound to %s@%s\n", service, host);
    fprintf(stderr, "cluster_autosock: bound to %s:%s\n", host, service);

    return s;
}
/*****************************************************************************/
static int
create_anysockets(int **p, int *n, const char *port, int ipvX)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s, err;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags  = AI_PASSIVE; /* For wildcard IP address */
    hints.ai_flags |= AI_ADDRCONFIG;
    switch (ipvX) {
    case 4:
        hints.ai_family = AF_INET;
        break;
    case 6:
        hints.ai_family = AF_INET6;
        break;
    default:
        //hints.ai_family = AF_INET6 /*AF_UNSPEC does not work correctly*/;
        hints.ai_family = AF_UNSPEC /*AF_UNSPEC does not work correctly*/;
    }
    hints.ai_socktype = SOCK_STREAM;

    err = getaddrinfo(0, port, &hints, &result);
    if (err) {
        fprintf(stderr, "cluster_autosock: getaddrinfo: %s\n",
                gai_strerror(err));
        return -1;
    }

    for (rp = result; rp; rp = rp->ai_next) {
        if (rp->ai_family==AF_INET6) {
            s=create_listensocket(rp);
            if (s>=0) {
                *p=realloc(*p, (*n+1)*sizeof(int));
                (*p)[*n]=s;
                (*n)++;
            }
        }
    }

    for (rp = result; rp; rp = rp->ai_next) {
        if (rp->ai_family!=AF_INET6) {
            s=create_listensocket(rp);
            if (s>=0) {
                *p=realloc(*p, (*n+1)*sizeof(int));
                (*p)[*n]=s;
                (*n)++;
            }
        }
    }

    freeaddrinfo(result);/* No longer needed */

    return 0;
}
/*****************************************************************************/
static void
close_sockets(struct sockets *sockets)
{
    int i;
    for (i=0; i<sockets->snum; i++)
        close(sockets->ssock[i]);
}
/*****************************************************************************/
int
main(int argc, char *argv[])
{
    if (readargs(argc, argv)<0)
        return 1;

    if (!v6) {
        if (create_anysockets(&sockets.ssock, &sockets.snum, portname, 4)<0) {
            printf("unable to create listening sockets for IPv4\n");
        }
    } else if (!v4) {
        if (create_anysockets(&sockets.ssock, &sockets.snum, portname, 6)<0) {
            printf("unable to create listening sockets for IPv6\n");
        }
    } else {
        if (create_anysockets(&sockets.ssock, &sockets.snum, portname, 0)<0) {
            printf("unable to create listening sockets\n");
        }
    }
    printf("%d listening socket%s created\n",
            sockets.snum, sockets.snum==1?"":"s");
    if (!sockets.snum) {
        return 2;
    }

    do_loop(&sockets);

    close_sockets(&sockets);

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
