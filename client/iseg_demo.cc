/*
 * iseg_demo.cc
 * 
 * created: 2006-Sep-26 PW
 */

//#include "config.h"
#include <iostream>
//#include <cerrno>
#include <versions.hxx>

#include "iseg_ems.hxx"

VERSION("2006-Nov-24", __FILE__, __DATE__, __TIME__,
"$ZEL: iseg_demo.cc,v 1.2 2006/11/27 11:03:36 wuestner Exp $")
#define XVERSION

const int bus=0;
const char *hostname="zelas3";
const int port=4096;
const char *vedname="q21";

using namespace std;

/*****************************************************************************/
int
main(int argc, char* argv[])
{
    void* ved_handle;
    struct iseg_ems_module *modules;
    int nr_modules;

    if (iseg_ems_init_communication(hostname, port)<0)
        return 1;
    if ((ved_handle=iseg_ems_open_is(vedname))==0)
        return 2;

#if 1
    nr_modules=iseg_ems_enumerate(ved_handle, bus, &modules);
    for (int mod=0; mod<nr_modules; mod++) {
        if (iseg_ems_setup_module(modules+mod)<0)
            return 3;
    }
#else
    modules=new struct iseg_ems_module[64];
    for (int mod=0; mod<...; mod++) {
        modules[mod].ved_handle=ved_handle;
        modules[mod].addr=...;
        modules[mod].bus=...;
        if (iseg_ems_setup_module(modules+mod)<0)
            return 3;
    }
#endif
    cout<<nr_modules<<" modules found"<<endl;

    for (int mod=0; mod<nr_modules; mod++) {
        iseg_ems_dump(cout, modules+mod);
    }

    for (int mod=0; mod<nr_modules; mod++) {
        iseg_ems_ramp(modules+mod, 10.);
    }

    for (int mod=0; mod<nr_modules; mod++) {
        for (int chan=0; chan<modules[mod].channels; chan++) {
            iseg_ems_voltage(modules+mod, chan, modules[mod].u_max[chan]/2.);
        }
    }

    for (int mod=0; mod<nr_modules; mod++) {
        iseg_ems_status_module(modules+mod, 0xa5a5);
    }

    for (int mod=0; mod<nr_modules; mod++) {
        iseg_ems_channel_mask(modules+mod, 0xa5a5);
    }

    for (int i=0; i<15; i++) {
        unsigned int chan_status, board_status;
        iseg_ems_status_module(modules+0, &board_status);
        iseg_ems_status_channel(modules+0, 0, &chan_status);
        cout<<hex<<setfill('0')<<"board: 0x"<<setw(2)<<board_status
            <<" channel: 0x"<<setw(4)<<chan_status
            <<setfill(' ')<<dec<<endl;
        sleep(1);
    }

    for (int mod=0; mod<nr_modules; mod++) {
        for (int chan=0; chan<modules[mod].channels; chan++) {
            float voltage;
            iseg_ems_voltage(modules+mod, chan, &voltage);
            cout<<setfill('0')<<setw(2)<<modules[mod].addr<<':'
                <<setw(2)<<chan<<setfill(' ')<<": "
                <<voltage<<" V"<<endl;
        }
    }

    for (int mod=0; mod<nr_modules; mod++) {
        iseg_ems_voltage(modules+mod, -1, modules[mod].u_max[0]/3.);
    }

    for (int mod=0; mod<nr_modules; mod++) {
        unsigned int mask;
        iseg_ems_channel_mask(modules+mod, &mask);
        cout<<"module "<<modules[mod].addr<<" channels_on: 0x"<<hex<<mask<<dec<<endl;
    }

    for (int mod=0; mod<nr_modules; mod++) {
        for (int chan=0; chan<modules[mod].channels; chan++) {
            float current;
            iseg_ems_current(modules+mod, chan, &current);
            cout<<setfill('0')<<setw(2)<<modules[mod].addr<<':'
                <<setw(2)<<chan<<setfill(' ')<<": "
                <<current<<" A"<<endl;
        }
    }

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
