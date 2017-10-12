/*
 * send_val2ems.c
 * 
 * created 2013-08-22 PW
 * $ZEL: send_val2ems.c,v 1.1 2013/10/28 21:45:07 wuestner Exp $
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
#include "xdrstring.h"

struct value {
    struct value *next;
    double val;
};

struct line {
    struct line *next;
    const char *s;
};

static int no_v4=0;
static int no_v6=0;
static FILE *log=0;

static const char *outsockname=0;
static struct value *values=0;
static struct line *lines=0;
static int numval=0;
static int numline=0;
static int p=-1;

static struct timeval now;

#define LOG(fmt, arg...)         \
    do {                         \
        FILE *l=log?log:stderr;  \
        fprintf(l, fmt, ## arg); \
        fflush(l);               \
    } while (0)

/******************************************************************************/
/*
 * Description
 * ===========
 * 
 * This program should be used to inject some data into an EMS data stream.
 * It accepts arbitrary strings and floating point (double) numbers at the 
 * command line. (see printusage()). It connects to a listening program 
 * via a TCP socket.
 * The preferred listening program is async2shm. async2shm writes the received
 * data to a shared memory segment, from which an EMS server can read the data
 * via the local procedure 'read_mapped_data'.
 * For testing async_save can be used as receiving program.
 * 
 * The data structure in the EMS stream will (as always) consist of a sequence
 * of 32-bit words:
 * 1: number of following words
 * 2: timestamp (seconds since the epoch)
 * 3: timestamp (microseconds)
 * 4: number of following double values (nv)
 *    each double is stored in two 32-bit words
 * 4+2*nv: number of following strings
 * 5+2*nv: start of first XDR string (if any)
 * ... next string
 * ...
 */
/******************************************************************************/
static int
store_value(const char *arg)
{
    struct value *value, *help;
    char *end;

    value=calloc(1, sizeof(struct value));
    value->val=strtod(arg, &end);
    if (end!=arg+strlen(arg)) {
        LOG("cannot convert \"%s\" to double\n", arg);
        return -1;
    }

    if (values) {
        help=values;
        while (help->next)
            help=help->next;
        help->next=value;
    } else {
        values=value;
    }
    numval++;

    return 0;
}
/******************************************************************************/
static int
store_text(const char *arg)
{
    struct line *line, *help;

    line=calloc(1, sizeof(struct line));
    line->s=arg;

    if (lines) {
        help=lines;
        while (help->next)
            help=help->next;
        help->next=line;
    } else {
        lines=line;
    }
    numline++;

    return 0;
}
/******************************************************************************/
static void
printusage(char* argv0)
{
    LOG("usage: %s [-h] [-4|-6] -o addr [-t text] [val] [val] ...\n", argv0);
    LOG("  -h: this help\n");
    LOG("  -4: use IPv4 only\n");
    LOG("  -6: use IPv6 only\n");
    LOG("  -o: output (host:port (should connect to async2shm)\n");
    LOG("  -t: arbitrary text (can be used more than once)\n");
    LOG("  val ...: arbitrary number of floating point values\n");
}
/******************************************************************************/
static int
readargs(int argc, char* argv[])
{
    int c, err, i;
    err=0;

    while (!err && ((c=getopt(argc, argv, "h46:o:t:")) != -1)) {
        switch (c) {
        case 'h': printusage(argv[0]); return 1;
        case '4': no_v6=1; break;
        case '6': no_v4=1; break;
        case 'o': outsockname=optarg; break;
        case 't': store_text(optarg); break;
        default: err=1;
        }
    }
    if (err) {
        printusage(argv[0]);
        return -1;
    }
    
    if (!outsockname) {
        LOG("output socket is mandatory.\n");
        printusage(argv[0]);
        return -1;
    }

    for (i=optind; i<argc; i++) {
        if (store_value(argv[i])<0) {
            printusage(argv[0]);
            return -1;
        }
    }

    return 0;
}
/******************************************************************************/
static int
connect_socket(const char *host, const char *service)
{
    struct addrinfo *addr, *a;
    struct addrinfo hints;
    int res, sock;

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
        LOG("request addrinfo for \"[%s]:%s\": %s\n",
            host, service, gai_strerror(res));
        goto error;
    }

    res=-1;
    for (a=addr; a; a=a->ai_next) {
        if ((sock=socket(a->ai_family, a->ai_socktype, a->ai_protocol))<0) {
            LOG("create socket: %s\n", strerror(errno));
            continue;
        }

        if (connect(sock, a->ai_addr, a->ai_addrlen)<0) {
            printf("connect to [%s]:%s: %s\n",
                    host, service, strerror(errno));
            close(sock);
            continue;
        }

        if (fcntl(sock, F_SETFL, O_NDELAY)<0) {
            LOG("connect_socket:fcntl(O_NDELAY): %s\n", strerror(errno));
            close(sock);
            continue;
        }

        res=0;
    }
    if (res<0)
        LOG("connection to [%s]:%s failed\n", host, service);

