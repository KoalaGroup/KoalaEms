/*
 * sis3100_pulser.c
 * 2008-Jul-27 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_pulser.c,v 1.2 2011/04/06 20:30:36 wuestner Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dev/pci/sis1100_var.h>
#include <rcs_ids.h>

RCS_REGISTER(cvsid, "trigger/vme/sis3100")

unsigned long int interval;
unsigned long int portmask;
int p, count, debug;
char *dev;

static void
print_usage(const char *argv0)
{
    fprintf(stderr, "usage: %s -i interval -m portmask device\n", argv0);
    fprintf(stderr, "  interval is given in ms\n");
    fprintf(stderr, "  portmask is magic\n");
}

static int
get_int(const char* arg, unsigned long int *val)
{
    char* end;

    *val=strtoul(arg, &end, 0);
    if (*val==0 && (errno!=0 || *end!=0)) {
        fprintf(stderr, "could not convert >%s< to integer");
        if (errno)
            fprintf(stderr, " errno=%s", strerror(errno));
        fprintf(stderr, "\n");
        return -1;
    }
    return 0;
}

static int
do_opts(int argc, char *argv[])
{
    int c, err=0;

    interval=0;
    portmask=0;
    debug=0;
    dev=0;

    optarg = NULL;
    while (!err && ((c=getopt(argc, argv, "hi:m:d"))!=-1)) {
        switch (c) {
        case 'h':
            err=1;
            break;
        case 'i':
            if (get_int(optarg, &interval)<0)
                err=1;
            break;
        case 'm':
            if (get_int(optarg, &portmask)<0)
                err=1;
            portmask&=0xfff;
            break;
        case 'd':
            debug=1;
            break;
        default :
            err=1;
        }
    }
    fprintf(stderr, "optind=%d, argc=%d\n", optind, argc);
    if (optind>=argc) {
        err=1;
    } else {
        dev=argv[optind];
    }

    if (interval==0 || portmask==0)
        err=1;

    if (err) {
        print_usage(argv[0]);
        return -1;
    }
    fprintf(stderr, "interval=%d, mask=0x%x dev=%s\n", interval, portmask, dev);
    return 0;
}

static int
open_dev(void)
{
    p=open(dev, O_RDWR, 0);
    if (p<0)
        fprintf(stderr, "open \"%s\": %s\n", dev, strerror(errno));
    return p;
}

static void
handler(int sig)
{
    //fprintf(stderr, "handler called\n");
    count++;
}

int
main(int argc, char *argv[])
{
    struct itimerval itval;
    struct sigaction action;
    sigset_t mask;

    if (do_opts(argc, argv)<0)
        return 1;

    if (open_dev()<0)
        return 2;

    sigemptyset(&mask);

    action.sa_handler=handler;
    action.sa_mask=mask;
    action.sa_flags=SA_RESTART;
    if (sigaction(SIGALRM, &action, 0)<0) {
        fprintf(stderr, "sigaction: %s\n", strerror(errno));
        return 3;
    }

    itval.it_interval.tv_sec=interval/1000;
    itval.it_interval.tv_usec=(interval%1000)*1000;
    itval.it_value.tv_sec=itval.it_interval.tv_sec;
    itval.it_value.tv_usec=itval.it_interval.tv_usec;

    if (setitimer(ITIMER_REAL, &itval, 0)<0) {
        fprintf(stderr, "setitimer: %s\n", strerror(errno));
        return 4;
    }

    count=0;
    while (1) {
        u_int32_t v;
        sigsuspend(&mask);
        if (debug)
            fprintf(stderr, "geweckt! count=%d\n", count);

        if (debug) {
            if (count&1)
                v=portmask;
            else
                v=portmask<<16;
                if(ioctl(p, SIS1100_FRONT_IO, &v)<0) {
                    fprintf(stderr, "ioctl(FRONT_IO, 0x%08x): %s\n",
                            v, strerror(errno));
                        return 5;
                }
        } else {
            v=portmask;
            if(ioctl(p, SIS1100_FRONT_IO, &v)<0) {
                fprintf(stderr, "ioctl(FRONT_IO, 0x%08x): %s\n",
                        v, strerror(errno));
                    return 5;
            }
            v=portmask<<16;
            if(ioctl(p, SIS1100_FRONT_IO, &v)<0) {
                fprintf(stderr, "ioctl(FRONT_IO, 0x%08x): %s\n",
                        v, strerror(errno));
                    return 5;
            }
        }
    }

    return 0;
}
