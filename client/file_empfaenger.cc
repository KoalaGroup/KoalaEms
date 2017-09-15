/*
 * file_empfaenger.cc
 * 
 * created: 03.05.1999
 * 06.05.1999 PW: modified for multiple connections
 */

#define _XOPEN_SOURCE 500
#include "config.h"
#include "cxxcompat.hxx"
#include <unistd.h>
#include <readargs.hxx>
#include <ved_errors.hxx>
#include <proc_communicator.hxx>
#include <proc_ved.hxx>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include "client_defaults.hxx"
#include "mkpath.hxx"
#include <versions.hxx>

VERSION("2007-04-17", __FILE__, __DATE__, __TIME__,
"$ZEL: file_empfaenger.cc,v 2.8 2007/04/18 19:44:51 wuestner Exp $")
#define XVERSION

#define swap_int(a) ( ( static_cast<u_int32_t>(a) << 24) | \
		      ((static_cast<u_int32_t>(a) << 8) & 0x00ff0000) | \
		      ((static_cast<u_int32_t>(a) >> 8) & 0x0000ff00) | \
	              ( static_cast<u_int32_t>(a) >>24) )

enum timesource {
    ts_local,
    ts_file,
    ts_sender
};

C_readargs* args;
int verbose;
const char* filename;
bool overwrite_time;
enum timesource timesource;

struct databuf {
    int size;
    int idx;
    char* buff;
};

struct sockdescr {
    databuf fbuf;
    int path;
};

sockdescr* extrasock;
int numextrasocks;

