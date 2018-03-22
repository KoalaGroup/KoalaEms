/*
 * dataout/cluster/do_cluster_file.c
 * created      26.05.98 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: do_cluster_file.c,v 1.37 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <glob.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif
#include <sys/param.h>
#include <sys/mount.h>
#include <unsoltypes.h>
#include <get_mount_dev.h>
#include <human_size.h>
#include <rcs_ids.h>
#include "do_cluster.h"
#include "../../main/scheduler.h"
#include "../../main/childhndlr.h"
#include "../../objects/domain/dom_dataout.h"
#include "../../objects/pi/readout.h"
#include "../../objects/ved/ved.h"
#include "../../commu/commu.h"

#ifdef __linux__
#define LINUX_LARGEFILE O_LARGEFILE
#else
#define LINUX_LARGEFILE 0
#endif

#if 1
#    define LOG(fmt, arg...)                     \
        do {                                     \
            printf("%s:%d " fmt "\n",            \
            "do_cluster_file.c", \
            __LINE__, ## arg);                   \
        } while (0)
#else
    #define LOG(fmt, arg...)
#endif

/* We use a filesystem only if it has at least 2GByte of free space */
#define MINSPACE (2UL*1024*1024*1024)

struct do_special_file {
    int path;
    pid_t filter;
    char filename[MAXPATHLEN+1];
    char filtername[MAXPATHLEN+1];
    time_t next_reopen; /* time of next reopen; 0 if no reopen required */
    time_t reopen_interval; /* interval for reopen; 0 if no reopen required */
};

static int globidx=0;

RCS_REGISTER(cvsid, "dataout/cluster")

