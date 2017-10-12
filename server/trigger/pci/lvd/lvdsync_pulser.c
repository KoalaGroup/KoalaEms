/*
 * lvdsync_pulser.c
 * 2009-Jun-13 PW
 * $ZEL$
 */

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

#include "/home/wuestner/drivers/dev/pci/sis1100_var.h"
#include "../../../lowlevel/lvd/lvd_sync_master_map.h"

unsigned long int interval;
unsigned long int aux_mask;
u_int32_t id, ser, cr, aux_bits;
int p, addr, count;
char *dev;

static void
print_usage(const char *argv0)
{
    fprintf(stderr, "usage: %s -a addr -i interval -m portmask device\n", argv0);
    fprintf(stderr, "  interval is given in ms\n");
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
    unsigned long int help;
    int c, err=0;

    interval=0;
    aux_mask=0;
    dev=0;

    optarg = NULL;
    while (!err && ((c=getopt(argc, argv, "ha:i:m:"))!=-1)) {
        switch (c) {
        case 'h':
            err=1;
            break;
        case 'i':
            if (get_int(optarg, &interval)<0)
                err=1;
            break;
        case 'm':
            if (get_int(optarg, &aux_mask)<0)
                err=1;
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

    fprintf(stderr, "addr=%d interval=%d, mask=0x%x dev=%s\n",
            addr, interval, aux_mask, dev);

    if (interval==0 || aux_mask==0)
        err=1;

    if (err) {
        print_usage(argv[0]);
        return -1;
    }
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

static int
sis1100_lvd_write(int p, int mod_addr, int reg_addr, int size, u_int32_t data)
{
    struct sis1100_vme_req req;
    int res, addr;

    addr=mod_addr*0x80+reg_addr;

    req.size=size;
    req.am=-1;
    req.addr=addr;
    req.data=data;
    res=ioctl(p, SIS3100_VME_WRITE, &req);
    if (res) {
        printf("sis1100_lvd_write (0x%04x) size=%d 0x%x: %s\n",
                addr, size, data, strerror(errno));
        return -1;
    }
    if (req.error) {
        printf("sis1100_lvd_write  (0x%04x) size=%d 0x%x: error=0x%x\n",
                addr, size, data, req.error);
        return -1;
    }

    return 0;
}

static int
sis1100_lvd_read(int p, int mod_addr, int reg_addr, int size, u_int32_t* data)
{
    struct sis1100_vme_req req;
    int res, addr;

    addr=mod_addr*0x80+reg_addr;

    req.size=size;
    req.am=-1;
    req.addr=addr;
    req.data=0;
    res=ioctl(p, SIS3100_VME_READ, &req);
    if (res) {
        printf("lvd_read (0x%04x) size=%d: %s\n",
                addr, size, strerror(errno));
        return -1;
    }
    if (req.error) {
        printf("lvd_read (0x%04x) size=%d: error=0x%x\n",
                addr, size, req.error);
        return -1;
    }

    *data=req.data;
    return 0;
}

static int
set_outputs(int p, unsigned long int mask)
{
    u_int32_t xcr=cr;

    xcr|=(mask<<6)&0xc0;
    sis1100_lvd_write(p, addr, 0xc, 2, xcr);
}

static int
reset_outputs(int p, unsigned long int mask)
{
    u_int32_t xcr=cr;

    xcr&=~((mask<<6)&0xc0);
    sis1100_lvd_write(p, addr, 0xc, 2, xcr);
}

int
main(int argc, char *argv[])
{
    struct itimerval itval;
    struct sigaction action;
    sigset_t mask;
    int i;

    if (do_opts(argc, argv)<0)
        return 1;

    if (open_dev()<0)
        return 2;

    /* find master sync module */
    for (i=0, addr=-1; i<16 && addr<0; i++) {
        if (sis1100_lvd_read(p, i, 0, 2, &id))
            continue;
        switch (id&0xf8) {
        case 0x50: /* SYNCH_MASTER */
            addr=i;
            aux_bits=MSYNC_CR_AUX;
            break;
        case 0x58: /* SYNCH_MASTER2 */
            addr=i;
            aux_bits=MSYNC2_CR_AUX_OUT;
            break;
        default:
            continue;
        }        
    }
    if (addr<0) {
        printf("no master sync module found\n");
        return 3;
    }
    sis1100_lvd_read(p, addr, 2, 2, &ser);
    printf("found %s master sync module at address %x, ser %d\n",
        id&0x8?"new":"old", addr, ser);

    /* calculate toggle mask */
    if (!aux_bits) {
        printf("aux_bits is zero\n");
        return 4;
    }
    i=0;
    while (!((aux_bits>>i)&1))
        i++;
    aux_mask=(aux_mask<<i)&aux_bits;
    printf("aux_mask=0x%x\n", aux_mask);

    if (sis1100_lvd_read(p, addr, 0xc, 2, &cr)<0)
        return 4;

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
        u_int32_t v=cr;
        sigsuspend(&mask);

        v^=aux_mask;
        sis1100_lvd_write(p, addr, 0xc, 2, v);
        v^=aux_mask;
        sis1100_lvd_write(p, addr, 0xc, 2, v);
    }

    return 0;
}
