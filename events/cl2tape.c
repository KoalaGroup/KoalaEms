/*
 * cl2tape.c
 * 
 * $ZEL: cl2tape.c,v 1.1 2004/11/26 15:09:54 wuestner Exp $
 * 
 * created: 2004-Oct-27 PW
 */

#define _XOPEN_SOURCE_EXTENDED
#define _GNU_SOURCE

#ifdef __linux__
#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE
#endif
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#define LINUX_LARGEFILE O_LARGEFILE
#else
#define LINUX_LARGEFILE 0

#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ftw.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mtio.h>
#include <emsctypes.h>

char* tape=0;
int verbose=0, debug=0;
int nfnames;
char** fnames;

enum compresstype {cpt_plain, cpt_compress, cpt_gzip, cpt_bzip2};
struct finfo {
    char* name;                     /* allocated via strdup */
    char* basename;                 /* pointer into name */
    enum compresstype compresstype;
    struct stat sb;
};

struct finfo* finfo=0;
int nfinfo=0, finfosize=0;

ems_u32* buf=0;
int bsize=0;

ems_u64 words_on_tape;
ems_u64 words_on_file;

#define swap_int(a) ( ((a) << 24) | \
                      (((a) << 8) & 0x00ff0000) | \
                      (((a) >> 8) & 0x0000ff00) | \
        ((unsigned int)(a) >>24) )

/*****************************************************************************/
static void
usage(char* argv0)
{
    printf("usage: %s [-v] [-d] -t tape file_or_dir ...)\n", argv0);
    printf("       -v: verbose\n");
    printf("       -d: debug\n");
    printf("       -h: this text\n");
    printf("       file_or_dir: filename or directory tree\n");
    printf("                    files may be compressed by gzip or bzip2 "
            "or compress\n");
}
/*****************************************************************************/
static int
readargs(int argc, char* argv[])
{
    int c, errflg=0;

    while (!errflg && ((c=getopt(argc, argv, "vdt:h"))!=-1)) {
        switch (c) {
        case 'v':
            verbose++;
            break;
        case 'd':
            debug++;
            break;
        case 't':
            tape=optarg;
            break;
        case 'h':
        default:
            errflg=1;
        }
    }

    if (errflg) {
        usage(argv[0]);
        return -1;
    }
    if (!tape) {
        printf("-t {tape} must be given\n");
        return -1;
    }

    fnames=argv+optind;
    nfnames=argc-optind;

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
            finfosize=128;
        else
            finfosize*=2;
        finfo=realloc(finfo, finfosize*sizeof(struct finfo));
        if (!finfo) {
            printf("realloc finfo: %s\n", strerror(errno));
            return -1;
        }
    }
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
        printf("  directory %s can't be read\n", file);
        break;
    case FTW_SL :
        break;
    case FTW_NS :
        printf("  %s: stat failed\n", file);
        break;
    case FTW_DP :
        break;
/*#ifdef __linux*/
    case FTW_SLN:
        printf("  %s: A symbolic link that names a non-existent file\n", file);
        break;
