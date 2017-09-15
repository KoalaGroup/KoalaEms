/*
 * ems/events++/tsm_cl2tade.cc
 * 
 * created 2004-11-24 PW
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
"$ZEL: tsm_cl2tade.cc,v 1.1 2005/02/11 15:45:16 wuestner Exp $")
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
int allow_run;

const char* optfile=".dsm.opt";
const char* tmpdir;
string filespace;
string destspace;
bool force;

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
    args->addoption("destspace",  "dfilespace", "", "destination filespace");
    args->addoption("allow_run",  "allownumber", true, "allow run numbers as file selector");
    args->addoption("objid",      "id", (ems_u64)0, "object id");
    args->addoption("tmpdir",     "tmpdir", "/var/tmp", "directory for temporary files");
    args->use_env  ("tmpdir",     "TMPDIR");
    args->addoption("deltemps",   "deltempfiles", true ,"delete temporary files");
    args->addoption("force",      "force", false, "force overwrite existing files");
    args->addoption("detdef",     "detdef", "", "replacement detdef file");

    args->multiplicity("verbose", 1);
    args->hints(C_readargs::required, "filespace", "destspace", 0);

    delete[] home_opt;

    if (args->interpret(1)!=0) return -1;

    verbose=args->getboolopt("verbose");
    debug=args->getboolopt("debug");
    allow_run=args->getboolopt("allow_run");
    tmpdir=args->getstringopt("tmpdir");
    force=args->getboolopt("force");

    filespace=args->getstringopt("filespace");
    if (filespace[0]!='/')
        filespace.insert(0, "/");
    destspace=args->getstringopt("destspace");
    if (destspace[0]!='/')
        destspace.insert(0, "/");

    return 0;
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
static struct l_file*
find_file_id(struct l_file* files, ems_u64 id)
{
    while (files) {
        ems_u64 objId=ds64_long(files->d.objId);
        if (objId==id)
            break;
        files=files->next;
    }
    return files;
}
//---------------------------------------------------------------------------//
static struct l_file*
find_file_run(struct l_file* files, ems_u64 run)
{
    while (files) {
        if (files->info.flags&emsinfo_run) {
            if (files->info.runnr==run)
                break;
        }
        files=files->next;
    }
    return files;
}
//---------------------------------------------------------------------------//
static int
select_files_range(struct l_file* files, ems_u64 run0, ems_u64 run1)
{
    int num=0;
    while (files) {
        if (files->info.flags&emsinfo_run) {
            if ((files->info.runnr>=run0) && (files->info.runnr<=run1)) {
                files->valid=1;
                num++;
            }
        }
        files=files->next;
    }
    return num;
}
//---------------------------------------------------------------------------//
static struct l_file*
find_file_name(struct l_file* files, const char* name)
{
    while (files) {
        char* llname=files->d.objName.ll; // use only low level name
        if (llname[0]=='/') llname++;     // ignore leading /
        if (strcmp(llname, name)==0)
            break;
        files=files->next;
    }
    return files;
}
//---------------------------------------------------------------------------//
// markes all wanted files as "valid"
// and returns number of files not found
static int
select_files(struct l_file* files)
{
    struct l_file* f;
    int i, misses=0;

    for (i=args->multiplicity("objid")-1; i>=0; i--) {
        ems_u64 id=args->getu64opt("objid", i);
        f=find_file_id(files, id);
        if (f) {
            f->valid=1;
        } else {
            printf("id %llu not found\n", (unsigned long long)id);
            misses++;
        }
    }

    for (i=args->numnames()-1; i>=0; i--) {
        const char* name=args->getnames(i);
        char* end;
        ems_u64 run0, run1;
        int res;

        f=find_file_name(files, name);
        if (f) {
            f->valid=1;
            continue;
        }
        if (!allow_run) {
            printf("file %s not found\n", name);
            misses++;
            continue;
        }

        run0=strtoll(name, &end, 10);
        if (end==name) { // name is not valid decimal integer
            printf("file %s not found\n", name);
            misses++;
            continue;
        }

        if (!*end) { // name is a valid decimal integer
            f=find_file_run(files, run0);
            if (f)
                f->valid=1;
            else
                printf("run %llu not found\n", (unsigned long long)run0);
            continue;
        }

        // is name a range? (run0-run1)
        if (*end!='-') { // no
            printf("%s is neither an existing file nor a valid range\n", name);
            misses++;
            continue;
        }
        run1=strtoll(end+1, &end, 10);
        if (*end) { // not valid
            printf("%s is neither an existing file nor a valid range\n", name);
            misses++;
            continue;
        }
        if (run1<=run0) {
            printf("%s is not a valid range\n", name);
            misses++;
            continue;
        }

        res=select_files_range(files, run0, run1);
        if (!res) {
            printf("no files in range %llu - %llu found\n",
                (unsigned long long)run0, (unsigned long long)run1);
            misses++;
        }
    }
    return misses;
}
//---------------------------------------------------------------------------//
int
try_extract(ems_object& ems, int final, compressed_output* of, int* nevents)
{
    while (1) {
        if (!final) {
            // check whether a queue is empty; this is the regular exit
            // of the loop
            for (int i=0; i<ems.num_ved; i++) {
                if (ems.ved_slots[i].empty()) {
                    //printf("slot %d empty\n", i);
                    return 0;
                }
            }
        } else {
            // same as above but until all queues are empty
            int n=0;
            for (int i=0; i<ems.num_ved; i++) {
                if (!ems.ved_slots[i].empty()) n++;
            }
            if (!n) return 0;
        }

        deque<sev_object> event;
        for (int i=0; i<ems.num_ved; i++) {
            deque<sev_object>* que=ems.ved_slots+i;
            while (!que->empty() && (que->front().event_nr<=ems.next_evno)) {
                sev_object& sevo=que->front();
                if (sevo.event_nr<ems.next_evno) {
                    printf("queue %d: event %d (%d expected)\n",
                            i, sevo.event_nr, ems.next_evno);
                    return -1;
                }
                event.push_back(sevo);
                que->pop_front();
            }
        }
        int res=cl2tade::decode_to_tade(event, ems,
            of[0].file(), of[1].file(), of[2].file(), of[3].file(),
            nevents);
        if (res<0)
            return res;
        ems.next_evno++;
    }
}
//---------------------------------------------------------------------------//
int
parse_events(ems_u32* buf, int num, ems_object& ems, compressed_output* of,
    int* nevents)
{
    ems_u32 optsize=buf[0];
    //ems_u32 flags=buf[optsize+1];
    ems_u32 ved_id=buf[optsize+2];
    ems_u32 events=buf[4+optsize];
    ems_u32* p=buf+optsize+5;

    int ved_idx=ems.get_vedindex(ved_id);
    if (ved_idx<0) {
        printf("unknown ved_id %d\n", ved_id);
        return -1;
    }
    deque<sev_object>* que=ems.ved_slots+ved_idx;
    bool empty=que->empty();

    for (ems_u32 event=0; event<events; event++) {
        /* ems_u32 evsize=* */p++;
        ems_u32 evno=*p++;
        ems_u32 trigno=*p++;
        ems_u32 subevents=*p++;
        if (((trigno!=1) && (trigno!=8)) || (evno==0)) {
            printf("  ev %d trigger %x %d subevents\n", evno, trigno, subevents);
            if ((trigno!=1) && (trigno!=8)) {
                return 0;
            }
        }
        for (ems_u32 subevent=0; subevent<subevents; subevent++) {
            ems_u32 IS_ID=*p++;
            ems_u32 size=*p++;
            sev_object sevo;
            sevo.event_nr=evno;
            sevo.trigger=trigno;
            sevo.sev_id=IS_ID;
            sevo.data.reserve(size);
            for (ems_u32 i=0; i<size; i++) {
                sevo.data.push_back(*p++);
            }
            que->push_back(sevo);
        }
    }
    if (empty) {
        int res;
        res=try_extract(ems, 0, of, nevents);
        if (res<0)
            return -1;
    }

    //printf("ved %d events: %d\n", ved, events);
    return 0;
}
//---------------------------------------------------------------------------//
int
parse_no_more_data(ems_u32* buf, int num, ems_object& ems, compressed_output* of,
    int* nevents)
{
    printf("no_more_data\n");
    try_extract(ems, 1, of, nevents);
    return 0;
}
//---------------------------------------------------------------------------//
int
parse_record(ems_u32* buf, int num, ems_object& ems, compressed_output* of,
    int* nevents)
{
    int res=0;
    switch (buf[0]) {
    case clusterty_events:
        res=parse_events(buf+1, num-1, ems, of, nevents);
        break;
    case clusterty_ved_info:
        res=ems.parse_ved_info(buf+1, num-1);
        break;
    case clusterty_text:
        res=ems.parse_text(buf+1, num-1);
        break;
    case clusterty_file:
        res=ems.parse_file(buf+1, num-1);
        break;
    case clusterty_no_more_data:
        res=parse_no_more_data(buf+1, num-1, ems, of, nevents);
        break;
    default:
        printf("unknown clustertype %d\n", buf[2]);
        res=-1;
    }
    return res;
}
//---------------------------------------------------------------------------//
int
xread(int p, char* b, int num)
{
    int da=0, rest=num, res;
    while (rest) {
        res=read(p, b+da, rest);
        if (res==0)
            return 0;
        if (res<0) {
            if (errno!=EINTR) {
                printf("xread: %s\n", strerror(errno));
                return -1;
            } else {
                res=0;
            }
        }
        rest-=res;
        da+=res;
    }
    return num;
}
//---------------------------------------------------------------------------//
/*
 * returns:
 * -1: fatal error
 *  0: no more data
 * >0: size of record (in words)
 */