error:
    freeaddrinfo(addr);
    return res==0?sock:-1;
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
            LOG("cannot parse %s\n", name);
            return -1;
        }
        if (pp-name-2>=NI_MAXHOST) {
            LOG("host part of %s too long\n", name);
            return -1;
        }
        *host=strndup(name+1, pp-name-1);
        if (!*host) {
            LOG("allocate space for hostname: %s\n", strerror(errno));
            return -1;
        }
        if (strlen(pp+2)>=NI_MAXSERV) {
            LOG("service part of %s too long\n", name);
            free(*host);
            return -1;
        }
        *service=strdup(pp+2);
        if (!*service) {
            LOG("allocate space for service: %s\n", strerror(errno));
            free(*host);
            return -1;
        }
    } else {
        pp=strrchr(name, ':');
        if (pp && pp==name) {
            pp++;
        } else if (pp) {
            if (pp-name>=NI_MAXHOST) {
                LOG("host part of %s too long\n", name);
                return -1;
            }
            *host=strndup(name, pp-name);
            if (!*host) {
                LOG("allocate space for hostname: %s\n", strerror(errno));
                return -1;
            }
            pp++;
        } else {
            pp=name;
        }
        if (strlen(pp)>=NI_MAXSERV) {
            LOG("service part of %s too long\n", name);
            free(*host);
            return -1;
        }
        *service=strdup(pp);
        if (!*service) {
            LOG("allocate space for service: %s\n", strerror(errno));
            free(*host);
            return -1;
        }
    }

#if 0
    printf("host   : >%s<\n", *host);
    printf("service: >%s<\n", *service);
#endif
    return 0;
}
/******************************************************************************/
static int
open_socket(const char *sname)
{
    char *host, *service;
    int p=-1;

    if (parse_name(sname, &host, &service)<0)
        return -1;

    if (!host) {
        LOG("no hostname given in \"%s\"\n", sname);
        return -1;
    }
    if (!service) {
        LOG("no service given in \"%s\"\n", sname);
        return -1;
    }

    p=connect_socket(host, service);

    free(host);
    free(service);
        
    return p;
}
/******************************************************************************/
int
main(int argc, char *argv[])
{
    struct sigaction sa;
    struct line *line;
    struct value *value;
    u_int32_t *data;
    u_int8_t *bdata;
    int datasize, idx, res;

    log=stderr;

    gettimeofday(&now, 0);

    if ((res=readargs(argc, argv)))
        return res<0?1:0;

    /* install an ignore handler for SIGPIPE */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) < 0) {
        LOG("Could not install SIGPIPE handler.\n");
        return 2;
    }

    p=open_socket(outsockname);
    if (p<0) {
        //LOG("cannot connect output socket\n");
        return 2;
    }

    /* alloc data storage */
    /* calculate the size (in terms of 32-bit words) */
    datasize=1; /* word counter */
    datasize+=2; /* timestamp */
    datasize+=1+numval*2; /* values + counter */
    datasize+=1; /* counter for text lines */
    line=lines;
    while (line) {
        datasize+=strxdrlen(line->s);
        line=line->next;
    }

    data=calloc(datasize, 4);
    if (!data) {
        LOG("alloc data space: %s\n", strerror(errno));
        return 2;
    }

    /* convert and store the data */
    data[0]=datasize-1; idx=1;
    data[idx++]=now.tv_sec;
    data[idx++]=now.tv_usec;

    data[idx++]=numval;
    value=values;
    while (value) {
        u_int64_t ival=*((u_int64_t*)&(value->val));
        data[idx++]=(ival>>32)&0xffffffff;
        data[idx++]=ival&0xffffffff;
        value=value->next;
    }

    line=lines;
    data[idx++]=numline;
    while (line) {
        u_int32_t *p;
        p=outstring(data+idx, line->s);
        idx=p-data;
        line=line->next;
    }

    /* convert to network byte ordering */
    swap_falls_noetig(data, datasize);

#if 0
    for (idx=0; idx<datasize; idx++) {
        int i;
        printf("[%3d] %08x %08x", idx, data[idx], be32toh(data[idx]));
        for (i=0; i<4; i++) {
            char c=(data[idx]>>(8*i))&0xff;
            printf(" %c", c>=' '&&c<='~'?c:'.');
        }
        printf("\n");
    }
#endif

    /* send the data */
    bdata=(u_int8_t*)data;
    datasize*=4;
    idx=0;
    while (datasize) {
        res=write(p, bdata+idx, datasize-idx);
        if (res<=0) {
            if (res==0) { /* blocked */
                sleep(1);
            } else {
                LOG("send data: %s\n", strerror(errno));
                return 2;
            }
        }
        idx+=res;
        datasize-=res;
    }
    close(p);

    return 0;
}
/******************************************************************************/
/******************************************************************************/
