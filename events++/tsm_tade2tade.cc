/*
 * ems/events++/tsm_tade2tade.cc
 * 
 * created 2007-06-29 PW
 */

#include "config.h"
//#include "cxxcompat.hxx"

#include <readargs.hxx>
#include <errno.h>
#include <sys/wait.h>
#include "../tsm/tsm_tools.hxx"
#include "../tsm/tsm_tools_login.hxx"
#include "compressed_io.hxx"
#include <versions.hxx>

VERSION("2007-06-29", __FILE__, __DATE__, __TIME__,
"$ZEL$")
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
bool verbose, debug, replace, allow_run, force;

const char* optfile=".dsm.opt";
const char* tmpdir;
string filespace;
string destspace;

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
    args->addoption("replace",    "replace", false, "remove original file");
    args->addoption("allow_run",  "allownumber", true, "allow run numbers as file selector");
    args->addoption("objid",      "id", (ems_u64)0, "object id");
    args->addoption("tmpdir",     "tmpdir", "/var/tmp", "directory for temporary files");
    args->use_env  ("tmpdir",     "TMPDIR");
    args->addoption("deltemps",   "deltempfiles", true ,"delete temporary files");
    args->addoption("force",      "force", false, "force overwrite existing files");

    args->multiplicity("verbose", 1);
    args->hints(C_readargs::required, "filespace", "destspace", 0);

    delete[] home_opt;

    if (args->interpret(1)!=0)
        return -1;

    verbose=args->getboolopt("verbose");
    debug=args->getboolopt("debug");
    allow_run=args->getboolopt("allow_run");
    tmpdir=args->getstringopt("tmpdir");
    force=args->getboolopt("force");
    replace=args->getboolopt("replace");

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
        if (files->info.infotype==infotype_ems &&
            files->info.x.ems.flags&emsinfo_run) {
            if (files->info.x.ems.runnr==run)
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
        if (files->info.infotype==infotype_ems &&
            files->info.x.ems.flags&emsinfo_run) {
            if ((files->info.x.ems.runnr>=run0) &&
                (files->info.x.ems.runnr<=run1)) {
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
archive_file(dsUint32_t sh, int p, struct l_file *ff)
{
    tsmObjName obj_name;
    struct ems_info emsinfo;
    struct stat stbuf;

    // remove .gz o.a. from original name
    string name(ff->d.objName.ll);
    string::size_type pos=name.find_last_of('.');
    if ((pos!=string::npos) && (name.size()-pos<5))
            name.erase(pos);
    // add the actual extension (i.e. .bz2)
    name+=comp_ext;

    memset(&obj_name, 0, sizeof(tsmObjName));
    obj_name.objType=DSM_OBJ_FILE;
    strncpy(obj_name.fs, destspace.c_str(), DSM_MAX_FSNAME_LENGTH);
    strncpy(obj_name.hl, ff->d.objName.hl, DSM_MAX_HL_LENGTH);
    strncpy(obj_name.ll, name.c_str(), DSM_MAX_LL_LENGTH);

    emsinfo=ff->info.x.ems;
    emsinfo.version=0;

    cerr<<"begin to save "<<name<<endl;;

    if (fstat(p, &stbuf)<0) {
        cerr<<"stat "<<name<<": "<<strerror(errno)<<endl;
        return -1;
    }

    if (archive_file(sh, p, &stbuf, &obj_name, emsinfo.digest,
            dsmFalse,
            0, 0, 0, 0)<0) {
        cerr<<"failed to archive"<<name<<endl;
        return -1;
    }

    {
        unsigned char info[DSM_MAX_OBJINFO_LENGTH];
        int len;
        len=encode_emsinfo(info, &emsinfo);

        /* tsmUpdateObj cannot be called inside a transaction :-( */
        if (update_object_info(sh, &obj_name, info, len)) {
            cerr<<"failed to update object "<<name<<endl;
        }
    }

    cerr<<name<<" saved"<<endl;

    if (begin_transaction(sh)<0)
        return -1;

    if (replace) {
        // remove original file
        delete_file(sh, &ff->d);
        cerr<<"old file deleted"<<endl;
    }

    end_transaction(sh);

    return 0;
}
//---------------------------------------------------------------------------//
static int
tade2tade(compressed_input &in, compressed_output &out)
{
    char s[1024], junk[1024];
    int t, q, det, sub;
    int res;

    while (fgets(s, 1024, in.file())) {
        res=sscanf(s, "%d %d %d %d %s", &t, &q, &det, &sub, junk);
        if (res==4) {
            //cerr<<"q="<<t<<" t="<<q<<" det="<<det<<" sub="<<sub<<endl;
            if (det!=99)
                fprintf(out.file(), "%6d %5d %5d %5d\n", t, q, det, sub);
        } else {
            //cerr<<"res="<<res<<" junk="<<junk<<endl;
            fprintf(out.file(), "%s", s);
        }
    }
    return 0;
}
//---------------------------------------------------------------------------//
int
process_file(struct l_file *ff)
{
    dsUint32_t sh;
    int path;
    int res;

    // create 'temporary' file names
    string oname(tmpdir);
    oname+="/";
    oname+=(ff->d.objName.ll);

    string nname=oname;
    string::size_type pos=nname.find_last_of('.');
    if ((pos!=string::npos) && (nname.size()-pos<5))
            nname.erase(pos);
    nname+=".new";
    nname+=comp_ext;


    int flags=O_RDWR|O_CREAT|LINUX_LARGEFILE|(force?O_TRUNC:O_EXCL);
    path=open(oname.c_str(), flags, 0644);
    if (path<0) {
        cerr<<"create \""<<oname<<"\": "<<strerror(errno)<<endl;
        return -1;
    }

    bool retry;
    do {
        retry=false;
        printf("login to tsm\n");
        res=tsm_login(args, &sh);
        if (res<1) {
            close(path);
            return -1;
        }
        cerr<<"begin to retrieve "<<(ff->d.objName.ll+1)
            <<" (run "<<ff->info.x.ems.runnr<<")"<<endl;
        // file is a simulated list of files with only 1 element

        res=retrieve_file(sh, ff, path, 0, 0);
        if (res>=0) {
            printf("successfully retrieved; logout from TSM\n");
        } else if (res==-1) {
            close(path);
            return -1;
        } else {
            cerr<<"will retry in 1 minute"<<endl;
            retry=true;
        }
        close_session(sh);
        if (retry)
            sleep(60);

    } while (retry);

    lseek(path, SEEK_SET, 0);

    compressed_input input(path);
    if (!input.good()) {
        cerr<<"error creating input chain for "<<oname<<endl;
        // path is closed automatically in case of error
        return -1;
    }
    input.fdopen();

    // create the output file
    compressed_output output(nname, comp_type, force);
    if (!output.good()) {
        cerr<<"error opening output for "<<nname<<endl;
        // leave all other descriptors open, the program will terminate
        return -1;
    }
    output.fdopen();


    res=tade2tade(input, output);


    input.close();

    output.flush();

    if (res<0) {
        cerr<<"conversion failed"<<endl;
        output.close();
        if (args->getboolopt("deltemps")) {
            remove(input.name().c_str());
            remove(output.name().c_str());
        }
        return -1;
    }

    printf("login to tsm\n");
    res=tsm_login(args, &sh);
    if (res<1) {
        return -1;
    }
    archive_file(sh, output.path(), ff);
    output.close();

    printf("logout from TSM\n");
    close_session(sh);
    if (args->getboolopt("deltemps")) {
        remove(input.name().c_str());
        remove(output.name().c_str());
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

    cerr<<"tsm_tade2tade "<<__DATE__<<" "<<__TIME__<<endl;

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
    decode_tsminfo(files, 1);

    // mark all files as "invalid" (unwanted)
    {
        struct l_file* f=files;
        while (f) {
            f->valid=0;
            f=f->next;
        }
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