/*****************************************************************************/
static int
announce_fileaction(int do_idx, const char* code, char* text)
{
    struct do_special_file *special=
            (struct do_special_file*)dataout_cl[do_idx].s;
    char* logfilename=dataout[do_idx].logfilename;
    FILE* f;
    struct timeval tv;
    struct tm* tm;
    time_t tt;
    char ts[50];

    if (!logfilename || (logfilename[0]==0)) {
        /*printf("do_cluster_file.c:announce_fileaction: no logfile given!\n");*/
        return 0;
    }

    f=fopen(logfilename, "a");
    if (!f) {
        complain("open dataout logfile \"%s\": %s\n",
                logfilename, strerror(errno));
        return-1;
    }

    gettimeofday(&tv, 0);
    tt=tv.tv_sec;
    tm=gmtime(&tt);
    strftime(ts, 50, "%Y-%m-%d_%H:%M:%S", tm);
    /*printf("printing %s %s %s\n", ts, code, special->filename);*/
    fprintf(f, "%s %s %s", ts, code, special->filename);
    if (text && text[0]) fprintf(f, " %s", text);
    fprintf(f, "\n");
    fclose(f);
    return 0;
}
/*****************************************************************************/
static int
make_runlink_abs(const char *runlinkdir, const char *filename)
{
    char linkname[1024];

    snprintf(linkname, 1024, "%s/run_%04d", runlinkdir, ved_globals.runnr);
    LOG("symlink(%s, %s)", filename, linkname);
    if (symlink(filename, linkname)<0) {
        complain("make run_link \"%s\" -> \"%s\": %s\n",
                filename, linkname, strerror(errno));
        return -1;
    }

    return 0;
}
/*****************************************************************************/
static int
make_runlink_rel(const char *runlinkdir, const char *filename)
{
    char *linkfile=0, *linkname=0;
    char *realdir, *realdir_saved=0;
    char *realname, *realname_saved=0;
    char *p0, *p1;
    int res=0, i, n;

    /* normalise names */
    realdir=realpath(runlinkdir, 0); /* using glibc extension */
    if (!realdir) {
        complain("make_runlink_rel: realpath(%s): %s\n",
                runlinkdir, strerror(errno));
        res=-1;
        goto error;
    }
    realdir_saved=realdir;

    realname=realpath(filename, 0); /* using glibc extension */
    if (!realname) {
        complain("make_runlink_rel: realpath(%s): %s\n",
                filename, strerror(errno));
        res=-1;
        goto error;
    }
    realname_saved=realname;

    /* remove common leading part of both strings */
    n=0;
    while (1) {
        p0=strchr(realdir, '/');
        p1=strchr(realname, '/');
        if ((p0-realdir)==(p1-realname)) {
            n=p0-realdir;
            if (strncmp(realdir, realname, n))
                break;
            realdir=p0+1;
            realname=p1+1;
        } else {
            break;
        }
    }

    /* count parts of realdir */
    n=1; p0=realdir;
    while ((p0=strchr(p0, '/'))) {
        p0++;
        n++;
    }

    /* generate linkfile */
    linkfile=calloc(n*3+strlen(realname)+1, 1);
    p0=linkfile;
    for (i=0; i<n; i++)
        p0+=sprintf(p0, "../");
    sprintf(p0, "%s", realname);

    /* generate linkname */
    linkname=malloc(sizeof(realdir)+25+1);
    sprintf(linkname, "%s/run_%04d", realdir, ved_globals.runnr);

    LOG("symlink(%s, %s)", filename, linkname);
    if (symlink(linkfile, linkname)<0) {
        complain("make run_link \"%s\" -> \"%s\": %s\n",
                filename, linkname, strerror(errno));
        res=-1;
        goto error;
    }

error:
    free(linkfile);
    free(linkname);
    free(realdir_saved);
    free(realname_saved);
    return res;
}
/*****************************************************************************/
static int
make_runlink(int do_idx)
{
    struct do_special_file *special=
            (struct do_special_file*)dataout_cl[do_idx].s;
    char* runlinkdir=dataout[do_idx].runlinkdir;

    if (!runlinkdir || (ved_globals.runnr==-1)) {
        return 0;
    }

    if (dataout[do_idx].runlinkabsolute)
        return make_runlink_abs(runlinkdir, special->filename);
    else
        return make_runlink_rel(runlinkdir, special->filename);
}
/*****************************************************************************/
static void
unsol_filename(int do_idx)
{
    struct do_special_file *special=
            (struct do_special_file*)dataout_cl[do_idx].s;

#if 0
    if (!dataout[do_idx].allow_notify)
        return;
#endif

    send_unsol_do_filename(do_idx, special->filename);
}
/*****************************************************************************/
static int
announce_file_closed(int do_idx)
{
    return announce_fileaction(do_idx, "FINISHED", dataout[do_idx].loginfo);
}
/*****************************************************************************/
static int
announce_file_opened(int do_idx)
{
    make_runlink(do_idx);
    unsol_filename(do_idx);
    return announce_fileaction(do_idx, "CREATED ", dataout[do_idx].loginfo);
}
/*****************************************************************************/
static int
announce_file_suspect(int do_idx)
{
    return announce_fileaction(do_idx, "SUSPECT ", dataout[do_idx].loginfo);
}
/*****************************************************************************/
static int
announce_file_error(int do_idx)
{
    return announce_fileaction(do_idx, "ERROR   ", dataout[do_idx].loginfo);
}
/*****************************************************************************/
/*
 * checks whether path1 and path2 (symbolic links) point to the same filesystem
 * the files pointed to dont need to exist
 * It returns -1 in case of error.
 * It returns 0 if they point to the same file system.
 * It returns 1 otherwise
 */
static int
not_same_fs(const char *path1, const char *path2)
{
    dev_t dev1, dev2;
    int res;

    if ((res=get_mount_dev(path1, &dev1, stderr))!=0)
        return res;
    if ((res=get_mount_dev(path2, &dev2, stderr))!=0)
        return res;

    if (dev1==dev2)
        return 0;
    else
        return 1;
}
/*****************************************************************************/
static void
free_tsm_lock(__attribute__((unused)) int do_idx)
{
    if (daq_lock_link && daq_lock_link[0]) {
        if (unlink(daq_lock_link)<0)
            complain("unlink lock_link %s: %s", daq_lock_link, strerror(errno));
    }
}
/*****************************************************************************/
/*
 * make_tsm_lock tries to verify that TSM is not reading from the same
 * file system
 * It maintains a symbolic link to the data file to be written and checks
 * a symbolic link to the data file to be read by TSM.
 * It returns -1 in case of error.
 * It returns 0 and delets the link if TSM is reading from the same file system.
 * It returns 1 if TSM is not reading from the same file system.
 */
