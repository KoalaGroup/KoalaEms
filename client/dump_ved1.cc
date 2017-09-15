/*
 * client/dump_ved1.cc
 *
 * created: 16.Apr.2004 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <errors.hxx>
#include <readargs.hxx>
#include <proc_communicator.hxx>
#include <proc_is.hxx>

#include <versions.hxx>

VERSION("Apr 11 2004", __FILE__, __DATE__, __TIME__,
"$ZEL: dump_ved1.cc,v 2.1 2005/05/27 19:00:18 wuestner Exp $";
#define XVERSION

C_readargs* args;
STRING* vednames;
int numveds;
int triggermask;

int test=0;

/*****************************************************************************/
static
int readargs()
{
    args->addoption("ems_host", "h", "", "commu host", "ems_host");
    args->addoption("ems_port", "p", 4096, "commu port", "ems_port");
    args->addoption("ems_sock", "s", "", "commu socket", "ems_socket");
    args->addoption("triggermask", "t", 0xf, "trigger mask for readout lists", "triggermask");

    args->addoption("ved", 0, "", "ved name", "ved");
    args->hints(C_readargs::exclusive, "ems_host", "ems_sock", 0);

    if (args->interpret(1)) return -1;

    triggermask=args->getintopt("triggermask");

    numveds=args->numnames();
    vednames=new STRING[numveds];
    for (int i=0; i<numveds; i++) vednames[i]=args->getnames(i);

    try {
        if (args->isargument("ems_host")) {        // host given on commandline
            communication.init(args->getstringopt("ems_host"),
                args->getintopt("ems_port"));
        } else if (args->isargument("ems_sock")) { // socket given on commandline
            communication.init(args->getstringopt("ems_sock"));
        } else {                                   // use default for C_communication
            communication.init();
        }
    } catch (C_error* e) {
        cout<<(*e)<<endl;
        delete e;
        return -1;
    }

    if (!communication.valid()) return -1;

    return 0;
}
/*****************************************************************************/
static void
dump_vedobject(C_VED* ved)
{
//    cout<<"vedobject not decoded"<<endl;
}
/*****************************************************************************/
static void
dump_dom_Modullist(C_VED* ved)
{
    if (test) cout<<"Modullist not decoded"<<endl;
}
/*****************************************************************************/
static void
dump_dom_LAMproclist(C_VED* ved)
{
    if (test) cout<<"LAMproclist not decoded"<<endl;
}
/*****************************************************************************/
static void
dump_dom_Trigger(C_VED* ved)
{
    if (test) cout<<"Trigger not decoded"<<endl;
}
/*****************************************************************************/
static void
dump_dom_Event(C_VED* ved)
{
    if (test) cout<<"Event not decoded"<<endl;
}
/*****************************************************************************/
static void
dump_dom_Datain(C_VED* ved)
{
    if (test) cout<<"Datain not decoded"<<endl;
}
/*****************************************************************************/
static void
dump_dom_Dataout(C_VED* ved)
{
    if (test) cout<<"Dataout not decoded"<<endl;
}
/*****************************************************************************/
static void
dump_domain(C_VED* ved)
{
    C_confirmation* conf;
    C_namelist* namelist;

    conf=ved->GetNamelist(Object_domain, Dom_null);
    namelist=new C_namelist(conf);
    delete conf;
    for (int i=0; i<namelist->size(); i++) {
        Domain dom=(Domain)(*namelist)[i];
        switch (dom) {
            case Dom_null:
                cout<<"Huuuu! There exists a Null Domain!"<<endl;
                break;
            case Dom_Modullist  : dump_dom_Modullist(ved)  ; break;
            case Dom_LAMproclist: dump_dom_LAMproclist(ved); break;
            case Dom_Trigger    : dump_dom_Trigger(ved)    ; break;
            case Dom_Event      : dump_dom_Event(ved)      ; break;
            case Dom_Datain     : dump_dom_Datain(ved)     ; break;
            case Dom_Dataout    : dump_dom_Dataout(ved)    ; break;
            default: cout<<"unknown domain type "<<dom<<endl;
        }
    }
    delete namelist;
}
/*****************************************************************************/
static void
decode_readoutlist(C_instr_system* is,  C_confirmation* conf)
{
    cout<<"# priority "<<conf->buffer(1)<<endl;
    int i=3;
    while (i<conf->size()) {
        cout<<"    proc: "<<is->procname(conf->buffer(i++));
        int n=conf->buffer(i++);
        cout<<' '<<n;
        for (int j=0; j<n; j++) {
            cout<<' '<<conf->buffer(i++);
        }
        cout<<endl;
    }
}
/*****************************************************************************/
static void
dump_is(C_VED* ved, int idx)
{
    C_instr_system* is;
    C_isstatus* status;
    C_confirmation* conf;

    is=new C_instr_system(ved, idx, C_instr_system::open);
    cout<<"IS: "<<is->is_id()<<endl;

    status=is->ISStatus();
    for (int tt=0; tt<status->num_trigger(); tt++) {
        ems_u32 t=status->trigger(tt);
        if (t&triggermask) {
            cout<<"  trigger: "<<t<<endl;
            conf=is->UploadReadoutlist(t);
            decode_readoutlist(is, conf);
            delete conf;
        }
    }

    delete status;
    delete is;
}
/*****************************************************************************/
static void
dump_is(C_VED* ved)
{
    C_confirmation* conf;
    C_namelist* namelist;

    conf=ved->GetNamelist(Object_is);
    namelist=new C_namelist(conf);
    delete conf;
    for (int i=0; i<namelist->size(); i++) {
        int idx=(*namelist)[i];
        dump_is(ved, idx);
    }
    delete namelist;
}
/*****************************************************************************/
static void
dump_var(C_VED* ved)
{
    if (test) cout<<"VARs not decoded"<<endl;
}
/*****************************************************************************/
static void
dump_pi(C_VED* ved)
{
    if (test) cout<<"PIs not decoded"<<endl;
}
/*****************************************************************************/
static void
dump_do(C_VED* ved)
{
    if (test) cout<<"DOs not decoded"<<endl;
}
/*****************************************************************************/
static void
dump_ved(C_VED* ved)
{
    C_confirmation* conf;
    C_namelist* namelist;

    cout<<endl<<"# VED "<<ved->name()<<":"<<endl;

    conf=ved->GetNamelist(Object_null);
    namelist=new C_namelist(conf);
    delete conf;

    for (int i=0; i<namelist->size(); i++) {
        Object obj=(Object)(*namelist)[i];
        switch (obj) {
            case Object_null:
                cout<<"Huuuu! There exists a Null Object!"<<endl;
                break;
            case Object_ved   : dump_vedobject(ved); break;
            case Object_domain: dump_domain(ved)   ; break;
            case Object_is    : dump_is(ved)       ; break;
            case Object_var   : dump_var(ved)      ; break;
            case Object_pi    : dump_pi(ved)       ; break;
            case Object_do    : dump_do(ved)       ; break;
            default: cout<<"unknown object type "<<obj<<endl;
        }
    }
    delete namelist;
}
/*****************************************************************************/
main(int argc, char* argv[])
{
    try {
        args=new C_readargs(argc, argv);
        if (readargs()) return 1;

        //cout<<"connected via "<<communication<<endl;

        for (int i=0; i<numveds; i++) {
            C_VED* ved;
            ved=new C_VED(vednames[i]);
            dump_ved(ved);
            delete ved;
        }
    } catch (C_error* e) {
        cout<<(*e)<<endl;
        delete e;
        return 2;
    }

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
