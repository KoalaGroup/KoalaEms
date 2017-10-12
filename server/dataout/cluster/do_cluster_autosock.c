/*
 * dataout/cluster/do_cluster_autosock.c
 * created      2010-Mar-02 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL$";

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sconf.h>
#include <debug.h>
#include <emssocktypes.h>
#include <unsoltypes.h>
#include "do_cluster.h"
#include "../../main/scheduler.h"
#include "../../objects/domain/dom_dataout.h"
#include "../../objects/pi/readout.h"
#include "../../commu/commu.h"

struct serversock {
    struct serversock *next;
    struct do_special_autosock *parent;
    struct seltaskdescr* task;
    int p;
};

struct datasock {
    struct datasock *next;
    struct do_special_autosock *parent;
    struct seltaskdescr* task;
    int p;
};

struct do_special_autosock {
    int do_idx;
    struct serversock *ssock;  /* server sockets */
    struct datasock   *dsock;  /* data sockets */
    int *p; /* just the server socket(s), copied to ssock */
    int n;  /* number of just the server socket(s) */
};

extern ems_u32* outptr;

/*****************************************************************************/
static int
create_anysockets(int **p, int *n, const char *port, int ipvX)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int err;

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
        hints.ai_family = AF_UNSPEC;
    }
    hints.ai_socktype = SOCK_STREAM;

    err = getaddrinfo(0, port, &hints, &result);
    if (err) {
        printf("cluster_autosock: getaddrinfo: %s\n", gai_strerror(err));
        return -1;
    }

    for (rp = result; rp; rp = rp->ai_next) {
        char host[NI_MAXHOST], service[NI_MAXSERV];
        int s;

#if 0
        printf("flags    =0x%x\n", rp->ai_flags);
        printf("family   =%d\n", rp->ai_family);
        printf("socktype =%d\n", rp->ai_socktype);
        printf("protocol =%d\n", rp->ai_protocol);
        printf("canonname=%s\n", rp->ai_canonname);
        printf("next     =%p\n", rp->ai_next);
#endif

        err=getnameinfo(rp->ai_addr, rp->ai_addrlen,
                               host, NI_MAXHOST,
                               service, NI_MAXSERV, NI_NUMERICSERV);

        s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (s<0)
            continue;

        if (fcntl(s, F_SETFD, FD_CLOEXEC)<0)
            printf("cluster_autosock: fcntl(CLOEXEC): %s\n", strerror(errno));

        if (bind(s, rp->ai_addr, rp->ai_addrlen)<0) {
#if 0
            printf("bind to %s%s unsuccessfull: %s\n",
                    host, service, strerror(errno));
#endif
            close(s);
            continue;
        }
        /* do we need O_NDELAY here? */
        if (listen(s, 8)<0) {
            printf("listen on %s%s unsuccessfull: %s\n",
                    host, service, strerror(errno));
            close(s);
            continue;
        }

        printf("cluster_autosock: bound to %s@%s\n", service, host);

        *p=realloc(*p, (*n+1)*sizeof(int));
        (*p)[*n]=s;
        (*n)++;
    }

    freeaddrinfo(result);           /* No longer needed */

    return 0;
}
/*****************************************************************************/
static void
do_cluster_autosock_cleanup(int do_idx)
{
    struct do_special_autosock* special=
            (struct do_special_autosock*)dataout_cl[do_idx].s;
    int i;

    T(dataout/cluster/do_cluster_autosock.c:do_cluster_autosock_cleanup)

    for (i=0; i<special->dnum; i++) {
        struct datasock *ds=special->dsock+i;
        if (ds->task)
            sched_delete_select_task(ds->task);
        if (ds->p>=0) {
            shutdown(ds->p, SHUT_RDWR);
            close(ds->p);
        }
    }
    free(special->dsock);

    for (i=0; i<special->snum; i++) {
        struct serversock *ss=special->ssock+i;
        if (ss->task)
            sched_delete_select_task(ss->task);
        if (ss->p>=0) {
            shutdown(ss->p, SHUT_RDWR);
            close(ss->p);
        }
    }
    free(special->ssock);

    free(special->p); /* they are closed in special->ssock already */

    free(special);
    dataout_cl[do_idx].s=0;
}
/*****************************************************************************/

