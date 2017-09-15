/*
 * normsock.h
 * 
 * created: 2011-08-23 PW
 * $ZEL: normsock.h,v 1.1 2011/08/25 23:47:10 wuestner Exp $
 */

#ifndef _normsock_h_
#define _normsock_h_

#include <sys/types.h>

/* this describes a data block, possible partially written or read */
struct data_descr {
    int refcount;
    u_int32_t size; /* bytes read or to be sent */
    size_t space;   /* bytes allocated for data */
    u_int8_t *opaque;
};

/* this describes an entry for a list of data_descr */
struct data_ent {
    struct data_ent *next;
    struct data_descr *data;
};

struct sockname {
    struct sockname *next;
    const char *name; /* pointer to argv[x], do not delete! */
    int maxqueue;
    int inout; /* 0: input, 1: output*/
};

struct sockdescr {
    struct sockdescr *next;
    char *host;
    char *service;
    int listening;
    int p, idx, stalled;
    int inout; /* 0: input, 1: output*/
    /* objsize defines, what the first word counts: 1: bytes 4: words */
    int objsize;
    int maxqueue;              /* maximum number of queue entries */
    struct data_descr *ddescr;
    struct data_ent *queue;    /* not used by reading sockets */
};

void normsock_set_loginfo(FILE *logfile, int level);
void normsock_set_ip_info(int no_v4, int no_v6);
int open_socket(struct sockname *name, struct sockdescr**, int objsize);
int accept_socket(struct sockdescr*, struct sockdescr**);
void close_socket(struct sockdescr *sock);
int sock_store(struct sockdescr *sock, struct data_descr *data);
int sock_write(struct sockdescr *sock);
int sock_read(struct sockdescr *sock);

#endif
