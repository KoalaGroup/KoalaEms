/*
 * ems/wago/wago2shm.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: wago2shm.c,v 1.3 2011/08/13 19:37:02 wuestner Exp $";

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
//#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/mman.h>

#define DEFPORT 22224
#define DEFHOST "ikpapb.ikp.kfa-juelich.de"

static int port;
static char *hostname;
static char *mapfile;
static char *logfile;
static key_t sem_key;
static int sem_id;
static FILE *log=0;

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
    fprintf(log, "usage: %s"
        " [-h host]"
        " [-p port]"
        " [-m mapfile]"
        " [-l logfile]"
        "\n", argv0);
}
/*****************************************************************************/
static int
decode_args(int argc, char* argv[])
{
    extern char *optarg;
    extern int optind;
    extern int opterr;
    extern int optopt;

    char* args="h:p:m:l:";
    int c, errflg=0;

    port=DEFPORT;
    hostname=DEFHOST;
    mapfile=0;
    logfile=0;

    optarg=0;
    while (!errflg && ((c=getopt(argc, argv, args))!=-1)) {
        switch (c) {
        case 'h':
            hostname=optarg;
            break;
        case 'p':
            port=atoi(optarg);
            break;
        case 'm':
            mapfile=optarg;
            break;
        case 'l':
            logfile=optarg;
            log=fopen(logfile, "a");
            if (!log) {
                log=stderr;
                fprintf(log, "open logfile \"%s\": %s\n",
                        logfile, strerror(errno));
                return -1;
            }
            break;
        default :
            errflg++;
        }
    }
    if (errflg)
        return -1;

    if (!mapfile) {
        fprintf(log, "mapfile must be given.\n");
        return -1;
    }

    fprintf(log, "using host:port %s:%d\n", hostname, port);

    return 0;
}
/******************************************************************************/
static int
xread(int p, char* b, int s)
{
int r, d=0;
while (d<s)
  {
  r=read(p, b+d, s-d);
  if (r<0)
    {
    fprintf(log, "read: %s\n", strerror(errno));
    return -1;
    }
  if (r==0)
    {
    fprintf(log, "broken pipe.\n");
    return -1;
    }
  d+=r;
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
if (semop(sem_id, &sops, 1)<0)
  {
  fprintf(log, "semop(-1): %s\n", strerror(errno));
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
  fprintf(log, "semop(+1): %s\n", strerror(errno));
}
/******************************************************************************/
static int
fill_file(int fd, size_t size)
{
    char null=0;
    if (lseek(fd, size-1, SEEK_SET)==-1) {
        fprintf(stderr, "lseek to %llu: %s\n", (unsigned long long)(size-1),
                strerror(errno));
        return -1;
    }
    if (write(fd, &null, 1)!=1) {
        fprintf(stderr, "write at %llu: %s\n", (unsigned long long)(size-1),
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
            fprintf(stderr, "open \"%s\": %s\n",
                    map->filename, strerror(errno));
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
        fprintf(stderr, "mmap \"%s\": %s\n",
                map->filename, strerror(errno));
        return -1;
    }
    fprintf(log, "mapped %llu bytes at %p\n",
            (unsigned long long)size, map->addr);
    

    return 0;
}
/******************************************************************************/
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
/******************************************************************************/
static int
do_read(int s)
{
    static u_int32_t *buf=0;
    static int bufsize=0;
    u_int32_t len;
    int i;

    if (xread(s, (char*)&len, sizeof(len))<0)
        return -1;
    len=ntohl(len);
    if (bufsize<len) {
        free(buf);
        buf=(u_int32_t*)malloc(len*sizeof(u_int32_t));
        if (!buf) {
            fprintf(log, "malloc: %s\n", strerror(errno));
            return -1;
        }
    }
    if (xread(s, (char*)buf, sizeof(u_int32_t)*len)<0)
        return -1;
    for (i=0; i<len; i++)
        buf[i]=ntohl(buf[i]);
    do_write(buf, len);

    return 0;
}
/******************************************************************************/
static int
init_sem(void)
{
union semun semarg;

sem_id=semget(sem_key, 1/*nsems*/, IPC_CREAT|0666 /*semflg*/);
if (sem_id<0)
  {
  fprintf(log, "semget: %s\n", strerror(errno));
  return -1;
  }

semarg.val=1;
if (semctl(sem_id, 0, SETVAL, semarg)<0)
  {
  fprintf(log, "semctl: %s\n", strerror(errno));
  return -1;
  }

return 0;
}
/******************************************************************************/
int
main(int argc, char **argv)
{
    struct hostent *host;
    struct sockaddr_in sa;
    int sock;

    log=stderr;

    if (decode_args(argc, argv)<0) {
        print_help(argv[0]);
        return 1;
    }

    if ((sock=socket(AF_INET, SOCK_STREAM, 0))==-1) {
        fprintf(log, "socket: %s\n", strerror(errno));
        return 2;
    }

    memset((char*)&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family=AF_INET;
    sa.sin_port=htons(port);
    host=gethostbyname(hostname);
    if (host!=0) {
        sa.sin_addr.s_addr= *(u_int*)(host->h_addr_list[0]);
    } else {
        sa.sin_addr.s_addr=inet_addr((char*)hostname);
        if (sa.sin_addr.s_addr==-1) {
            fprintf(log, "cannot resolve hostname \"%s\"\n", hostname);
            return 2;
        }
    }

    if (connect(sock, (struct sockaddr*)&sa, sizeof(struct sockaddr_in))==-1) {
        fprintf(log, "connect: %s\n", strerror(errno));
        return 2;
    }

    map.filename=mapfile;
    map.addr=0;
    if (map_file(&map, sizeof(struct mem))<0)
        return 2;
    mem=(struct mem*)map.addr;
    fprintf(log, "mem=%p len=%llu\n", mem, (unsigned long long)map.length);

    sem_key=ftok(mapfile, proj_id);
    if (sem_key==-1) {
        fprintf(log, "ftok: %s\n", strerror(errno));
        return 1;
    }

    if (init_sem()<0)
        return 2;

    while (do_read(sock)>=0);

    return 0;
}
/******************************************************************************/
/******************************************************************************/
