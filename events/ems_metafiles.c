/*
 * ems_metafiles.c
 * created 2009-05-25 PW
 * $ZEL$
 */

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <utime.h>
#include <unistd.h>
#include <libgen.h>

#include <emsctypes.h>
#include <xdrstring.h>
#include <clusterformat.h>
#include <swap.h>

#include "makepath.h"

#include "canonic_path.h"

static ems_u32* cldata;
static int clsize;
static int cldatasize;

static const char *linkdir;
static const char *written_files;
static int nfnames;
static char** fnames;
static int verbose;

static FILE *fw;
static DIR *ldir, *cdir;

enum compresstype {cpt_plain, cpt_compress, cpt_gzip, cpt_bzip2};
struct finfo {
    char* name;                     /* allocated via strdup */
    char* basename;                 /* pointer into name */
    char* canonic_path;
    enum compresstype compresstype;
    struct stat sb;

    time_t start;
    time_t stop;
    int events;
    int run;
    struct timeval tv;
};

struct finfo* finfo=0;
int nfinfo=0, finfosize=0;

#define SWAP_32(n) (((n & 0xff000000) >> 24) | \
    ((n & 0x00ff0000) >>  8) | \
    ((n & 0x0000ff00) <<  8) | \
    ((n & 0x000000ff) << 24))

