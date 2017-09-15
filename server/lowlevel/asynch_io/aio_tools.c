/*
 * lowlevel/asynch_io/aio_tools.c
 * created: 2007-Apr-23 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: aio_tools.c,v 1.4 2011/04/06 20:30:22 wuestner Exp $";

#define _GNU_SOURCE
#ifdef __linux__
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#define LINUX_LARGEFILE O_LARGEFILE
#else
#define LINUX_LARGEFILE 0
#endif

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <aio.h>
#ifndef HAVE_CGETCAP
#include <getcap.h>
#endif
#include <rcs_ids.h>

#include "asynch_io.h"
#include "aio_tools.h"

#define ALIGNBITS 12

#define HND ((struct aio_handle*)(hnd))
#define HNDp ((struct aio_handle*)(*hnd))

enum aio_status {aio_free=0, aio_used, aio_ready};

struct aio_buffer {
    char *buf;
    size_t size;
    struct aiocb aiocb;
    enum aio_status status;
};

struct aio_handle {
    int fd;
    int dir_write;
    int num_buffers;
    struct aio_buffer *aio_buffers;
    const struct aiocb **cblist;
    off_t file_ptr;
    int current_buffer;
    size_t current_position;
    char* fname;
};

static size_t defbufsize;
static int    defbufnum;

RCS_REGISTER(cvsid, "lowlevel/asynch_io")

/*****************************************************************************/
static long int
get_alignement(int p)
{
    long int pagesize;
    long int pathalign;

/*
 *     try to get the appropriate alignement for the given path
 */
    errno=0;
    pathalign=fpathconf(p, _PC_REC_XFER_ALIGN);
    /*printf("REC_XFER_ALIGN=%ld\n", pathalign);*/
    if (pathalign<0) {
        if (errno==0)
            printf("pathconf: REC_XFER_ALIGN not defined\n");
        else
            printf("pathconf(REC_XFER_ALIGN): %s\n", strerror(errno));
    } else {
        return pathalign;
    }

/*
 *     if fpathconf failed try to use PAGESIZE instead
 */
    errno=0;
    pagesize=sysconf(_SC_PAGESIZE);
    printf("PAGESIZE=%ld\n", pagesize);
    if (pagesize<0) {
        if (errno==0)
            printf("sysconf: PAGESIZE not defined\n");
        else
            printf("sysconf(PAGESIZE): %s\n", strerror(errno));
    } else {
        return pagesize;
    }
/*
 *     last resort: use (1<<ALIGNBITS)==4096 Byte
 */
    return 1<<ALIGNBITS;
}
/*****************************************************************************/
static int
async_init_read(struct aio_handle *hnd)
{
    int errcode=0;
    int i, j;

    /* start read */
    hnd->file_ptr=0;
    for (i=0; i<hnd->num_buffers; i++) {
        struct aio_buffer *cbuf=hnd->aio_buffers+i;
        /*printf("start block %d for %d\n", i, hnd->file_ptr);*/
        cbuf->aiocb.aio_fildes=hnd->fd;
        cbuf->aiocb.aio_reqprio=0;
        cbuf->aiocb.aio_buf=cbuf->buf;
        cbuf->aiocb.aio_nbytes=cbuf->size;
        cbuf->aiocb.aio_offset=hnd->file_ptr;
        hnd->file_ptr+=cbuf->size;

        if (aio_read(&cbuf->aiocb)<0) {
            errcode=errno;
            printf("aio_read[%d]: %s\n", hnd->current_buffer, strerror(errno));
            /* cancel all previous reads */
            for (j=0; j<i; j++) {
                cbuf=hnd->aio_buffers+i;
                int res;
                res=aio_cancel(hnd->fd, &cbuf->aiocb);
                if (res!=AIO_CANCELED && res!=AIO_ALLDONE) {
                    printf("aio_cancel[%di]: res=%d errno=%d\n", j, res, errno);
                }
            }
            goto error;
        }
        cbuf->status=aio_used;
    }
    hnd->current_buffer=0;
    hnd->current_position=0;

    return 0;

error:
    errno=errcode;
    return -1;
}
/*****************************************************************************/
static int
async_init_write(struct aio_handle *hnd)
{
    hnd->current_buffer=-1;
    hnd->file_ptr=0;
    return 0;
}
/*****************************************************************************/
/*
 * async_init initializes a handle for asynchronous IO
 * it opens the file, and allocates aligned buffers
 * parameters:
 *     fname: name of the file
 *     flags: open flags (see open(2))
 *            has to contain either O_RDONLY or O_WRONLY but not both
 *            should contain O_DIRECT if direct IO is desired
 *            O_LARGEFILE is automatically added for LINUX
 *     mode: permissions for a new file (see open(2))
 *     bufsize: size of a read/write buffer
 *     bufnum: number of read/write buffers
 *     handle: address of a pointer to the handle to be created
 * returns:
 *     0: success
 *     -1: if an error occurred (errno is set appropriately, handle is invalid)
 */
