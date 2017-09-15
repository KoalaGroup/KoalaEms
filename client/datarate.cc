/*
 * datarate.cc
 * 
 * created: 27.04.99
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <unistd.h>
#include <readargs.hxx>
#include <ved_errors.hxx>
#include <proc_communicator.hxx>
#include <proc_ved.hxx>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <client_defaults.hxx>
#include <versions.hxx>

VERSION("Apr 27 1999", __FILE__, __DATE__, __TIME__,
"$ZEL: datarate.cc,v 1.5 2009/10/02 11:22:34 wuestner Exp $")
#define XVERSION

C_VED* ved;
C_readargs* args;
int running, verbose;
int num_do, *dolist;
const char* timeformat;

/*****************************************************************************/
int readargs()
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
    args->addoption("do", "do", -1, "dataout to be used", "dataout");
    args->addoption("list", "list", false, "list all tape dataouts", "list");
    args->addoption("interval", "i", 10, "interval in seconds", "interval");
    args->addoption("verbose", "v", false, "verbose");
    args->addoption("vedname", 0, "", "name of VED", "vedname");
    args->addoption("timeformat", "tf", "%a %H:%M:%S", "format for timestamp", "timeformat");
    return args->interpret(1);
}
/*****************************************************************************/
extern "C" {
#if defined(SA_HANDLER_VOID)
void alarmhand()
#else
void alarmhand(int sig)
#endif
{}
}
/*****************************************************************************/
extern "C" {
#if defined(SA_HANDLER_VOID)
void inthand()
#else
void inthand(int sig)
#endif
{
    running=0;
}
}
/*****************************************************************************/
static int
namelist_do(int* num, int** list)
{
    try {
        C_confirmation* conf=ved->GetNamelist(Object_domain, Dom_Dataout);
        *num=conf->buffer(1);
        *list=new int[*num];
        for (int i=0; i<*num; i++)
            (*list)[i]=conf->buffer(2+i);
        delete conf;
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return -1;
    }
    return 0;
}
/*****************************************************************************/
void list_dataouts()
{
    for (int d=0; d<num_do; d++) {
        C_data_io* addr;
        try {
            addr=ved->UploadDataout(dolist[d]);
            cout<<"dataout "<<dolist[d]<<": "<<(*addr)<<endl;
            delete addr;
        } catch (C_error* e) {
            cout<<"dataout "<<dolist[d]<<": "<<(*e)<< endl;
            delete e;
        }
    }
}
/*****************************************************************************/
int rate(int dataout, int interval)
{
    int first;
    struct timeval vorher;
    int old_clusters, old_words;

    cout<<"using dataout "<<dataout<<endl;
    first=1;
    running=1;

    while (running) {
        if (!first) {
            //alarm(interval);
            //sigpause(0);
            sleep(interval);
        }
        try {
            C_confirmation* conf;
            struct timeval jetzt;
            double sec, usec;
            float zeit;
            struct tm *tm;
            char s[1024];

            conf=ved->GetDataoutStatus(dataout, 1);

            gettimeofday(&jetzt, 0);
            sec=jetzt.tv_sec-vorher.tv_sec;
            usec=jetzt.tv_usec-vorher.tv_usec;
            if (usec<0) {usec+=1000000; sec--;}
            zeit=sec+usec/1000000.0;
            tm=localtime(&jetzt.tv_sec);
            strftime(s, 1024, timeformat, tm);
            vorher=jetzt;

            switch (static_cast<DoRunStatus>(conf->buffer(2))) {
            case Do_running: break;
            case Do_neverstarted: cout << "ready to run; "; break;
            case Do_done: cout << "no more data; "; break;
            case Do_error: cout << "not ready; "; break;
            default: cout << "unknown status " << conf->buffer(2) << "; "; break;
            }
            if (conf->buffer(3))
                cout << "disabled; ";

            if (conf->buffer(1)) {
                cout << "error " << conf->buffer(1) << endl;
            } else {
                int clusters, words;

                clusters=conf->buffer(5);
                words=conf->buffer(6);

                if (!first) {
                    float clusters_rate=float(clusters-old_clusters)/zeit;
                    float kbyte_rate=float(words-old_words)/zeit*4./1024.;
                    cout<<s;
                    cout.precision(2);
                    cout.setf(ios::fixed, ios::floatfield);
                    cout.width(9);
                    cout<<" "<<kbyte_rate<<" KByte/s";
                    cout<<" "<<clusters_rate<<" clusters/s";
                    if (clusters_rate)
                        cout<<" ("<<kbyte_rate/clusters_rate<<" KByte/cluster)";
                    cout<<endl;
                }
                old_clusters=clusters;
                old_words=words;
            }
        } catch (C_error* error) {
            cerr << (*error) << endl;
            delete error;
        }
        first=0;
    }
    return(0);
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
    args=new C_readargs(argc, argv);
    if (readargs()!=0)
        return 0;

    verbose=args->getboolopt("verbose");
    timeformat=args->getstringopt("timeformat");

    if (args->isdefault("vedname")) {
        cerr << "vedname expected" << endl;
        return 0;
    }
    try {
        if (!args->isdefault("hostname") || !args->isdefault("port")) {
            communication.init(args->getstringopt("hostname"),
                args->getintopt("port"));
        } else {
            communication.init(args->getstringopt("sockname"));
        }
        ved=new C_VED(args->getstringopt("vedname"));
    } catch(C_error* error) {
        cerr << (*error) << endl;
        delete error;
        return 0;
    }

    struct sigaction vec, ovec;

    if (sigemptyset(&vec.sa_mask)==-1) {
        cerr << "sigemptyset: " << strerror(errno) << endl;
        return -1;
    }
    vec.sa_flags=0;
    vec.sa_handler=alarmhand;
    if (sigaction(SIGALRM, &vec, &ovec)==-1) {
        cerr << "sigaction: " << strerror(errno) << endl;
        return -1;
    }
    vec.sa_handler=inthand;
    if (sigaction(SIGINT, &vec, &ovec)==-1) {
        cerr << "sigaction: " << strerror(errno) << endl;
        return -1;
    }

    if (namelist_do(&num_do, &dolist)<0)
        return 0;
    if (num_do==0) {
        cout<<"no dataouts available."<<endl;
        return 0;
    }
    if (args->getboolopt("list")) {
        list_dataouts();
        return 0;
    }
    int dataout=args->getintopt("do");
    if (dataout<0)
        dataout=dolist[0];

    rate(dataout, args->getintopt("interval"));

    delete ved;
    communication.done();
    cout << "ende" << endl;
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
