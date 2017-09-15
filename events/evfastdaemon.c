/*
 * evfastdaemon.c
 * $ZEL: evfastdaemon.c,v 1.2 2004/11/26 14:40:17 wuestner Exp $
 * created 09.Jul.2003 PW
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#ifndef __linux__
#include <sys/timers.h>
#endif
#include <string.h>
#include <sys/param.h>
#include <unistd.h>
#include <math.h>
#include <syslog.h>

#define BUFSIZE 4194304

char* dir;

/*****************************************************************************/
int readargs(int argc, char* argv[])
{
    if (argc<2)
        dir=0;
    else
        dir=argv[1];

    return 0;
}
/*****************************************************************************/

main(int argc, char* argv[])
{
    int fd, ok;
    struct timeval tv;
    struct tm* tm;
    char name[MAXPATHLEN];
    char buffer[BUFSIZE];

    openlog(argv[0], LOG_CONS|LOG_NDELAY, LOG_DAEMON);

    if (readargs(argc, argv)<0) return 1;

    if (dir) {
        gettimeofday(&tv, 0);
        tm=localtime(&tv.tv_sec);
        if (chdir(dir)!=0) {
            syslog(LOG_ERR, "chdir to %s: %s\n", dir, strerror(errno));
            return 1;
        }
        strftime(name, MAXPATHLEN, "%d.%b.%Y_%H:%M:%S_XXXXXX", tm);
        mktemp(name);
        syslog(LOG_WARNING, "%s/%s\n", dir, name);
        if ((fd=open(name, O_WRONLY|O_CREAT|O_EXCL/*|O_LARGEFILE*/, 0644))<0) {
            syslog(LOG_ERR, "open: %s\n", strerror(errno));
            return 2;
        }
    } else {
        syslog(LOG_WARNING, "no output\n", dir, name);
        fd=-1;
    }
    ok=1;

    do {
        int res;
        res=read(0, buffer, BUFSIZE);
        if (res>0) {
            if (fd>=0) {
                int r1=write(fd, buffer, res);
                if (r1!=res) {
                    syslog(LOG_ERR, "write: res=%d; %s\n",
                            r1, strerror(errno));
                    ok=0;
                }
            }
        } else if (res<0) {
            switch (errno) {
            case EINTR: break;
            default:
                syslog(LOG_ERR, "read: res=%d; %s\n",
                        res, strerror(errno));
                ok=0;
            break;
            }
        } else {
            syslog(LOG_WARNING, "no more data\n");
            ok=0;
        }
    } while(ok);
    close(fd);
    closelog();
}
