/*
 * lowlevel/ipmod/ipmodules.c
 * 
 * created: 2015-Dec-19 PeWue
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: ipmodules.c,v 1.10 2016/11/26 01:22:56 wuestner Exp $";

#include <sconf.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <errorcodes.h>
#include "../../commu/commu.h"
#include "ipmodules.h"
#ifndef HAVE_CGETCAP
#include <getcap.h>
#endif
#include <rcs_ids.h>

RCS_REGISTER(cvsid, "lowlevel/ipmodules")

extern ems_u32* outptr;

#define MAX(a, b) ((a>b)?a:b)

struct udp_sock {
    struct udp_sock *next;
    int port;
    int p;
    int refcount;
};

static struct udp_sock *udp_socks=0;

#undef DUMP_UDP_SOCKS

/*****************************************************************************/
#ifdef DUMP_UDP_SOCKS
static void
dump_udp_socks(const char *txt)
{
    struct udp_sock *sock;
    printf("udp_socks: %s\n", txt);
    printf("udp_socks=%p\n", udp_socks);
    sock=udp_socks;
    while (sock) {
        printf("%p->next=%p\n", sock, sock->next);
        printf("%p->port=%d\n", sock, sock->port);
        printf("%p->p   =%d\n", sock, sock->p);
        printf("%p->rcnt=%d\n", sock, sock->refcount);
        sock=sock->next;
    }
}
#endif
/*****************************************************************************/
void
ipmodules_init(void)
{
printf("ipmodules_init\n");
    udp_socks=0;
}
/*****************************************************************************/
void
ipmodules_done(void)
{
printf("ipmodules_done\n");
    if (udp_socks)
        printf("IPMOD_LOW_DONE: UDP_SOCKS NOT EMPTY!\n");
}
/*****************************************************************************/
/*
 * sockaddr2string returns the address in a "malloced" string.
 * The caller is responsible to free it.
 */
static char*
sockaddr2string(struct sockaddr *addr)
{
    const size_t size=MAX(INET_ADDRSTRLEN, INET6_ADDRSTRLEN);
    char *cc, *cp;
    uint16_t port;
    void *ad;

    /* we need '[', size, ']', ':' and portnumber */
    cc=malloc(size+6);
    if (!cc) {
        printf("sockaddr2string: %s\n", strerror(errno));
        return 0;
    }
    cp=cc;

    switch (addr->sa_family) {
    case AF_INET:
        port=ntohs(((struct sockaddr_in*)addr)->sin_port);
        ad=&((struct sockaddr_in*)addr)->sin_addr;
        break;
    case AF_INET6:
        port=ntohs(((struct sockaddr_in6*)addr)->sin6_port);
        ad=&((struct sockaddr_in6*)addr)->sin6_addr;
        *cp++='[';
        break;
    default:
        printf("sockaddr2string: unknown family %d\n", addr->sa_family);
        free(cc);
        return 0;
    }

    if ((inet_ntop(addr->sa_family, ad, cp, size))==0) {
        printf("inet_ntop: %s\n", strerror(errno));
        free(cc);
        return 0;
    }
    cp=cc+strlen(cc);

    if (addr->sa_family==AF_INET6)
        *cp++=']';

    sprintf(cp, ":%d", port);

    return cc;
}
/*****************************************************************************/
void
print_sockaddr(struct sockaddr *addr)
{
    const size_t size=MAX(INET_ADDRSTRLEN, INET6_ADDRSTRLEN);
    char cc[size];
    uint16_t port;
    void *ad;

    switch (addr->sa_family) {
    case AF_INET:
        port=ntohs(((struct sockaddr_in*)addr)->sin_port);
        ad=&((struct sockaddr_in*)addr)->sin_addr;
        break;
    case AF_INET6:
        port=ntohs(((struct sockaddr_in6*)addr)->sin6_port);
        ad=&((struct sockaddr_in6*)addr)->sin6_addr;
        break;
    default:
        printf("print_sockaddr: unknown family %d\n", addr->sa_family);
        return;
    }

    if ((inet_ntop(addr->sa_family, ad, cc, size))==0) {
        printf("inet_ntop: %s\n", strerror(errno));
        return;
    }
    printf("[%s]:%d", cc, port);
}

