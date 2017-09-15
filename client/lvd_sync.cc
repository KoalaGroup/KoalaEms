/*
 * lvd_sync.cc
 * 
 * created: 2006-Sep-01 PW
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

VERSION("2006-Sep-01", __FILE__, __DATE__, __TIME__,
"$ZEL: lvd_sync.cc,v 2.4 2006/09/03 17:03:03 wuestner Exp $")
#define XVERSION

#define DEFUSOCK "/var/tmp/emscomm"
#define DEFISOCK 4096

struct runstatus {
    InvocStatus state;
    ems_u32 evnr;
};

C_readargs* args;
C_VED** ved;
C_VED* vme;
struct runstatus *status;
int numveds, interval;
ems_u32 mask;
bool nosync;

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
    args->addoption("interval", "interval", 100, "interval in s", "interval");
    args->addoption("vmename", "vme", "scaler", "name of the VME-VED", "vmename");
    args->addoption("mask", "mask", 0x10, "mask for sis3100", "mask");
    args->addoption("nosync", "nosync", false, "do not resynchronize", "no");
    args->hints(C_readargs::exclusive, "hostname", "sockname", 0);
    args->hints(C_readargs::exclusive, "port", "sockname", 0);
    if (args->interpret()<0)
        return -1;
    interval=args->getintopt("interval");
    numveds=args->numnames();
    mask=args->getintopt("mask");
    nosync=args->getboolopt("nosync");
    return 0;    
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
static int
set_evnr(ems_u32 evnr)
{
    for (int i=0; i<numveds; i++) {
        C_instr_system* is=ved[i]->is0();

        is->clear_command();
        is->command("lvd_eventcount");
        is->add_param(2, 0, evnr);
        delete is->execute();
    }
}
/*****************************************************************************/
static int
read_status()
{
    int running=0;

    for (int i=0; i<numveds; i++) {
        status[i].state=ved[i]->GetReadoutStatus(&status[i].evnr);
        if (status[i].state==Invoc_active)
            running++;
    }
    return running;
}
/*****************************************************************************/
static int
start_trigger()
{
    C_confirmation *conf;
    C_instr_system* is=vme->is0();

    is->clear_command();
    is->command("sis3100_write");
    is->add_param(4, 0, 1, 0x80, mask<<16);
    delete is->execute();
    return 0;
}
/*****************************************************************************/
static int
stop_trigger()
{
    C_confirmation *conf;
    ems_i32 *buf, flags;
    C_instr_system* is=vme->is0();

    is->clear_command();
    is->command("sis3100_read");
    is->add_param(3, 0, 1, 0x80);
    conf=is->execute();
    buf=conf->buffer();
    flags=buf[1];
    delete conf;
    if (flags&mask) {
        return 1;
    }

    is->clear_command();
    is->command("sis3100_write");
    is->add_param(4, 0, 1, 0x80, mask);
    delete is->execute();
    return 0;
}
/*****************************************************************************/
static void
print_status()
{
    struct timeval tv;
    time_t tt;
    struct tm *tm;
    char ts[1024];
    InvocStatus state;
    ems_u32 evnr;
    int differs=0;

    gettimeofday(&tv, 0);
    tt=tv.tv_sec;
    tm=gmtime(&tt);
    strftime(ts, 1024, "%FT%TZ", tm);

    cout<<ts<<": ";

    state=status[0].state;
    evnr=status[0].evnr;
    for (int i=1; i<numveds; i++) {
        if ((status[i].state!=state) || (status[i].evnr!=evnr))
            differs=1;
    }

    if (differs) {
        cout<<"status mismatch"<<endl;
        for (int i=0; i<numveds; i++) {
            const char* s;
            switch (status[i].state) {
            case Invoc_error:     s="error  "; break;
            case Invoc_alldone:   s="done   "; break;
            case Invoc_stopped:   s="stopped"; break;
            case Invoc_notactive: s="idle   "; break;
            case Invoc_active:    s="running"; break;
            default:              s="illegal";
            }
            cout<<"  "<<ved[i]->name()<<" "<<s<<" "<<status[i].evnr<<endl;
        }   
    } else {
        const char* s;
        switch (state) {
        case Invoc_error:     s="error"; break;
        case Invoc_alldone:   s="done"; break;
        case Invoc_stopped:   s="stopped"; break;
        case Invoc_notactive: s="idle"; break;
        case Invoc_active:    s="running"; break;
        default:              s="illegal";
        }
        cout<<s<<" "<<evnr<<"  ";
    }
}
/*****************************************************************************/
static int
synchronize()
{
    ems_u32 evnr;
    int first=1, unsynced=0;

    if (stop_trigger()>0) {
        cout<<"trigger blocked by another process"<<endl;
        return 0;
    }

    read_status();
    for (int i=0; i<numveds; i++) {
        if (status[i].state==Invoc_active) {
            if (first) {
                evnr=status[i].evnr;
                first=0;
            } else {
                if (evnr!=status[i].evnr)
                    unsynced=1;
                if (evnr<status[i].evnr)
                    evnr=status[i].evnr;
            }
        }
    }
    if (nosync) {
        print_status();
    } else {
        if (!unsynced) {
            print_status();
            cout<<"daq synchronized"<<endl;
        } else {
            print_status();
            cout<<"daq NOT synchronized"<<endl;
            sleep(1);
            set_evnr(evnr);
            cout<<"all crates set to event "<<evnr<<endl;
        }
    }

    start_trigger();
    return 0;
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
    args=new C_readargs(argc, argv);
    if (readargs())
        return 1;

    ved=new C_VED*[numveds];
    status=new struct runstatus[numveds];

    try {
        start_communication();
        vme=new C_VED(args->getstringopt("vmename"));
        if (vme->is0()==0) {
            cerr<<"ved "<<vme->name()<<": no IS 0"<<endl;
            return 2;
        }
        vme->is0()->execution_mode(delayed);

        for (int i=0; i<numveds; i++) {
            ved[i]=new C_VED(args->getnames(i));
            if (ved[i]->is0()==0) {
                cerr<<"ved "<<ved[i]->name()<<": no IS 0"<<endl;
                return 2;
            }
            ved[i]->is0()->execution_mode(delayed);
        }
        while (1) {
            int running;
            running=read_status();
            //print_status();
            if (running>1) {
                synchronize();
            } else {
                print_status();
                cout<<endl;
            }
            sleep(interval);
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