int
read_record(int p, ems_u32** buf)
{
    ems_u32 head[2], *b;
    ems_u32 size;
    int wenden;

    switch (xread(p, (char*)head, 8)) {
    case -1: // error
        printf("read header: %s\n", strerror(errno));
        return -1;
    case 0:
        printf("read: end of file\n");
        return 0;
    default: // OK
        break;
    }

    //for (int i=0; i<2; i++) printf("head: 0x%08x\n", head[i]);
    switch (head[1]) {
    case 0x12345678:
        wenden=0;
        size=head[0];
        break;
    case 0x78563412:
        wenden=1;
        size=swap_int(head[0]);
        break;
    default:
        printf("unknown endian 0x%08x\n", head[1]);
        return -1;
    }

    b=new ems_u32[size+1];
    if (!b) {
        printf("new buf: %s\n", strerror(errno));
        return -1;
    }

    b[0]=head[0];
    b[1]=head[1];
    switch (xread(p, (char*)(b+2), (size-1)*4)) {
    case -1: // error
        printf("read body: %s\n", strerror(errno));
        return -1;
    case 0:
        printf("read body: unexpected end of file\n");
        return -1;
    default: // OK
        break;
    }

    if (wenden) {
        for (int i=size; i>=0; i--) {
            ems_u32 v=b[i];
            b[i]=swap_int(v);
        }
    }
    //for (int i=0; i<6; i++) printf("0x%08x\n", b[i]);
    *buf=b;
    return size+1;
}
//---------------------------------------------------------------------------//
int
archive_file(dsUint32_t sh, int p, struct l_file *ff, const char *stub, int events)
{
    tsmObjName obj_name;
    struct ems_info emsinfo;
    struct stat stbuf;

    // remove .gz o.a. from original name
    string name(ff->d.objName.ll);
    string::size_type pos=name.find_last_of('.');
    if ((pos!=string::npos) && (name.size()-pos<5))
            name.erase(pos);
    name+="_";
    name+=stub;
    name+=".tade";
    name+=comp_ext;

    memset(&obj_name, 0, sizeof(tsmObjName));
    obj_name.objType=DSM_OBJ_FILE;
    strncpy(obj_name.fs, destspace.c_str(), DSM_MAX_FSNAME_LENGTH);
    strncpy(obj_name.hl, ff->d.objName.hl, DSM_MAX_HL_LENGTH);
    strncpy(obj_name.ll, name.c_str(), DSM_MAX_LL_LENGTH);

    emsinfo=ff->info;
    emsinfo.version=0;
    emsinfo.events=events;

    cerr<<"begin to save "<<name<<endl;;

    if (fstat(p, &stbuf)<0) {
        cerr<<"stat "<<name<<": "<<strerror(errno)<<endl;
        return -1;
    }

    if (archive_file(sh, p, &stbuf, &obj_name, emsinfo.digest, 0, 0, 0, 0)<0) {
        cerr<<"failed to archive"<<name<<endl;
        return -1;
    }

    {
        unsigned char info[DSM_MAX_OBJINFO_LENGTH];
        int len;
        len=encode_emsinfo(info, &emsinfo);

        if (update_object_info(sh, &obj_name, info, len)) {
            cerr<<"failed to update object "<<name<<endl;
        }
    }

    cerr<<name<<" saved"<<endl;
    return 0;
}
//---------------------------------------------------------------------------//
int
process_file(struct l_file *ff)
{
    dsUint32_t sh;
    compressed_output output[4];
    const char *stub[]={"even", "odd", "scaler", "d15d16"};
    int nevents[4]={0, 0, 0, 0};
    ems_object ems;
    ems_u32 *buf;
    int path;
    int res, i, rres, wres;
    int error=0;

    cl2tade::clear();

    // create 'temporary' file name
    ostringstream st;
    st<<tmpdir<<"/run_"<<setw(4)<<setfill('0')<<ff->info.runnr;
    string oname(ff->d.objName.ll);
    string fname(st.str());

    string::size_type pos=oname.find_last_of('.');
    if ((pos!=string::npos) && (oname.size()-pos<5))
            fname+=oname.substr(pos);

    int flags=O_RDWR|O_CREAT|LINUX_LARGEFILE|(force?O_TRUNC:O_EXCL);
    path=open(fname.c_str(), flags, 0644);
    if (path<0) {
        cerr<<"create \""<<fname<<"\": "<<strerror(errno)<<endl;
        return -1;
    }

    printf("login to tsm\n");
    res=tsm_login(args, &sh);
    if (res<1) {
        close(path);
        return -1;
    }
    cerr<<"begin to retrieve "<<(ff->d.objName.ll+1)
        <<" (run "<<ff->info.runnr<<")"<<endl;
    // file is a simulated list of files with only 1 element
    res=retrieve_file(sh, ff, path, 0, 0);
    if (res<0) {
        close(path);
        return -1;
    }
    printf("successfully retrieved; logout from TSM\n");
    close_session(sh);

    lseek(path, SEEK_SET, 0);

    compressed_input input(path);
    if (!input.good()) {
        cerr<<"error creating input chain for "<<fname<<endl;
        // path is closed automatically in case of error
        return -1;
    }

    // we need the run number before we can open the output files
    // parse initial records but stop at the first event cluster
    do {
        rres=read_record(input.path(), &buf);
        if (rres>0) {
            if (buf[2]==clusterty_events) {
                // we have to open the output files first
                break;
            }
            wres=parse_record(buf+2, rres-2, ems, output, nevents);
            delete[] buf;
            if (wres) {
                error=1;
                break;
            }
        } else {
            error=1;
            break;
        }
    } while (1);

    if (error) {
        cerr<<"file "<<fname<<" does not contain events"<<endl;
        return -1;
    }

    // read the replacement detdef, if any
    if (!args->isdefault("detdef")) {
        res=ems.replace_file_object("detdef.lis", args->getstringopt("detdef"));
        if (res<0)
            return -1;
    }

    if (!ems.runnr_valid()) {
        cerr<<"file "<<fname<<" does not contain the run number"<<endl;
        return -1;
    }

    file_object& detdef=ems.find_file_object("detdef.lis");
    res=cl2tade::parse_detdef(detdef, /*ignore_duplicate*/false);
    if (res<0)
        return -1;

    // create three tfiles for 'even' 'odd' and 'scaler'
    for (int i=0; i<4; i++) {
        char tname[1024];
        snprintf(tname, 1024, "%s/run_%04d_%s.tade%s",
                tmpdir, ems.runnr(), stub[i], comp_ext);

        output[i].open(tname, comp_type, force);
        output[i].fdopen();
        if (!output[i].good()) {
            cerr<<"error opening output for "<<tname<<endl;
            // leave all other descriptors open, the program will terminate
            return -1;
        }
    }

    //ems.ignore_missing=ignore_missing;
    ems.ignore_missing=false;

    // process the first buffered event record
    wres=parse_record(buf+2, rres-2, ems, output, nevents);
    delete[] buf;
    if (wres) {
        return -1;
    }

    do {
        rres=read_record(input.path(), &buf);
        if (rres>0) {
            wres=parse_record(buf+2, rres-2, ems, output, nevents);
            delete[] buf;
            if (wres) {
                error=1;
                break;
            }
        } else {
            error=1;
            break;
        }
    } while (1);

    input.close();

    for (int i=0; i<4; i++) {
        output[i].flush();
    }

    cerr<<ems.next_evno-1<<" events converted"<<endl;
    cerr<<"events[0]="<<nevents[0]<<endl;
    cerr<<"events[1]="<<nevents[1]<<endl;
    cerr<<"events[2]="<<nevents[2]<<endl;
    cerr<<"events[3]="<<nevents[3]<<endl;
    if (ems.ignored_missings)
        cerr<<ems.ignored_missings<<" hits ignored (no detector definition)"<<endl;

    printf("login to tsm\n");
    res=tsm_login(args, &sh);
    if (res<1) {
        return -1;
    }
    for (i=0; i<4; i++) {
        archive_file(sh, output[i].path(), ff, stub[i], nevents[i]);
        output[i].close();
    }
    printf("logout from TSM\n");
    close_session(sh);
    if (args->getboolopt("deltemps")) {
        remove(fname.c_str());
        for (i=0; i<4; i++) {
            remove(output[i].name().c_str());
        }
    }
    return 0;
}
//---------------------------------------------------------------------------//

}; //end of anonymous namespace

