/*
 * normsock.c
 * 
 * created: 2011-08-23 PW
 * $ZEL: normsock.c,v 1.3 2013/10/30 18:21:28 wuestner Exp $
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "normsock.h"

#define MAXSIZE 0x3fffff /* arbitrary limit :-| */

static FILE *log=0;
static int loglevel=0;
static int no_v4=0;
static int no_v6=0;

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
void
normsock_set_loginfo(FILE *logfile, int level)
{
    log=logfile;
    loglevel=level;
}
/******************************************************************************/
void
normsock_set_ip_info(int no_v4_, int no_v6_)
{
    no_v4=no_v4_;
    no_v6=no_v6_;
}
/******************************************************************************/
static void
free_data_descr(struct data_descr *descr)
{
    if (!--descr->refcount) {
        free(descr->opaque);
        free(descr);
    }
}
/******************************************************************************/
void
close_socket(struct sockdescr *sock)
{
    close(sock->p);
    sock->p=-1;
    free(sock->host);
    free(sock->service);
    free_data_descr(sock->ddescr);
    sock->ddescr=0;
    while (sock->queue) {
        struct data_ent *ent=sock->queue;
        sock->queue=ent->next;
        free_data_descr(ent->data);
        free(ent);
    }
}
/******************************************************************************/
int
accept_socket(struct sockdescr *sock, struct sockdescr **list)
{
    char host[NI_MAXHOST+1];
    char service[NI_MAXSERV];
    struct sockaddr_storage addr;
    struct sockdescr *newdescr, *next;
    socklen_t addr_len;
    int np, res;

    memset(&addr, 0, sizeof(struct sockaddr_storage));
    addr_len=sizeof(struct sockaddr_storage);
    np=accept(sock->p, (struct sockaddr*)&addr, &addr_len);
    if (np<0) {
        eLOG("accept: %s\n", strerror(errno));
        return -1;
    }

    if (fcntl(np, F_SETFL, O_NDELAY)<0) {
        eLOG("accept:fcntl(O_NDELAY): %s\n", strerror(errno));
        close(np);
        return -1;
    }

    /* find an old free entry */
    newdescr=*list;
    while (newdescr) {
        if (newdescr->p<0)
            break;
        newdescr=newdescr->next;
    }
    /* alloc new entry if necessary*/
    if (!newdescr) {
        newdescr=malloc(sizeof(struct sockdescr));
        if (!newdescr)   {
            eLOG("accept:alloc descriptor: %s\n", strerror(errno));
            close(np);
            return -1;
        }
        newdescr->next=*list;
        *list=newdescr;
    }

    /* fill out the new or recycled entry */
    next=newdescr->next;
    memset(newdescr, 0, sizeof(struct sockdescr));
    newdescr->next=next;
    newdescr->p=np;
    newdescr->maxqueue=sock->maxqueue;
    newdescr->inout=sock->inout;
    newdescr->objsize=sock->objsize;
    res=getnameinfo((struct sockaddr*)&addr, addr_len,
            host, NI_MAXHOST,
            service, NI_MAXSERV, NI_NUMERICSERV);
    if (res) {
        eLOG("accept:getnameinfo: %s\n", gai_strerror(res));
        strcpy(newdescr->host, "unknown");
        strcpy(newdescr->service, "unknown");
    } else {
        newdescr->host=strdup(host);
        newdescr->service=strdup(service);
        LOG(0, "accepted from %s:%s\n", newdescr->host, newdescr->service);
    }

    return 0;
}
/******************************************************************************/
/*
 * connect_socket opens a socket and connects it.
 * It returns
 *   0: if the connection is established
 *  -1: in any other case
 */
