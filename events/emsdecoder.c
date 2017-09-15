/*
 * clusterdecoder.c
 * created 08.Apr.2004 PW
 * $ZEL: emsdecoder.c,v 1.1 2006/02/17 15:15:23 wuestner Exp $
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <utime.h>
#include <unistd.h>
#include <libgen.h>

#include <emsctypes.h>
#include <xdrstring.h>
#include <clusterformat.h>
#include <swap.h>

#include "makepath.h"
#include "read_procfile.h"

#include "canonic_path.h"

static int ip;   /* input path; normally 0 */
static FILE* op; /* output path; normally stdin */
static FILE* ep; /* path for messages and statistics; normally stderr */

static const char* procfile;

static ems_u32* cldata;
static int clsize;
static int cldatasize;

static int clnum_events;
static int clnum_ved_info;
static int clnum_text;
static int clnum_wendy_setup;
static int clnum_file;
static int clnum_no_more_data;

static struct iss iss;

#define MAX_CLUSTERSIZE 16384 /* 64 KByte */

#define SWAP_32(n) (((n & 0xff000000) >> 24) | \
    ((n & 0x00ff0000) >>  8) | \
    ((n & 0x0000ff00) <<  8) | \
    ((n & 0x000000ff) << 24))

/******************************************************************************/
static void
printusage(char* argv0)
{
    fprintf(stderr, "usage: %s [-h]\n",
        basename(argv0));
    fprintf(stderr, "  -h: this help\n");
}
/*****************************************************************************/
static int
readargs(int argc, char* argv[])
{
    const char* args="hf:p:";
    int c, err;

    procfile=0;

    err=0;
    while (!err && ((c=getopt(argc, argv, args)) != -1)) {
        switch (c) {
        case 'h':
            printusage(argv[0]);
            return 1;
        case 'p':
            procfile=optarg;
            break;
        default:
            err=1;
        }
    }
    if (err) {
        printusage(argv[0]);
        return -1;
    }
    if (!procfile) {
        fprintf(stderr, "-p <procfile> is required.\n");
        return -1;
    }
    return 0;
}
/******************************************************************************/
static int
xread(int p, ems_u32* buf, int len)
{
    int restlen, da;

    restlen=len*sizeof(ems_u32);
    while (restlen) {
        da=read(p, buf, restlen);
        if (da>0) {
            buf+=da;
            restlen-=da;
        } else if (errno==EINTR) {
            continue;
        } else {
            printf("xread: %s\n", strerror(errno));
            return -1;
        }
    }
    return 0;
}
/******************************************************************************/
static int
dump_data(ems_u32* d, int num, char* text)
{
    int i;

    if (text) {
        fprintf(ep, "dump_data: %s\n", text);
    }
    for (i=1; num; num--, i++) {
        fprintf(ep, "0x%08x ", *d++);
        if (!(i&3)) fprintf(ep, "\n");
    }
    fprintf(ep, "\n");
}
/******************************************************************************/
static int
dump_cluster(int maxwords, char* text, int code)
{
    fprintf(ep, "dump_cluster%s (%d)", maxwords>0?"head":"", code);
    if (text && *text) fprintf(ep, " %s", text);
    fprintf(ep, "\n");

    maxwords=(maxwords<=0)||(maxwords>clsize)?clsize:maxwords;

    dump_data(cldata, maxwords, 0);
}
/******************************************************************************/
static int
decode_timestamp(ems_u32* data, int size)
{
    struct timeval tv;
    char s[1024];
    struct tm* time;
    time_t tt;

    if (size<2) {
        fprintf(ep, "timestamp: too short");
        return -1;
    }
    tv.tv_sec=*data++;
    tv.tv_usec=*data;
    tt=tv.tv_sec;
    time=localtime(&tt);
    strftime(s, 1024, "%c", time);
    /*fprintf(op, "timestamp: %d:%06d (%s)\n", tv.tv_sec, tv.tv_usec, s);*/
    return 2;
}
/******************************************************************************/
static int
decode_checksum(ems_u32* data, int size)
{
    fprintf(ep, "checksum (not implemented)\n");
    return -1;
}
/******************************************************************************/
static int
decode_options(ems_u32* data, int size)
{
    int idx=0, optsize, flags, res;

    if (size<1) {
        fprintf(ep, "options: too short\n");
        return -1;
    }
    optsize=data[idx++];
    if (size<optsize+1) {
        fprintf(ep, "options: size=%d, optsize=%d\n", size, optsize);
        return -1;
    }
    if (!optsize) return idx;
    flags=data[idx++];
    if (flags&1) {
        res=decode_timestamp(data+idx, size-idx);
        if (res<0) return -1;
        idx+=res;
    }
    if (flags&2) {
        res=decode_checksum(data+idx, size-idx);
        if (res<0) return -1;
        idx+=res;
    }
    if (flags&~3) {
        fprintf(ep, "options: unknown flags 0x%08x\n", flags);
        return -1;
    }

    return idx;
}
/******************************************************************************/
static int
decode_proc_Timestamp(ems_u32* data, int size, struct proc* proc)
{
    struct tm* tm;
    time_t t;
    char str[1024];

    if (size<2) {
        fprintf(ep, "  Timestamp: size=%d (not 2)\n", size);
        return -1;
    }
    t=data[0];
    tm=localtime(&t);
    strftime(str, 1024, "%c", tm);
    /*fprintf(op, "  Timestamp: %s\n", str);*/
    return 2;
}
/******************************************************************************/
static int
init_cache_sis3800ShadowUpdate(struct proc* proc)
{
    int modnum, channelnum, i;

    if (proc->num_args<3) {
        fprintf(ep, "  init_cache_sis3800ShadowUpdate: not enought args\n");
        return -1;
    }
    modnum=proc->args[1];
    if (proc->num_args!=(2*modnum+2)) {
        fprintf(ep, "  init_cache_sis3800ShadowUpdate: illegal number of args\n");
        return -1;
    }
    proc->num_cachevals=modnum+2;
    proc->cachevals=malloc(proc->num_cachevals*sizeof(ems_u32));
    proc->cachevals[1]=modnum;
    channelnum=0;
    for (i=0; i<modnum; i++) {
        int channels=0;
        ems_u32 mask=proc->args[modnum+2+i];
        while (mask) {
            if (mask&1) channels++;
            mask>>=1;
        }
        proc->cachevals[2+i]=channels;
        channelnum+=channels;
    }
    proc->cachevals[0]=channelnum+modnum+1;
    return 0;
}
/******************************************************************************/
static int
decode_proc_sis3800ShadowUpdate(ems_u32* data, int size, struct proc* proc)
{
    int idx=0, i;
    if (!proc->num_cachevals)
        if (init_cache_sis3800ShadowUpdate(proc)<0) return -1;

    if (size<proc->cachevals[0]) { /* number of words */
        fprintf(ep, "sis3800ShadowUpdate: not enought data\n");
        return -1;
    }
    if (data[idx++]!=proc->cachevals[1]) { /* number of modules */
        fprintf(ep, "sis3800ShadowUpdate: first word is not module count\n");
        return -1;
    }
    for (i=0; i<proc->cachevals[1]; i++) {
        if (data[idx++]!=proc->cachevals[2+i]) { /* number of channels */
            fprintf(ep, "sis3800ShadowUpdate: wrong channelcount\n");
            return -1;
        }
        idx+=2*proc->cachevals[2+i];
    }

    return idx;
}
/******************************************************************************/
static int
init_cache_ScalerLCxxxx_update_read(struct proc* proc)
{
    int channels, i;

    if (proc->num_args!=3) {
        fprintf(ep, "  init_cache_ScalerLCxxxx_update_read: illegal number of args\n");
        return -1;
    }
    proc->num_cachevals=1;
    proc->cachevals=malloc(proc->num_cachevals*sizeof(ems_u32));
    channels=0;
    ems_u32 mask=proc->args[2];
    while (mask) {
        if (mask&1) channels++;
        mask>>=1;
    }
    proc->cachevals[0]=channels;
    return 0;
}
/******************************************************************************/
static int
decode_proc_ScalerLCxxxx_update_read(ems_u32* data, int size, struct proc* proc)
{
    int idx=0, i;
    if (!proc->num_cachevals)
        if (init_cache_ScalerLCxxxx_update_read(proc)<0) return -1;

    if (data[idx++]!=proc->cachevals[0]) { /* number of channels */
        fprintf(ep, "ScalerLCxxxx_update_read: first word is not channel count\n");
        return -1;
    }
    if (size-idx<proc->cachevals[0]*2) { /* number of words */
        fprintf(ep, "ScalerLCxxxx_update_read: invalid wordcount\n");
        return -1;
    }
    idx+=proc->cachevals[0]*2;

    return idx;
}
/******************************************************************************/
static int
decode_proc_fb_lc1881_readout(ems_u32* data, int size, struct proc* proc)
{
    int idx=0, i;
    int length;

    if (size<1) { /* number of words */
        fprintf(ep, "fb_lc1881_readout: not enought data\n");
        return -1;
    }
    length=data[idx++]+1;
    if (size<length) {
        fprintf(ep, "fb_lc1881_readout (b): not enought data\n");
        return -1;
    }
    return length;
}
/******************************************************************************/
static int
decode_proc_fb_lc1881_check_buf(ems_u32* data, int size, struct proc* proc)
{
    if (size<4) {
        fprintf(ep, "fb_lc1881_check_buf: not enought data\n");
        return -1;
    }
    if (data[3]) {
        fprintf(op, "fb_lc1881_check_buf:\n");
        fprintf(op, "  modultype=0x%08x\n", data[0]);
        fprintf(op, "  last_good: %d\n", data[1]);
        fprintf(op, "  current: %d\n", data[2]);
        fprintf(op, "  mismatches: %d\n", data[3]);
    }
    return 4;
}
/******************************************************************************/
static int
decode_proc_fb_ph10Cx_read(ems_u32* data, int size, struct proc* proc)
{
    int idx=0, i;
    int length;

    if (size<1) { /* number of words */
        fprintf(ep, "fb_ph10Cx_read: not enought data\n");
        return -1;
    }
    length=data[idx++]+1;
    if (size<length) {
        fprintf(ep, "fb_ph10Cx_read (b): not enought data\n");
        return -1;
    }
    return length;
}
/******************************************************************************/
static int
decode_proc_FRDB_multi_read(ems_u32* data, int size, struct proc* proc)
{
    int idx=0, i;
    int length;

    if (size<1) { /* number of words */
        fprintf(ep, "FRDB_multi_read: not enought data\n");
        return -1;
    }
    length=data[idx++]+1;
    if (size<length) {
        fprintf(ep, "FRDB_multi_read (b): not enought data\n");
        return -1;
    }
    return length;
}
/******************************************************************************/
static int
decode_proc_GetIntVar(ems_u32* data, int size, struct proc* proc)
{
    int length;

    length=proc->args[0];
    if (size<length) { /* number of words */
        fprintf(ep, "GetIntVar: not enought data\n");
        return -1;
    }
    return length;
}
/******************************************************************************/
static int
decode_proc_GetPCITrigData(ems_u32* data, int size, struct proc* proc)
{
    int length;

    length=16;
    if (size<length) { /* number of words */
        fprintf(ep, "GetPCITrigData: not enought data\n");
        return -1;
    }
    return length;
}
/******************************************************************************/
static int
decode_proc_C219statist(ems_u32* data, int size, struct proc* proc)
{
    int length;

    length=1;
    if (size<length) { /* number of words */
        fprintf(ep, "C219statist: not enought data\n");
        return -1;
    }
    return length;
}
/******************************************************************************/
static int
decode_proc_sis3600wait(ems_u32* data, int size, struct proc* proc)
{
    int length;

    length=0;
    if (size<length) { /* number of words */
        fprintf(ep, "sis3600wait: not enought data\n");
        return -1;
    }
    return length;
}
/******************************************************************************/
static int
decode_proc(ems_u32* data, int size, int ved, ems_u32 evno, ems_u32 trigno,
        int is, struct proc* proc)
{
    char* name=proc->name;
    int res=0;

    /*fprintf(op, "decoding is %d trigger %d: %s\n", is, trigno, name);*/

    if (!strcmp(name, "")) {
    } else if (!strcmp(name, "Timestamp.1")) {
        res=decode_proc_Timestamp(data, size, proc);
    } else if (!strcmp(name, "sis3800ShadowUpdate.1")) {
        res=decode_proc_sis3800ShadowUpdate(data, size, proc);
    } else if (!strcmp(name, "fb_lc1881_readout.4")) {
        res=decode_proc_fb_lc1881_readout(data, size, proc);
    } else if (!strcmp(name, "fb_lc1881_check_buf.2")) {
        res=decode_proc_fb_lc1881_check_buf(data, size, proc);
    } else if (!strcmp(name, "fb_ph10Cx_read.1")) {
        res=decode_proc_fb_ph10Cx_read(data, size, proc);
    } else if (!strcmp(name, "GetIntVar.1")) {
        res=decode_proc_GetIntVar(data, size, proc);
    } else if (!strcmp(name, "GetPCITrigData.1")) {
        res=decode_proc_GetPCITrigData(data, size, proc);
    } else if (!strcmp(name, "Scaler4434_update_read.1")) {
        res=decode_proc_ScalerLCxxxx_update_read(data, size, proc);
    } else if (!strcmp(name, "Scaler2551_update_read.1")) {
        res=decode_proc_ScalerLCxxxx_update_read(data, size, proc);
    } else if (!strcmp(name, "C219statist.1")) {
        res=decode_proc_C219statist(data, size, proc);
    } else if (!strcmp(name, "sis3600wait.1")) {
        res=decode_proc_sis3600wait(data, size, proc);
    } else if (!strcmp(name, "FRDB_multi_read.1")) {
        res=decode_proc_FRDB_multi_read(data, size, proc);
    } else if (!strcmp(name, "")) {
    } else if (!strcmp(name, "")) {
    } else if (!strcmp(name, "")) {
    } else if (!strcmp(name, "")) {
    } else if (!strcmp(name, "")) {
    } else if (!strcmp(name, "")) {
    } else if (!strcmp(name, "")) {
    } else if (!strcmp(name, "")) {
    } else {
        fprintf(op, "no procedure %s\n", name);
        return -1;
    }
    return res;
}
/******************************************************************************/
static int
decode_subevent(ems_u32* data, int size, int ved, ems_u32 evno, ems_u32 trigno)
{
    int ssize, idx=0, sidx, i;
    ems_u32 *sdata, is_id;
    struct is* is;
    struct proclist* proclist;

    if (size<2) {
        fprintf(ep, "subevent too short\n");
        return -1;
    }
    is_id=data[idx++];
    ssize=data[idx++];
    if (size-idx<ssize) {
        fprintf(ep, "subevent: subeventsize=%d size=%d\n", ssize, size);
        return -1;
    }
/*
    fprintf(op, "subevent: id=%d size=%d\n", is_id, ssize);
*/
    for (i=0; (i<iss.num_is) && (iss.iss[i]->id!=is_id); i++);
    if (i==iss.num_is) {
        fprintf(ep, "subevent %d not valid\n", is_id);
        return -1;
    }
    is=iss.iss[i];
    if (trigno>=MAXTRIGGER) {
        fprintf(ep, "trigger %d not valid\n", trigno);
        return -1;
    }
    proclist=is->proclists[trigno];
    if (!proclist) {
        fprintf(ep, "is %d trigger %d: no valid proclist\n", is_id, trigno);
        return -1;
    }
    sdata=data+idx;
    sidx=0;
    for (i=0; i<proclist->num_procs; i++) {
        struct proc* proc=proclist->procs[i];
        int res;

        res=decode_proc(sdata+sidx, ssize-sidx, ved, evno, trigno, is_id, proc);
        if (res<0) return -1;
        sidx+=res;
    }
    if (sidx!=ssize) {
        fprintf(ep, "ERROR: is %d trigger %d: ssize=%d; used=%d\n", is_id, trigno,
                ssize, sidx);
        return -1;
    }

    return idx+sidx;
}
/******************************************************************************/
static int
decode_event(ems_u32* data, int size, int ved)
{
    int eventsize, num_subevents, idx=0, res, i;
    ems_u32 evno, trigno;

    if (size<4) {
        fprintf(ep, "event too short\n");
        return -1;
    }
    eventsize=data[idx++];
    if (size-idx<eventsize) {
        fprintf(ep, "event: eventsize=%d size=%d\n", eventsize, size);
        return -1;
    }
    evno=data[idx++];
    trigno=data[idx++];
    num_subevents=data[idx++];
/*
    fprintf(op, "event: evno=%d trigger=0x%x %d subevents\n",
            evno, trigno, num_subevents);
*/
    for (i=0; i<num_subevents; i++) {
        res=decode_subevent(data+idx, size-idx, ved, evno, trigno);
        if (res<0) return -1;
        idx+=res;
    }

    return idx;
}
/******************************************************************************/
static int
decode_cluster_events(ems_u32* data, int size)
{
    int num_events, flags, ved, fragment, idx=0, res, i;

    res=decode_options(data, size);
    if (res<0) return -1;
    idx+=res;

    if (size-idx<4) {
        fprintf(ep, "cluster_events too short\n");
        return -1;
    }
    flags=data[idx++];
    if (flags) {
        fprintf(ep, "cluster_events: unknown flags=0x%08x\n", flags);
        return -1;
    }
    ved=data[idx++];
    /*fprintf(op, "events: ved=%d\n");*/

    fragment=data[idx++];
    if (fragment) {
        fprintf(ep, "cluster_events: fragmented\n");
        return -1;
    }
    num_events=data[idx++];
    for (i=0; i<num_events; i++) {
        res=decode_event(data+idx, size-idx, ved);
        if (res<0) return -1;
        idx+=res;
    }
   
    return idx;
}
/******************************************************************************/
static int
decode_ved_info1(ems_u32* data, int size)
{
    int idx=0, ved, num_is, i;

    if (size<2) {
        fprintf(ep, "ved_info type 1: too short\n");
        return -1;
    }
    ved=data[idx++];
    num_is=data[idx++];
    if (size-idx<num_is) {
        fprintf(ep, "ved_info type 1: size=%d num_is=%d\n", size, num_is);
        return -1;
    }
    fprintf(op, "ved_info_1: ved=%d; %d is%s", ved, num_is, num_is?":":"");
    for (i=0; i<num_is; i++)
        fprintf(op, " %d", data[idx++]);
    fprintf(op, "\n");

    return idx;
}
/******************************************************************************/
static int
decode_ved_infos1(ems_u32* data, int size)
{
    int num_infos, i, res, idx=0;

    if (size<1) {
        fprintf(ep, "ved_infos type 1: too short\n");
        return -1;
    }
    num_infos=data[idx++];

    for (i=0; i<num_infos; i++) {
        res=decode_ved_info1(data+idx, size-idx);
        if (res<0) return -1;
        idx+=res;
    }

    return idx;
}
/******************************************************************************/
static int
decode_ved_infos2(ems_u32* data, int size)
{
    fprintf(ep, "ved_info type 2 (not implemented)\n");
    return -1;
}
/******************************************************************************/
static int
decode_ved_infos3(ems_u32* data, int size)
{
    fprintf(ep, "ved_info type 3 (not implemented)\n");
    return -1;
}
/******************************************************************************/
static int
decode_cluster_ved_info(ems_u32* data, int size)
{
    int res, version, idx=0;

    fprintf(ep, "cluster_ved_info\n");

    res=decode_options(data, size);
    if (res<0) return -1;
    idx+=res;

    if (size-idx<1) {
        fprintf(ep, "cluster_ved_info too short\n");
        return -1;
    }
    version=data[idx++];
    switch (version) { /* version */
    case 0x80000001:
        res=decode_ved_infos1(data+idx, size-idx);
        break;
    case 0x80000002:
        res=decode_ved_infos2(data+idx, size-idx);
        break;
    case 0x80000003:
        res=decode_ved_infos3(data+idx, size-idx);
        break;
    default:
        fprintf(ep, "cluster_ved_info: unknown version 0x%08x\n", version);
        return -1;
    }
    if (res<0) return -1;
    idx+=res;
    return idx;
}
/******************************************************************************/
static int
create_file(char* filename, char** newname)
{
    char *name, *dir, *base;
    int v, res;

    if (canonic_path(filename, &dir, &base)) {
        fprintf(ep, "path \"%s\" is illegal\n", filename);
        return -1;
    }

    if (*dir) res=make_path(dir, 0777, 0777);

    name=malloc(strlen(dir)+strlen(base)+10);

    v=0;
    do {
        sprintf(name, "%s%s_%02d", dir, base, v);
        res=open(name, O_WRONLY|O_CREAT|O_EXCL, 0666);
        v++;
    } while ((res<0) && (errno==EEXIST));
    if (res<0) {
        fprintf(ep, "create \"%s\": %s\n", name, strerror(errno));
    }
    free(dir);
    free(base);
    if (newname)
        *newname=name;
    else
        free(name);
    return res;
}
/*****************************************************************************/
static int
decode_line(ems_u32* data, int size, char** line)
{
    int len;

    len=xdrstrlen(data);
    if (size<len) {
        fprintf(ep, "cluster_text too short for line: size=%d, need %d",
                size, len);
        return -1;
    }
    xdrstrcdup(line, (const int*)data);
    /*fprintf(op, "%s\n", line);*/
    return len;
}
/******************************************************************************/
static int
decode_cluster_text(ems_u32* data, int size)
{
    int res, idx=0, line, p;
    int fragment, nlines;
    ems_u32 flags;

    fprintf(ep, "cluster_text\n");

    res=decode_options(data, size);
    if (res<0) return -1;
    idx+=res;
    if (size-idx<3) {
        fprintf(ep, "cluster_text too short after options\n");
        return -1;
    }
    flags=data[idx++];
    fragment=data[idx++];
    nlines=data[idx++];
    fprintf(ep, "  flags=0x%08x\n", flags);
    fprintf(ep, "  fragment=%d\n", fragment);
    fprintf(ep, "  nlines=%d\n", nlines);

    for (line=0; line<nlines; line++) {
        char *cline, nl='\n';
        res=decode_line(data+idx, size-idx, &cline);
        if (res<0) return -1;
        if (line==0) {
            char* filename;
            fprintf(op, "  %s\n", cline);
            if (!strncmp(cline, "Key: ", 5)) {
                filename=cline+5;
            } else {
                filename="unknown_textcluster";
            }
            p=create_file(filename, 0);
        }
        if (p>=0) {
            write(p, cline, strlen(cline));
            write(p, &nl, 1);
        }
        free(cline);
        idx+=res;
    }
    close(p);

    return idx;
}
/******************************************************************************/
static int
decode_cluster_wendy_setup(ems_u32* data, int size)
{
    fprintf(ep, "cluster_wendy_setup\n");
    return -1;
}
/******************************************************************************/
static int
decode_cluster_file(ems_u32* data, int size)
{
    static char *old_filename=0, *newname=0;
    static int p=-1;
    int res, idx=0, len, fragment;
    ems_u32 flags;
    char *filename;
    int ctime, mtime, perm, contentsize, wordsize;

    fprintf(ep, "cluster_file\n");

    res=decode_options(data, size);
    if (res<0) {
        fprintf(ep, "RAUS 1\n");
        return -1;
    }
    idx+=res;

    if (size-idx<3) {
        fprintf(ep, "cluster_text too short after options\n");
        fprintf(ep, "RAUS 2\n");
        return -1;
    }
    flags=data[idx++];
    fragment=data[idx++];

    len=xdrstrlen(data+idx);
    if (size-idx<len) {
        fprintf(ep, "cluster_file too short for filename: size=%d, need %d",
                size, len);
        fprintf(ep, "RAUS 3\n");
        return -1;
    }
    xdrstrcdup(&filename, (data+idx));
    fprintf(op, "name=%s\n", filename);
    idx+=len;

    if (fragment>0) {
        if (!old_filename || strcmp(filename, old_filename)) {
            fprintf(ep, "cluster_file: fragment=%d but names do not match\n",
                    fragment);
            free(filename);
            free(old_filename); old_filename=0;
            close(p); p=-1;
        }
    } else {
        p=create_file(filename, &newname);
        if (p<0) {
            free(filename);
            free(newname); newname=0;
            return -1;
        }
    }
    ctime=data[idx++];
    mtime=data[idx++];
    perm=data[idx++];
    size=data[idx++];

    fprintf(op, "flags=0x%08x fragment=%d\n", flags, fragment);
    fprintf(op, "ctime=0x%08x mtime=0x%08x perm=0x%08x size=%d\n",
            ctime, mtime, perm, size);
    contentsize=data[idx++];
    wordsize=(contentsize+3)/4;

    fprintf(op, "contentsize=%d, wordsize=%d\n", contentsize, wordsize);

    swap_falls_noetig(data+idx, wordsize);
    res=write(p, data+idx, contentsize);
    if (res!=contentsize) {
        fprintf(ep, "write >%s<: ", filename);
        if (res<0)
            fprintf(ep, "errno=%s\n", strerror(errno));
        else
            fprintf(ep, "only %d bytes written (not %d)\n", res, contentsize);
        free(filename);
        return -1;
    }

    if (!(flags&2)) {
        struct utimbuf buf;

        close(p); p=-1;

        buf.actime=mtime;
        buf.modtime=mtime;
        utime(newname, &buf);
        free(newname); newname=0;
    }
    if (fragment==0) {
        if (flags&2) {/* not the last fragment */
            old_filename=filename;
        } else {
            free(filename);
        }
    } else {
        free(filename);
        if (!(flags&2)) {/* last fragment */
            free(old_filename); old_filename=0;
        }
    }


    idx+=wordsize;

    return idx;
}
/******************************************************************************/
static int
read_cluster(int p)
{
    ems_u32 head[2], v;
    ems_u32* data;
    int wenden, evnum, i;

    if (xread(p, head, 2)) return -1;
    switch (head[1]) {
    case 0x12345678: wenden=0; break;
    case 0x78563412: wenden=1; break;
    default:
        printf("unkown endien 0x%x\n", head[1]);
        return -1;
    }
    clsize=(wenden?SWAP_32(head[0]):head[0])+1;
    if (clsize>MAX_CLUSTERSIZE) {
        fprintf(ep, "clustersize=%d\n", clsize);
    }
    if (clsize>cldatasize) {
        if (cldata) free(cldata);
        cldata=malloc(clsize*sizeof(ems_u32));
        if (!cldata) {
            perror("malloc");
            return -1;
        }
    }
    if (xread(p, cldata+2, clsize-2)) return -1;
    if (wenden) {
        for (i=0; i<clsize; i++) cldata[i]=SWAP_32(cldata[i]);
    }
    cldata[0]=head[0];
    cldata[1]=head[1];
    return clsize;
}
/*****************************************************************************/
static int
decode_cluster(ems_u32* cl, int size)
{
    int res;

    /*dump_cluster(128, "decode_cluster", ClTYPE(cldata));*/
    switch (ClTYPE(cldata)) {
    case clusterty_events:
        clnum_events++;
        res=decode_cluster_events(cl+3, size-3);
        break;
    case clusterty_ved_info:
        clnum_ved_info++;
        res=decode_cluster_ved_info(cl+3, size-3);
        break;
    case clusterty_text:
        clnum_text++;
        res=decode_cluster_text(cl+3, size-3);
        break;
#if 0
    case clusterty_x_wendy_setup:
        clnum_wendy_setup++;
        res=decode_cluster_wendy_setup(cl+3, size-3);
        break;
#endif
    case clusterty_file:
        clnum_file++;
        res=decode_cluster_file(cl+3, size-3);
        break;
    case clusterty_no_more_data:
        clnum_no_more_data++;
        fprintf(ep, "no more data\n");
        return 1;
    }
    if (res<0) return -1;
    if (res!=size-3) {
        fprintf(ep, "ERROR: decode_cluster: cluster not exhausted: size=%d, used=%d\n",
                size, res);
        return -1;
    }

    return 0;
}
/*****************************************************************************/
static int
do_cluster(void)
{
    int size;
    if ((size=read_cluster(ip))<0) return -1;
    if (decode_cluster(cldata, size)<0) return -1;
    return 0;
}
/*****************************************************************************/
static void
do_scan(void)
{
    int res;
    do {
        res=do_cluster();
    } while (!res);
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
    if (readargs(argc, argv)<0) return 1;

    if (read_procfile(procfile, &iss)<0) return -1;
    /*dump_procfile(&iss);*/

    ip=0; op=stdout; ep=stdout;
    cldata=0;
    cldatasize=0;

    clnum_events=0;
    clnum_ved_info=0;
    clnum_text=0;
    clnum_wendy_setup=0;
    clnum_file=0;
    clnum_no_more_data=0;

    do_scan();

    fprintf(ep, "clnum_events      =%d\n", clnum_events);
    fprintf(ep, "clnum_ved_info    =%d\n", clnum_ved_info);
    fprintf(ep, "clnum_text        =%d\n", clnum_text);
    fprintf(ep, "clnum_wendy_setup =%d\n", clnum_wendy_setup);
    fprintf(ep, "clnum_file        =%d\n", clnum_file);
    fprintf(ep, "clnum_no_more_data=%d\n", clnum_no_more_data);

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
