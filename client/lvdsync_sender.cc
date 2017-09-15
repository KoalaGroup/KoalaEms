/*
 * lvdsync_sender.cc
 * 
 * created: 2007-02-28
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <readargs.hxx>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <versions.hxx>

#include "/usr/local/sis1100/include/dev/pci/sis1100_var.h"

VERSION("2007-03-01", __FILE__, __DATE__, __TIME__,
"$ZEL: lvdsync_sender.cc,v 2.2 2007/04/11 12:05:37 wuestner Exp $")
#define XVERSION

#define LVD_HWtyp(id) (((id)>>4)&0xf)
#define LVD_HWver(id) ((id)&0xf)
#define LVD_FWver(id) (((id)>>8)&0xff)

enum  lvd_cardid {
    LVD_CARDID_CONTROLLER_GPX   = 0,
    LVD_CARDID_CONTROLLER_F1    = 1,
    LVD_CARDID_SLAVE_CONTROLLER = 2,
    LVD_CARDID_F1               = 3,
    LVD_CARDID_VERTEX           = 4,
    LVD_CARDID_SYNCH_MASTER     = 5,
    LVD_CARDID_SYNCH_OUTPUT     = 6,
    LVD_CARDID_FQDC             = 7,
    LVD_CARDID_GPX              = 8,
    LVD_CARDID_SQDC             = 9
};

int mask;
int module;
int p;
// addresses see ems/server/lowlevel/lvd/lvd_map.h
//           and ems/server/lowlevel/lvd/lvd_sync_master_map.h
bool use_mmap;
int base_offs;
C_readargs *args;
unsigned long int delaymagic;

//----------------------------------------------------------------------------
int readargs()
{
    args->addoption("dev", 0, "/tmp/sis1100_01", "device", "dev");
    args->addoption("module", "module", 0x100, "module", "module");
    args->addoption("mask", "mask", 0xf, "mask", "mask");
    args->addoption("interval", "i", 1, "interval in microseconds", "interval");
    args->addoption("mmap", "mmap", false, "use mmap", "mmap");

    return args->interpret();
}
//----------------------------------------------------------------------------
static int
prim_delay(void)
{
    int ver;
    if (ioctl(p, SIS1100_DRIVERVERSION, &ver)<0) {
        cerr<<"SIS1100_DRIVERVERSION: "<<strerror(errno)<<endl;
        return -1;
    }
    return ver;
}
//----------------------------------------------------------------------------
static int
delay_10us(void)
{
    int vv=0;
    for (unsigned long int i=0; i<delaymagic; i++) {
        vv+=prim_delay();
    }
    return vv;
}
//----------------------------------------------------------------------------
static int
delay_10us(int n) // in units of 10 us!
{
    int vv=0;
    for (int i=0; i<n; i++) {
        vv+=delay_10us();
    }
    return vv;
}
//----------------------------------------------------------------------------
static int
delay_ms(int n)
{
    int vv=0;
    for (int i=0; i<n; i++) {
        vv+=delay_10us(100);
    }
    return vv;
}
//----------------------------------------------------------------------------
static int
open_device(const char *devname)
{
    p=open(devname, O_RDWR, 0);
    if (p<0) {
        cerr<<"open "<<devname<<": "<<strerror(errno)<<endl;
        return -1;
    }
    return p;
}
//----------------------------------------------------------------------------
static int
readreg2(int offs, ems_u32 *val)
{
    if (use_mmap) {
    } else {
        struct sis1100_vme_req req;
        req.size=2;
        req.am=-1;
        req.addr=offs;
        req.data=0;
        req.error=0;
        if (ioctl(p, SIS3100_VME_READ, &req)<0) {
            cerr<<"read 0x"<<hex<<offs<<dec<<": "<<strerror(errno)<<endl;
            return -1;
        }
        if (req.error) {
            cerr<<"read 0x"<<hex<<offs<<": 0x"<<req.error<<dec<<endl;
            return -1;
        }
        *val=req.data;
    }
    return 0;
}
//----------------------------------------------------------------------------
static int
writereg2(int offs, ems_u32 val)
{
    //cerr<<hex<<"write 0x"<<val<<" to 0x"<<offs<<dec<<endl;
    if (use_mmap) {
    } else {
        struct sis1100_vme_req req;
        req.size=2;
        req.am=-1;
        req.addr=offs;
        req.data=val;
        req.error=0;
        if (ioctl(p, SIS3100_VME_WRITE, &req)<0) {
            cerr<<"write 0x"<<hex<<offs<<dec<<": "<<strerror(errno)<<endl;
            return -1;
        }
        if (req.error) {
            cerr<<"write 0x"<<hex<<offs<<": 0x"<<req.error<<dec<<endl;
            return -1;
        }
    }
    return 0;
}
//----------------------------------------------------------------------------
static int
writereg_blind2(int offs, ems_u32 val)
{
    if (use_mmap) {
    } else {
        struct sis1100_vme_req req;
        req.size=2;
        req.am=-1;
        req.addr=offs;
        req.data=val;
        if (ioctl(p, SIS3100_VME_WRITE, &req)<0) {
            cerr<<"write 0x"<<hex<<offs<<dec<<": "<<strerror(errno)<<endl;
            return -1;
        }
    }
    return 0;
}
//----------------------------------------------------------------------------
static int
check_module(void)
{
    base_offs=0x100*module;
    ems_u32 val;

    if (readreg2(base_offs, &val)<0)
        return -1;
    if (LVD_HWtyp(val)!=LVD_CARDID_SYNCH_MASTER) {
        cerr<<"ID=0x"<<hex<<setfill('0')<<setw(4)<<val<<setfill(' ')<<dec<<endl;
        cerr<<"Module is not a synchronisation master"<<endl;
        return -1;
    }

    return 0;
}
//----------------------------------------------------------------------------
static int
senderloop(void)
{
    int offs_cr;
    ems_u32 on, off;

    offs_cr=base_offs+0xc;
    if (readreg2(offs_cr, &off)<0)
        return -1;
    off&=~(mask<<4);
    on=off|(mask<<4);
    while (1) {
        writereg2(offs_cr, on);
        writereg2(offs_cr, off);
        delay_10us(10);
    }
    return 0;
}
//----------------------------------------------------------------------------
static void
calibrate_delay(void)
{
    struct timeval a, b;
    double t; /* time in s */
    double q;
    int target=1000;
    int count=0;

    delaymagic=10;
    do {
        gettimeofday(&a, 0);
        delay_ms(1000); // try to delay 1s
        gettimeofday(&b, 0);
        t=static_cast<double>(b.tv_sec-a.tv_sec)*1000.+
            (static_cast<double>(b.tv_usec-a.tv_usec))/1000.;
        q=target/t;
        delaymagic=static_cast<unsigned long int>(delaymagic*q);
        if (count<10) {
            printf("a=(%ld, %ld)\n", a.tv_sec, a.tv_usec);
            printf("b=(%ld, %ld)\n", b.tv_sec, b.tv_usec);
            printf("t=%f s, new magic=%ld\n", t, delaymagic);
        }
    } while (((q<.990)||(q>1.100))&&(count++<1000));
    printf("calibrate_delay: magic=%ld (after %d loops)\n",
            delaymagic, count);
}
//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    args=new C_readargs(argc, argv);

    if (readargs()!=0)
        return 1;

    if (open_device(args->getstringopt("dev"))<0)
        return 2;

    mask=args->getintopt("mask");
    module=args->getintopt("module");
    use_mmap=args->getboolopt("mmap");

    if (check_module()<0)
        return 3;

    calibrate_delay();
    delaymagic*=2;
    senderloop();

    close(p);
    return 0;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