/******************************************************************************/
static void
printusage(char* argv0)
{
    char *arg0=strdup(argv0);
    fprintf(stderr, "usage: %s [-h] [-v] [-l linkdir] [-w written_files] [dir ...]\n",
        basename(arg0));
    fprintf(stderr, "  -h: this help\n");
    fprintf(stderr, "  -v: verbose output\n");
    fprintf(stderr, "  linkdir      : directory for symbolic links\n");
    fprintf(stderr, "  written_files: filename for log\n");
    fprintf(stderr, "    either linkdir or written_files must be given\n");
    fprintf(stderr, "  dir: one or more directories with ems-clusterfiles\n");
    free(arg0);
}
/*****************************************************************************/
static int
readargs(int argc, char* argv[])
{
    const char* args="hvl:w:";
    int c, err;

    linkdir=0;
    written_files=0;
    verbose=0;

    err=0;
    while (!err && ((c=getopt(argc, argv, args)) != -1)) {
        switch (c) {
        case 'h':
            printusage(argv[0]);
            return 1;
        case 'v':
            verbose=1;
            break;
        case 'l':
            linkdir=optarg;
            break;
        case 'w':
            written_files=optarg;
            break;
        default:
            err=1;
        }
    }

    if (err || (!linkdir && !written_files)) {
        printusage(argv[0]);
        return -1;
    }

    fnames=argv+optind;
    nfnames=argc-optind;

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
            fprintf(stderr, "xread: %s\n", strerror(errno));
            return -1;
        }
    }
    return 0;
}
/******************************************************************************/
static int
decode_timestamp(ems_u32* data, int size, struct finfo *info)
{
    struct timeval tv;

    if (size<2) {
        fprintf(stderr, "timestamp: too short");
        return -1;
    }
    tv.tv_sec=*data++;
    tv.tv_usec=*data;
    info->tv=tv;
    return 2;
}
/******************************************************************************/
static int
decode_checksum(ems_u32* data, int size)
{
    fprintf(stderr, "checksum (not implemented)\n");
    return -1;
}
/******************************************************************************/
static int
decode_options(ems_u32* data, int size, struct finfo *info)
{
    int idx=0, optsize, flags, res;

    if (size<1) {
        fprintf(stderr, "options: too short\n");
        return -1;
    }
    optsize=data[idx++];
    if (size<optsize+1) {
        fprintf(stderr, "options: size=%d, optsize=%d\n", size, optsize);
        return -1;
    }
    if (!optsize) return idx;
    flags=data[idx++];
    if (flags&1) {
        res=decode_timestamp(data+idx, size-idx, info);
        if (res<0) return -1;
        idx+=res;
    }
    if (flags&2) {
        res=decode_checksum(data+idx, size-idx);
        if (res<0) return -1;
        idx+=res;
    }
    if (flags&~3) {
        fprintf(stderr, "options: unknown flags 0x%08x\n", flags);
        return -1;
    }

    return idx;
}
/*****************************************************************************/
static int
decode_line(ems_u32* data, int size, char** line)
{
    int len;

    len=xdrstrlen(data);
    if (size<len) {
        fprintf(stderr, "cluster_text too short for line: size=%d, need %d",
                size, len);
        return -1;
    }
    xdrstrcdup(line, data);
    /*fprintf(op, "%s\n", line);*/
    return len;
}
/******************************************************************************/
/*
Key: superheader
Facility: COSY-TOF
Location: Research Center Juelich
Devices: em sync t01 t01a t03 t04 t06 t16 t17 t18 t44
Run: 9712
Start Time: 2009-05-25 02:18:25 CEST
Start Time_coded: 1243210705
Stop Time: 2009-05-25 02:51:17 CEST
Stop Time_coded: 1243212677
*/
static int
decode_cluster_text(ems_u32* data, int size, struct finfo *info)
{
    int res, idx=0, line, superheader=0;
    int fragment, nlines;
    ems_u32 flags;
    char *val;

    res=decode_options(data, size, info);
    if (res<0)
        return -1;
    idx+=res;
    if (size-idx<3) {
        fprintf(stderr, "cluster_text too short after options\n");
        return -1;
    }
    flags=data[idx++];
    fragment=data[idx++];
    nlines=data[idx++];

    for (line=0; line<nlines; line++) {
        char *cline;
        res=decode_line(data+idx, size-idx, &cline);
        if (res<0)
            return -1;
        if (line==0) {
            if (!strncmp(cline, "Key: ", 5)) {
                val=cline+5;
                if (!strcmp(val, "superheader")) {
                    superheader=1;
                }
            }
        } else if (superheader) {
            if (!strncmp(cline, "Run: ", 5)) {
                val=cline+5;
                info->run=atoi(val);
            } else if (!strncmp(cline, "Start Time_coded: ", 18)) {
                val=cline+18;
                info->start=atol(val);
            } else if (!strncmp(cline, "Stop Time_coded: ", 17)) {
                val=cline+17;
                info->stop=atol(val);
            }
        }
        free(cline);
        idx+=res;
    }

    return idx;
}
/******************************************************************************/
static int
decode_cluster_events(ems_u32* data, int size, struct finfo *info)
{
    int num_events, evno, last_event, flags, ved, fragment, idx=0, res;

    res=decode_options(data, size, info);
    if (res<0)
        return -1;
    idx+=res;

    if (size-idx<4) {
        fprintf(stderr, "cluster_events too short\n");
        return -1;
    }
    flags=data[idx++];
    if (flags) {
        fprintf(stderr, "cluster_events: unknown flags=0x%08x\n", flags);
        return -1;
    }
    ved=data[idx++];

    fragment=data[idx++];
    if (fragment) {
        fprintf(stderr, "cluster_events: fragmented\n");
        return -1;
    }
    num_events=data[idx++];
    if (num_events) {
        /* read evno of first event */
        evno=data[idx+1];
    }

    last_event=evno+num_events-1;
    if (info->events<last_event)
        info->events=last_event;

    return idx;
}
/******************************************************************************/
static int
read_cluster(int p)
{
    ems_u32 head[2];
    int wenden, i;

    if (xread(p, head, 2)) return -1;
    switch (head[1]) {
    case 0x12345678: wenden=0; break;
    case 0x78563412: wenden=1; break;
    default:
        fprintf(stderr, "unkown endien 0x%x\n", head[1]);
        return -1;
    }
    clsize=(wenden?SWAP_32(head[0]):head[0])+1;
    if (clsize>cldatasize) {
        if (cldata)
            free(cldata);
        cldatasize=clsize;
        cldata=malloc(cldatasize*sizeof(ems_u32));
        if (!cldata) {
            perror("malloc");
            return -1;
        }
    }
    if (xread(p, cldata+2, clsize-2))
        return -1;
    if (wenden) {
        for (i=0; i<clsize; i++)
            cldata[i]=SWAP_32(cldata[i]);
    }
    cldata[0]=head[0];
    cldata[1]=head[1];
    return clsize;
}
/*****************************************************************************/
static int
decode_cluster(ems_u32* cl, int size, struct finfo *info)
{
    int res;

    /*dump_cluster(128, "decode_cluster", ClTYPE(cldata));*/
    switch (ClTYPE(cldata)) {
    case clusterty_text:
        res=decode_cluster_text(cl+3, size-3, info);
        break;
    case clusterty_no_more_data:
        fprintf(stderr, "no more data\n");
        return 1;
    case clusterty_events:
        res=decode_cluster_events(cl+3, size-3, info);
        if (res>=0)
            res=size-3;
        break;
    default:
        res=size-3;
    }
    if (res<0)
        return -1;
    if (res!=size-3) {
        fprintf(stderr, "ERROR: decode_cluster: cluster not exhausted: size=%d, used=%d\n",
                size, res);
        return -1;
    }

    return 0;
}
/*****************************************************************************/
static int
dump_info(struct finfo *info)
{
    char s[1024];
    struct tm* time;
    time_t tt;

    printf("%s\n", info->basename);
    printf("%s\n", info->canonic_path);

    printf("run: %d\n", info->run);
    printf("events: %d\n", info->events);

    tt=info->tv.tv_sec;
    time=localtime(&tt);
    strftime(s, 1024, "%c", time);
    printf("timestamp: %s\n", s);

    time=localtime(&info->start);
    strftime(s, 1024, "%c", time);
    printf("start    : %s\n", s);

    time=localtime(&info->stop);
    strftime(s, 1024, "%c", time);
    printf("stop     : %s\n", s);

    return 0;
}
/*****************************************************************************/
/*
2009-05-25_04:06:25 CREATED  /data1/daq/2009-05-25/2009-05-25_04_06_25
2009-05-25_04:39:23 FINISHED /data1/daq/2009-05-25/2009-05-25_04_06_25 9719 \
    1243224385 1243226356 577857
*/
static int
do_work(struct finfo *info)
{
    if (ldir) {
        char name[24];
        int res;

        sprintf(name, "run_%04d", info->run);
        if (fchdir(dirfd(ldir))<0) {
            fprintf(stderr, "chdir to %s: %s\n", linkdir, strerror(errno));
            return -1;
        }
        res=symlink(info->canonic_path, name);
        if (res)
            fprintf(stderr, "link %s to %s: %s\n",
                    name, info->name, strerror(errno));
        if (fchdir(dirfd(cdir))<0) {
            fprintf(stderr, "chdir to \".\": %s\n", strerror(errno));
            return -1;
        }
    }
    if (fw) {
        if (info->stop==0) {
            info->stop=info->tv.tv_sec;
        }
        fprintf(fw, "%s %s %s %d %lld %lld %d\n",
            "unknown", "FINISHED", info->canonic_path, info->run,
            (unsigned long long)info->start, 
            (unsigned long long)info->stop,
            info->events);
        fflush(fw);
    }
    return 0;    
}
/*****************************************************************************/
static int
do_scan(struct finfo *info)
{
    int in;

    printf("scanning %s\n", info->name);
    in=open(info->name, O_RDONLY, 0);
    if (in<0) {
        fprintf(stderr, "open %s: %s\n", info->name, strerror(errno));
        return -1;
    }

    do {
        int size;
        if ((size=read_cluster(in))<0)
        if (size<=0)
            break;
        if (decode_cluster(cldata, size, info)<0)
            break;
    } while (written_files || !info->run);

    close(in);

    if (verbose)
        dump_info(info);

    do_work(info);

    return 0;
}
/*****************************************************************************/
static int
add_file(const char* name, const struct stat* sb,
        enum compresstype compresstype)
{
    char* bp;

