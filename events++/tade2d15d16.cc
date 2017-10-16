/*
 * ems/events++/tade2d15d16.cc
 * 
 * created 2004-12-11 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <vector>
#include <queue>
#include <readargs.hxx>
#include <errno.h>
#include <sys/wait.h>
#include <xdrstring.h>
#include <clusterformat.h>
#include "../tsm/tsm_tools.hxx"
#include "../tsm/tsm_tools_login.hxx"
#include "compressed_io.hxx"
#include "ems_objects.hxx"
#include "event2tade.hxx"
#include <versions.hxx>

VERSION("Dec 11 2004", __FILE__, __DATE__, __TIME__,
"$ZEL$")
#define XVERSION

#define __xSTRING(x) #x
#define __SSTRING(x) __xSTRING(x)

namespace {

const char comp_ext[]=".bz2";
const compressed_io::compresstype comp_type=compressed_io::cpt_bzip2;

C_readargs* args;
int verbose, debug;
int allow_run;

const char* optfile=".dsm.opt";
string destdir;
string filespace;
int run_a, run_b;

//---------------------------------------------------------------------------//
int readargs()
{
    const char* homedir=getenv("HOME");
    char* home_opt;

    if (homedir) {
        home_opt=new char[strlen(homedir)+strlen(optfile)+2];
        strcpy(home_opt, homedir);
        strcat(home_opt, "/");
        strcat(home_opt, optfile);
    } else {
        home_opt=new char[2];
        strcpy(home_opt, "");
    }

    args->addoption("verbose",    "verbose", false, "verbose output");
    args->addoption("debug",      "debug", false, "debug output");
    args->addoption("version",    "Version", false, "output version only");
    args->hints(C_readargs::implicit, "version", "verbose", 0);
    args->addoption("dsmiConfig", "config", home_opt, "config file");
    args->addoption("dsmiDir",    "tsmdir", __SSTRING(TSMDIR), "tsm directory");
    args->addoption("dsmiLog",    "logdir", ".", "log directory");

    args->addoption("clientNode", "node", "", "virtual node name");
    args->addoption("clientPassword", "passwd", "", "client password");
    args->addoption("server",     "server", "archive", "serverstring");

    args->addoption("experiment", "experiment", "", "experiment name");
    args->use_env  ("experiment", "TSM_EXPERIMENT");
    args->addoption("filespace",  "filespace", "", "filespace");
    args->use_env  ("filespace",  "TSM_FILESPACE");
    args->addoption("destdir",    "destdir", ".", "destination directory");
    args->addoption("run_a",      0, 0, "first run");
    args->addoption("run_b",      1, 0, "last run");

    args->multiplicity("verbose", 1);
    args->hints(C_readargs::required, "filespace", "run_a", "run_b", 0);

    delete[] home_opt;

    if (args->interpret(1)!=0) return -1;

    verbose=args->getboolopt("verbose");
    debug=args->getboolopt("debug");
    destdir=args->getstringopt("destdir");
    run_a=args->getintopt("run_a");
    run_b=args->getintopt("run_b");

    filespace=args->getstringopt("filespace");
    if (filespace[0]!='/')
        filespace.insert(0, "/");

    return 0;
}
//---------------------------------------------------------------------------//
struct head {
    FILE* f;
    unsigned int evno;
    unsigned int size;
    unsigned int runno;
    unsigned int triggerPattern;
};
int
read_head(head& h)
{
    int res;
    res=fscanf(h.f, "%u %u %u %u  Event\n", &h.evno, &h.size, &h.runno, &h.triggerPattern);
    if (res!=4) {
        cerr<<"read error (head): res="<<res<<endl;
        h.evno=0xffffffff;
    }
    return 0;
}
//---------------------------------------------------------------------------//
int
extract_d15d16(FILE* f1, FILE* f2, FILE* fo)
{
    struct line {
        int tdc;
        int qdc;
        int det;
        int elem;
    };
    line lines[100];

    head hh1={f1, 0xffffffffU, 0, 0};
    head hh2={f2, 0xffffffffU, 0, 0};
    head h[2]={hh1, hh2};

    // read the first event header from each file
    read_head(h[0]);
    read_head(h[1]);

    while (1) {
        // sort evno
        if (h[0].evno==h[1].evno) {
            cerr<<"identical events "<<h[0].evno<<endl;
        } else if (h[1].evno<h[0].evno) {
            head help=h[0];
            h[0]=h[1];
            h[1]=help;
        }

        // all events read?
        if (h[0].evno==0xffffffff)
            break;

        // read and extract from h0
        int nlines=0;
        for (unsigned int i=0; i<h[0].size; i++) {
            int res, tdc=-1, qdc=-1, det=-1, elem=-1;
            res=fscanf(h[0].f, "%d %d %d %d\n", &tdc, &qdc, &det, &elem);
            if (res!=4) {
                cerr<<"read error (data): res="<<res<<endl;
                return -1;
            }
            if ((det==15) || (det==16)) {
                lines[nlines].tdc=tdc;
                lines[nlines].qdc=qdc;
                lines[nlines].det=det;
                lines[nlines].elem=elem;
                nlines++;
                if (nlines>=100) {
                    cerr<<"too many hits"<<endl;
                    return -1;
                }
            }
        }
        if (nlines) {
            fprintf(fo, "%d %d %d %d  Event\n",
                h[0].evno, nlines, h[0].runno, h[0].triggerPattern);

            for (int j=0; j<nlines; j++) {
                fprintf(fo, "%6d %5d %5d %5d\n", lines[j].tdc, lines[j].qdc,
                        lines[j].det, lines[j].elem);
            }
        }
        // read new event header for h0
        read_head(h[0]);
    }
    return 0;
}
//---------------------------------------------------------------------------//
int
filter_run(int run) {
    // we need three file: even, odd and d15d16
    string filename_even;
    string filename_odd;
    string filename_d15d16;
    {
        ostringstream st;
        st<<setfill('0')<<destdir<<"/run_"<<setw(4)<<run<<"_even.tade.bz2";
        filename_even=st.str();
    }
    {
        ostringstream st;
        st<<setfill('0')<<destdir<<"/run_"<<setw(4)<<run<<"_odd.tade.bz2";
        filename_odd=st.str();
    }
    {
        ostringstream st;
        st<<setfill('0')<<destdir<<"/run_"<<setw(4)<<run<<"_d15d16.tade.bz2";
        filename_d15d16=st.str();
    }
cout<<"looking for files:"<<endl
    <<"  "<<filename_even<<endl
    <<"  "<<filename_odd<<endl
    <<"  "<<filename_d15d16<<endl;

    int res;
    struct stat buf;

    // does d15d16 already exist?
    res=stat(filename_d15d16.c_str(), &buf);
    if ((res==0) || (errno!=ENOENT)) {
        cerr<<filename_d15d16<<" does already exist."<<endl;
        return 0;
    }

    // try to open 'even' and 'odd'
    compressed_input inp_even(filename_even);
    compressed_input inp_odd(filename_odd);
    if (!inp_even.good()) {
        cerr<<filename_even<<" does not exist."<<endl;
        return -1;
    }
    if (!inp_odd.good()) {
        cerr<<filename_odd<<" does not exist."<<endl;
        return -1;
    }

    // both input files exist; no we can create the output file
    compressed_output out(filename_d15d16, compressed_io::cpt_bzip2);
    if (!out.good()) {
        cerr<<"cannot create output file "<<filename_d15d16<<endl;
        inp_even.close();
        inp_odd.close();
    }

    // no do the real work
    res=extract_d15d16(inp_even.fdopen(), inp_odd.fdopen(), out.fdopen());

    // close all files
    inp_even.close();
    inp_odd.close();
    out.close();

    if (res<0) {
        cerr<<"error extracting events"<<endl;
        remove(filename_d15d16.c_str());
    }

    return 0;
}
//---------------------------------------------------------------------------//

}; //end of anonymous namespace

int
main(int argc, char* argv[])
{
    int ret=0;

    cerr<<"tade2d15d16 "<<__DATE__<<" "<<__TIME__<<endl;

    args=new C_readargs(argc, argv);
    if (readargs())
        return 1;

    for (int run=run_a; run<=run_b; run++) {
        filter_run(run);
    }

goto raus;

raus:
    return ret;
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
