/*
 * iseg_enumerate.cc
 * 
 * created: 2006-Nov-27 PW
 */

#include <iostream>
#include <cerrno>
#include <readargs.hxx>
#include <versions.hxx>

#include "iseg_ems.hxx"

VERSION("2006-Nov-27", __FILE__, __DATE__, __TIME__,
"$ZEL: iseg_enumerate.cc,v 2.1 2006/11/28 22:32:31 wuestner Exp $")
#define XVERSION

C_readargs* args;

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
    args->hints(C_readargs::required, "vedname", 0);
    args->hints(C_readargs::exclusive, "hostname", "sockname", 0);
    args->hints(C_readargs::exclusive, "port", "sockname", 0);
    if (args->interpret()<0)
        return -1;

    return 0;
}
//---------------------------------------------------------------------------//
static int
dump_crate(struct iseg_ems_crate *crate)
{
    float val;
    int fans, ac, dc;
    bool on;

    cout<<hex<<setfill('0')
        <<"addr 0x"<<crate->addr
        <<" sub_id 0x"<<setw(3)<<crate->sub_id
        <<" emcy_id 0x"<<setw(3)<<crate->emcy_id
        <<setfill(' ')<<dec
        <<" serial "<<setw(4)<<crate->serial
        <<endl;

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
name_module(struct iseg_ems_module *module)
{
    cout<<setw(2)<<module->addr
        <<" class="<<module->mclass
        <<" ser="<<module->serial
        <<" fw="<<module->firmware
        <<" channels="<<module->channels
        <<" chanmask=0x"<<hex<<module->equipped_channels<<dec<<endl;
    return 0;
}
//---------------------------------------------------------------------------//
static int
dump_module(struct iseg_ems_module *module)
{
    float u, i, ramp;
    float u_set, i_set;
    float Vp24, Vp15, Vp5, Vn15, Vn5, T;
    unsigned int status;

    cout<<setw(2)<<module->addr
        <<" class="<<module->mclass
        <<" ser="<<module->serial
        <<" fw="<<module->firmware
        <<" channels="<<module->channels
        <<" chanmask=0x"<<hex<<module->equipped_channels<<dec<<endl;
    //cout<<"module limit  : u: "<<module->u_max_mod<<" V"
    //    <<" i: "<<module->i_max_mod*1000.<<" mA"<<endl;
    iseg_ems_current_limit(module, &i);
    iseg_ems_voltage_limit(module, &u);
    cout<<"hardware limit: u: "<<u*100.<<"% i: "<<i*100.<<"%"<<endl;
    iseg_ems_ramp(module, &ramp);
    cout<<"ramp time: "<<ramp<<" s"<<endl;
    iseg_ems_supply_voltages(module, &Vp24, &Vp15, &Vp5, &Vn15, &Vn5, &T);
    cout<<"supply: Vp24="<<Vp24<<", Vp15="<<Vp15<<", Vp5="<<Vp5
        <<", Vn15="<<Vn15<<", Vn5="<<Vn5<<", T="<<T<<endl;
    iseg_ems_status_module(module, &status);
    cout<<"status: 0x"<<hex<<status<<dec<<endl;
    if (module->channels_identical)
        cout<<"all channels are identical"<<endl;

    cout<<"channels:"<<endl;
    for (int c=0; c<module->channels; c++) {
        iseg_ems_voltage(module, c, &u);
        iseg_ems_voltage_set(module, c, &u_set);
        iseg_ems_current(module, c, &i);
        iseg_ems_current_set(module, c, &i_set);
        iseg_ems_status_channel(module, c, &status);
        cout<<setw(2)<<c<<":"<<endl;
        cout<<"  max u: "<<module->u_max[c]<<" V"
            <<" set u: "<<u_set<<" V"
            <<" actual u: "<<u<<" V"<<endl;
        cout<<"  max i: "<<module->i_max[c]*1000.<<" mA"
            <<" set i: "<<i_set*1000.<<" mA"
            <<" actual i: "<<i*1000.<<" mA"<<endl;
        cout<<"status: 0x"<<hex<<status<<dec<<endl;
    }
    return 0;
}
//---------------------------------------------------------------------------//
int
main(int argc, char* argv[])
{
    struct iseg_ems_crate *crates;
    int num_crates;

    struct iseg_ems_module *modules;
    int num_modules;

    void* ved_handle;
    int bus;

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
    bus=args->getintopt("bus");

    num_crates=iseg_ems_enumerate_crates(ved_handle, bus, &crates);
    if (num_crates>=0) {
        cout<<num_crates<<" crate"<<(num_crates==1?"":"s")<<" found"<<endl;
        for (int i=0; i<num_crates; i++) {
            if (iseg_ems_setup_crate(crates+i)<0)
                break;
            dump_crate(crates+i);
            cout<<endl;
        }
    } else {
        cout<<"iseg_ems_enumerate_crates returns -1"<<endl;
    }

    num_modules=iseg_ems_enumerate(ved_handle, bus, &modules);
    if (num_modules>=0) {
        cout<<num_modules<<" module"<<(num_modules==1?"":"s")<<" found"<<endl;
        for (int i=0; i<num_modules; i++) {
            if (iseg_ems_setup_module(modules+i)<0)
                return 0;
        }
        for (int i=0; i<num_modules; i++) {
            name_module(modules+i);
        }
        cout<<endl;
        for (int i=0; i<num_modules; i++) {
            dump_module(modules+i);
            cout<<endl;
        }
    } else {
        cout<<"iseg_ems_enumerate returns -1"<<endl;
    }
    
    return 0;
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
