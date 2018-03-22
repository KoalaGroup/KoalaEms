/*
 * lowlevel/ipmod/ipmodules.h
 * created 2016-Jan-08 PeWue
 * $ZEL: ipmodules.h,v 1.7 2016/07/01 16:35:23 wuestner Exp $
 */

#ifndef _ipmodules_h_
#define _ipmodules_h_

#include <sconf.h>
#include <stdio.h>
#include <netdb.h>
#include <errorcodes.h>
#include "../../objects/domain/dom_ml.h"

struct ipsock {
    int p;
    int socktype;
    socklen_t addrlen;
    struct sockaddr *addr;
    char *addrstr;  /* remote address, prepared for debug/log output
                       if the socket is not connected this is NULL */
};

void ipmodules_init(void);
void ipmodules_done(void);
plerrcode ipmod_init(struct ml_entry* module);
void ipmod_unlink(struct ipsock *sock);

plerrcode ipmod_write_stream(struct ml_entry*, ems_u8*, int);
plerrcode ipmod_read_stream(struct ml_entry*, ems_u8*, int);
plerrcode ipmod_read_stream_nonblock(struct ml_entry *module, ems_u8 *buf,
        int len, int *received);
plerrcode ipmod_read_stream_timeout(struct ml_entry*, ems_u8*, int,
        struct timeval*);

void print_sockaddr(struct sockaddr *addr);
void print_addrinfo(struct addrinfo *info);
void print_addrinfo_rec(struct addrinfo *info);

#endif
