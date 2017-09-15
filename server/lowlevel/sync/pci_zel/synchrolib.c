/*
 * lowlevel/sync/pci_zel/synchrolib.c
 * created 12.09.96 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: synchrolib.c,v 1.9 2011/04/06 20:30:28 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#ifndef __linux__
#include <sys/proc.h>
#include <machine/bus.h>
#endif
#include <rcs_ids.h>
#include "synchrolib.h"

RCS_REGISTER(cvsid, "lowlevel/sync/pci_zel")

/*****************************************************************************/

int syncmod_map(struct syncmod_info* info)
{
    int offs;

    if ((info->path=open(info->pathname, O_RDWR, 0))<0) {
        printf("syncmod_map: open \"%s\": %s\n",
                info->pathname, strerror(errno));
        return -1;
    }
    if (fcntl(info->path, F_SETFD, FD_CLOEXEC)<0) {
        printf("synchrolib.c: fcntl(info->path, FD_CLOEXEC): %s\n",
                strerror(errno));
    }

#ifdef __linux__
/*
 * Offset should be a multiple of the page size as returned by getpagesize(2).
 * The address start must be a multiple of the page size.
 */
    info->mapsize=getpagesize();
    printf("syncmod_map: pagesize=%llu\n", (unsigned long long)info->mapsize);
    if (ioctl(info->path, PCISYNC_GETMAPOFFSET, &offs)<0) {
        printf("GETMAPOFFSET: %s\n", strerror(errno));
        close(info->path);
        return -1;
    }
    printf("syncmod_map: MAPOFFSET=%d\n", offs);

    printf("call mmap(0, %llu, PROT_READ|PROT_WRITE, MAP_SHARED, %d, 0)\n",
        (unsigned long long)info->mapsize, info->path);

    info->mapbase=mmap(0, info->mapsize, PROT_READ|PROT_WRITE, MAP_SHARED,
            info->path, 0);
    if (info->mapbase==MAP_FAILED) {
        printf("syncmod_map: mmap failed: %s\n", strerror(errno));
        close(info->path);
        return -1;
    }
    info->base=(volatile ems_u32*)((char*)info->mapbase+offs);
#else /* BSD */
    info->mapsize=0x40;
    if (ioctl(info->path, PCISYNC_GETMAPOFFSET, &offs)<0) {
        printf("GETMAPOFFSET: %s\n", strerror(errno));
        close(info->path);
        return -1;
    }
    if ((info->mapbase=mmap(0, info->mapsize, PROT_READ|PROT_WRITE, 0,
            info->path, (off_t)offs))==(void*)-1) {
        printf("syncmod_map: mmap failed: %s\n", strerror(errno));
        close(info->path);
        return -1;
    }
    info->base=(volatile ems_u32*)((char*)info->mapbase+offs);
#endif
    return 0;
}

/*****************************************************************************/

int syncmod_unmap(struct syncmod_info* info)
{
    int res=0;

    if (!info->valid)
        return 0;
    if (munmap(info->mapbase, info->mapsize)<0) {
        printf("syncmod_unmap: %s\n", strerror(errno));
        res=-1;
    }
    if (close(info->path)<0)
        res=-1;
    return res;
}

/*****************************************************************************/

int syncmod_intctrl(struct syncmod_info* info, struct syncintctrl* ctrl)
{
int res;

if (!info->valid) return 0;
res=ioctl(info->path, PCISYNC_INTCTRL, ctrl);
if (res==0)
  info->intctrl=*ctrl;
else
  {
  printf("syncmod_intctrl: %s\n", strerror(errno));
  }
return res;
}

/*****************************************************************************/
/*****************************************************************************/