    if (nfinfo==finfosize) {
        if (!finfosize)
            finfosize=1024;
        else
            finfosize*=2;
        finfo=realloc(finfo, finfosize*sizeof(struct finfo));
        if (!finfo) {
            printf("realloc finfo: %s\n", strerror(errno));
            return -1;
        }
    }

    memset(finfo+nfinfo, 0, sizeof(struct finfo));
    finfo[nfinfo].name=strdup(name);
    if (!finfo[nfinfo].name) {
        printf("strdup: %s\n", strerror(errno));
        return -1;
    }
    bp=strrchr(finfo[nfinfo].name, '/');
    if (bp)
        finfo[nfinfo].basename=bp+1;
    else
        finfo[nfinfo].basename=finfo[nfinfo].name;
    if (canonic_path(name, &finfo[nfinfo].canonic_path, 0, 0)<0)
        return -1;
    finfo[nfinfo].compresstype=compresstype;
    finfo[nfinfo].sb=*sb;
    nfinfo++;

    return 0;
}
/*****************************************************************************/
/*
gzip:     0       string          \037\213
bzip2:    0       string          BZh
compress: 0       string          \037\235
*/
static int
process_file(const char* name, const struct stat* sb)
{
    char magic[4];
    enum compresstype compresstype;
    int p, res;

    p=open(name, O_RDONLY, 0);
    if (p<0) {
        printf("open %s: %s\n", name, strerror(errno));
        return -1;
    }
    res=read(p, magic, 4);
    if (res!=4) {
        if (res<0)
            printf("read %s: %s\n", name, strerror(errno));
        else
            printf("read %s: could only read %d byte\n", name, res);
        return -1;
    }
    close(p);

    compresstype=cpt_plain;
    if (strncmp(magic, "\037\213", 2)==0) {
        compresstype=cpt_gzip;
    } else if (strncmp(magic, "BZh", 3)==0) {
        compresstype=cpt_bzip2;
    } else if (strncmp(magic, "\037\235", 2)==0) {
        compresstype=cpt_compress;
    }
    res=add_file(name, sb, compresstype);
    return res;
}
/*****************************************************************************/
static int
process_nftw(const char *file, const struct stat *sb, int flag, struct FTW *ftw)
{
    int res=0;

    switch (flag) {
    case FTW_F  :
        res=process_file(file, sb);
        break;
    case FTW_D  :
        break;
    case FTW_DNR:
        fprintf(stderr, "  directory %s can't be read\n", file);
        break;
    case FTW_SL :
        break;
    case FTW_NS :
        fprintf(stderr, "  %s: stat failed\n", file);
        break;
    case FTW_DP :
        break;
/*#ifdef __linux*/
    case FTW_SLN:
        fprintf(stderr, "  %s: A symbolic link that names a non-existent file\n", file);
        break;
/*#endif*/
    default:
        fprintf(stderr, "  %s: nftw flag=%d\n", file, flag);
    }

    return res;
}
/*****************************************************************************/
static int
process_dir(const char* name)
{
    int res;

    res=nftw(name, process_nftw, 0, 0/*FTW_PHYS*/);
    if (res<0)
        fprintf(stderr, "nftw: %s\n", strerror(errno));
    return res;
}
/*****************************************************************************/
static int
process_name(const char* name)
{
    struct stat stbuf;
    mode_t mode;
    int res;

    res=stat(name, &stbuf);
    if (res<0) {
        fprintf(stderr, "cannot stat %s: %s\n", name, strerror(errno));
        return -1;
    }
    mode=stbuf.st_mode;
    if (S_ISDIR(mode)) {
        res=process_dir(name);
    } else if (S_ISREG(mode)) {
        res=process_file(name, &stbuf);
    } else {
        fprintf(stderr, "%s is neither a directory nor a plain file.\n", name);
        res=-1;
    }

    return res;
}
/*****************************************************************************/
static int
finfo_compare(const void *va, const void *vb)
{
    struct finfo* a=(struct finfo*)va;
    struct finfo* b=(struct finfo*)vb;
    char* sa=a->basename;
    char* sb=b->basename;
    return strcmp(sa, sb);
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
    int i, res=0, ret=0;

    if (readargs(argc, argv)<0)
        return 1;

    cldata=0;
    cldatasize=0;

    for (i=0; (i<nfnames) && !res; i++) {
        printf("processing %s\n", fnames[i]);
        res=process_name(fnames[i]);
    }

    qsort(finfo, nfinfo, sizeof(struct finfo), finfo_compare);

    fw=0;
    cdir=0;
    ldir=0;

    if (written_files) {
        fw=fopen(written_files, "w");
        if (!fw) {
            fprintf(stderr, "cannot open %s: %s\n",
                    written_files, strerror(errno));
            ret=2;
            goto error;
        }
    }

    if (linkdir) {
        cdir=opendir(".");
        if (!cdir) {
            fprintf(stderr, "cannot open dir %s: %s\n",
                    ".", strerror(errno));
            ret=2;
            goto error;
        }
        ldir=opendir(linkdir);
        if (!ldir) {
            fprintf(stderr, "cannot open dir %s: %s\n",
                    linkdir, strerror(errno));
            ret=2;
            goto error;
        }
    }

    for (i=0; i<nfinfo; i++) {
        res=do_scan(finfo+i);
        if (res) {
            ret=3;
            goto error;
        }
    }

error:
    if (fw)
        fclose(fw);
    if (ldir)
        closedir(ldir);
    if (cdir)
        closedir(cdir);

    return ret;
}
/*****************************************************************************/
/*****************************************************************************/