/*#endif*/
    default:
        printf("  %s: nftw flag=%d\n", file, flag);
    }

    return res;
}
/*****************************************************************************/
static int
process_dir(char* name)
{
    int res;

    res=nftw(name, process_nftw, 0, 0/*FTW_PHYS*/);
    if (res<0)
        printf("nftw: %s\n", strerror(errno));
    return res;
}
/*****************************************************************************/
static int
process_name(char* name)
{
    struct stat stbuf;
    mode_t mode;
    int res;

    res=stat(name, &stbuf);
    if (res<0) {
        printf("cannot stat %s: %s\n", name, strerror(errno));
        return -1;
    }
    mode=stbuf.st_mode;
    if (S_ISDIR(mode)) {
        res=process_dir(name);
    } else if (S_ISREG(mode)) {
        res=process_file(name, &stbuf);
    } else {
        printf("%s is neither a directory nor a plain file.\n", name);
        res=-1;
    }

    return res;
}
/*****************************************************************************/
static int
tape_MTWEOF(int pt, int n)
{
    struct mtop mt_com;
    int res;

    mt_com.mt_op=MTWEOF;
    mt_com.mt_count=n;
    res=ioctl(pt, MTIOCTOP, &mt_com);
    if (res<0) {
        printf("tape_MTWEOF(filemark): %s\n", strerror(errno));
    }
    return res;
}
/*****************************************************************************/
static int
tape_MTBSF(int pt, int n)
{
    struct mtop mt_com;
    int res;

    mt_com.mt_op=MTBSF;
    mt_com.mt_count=n;
    res=ioctl(pt, MTIOCTOP, &mt_com);
    if (res<0) {
        printf("tape_MTBSF(bsf): %s\n", strerror(errno));
    }
    return res;
}
/*****************************************************************************/
static int
tape_MTBSFM(int pt, int n)
{
    struct mtop mt_com;
    int res;

    mt_com.mt_op=MTBSFM;
    mt_com.mt_count=n;
    res=ioctl(pt, MTIOCTOP, &mt_com);
    if (res<0) {
        printf("tape_MTBSFM(bsfM): %s\n", strerror(errno));
    }
    return res;
}
/*****************************************************************************/
static int
tape_MTOFFL(int pt)
{
    struct mtop mt_com;
    int res;

    mt_com.mt_op=MTOFFL;
    mt_com.mt_count=1;
    res=ioctl(pt, MTIOCTOP, &mt_com);
    if (res<0) {
        printf("tape_MTOFFL(eject): %s\n", strerror(errno));
    }
    return res;
}
/*****************************************************************************/
static int
tape_MTREW(int pt)
{
    struct mtop mt_com;
    int res;

    mt_com.mt_op=MTREW;
    mt_com.mt_count=1;
    res=ioctl(pt, MTIOCTOP, &mt_com);
    if (res<0) {
        printf("tape_MTREW(rewind): %s\n", strerror(errno));
    }
    return res;
}
/*****************************************************************************/
/*
 * returns:
 *  0: file successfull saved
 *  1: file partially saved; tape full
 * -1: fatal error
 */
static int
write_record(int pt, int size)
{
    int res;
    res=write(pt, buf, size*4);
    if (res!=size*4) {
        if ((res>0) || (errno==ENOSPC)) { /* res>0 is just paranoia */
            return 1;
        } else {
            printf("write_record: %s\n", strerror(errno));
            return -1;
        }
    }
    words_on_file+=size;
    return 0;
}
/*****************************************************************************/
static int
delete_record(int pt)
{
    printf("delete_record\n");
    return 0;
}
/*****************************************************************************/
static int
delete_file(int pt)
{
    printf("delete_file\n");
    return 0;
}
/*****************************************************************************/
static int
open_tape(char* tape)
{
    int p;

    p=open(tape, O_WRONLY|O_CREAT/*|O_EXCL*/, 0666);
    if (p<0)
        printf("open %s: %s\n", tape, strerror(errno));
    return p;
}
/*****************************************************************************/
/*
 * returns:
 *  -1: error
 * >=0: number of words read
 *      (==0: no more data)
 */
static int
read_data(int p, ems_u32* b, int len, char* name)
{
    int res, blen=len*4, got=0;
    char* cb=(char*)b;

    do {
        res=read(p, cb+got, blen-got);
        if (res>0) {
            got+=res;
        } else if (res<0) {
            printf("read %s: %s\n", name, strerror(errno));
        } /* res==0: == no more data */
    } while ((res>0) && (got<blen));

    return (res<0)?-1:got/4;
}
/*****************************************************************************/
/*
 * returns:
 * -1: fatal error
 *  0: no more data
 * >0: size of record (in words)
 */