int
async_init(const char *fname, int flags, int mode,
        size_t bufsize, int bufnum, void **handle)
{
    struct aio_handle *hnd=0;
    int errcode=0;
    int i;
    unsigned long int alignsize;

    *handle=0;

    /* trivial test */
    if ((flags&O_RDWR)==O_RDWR) {
        errcode=EINVAL;
        printf("async_init(\"%s\"):either O_RDONLY or O_WRONLY "
                "(but not O_RDWR) expected\n",
                fname);
        goto error;
    }

    if ((hnd=calloc(1, sizeof(struct aio_handle)))==0) {
        errcode=errno;
        printf("async_init(\"%s\"):alloc handle: %s\n",
                fname, strerror(errno));
        goto error;
    }
    /* initialize field(s) which should not default to zero */
    hnd->fd=-1;
    hnd->dir_write=(flags&O_WRONLY)==O_WRONLY;

    if ((hnd->fname=strdup(fname))==0) {
        errcode=errno;
        printf("async_init(\"%s\"):strdup(fname): %s\n",
                fname, strerror(errno));
        goto error;
    }

    hnd->fd=open(fname, flags|LINUX_LARGEFILE, mode);
    if (hnd->fd<0) {
        errcode=errno;
        printf("async_init(\"%s\"):open: %s\n", fname, strerror(errno));
        goto error;
    }
    fcntl(hnd->fd, F_SETFD, FD_CLOEXEC);

    /* alignement is only really needed for direct IO */
    alignsize=get_alignement(hnd->fd);    

    hnd->aio_buffers=(struct aio_buffer*)calloc(bufnum,
            sizeof(struct aio_buffer));
    if (hnd->aio_buffers==0) {
        errcode=errno;
        printf("async_init(\"%s\"):alloc aio_buffers: %s\n",
                fname, strerror(errno));
        goto error;
    }
    hnd->cblist=(const struct aiocb**)calloc(bufnum, sizeof(struct aiocb*));
    if (hnd->cblist==0) {
        errcode=errno;
        printf("async_init(\"%s\"):alloc cblist: %s\n",
                fname, strerror(errno));
        goto error;
    }

    bufsize=(bufsize+alignsize-1)&~(alignsize-1); /* round up for alignement */
    for (i=0; i<bufnum; i++) {
        void *ptr;
        errcode=posix_memalign(&ptr, alignsize, bufsize);
        if (errcode!=0) {
            printf("async_init(\"%s\"):posix_memalign[%d]: %s\n",
                    fname, i, strerror(errcode));
            goto error;
        }
        //printf("buf[%d]: %p\n", i, aio_buffers[i].buf);
        hnd->aio_buffers[i].buf=ptr;
        hnd->aio_buffers[i].size=bufsize;
    }
    hnd->num_buffers=bufnum;

    if ((hnd->dir_write?async_init_write(hnd):async_init_read(hnd))<0)
        goto error;

    *handle=hnd;
    return 0;

error:
    if (hnd) {
        if (hnd->fd>=0)
            close(hnd->fd);
        if (hnd->aio_buffers) {
            for (i=0; i<bufnum; i++) {
                if (hnd->aio_buffers[i].buf)
                    free(hnd->aio_buffers[i].buf);
            }
            free(hnd->aio_buffers);
        }
        if (hnd->cblist)
            free(hnd->cblist);
        if (hnd->fname)
            free(hnd->fname);
        free(hnd);
    }
    errno=errcode;
    return -1;
}
/*****************************************************************************/
static int
async_find_free_buf(struct aio_handle *hnd)
{
    int i;
    for (i=0; i<hnd->num_buffers; i++) {
        if (hnd->aio_buffers[i].status==aio_free)
            return i;
    }
    return -1;
}
/*****************************************************************************/
static int
async_collect_status(struct aio_handle *hnd, char rw)
{
    int i, err, n=0;

    for (i=0; i<hnd->num_buffers; i++) {
        if (hnd->aio_buffers[i].status==aio_used) {
            n++;
            err=aio_error(&hnd->aio_buffers[i].aiocb);
            /*printf("async_collect_status[%d%c]: %s\n",
                    i, rw, strerror(err));*/
            if (err!=EINPROGRESS) {
                ssize_t res;
                res=aio_return(&hnd->aio_buffers[i].aiocb);
                if (res<=0)
                    printf("aio_return[%d%c]: res=%lld\n",
                            i, rw, (long long)res);
                hnd->aio_buffers[i].status=aio_free;
                n--;
            }
        }
    }
    return n;
}
/*****************************************************************************/
static int
async_get_free_buf(struct aio_handle *hnd, char rw)
{
    int i, n;

    if ((i=async_find_free_buf(hnd))>=0)
        return i;
    /* wait for finished buffers */
    n=0;
    for (i=0; i<hnd->num_buffers; i++) {
        if (hnd->aio_buffers[i].status==aio_used) {
            hnd->cblist[n++]=&hnd->aio_buffers[i].aiocb;
        }
    }
    if (n==0) {
        printf("logic error in get_free_async_buf: all bufs are "
                "neither free nor used\n");
        return -1;
    }

    if (aio_suspend(hnd->cblist, n, 0)<0) {
        printf("aio_suspend: %s\n", strerror(errno));
        return -1;
    }

    async_collect_status(hnd, rw);

    if ((i=async_find_free_buf(hnd))>=0) {
        return i;
    }
    printf("no free bufs after aio_suspend\n");
    return -1;
}
/*****************************************************************************/
static ssize_t
async_write_(struct aio_handle *hnd, const void *data, size_t count)
{
    struct aio_buffer *cbuf;
    ssize_t res;

    /* do we need a new buffer? */
    if (hnd->current_buffer<0) {
        int i;
        /* get an unused buffer (or wait for one) */
        i=async_get_free_buf(hnd, 'w');
        if (i<0)
            return -1;
        hnd->current_buffer=i;
        hnd->current_position=0;
    }

    cbuf=hnd->aio_buffers+hnd->current_buffer;

    /* how many data fit in the buffer? */
    if (hnd->current_position+count>cbuf->size)
        res=cbuf->size-hnd->current_position;
    else
        res=count;

    /* append data to the end of current buffer */
    memcpy(cbuf->buf+hnd->current_position, data, res);
    hnd->current_position+=res;
    /* buffer full? */
    if (hnd->current_position==cbuf->size) {
        /* start aio */
        cbuf->aiocb.aio_fildes=hnd->fd;
        cbuf->aiocb.aio_reqprio=0;
        cbuf->aiocb.aio_buf=cbuf->buf;
        cbuf->aiocb.aio_nbytes=cbuf->size;
        cbuf->aiocb.aio_offset=hnd->file_ptr;
        hnd->file_ptr+=cbuf->size;

        if (aio_write(&cbuf->aiocb)<0) {
            printf("aio_write[%d]: %s\n", hnd->current_buffer, strerror(errno));
            res=-1;
        } else {
            //printf("buffer %d started\n", current_buffer);
        }
        hnd->current_buffer=-1;
        cbuf->status=aio_used;
    }
    return res;
}
/*****************************************************************************/
ssize_t
async_write(void *hnd, const void *data, size_t count)
{
    ssize_t res;
    size_t pos=0;

    while (pos<count) {
        res=async_write_(HND, (char*)data+pos, count-pos);
        if (res<=0) {
            printf("async_write_all: pos=%llu, count=%llu, res=%lld errno=%s\n",
                    (unsigned long long)pos, (unsigned long long)count,
                    (long long)res, strerror(errno));
            return res;
        }
        pos+=res;
    }
    return count;
}
/*****************************************************************************/
ssize_t
async_read(void *hnd, void **buf, size_t count)
{
    int bi=HND->current_buffer%HND->num_buffers;
    struct aio_buffer *cbuf=HND->aio_buffers+bi;
    ssize_t res;

    if (HND->current_position==0) {
        int err;

        /* start next read on previous buffer */
        if (HND->current_buffer>0) {
            struct aio_buffer *pcbuf=HND->aio_buffers+(HND->current_buffer-1)%HND->num_buffers;
            //printf("restart block %d for %d\n", HND->current_buffer-1, HND->file_ptr);
            pcbuf->aiocb.aio_fildes=HND->fd;
            pcbuf->aiocb.aio_reqprio=0;
            pcbuf->aiocb.aio_buf=pcbuf->buf;
            pcbuf->aiocb.aio_nbytes=pcbuf->size;
            pcbuf->aiocb.aio_offset=HND->file_ptr;
            HND->file_ptr+=pcbuf->size;

            if (aio_read(&pcbuf->aiocb)<0) {
                printf("aio_read[%d]: %s\n",
                        HND->current_buffer-1, strerror(errno));
                return -1;
            }
            pcbuf->status=aio_used;
        }

        /* wait for the current buffer to become ready */
        HND->cblist[0]=&cbuf->aiocb;
        if (aio_suspend(HND->cblist, 1, 0)<0) {
            printf("aio_suspend: %s\n", strerror(errno));
            return -1;
        }
        /* read the status */
        err=aio_error(&cbuf->aiocb);
        if (err!=0) {
            printf("aio_error[%dr]: err=%d\n", bi, err);
            return -1;
        }
        res=aio_return(&HND->aio_buffers[bi].aiocb);
        if (res<0) {
            printf("aio_return[%dr]: res=%lld\n", bi, (long long)res);
            return -1;
        } else if (res<cbuf->size) {
            /* end of file reached */
            printf("aio_return[%dr]: res=%lld\n", bi, (long long)res);
            cbuf->aiocb.aio_nbytes=res;
            /* OK */
        } else {
            /* OK */
        }
        cbuf->status=aio_ready;
    }
    *buf=((char*)cbuf->buf)+HND->current_position;
    if (HND->current_position+count>=cbuf->aiocb.aio_nbytes) {
        res=cbuf->aiocb.aio_nbytes-HND->current_position;
        HND->current_position=0;
        HND->current_buffer++;
    } else {
        HND->current_position+=count;
        res=count;
    }
/*
 *     printf("current_buffer=%d current_position=%d file_ptr=%d res=%d\n",
 *             HND->current_buffer,
 *             HND->current_position,
 *             HND->file_ptr,
 *             res);
 */
    return res;
}
/*****************************************************************************/
static int
async_done_write(struct aio_handle *hnd)
{
    int errcode=0, i, n;

    /* write the last buffer */
    if (hnd->current_buffer>=0) {
        struct aio_buffer *cbuf=hnd->aio_buffers+hnd->current_buffer;
        printf("will flush last buffer, size=%llu\n",
                (unsigned long long)hnd->current_position);
        cbuf->aiocb.aio_fildes=hnd->fd;
        cbuf->aiocb.aio_reqprio=0;
        cbuf->aiocb.aio_buf=cbuf->buf;
        cbuf->aiocb.aio_nbytes=cbuf->size; /* write full buffer, truncate later */
        cbuf->aiocb.aio_offset=hnd->file_ptr;
        hnd->file_ptr+=hnd->current_position;
        if (aio_write(&cbuf->aiocb)<0) {
            errcode=errno;
            printf("aio_write(last buffer)[%d]: %s\n",
                    hnd->current_buffer, strerror(errno));
            goto error;
        }
    }

    /* find a buffer still in use */
    for (i=0; i<hnd->num_buffers; i++) {
        if (hnd->aio_buffers[i].status==aio_used)
            break;
    }
    if (hnd->aio_buffers[i].status!=aio_used) {
        /*printf("async_fsync: no buffer in use\n");*/
        goto wait_ok;
    }

#if 0
    if (aio_fsync(O_DSYNC, &hnd->aio_buffers[i].aiocb)<0) {
        errcode=errno;
        printf("aio_fsync: %s\n", strerror(errno));
        goto error;
    }
#endif

    /* wait for all buffers */
    do {
        /* wait for finished buffers */
        n=0;
        for (i=0; i<hnd->num_buffers; i++) {
            if (hnd->aio_buffers[i].status==aio_used) {
                hnd->cblist[n++]=&hnd->aio_buffers[i].aiocb;
            }
        }
        if (n==0)
            break;

        /*printf("waiting for %d buffers\n", n);*/

        if (aio_suspend(hnd->cblist, n, 0)<0) {
            errcode=errno;
            printf("aio_suspend: %s\n", strerror(errno));
            goto error;
        }

        n=async_collect_status(hnd, 'w');

    } while (n>0);

wait_ok:

    /* truncate to the correct size */
    printf("will truncate at %lld\n", (long long)hnd->file_ptr);
    if (ftruncate(hnd->fd, hnd->file_ptr)<0) {
        errcode=errno;
        printf("async_fsync:ftruncate: %s\n", strerror(errno));
        goto error;
    }
    return 0;

error:
    errno=errcode;
    return -1;
}
/*****************************************************************************/
static int
async_done_read(struct aio_handle *hnd)
{
    int i;
    /* cancel all reads if they are still active */
    for (i=hnd->num_buffers-1; i>=0; i--) {
        struct aio_buffer *cbuf=hnd->aio_buffers+i;
        if (cbuf->status==aio_used) {
            int res;
            res=aio_cancel(hnd->fd, &cbuf->aiocb);
            if (res!=AIO_CANCELED && res!=AIO_ALLDONE) {
                if (res==AIO_NOTCANCELED) {
                    printf("async_done_read: block %d not cancelled, "
                            "will wait for completion\n", i);
                    hnd->cblist[0]=&cbuf->aiocb;
                    if (aio_suspend(hnd->cblist, 1, 0)<0) {
                        printf("aio_suspend: %s\n", strerror(errno));
                    }
                }
            }
        }
    }
    return 0;
}
/*****************************************************************************/
int
async_done(void **hnd)
{
    int i;
    int errcode=0;

    if ((HNDp->dir_write?async_done_write(HNDp):async_done_read(HNDp))<0)
        errcode=errno;

    close(HNDp->fd);

    for (i=0; i<HNDp->num_buffers; i++) {
        free(HNDp->aio_buffers[i].buf);
    }
    free(HNDp->aio_buffers);
    free(HNDp->cblist);
    free(HNDp->fname);
    free(*hnd);
    *hnd=0;

    errno=errcode;
    return errcode?-1:0;
}
/*****************************************************************************/
size_t async_defbufsize(void)
{
    return defbufsize;
}
int async_defbufnum(void)
{
    return defbufnum;
}
/*****************************************************************************/
int asynch_io_low_printuse(FILE* outfilepath)
{
    fprintf(outfilepath,
        "  [:aiobufsize#size][:aiobufnum#num]\n"
        "    defaults: aiobufsize#%d aiobufnum#%d\n",
        AIO_BUFSIZE, AIO_BUFNUM);
    return 1;
}
/*****************************************************************************/
errcode asynch_io_low_init(char* arg)
{
    long argval;

    defbufsize=AIO_BUFSIZE;
    defbufnum=AIO_BUFNUM;
    if (arg!=0) {
        if (cgetnum(arg, "iobufsize", &argval)==0)
            defbufsize=argval;
        if (cgetnum(arg, "aiobufnum", &argval)==0)
            defbufnum=argval;
    }
    return OK;
}
/*****************************************************************************/
errcode asynch_io_low_done()
{
    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