static int
connect_socket(const char *host, const char *service,
        struct sockdescr **list, int objsize, int maxqueue, int inout)
{
    struct sockdescr *newdescr, *next;
    struct addrinfo *addr, *a;
    struct addrinfo hints;
    int res;

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
        LOG(1, "request addrinfo for \"[%s]:%s\": %s\n",
            host, service, gai_strerror(res));
        goto error;
    }

    res=-1;
    for (a=addr; a; a=a->ai_next) {
        int sock;
        if ((sock=socket(a->ai_family, a->ai_socktype, a->ai_protocol))<0) {
            LOG(1, "create socket: %s\n", strerror(errno));
            continue;
        }

        if (connect(sock, a->ai_addr, a->ai_addrlen)<0) {
            LOG(1, "connect to [%s]:%s: %s\n",
                    host, service, strerror(errno));
            close(sock);
            continue;
        }

        if (fcntl(sock, F_SETFL, O_NDELAY)<0) {
            eLOG("connect_socket:fcntl(O_NDELAY): %s\n", strerror(errno));
            close(sock);
            continue;
        }

        /* find an old free entry */
        newdescr=*list;
        while (newdescr) {
            if (newdescr->p<0)
                break;
            newdescr=newdescr->next;
        }
        /* alloc new entry if necessary*/
        if (!newdescr) {
            newdescr=malloc(sizeof(struct sockdescr));
            if (!newdescr)   {
                eLOG("connect_socket:alloc descriptor: %s\n", strerror(errno));
                return -1;
            }
            newdescr->next=*list;
            *list=newdescr;
        }

        next=newdescr->next;
        memset(newdescr, 0, sizeof(struct sockdescr));
        newdescr->next=next;
        newdescr->host=strdup(host);
        newdescr->service=strdup(service);
        newdescr->p=sock;
        newdescr->inout=inout;
        newdescr->objsize=objsize;
        newdescr->maxqueue=maxqueue;
        LOG(0, "successfully connected to [%s]:%s\n", host, service);
        res=0;
    }
    if (res<0)
        eLOG("connection to [%s]:%s failed\n", host, service);

error:
    freeaddrinfo(addr);
    return res;
}
/******************************************************************************/
/*
 * open_listening_socket creates one or more sockets, bind them to
 * service and brings them into listening state.
 * socklist and num have either to be initialised with zero or have
 * to be results of a previous call to *_socket.
 */
static int
open_listening_socket(const char *service,
        struct sockdescr **list, int objsize, int maxqueue, int inout)
{
    struct sockdescr *newdescr, *next;
    struct addrinfo *addr, *a;
    struct addrinfo hints;
    int n=0, res;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family=no_v6?AF_INET:AF_INET6;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_protocol=0;
    hints.ai_flags=AI_PASSIVE;
    if ((res=getaddrinfo(0, service, &hints, &addr))) {
        LOG(1, "request addrinfo for \"%s\": %s\n",
            service, gai_strerror(res));
        goto error;
    }

    for (a=addr; a; a=a->ai_next) {
        int sock;
        if ((sock=socket(a->ai_family, a->ai_socktype, a->ai_protocol))<0) {
            LOG(1, "create socket: %s\n", strerror(errno));
            continue;
        }

        if (bind(sock, a->ai_addr, a->ai_addrlen)<0) {
            LOG(1, "bind socket to port %s: %s\n", service, strerror(errno));
            close(sock);
            continue;
        }

        if (listen(sock, 32)<0) {
            LOG(1, "listen on port %s: %s\n", service, strerror(errno));
            close(sock);
            continue;
        }

        /* find an old free entry */
        newdescr=*list;
        while (newdescr) {
            if (newdescr->p<0)
                break;
            newdescr=newdescr->next;
        }
        /* alloc new entry if necessary*/
        if (!newdescr) {
            newdescr=malloc(sizeof(struct sockdescr));
            if (!newdescr)   {
                eLOG("open_listening_socket:alloc descriptor: %s\n",
                        strerror(errno));
                close(sock);
                return -1;
            }
            newdescr->next=*list;
            *list=newdescr;
        }

        next=newdescr->next;
        memset(newdescr, 0, sizeof(struct sockdescr));
        newdescr->next=next;
        newdescr->listening=1;
        newdescr->service=strdup(service);
        newdescr->p=sock;
        newdescr->maxqueue=maxqueue;
        newdescr->inout=inout;
        newdescr->objsize=objsize;
        n++;
        LOG(0, "port %s successfully opened\n", service);
    }

    freeaddrinfo(addr);

error:
    if (!n) {
        eLOG("could not open any listening socket for port %s\n", service);
        return -1;
    } else {
        return 0;
    }
}
/******************************************************************************/
/*
 * parse_name splits name into a host and a service part.
 * *host and *service are allocated using strdup and should be freed by
 * the caller.
 * In case of error (return -1) *host and *service are undefined;
 * If only a service is given *host is zero.
 * The format for name is:
 * [host]:service (host and service for numerical IPv6 addresses)
 * host:service   (host and service)
 * :service       (service only)
 * service        (service only)
 */
