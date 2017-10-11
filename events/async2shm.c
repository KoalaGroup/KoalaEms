/*
 * ems/events/async2shm.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: async2shm.c,v 1.3 2013/10/30 18:22:51 wuestner Exp $";

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include "normsock.h"

static char *mapfile=0;
static key_t sem_key;
static int sem_id;
static int loglevel=0;
static int no_v4=0;
static int no_v6=0;
static int objsize=0;
static char *logname=0;
static FILE *logfile;
static FILE *log=0;

static struct sockname  *socknames=0;
static struct sockdescr *insockets=0;
static struct sockdescr *listensockets=0;

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

struct map {
    char *filename;
    void *addr;
    size_t length;
    int prot;
    int flags;
    int fd;
};
static struct map map;

struct mem {
    int sequence;
    int valid;
    u_int32_t size;
    u_int32_t data[1];
};

static struct mem* mem;

static const int proj_id=31; /* arbitrary constant for ftok */

#ifdef __linux
union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO
                                (Linux specific) */
};
#endif

/*****************************************************************************/
static void
print_help(char* argv0)
{
    eLOG("usage: %s [-h] [-4] [-s size] [-q] [-v] [-d] [-l <logname>] "
            "-m mapfile\n", argv0);
    eLOG("  -h: this help\n");
    eLOG("  -4: use IPv4 only\n");
    eLOG("  -s: objectsize (1 or 4)\n");
    eLOG("  -i: <[host]:port>: address for data input\n");
    eLOG("  -l <filename>: logfile\n");
    eLOG("  -q: quiet (no informational output)\n");
    eLOG("  -v: verbose (disables quiet)\n");
    eLOG("  -d: debug (disables quiet, enables verbose)\n");
    eLOG("  -m mapfile\n");
}
/*****************************************************************************/
static int
decode_args(int argc, char* argv[])
{
    char* args="h46s:i:qvdl:m:";
    int c, errflg=0;

    while (!errflg && ((c=getopt(argc, argv, args))!=-1)) {
        switch (c) {
        case 'h':
            print_help(argv[0]);
            return 1;
        case 'q':
            if (loglevel>0 && loglevel<10) /* >10 is forbidden */
                loglevel++;
            else
                loglevel=1;
            break;
        case 'v':
            if (loglevel<0)
                loglevel--;
            else
                loglevel=-1;
            break;
        case 'd':
            if (loglevel<0)
                loglevel--;
            else
                loglevel=-2;
            break;
        case '4': no_v6=1; break;
        case '6': no_v4=1; break;
        case 's': objsize=atoi(optarg); break;
        case 'i': {
                struct sockname *sockname;
                sockname=calloc(1, sizeof(struct sockname));
                sockname->next=socknames;
                sockname->name=optarg;
                sockname->maxqueue=0;
                sockname->inout=0;
                socknames=sockname;
            }
            break;
        case 'm':
            mapfile=optarg;
            break;
        case 'l':
            logname=optarg;
            logfile=fopen(logname, "a");
            if (!logfile) {
                eLOG("open logfile \"%s\": %s\n",
                        logname, strerror(errno));
                return -1;
            }
            log=logfile;
            break;
        default :
            errflg++;
        }
    }
    if (errflg || argc!=optind || !mapfile) {
        print_help(argv[0]);
        return -1;
    }

    return 0;
}
/******************************************************************************/
static int
get_lock(void)
{
    struct sembuf sops;

    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=SEM_UNDO;
    if (semop(sem_id, &sops, 1)<0) {
        eLOG("semop(-1): %s\n", strerror(errno));
        return -1;
    }
    return 0;
}
/******************************************************************************/
static void
release_lock(void)
{
    struct sembuf sops;

    sops.sem_num=0;
    sops.sem_op=1;
    sops.sem_flg=SEM_UNDO;
    if (semop(sem_id, &sops, 1)<0)
        eLOG("semop(+1): %s\n", strerror(errno));
}
/******************************************************************************/
static int
fill_file(int fd, size_t size)
{
    char null=0;
    if (lseek(fd, size-1, SEEK_SET)==-1) {
        eLOG("lseek to %llu: %s\n", (unsigned long long)(size-1),
                strerror(errno));
        return -1;
    }
    if (write(fd, &null, 1)!=1) {
        eLOG("write at %llu: %s\n", (unsigned long long)(size-1),
                strerror(errno));
        return -1;
    }
    return 0;
}
/******************************************************************************/
static int
map_file(struct map *map, size_t size)
{
    int pagesize=getpagesize();

    /* because pagesize is a power of 2 the next line rounds size up to
       the next page boundery */
    size=(size+pagesize-1)&~(pagesize-1);

    if (map->addr) { /* we have to remap */
        if (fill_file(map->fd, size)<0)
            return -1;
        map->addr=mremap(map->addr, map->length, size, MREMAP_MAYMOVE);
        map->length=size;
    } else {    /* we need a new mapping */
        map->fd=open(map->filename, O_RDWR|O_CREAT, 0644);
        if (map->fd<0) {
            eLOG("open \"%s\": %s\n", map->filename, strerror(errno));
            return -1;
        }
        if (fill_file(map->fd, size)<0)
            return -1;
        map->prot=PROT_WRITE|PROT_READ;
        map->flags=MAP_SHARED;
        map->length=size;
        map->addr=mmap(0, map->length, map->prot, map->flags, map->fd, 0);
    }
    if (map->addr==MAP_FAILED) {
        eLOG("mmap \"%s\": %s\n", map->filename, strerror(errno));
        return -1;
    }
    dLOG("mapped %llu bytes at %p\n", (unsigned long long)size, map->addr);

    return 0;
}
/******************************************************************************/
#if 0
static int
do_write(u_int32_t* buf, int len)
{
    int res=-1, i;

    if (get_lock()<0)
        return -1;

    if (len+sizeof(struct mem)>map.length) {
        if (map_file(&map, len+sizeof(struct mem))<0)
            goto error;
        mem=(struct mem*)map.addr;
    }

    mem->size=len;
    for (i=0; i<len; i++)
        mem->data[i]=buf[i];
    mem->sequence++;
    mem->valid=1;
    if (msync(mem, map.length, MS_SYNC|MS_INVALIDATE)<0) {
        fprintf(log, "msync: %s\n", strerror(errno));
        goto error;
    }
    res=0;

error:
    release_lock();
    return res;
}
#else
static int
do_write(struct data_descr *descr)
{
    /* descr->size is given in bytes! (but is always a multiple of 4) */
    int len=descr->size/4;
    u_int32_t *buf=(u_int32_t*)descr->opaque;
    int i, res=-1;

    if (get_lock()<0)
        return -1;

    if (descr->size+sizeof(struct mem)>map.length) {
        if (map_file(&map, descr->size+sizeof(struct mem))<0)
            goto error;
        mem=(struct mem*)map.addr;
    }

/*
 *  The first word of buf counts the amount of following data objects.
 *  In case of objsize==4 we can remove it because mem->size can be
 *  used instead. sock_read will not return empty blocks, thus we know
 *  that it is safe to decrement len.
 */
    if (objsize==4) {
        len--;
        buf++;
    }

    mem->size=len;
    for (i=0; i<len; i++)
        mem->data[i]=ntohl(buf[i]);
    mem->sequence++;
    mem->valid=1;
    if (msync(mem, map.length, MS_SYNC|MS_INVALIDATE)<0) {
        eLOG("msync: %s\n", strerror(errno));
        goto error;
    }
    res=0;

error:
    release_lock();
    /* free the data */
    free(descr->opaque);
    free(descr);
    return res;
}
#endif
/******************************************************************************/
static int
init_sem(void)
{
    union semun semarg;

    sem_id=semget(sem_key, 1/*nsems*/, IPC_CREAT|0666 /*semflg*/);
    if (sem_id<0) {
        eLOG("semget: %s\n", strerror(errno));
        return -1;
    }

    semarg.val=1;
    if (semctl(sem_id, 0, SETVAL, semarg)<0) {
        eLOG("semctl: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}
/******************************************************************************/
static int
do_select(void)
{
    struct sockdescr *sock;
    fd_set rfds, wfds;
    int nfds=0, res;

    FD_ZERO(&rfds);
    FD_ZERO(&wfds);

    /* insert all listening sockets */
    sock=listensockets;
    while (sock) {
        if (sock->p>=0) {
            FD_SET(sock->p, &rfds);
            if (sock->p>nfds)
                nfds=sock->p;
        }
        sock=sock->next;
    }
    /* insert all insockets */
    sock=insockets;
    while (sock) {
        if (sock->p>=0) {
            FD_SET(sock->p, &rfds);
            if (sock->p>nfds)
                nfds=sock->p;
        }
        sock=sock->next;
    }
    nfds++;

    res=select(nfds, &rfds, &wfds, 0, 0);
    if (res<0) {
        eLOG("select: %s\n", strerror(errno));
        return -1;
    }

    /* accept new connections */
    sock=listensockets;
    while (sock) {
        if (sock->p>=0 && FD_ISSET(sock->p, &rfds))
            accept_socket(sock, &insockets);
        sock=sock->next;
    }

    /* read data */
    sock=insockets;
    while (sock) {
        if (sock->p>=0 && FD_ISSET(sock->p, &rfds)) {
            res=sock_read(sock);
            if (res<0) {
                close_socket(sock);
            } else if (res>0) {
                res=do_write(sock->ddescr);
                sock->ddescr=0;
                if (res<0)
                    return -1;
            }
        }
        sock=sock->next;
    }
    return 0;
}
/******************************************************************************/
int
main(int argc, char **argv)
{
    struct sockname *sockname;
    struct sockdescr *sockets;
    int res;

    log=stderr;

    if ((res=decode_args(argc, argv)))
        return res<0?1:0;

    normsock_set_loginfo(log, loglevel);
    normsock_set_ip_info(no_v4, no_v6);

    /* open all sockets */
    sockets=0;
    sockname=socknames;
    while (sockname) {
        if (open_socket(sockname, &sockets, objsize)<0) {
            eLOG("open sockets failed\n");
            return 2;
        }
        sockname=sockname->next;
    }

    /* sort the sockets to different lists */
    insockets=0;
    listensockets=0;
    while (sockets) {
        struct sockdescr *next=sockets->next;
        if (sockets->listening) {
            sockets->next=listensockets;
            listensockets=sockets;
        } else {
            sockets->next=insockets;
            insockets=sockets;
        }
        sockets=next;
    }

    map.filename=mapfile;
    map.addr=0;
    if (map_file(&map, sizeof(struct mem))<0)
        return 2;
    mem=(struct mem*)map.addr;
    dLOG("mem=%p len=%llu\n", mem, (unsigned long long)map.length);

    sem_key=ftok(mapfile, proj_id);
    if (sem_key==-1) {
        eLOG("ftok: %s\n", strerror(errno));
        return 1;
    }

    if (init_sem()<0)
        return 2;

    while (do_select()>=0);

    return 0;
}
/******************************************************************************/
/******************************************************************************/
