/*
 * lowlevel/unixvme/sis3100/read_dsp_raw.c
 * PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: read_dsp_raw.c,v 1.5 2011/04/06 20:30:28 wuestner Exp $";

#define _GNU_SOURCE

#include <sconf.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif
#include <rcs_ids.h>

#ifndef MAP_VARIABLE
#define MAP_VARIABLE 0x0000
#endif

#include "dspcode.h"

struct opaque_info {
    void *addr;
    size_t len;
    int path;
};

RCS_REGISTER(cvsid, "lowlevel/unixvme/sis3100")

static int
free_dsp_raw(struct code_info* info)
{
    struct opaque_info* info_=info->opaque_info;
    munmap(info_->addr, info_->len);
    close(info_->path);
    free(info_);
    return 0;
}

size_t
read_dsp_raw(int p, const char* codepath, u_int8_t** code,
        struct code_info* info)
{
    struct stat stat;
    void *mp;
    struct opaque_info* info_;

    info->free=0;

    if (fstat(p, &stat)<0) {
        printf("cannot stat \"%s\": %s\n", codepath, strerror(errno));
        close(p);
        return -1;
    }
    mp=mmap(0, stat.st_size, PROT_READ,
            MAP_VARIABLE|MAP_FILE|MAP_SHARED, p, 0);
    if (mp==MAP_FAILED) {
        printf("cannot mmap \"%s\": %s\n", codepath, strerror(errno));
        close(p);
        return -1;
    }
    *code=mp;

    info_=malloc(sizeof(struct opaque_info));
    if (!info_) {
        printf("malloc info (%lld bytes): %s\n",
                (unsigned long long)(sizeof(struct opaque_info)),
                strerror(errno));
        munmap(mp, stat.st_size);
        close(p);
        return -1;
    }
    info_->addr=mp;
    info_->len=stat.st_size;
    info_->path=p;
    info->opaque_info=info_;
    info->free=free_dsp_raw;

    return stat.st_size;
}
