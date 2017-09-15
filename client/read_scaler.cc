/*
 * read_scaler.cc
 * 
 * created: 2006-Aug-20 PW
 */

#include "config.h"
#include <iostream>
#include <cmath>
#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <versions.hxx>
#include <readargs.hxx>
#include <ved_errors.hxx>
#include <errors.hxx>
#include <proc_communicator.hxx>
#include <proc_is.hxx>
#include <conststrings.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <modultypes.h>
#include "mkpath.hxx"

VERSION("2006-Nov-06", __FILE__, __DATE__, __TIME__,
"$ZEL: read_scaler.cc,v 1.6 2007/04/10 00:01:05 wuestner Exp $")
#define XVERSION

#define DEFUSOCK "/var/tmp/emscomm"
#define DEFISOCK 4096

C_readargs* args;
C_VED* ved;
C_instr_system *is;
int ems_var;
int latch_mask, p;
int last_day;

ems_u64 timestamp; /* usec */
ems_u32 nr_modules;
ems_u32 nr_channels[32]; /* no more than 32 modules...*/
ems_u64 val[32][32]; /* val[module][channel] */

/*****************************************************************************/
static int
readargs(void)
{
    args->addoption("hostname", "h", "localhost",
        "host running commu", "hostname");
    args->use_env("hostname", "EMSHOST");
    args->addoption("port", "p", DEFISOCK,
        "port for connection with commu", "port");
    args->use_env("port", "EMSPORT");
    args->addoption("sockname", "s", DEFUSOCK,
        "socket for connection with commu", "socket");
    args->use_env("sockname", "EMSSOCKET");
    args->addoption("vedname", "ved", "scaler", "name of the VED", "vedname");
    args->addoption("is", "is", 1, "Instrumentation system", "is");
    args->addoption("ems_var", "var", 1, "ems variable", "var");
    args->addoption("latch_mask", "latchmask", 0x0f, "latch mask for sis3100", "lmask");
    args->addoption("interval", "interval", 1.0, "Interval", "interval");
    args->addoption("dumppath", "dumppath", ".", "dump directory", "dumpdir");
    //args->hints(C_readargs::required, "vedname", 0);
    args->hints(C_readargs::exclusive, "hostname", "sockname", 0);
    args->hints(C_readargs::exclusive, "port", "sockname", 0);
    return args->interpret();
}
/*****************************************************************************/
static void
start_communication(void)
{
    if (!args->isdefault("hostname") || !args->isdefault("port")) {
        communication.init(args->getstringopt("hostname"),
                args->getintopt("port"));
    } else {
        communication.init(args->getstringopt("sockname"));
    }
}
/*****************************************************************************/
static void
open_ved(void)
{
    ved=new C_VED(args->getstringopt("vedname"));
}
/*****************************************************************************/
static void
open_is(void)
{
    is=new C_instr_system(ved, args->getintopt("is"), C_instr_system::open);
    is->execution_mode(delayed);
}
/*****************************************************************************/
static int
open_file(void)
{
    char fname[1000], fname_[1024];
    struct timeval now;
    struct tm *tm;
    time_t tt;
    int again, idx=0;

    gettimeofday(&now, 0);
    tt=now.tv_sec;
    tm=gmtime(&tt);

    if ((p<0) || (tm->tm_mday!=last_day)) {
        if (p>=0)
            close(p);
        strftime(fname, 1000, "scaler-%F", tm);
        strncpy(fname_, fname, 1024);
        do {
            again=0;
            p=open(fname_, O_WRONLY|O_CREAT|O_EXCL, 0644);
            if (p<0) {
                if (errno==EEXIST) {
                    idx++;
                    snprintf(fname_, 1024, "%s.%d", fname, idx);
                    again=1;
                } else {
                    cerr<<"create "<<fname_<<": "<<strerror(errno)<<endl;
                    return -1;
                }
            }
        } while (again);
        last_day=tm->tm_mday;
    }
    return 0;
}
/*****************************************************************************/
static int
read_scaler(void)
{
    C_confirmation* conf;
    ems_u32* buf;

    is->clear_command();
    is->command("Timestamp");
    is->add_param(1, 1);
    is->command("sis3800ShadowUpdateAll");
    is->add_param(2, ems_var, latch_mask);
    conf=is->execute();
    buf=reinterpret_cast<ems_u32*>(conf->buffer());
    #if 0
    for (int i=0; i<conf->size(); i++) {
        printf("%d ", buf[i]);
    }
    printf("\n");
    #endif

    if (buf[0]) {
        cerr<<"error in EMS procedure: "<<buf[0]<<endl;
        delete conf;
        return -1;
    }
    buf++;

    timestamp=*buf++;   /* sec */
    timestamp*=1000000; /* converted to usec */
    timestamp+=*buf++;  /* usec */
    nr_modules=*buf++;
    for (unsigned int mod=0; mod<nr_modules; mod++) {
        nr_channels[mod]=*buf++;
        for (unsigned int chan=0; chan<nr_channels[mod]; chan++) {
            ems_u64 v;
            v=*buf++;
            v<<=32;
            v+=*buf++;
            val[mod][chan]=v;
        }
    }
    delete conf;
    return 0;
}
/*****************************************************************************/
#if 0
static int
dump_scaler(void)
{
    printf("timestamp: sec=%d usec=%d\n", tv.tv_sec, tv.tv_usec);
    printf("%d module\n", nr_modules);
    for (int i=0; i<nr_modules; i++) {
        printf("%d:", i);
        for (int j=0; j<nr_channels[i]; j++) {
            printf(" %u", val[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}
#endif
/*****************************************************************************/
static int
write_scaler(void)
{
    static const ems_u32 magic=0x12345678;
    open_file();
    if (p<0)
        return -1;

    write(p, &magic, 4);
    write(p, &timestamp, 8);
    write(p, &nr_modules, 4);
    for (unsigned int i=0; i<nr_modules; i++) {
        write(p, &nr_channels[i], 4);
        for (unsigned int j=0; j<nr_channels[i]; j++) {
            write(p, &val[i][j], 8);
        }
    }
    return 0;
}
/*****************************************************************************/
static void
wait_interval(int tsec)
{
    /* nsec ist ignored */
    struct timeval now;
    ems_u64 usec_now, usec_next, usec=tsec*100000, usectowait;
    struct timespec rqt, rmt;
    int again;

    gettimeofday(&now, 0);
    usec_now=static_cast<ems_u64>(now.tv_sec)*1000000
            +static_cast<ems_u64>(now.tv_usec);
    usec_next=usec_now+usec;
    usec_next-=usec_next%usec;
    while (usec_next<=usec_now)
        usec_next+=usec;

    usectowait=usec_next-usec_now;
    rqt.tv_sec=usectowait/1000000;
    rqt.tv_nsec=(usectowait%1000000)*1000;
    do {
        again=0;
        if (nanosleep(&rqt, &rmt)<0) {
            if (errno==EINTR) {
                again=1;
                rqt=rmt;
            } else {
                cerr<<"nanosleep: "<<strerror(errno)<<endl;
            }
        }
    } while (again);
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
    float interval;
    int tsec;

    args=new C_readargs(argc, argv);
    if (readargs())
        return 1;

    interval=args->getrealopt("interval");
    tsec=static_cast<int>(interval*10.);

    const char* dir=args->getstringopt("dumppath");
    if (makedir(dir, 0755)<0) return -1;
    if (chdir(dir)<0) {
        cerr<<"chdir("<<dir<<"): "<<strerror(errno)<<endl;
        return -1;
    }

    ems_var=args->getintopt("ems_var");
    latch_mask=args->getintopt("latch_mask");

    p=-1;

    try {
        start_communication();
        open_ved();
        open_is();
        while (1) {
            if (read_scaler()==0) {
                /* dump_scaler(); */
                write_scaler();
            }
            wait_interval(tsec);
        }
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return 2;
    }

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
