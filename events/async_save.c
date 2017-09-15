/*
 * async_save.c
 * 
 * created 2011-08-26 PW
 * $ZEL: async_save.c,v 1.2 2011/09/01 19:56:02 wuestner Exp $
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include "normsock.h"

static int loglevel=0;
static int no_v4=0;
static int no_v6=0;
static int objsize=0;
static int nhours=24;
static char *logname=0;
static FILE *logfile;
static FILE *log=0;
static const char* startdir;
static int path=-1;
static int basedir;
static time_t last_hour=0;

static struct sockname  *socknames=0;
static struct sockdescr *insockets=0;
static struct sockdescr *listensockets=0;

static volatile int int_received=0;

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

static const char dirformat[]="%G/W%V";
static const char filformat[]="%G/W%V/%Y%m%dT%H%M%SZ";

/******************************************************************************/
static void
printusage(char* argv0)
{
    eLOG("usage: %s [-h] [-4] [-s size] [-q] [-v] [-d] [-l <logname>] "
                 "[-n hours] -i <inport> directory\n", argv0);
    eLOG("  -h: this help\n");
    eLOG("  -4: use IPv4 only\n");
    eLOG("  -s: objectsize (1 or 4)\n");
    eLOG("  -i: <[host]:port>: address for data input\n");
    eLOG("  -n: start a new file every n hours (default: 24)\n");
    eLOG("  -l <filename>: logfile\n");
    eLOG("  -q: quiet (no informational output)\n");
    eLOG("  -v: verbose (disables quiet)\n");
    eLOG("  -d: debug (disables quiet, enables verbose)\n");
    eLOG("  directory: the files will be created here\n");
}
/******************************************************************************/
static int
readargs(int argc, char* argv[])
{
    int c, err;
    err=0;

    while (!err && ((c=getopt(argc, argv, "hqvd64i:n:l:s:")) != -1)) {
        switch (c) {
        case 'h': printusage(argv[0]); return 1;
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
        case 'n': nhours=atoi(optarg); break;
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
        default: err=1;
        }
    }
    if (err || argc-optind!=1 || objsize==0) {
        printusage(argv[0]);
        return -1;
    }

    startdir=argv[optind];

    return 0;
}
/*****************************************************************************/
static int
seek_to_end(int path)
{
    struct stat stat;
    u_int32_t size;
    off_t last, next;
    off_t ores;
    int res;

    if (fstat(path, &stat)<0) {
        eLOG("stat old file: %s\n", strerror(errno));
        return -1;
    }
    vLOG("size of file: %lld\n", (long long)stat.st_size);

    last=next=0;
    do {
        ores=lseek(path, next, SEEK_SET);
        if (ores!=next) {
            eLOG("seek to %lld: %s\n", (long long)next, strerror(errno));
            break;
        }
        res=read(path, &size, 4);
        if (res!=4) {
            eLOG("read old file: %s\n", strerror(errno));
            break;
        }
        size=ntohl(size); /* this is either in bytes or words */
        size*=objsize;    /* this is the size of paylod in bytes */
        size=((size+3)/4)*4; /* and now rounded up to a multiple of 4 */
        last=next;
        next+=size+4;     /* we have to add the size of the counter
                             because we do an absolute seek */
    } while (next<stat.st_size);

    if (next==stat.st_size) {
        /* end of file exactly reached */
        vLOG("will seek to end of file\n");
        ores=lseek(path, 0, SEEK_END);
    } else {
        /* error before end of file; use last valid offset */
        vLOG("will seek to %lld\n", (long long)last);
        ores=lseek(path, last, SEEK_SET);
    }
    if (ores==(off_t)-1) {
        eLOG("final seek: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}
/*****************************************************************************/
static int
makedir(int basedir, char* path, mode_t mode)
{
    char *p0, *p1;

    p0=path;
    if (*p0=='/')
        p0++;

#if 0
    p1=strchr(p0, '/');
    while (p1) {
        *p1=0;
        if (mkdirat(basedir, path, mode)<0) {
            if (errno!=EEXIST) {
                eLOG("cannot create dir \"%s\": %s\n", path, strerror(errno));
                *p1='/';
                return -1;
            }
        }
        *p1='/';
        p0=p1+1;
        p1=strchr(p0, '/');
    }
    if (strlen(p0)) {
        if (mkdirat(basedir, path, mode)<0) {
            if (errno!=EEXIST) {
                eLOG("cannot create dir \"%s\": %s\n", path, strerror(errno));
                return -1;
            }
        }
    }
#else
    do {
        p1=strchr(p0, '/');
        if (p1)
            *p1=0;
        if (strlen(p0)) {
            if (mkdirat(basedir, path, mode)<0) {
                if (errno!=EEXIST) {
                    eLOG("cannot create dir \"%s\": %s\n",
                            path, strerror(errno));
                    if (p1)
                        *p1='/';
                    return -1;
                }
            }
        }
        if (p1) {
            *p1='/';
            p0=p1+1;
        }
    
    } while (p1);
#endif

    return 0;
}
/******************************************************************************/
static int
prepare_file(void)
{
    struct timeval tv;
    struct tm *tm;
    time_t hour, begin;
    char dirname[1024];
    char filename[1024];

    /* calculate the begin of the current time interval of 'nhours' */
    gettimeofday(&tv, 0);
    hour=tv.tv_sec/3600;
    begin=(hour-(hour%nhours))*3600;

    /* check whether the current file belongs to the current interval */
    dLOG("path=%d begin=%ld last_hour=%ld\n", path, begin, last_hour);
    if (begin==last_hour && path>=0)
        return 0;

    /* close the old file */
    close(path);
    path=-1;

    tm=gmtime(&begin);

    /* create the directory */
    strftime(dirname, 1023, dirformat, tm);
    if (makedir(basedir, dirname, 0755)<0)
        return -1;

    /* create or open the file */
    strftime(filename, 1023, filformat, tm);
    path=openat(basedir, filename, O_CREAT|O_EXCL|O_RDWR, 0644);
    if (path<0) {
        if (errno==EEXIST) {
            path=openat(basedir, filename, O_RDWR, 0644);
            if (path<0) {
                eLOG("open file \"%s\": %s\n", filename, strerror(errno));
                return -1;
            }
            if (seek_to_end(path)<0) {
                close(path);
                path=-1;
                return -1;
            }
        }
    }
    last_hour=begin;

    return 0;
}
/******************************************************************************/
static int
store_data(struct data_descr *descr)
{
    ssize_t sres;

    if (prepare_file()<0)
        return 0;

    sres=write(path, descr->opaque, descr->size);
    if (sres!=descr->size) {
        eLOG("write: %s\n", strerror(errno));
        return -1;
    }

    /* free the data */
    free(descr->opaque);
    free(descr);

    return 0;
}
/******************************************************************************/
static void
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
        LOG(0, "select: %s\n", strerror(errno));
        return;
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
                store_data(sock->ddescr);
                sock->ddescr=0;
            }
        }
        sock=sock->next;
    }
}
/******************************************************************************/
static void
sighand(int sig)
{
    int_received=1;
}
/******************************************************************************/
int
main(int argc, char *argv[])
{
    struct sockname *sockname;
    struct sockdescr *sockets;
    struct sigaction sa;
    int res;

    log=stderr;

    if ((res=readargs(argc, argv)))
        return res<0?1:0;

    normsock_set_loginfo(log, loglevel);
    normsock_set_ip_info(no_v4, no_v6);

    /* install handlers for SIGINT and SIGTERM signal */
    int_received=0;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sighand;
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        eLOG("Could not install SIGINT handler.\n");
        return 2;
    }
    if (sigaction(SIGTERM, &sa, NULL) < 0) {
        eLOG("Could not install SIGTERM handler.\n");
        return 2;
    }
    /* install an ignore handler for SIGPIPE. */
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) < 0) {
        eLOG("Could not install SIGPIPE handler.\n");
        return 2;
    }

    basedir=open(startdir, O_DIRECTORY);
    if (basedir<0) {
        eLOG("Could not open startdir \"%s\": %s\n", startdir, strerror(errno));
        return 2;
    }

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

    while (!int_received) {
        do_select();
    }

    dLOG("main loop finished\n");

    close(path);

    return 0;
}
/******************************************************************************/
/******************************************************************************/