void
print_addrinfo(struct addrinfo *info)
{
    printf("  ai_flags:    0x%x", info->ai_flags);
    if (info->ai_flags&AI_PASSIVE)     printf(" AI_PASSIVE");
    if (info->ai_flags&AI_CANONNAME)   printf(" AI_CANONNAME");
    if (info->ai_flags&AI_NUMERICHOST) printf(" AI_NUMERICHOST");
    if (info->ai_flags&AI_V4MAPPED)    printf(" AI_V4MAPPED");
    if (info->ai_flags&AI_ALL)         printf(" AI_ALL");
    if (info->ai_flags&AI_ADDRCONFIG)  printf(" AI_ADDRCONFIG");
    printf("\n");

    printf("  ai_family:   ");
    switch (info->ai_family) {
    case AF_UNSPEC: printf("AF_UNSPEC\n"); break;
    case AF_UNIX:   printf("AF_UNIX\n");   break;
    case AF_INET:   printf("AF_INET\n");   break;
    case AF_INET6:  printf("AF_INET6\n");  break;
    default:
        printf("unknown family %d\n", info->ai_family);
    }

    printf("  ai_socktype: ");
    switch (info->ai_socktype) {
    case SOCK_STREAM:    printf("SOCK_STREAM\n");    break;
    case SOCK_DGRAM:     printf("SOCK_DGRAM\n");     break;
    case SOCK_RAW:       printf("SOCK_RAW\n");       break;
    case SOCK_RDM:       printf("SOCK_RDM\n");       break;
    case SOCK_SEQPACKET: printf("SOCK_SEQPACKET\n"); break;
    case SOCK_DCCP:      printf("SOCK_DCCP\n");      break;
    case SOCK_PACKET:    printf("SOCK_PACKET\n");    break;
    default:
        printf("unknown socktype %d\n", info->ai_socktype);
    }

    printf("  ai_protocol: %d\n", info->ai_protocol);

    printf("  ai_addrlen:  %d\n", info->ai_addrlen);

    printf("  ai_addr:     ");
    print_sockaddr(info->ai_addr);
    printf("\n");
}
void
print_addrinfo_rec(struct addrinfo *info)
{
    while (info) {
        print_addrinfo(info);
        info=info->ai_next;
    }
}
/*****************************************************************************/
void
ipmod_unlink(struct ipsock *sock)
{
    struct udp_sock *s0, *s1;

#if 1
    printf("ipmod_unlink(%p %d)\n", sock, sock->p);
#endif

    if (sock->socktype==SOCK_DGRAM) {
        /* find the associated struct udp_sock */
        s0=0;
        s1=udp_socks;
        while (s1 && s1->p!=sock->p) {
            s0=s1;
            s1=s1->next;
        }
        if (!s1) { /* not possible */
            printf("ipmod_unlink: socket %d not found!\n", sock->p);
        } else {
#if 1
            printf("ipmod_unlink(%p) refcount=%d\n", s1, s1->refcount);
#endif
            s1->refcount--;
            if (s1->refcount<=0) {
                close(s1->p);
                if (s0)
                    s0->next=s1->next;
                else
                    udp_socks=s1->next;
                free(s1);
            }
        }
    } else {
        printf("ipmod_unlink: close %d\n", sock->p);
        close(sock->p);
    }

    free(sock->addr);
    free(sock->addrstr);
    free(sock);
}
/*****************************************************************************/
static int
set_recv_timeout(int p, int timeout /* in ms*/)
{
    struct timeval to;
    int res;

    to.tv_sec=timeout/1000;
    to.tv_usec=(timeout%1000)*1000;
    res=setsockopt(p, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(struct timeval));
    if (res<0)
        complain("ipmod_init:set_recv_timeout: %s", strerror(errno));

    return res;
}
/*****************************************************************************/
/* extract the port number from addrinfo */
static int
extract_port(struct addrinfo *addr)
{
    int port;

    switch (addr->ai_family) {
    case AF_INET:
        port=ntohs(((struct sockaddr_in*)(addr->ai_addr))->sin_port);
        break;
    case AF_INET6:
        port=ntohs(((struct sockaddr_in6*)(addr->ai_addr))->sin6_port);
        break;
    default:
        complain("ipmod_init:extract_port: unknown family %d", addr->ai_family);
        port=-1;
    }
#if 0
    printf("extract_port: returning %d\n", port);
#endif
    return port;
}
/*****************************************************************************/
static struct addrinfo*
resolve_address(const char *node, const char *serv, int socktype)
{
    char service[NI_MAXSERV];
    struct addrinfo *addr;
    struct addrinfo hints;
    int res;

#if 0
    printf("resolve_address: node=%p %s\n", node, node);
    printf("resolve_address: serv=%p %s\n", serv, serv);
    printf("resolve_address: socktype=%d\n", socktype);
#endif

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_flags=AI_ADDRCONFIG;
    if (!node)
        hints.ai_flags|=AI_PASSIVE;

    hints.ai_family=AF_INET; /* no current module supports IPv6 */

    hints.ai_socktype=socktype;

    /* hints.ai_protocol=0; */

    /* Is the port given as a number?
     * getaddrinfo understands decimal numbers only.
     */
    {
        long int port;
        char *endptr;
        errno=0;
        port=strtol(serv, &endptr, 0);
        if (errno==0 && *endptr=='\0')
            snprintf(service, NI_MAXSERV, "%ld", port);
        else
            strncpy(service, serv, NI_MAXSERV);
    }

    if ((res=getaddrinfo(node, service, &hints, &addr))) {
        complain("ipmod_init: request addrinfo for \"%s:%s\": %s\n",
            node, service, gai_strerror(res));
        return 0;
    }

    return addr;
}
/*****************************************************************************/
static struct udp_sock*
find_udp_sock(struct addrinfo *addr)
{
    struct udp_sock *sock;
    int port;

    port=extract_port(addr);
    if (port==-1)
        return 0;

    sock=udp_socks;
    while (sock && sock->port!=port)
        sock=sock->next;
#if 0
    printf("find_udp_sock(%d): %p\n", port, sock);
#endif
    return sock;
}
/*****************************************************************************/
/*
 * ipmod_sock_init initialises a socket using the given addressinfo.
 * In case of UDP the socket is bound to INADDR_ANY/port and not connected.
 */