static struct do_procs autosock_procs={
  /*do_cluster_autosock_start*/ 0,
  /*do_cluster_autosock_reset*/ 0,
  /*do_cluster_autosock_freeze*/ 0,
  do_cluster_autosock_advertise,
  /*do_cluster_autosock_patherror*/ 0,
  do_cluster_autosock_cleanup,
  /*wind*/ do_NotImp_err_ii,
  /*status*/ 0,
  /*filename*/ 0,
  };

/*****************************************************************************/
static void
do_autosock_accept(int path, enum select_types selected,
        union callbackdata data)
{
    struct serversock *ssock=(struct serversock*)data.p;
    struct datasock *dsock;
    struct do_special_autosock* special=ssock->parent;
    int do_idx=special->do_idx;
    union callbackdata data;
    int ns;
    struct sockaddr addr;
    socklen_t addrlen=sizeof(struct sockaddr);

    /* accept4 is linux specific,
       here it calls implicitely fcntl(O_NONBLOCK) and fcntl(FD_CLOEXEC)
    */
    ns=accept4(path, &addr, &addrlen, SOCK_NONBLOCK|SOCK_CLOEXEC);
    if (ns<0) {
        complain("autosock_accept: %s", strerror(errno));
        return;
    }

    dsock=malloc(sizeof(struct datasock));
    if (!dsock) {
        complain("autosock_accept: %s", strerror(errno));
        close(ns);
        return;
    }

    data.p=dsock;
    dsock->parent=special;
    dsock->p=ns;
    dsock->task=sched_insert_select_task(do_autosock_write, data,
            "do_autosock_write", dsock->p, select_write
#ifdef SELECT_STATIST
            , 1
#endif
        );
        if (dsock->task==0) {
            printf("autosock_accept: can't create write task.\n");
            close(ns);
            free(dsock);
            return;
        }

    dsock->next=special->dsock;
    special->dsock=dsock;


    prepare for delivery of ved_cluster

    prepare accepting of advertized clusters


    do something with do_status
}
/*****************************************************************************/
errcode
do_cluster_autosock_init(int do_idx)
{
    struct do_special_autosock* special;
    char port[8];
    errcode err=OK;
    int i;

    T(dataout/cluster/do_cluster_sock.c:do_cluster_sock_init)

    special=calloc(1, sizeof(struct do_special_autosock));
    if (!special) {
        complain("cluster_autosock_init: %s", strerror(errno));
        return Err_NoMem;
    }
    dataout_cl[do_idx].s=special;
    dataout_cl[do_idx].procs=autosock_procs;
    special->do_idx=do_idx;

    snprintf(port, 8, "%d", dataout[do_idx].addr.inetsock.port);

    if (create_anysockets(&special->p, &special->n, port, 6)<0) {
        printf("unable to create listening sockets for IPv6\n");
    }
    if (create_anysockets(&special->p, &special->n, port, 4)<0) {
        printf("unable to create listening sockets for IPv4\n");
    }
    if (create_anysockets(&special->p, &special->n, port, 0)<0) {
        printf("unable to create listening sockets\n");
    }

    printf("cluster_autosock_init: %d listening socket%s created\n",
            special->n, special->n==1?"":"s");

    /* create the tasks */
    special->ssock=calloc(special->snum, sizeof(struct serversock));
    if (!special->ssock) {
        complain("cluster_autosock_init: %s", strerror(errno));
        err=Err_NoMem;
        goto error;
    }
    special->snum=special->n;
    for (i=0; i<special->n; i++) {
        union callbackdata data;
        struct serversock *ssock=special->ssock+i;
        ssock->parent=special;
        ssock->p=special->p[i];
        data.p=ssock;

        ssock->task=sched_insert_select_task(do_autosock_accept, data,
            "do_autosock_accept", ssock->p, select_read
#ifdef SELECT_STATIST
            , 1
#endif
        );
        if (ssock->task==0) {
            printf("cluster_autosock_init: can't create listen task.\n");
            err=Err_System;
            goto error;
        }
    }

    return OK;

error:

    for (i=0; i<special->n; i++) {
        close(special->p[i]);
    }
    for (i=0; i<special->snum; i++) {
        struct serversock *ssock=special->ssock+i;
        if (ssock->task)
            sched_delete_select_task(ssock->task);
    }
    free(special->ssock);
    free(special);
    dataout_cl[do_idx].s=0;

    return err;
}
/*****************************************************************************/
/*****************************************************************************/
