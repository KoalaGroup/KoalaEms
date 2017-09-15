/*
 * client/read_iseg_state.cc
 * 
 * created: 2006-Nov-20 PW
 */

#include "config.h"
#include <iostream>
#include <cstdio>
#include <fstream>
#include <cerrno>
#include <sys/time.h>
#include <time.h>
#include <versions.hxx>

#include <readargs.hxx>
#include "mkpath.hxx"
#include "iseg_ems.hxx"

VERSION("2006-Nov-20", __FILE__, __DATE__, __TIME__,
"$ZEL: read_iseg_state.cc,v 1.1 2006/11/27 10:56:39 wuestner Exp $")
#define XVERSION

#define DEFUSOCK "/var/tmp/emscomm"
#define DEFISOCK 4096

C_readargs* args;
int last_day;
struct iseg_ems_module *modules;
int nr_modules;

FILE* fdump;

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
    args->addoption("vedname", "ved", "iseg", "name of the VED", "vedname");
    args->addoption("bus", "bus", 0, "CANbus", "CANbus");
    args->addoption("interval", "interval", 1.0, "Interval", "interval");
    args->addoption("dumppath", "dumppath", ".", "dump directory", "dumpdir");
    //args->hints(C_readargs::required, "vedname", 0);
    args->hints(C_readargs::exclusive, "hostname", "sockname", 0);
    args->hints(C_readargs::exclusive, "port", "sockname", 0);
    return args->interpret();
}
/*****************************************************************************/
static int
dump_timestamp()
{
    char ss[20];
    struct timeval now;
    struct tm *tm;
    time_t tt;
    double dsec;

    gettimeofday(&now, 0);
    dsec=now.tv_sec+now.tv_usec/1000000.;
    tt=now.tv_sec;
    tm=gmtime(&tt);
    strftime(ss, 1000, "%FT%T", tm);

    fprintf(fdump, "T %sZ %f\n", ss, dsec);
    return 0;
}
/*****************************************************************************/
static int
dump_header()
{
    fprintf(fdump, "# addr class serial channels    Umax    Imax\n");
    for (int mod=0; mod<nr_modules; mod++) {
        struct iseg_ems_module *m=modules+mod;
        fprintf(fdump, "H   %02d     %d %06d        %d %7.2f %7.2f\n",
            m->addr, m->mclass, m->serial, m->channels,
            m->u_max[0], m->i_max[0]*1000.);
    }
    return 0;
}
/*****************************************************************************/
static int
read_and_dump_module(struct iseg_ems_module *m)
{
    unsigned int status;
    float val;

// module status
    iseg_ems_status_module(m, &status);
    fprintf(fdump, "Mstate %02d: 0x%02x\n", m->addr, status);

// status of all channels
    fprintf(fdump, "Cstate %02d:", m->addr);
    for (int chan=0; chan<m->channels; chan++) {
        iseg_ems_status_channel(m, chan, &status);
        fprintf(fdump, "  0x%04x", status);
    }
    fprintf(fdump, "\n");

// voltage of all channels
    fprintf(fdump, "Cvolt  %02d:", m->addr);
    for (int chan=0; chan<m->channels; chan++) {
        iseg_ems_voltage(m, chan, &val);
        fprintf(fdump, " %7.2f", val);
    }
    fprintf(fdump, "\n");

// current of all channels
    fprintf(fdump, "Ccurr  %02d:", m->addr);
    for (int chan=0; chan<m->channels; chan++) {
        iseg_ems_current(m, chan, &val);
        fprintf(fdump, " %7.2f", val*1000.);
    }
    fprintf(fdump, "\n");

    return 0;
}
/*****************************************************************************/
static void
wait_interval(int msec)
{
    struct timeval now;
    ems_u64 usec_now, usec_next, usec=msec*1000, usectowait;
    struct timespec rqt, rmt;
    int again;

    gettimeofday(&now, 0);
    usec_now=now.tv_sec;
    usec_now*=1000000;
    usec_now+=now.tv_usec;
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
                std::cerr<<"nanosleep: "<<strerror(errno)<<std::endl;
            }
        }
    } while (again);
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

    if ((!fdump) || (tm->tm_mday!=last_day)) {
        int p;
        if (fdump) {
            fclose(fdump);
            fdump=0;
        }
        strftime(fname, 1000, "iseghv-%F", tm);
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
                    std::cerr<<"create "<<fname_<<": "<<strerror(errno)
                            <<std::endl;
                    return -1;
                }
            }
        } while (again);
        fdump=fdopen(p, "w");
        last_day=tm->tm_mday;
        dump_header();
    }
    return 0;
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
    void* ved_handle;
    int interval, bus;

    args=new C_readargs(argc, argv);
    if (readargs())
        return 1;

    interval=static_cast<int>(args->getrealopt("interval")*1000);

    const char* dir=args->getstringopt("dumppath");
    if (makedir(dir, 0755)<0) return -1;
    if (chdir(dir)<0) {
        std::cerr<<"chdir("<<dir<<"): "<<strerror(errno)<<std::endl;
        return -1;
    }

    if (!args->isdefault("hostname") || !args->isdefault("port")) {
        if (iseg_ems_init_communication(args->getstringopt("hostname"),
                args->getintopt("port"))<0)
            return 1;
    } else {
        if (iseg_ems_init_communication(args->getstringopt("sockname"))<0)
            return 1;
    }
    if ((ved_handle=iseg_ems_open_is(args->getstringopt("vedname")))==0)
        return 2;
    bus=args->getintopt("bus");


    nr_modules=iseg_ems_enumerate(ved_handle, bus, &modules);
    for (int mod=0; mod<nr_modules; mod++) {
        if (iseg_ems_setup_module(modules+mod)<0)
            return 3;
    }

    while (1) {
        open_file();
        dump_timestamp();
        for (int mod=0; mod<nr_modules; mod++) {
            read_and_dump_module(modules+mod);
        }
        fflush(fdump);
        wait_interval(interval);
    }



    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
