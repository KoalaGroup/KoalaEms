/*
 * iseg_crate.cc
 * 
 * created: 2006-Nov-25 PW
 */

#include <iostream>
#include <cerrno>
#include <readargs.hxx>
#include <versions.hxx>

#include "iseg_ems.hxx"

VERSION("2006-Nov-25", __FILE__, __DATE__, __TIME__,
"$ZEL: iseg_crate.cc,v 1.2 2006/11/27 13:01:12 wuestner Exp $")
#define XVERSION

C_readargs* args;

int bus;
struct iseg_ems_crate *crates;
int num_crates;

bool on, off;

using namespace std;

//---------------------------------------------------------------------------//
static int
readargs(void)
{
    args->addoption("hostname", "h", "localhost",
        "host running commu", "hostname");
    args->use_env("hostname", "EMSHOST");
    args->addoption("port", "p", 4096,
        "port for connection with commu", "port");
    args->use_env("port", "EMSPORT");
    args->addoption("sockname", "s", "/var/tmp/emscomm",
        "socket for connection with commu", "socket");
    args->use_env("sockname", "EMSSOCKET");
    args->addoption("vedname", "ved", "", "name of the VED", "vedname");
    args->addoption("bus", "bus", 0, "Idx of CANbus", "canbus");
    args->addoption("on", "on", false, "switch crate on", "on");
    args->addoption("off", "off", false, "switch crate off", "off");
    args->hints(C_readargs::required, "vedname", 0);
    args->hints(C_readargs::exclusive, "hostname", "sockname", 0);
    args->hints(C_readargs::exclusive, "port", "sockname", 0);
    args->hints(C_readargs::exclusive, "on", "off", 0);
    if (args->interpret()<0)
        return -1;

    on=args->getboolopt("on");
    off=args->getboolopt("off");
    bus=args->getintopt("bus");

    num_crates=args->numnames();
    if (num_crates)
        crates=new struct iseg_ems_crate[num_crates];
    for (int i=0; i<num_crates; i++) {
        char *end;
        const char *s=args->getnames(i);
        errno=0;
        crates[i].sub_id=strtol(s, &end, 0);
        if ((crates[i].sub_id==0) && ((errno!=0)||(end!=s+strlen(s)))) {
            cerr<<"cannot convert "<<s<<" to ineger"<<endl;
            return -1;
        }
    }

    return 0;
}
//---------------------------------------------------------------------------//
static int
enumerate_crates(void* ved_handle)
{
    num_crates=iseg_ems_enumerate_crates(ved_handle, bus, &crates);
    if (num_crates<0)
        return -1;
    for (int i=0; i<num_crates; i++) {
        iseg_ems_setup_crate(crates+i);
        cout<<hex<<setfill('0')
            <<"addr 0x"<<crates[i].addr
            <<" sub_id 0x"<<setw(3)<<crates[i].sub_id
            <<" emcy_id 0x"<<setw(3)<<crates[i].emcy_id
            <<setfill(' ')<<dec
            <<" serial "<<setw(4)<<crates[i].serial
            <<endl;
    }
    return 0;
}
//---------------------------------------------------------------------------//
static int
crate_status(struct iseg_ems_crate *crate)
{
    float val;
    int fans, ac, dc;
    bool on;

    iseg_ems_setup_crate(crate);
    cout<<"serial no.: "<<crate->serial<<" firmware "<<crate->firmware<<endl;

    iseg_ems_crate_on(crate, &on);
    cout<<"Crate is "<<(on?"on":"off")<<endl;

    iseg_ems_crate_voltage(crate, 0, &val);
    cout<<" +24: "<<val<<" V"<<endl;
    iseg_ems_crate_voltage(crate, 1, &val);
    cout<<"  +5: "<<val<<" V"<<endl;
    iseg_ems_crate_voltage(crate, 2, &val);
    cout<<"Batt: "<<val<<" V"<<endl;

    iseg_ems_crate_fans(crate, 3, &fans, &val);
    cout<<"fans: 0x"<<hex<<fans<<dec<<endl;
    cout<<"T1: "<<val<<" C"<<endl;
    iseg_ems_crate_fans(crate, 3, &fans, &val);
    cout<<"T2: "<<val<<" C"<<endl;

    if (iseg_ems_crate_power(crate, &ac, &dc)==0)
        cout<<hex<<"AC: 0x"<<ac<<" DC: 0x"<<dc<<dec<<endl;
    return 0;
}
//---------------------------------------------------------------------------//
static int
do_crate(struct iseg_ems_crate *crate)
{
    if (on||off) {
        bool _on;
        iseg_ems_crate_on(crate, &_on);
        cout<<"Crate was    "<<(_on?"on":"off")<<endl;
        cout<<"switching it "<<(on?"on":"off")<<endl;
        iseg_ems_crate_on(crate, on);
        iseg_ems_crate_on(crate, &_on, 2000);
        cout<<"Crate is now "<<(_on?"on":"off")<<endl;
    } else {
        crate_status(crate);
    }
    return 0;
}
//---------------------------------------------------------------------------//
int
main(int argc, char* argv[])
{
    void* ved_handle;

    args=new C_readargs(argc, argv);
    if (readargs())
        return 1;

    if (!args->isdefault("hostname") || !args->isdefault("port")) {
        if (iseg_ems_init_communication(args->getstringopt("hostname"),
                args->getintopt("port"))<0)
            return 2;
    } else {
        if (iseg_ems_init_communication(args->getstringopt("sockname"))<0)
            return 2;
    }
    if ((ved_handle=iseg_ems_open_is(args->getstringopt("vedname")))==0)
        return 2;

    if (num_crates==0) {
        enumerate_crates(ved_handle);
    } else {
        for (int i=0; i<num_crates; i++) {
            crates[i].ved_handle=ved_handle;
            crates[i].bus=bus;
        }
        for (int i=0; i<num_crates; i++) {
            do_crate(crates+i);
        }
    }
    
    return 0;
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