static int
make_tsm_lock(int do_idx, int forced)
{
    struct do_special_file *special=
            (struct do_special_file*)dataout_cl[do_idx].s;
#if 0
    char absname[PATH_MAX+1];
#endif
    int res=1;

    if (!daq_lock_link || !daq_lock_link[0])
        return 1; /* no information available */
    if (!tsm_lock_link || !tsm_lock_link[0]) {
        complain("make_tsm_lock: daq_lock_link is set, but not tsm_lock_link.");
        return -1;
    }

    unlink(daq_lock_link);
    if (symlink(special->filename, daq_lock_link)<0) {
        complain("make_tsm_lock: link %s to %s: %s",
                daq_lock_link, special->filename, strerror(errno));
        return -1;
    }

    if (!forced) {
        res=not_same_fs(daq_lock_link, tsm_lock_link);
        if (res==0)
            free_tsm_lock(do_idx);
        /* res<0 (error) cannot be handled and is therefore ignored */
    }

    return res;
}
/*****************************************************************************/
static void
filechildhandler(pid_t pid, int status, union callbackdata data)
{
    int do_idx=data.i;
    const char *info=0;
    int fatal=0;

/*
 * Because filechildhandler is called 'asynchronously' we have to check
 * whether the dataout object still exists and information about filters
 * is valid.
 */
    if (dataout_cl[do_idx].s) {
        struct do_special_file* special=
            (struct do_special_file*)dataout_cl[do_idx].s;
        info=special->filter==pid?"":"old ";
    } else {
        info="orphaned ";
    }

    T(cluster/do_cluster_file:filechildhandler)

    if (WIFEXITED(status)) {
        int exitstatus=WEXITSTATUS(status);
        complain("%sfilter for dataout %d terminated (pid=%d); exitcode %d\n",
                info, do_idx, pid, exitstatus);
        fatal=1;
    }
    if (WIFSIGNALED(status)) {
        int signal=WTERMSIG(status);
        complain("%sfilter for dataout %d terminated (pid=%d); signal %d\n",
                info, do_idx, pid, signal);
        fatal=1;
    }
    if (WIFSTOPPED(status)) {
        int signal=WSTOPSIG(status);
        complain("%sfilter for dataout %d suspended (pid=%d); signal %d\n",
                info, do_idx, pid, signal);
    }
    #ifdef __osf__
    if (WIFCONTINUED(status)) {
        complain("%sfilter for dataout %d reactivated (pid=%d)\n",
                info, do_idx, pid);
    }
    #endif
    if (fatal) {
        install_childhandler(0, pid, data, "filechildhandler/fatal");
        if (dataout_cl[do_idx].s &&
                ((struct do_special_file*)dataout_cl[do_idx].s)->filter==pid)
            ((struct do_special_file*)dataout_cl[do_idx].s)->filter=0;
    }
}
/*****************************************************************************/
static int
separate_filter_and_filename(const char* name, char* filtername, char* filename)
{
    char* p;
    int flen;

    p=strchr(name, '|');
    if (!p) {
        strncpy(filename, name, MAXPATHLEN);
        *filtername=0;
        return 0;
    }
    strncpy(filename, p+1, MAXPATHLEN);
    filename[MAXPATHLEN]=0;
    flen=p-name<MAXPATHLEN?p-name:MAXPATHLEN;
    strncpy(filtername, name, flen);
    filtername[flen]=0;
    return 0;
}
/*****************************************************************************/
static int
expand_filename_glob(char* filename, int *choices)
{
    char globstr[MAXPATHLEN+1];
    glob_t globbuf;
    char *p; 
    unsigned int i;
    int res;

    struct globstat {
        char* name;
        struct statvfs stbuf;
        unsigned long long size;
        int valid;
    };
    struct globstat* globstat;
    int* valids;
    unsigned int idx;

    p=strstr(filename, "//");

    if (!p)
        return 0;

    strncpy(globstr, filename, p-filename);
    globstr[p-filename]=0;
    LOG("expand_filename: vor //  : %s", globstr);
    LOG("expand_filename: nach // : %s", p+2);

    memset(&globbuf, 0, sizeof(glob_t));
    res=glob(globstr, 0/*|GLOB_MARK*//*|GLOB_NOCHECK*/, 0 , &globbuf);
    if (!res && !globbuf.gl_pathc)
        res=GLOB_NOMATCH;
    if (res!=0) {
        char s[MAXPATHLEN+64];
        sprintf(s, "do_cluster_file.c: glob(%s) failed: ", globstr);
        switch (res) {
        case GLOB_NOSPACE:
            sprintf(s+strlen(s), "NOSPACE");
            break;
        case GLOB_ABORTED:
            sprintf(s+strlen(s), "ABORTED");
            break;
        case GLOB_NOMATCH:
            sprintf(s+strlen(s), "NOMATCH");
            break;
        default:
            sprintf(s+strlen(s), "%d", res);
        }
        printf("%s\n", s);
        send_unsol_text(s, 0);
        return -1;
    }

    globstat=malloc(globbuf.gl_pathc*sizeof(struct globstat));
    valids=malloc(globbuf.gl_pathc*sizeof(int));
    for (i=0, idx=0; i<globbuf.gl_pathc; i++) {
        char* sp=globbuf.gl_pathv[i];
        globstat[i].name=sp;
        globstat[i].valid=0;
        res=statvfs(sp, &globstat[i].stbuf);
        if (res<0) {
            complain("do_cluster_file.c:glob:statfs(%s): %s",
                    sp, strerror(errno));
        } else {
            globstat[i].size=(unsigned long long)globstat[i].stbuf.f_bsize*
                    (unsigned long long)globstat[i].stbuf.f_bavail;
            if (globstat[i].size>=MINSPACE) {
                globstat[i].valid=1;
            } else {
                complain("do_cluster_file: %s cannot be used, "
                        "it is (nearly) full", sp);
            }
        }
        if (globstat[i].valid) {
            valids[idx++]=i;
        }
    }
    *choices=idx;
    if (idx==0) {
        complain("do_cluster_file: no valid pathes found using %s", filename);
        return -1;
    }
    printf("found %d valid pathes:\n", idx);
    for (i=0; i<idx; i++) {
        printf("  %s (%sByte)\n", globstat[valids[i]].name,
                human_size(globstat[valids[i]].size));
    }
    globidx++;
    globidx%=idx;

    snprintf(filename, MAXPATHLEN, "%s/%s", globstat[valids[globidx]].name,
            p+2);
    printf("do_cluster_file.c:glob: selected path: %s\n", filename);
    free(globstat);
    free(valids);
    globfree(&globbuf);
    return 0;
}
/*****************************************************************************/
/*
 * if '%q' occures in the filename %q is replaced by the run_number (if set)
 */
