/*
 * dataout/cluster/do_cluster_asynchfile.c
 * created 2007-Oct-24 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: do_cluster_asynchfile.c,v 1.1 2007/11/03 16:07:17 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif
#include <sys/param.h>
#include <sys/mount.h>
#include <unsoltypes.h>
#include "do_cluster.h"
#include "../../main/scheduler.h"
#include "../../objects/domain/dom_dataout.h"
#include "../../objects/pi/readout.h"
#include "../../objects/ved/ved.h"
#include "../../commu/commu.h"
#include "../../lowlevel/asynch_io/aio_tools.h"

static int globidx=0;

/*****************************************************************************/
static int
announce_fileaction(int do_idx, const char* code, char* text)
{
    char* logfilename=dataout[do_idx].logfilename;
    char* filename;
    FILE* f;
    struct timeval tv;
    struct tm* tm;
    time_t tt;
    char ts[50];

    if (!logfilename || (logfilename[0]==0)) {
        /*printf("do_cluster_asynchfile.c:announce_fileaction: no logfile given!\n");*/
        return 0;
    }
    filename=dataout_cl[do_idx].s.afile_data.filename;
    /*printf("dataout %d: logfile: %s filename: %s\n", logfilename, filename);*/

    f=fopen(logfilename, "a");
    if (!f) {
        char s[1024];
        printf("open dataout logfile \"%s\": %s\n",
                logfilename, strerror(errno));
        snprintf(s, 1024, "open dataout logfile \"%s\": %s",
                logfilename, strerror(errno));
        send_unsol_text(s, 0);
        return-1;
    }

    gettimeofday(&tv, 0);
    tt=tv.tv_sec;
    tm=gmtime(&tt);
    strftime(ts, 50, "%Y-%m-%d_%H:%M:%S", tm);
    fprintf(f, "%s %s %s", ts, code, filename);
    if (text && text[0])
        fprintf(f, " %s", text);
    fprintf(f, "\n");
    fclose(f);
    return 0;
}
/*****************************************************************************/
static int
make_runlink(int do_idx)
{
    char* runlinkdir=dataout[do_idx].runlinkdir;
    char* filename=dataout_cl[do_idx].s.afile_data.filename;
    char linkname[1024];

    if (!runlinkdir || (ved_globals.runnr==-1)) {
        return 0;
    }

    snprintf(linkname, 1024, "%s/run_%04d", runlinkdir, ved_globals.runnr);
    if (symlink(filename, linkname)<0) {
        printf("make run_link \"%s\" -> \"%s\": %s\n",
                filename, linkname, strerror(errno));
        return -1;
    }

    return 0;
}
/*****************************************************************************/
static int
announce_file_closed(int do_idx)
{
    make_runlink(do_idx);
    return announce_fileaction(do_idx, "FINISHED", dataout[do_idx].loginfo);
}
/*****************************************************************************/
static int
announce_file_opened(int do_idx)
{
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
expand_filename_glob(char* filename)
{
    char globstr[MAXPATHLEN+1];
    glob_t globbuf;
    char *p; 
    int res, i;

    struct globstat {
        char* name;
        struct statfs stbuf;
        unsigned long long size;
        int valid;
    };
    struct globstat* globstat;
    int* valids;
    int idx;

    p=strstr(filename, "//");

    if (!p)
        return 0;

    strncpy(globstr, filename, p-filename);
    globstr[p-filename]=0;
    printf("expand_filename: vor //  : %s\n", globstr);
    printf("expand_filename: nach // : %s\n", p+2);

    memset(&globbuf, 0, sizeof(glob_t));
    res=glob(globstr, 0/*|GLOB_MARK*//*|GLOB_NOCHECK*/, 0 , &globbuf);
    if (res!=0) {
        printf("do_cluster_asynchfile.c:glob failed, res=%d\n", res);
        send_unsol_text("do_cluster_asynchfile.c:glob", "failed", globstr, 0);
        return -1;
    }
    if (globbuf.gl_pathc==0) {
        printf("do_cluster_asynchfile.c:glob: no match with %s\n", globstr);
        send_unsol_text("do_cluster_asynchfile.c:glob", "no match", globstr, 0);
        return -1;
    }

    globstat=malloc(globbuf.gl_pathc*sizeof(struct globstat));
    valids=malloc(globbuf.gl_pathc*sizeof(int));
    for (i=0, idx=0; i<globbuf.gl_pathc; i++) {
        char* sp=globbuf.gl_pathv[i];
        globstat[i].name=sp;
        res=statfs(sp, &globstat[i].stbuf);
        if (res<0) {
            printf("statfs(%s): %s\n", sp, strerror(errno));
            send_unsol_text("do_cluster_asynchfile.c:glob:statfs", sp,
                    strerror(errno), 0);
            globstat[i].valid=0;
        } else {
            if (globstat[i].stbuf.f_bavail<0) {
                globstat[i].valid=0;
            } else {
                globstat[i].size=(unsigned long long)globstat[i].stbuf.f_bsize*
                        (unsigned long long)globstat[i].stbuf.f_bavail;
                globstat[i].valid=globstat[i].size>=2147483648UL;
            }
        }
        if (globstat[i].valid) {
            valids[idx++]=i;
        }
    }
    printf("found %d valid pathes:\n", idx);
    for (i=0; i<idx; i++) {
        printf("  %s (%llu)\n", globstat[valids[i]].name,
                globstat[valids[i]].size);
    }
    globidx++;
    globidx%=idx;

    snprintf(filename, MAXPATHLEN, "%s/%s", globstat[valids[globidx]].name,
            p+2);
    printf("do_cluster_asynchfile.c:glob: selected path: %s\n", filename);
    free(globstat);
    free(valids);
    globfree(&globbuf);
    return 0;
}
/*****************************************************************************/
static int
expand_filename_time(char* filename)
{
    char help[MAXPATHLEN+1];
    struct timeval tv;
    struct tm* tm;
    time_t tt;

    if (!strchr(filename, '%'))
        return 0;

    strcpy(help, filename);

    gettimeofday(&tv, 0);
    tt=tv.tv_sec;
    tm=gmtime(&tt);
    /* recommended time part of filename: %Y-%m-%d_%H:%M:%S */
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
        mkdir(help, 0755);
        p0=p1+1;
    }

    return 0;
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
 * do_cluster_asynchfile_start. If the reevaluation of glob(dirname) gives an
 * other sequence of pathes an arbitrary jump can happen.
 * The complete resulting pathname is used as format for strftime.
 * UTC is used for this substitution.
 *
 * Pathes up to // have to exist; pathes after // will be created if necessary.
 *
 * Example:
 * "/bin/gzip -6|/local/data[01]//ems/%Y-%m-%d/%Y-%m-%d_%H:%M:%S"
 *
 * !!! filter is (not yet?) implemented !!!
 *
 */

static errcode do_cluster_asynchfile_start(int do_idx)
{
    /*
    called from start readout
    */
    struct do_special_afile* special=&dataout_cl[do_idx].s.afile_data;
    struct do_cluster_data* dat=&dataout_cl[do_idx].d.c_data;
    char filtername[MAXPATHLEN+1];
    int res;

    T(cluster/do_cluster_asynchfile.c:do_cluster_asynchfile_start)
#if 0
    printf("file_start: &dataout[%d].logfilename=%p\n", do_idx,
            &dataout[do_idx].logfilename);
    printf("file_start: dataout[%d].logfilename=%p\n", do_idx,
            dataout[do_idx].logfilename);
    printf("file_start: dataout[%d].logfilename=%s\n", do_idx,
            dataout[do_idx].logfilename);
#endif
    if (separate_filter_and_filename(dataout[do_idx].addr.filename,
            filtername, special->filename)<0) {
        return Err_System;
    }
#if 0
printf("do_cluster_asynchfile_start: filter=>%s<\n", special->filtername);
printf("do_cluster_asynchfile_start: file  =>%s<\n", special->filename);
#endif
    if (filtername[0]) {
        printf("do_cluster_asynchfile: use of filter is not allowed\n");
        return Err_ArgRange;
    }

    if (expand_filename_glob(special->filename)<0) {
        return Err_System;
    }
    if (expand_filename_time(special->filename)<0) {
        return Err_System;
    }
    if (make_directories(special->filename)<0) {
        return Err_System;
    }

    if (async_init(special->filename, O_CREAT|O_WRONLY|O_EXCL|O_DIRECT, 0644,
            async_defbufsize(), async_defbufnum(), &special->hnd)<0) {
        printf("do_cluster_asynchfile_start(do_idx=%d): can't init file \"%s\": %s\n",
                do_idx, special->filename, strerror(errno));
        return Err_System;
    }

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
        printf("announce_file_opened failed\n");
        return Err_System;
    }

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
static errcode do_cluster_asynchfile_reset(int do_idx)
{
    struct do_special_afile* special=&dataout_cl[do_idx].s.afile_data;

    T(cluster/do_cluster_asynchfile.c:do_cluster_asynchfile_reset)

    dataout_cl[do_idx].errorcode=0;
    if (dataout_cl[do_idx].do_status!=Do_done)
            dataout_cl[do_idx].do_status=Do_neverstarted;

    if (special->hnd) {
        int res;

        if (async_done(&special->hnd)<0) {
            printf("cluster_asynchfile_reset: async_done failed: %s\n",
                    strerror(errno));
        }
        res=announce_file_closed(do_idx);
        if (res<0) {
            printf("announce_file_closed failed\n");
            return Err_System;
        }
    }

    return OK;
}
/*****************************************************************************/
static void do_cluster_asynchfile_freeze(int do_idx)
{
/*
called from fatal_readout_error()
stops all activities of this dataout, but leaves all other status unchanged
*/
    T(cluster/do_cluster_asynchfile.c:do_cluster_asynchfile_freeze)
    D(D_TRIG, printf("do_cluster_asynchfile_freeze(do_idx=%d)\n", do_idx);)
}
/*****************************************************************************/
static void do_cluster_asynchfile_patherror(int do_idx, int error)
{
    struct do_special_afile* special=&dataout_cl[do_idx].s.afile_data;
    T(dataout/cluster/do_cluster_asynchfile.c:do_cluster_asynchfile_patherror)
    if (special->hnd) {
        async_done(&special->hnd);
        announce_file_error(do_idx);
    }
}
/*****************************************************************************/
static void do_cluster_asynchfile_cleanup(int do_idx)
{
    struct do_special_afile* special=&dataout_cl[do_idx].s.afile_data;
    T(dataout/cluster/do_cluster_asynchfile.c:do_cluster_asynchfile_cleanup)

    if (special->hnd) {
        async_done(&special->hnd);
        announce_file_suspect(do_idx);
    }
}
/*****************************************************************************/
static void do_cluster_asynchfile_advertise(int do_idx, struct Cluster* cl)
{
    struct do_special_afile* special=&dataout_cl[do_idx].s.afile_data;
    int res, bytes;

    T(dataout/cluster/do_cluster_asynchfile.c:do_cluster_asynchfile_advertise)

    if (cl->predefined_costumers && !cl->costumers[do_idx]) return;
    bytes=cl->size*sizeof(int);

    res=async_write(special->hnd, cl->data, bytes);
    if (res!=bytes) {
        ems_u32 buf[4];
        buf[0]=rtErr_OutDev;
        buf[1]=do_idx;
        buf[2]=0;
        buf[3]=errno;
        send_unsolicited(Unsol_RuntimeError, buf, 4);
        printf("do_cluster_asynchfile_write(do_idx=%d, bytes=%d): %s\n",
                buf[1], bytes, strerror(errno));
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
}
/*****************************************************************************/
static struct do_procs file_procs={
    do_cluster_asynchfile_start,
    do_cluster_asynchfile_reset,
    do_cluster_asynchfile_freeze,
    do_cluster_asynchfile_advertise,
    do_cluster_asynchfile_patherror,
    do_cluster_asynchfile_cleanup,
    /*wind*/ do_NotImp_err_ii,
    /*status*/ 0,
};
/*****************************************************************************/
errcode do_cluster_asynchfile_init(int do_idx)
{
    struct do_special_afile* special=&dataout_cl[do_idx].s.afile_data;

    T(dataout/cluster/do_cluster_asynchfile.c:do_cluster_asynchfile_init)

    dataout_cl[do_idx].procs=file_procs;
    dataout_cl[do_idx].do_status=Do_neverstarted;
    dataout_cl[do_idx].doswitch=
        readout_active==Invoc_notactive?Do_enabled:Do_disabled;

    special->hnd=0;
    special->filename[0]=0;
    dataout_cl[do_idx].seltask=0;
    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