static int
parse_name(const char *name, char **host, char **service)
{
    const char *pp;

    *host=0;
    *service=0;

    if (name[0]=='[') {
        pp=strstr(name, "]:");
        if (!pp) {
            eLOG("cannot parse %s\n", name);
            return -1;
        }
        if (pp-name-2>=NI_MAXHOST) {
            eLOG("host part of %s too long\n", name);
            return -1;
        }
        *host=strndup(name+1, pp-name-1);
        if (!*host) {
            eLOG("allocate space for hostname: %s\n", strerror(errno));
            return -1;
        }
        if (strlen(pp+2)>=NI_MAXSERV) {
            eLOG("service part of %s too long\n", name);
            free(*host);
            return -1;
        }
        *service=strdup(pp+2);
        if (!*service) {
            eLOG("allocate space for service: %s\n", strerror(errno));
            free(*host);
            return -1;
        }
    } else {
        pp=strrchr(name, ':');
        if (pp && pp==name) {
            pp++;
        } else if (pp) {
            if (pp-name>=NI_MAXHOST) {
                eLOG("host part of %s too long\n", name);
                return -1;
            }
            *host=strndup(name, pp-name);
            if (!*host) {
                eLOG("allocate space for hostname: %s\n", strerror(errno));
                return -1;
            }
            pp++;
        } else {
            pp=name;
        }
        if (strlen(pp)>=NI_MAXSERV) {
            eLOG("service part of %s too long\n", name);
            free(*host);
            return -1;
        }
        *service=strdup(pp);
        if (!*service) {
            eLOG("allocate space for service: %s\n", strerror(errno));
            free(*host);
            return -1;
        }
    }

    dLOG("host   : >%s<\n", *host);
    dLOG("service: >%s<\n", *service);
    return 0;
}
/******************************************************************************/
/*
 * open_socket creates a socket from name.
 * It will be a listening socket if only the service name is given.
 * It will be connected if a host name is given.
 * *list has either to be initialised with zero or have
 * to be a result of a previous call to open_socket.
 */
int
open_socket(struct sockname *sname, struct sockdescr **list, int objsize)
{
    char *host, *service;
    int res;

    if (parse_name(sname->name, &host, &service)<0)
        return -1;

    if (host)
        res=connect_socket(host, service, list, objsize,
                sname->maxqueue, sname->inout);
    else
        res=open_listening_socket(service, list, objsize,
                sname->maxqueue, sname->inout);

    free(host);
    free(service);
        
    return res;
}
/******************************************************************************/
static void
dump_ddescr(struct data_descr *ddescr, int objsize)
{
    u_int32_t *data;
    int num, i;

    num=(ddescr->size-4)/objsize;
    data=(u_int32_t*)ddescr->opaque;
    printf("dump: %d objects of size %d (size=%d)\n",
            num, objsize, ddescr->size);
    printf("CCC %08x %10d\n", ntohl(data[0]), ntohl(data[0]));
    data++;
    if (objsize!=4)
        return;
    for (i=0; i<num; i++)
        printf("%3d %08x %10d\n", i, ntohl(data[i]), ntohl(data[i]));

}
/******************************************************************************/
static struct data_descr*
alloc_data(void)
{
    struct data_descr *data;

    data=calloc(1, sizeof(struct data_descr));
    if (!data) {
        eLOG("alloc data block: %s\n", strerror(errno));
        return 0;
    }
    data->opaque=malloc(4);
    if (!data->opaque) {
        eLOG("alloc data block: %s\n", strerror(errno));
        free(data);
        return 0;
    }
    data->space=4;
    return data;
}
/******************************************************************************/
/*
 * sock_read reads some data (nonblocking) from the socket.
 * It returns
 *   -2 in case of fatal error
 *   -1 if the socket was closed "at the right point"
 *    0 if no error occured and no data are available
 *    1 if a block of data is complete
 * If the result is 1 (data complete) it is necessary to extract the data
 * from the sockdescr before the next call to sock_read!
 */
