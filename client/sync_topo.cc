/*
 * sync_topo.cc
 * created 2006-Apr-03 PW
 */

#include "config.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "cxxcompat.hxx"
#include <readargs.hxx>
#include <ved_errors.hxx>
#include <errors.hxx>
#include <proc_communicator.hxx>
#include <proc_is.hxx>
#include <errorcodes.h>
#include <stdio.h>
#include <errno.h>
#include "client_defaults.hxx"

#include <versions.hxx>

VERSION("2006-Apr-03", __FILE__, __DATE__, __TIME__,
"$ZEL: sync_topo.cc,v 1.1 2006/05/15 20:44:17 wuestner Exp $")

int vednum;
STRING* vednames;
C_VED** ved;
C_instr_system** is;
int* p;
C_readargs* args;

/*****************************************************************************/
static int
readargs(void)
{
    int res;
    args->addoption("hostname", "h", "localhost",
        "host running commu", "hostname");
    args->use_env("hostname", "EMSHOST");
    args->addoption("port", "p", DEFISOCK,
        "port for connection with commu", "port");
    args->use_env("port", "EMSPORT");
    args->addoption("sockname", "s", DEFUSOCK,
        "socket for connection with commu", "socket");
    args->use_env("sockname", "EMSSOCKET");

    args->hints(C_readargs::exclusive, "hostname", "sockname", 0);
    args->hints(C_readargs::exclusive, "port", "sockname", 0);

    if ((res=args->interpret(1))!=0) return res;

    return 0;
}
/*****************************************************************************/
static void
open_veds(void)
{
    int i;

    ved=(C_VED**)calloc(sizeof(C_VED*), vednum);
    is=(C_instr_system**)calloc(sizeof(C_instr_system*), vednum);
    for (i=0; i<vednum; i++) {
        cerr<<"ved "<<i<<": "<<vednames[i]<<endl;
        ved[i]=new C_VED(args->getnames(i));
        is[i]=ved[i]->is0();
    }
}
/*****************************************************************************/
static void
open_syncpath(void)
{
    C_confirmation* conf;
    int i;

    p=(int*)calloc(sizeof(int), vednum);
    for (i=0; i<vednum; i++) {
        cerr<<"ved "<<vednames[i]<<" try /tmp/sync0"<<endl;
        try {
            is[i]->execution_mode(delayed);
            is[i]->command("SyncOpen");
            is[i]->add_param("/tmp/sync0");
            conf=is[i]->execute();
            p[i]=conf->buffer(1);
            delete conf;
        } catch (C_error* e) {
            cerr << (*e) << endl;
            delete e;

            try {
                is[i]->clear_command();
                cerr<<"ved "<<vednames[i]<<" try /tmp/pcisync0"<<endl;
                is[i]->execution_mode(delayed);
                is[i]->command("SyncOpen");
                is[i]->add_param("/tmp/pcisync0");
                conf=is[i]->execute();
                p[i]=conf->buffer(1);
                delete conf;
            } catch (C_error* e) {
                cerr << (*e) << endl;
                delete e;
                exit(1);
            }
        }
    }
}
/*****************************************************************************/
static void
sync_topo(void)
{
    for (int i=0; i<vednum; i++) {
        is[i]->clear_command();
        is[i]->command("SyncWrite", 3, p[i], 8, 0); // master reset
        is[i]->execute();
    }
    for (int i=0; i<vednum; i++) {
        is[i]->clear_command();
        is[i]->command("SyncRead", 2, p[i], 0); // read CSR
        C_confirmation* conf=is[i]->execute();
        printf("status[%5s]=0x%08x\n", vednames[i].c_str(), conf->buffer(1));
        delete conf;
    }
    printf("\n");

    for (int i=0; i<vednum; i++) {
        cout<<"setting GO in "<<vednames[i]<<endl;
        for (int j=0; j<vednum; j++) {
            is[j]->clear_command();
            is[j]->command("SyncWrite", 3, p[j], 8, 0); // master reset
            is[j]->execute();
        }
        is[i]->clear_command();
        is[i]->command("SyncWrite", 3, p[i], 1, 2); // set GO
        is[i]->execute();
        for (int j=0; j<vednum; j++) {
            is[j]->clear_command();
            is[j]->command("SyncRead", 2, p[j], 0); // read CSR
            C_confirmation* conf=is[j]->execute();
            printf("status[%5s]=0x%08x\n", vednames[j].c_str(), conf->buffer(1));
            delete conf;
        }
        printf("\n");
    }

}
/*****************************************************************************/
main(int argc, char* argv[])
{
    int i;

    try {
        args=new C_readargs(argc, argv);
        if (readargs()!=0)
            return(1);

        vednum=args->numnames();
        vednames=new STRING[vednum];
        for (i=0; i<vednum; i++)
            vednames[i]=args->getnames(i);

        if (!args->isdefault("hostname") || !args->isdefault("port")) {
            communication.init(args->getstringopt("hostname"),
                args->getintopt("port"));
        } else { //if (!args->isdefault("sockname"))
            communication.init(args->getstringopt("sockname"));
        }

        open_veds();
        open_syncpath();
        sync_topo();

    } catch (C_error* e) {
        cerr << (*e) << endl;
        delete e;
        return 5;
    }
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