static int /* returns size of record in words */
read_record(int p, char* name)
{
    int res, size=0;

    res=read_data(p, buf, 2, name);
    if (res<2) {
        if (res<0) {
            printf("read header: %s\n", strerror(errno));
        } else if (res>0) {
            printf("read header: short read\n");
            return 0; /* fuer verstuemmelte files */
        }
        return (res!=0)?-1:0;
    }

    switch (buf[1]) {
    case 0x12345678:
        size=buf[0]-1;
        break;
    case 0x78563412:
        size=swap_int(buf[0])-1;
        break;
    default:
        printf("%s: unknown endian 0x%08x\n", name, buf[1]);
        return -1;
    }

    if (size+1>bsize) {
        do
            bsize*=2;
        while (size+1>bsize);

        buf=realloc(buf, bsize);
        if (!buf) {
            printf("realloc %d words: %s\n", bsize, strerror(errno));
            return -1;
        }
    }

    res=read_data(p, buf+2, size, name);
    if (res<size) {
        if (res<0) {
            printf("read data: %s\n", strerror(errno));
        } else {
            printf("read data: only %d words read\n", res);
            return 0; /* fuer verstuemmelte files */
        }
        return -1;
    }

    return size+2;
}
/*****************************************************************************/
/*
 * returns:
 *  0: file successfull saved
 *  1: file partially saved; tape full
 * -1: fatal error
 */
static int
read_and_save(int p, int pt, char* name)
{
    int again=1, rres=0, wres=0;

    do {
        rres=read_record(p, name);
        if (rres>0) {
            wres=write_record(pt, rres);
            if (wres)
                return wres;
        } else {
            again=0;
        }
    } while (again);
    
    return rres;
}
/*****************************************************************************/
/*
 * returns:
 *  0: file successfull saved
 *  1: file partially saved; tape full
 * -1: fatal error
 * (identical to read_and_save)
 */
static int
save_file_once(struct finfo* info, int pt)
{
    int p, res;
    char* argv[3];

    p=open(info->name, O_RDONLY|LINUX_LARGEFILE, 0);
    if (p<0) {
        printf("open %s: %s\n", info->name, strerror(errno));
        return -1;
    }
    switch (info->compresstype) {
    case cpt_plain: /* no compression */
        break;
    case cpt_compress:
        argv[0]="uncompress";
        argv[1]="-c";
        argv[2]=0;
        break;
    case cpt_gzip:
        argv[0]="gunzip";
        argv[1]="-c";
        argv[2]=0;
        break;
    case cpt_bzip2:
        argv[0]="bunzip2";
        argv[1]=0;
        break;
    default:
        printf("program error: unknown compression type %d\n",
            info->compresstype);
        return -1;
    }

    if (info->compresstype!=cpt_plain) {
        int pp[2];
        pid_t pid;
        int res;

        res=pipe(pp);
        if (res) {
            printf("create pipe: %s\n", strerror(errno));
            close(p);
            return -1;
        }

        pid=fork();
        if (pid<0) { /* error */
            printf("fork: %s\n", strerror(errno));
            close(p);
            return -1;
        }

        if (pid==0) { /* child */
            close(pp[0]);
            dup2(p, 0);
            dup2(pp[1], 1);
            execvp(argv[0], argv);
            printf("exec %s: %s\n", argv[0], strerror(errno));
            exit(255);
        } else { /* parent */
            /* printf("pid=%d\n", pid); */
            p=pp[0];
            close(pp[1]);
        }
    }

    res=read_and_save(p, pt, info->basename);
    close(p);
    {
        pid_t pid;
        do {
            int status;
            pid=waitpid((pid_t)-1, &status, WNOHANG);
            /* if (pid>0) printf("pid %d terminated\n", pid); */
        } while (pid>0);
    }
    return res;
}
/*****************************************************************************/
/*
 * wrapper for save_file_once;
 * ONLY for statistics and printing of messages
 * returns the same as save_file_once
 */