static plerrcode
ipmod_sock_init(struct addrinfo *raddr, struct addrinfo *laddr, int socktype,
        int *p, char **addrstr)
{
    struct addrinfo *ra, *la;
    int res;

    *p=-1;

    /* try to create the socket */
    for (ra=raddr; ra; ra=ra->ai_next) {
        if ((*p=socket(ra->ai_family, ra->ai_socktype, ra->ai_protocol))<0) {
            printf("create socket: %s\n", strerror(errno));
            continue;
        }
#if 0
        printf("ipmod_sock_init: p=%d\n", *p);
#endif

        /* for TCP we have to connect */
        if (socktype==SOCK_STREAM) {
#if 0
            printf("ipmod_sock_init: try to connect to\n");
            print_addrinfo(ra);
#endif
            if (connect(*p, ra->ai_addr, ra->ai_addrlen)<0) {
                int error=errno;
                printf("ipmod_init: connect to\n");
                print_addrinfo(ra);
                printf("failed : %s\n", strerror(error));
                close(*p);
                *p=-1;
                continue;
            }
            /* here the connection is established and we can convert
               the actual address to a string (for debug outputs) */
            *addrstr=sockaddr2string(ra->ai_addr);
        }

        /* for UDP we have to bind and we do it for TCP too if laddr is given */
        if (laddr) {
            int bound=0;
            for (la=laddr; la; la=la->ai_next) {
#if 0
                printf("ipmod_sock_init: try to bind to ");
                print_sockaddr(la->ai_addr);
                printf("\n");
#endif
                res=bind(*p, la->ai_addr, la->ai_addrlen);
                if (res==0) { /* success */
                    bound=1;
                    break;
                } else {
                    int error=errno;
                    printf("ipmod_init: bind to ");
                    print_sockaddr(la->ai_addr);
                    printf(": %s\n", strerror(error));
                }
            }
            if (!bound) {
                close(*p);
                *p=-1;
                continue;
            }
        }

        printf("ipmod_init: socket %d to ", *p);
        print_sockaddr(ra->ai_addr);
        printf(" successfully created\n");
        break;
    }

    if (*p<0) {
        complain("ipmod_init: failed to create socket");
        return plErr_System;
    } else {
        return plOK;
    }
}
/*****************************************************************************/
/*
 * ipmod_init initialises the socket for an 'ip' module.
 * In case of UDP the socket is bound to INADDR_ANY/port and not connected.
 * A UDP socket is shared with other modules if the same local port is
 * required.
 */