int
main(int argc, char* argv[])
{
    dsUint32_t sh;
    struct l_file *files, *ff;
    int res, ret=0;

    cerr<<"tsm_cl2tade "<<__DATE__<<" "<<__TIME__<<endl;

    args=new C_readargs(argc, argv);
    if (readargs())
        return 1;

    res=tsm_login(args, &sh);
    if (res<1)
        return -res;

    // query all files of this filespace
    res=query_files(sh, filespace.c_str(), "*", "/*", DSM_OBJ_FILE, &files, 0, 0);
    close_session(sh);
    if (res<0) {
        /* 'files' is corrupted at this point; DONT free it! */
        ret=3;
        goto raus;
    }
    //dump_files(files, 1, (tsm_emsinfos)emsinfo_run);
    decode_emsinfo(files, 1);

    // mark all files as "invalid" (unwanted)
    {
        struct l_file* f=files;
        while (f) {f->valid=0; f=f->next;}
    }

    // select files
    res=select_files(files);
    if (res!=0) {
        printf("%d file%s not found\n", res, res!=1?"s":"");
        ret=4;
        goto raus;
    }

    // remove all unwanted files from list
    clean_file_list(&files);

    if (!files)
        goto raus; // ret=0

    if (verbose)
        dump_files(files, 1, "rs");

    ff=files;
    while (ff) {
        res=process_file(ff);
        if (res<0)
            break;
        ff=ff->next;
    }

    free_l_files(files);

raus:
    return ret;
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
