/*
 * lvd_enumerate.cc
 * 
 * created: 2006-Aug-22 PW
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

VERSION("2009-Oct-19", __FILE__, __DATE__, __TIME__,
"$ZEL: lvd_enumerate.cc,v 1.14 2010/02/05 14:44:47 wuestner Exp $")
#define XVERSION

#define DEFUSOCK "/var/tmp/emscomm"
#define DEFISOCK 4096

// definitions and enum lvd_cardid copied from server/lowlevel/lvd/lvd_map.h
#define LVD_HWtyp(id) ((id)&0xf8)
#define LVD_HWver(id) ((id)&0x7)
#define LVD_FWver(id) (((id)>>8)&0xff)

enum lvd_cardid {
    LVD_CARDID_CONTROLLER_GPX   = 0x00,
    LVD_CARDID_CONTROLLER_F1    = 0x10,
    LVD_CARDID_CONTROLLER_F1GPX = 0x08,
    LVD_CARDID_SLAVE_CONTROLLER = 0x20,
    LVD_CARDID_F1               = 0x30,
    LVD_CARDID_VERTEX           = 0x40,
    LVD_CARDID_VERTEXM3         = 0x48,
    LVD_CARDID_SYNCH_MASTER     = 0x50,
    LVD_CARDID_SYNCH_MASTER2    = 0x58,
    LVD_CARDID_SYNCH_OUTPUT     = 0x60,
    LVD_CARDID_TRIGGER          = 0x68,
    LVD_CARDID_FQDC             = 0x70,
    LVD_CARDID_VFQDC            = 0x78,
    LVD_CARDID_GPX              = 0x80,
    LVD_CARDID_SQDC             = 0x90
};

int bus=0;

C_readargs* args;
C_VED* ved;
C_instr_system *is0;

struct controller {
    ems_u32 id;
    ems_u32 serial;
    const char* typ;
};
struct module {
    ems_u32 addr;
    ems_u32 mtype;
    ems_u32 id;
    ems_u32 serial;
    const char* typ;
    int new_firmware;
};

struct controller controller;
struct module modules[16];
int nr_modules;
int hsdiv;

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
    args->addoption("bus", "bus", 0, "LVDS bus", "bus");
    args->addoption("init", "init", false, "initialize bus", "init");
    args->addoption("hsdiv", "hsdiv", 0x0, "HSDIV", "hsdiv");
    args->addoption("vedname", 0, "", "Name des VED", "vedname");
    args->hints(C_readargs::required, "vedname", 0);
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
static int
open_is0(void)
{
    is0=ved->is0();
    if (is0==0) {
        cerr<<"VED "<<ved->name()<<" has no procedures."<<endl;
        return -1;
    }
    is0->execution_mode(delayed);
    return 0;
}
/*****************************************************************************/
#if 0
static void
reset_ved(void)
{
    try {
        ved->ResetReadout();
    } catch (C_error* e) {
        cerr << (*e) << endl;
        delete e;
    }
    ved->ResetVED();
}
#endif
/*****************************************************************************/
static void
lvdbus_init(void)
{
    is0->clear_command();
    is0->command("lvdbus_init");
    is0->add_param(1, bus);
    delete is0->execute();
}
/*****************************************************************************/
static void
enumerate_modules(void)
{
    C_confirmation* conf;
    ems_i32* buf;

    for (int i=0; i<16; i++) {
        modules[i].addr=0;
        modules[i].mtype=0;
        modules[i].id=0;
        modules[i].serial=static_cast<ems_u32>(-1);
        modules[i].typ=0;
    }
    nr_modules=0;

    is0->clear_command();
    is0->command("lvd_enumerate");
    is0->add_param(1, bus);
    conf=is0->execute();
    buf=conf->buffer();
    buf++; // skip error code
    nr_modules=*buf++;
    for (int i=0; i<nr_modules; i++) {
        modules[i].addr=*buf++;
        modules[i].mtype=*buf++;
        modules[i].id=*buf++;
        modules[i].serial=*buf++;
        switch (LVD_HWtyp(modules[i].id)) {
        case LVD_CARDID_F1:                /* 0x30 */
            modules[i].typ="F1            ";
            break;
        case LVD_CARDID_GPX:               /* 0x80 */
            modules[i].typ="GPX           ";
            break;
        case LVD_CARDID_VERTEX:            /* 0x40 */
            modules[i].typ="VERTEX        ";
            break;
        case LVD_CARDID_VERTEXM3:          /* 0x48 */
            modules[i].typ="VERTEX M3     ";
            break;
        case LVD_CARDID_FQDC:              /* 0x70 */
            modules[i].typ="FQDC          ";
            break;
        case LVD_CARDID_VFQDC:             /* 0x78 */
            modules[i].typ="VFQDC         ";
            break;
        case LVD_CARDID_SQDC:              /* 0x90 */
            modules[i].typ="SQDC          ";
            break;
        case LVD_CARDID_SYNCH_MASTER:      /* 0x50 */
            modules[i].typ="SYNCH Master  ";
            break;
        case LVD_CARDID_SYNCH_MASTER2:     /* 0x58 */
            modules[i].typ="SYNCH Master 2";
            break;
        case LVD_CARDID_SYNCH_OUTPUT:      /* 0x60 */
            modules[i].typ="SYNCH Output  ";
            break;
        case LVD_CARDID_TRIGGER:           /* 0x68 */
            modules[i].typ="Trigger       ";
            break;
        default:
            modules[i].typ="ALIEN         ";
        }
    }
    delete conf;
}
/*****************************************************************************/
static void
list_modules(void)
{
    struct timeval now;
    time_t tt;
    struct tm *tm;
    char ss[1024];

    gettimeofday(&now, 0);
    tt=now.tv_sec;
    tm=gmtime(&tt);
    strftime(ss, 1024, "%F %T %Z", tm);
    cout<<ss<<endl;
    cout<<ved->name()<<endl;
    cout<<"Controller:"<<endl;
    cout<<"  "<<controller.typ
        <<" id 0x"<<hex<<setfill('0')<<setw(4)<<controller.id<<dec;
        if (controller.serial==static_cast<ems_u32>(-1))
            cout<<" (no serial no.)";
        else
            cout<<" serial "<<dec<<controller.serial;
        cout<<endl;

    cout<<nr_modules<<" module"<<(nr_modules==1?"":"s")<<":"<<endl;
    for (int i=0; i<nr_modules; i++) {
        cout<<"  "
            <<hex<<modules[i].addr<<dec<<": "
            <<modules[i].typ
            <<" id 0x"<<hex<<setfill('0')<<setw(4)<<modules[i].id
            <<dec<<setfill(' ');
        if (modules[i].serial==static_cast<ems_u32>(-1))
            cout<<" (no serial no.)";
        else
            cout<<" serial "<<setw(3)<<modules[i].serial;
        if ((modules[i].mtype==ZEL_LVD_SQDC) && modules[i].new_firmware)
            cout<<" new firmware";
        cout<<endl;
    }
}
/*****************************************************************************/
static void
identify_controller(void)
{
    C_confirmation* conf;
    ems_i32* buf;

    is0->clear_command();
    is0->command("lvd_creg.1");
    is0->add_param(4, bus, 0x10, 0, 2);
    is0->command("lvd_creg.1");
    is0->add_param(4, bus, 0x10, 2, 2);
    conf=is0->execute();
    buf=conf->buffer();
    buf++; // skip error code
    controller.id=*buf++;
    controller.serial=*buf++;
    delete conf;
    switch (controller.id&0x00f8) {
    case 0x10:
        controller.typ="F1";
        break;
    case 0x00:
        controller.typ="GPX";
        break;
    case 0x08:
        controller.typ="F1GPX";
        break;
    default:
        controller.typ="ALIEN";
    }
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
    args=new C_readargs(argc, argv);
    if (readargs())
        return 1;

    try {
        bool init=args->getboolopt("init");
        bus=args->getintopt("bus");
        start_communication();
        open_ved();

        //if (init)
        //    reset_ved();
        if (open_is0()<0)
            return 3;
        if (init)
            lvdbus_init();
        identify_controller();
        enumerate_modules();
        list_modules();

    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return 2;
    }

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