plerrcode
ipmod_init(struct ml_entry* module)
{
    struct addrinfo *raddr=0;
    struct addrinfo *laddr=0;
    struct ipsock *sock=0;
    int socktype;
    plerrcode pres;

#ifdef DUMP_UDP_SOCKS
    dump_udp_socks("ipmod_init");
#endif

    if (module->modulclass!=modul_ip) {
        complain("ipmod_init: module is not an ip module");
        return plErr_BadModTyp;
    }

    /* we can support tcp and udp */
    if (!module->address.ip.protocol || !module->address.ip.protocol[0]) {
        complain("ipmod_init: protocol not given");
        return plErr_ArgRange;
    }
    if (strcasecmp(module->address.ip.protocol, "udp")==0) {
        socktype=SOCK_DGRAM;
    } else if (strcasecmp(module->address.ip.protocol, "tcp")==0) {
        socktype=SOCK_STREAM;
    } else {
        complain("ipmod_init: protocol \"%s\" not supported",
                module->address.ip.protocol);
        return plErr_ArgRange;
    }

    /* check the existence of parameters needed */
    if (!module->address.ip.node || !module->address.ip.node[0]) {
        complain("ipmod_init: no node name given");
        return plErr_ArgNum;
    }
    if (!module->address.ip.rserv || !module->address.ip.rserv[0]) {
        complain("ipmod_init: no remote service name given");
        return plErr_ArgNum;
    }
    if (socktype==SOCK_DGRAM &&
            (!module->address.ip.lserv || !module->address.ip.rserv[0])) {
        complain("ipmod_init: no local service name given");
        return plErr_ArgNum;
    }

    /*
     * try to resolve the given addresses
     */

    /* remote address */
    raddr=resolve_address(module->address.ip.node, module->address.ip.rserv,
            socktype);
    if (!raddr) {
        pres=plErr_System;
        goto error;
    }
#if 0
    printf("ipmod_init: resolved remote address:\n");
    print_addrinfo_rec(raddr);
#endif

    /* local address */
    /* needed for bind; mandatory for UDP, optional for TCP */
    if (module->address.ip.lserv) {
        laddr=resolve_address(0, module->address.ip.lserv, socktype);
        if (!laddr) {
            pres=plErr_System;
            goto error;
        }
#if 0
        printf("ipmod_init: resolved local address:\n");
        print_addrinfo_rec(laddr);
#endif
    }

    /* create an empty socket info structure */
    sock=calloc(1, sizeof(struct ipsock));
    if (!sock) {
        complain("ipmod_init: malloc sock structure: %s", strerror(errno));
        pres=plErr_System;
        goto error;
    }
    sock->socktype=socktype; /* nedded for ipmod_unlink only */

    /*
     * copy the remote address (needed for sendto)
     * raddr may contain more than one address, but we have no chance to know
     * the best one, we just use the first one
     * (in case of IPv4 there should be only one entry anyway)
     */
    sock->addrlen=raddr->ai_addrlen;
    sock->addr=malloc(raddr->ai_addrlen);
    if (!sock->addr) {
        complain("ipmod_init: malloc address: %s", strerror(errno));
        pres=plErr_System;
        goto error;
    }
    memcpy(sock->addr, raddr->ai_addr, raddr->ai_addrlen);

    /* if socktype==SOCK_DGRAM: search for an existing socket */
    if (socktype==SOCK_DGRAM) {
        struct udp_sock *udp_sock;

        udp_sock=find_udp_sock(laddr);
        if (udp_sock) {
            /* here we are mostly done, just have to increment
               the link counter and copy something */
            udp_sock->refcount++;
            sock->p=udp_sock->p;
            module->address.ip.sock=sock;
#if 0
            printf("ipmod_init returning(B):\n");
            printf("  sock=%p\n", sock);
            printf("  sock->p=%d\n", sock->p);
#endif
            pres=plOK;
            goto no_error;
        }
    }

    /*
     * We did not find a UDP socket with the same port or we need a
     * TCP socket. Let's create a new one.
     */

    /*
    ipsock sock existiert hier,
    udp_sock muss noch gemacht werden
    */

    pres=ipmod_sock_init(raddr, laddr, socktype, &sock->p, &sock->addrstr);
    if (pres!=plOK) { /* ipmod_sock_init has to copy struct sockaddr *addr */
        goto error;
    }

    (void)set_recv_timeout(sock->p, module->address.ip.recvtimeout);

#if 0
    dump_udp_socks("before allocating");
#endif

    /*
     * now we have a new valid socket
     */
    /* let's store it in the list if it is a UDP socket */
    if (socktype==SOCK_DGRAM) {
        struct udp_sock *udp_sock;
        udp_sock=calloc(1, sizeof(struct udp_sock));
        if (!udp_sock) {
            printf("pmod_init:malloc udp_sock: %s\n", strerror(errno));
            pres=plErr_NoMem;
            goto error;
        }
        udp_sock->port=extract_port(laddr);
        udp_sock->p=sock->p;
        udp_sock->refcount=1;
        udp_sock->next=udp_socks;
        udp_socks=udp_sock;
#if 1
        printf("ipmod_init: stored %p for port %d and path %d\n",
                udp_sock, udp_sock->port, udp_sock->p);
#endif
    }
#if 0
    dump_udp_socks("after allocating");
#endif

    /* and update module->... */
    module->address.ip.sock=sock;
#if 0
    printf("ipmod_init returning(A):\n");
    printf("  sock=%p\n", sock);
    printf("  sock->p=%d\n", sock->p);
#endif
    pres=plOK;
    goto no_error;

error:
    if (sock)
        free(sock->addr);
    free(sock);

no_error:
    freeaddrinfo(laddr);
    freeaddrinfo(raddr);

    return pres;
}
/*****************************************************************************/
plerrcode
ipmod_write_stream(struct ml_entry *module, ems_u8 *buf, int len)
{
    int p, da, restlen;

    /* do we really need a sanity check here? */
    if (module->modulclass!=modul_ip) {
        complain("ipmod_write_stream: module is not an ip module");
        return plErr_BadModTyp;
    }
    if (!module->address.ip.sock) {
        complain("ipmod_write_stream: module is not initialised");
        return plErr_Program;
    }

#if 0
    {
        int i;
        if (module->address.ip.sock->addrstr)
            printf("send %d bytes to %s: ", len,
                    module->address.ip.sock->addrstr);
        else
            printf("send %d bytes: ", len);
        for (i=0; i<len; i++) {
            printf(" %02x", buf[i]);
        }
        printf("\n");
    }
#endif

    p=module->address.ip.sock->p;
    restlen=len;
    while (restlen) {
        da=write(p, buf, restlen);
        if (da>0) {
            buf+=da;
            restlen-=da;
        } else {
            if (errno!=EINTR) {
                printf("ipmod_write_stream: %s\n", strerror(errno));
                return plErr_System;
            }
        }
    }
    return plOK;
}
/*****************************************************************************/
plerrcode
ipmod_read_stream(struct ml_entry *module, ems_u8 *buf, int len)
{
    int p, restlen, da;

    /* do we really need a sanity check here? */
    if (module->modulclass!=modul_ip) {
        complain("ipmod_read_stream: module is not an ip module");
        return plErr_BadModTyp;
    }
    if (!module->address.ip.sock) {
        complain("ipmod_read_stream: module is not initialised");
        return plErr_Program;
    }

    p=module->address.ip.sock->p;
    restlen=len;
    while (restlen) {
        da=read(p, buf, restlen);
        if (da>0) {
            buf+=da;
            restlen-=da;
        } else if (errno==EINTR) {
            continue;
        } else {
            printf("ipmod_read_stream: %s\n", strerror(errno));
            return plErr_System;
        }
    }
    return plOK;
}
/*****************************************************************************/
plerrcode
ipmod_read_stream_timeout(struct ml_entry *module, ems_u8 *buf, int len,
    struct timeval *timeout)
{
    int p, restlen, da, res;
    fd_set readfds;
    struct timeval t0, now;
    struct timeval to, end;
    plerrcode plerr=plOK;