static int
expand_filename_runnr(char* filename)
{
    char help[MAXPATHLEN+1];
    char *p;

    if (!strstr(filename, "%q"))
        return 0;
    if (ved_globals.runnr==-1) {
        complain("do_cluster_file.c: filename contains %%q but runnr is not set\n");
        return -1;
    }

    /* XXX possible buffer overflow */
    while ((p=strstr(filename, "%q"))) {
        strcpy(help, filename);
        sprintf(p, "%d", ved_globals.runnr);
        strcat(filename, help+(p-filename+2));
    }

    return 0;
}
/*****************************************************************************/
/*
 * convert filename by using it as format for strftime
 * the recommended format for time is %Y-%m-%d_%H-%M-%S because it is
 * expected by the tsm progams
 */
static int
expand_filename_time(char* filename, struct timeval *now)
{
    char help[MAXPATHLEN+1];
    struct tm* tm;
    time_t tt;

    if (!strchr(filename, '%'))
        return 0;

    strcpy(help, filename);

    tt=now->tv_sec;
    tm=gmtime(&tt);
    strftime(filename, MAXPATHLEN, help, tm);
    return 0;
}
/*****************************************************************************/
static int
make_directories(char* filename)
{
    char help[MAXPATHLEN+1];
    char *p0, *p1;

    /* leading / is part of filename */
    p0=filename+1;
    if (!strchr(p0, '/'))
        return 0;

    while ((p1=strchr(p0, '/'))) {
        strncpy(help, filename, p1-filename);
        help[p1-filename]=0;
        /*printf("create dir %s\n", help);*/
        LOG("mkdir(%s)", help);
        mkdir(help, 0755);
        p0=p1+1;
    }

    return 0;
}
/*****************************************************************************/
static void
do_child(char* filtername, int in, int out)
{
    const char* delim=" ";
    char* argv[20]; /* XXX arbitrary limit */
    int argc, res, i;
    char *f, *p;

    f=strdup(filtername);
    printf("filtername=>%s<\n", f);

    argc=0;
    if ((p=strtok(f, delim))) {
        argv[argc]=p;
        argc++;
        while ((p=strtok(0, delim))) {
            if (argc<20) argv[argc]=p;
            argc++;
        }
        if (argc<20) {
            argv[argc]=0;
        } else {
            complain("do_cluster_file.c:do_child: more than 20 tokens in "
                    "filtername (%d)\n", argc);
            free(f);
            return;
        }
    } else {
        complain("do_cluster_file.c:do_child: empty filtername\n");
        free(f);
        return;
    }

    for (i=0; i<argc; i++) {
        printf("argv[%d]=>%s<\n", i, argv[i]);
    }
    
    if ((in<2) || (out<2)) {
        printf("WARNING: do_cluster_file.c:do_child: in=%d out=%d\n",
                in, out);
    }

    res=dup2(in, 0);
    if (res<0) {
        complain("do_cluster_file.c:do_child: child: dup2(in, 0): %s\n",
                strerror(errno));
        return;
    }
    close(in);

    res=dup2(out, 1);
    if (res<0) {
        complain("do_cluster_file.c:do_child: child: dup2(out, 1): %s\n",
                strerror(errno));
        return;
    }
    close(out);

    execvp(argv[0], argv);
    printf("do_cluster_file_start: exec %s: %s\n", filtername, strerror(errno));
}
/*****************************************************************************/
static pid_t
open_filter(int* p, char* filtername)
{
    int pp[2];
    pid_t pid;
    int res;

    res=pipe(pp);
    if (res) {
        complain("do_cluster_file_start: pipe: %s\n", strerror(errno));
        return 0;
    }

    pid=fork();
    if (pid<0) { /* error */
        complain("do_cluster_file_start: fork: %s\n", strerror(errno));
        return 0;
    }

    if (pid==0) { /* child */
        close(pp[1]);
        do_child(filtername, pp[0], *p);
        exit(255);
    } else { /* parent */
        close(*p);
        *p=pp[1];
        close(pp[0]);
    }

    return pid;
}
/*****************************************************************************/
/*
 * structure of "filename":
 * [filter|][dirname//]filename
 * 
 * Outputdata are piped through filter (if given) before
 * written to dirname/filename.
 * 
 * Dirname can (and should) contain wildcards (like shell glob function).
 * The resulting pathes should point to different independent volumes.
 * All pathes with at least 2 GByte free space will be used in turn.
 * Only the index of the last used path is saved between invocations of
 * do_cluster_file_start. If the reevaluation of glob(dirname) gives an
 * other sequence of pathes an arbitrary jump can happen.
 * If the resulting pathname contains '%q' %q is replaced by the actual
 * run number
 * The complete resulting pathname is used as format for strftime.
 * UTC is used for this substitution.
 *
 * Pathes up to // have to exist; pathes after // will be created if necessary.
 *
 * Example:
 * "/bin/gzip -6|/local/data[01]//ems/%Y-%m-%d/%Y-%m-%d_%H:%M:%S"
 *
 */
