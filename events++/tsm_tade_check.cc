/*
 * ems/events++/tsm_tade_check.cc
 * 
 * created 2004-12-14 PW
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

VERSION("Dec 14 2004", __FILE__, __DATE__, __TIME__,
"$ZEL: tsm_tade_check.cc,v 1.1 2005/02/11 15:45:18 wuestner Exp $")
#define XVERSION

#define __xSTRING(x) #x
#define __SSTRING(x) __xSTRING(x)

#define swap_int(a) ( ((a) << 24) | \
                      (((a) << 8) & 0x00ff0000) | \
                      (((a) >> 8) & 0x0000ff00) | \
        ((unsigned int)(a) >>24) )

namespace {

const char comp_ext[]=".bz2";
const compressed_io::compresstype comp_type=compressed_io::cpt_bzip2;

C_readargs* args;
int verbose, debug;

const char* optfile=".dsm.opt";
const char* tadedir;
string filespace;
string tadespace;
unsigned int firstrun, lastrun;

//---------------------------------------------------------------------------//
int readargs()
{
    const char* homedir=getenv("HOME");
    string home_opt;

    if (homedir) {
        home_opt=homedir;
        home_opt+="/";
        home_opt+=optfile;
    } else {
        home_opt="";
    }

    args->addoption("verbose",    "verbose", false, "verbose output");
    args->addoption("debug",      "debug", false, "debug output");
    args->addoption("version",    "Version", false, "output version only");
    args->hints(C_readargs::implicit, "version", "verbose", 0);
    args->addoption("dsmiConfig", "config", home_opt.c_str(), "config file");
    args->addoption("dsmiDir",    "tsmdir", __SSTRING(TSMDIR), "tsm directory");
    args->addoption("dsmiLog",    "logdir", ".", "log directory");

    args->addoption("clientNode", "node", "", "virtual node name");
    args->addoption("clientPassword", "passwd", "", "client password");
    args->addoption("server",     "server", "archive", "serverstring");

    args->addoption("experiment", "experiment", "", "experiment name");
    args->use_env  ("experiment", "TSM_EXPERIMENT");
    args->addoption("filespace",  "filespace", "", "raw filespace");
    args->addoption("tadespace",  "tadespace", "", "tade filespace");
    args->addoption("tadedir",    "tadedir", "/local/data01/tade", "directory of tade files");
    args->addoption("firstrun",   "r0", 0, "first run");
    args->addoption("lastrun",    "r1", 10000, "last run");

    args->multiplicity("verbose", 1);
    args->hints(C_readargs::required, "experiment", 0);
    args->hints(C_readargs::required, "filespace", 0);
    args->hints(C_readargs::required, "tadespace", 0);
    args->hints(C_readargs::required, "tadedir", 0);

    if (args->interpret(1)!=0) return -1;

    verbose=args->getboolopt("verbose");
    debug=args->getboolopt("debug");
    tadedir=args->getstringopt("tadedir");
    firstrun=args->getintopt("firstrun");
    lastrun=args->getintopt("lastrun");

    filespace=args->getstringopt("filespace");
    if (filespace[0]!='/')
        filespace.insert(0, "/");
    tadespace=args->getstringopt("tadespace");
    if (tadespace[0]!='/')
        tadespace.insert(0, "/");

    return 0;
}
//---------------------------------------------------------------------------//
void
check_tade(dsUint32_t sh, struct l_file *ff)
{
    //static int count=0;
    struct l_file *tf;
    ems_info* info=&ff->info;
    bool name_written=false;
    int res, numtade=0;
    int even=0, odd=0, all=0, scaler=0, d15d16=0;

    ems_u32 expected_flags=emsinfo_digest|emsinfo_run|emsinfo_start|
            emsinfo_stop|emsinfo_events;
    if ((info->flags&expected_flags)!=expected_flags) {
        if (!name_written) {
            cout<<(ff->d.objName.ll+1)<<":"<<endl;
            name_written=true;
        }
        cout<<"  missing infos:";
        if (!(info->flags&emsinfo_digest))cout<<" md5";
        if (!(info->flags&emsinfo_run))cout<<" runnr";
        if (!(info->flags&emsinfo_start))cout<<" start";
        if (!(info->flags&emsinfo_stop))cout<<" stop";
        if (!(info->flags&emsinfo_events))cout<<" events";
        cout<<endl;
    }
    if (!(info->flags&emsinfo_run))
        return;
    if ((info->runnr<firstrun) || (info->runnr>lastrun))
        return;
    if (!info->events)
        return;

    //count++;
    //if (count>10) return;

    string tadeexpr(ff->d.objName.ll);
    tadeexpr.erase(tadeexpr.size()-3);
    tadeexpr.append("*");
    res=query_files(sh, tadespace.c_str(), "*", tadeexpr.c_str(), DSM_OBJ_FILE,
            &tf, 0, 0);
    if (res<0) {
        cout<<"run nr. was "<<info->runnr<<endl;
        // don't free tf; it is corrupted (and will generate a memory leak here)
        return;
    }
    decode_emsinfo(tf, 1);
    struct l_file* tff=tf;
    while (tff) {
        //cout<<"found "<<tff->d.objName.ll+1<<endl;
        if (strstr(tff->d.objName.ll, "even")) even++;
        if (strstr(tff->d.objName.ll, "odd")) odd++;
        if (strstr(tff->d.objName.ll, "all")) all++;
        if (strstr(tff->d.objName.ll, "scaler")) scaler++;
        if (strstr(tff->d.objName.ll, "d15d16")) d15d16++;
        numtade++;
        tff=tff->next;
    }
    if ((even!=1) || (odd!=1)/* || (d15d16!=1)*/) {
        if (!name_written) {
            cout<<(ff->d.objName.ll+1)<<":"<<endl;
            name_written=true;
        }
        cout<<"files found for run "<<info->runnr<<":"<<endl;
        l_file *tff=tf;
        while (tff) {
            cout<<"found "<<tff->d.objName.ll+1<<endl;
            tff=tff->next;
        }
    }
    // compare size of all files with files in tadedir
    tff=tf;
    while (tff) {
        // generate file name
        struct stat stbuf;
        ostringstream fstr;
        fstr<<"run_"<<setfill('0')<<setw(4)<<info->runnr<<"_";
        if (strstr(tff->d.objName.ll, "even")) fstr<<"even";
        if (strstr(tff->d.objName.ll, "odd")) fstr<<"odd";
        if (strstr(tff->d.objName.ll, "all")) fstr<<"all";
        if (strstr(tff->d.objName.ll, "scaler")) fstr<<"scaler";
        if (strstr(tff->d.objName.ll, "d15d16")) fstr<<"d15d16";
        fstr<<".tade.bz2";
        string fname=fstr.str();
        res=stat(fname.c_str(), &stbuf);
        if (res!0) {
            if (!name_written) {
                cout<<(ff->d.objName.ll+1)<<":"<<endl;
                name_written=true;
            }
            cout<<"stat "<<fname<<": "<<strerror(errno)<<endl;
        } else {
            if (stbuf.st_size!=tff->d.sizeEstimate) {
                if (!name_written) {
                    cout<<(ff->d.objName.ll+1)<<":"<<endl;
                    name_written=true;
                }
                cout<<fname<<": size: arch="<<tff->d.sizeEstimate
                    <<" file="<<stbuf.st_size<<endl;
            }
        }
        tff=tff->next;
    }

    free_l_files(tf);
}
//---------------------------------------------------------------------------//

}; //end of anonymous namespace

int
main(int argc, char* argv[])
{
    dsUint32_t sh;
    struct l_file *files, *ff;
    int res, ret=0;

    cerr<<"tsm_tade_check "<<__DATE__<<" "<<__TIME__<<endl;

    args=new C_readargs(argc, argv);
    if (readargs())
        return 1;

    res=tsm_login(args, &sh);
    if (res<1)
        return -res;

    // query all files of cluster filespace
    res=query_files(sh, filespace.c_str(), "*", "/*", DSM_OBJ_FILE, &files,
            0, 0);
    if (res<0) {
        /* 'files' is corrupted at this point; DONT free it! */
        close_session(sh);
        ret=3;
        goto raus;
    }

    //dump_files(files, 1, (tsm_emsinfos)emsinfo_run);
    decode_emsinfo(files, 1);

    ff=files;
    while (ff) {
        check_tade(sh, ff);
        ff=ff->next;
    }

    close_session(sh);
    free_l_files(files);

raus:
    return ret;
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