int
sock_read(struct sockdescr *sock)
{
    struct data_descr *ddescr;
    ssize_t res;

    /* if necessary: alloc initial data block */
    if (!sock->ddescr) {
        sock->idx=0;
        sock->ddescr=alloc_data();
        if (!sock->ddescr)
            return -2;
    }

    ddescr=sock->ddescr;

    /* if necessary: read size */
    if (!ddescr->size) { /* 4 bytes for size are preallocated */
        dLOG("will read %d bytes for size\n", 4-sock->idx);
        res=read(sock->p, ddescr->opaque+sock->idx, 4-sock->idx);
        if (res<=0) {
            if (res==0) {
                /* socket closed (or broken)
                   at this point this is not an error */
                return -1;
            }
            if (errno==EINTR || errno==EAGAIN)
                return 0;
            if (loglevel<-1 || errno!=EPIPE)
                LOG_("read head from %s:%s: %s\n",
                        sock->host, sock->service, strerror(errno));
            return -2;
        }
        dLOG("got %lld bytes\n", (long long)res);
        sock->idx+=res;
        if (sock->idx==4) {
            ddescr->size=
                (ddescr->opaque[0]<<24) |
                (ddescr->opaque[1]<<16) |
                (ddescr->opaque[2]<<8) |
                 ddescr->opaque[3];
            dLOG("opaque size is %d\n", ddescr->size);
            if (ddescr->size & ~MAXSIZE) {
                eLOG("size %d is too large\n", ddescr->size);
                return -2;
            }
            if (!ddescr->size) { /* empty block ignored */
                sock->idx=0;
                return 0;
            }
            /* we need the size in bytes (plus the counter itself) */
            ddescr->size*=sock->objsize;
            /* round to next multiple of 4 */
            ddescr->size=((ddescr->size+3)/4)*4;
            /* add the size of the counter */
            ddescr->size+=4;
            dLOG("have to read %d bytes\n", ddescr->size-4);
        } else {
            return 0;
        }
    }

    /* if necessary: alloc space for data */
    if (ddescr->space<ddescr->size) {
        u_int8_t *opaque;
        dLOG("realloc %d bytes\n", ddescr->size);
        opaque=realloc(ddescr->opaque, ddescr->size);
        dLOG("realloc data: %p --> %p\n", ddescr->opaque, opaque);
        if (!opaque) {
            eLOG("alloc %d bytes for %s:%s: %s\n",
                    ddescr->size,
                    sock->host, sock->service,
                    strerror(errno));
            return -2;
        }
        ddescr->opaque=opaque;
        ddescr->space=ddescr->size;
    }

    /* read data */
    dLOG("will read %d bytes\n", ddescr->size-sock->idx);
    res=read(sock->p, ddescr->opaque+sock->idx, ddescr->size-sock->idx);
    if (res<=0) {
        if (res==0) {
            /* length word valid, but no data
               socket close (or broken) at the wrong point, fatal error */
            errno=EPIPE;
        }
        if (errno==EINTR || errno==EAGAIN)
            return 0;
        if (loglevel<-1 || errno!=EPIPE)
            LOG_("read data from %s:%s: %s\n",
                    sock->host, sock->service, strerror(errno));
        return -2;
    }

    sock->idx+=res;
    if (sock->idx==ddescr->size) { /* data complete */
        dLOG("received from %s:%s: %d bytes\n",
            sock->host, sock->service, ddescr->size);
        return 1;
    } else {
        return 0;
    }
}
/******************************************************************************/
/*
 * sock_write writes some data (nonblocking) to the socket.
 * It returns
 *   -1 in case of fatal error
 *    0 if no fatal error occured
 *    1 if a block of data has completely been sent
 */
int
sock_write(struct sockdescr *sock)
{
    int res;

    if (!sock->ddescr) {
        struct data_ent *ent=sock->queue;
        if (!ent) {
            dLOG("sock_write: no data\n");
            return 0;
        }
        sock->ddescr=ent->data;
        sock->queue=ent->next;
        free(ent);
        sock->idx=0; /* no bytes written yet */
    }

    dump_ddescr(sock->ddescr, sock->objsize);

    res=write(sock->p, sock->ddescr->opaque+sock->idx,
            sock->ddescr->size-sock->idx);
    if (res<=0) {
        if (errno==EINTR || errno==EAGAIN) {
            return 0;
        } else {
            if (loglevel<0 || errno!=EPIPE)
                LOG_("write to %s:%s: %s\n", sock->host, sock->service,
                        strerror(errno));
            return -1;
        }
    }
    sock->idx+=res;
    sock->stalled=0;

    if (sock->idx==sock->ddescr->size) { /* all data sent */
        free_data_descr(sock->ddescr);
        sock->ddescr=0;
        return 1;
    }
    return 0;
}
/******************************************************************************/
/*
 * append a new data_descr to the end of list
 * if the list is now longer than maxqueue remove the first (oldest) entry
 */
int
sock_store(struct sockdescr *sock, struct data_descr *data)
{
    struct data_ent *ent;
    int n=0;

    ent=calloc(1, sizeof(struct data_ent));
    if (!ent) {
        eLOG("alloc data_ent: %s\n", strerror(errno));
        return -1;
    }
    ent->data=data;
    data->refcount++;

    if (!sock->queue) { /* list is empty */
        sock->queue=ent;
    } else { /* list is not empty */
        struct data_ent *last_ent;
        /* find last entry and count the entries*/
        last_ent=sock->queue; /* first entry */
        while (last_ent->next) {
            last_ent=last_ent->next;
            n++;
        }
        last_ent->next=ent;
    }
    if (sock->maxqueue>0 && n>=sock->maxqueue) { /* remove oldest entry */
        if (!sock->stalled) {
            vLOG("output queue to %s:%s stalled\n", sock->host, sock->service);
            sock->stalled=1;
        } 
        ent=sock->queue;
        sock->queue=ent->next;
        free_data_descr(ent->data);
        free(ent);
    }
    return 0;
}
/******************************************************************************/
/******************************************************************************/
