/*
 * jessica_dump.c
 * 
 * created 26.Oct.2003 
 * $ZEL: jessica_dump.c,v 1.2 2003/10/30 13:09:43 wuestner Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>

typedef unsigned int ems_u32;

#define SWAP_32(n) (((n & 0xff000000) >> 24) | \
    ((n & 0x00ff0000) >>  8) | \
    ((n & 0x0000ff00) <<  8) | \
    ((n & 0x000000ff) << 24))

struct event_buffer {
    ems_u32 size;
    ems_u32 evno;
    ems_u32 trigno;
    ems_u32 subevents;
    size_t max_size;
    ems_u32* data;
    size_t max_subevents;
    ems_u32** subeventptr;
};

typedef enum {intype_sock, intype_stdin} intypes;
static intypes intype;
static char *inname;
static int inport;
static unsigned int inhostaddr;
static int verbose, count;

/******************************************************************************/
static void
printusage(char* argv0)
{
    printf("usage: %s [-h] [-v] [inport]\n",
        basename(argv0));
    printf("  -h: this help\n");
    printf("  -v: verbose\n");
    printf("  -c: count\n");
    printf("  inport: hostname:(portnum|portname) or '-' for stdin\n");
    printf("    default is \"evdistin\"\n");
}
/******************************************************************************/
static int
readargs(int argc, char* argv[])
{
    /*extern char *optarg;*/
    int c, err;
    struct servent* service;
    verbose=0;
    count=-1;

    err=0;
    while (!err && ((c=getopt(argc, argv, "hvc:")) != -1)) {
        switch (c) {
        case 'h': printusage(argv[0]); return 1;
        case 'v': verbose=1; break;
        case 'c': count=atoi(optarg); break;
        default: err=1;
        }
    }
    if (err) {
        printusage(argv[0]);
        return -1;
    }

    if (optind<argc)
        inname=argv[optind];
    else
        inname="evdistin";
    return 0;
}
/******************************************************************************/
static int
decode_run_nr(ems_u32* ptr, int size)
{
    if (size!=1) {
        printf("\nrun nr: size=%d\n", size);
        return -1;
    }
    printf("\nrun nr=%d\n", ptr[0]);
    return 0;
}
/******************************************************************************/
static int
decode_timestamp(ems_u32* ptr, int size)
{
    struct tm* tm;
    time_t t;
    char s[1024];

    if (size!=2) {
        printf("\ntimestamp: size=%d\n", size);
        return -1;
    }
    /* printf("\ntimestamp: %08x %08x\n", ptr[0], ptr[1]); */

    t=ptr[0];
    tm=localtime(&t);
    strftime(s, 1024, "%Y-%m-%d %H:%M:%S", tm);
    printf("\ntimestamp: %s.%06d\n", s, ptr[1]);
    return 0;
}
/******************************************************************************/
static int
decode_caen_v265(ems_u32* ptr, int size)
{
    int nr_mod, i, j;

    nr_mod=*ptr++; size--;

    if (size!=16) {
        printf("\ncaen v265: size=%d\n", size);
        return -1;
    }

    printf("\nV265 Charge Integrating ADC:\n");
    for (j=0; j<2; j++) {
        printf("%d: ", j?15:12);
        for (i=0; i<16; i++) {
            ems_u32 v=ptr[i];
            int r15=!!(v&(1<<12));
            if (r15==j) {
                int chan=(v>>13)&7;
                int val=v&0xfff;
                if (!r15) val<<=3;
                printf("[%d %04x] ", chan, val);
            }
        }
        printf("\n");
    }
    return 0;
}
/******************************************************************************/
static int
decode_caen_v556(ems_u32* ptr, int size)
{
    int nr_mod, i;

    printf("\nV556 Peak Sensing ADC\n");

    nr_mod=*ptr++; size--;

    for (i=0; i<nr_mod; i++) {
        ems_u32 head;
        int nr_data, j;

        nr_data=*ptr++;
        head=*ptr++; nr_data--;
        printf("module %d, header 0x%04x; mult=%d\n", i, head, ((head>>12)&7)+1);
        for (j=0; j<nr_data; j++) {
            ems_u32 v;
            v=*ptr++;
            printf("[%d %04x] ", (v>>12)&7, v&0xfff);
        }
    }

    return 0;
}
/******************************************************************************/
static int
decode_sis3400(ems_u32* ptr, int size)
{
    ems_u32 v, count;
    int i;

    /* printf("\nsis3400: size=%d\n", size); */
    v=*ptr++; size--;
    if (v!=1) {
        printf("\nsis3400: %d modules\n", v);
        return -1;
    }
    count=*ptr++; size--;
    if (count!=size) {
        printf("\nsis3400: size=%d count=%d\n", size, count);
        return -1;
    }
    printf("\nSIS3400 Time Stamper\n");
#if 0
    for (i=0; i<count; i++) {
        v=*ptr++; size--;
        printf("  %08x id %d chan %2d stamp %7d %c\n",
            v,
            (v>>26)&0x1f,
            (v>>20)&0x3f,
            v&0xfffff,
            (v&0x80000000)?'t':' ');
    }    
#endif    
    printf("%d words\n", count);
    return 0;
}
/******************************************************************************/
static int
proceed_event(struct event_buffer* eb)
{
    ems_u32* sptr=eb->data;
    int i;

    printf("event %d trig %d sub %d size %d\n",
            eb->evno, eb->trigno, eb->subevents, eb->size);
/*
    for (i=0; i<eb->size; i++) printf("%3d 0x%08x\n", i, eb->data[i]);
*/
    for (i=0; i<eb->subevents; i++) {
        int size;
        eb->subeventptr[i]=sptr;
        size=sptr[1];
        sptr+=size+2;
    }

    for (i=0; i<eb->subevents; i++) {
        ems_u32* ptr=eb->subeventptr[i];
/*
        printf("subevent %d size=%d\n", ptr[0], ptr[1]);
*/
        switch (ptr[0]) {
        case 0: decode_run_nr(ptr+2, ptr[1]); break;
        case 1: decode_timestamp(ptr+2, ptr[1]); break;
        case 2: decode_caen_v265(ptr+2, ptr[1]); break;
        case 3: decode_caen_v556(ptr+2, ptr[1]); break;
        case 4: decode_sis3400(ptr+2, ptr[1]); break;
        default: printf("unknown subevent %d; size=%d\n", ptr[0], ptr[1]);
        }
    }
    return 0;
}
/******************************************************************************/
static void
sigpipe(int num)
{}
/******************************************************************************/
static int
adjust_event_buffer(struct event_buffer* eb)
{
    if (eb->max_size<eb->size) {
        if (eb->data) free(eb->data);
        eb->data=malloc(eb->size*sizeof(ems_u32));
        if (!eb->data) {
            printf("malloc %d words for event: %s\n", eb->size, strerror(errno));
            return -1;
        }
        eb->max_size=eb->size;
    }

    if (eb->max_subevents<eb->subevents) {
        if (eb->subeventptr) free(eb->subeventptr);
        eb->subeventptr=malloc(eb->subevents*sizeof(ems_u32*));
        if (!eb->subeventptr) {
            printf("malloc %d ptrs for subeventptr: %s\n", eb->subevents, strerror(errno));
            return -1;
        }
        eb->max_subevents=eb->subevents;
    }

    return 0;
}
/*****************************************************************************/
static int
xread(int p, ems_u32* buf, int len)
{
    int restlen, da;
    char* cbuf=(char*)buf;

    restlen=len*sizeof(ems_u32);
    while (restlen) {
        da=read(p, cbuf, restlen);
        if (da>0) {
            cbuf+=da;
            restlen-=da;
        } else if (errno==EINTR) {
            continue;
        } else {
            printf("xread: %s\n", strerror(errno));
            return -1;
        }
    }
    return 0;
}
/******************************************************************************/
static int
read_event(int p, struct event_buffer* eb)
{
    size_t n;
    ems_u32 head[4];
    int swap, i;

    if (xread(p, head, 4)<0) return -1;
/*
    printf("head=0x%08x\n", head[0]);
*/
    if ((head[3]&0xffff0000) || ((head[3]==0) && (head[0]&0xffff0000)))
        swap=1;
    else
        swap=0;

    if (swap) {
        for (i=0; i<4; i++) head[i]=SWAP_32(head[i]);
    }
    eb->size=head[0]-3;
    eb->evno=head[1];
    eb->trigno=head[2];
    eb->subevents=head[3];

    if (adjust_event_buffer(eb)<0) return -1;
/*
    printf("size=%d\n", eb->size);
*/
    if (xread(p, eb->data, eb->size)<0) return -1;
    if (swap) {
        for (i=0; i<eb->size; i++) eb->data[i]=SWAP_32(eb->data[i]);
    }
    return 0;
}
/******************************************************************************/
static int
create_connecting_socket(unsigned int host, int port)
{
    int sock, res;
    struct sockaddr_in address;

    if ((sock=socket(AF_INET, SOCK_STREAM, 0))<0) {
        printf("create connecting_socket: %s\n", strerror(errno));
        return -1;
    }

    bzero((void*)&address, sizeof(struct sockaddr_in));
    address.sin_family=AF_INET;
    address.sin_port=htons(port);
    address.sin_addr.s_addr=host;

    res=connect(sock, (struct sockaddr*)&address, sizeof(struct sockaddr_in));
    if (res==0) return sock;

    printf("connect inputsocket: %s\n", strerror(errno));
    close(sock);
    return -1;
}
/******************************************************************************/
static int
connect_insock()
{
    int sock;

    if (intype==intype_stdin) return -1; /* cannot 'reconnect' stdin */

    sock=create_connecting_socket(inhostaddr, inport);
    return sock;
}
/******************************************************************************/
static void
main_loop(void)
{
    struct event_buffer eb;
    int inpath;

    memset(&eb, 0, sizeof(struct event_buffer));

    if (intype==intype_stdin)
        inpath=0;
    else
        inpath=-1;

    while (1) {
        int nfds, idx, need_data, i, res;
        struct timeval to, *timeout;

        if (inpath<0) {
            inpath=connect_insock();
        }
        if (inpath<0) return;

        do {
            if (read_event(inpath, &eb)<0) {
                if (inpath>0) close(inpath);
                inpath=-1;
            } else {
                if (proceed_event(&eb)<0) return;
            }
            if ((count>=0) && (!--count)) return;
        } while (inpath>=0);
    }
}
/******************************************************************************/
static int
portname2portnum(char* name)
{
    struct servent* service;
    int port;

    if ((service=getservbyname(name, "tcp"))) {
        port=ntohs(service->s_port);
    } else {
        char* end;
        port=strtol(name, &end, 0);
        if (*end!=0) {
            printf("cannot convert \"%s\" to integer.\n", name);
            port=-1;
        }
    }
    return port;
}
/******************************************************************************/
main(int argc, char *argv[])
{
    char *p;

    if (readargs(argc, argv)) return 1;

    printf("inname=%s\n", inname);

    if (strcmp(inname, "-")==0) {
        intype=intype_stdin;
    } else if ((p=index(inname, ':'))) {
        struct hostent *host;
        char hostname[1024];
        int idx=p-inname;
        intype=intype_sock;
        strncpy(hostname, inname, idx);
        hostname[idx]=0;
        if ((inport=portname2portnum(p+1))<0) return 1;
        host=gethostbyname(hostname);
        if (host!=0) {
            inhostaddr=*(unsigned int*)(host->h_addr_list[0]);
        } else {
            inhostaddr=inet_addr(hostname);
            if (inhostaddr==-1) {
                printf("unknown host: %s\n", hostname);
                return 1;
            }
        }
    } else {
        printf("invalid socket name %s\n", inname);
        return 1;
    }

    printf("using ");
    switch (intype) {
    case intype_stdin:
        printf("stdin");
        break;
    case intype_sock:
        printf("%d.%d.%d.%d:%d",
                inhostaddr&0xff, (inhostaddr>>8)&0xff,
                (inhostaddr>>16)&0xff, (inhostaddr>>24)&0xff, inport);
        break;
    }
    printf(" for input\n");

    signal (SIGPIPE, sigpipe);

    main_loop();
}
/******************************************************************************/
/******************************************************************************/