static int
save_file_once_wrap(struct finfo* info, int pt)
{
    struct timeval tv0, tv1;
    int res;

    printf("saving %s (%Lu bytes)\n", info->name, info->sb.st_size);
    words_on_file=0;
    gettimeofday(&tv0, 0);
    res=save_file_once(info, pt);
    gettimeofday(&tv1, 0);

    switch (res) {
    case  0: {
        int tdiff, speed;
        double mspeed;
        words_on_tape+=words_on_file;
        tdiff=tv1.tv_sec-tv0.tv_sec; /* mikroseconds ignored */
        if (tdiff<1) tdiff=1; /* aviod division by zero and negative times */
        speed=words_on_file/tdiff*4;
        mspeed=(double)speed/1048576.;
        printf("successfully saved (%Lu bytes, %.3f MByte/s)\n",
                words_on_file*4, mspeed);
        }
        break;
    case  1:
        printf("NOT saved; tape full\n");
        break;
    case -1:
        printf("NOT saved; fatal error\n");
        break;
    default:
        printf("unknown errorcode from save_file_once: %d\n", res);
    }
    return res;
}
/*****************************************************************************/
/*
 * returns:
 * -1: error
 *  0: success
 */
static int
save_file(struct finfo* info, int pt)
{
    char s[10];
    int res;

    res=save_file_once_wrap(info, pt);
    switch (res) {
    case 0: /* success */
        tape_MTWEOF(pt, 1);
        break;
    case 1: /* tape full */
        tape_MTBSFM(pt, 1); /* wind after the last complete file*/
        tape_MTWEOF(pt, 1); /* write a second filemark */
        tape_MTREW(pt);     /* rewind */
        tape_MTOFFL(pt);    /* offline */
        close(pt);
        printf("Full tape capacity is %Lu bytes; %Lu bytes are used\n",
            (words_on_tape+words_on_file)*4, words_on_tape*4);
        words_on_tape=0;

        printf("Tape is full. Please insert a new tape and press ENTER.");
        fflush(stdout);
        fgets(s, 8, stdin);

        pt=open_tape(tape);
        if (pt<0) {
            return -1;
        }
        res=save_file_once_wrap(info, pt);
        switch (res) {
        case 0: /* success */
            tape_MTWEOF(pt, 1);
            break;
        case 1: /* tape full */
            /* ask_for_tape() */
            printf("File too big?\n");
            res=-1;
            break;
        case -1:
            tape_MTWEOF(pt, 1);
            break;
        }
        break;
    case -1:
        tape_MTWEOF(pt, 1);
        break;
    }
    return res;
}
/*****************************************************************************/
static int
save_files(int pt)
{
    int i, res;

    for (i=0; i<nfinfo; i++) {
        res=save_file(finfo+i, pt);
        if (res)
            return res;
    }
    return 0;
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
static void
sort_finfo(void)
{
    qsort(finfo, nfinfo, sizeof(struct finfo), finfo_compare);
}
/*****************************************************************************/
static void
dump_finfo(void)
{
    int i;

    printf("++++++++++++\n");
    for (i=0; i<nfinfo; i++) {
        printf("%s %d\n", finfo[i].name, finfo[i].compresstype);
    }
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
    int i, res=0, pt;

    if (readargs(argc, argv)<0)
        return 1;

    bsize=131072;
    buf=(ems_u32*)malloc(bsize*4);
    if (!buf) {
        printf("malloc %d words: %s\n", bsize, strerror(errno));
        return 3;
    }

    for (i=0; (i<nfnames) && !res; i++) {
        if (debug) printf("processing %s\n", fnames[i]);
        res=process_name(fnames[i]);
    }
    if (res) {
        printf("abort!\n");
        return 2;
    }

    /*dump_finfo();*/
    sort_finfo();
    /*dump_finfo();*/

    pt=open_tape(tape);
    if (pt<0)
        return 2;

    words_on_tape=0;
    words_on_file=0;
    save_files(pt);

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