/*****************************************************************************/
static int
readargs(void)
{
    args->addoption("verbose", "v", false, "verbose");
    args->addoption("dumppath", "dumppath", ".", "dump directory", "dumpdir");
    args->addoption("filename", "fname", "%Y/%B/%d/%H/%M:%S", "filename (strftime())", "fname");
    args->addoption("serverport", "port", 9000, "serverport", "serverport");
    args->addoption("senderhost", "sh", "tac", "hostname of sender", "senderhost");
    args->addoption("senderport", "sp", 9000, "portnumber of sender", "senderport");
    args->addoption("timesource", "ts", "local", "source for timestamp (local|file|sender)", "timesource");
    args->addoption("overwrite_time", "ovt", true, "overwrite time inside file", "ovt");

    return args->interpret(1);
}
/*****************************************************************************/
extern "C" {
void inthand(int sig)
{}
}
/*****************************************************************************/
static int xread(int s, char* buf, int count)
{
    int idx=0, res;
    while (count) {
        res=read(s, buf+idx, count);
        if (res<0) {
            if (errno!=EINTR) {
                syslog(LOG_ERR, "xread: %s", strerror(errno));
                return -1;
            }
        } else if (res==0) {
            syslog(LOG_ERR, "xread: read 0 bytes\n");
            return -1;
        } else {
            idx+=res;
            count-=res;
        }
    }
    return 0;
}
/*****************************************************************************/
static int
read_sync(int sock)
{
    char abcd[16];

    if (xread(sock, abcd, 16)<0)
        return -1;
    if (strncmp(abcd, "ABCDEFGHIJKLMNOP", 16)!=0) {
        syslog(LOG_WARNING, "synchronisation lost, try to resync");
        int count=0;
        do {
            if (xread(sock, abcd, 1)<0)
                return -1;
            if (abcd[0]=='A'+count) {
                count++;
            } else {
                if (abcd[0]=='A')
                    count=1;
                else
                    count=0;
            }
        } while (count<16);
        syslog(LOG_WARNING, "synchronized");
    }
    return 0;
}
/*****************************************************************************/
static int
read_header(int sock, int* version, int* stamp, struct timeval* tv, int* size)
{
    int endian;
    if (read_sync(sock)<0) return -1;
    if (xread(sock, reinterpret_cast<char*>(&endian), sizeof(int))<0) return -1;
    if (xread(sock, reinterpret_cast<char*>(version), sizeof(int))<0) return -1;
    if (xread(sock, reinterpret_cast<char*>(stamp), sizeof(int))<0) return -1;
    if (xread(sock, reinterpret_cast<char*>(&tv->tv_sec), sizeof(int))<0) return -1;
    if (xread(sock, reinterpret_cast<char*>(&tv->tv_usec), sizeof(int))<0) return -1;
    if (xread(sock, reinterpret_cast<char*>(size), sizeof(int))<0) return -1;
    switch (endian) {
        case 0x12345678: break;
        case 0x78563412:
            *version=swap_int(*version);
            *stamp=swap_int(*stamp);
            tv->tv_sec=swap_int(tv->tv_sec);
            tv->tv_usec=swap_int(tv->tv_usec);
            *size=swap_int(*size);
            break;
        default:
            syslog(LOG_ERR, "unknown endian: 0x%08x", endian);
            return -1;
    }
#if 0
    {
        char ss[1024];
        struct tm *tm;
        time_t tt;

        cerr<<"version: "<<*version<<endl;
        cerr<<"size   : "<<*size<<endl;

        tt=*stamp;
        tm=localtime(&tt);
        strftime(ss, 1023, "%Y-%m-%dT%H:%M:%S", tm);
        cerr<<"stamp  : "<<ss<<endl;

        tt=tv->tv_sec;
        tm=localtime(&tt);
        strftime(ss, 1023, "%Y-%m-%dT%H:%M:%S", tm);
        cerr<<"tv     : "<<ss<<endl;
    }
#endif
    return 0;
}
/*****************************************************************************/
static int
write_file(int p, char* buf, int size)
{
    if (write(p, buf, size)!=size) {
        syslog(LOG_ERR, "write file: %s", strerror(errno));
        return -1;
    }
    return 0;
}
/*****************************************************************************/
static int
make_name_and_open_file(time_t timestamp)
{
    char ss[1024];
    struct tm *zeit;
    int p;

    zeit=localtime(&timestamp);
    strftime(ss, 1023, filename, zeit);
    p=makefile(ss, O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if (verbose)
        syslog(LOG_INFO, "%s created", ss);
    return p;
}
/*****************************************************************************/
static void
edit_buf(char* buf, int size, time_t timestamp)
{
    if (buf[24]!=0xa) {
        cerr<<"edit_buf: unexpected format"<<endl;
    }
    struct tm *time;
    time=localtime(&timestamp);
    strftime(buf, 25, "%a %d/%b/%Y %H:%M:%S", time);
    buf[24]=0xa;
}
/*****************************************************************************/
static int
read_and_write_file(int sock)
{
    char* buf;

    int version;
    int stamp;
    struct timeval tv, ltv;
    time_t timestamp;
    int size;

    if (read_header(sock, &version, &stamp, &tv, &size)<0)
        return -1;
    gettimeofday(&ltv, 0);

    buf=new char[size];
    if (buf==0) {
        syslog(LOG_ERR, "new char[%d]: %s", size, strerror(errno));
        return -1;
    }

    if (xread(sock, buf, size)<0)
        return -1;

    switch (timesource) {
    case ts_local:
        timestamp=ltv.tv_sec;
        break;
    case ts_file:
        timestamp=stamp;
        break;
    case ts_sender:
        timestamp=tv.tv_sec;
        break;
    }

    int p;
    p=make_name_and_open_file(timestamp);
    if (p<0) {
        delete[] buf;
        return -1;
    }

    if (overwrite_time)
        edit_buf(buf, size, timestamp);

    write_file(p, buf, size);
    close(p);

    int count=0;
    for (int i=0; i<numextrasocks; i++) {
        if (extrasock[i].fbuf.size==0) {
            if (count) {
                extrasock[i].fbuf.buff=new char[size];
                if (extrasock[i].fbuf.buff==0) {
                    syslog(LOG_ERR, "new char[%d]: %s", size, strerror(errno));
                } else {
                    for (int j=0; j<size; j++)
                        extrasock[i].fbuf.buff[j]=buf[j];
                }
            } else {
                extrasock[i].fbuf.buff=buf;
            }
            extrasock[i].fbuf.size=size;
            extrasock[i].fbuf.idx=0;
            count++;
        }
    }
    if (count==0)
        delete[]buf;

    return(0);
}
/*****************************************************************************/
static int
    do_write(int p, databuf* buffer)
{
    int res;

    if (buffer->size<=0)
        return 0;

    res=write(p, buffer->buff+buffer->idx, buffer->size-buffer->idx);
    if (res<0) {
        if (errno!=EINTR) {
            syslog(LOG_ERR, "write to extrasocket: %s", strerror(errno));
            return -1;
        }
    } else if (res==0) {
        syslog(LOG_ERR, "write to extrasocket: broken pipe");
        return -1;
    } else {
        buffer->idx+=res;
        if (buffer->idx>=buffer->size) {
            buffer->size=0;
            delete[] buffer->buff;
            buffer->buff=0;
        }
    }
    return 0;
}
/*****************************************************************************/
static int
do_accept(int p)
{
    struct sockaddr_in address;
    socklen_t len;
    int ns;

    len=sizeof(struct sockaddr_in);
    ns=accept(p, reinterpret_cast<struct sockaddr*>(&address), &len);
    if (ns<0) {
        syslog(LOG_ERR, "accept: %s", strerror(errno));
        return -1;
    }
    syslog(LOG_INFO, "accepted from %s:%d",
            inet_ntoa(address.sin_addr),
            ntohs(address.sin_port));
    return ns;
}
/*****************************************************************************/
static void
main_loop(int insock, int serversock)
{
    int nfds, i;
    fd_set readfds, writefds;

    extrasock=0;
    numextrasocks=0;

    nfds=insock;
    if (serversock>nfds)
        nfds=serversock;
    nfds++;

    while (1) {
        int res;

        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_SET(serversock, &readfds);
        FD_SET(insock, &readfds);
        for (i=0; i<numextrasocks; i++) {
            if (extrasock[i].fbuf.size)
                FD_SET(extrasock[i].path, &writefds);
        }
        res=select(nfds, &readfds, &writefds, 0, 0);
        if (res<=0) {
            cerr<<"select: res="<<res<<"; : "<<strerror(errno)<<endl;
        }
        if (FD_ISSET(insock, &readfds)) {
            read_and_write_file(insock);
        }
        if (FD_ISSET(serversock, &readfds)) {
            int ns;
            ns=do_accept(serversock);
            if (ns>=0) {
                sockdescr* help=new sockdescr[numextrasocks+1];
                if (help==0) {
                    close(ns);
                    syslog(LOG_ERR, "new sockdescr: %s", strerror(errno));
                } else {
                    for (i=0; i<numextrasocks; i++)
                        help[i]=extrasock[i];
                    delete[] extrasock;
                    extrasock=help;
                    extrasock[numextrasocks].path=ns;
                    extrasock[numextrasocks].fbuf.size=0;
                    extrasock[numextrasocks].fbuf.buff=0;
                    numextrasocks++;
                    if (ns+1>nfds)
                        nfds=ns+1;
                }
            }
        }
        for (i=0; i<numextrasocks; i++) {
            if (FD_ISSET(extrasock[i].path, &writefds))
            if (do_write(extrasock[i].path, &extrasock[i].fbuf)<0) {
                close(extrasock[i].path);
                delete[] extrasock[i].fbuf.buff;
                for (int j=i; j<numextrasocks-1; j++)
                    extrasock[j]=extrasock[j+1];
                numextrasocks--;
            }
        }
    }
}
/*****************************************************************************/
static int
master_loop(void)
{
    tcp_socket sourcesocket("insock");
    int serversocket;

    try {
        sourcesocket.create();
        cerr<<"try connect"<<endl;
        sourcesocket.connect(args->getstringopt("senderhost"),
                args->getintopt("senderport"));
        cerr<<"connected"<<endl;
    } catch(C_error* error) {
        cerr << (*error) << endl;
        delete error;
        return 2;
    }

    struct sockaddr_in address;
    if ((serversocket=socket(AF_INET, SOCK_STREAM, 0))<0) {
        perror("create serversocket");
        return 2;
    }
    bzero(&address, sizeof(struct sockaddr_in));
    address.sin_family=AF_INET;
    address.sin_port=htons(args->getintopt("serverport"));
    address.sin_addr.s_addr=htonl(INADDR_ANY);

    if ((bind(serversocket, reinterpret_cast<struct sockaddr*>(&address),
            sizeof(struct sockaddr_in)))<0) {
        perror("bind serversocket");
        return 3;
    }
    if ((listen(serversocket, 1))<0) {
        perror("listen serversocket");
        return 3;
        }

    main_loop(sourcesocket, serversocket);

    return 0;
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
    struct sigaction vec, ovec;
    openlog(argv[0], LOG_PID|LOG_CONS, LOG_USER);

    args=new C_readargs(argc, argv);
    if (readargs()!=0)
        return 1;
    verbose=args->getboolopt("verbose");
    overwrite_time=args->getboolopt("overwrite_time");
    filename=args->getstringopt("filename");
    const char* ts_str=args->getstringopt("timesource");
    if (strcmp(ts_str, "local")==0) {
        timesource=ts_local;
    } else if (strcmp(ts_str, "file")==0) {
        timesource=ts_file;
    } else if (strcmp(ts_str, "sender")==0) {
        timesource=ts_sender;
    } else {
        cerr<<"illegal value \"<<ts_str<<\" as timesource"<<endl;
        return 1;
    }

    if (sigemptyset(&vec.sa_mask)==-1) {
        cerr << "sigemptyset: " << strerror(errno) << endl;
        return 2;
    }
    vec.sa_flags=0;
    vec.sa_handler=inthand;
    if (sigaction(SIGPIPE, &vec, &ovec)==-1) {
        cerr << "sigaction: " << strerror(errno) << endl;
        return 2;
    }

    const char* dir=args->getstringopt("dumppath");
    if (makedir(dir, 0755)<0)
        return 2;
    if (chdir(dir)<0) {
        cerr<<"chdir("<<dir<<"): "<<strerror(errno)<<endl;
        return 2;
    }

    while (1) {
        master_loop();
        sleep(60);
    }

    syslog(LOG_INFO, "ende");

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