    /* do we really need a sanity check here? */
    if (module->modulclass!=modul_ip) {
        complain("ipmod_read_stream_timeout: module is not an ip module");
        return plErr_BadModTyp;
    }
    if (!module->address.ip.sock) {
        complain("ipmod_read_stream_timeout: module is not initialised");
        return plErr_Program;
    }

    p=module->address.ip.sock->p;

    //printf("req.  timeout: (%ld:%ld)\n", timeout->tv_sec, timeout->tv_usec);

    gettimeofday(&t0, 0);
    timeradd(&t0, timeout, &end);
    //printf("t0: (%ld:%ld) end: (%ld:%ld)\n",
    //        t0.tv_sec, t0.tv_usec,
    //        end.tv_sec, end.tv_usec);

    restlen=len;
    while (restlen) {
        gettimeofday(&now, 0);
        timersub(&end, &now, &to);
        //printf("calc. timeout: (%ld:%ld)\n", to.tv_sec, to.tv_usec);
        if (to.tv_sec<0)
            timerclear(&to);

        FD_ZERO(&readfds);
        FD_SET(p, &readfds);
        res=select(p+1, &readfds, 0, 0, &to);
        if (res<0) {
            if (errno==EINTR) {
                printf("ipmod_read_stream_timeout:select: EINTR\n");
                continue;
            }
            complain("ipmod_read_stream_timeout:select: %s", strerror(errno));
            *outptr++=errno;
            plerr=plErr_System;
            goto error;
        }

        if (!FD_ISSET(p, &readfds)) {
 #if 0
            complain("ipmod_read_stream_timeout(%ld:%ld): timeout",
                timeout->tv_sec, timeout->tv_usec);
 #endif
            plerr=plErr_Timeout;
            goto error;
        }

        da=read(p, buf, restlen);
//printf("da=%d, buf[0]=%02x\n", da, buf[0]);
        if (da>0) {
            buf+=da;
            restlen-=da;
        } else if (da==0) {
            printf("ipmod_read_stream_timeout: pipe broken\n");
            plerr=plErr_System;
            goto error;
        } else if (errno==EINTR) {
            continue;
        } else {
            printf("ipmod_read_stream_timeout: %s\n", strerror(errno));
            *outptr++=errno;
            plerr=plErr_System;
            goto error;
        }
    }

error:
    /* new_timeout = old_timeout-(now-t0) */
    /* new_timeout = old_timeout+t0-now   */
    gettimeofday(&now, 0);
    timeradd(timeout, &t0, &to);
    timersub(&to, &now, timeout);
    //printf("new   timeout: (%ld:%ld)\n", timeout->tv_sec, timeout->tv_usec);
    if (plerr!=plOK || timeout->tv_sec<0)
        timerclear(timeout);

    return plerr;
}
/*****************************************************************************/
plerrcode
ipmod_read_stream_nonblock(struct ml_entry *module, ems_u8 *buf, int len,
    int *received)
{
    plerrcode pres=plOK;
    int p;

    /* do we really need a sanity check here? */
    if (module->modulclass!=modul_ip) {
        complain("ipmod_read_stream: module is not an ip module");
        return plErr_BadModTyp;
    }
    if (!module->address.ip.sock) {
        complain("ipmod_read_stream: module is not initialised");
        return plErr_Program;
    }

    p=module->address.ip.sock->p;
  
    *received=recv(p, buf, len, MSG_DONTWAIT);
    if (*received<0 && errno!=EAGAIN && errno!=EWOULDBLOCK) {
        printf("ipmod_read_stream_nonblock: %s\n", strerror(errno));
        pres=plErr_System;
    }

    return pres;
}
/*****************************************************************************/
/*****************************************************************************/