static
errcode do_cluster_file_open(int do_idx)
{
    /*
    called from start_readout or reopen
    */
    struct do_special_file* special=
            (struct do_special_file*)dataout_cl[do_idx].s;
    union callbackdata data;
    int p, res, choices=0, loop=0, again;
    data.i=do_idx;
    struct timeval now;

    T(cluster/do_cluster_file.c:do_cluster_file_start)
#if 0
    printf("file_start: &dataout[%d].logfilename=%p\n", do_idx,
            &dataout[do_idx].logfilename);
    printf("file_start: dataout[%d].logfilename=%p\n", do_idx,
            dataout[do_idx].logfilename);
    printf("file_start: dataout[%d].logfilename=%s\n", do_idx,
            dataout[do_idx].logfilename);
#endif

    gettimeofday(&now, 0);
    special->reopen_interval=dataout[do_idx].address_arg_num>0?
        dataout[do_idx].address_args[0]:0;
    if (special->reopen_interval) {
        special->next_reopen=(now.tv_sec/special->reopen_interval+1)*
                special->reopen_interval;
    } else {
        special->next_reopen=0;
    }

    do {
        int lock;
        if (separate_filter_and_filename(dataout[do_idx].addr.filename,
                special->filtername, special->filename)<0) {
            return Err_System;
        }
        if (expand_filename_glob(special->filename, &choices)<0) {
            return Err_System;
        }
        if (expand_filename_runnr(special->filename)<0) {
            return Err_System;
        }
        if (expand_filename_time(special->filename, &now)<0) {
            return Err_System;
        }
        again=0;
        lock=make_tsm_lock(do_idx, 0);
        if (lock<0)
            return Err_System;
        if (lock==0) {
            if (choices>++loop) {
                complain("filesystem for %s is used by TSM, "
                        "retry with next choice",
                    special->filename);
                again=1;
            } else {
                complain("filesystem for %s is used by TSM, "
                        "but there is no choice",
                    special->filename);
                lock=make_tsm_lock(do_idx, 1);
                if (lock<0)
                    return Err_System;
            }
        }
    } while (again);

    if (make_directories(special->filename)<0)
        goto error;

    LOG("do_cluster_file_start: filter=>%s<", special->filtername);
    LOG("do_cluster_file_start: file  =>%s<", special->filename);

    LOG("open(%s)", special->filename);
    p=open(special->filename,
                O_WRONLY|O_CREAT|O_EXCL|LINUX_LARGEFILE, 0644);

    if (p<0) {
        complain("do_cluster_file_open(do_idx=%d): can't open file \"%s\": %s\n",
                do_idx, special->filename, strerror(errno));
        goto error;
    }

    if (special->filtername[0]) {
        special->filter=open_filter(&p, special->filtername);
        if (!special->filter) {
            complain("cannot open filter \"%s\"", special->filtername);
            close(p);
            goto error;
        }
        install_childhandler(filechildhandler, special->filter, data,
                "do_cluster_file_start");
    } else {
        special->filter=0;
    }
    fcntl(p, F_SETFD, FD_CLOEXEC);
//printf("do_cluster_file_open(idx=%d): special->path:=%d\n", do_idx, p);
    special->path=p;
#if 0
    printf("file_start: &dataout[%d].logfilename=%p\n", do_idx,
            &dataout[do_idx].logfilename);
    printf("file_start: dataout[%d].logfilename=%p\n", do_idx,
            dataout[do_idx].logfilename);
    printf("file_start: dataout[%d].logfilename=%s\n", do_idx,
            dataout[do_idx].logfilename);
#endif
    res=announce_file_opened(do_idx);
    if (res<0) {
        complain("announce_file_opened failed\n");
        close(p);
        goto error;
    }

    return OK;

error:
    free_tsm_lock(do_idx);
    return Err_System;
}
/*****************************************************************************/
static errcode do_cluster_file_start(int do_idx)
{
    /*
    called from start readout
    */
    struct do_cluster_data* dat=&dataout_cl[do_idx].d.c_data;
    errcode eres;

    T(cluster/do_cluster_file.c:do_cluster_file_start)

    eres=do_cluster_file_open(do_idx);
    if (eres!=OK)
        return eres;

    dataout_cl[do_idx].suspended=0;
    dataout_cl[do_idx].vedinfo_sent=0;

    do_statist[do_idx].clusters=0;
    do_statist[do_idx].words=0;
    do_statist[do_idx].events=0;
    do_statist[do_idx].suspensions=0;

    dat->active_cluster=0;
    dataout_cl[do_idx].advertised_cluster=0;

    dataout_cl[do_idx].do_status=Do_running;

    return OK;
}
/*****************************************************************************/
static plerrcode
do_cluster_file_filename(int do_idx, const char **name)
{
    struct do_special_file* special=
        (struct do_special_file*)dataout_cl[do_idx].s;
    *name=special->filename;
    return plOK;
}
/*****************************************************************************/
static errcode do_cluster_file_reset(int do_idx)
{
    struct do_special_file* special=
        (struct do_special_file*)dataout_cl[do_idx].s;

    T(cluster/do_cluster_file.c:do_cluster_file_reset)

    dataout_cl[do_idx].errorcode=0;
    if (dataout_cl[do_idx].do_status!=Do_done)
            dataout_cl[do_idx].do_status=Do_neverstarted;

    if (special->path>=0) {
        int res;

        close(special->path);
//printf("do_cluster_file_reset(idx=%d): special->path=-1\n", do_idx);
        special->path=-1;
        free_tsm_lock(do_idx);
        res=announce_file_closed(do_idx);
        if (res<0) {
            complain("announce_file_closed failed\n");
            return Err_System;
        }
    }

    return OK;
}
/*****************************************************************************/
static errcode
do_cluster_file_reopen(int do_idx)
{
    struct do_special_file* special=
        (struct do_special_file*)dataout_cl[do_idx].s;

    printf("do_cluster_file(%d): reopen file\n", do_idx);

    if (special->path>=0) {
        close(special->path);
//printf("do_cluster_file_reopen(idx=%d): special->path:=-1\n", do_idx);
        special->path=-1;
        free_tsm_lock(do_idx);
        if (announce_file_closed(do_idx)<0)
            complain("announce_file_closed failed\n");
    }

    return do_cluster_file_open(do_idx);
}
/*****************************************************************************/
errcode
do_cluster_file_check_reopen(int do_idx)
{
    struct do_special_file* special=
        (struct do_special_file*)dataout_cl[do_idx].s;
    struct timeval now;
    errcode eres;

    if (special->next_reopen==0)
        return OK;

    gettimeofday(&now, 0);
    if (now.tv_sec<=special->next_reopen)
        return OK;

    eres=do_cluster_file_reopen(do_idx);

    /* this check is probably not necessary (reopen is only called if
       reopen_interval is not zero), but who knows ... */
    if (special->reopen_interval) {
        special->next_reopen=(now.tv_sec/special->reopen_interval+1)*
                special->reopen_interval;
    } else {
        special->next_reopen=0;
    }

    return eres;
}
/*****************************************************************************/
static void do_cluster_file_freeze(int do_idx)
{
/*
called from fatal_readout_error()
stops all activities of this dataout, but leaves all other status unchanged
*/
    T(cluster/do_cluster_file.c:do_cluster_file_freeze)
    D(D_TRIG, printf("do_cluster_file_freeze(do_idx=%d)\n", do_idx);)
}
/*****************************************************************************/
static void do_cluster_file_patherror(int do_idx,
        __attribute__((unused)) int error)
{
    struct do_special_file* special=
        (struct do_special_file*)dataout_cl[do_idx].s;
    T(dataout/cluster/do_cluster_file.c:do_cluster_file_patherror)
    if (special->path>=0) {
        close(special->path);
//printf("do_cluster_file_patherror(idx=%d): special->path:=-1\n", do_idx);
        special->path=-1;
        free_tsm_lock(do_idx);
        announce_file_error(do_idx);
        fatal_readout_error();
    }
}
/*****************************************************************************/
static void do_cluster_file_cleanup(int do_idx)
{
    struct do_special_file* special=
        (struct do_special_file*)dataout_cl[do_idx].s;
    T(dataout/cluster/do_cluster_file.c:do_cluster_file_cleanup)

    if (special->path>=0) {
        close(special->path);
        free_tsm_lock(do_idx);
        announce_file_suspect(do_idx);
    }
    free(special);
    dataout_cl[do_idx].s=0;
//printf("do_cluster_file_cleanup(idx=%d)\n", do_idx);
}
/*****************************************************************************/
static void
do_cluster_file_advertise(int do_idx, struct Cluster* cl)
{
    struct do_special_file* special=
        (struct do_special_file*)dataout_cl[do_idx].s;
    struct dataout_info *do_info=dataout+do_idx;
    errcode err;
    int res, bytes;

    T(dataout/cluster/do_cluster_file.c:do_cluster_file_advertise)

    if (cl->predefined_costumers && !cl->costumers[do_idx]) return;

    /*
     * At this point we should give the dataout the chance to
     * switch to a new file.
     * This is not a good idea if our events consists of subevents
     * from different servers (and are saved in different clusters),
     * but if we have a single data source only this is a good idea.
     * The main use is for MQTT data.
     */
     if ((err=do_cluster_file_check_reopen(do_idx))!=OK) {
         dataout_cl[do_idx].errorcode=err;
         dataout_cl[do_idx].do_status=Do_error;
         if (dataout[do_idx].wieviel==0) {
             send_unsol_var(Unsol_RuntimeError, 4, rtErr_OutDev, do_idx, 0,
                    err);
             announce_file_error(do_idx);
             fatal_readout_error();
         }
         return;
     }

    bytes=cl->size*sizeof(int);
    if ((res=write(special->path, (char*)cl->data, bytes))!=bytes) {
        ems_u32 buf[4];
        int error;
        if (res<0) {
            error=errno;
        } else {
            /* try a second time to obtain a valid errno */
            bytes-=res;
            res=write(special->path, (char*)cl->data-res, bytes);
            if (res<0) {
                error=errno;
            } else {
                printf("do_cluster_file_write: res=%d after second try\n",
                        bytes);
                error=ENOSPC;
            }
        }
        buf[0]=rtErr_OutDev;
        buf[1]=do_idx;
        buf[2]=0;
        buf[3]=error;
        send_unsolicited(Unsol_RuntimeError, buf, 4);
        printf("do_cluster_file_write(path=%d, do_idx=%d, bytes=%d): %s\n",
                special->path, buf[1], bytes, strerror(error));
        dataout_cl[do_idx].do_status=Do_error;
        dataout_cl[do_idx].errorcode=errno;
        fatal_readout_error();
        dataout_cl[do_idx].procs.patherror(do_idx, -1);
    }

    if (cl->type==clusterty_no_more_data) {
/* XXX wieso passiert das hier schon, und nicht wenn alle Daten wirklich
geschrieben sind?
Reingefallen: File wird synchron geschrieben, da ist nix 'pending'
in write(special->path, ...) ist alles erledigt worden
*/
        dataout_cl[do_idx].do_status=Do_done;
        dataout_cl[do_idx].procs.reset(do_idx);
        notify_do_done(do_idx);
    }
    do_statist[do_idx].clusters++;
    do_statist[do_idx].words+=cl->size;

    if (do_info->fadvise_flag) {
#if 0
        ems_u64 mask=0xffffffffffffffffULL;
        /* XXX only vald for 4096 byte pages */
        ems_u64 pages=do_statist[do_idx].words/1024;
        mask<<=do_info->fadvise_data;
        mask=~mask;
        if (!(mask & pages)) {
#endif
        if (1) {
            if (do_info->fadvise_flag&2) {
                if (fdatasync(special->path)<0) {
                    printf("do_cluster_file_advertise: fdatasync(do=%d): %s\n",
                            do_idx, strerror(errno));
                }
            }
            if (do_info->fadvise_flag&1) {
                if ((res=posix_fadvise(special->path, 0, 0, POSIX_FADV_DONTNEED))!=0) {
                    printf("do_cluster_file_advertise: fadvise(do=%d): %s\n",
                            do_idx, strerror(res));
                }
            }
        }
    }
}
/*****************************************************************************/
static struct do_procs file_procs={
    do_cluster_file_start,
    do_cluster_file_reset,
    do_cluster_file_freeze,
    do_cluster_file_advertise,
    do_cluster_file_patherror,
    do_cluster_file_cleanup,
    /*wind*/ do_NotImp_err_ii,
    /*status*/ 0,
    do_cluster_file_filename,
};
/*****************************************************************************/
errcode do_cluster_file_init(int do_idx)
{
    struct do_special_file* special;

    T(dataout/cluster/do_cluster_file.c:do_cluster_file_init)

    special=calloc(1, sizeof(struct do_special_file));
    if (!special) {
        complain("cluster_file_init: %s", strerror(errno));
        return Err_NoMem;
    }
    dataout_cl[do_idx].s=special;

    dataout_cl[do_idx].procs=file_procs;
    dataout_cl[do_idx].do_status=Do_neverstarted;
    dataout_cl[do_idx].doswitch=
        readout_active==Invoc_notactive?Do_enabled:Do_disabled;

//printf("do_cluster_file_init(idx=%d): special->path:=-1\n", do_idx);
    special->path=-1;
    special->filter=0;
    special->filename[0]=0;
    special->filtername[0]=0;
    special->next_reopen=0;
    special->reopen_interval=0;
    dataout_cl[do_idx].seltask=0;
    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
