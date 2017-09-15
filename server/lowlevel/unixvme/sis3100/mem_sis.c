/*
 * lowlevel/unixvme/sis3100/mem_sis.c
 * created: 27.Jun.2003
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: mem_sis.c,v 1.9 2011/04/06 20:30:28 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <rcs_ids.h>

#include "dev/pci/sis1100_var.h"
#include "../vme.h"
#include "vme_sis.h"
#include "mem_sis.h"
#include "../../../objects/pi/readout.h"

RCS_REGISTER(cvsid, "lowlevel/unixvme/sis3100")

/*****************************************************************************/
int sis3100_mem_size(struct vme_dev* dev)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    ems_u32 size;
    int res;

    res=ioctl(info->p_mem, SIS1100_MAPSIZE, &size);
    if (res<0) {
        printf("sis3100_mem_size: %s\n", strerror(errno));
        return -1;
    }
    return size;
}

int sis3100_mem_write(struct vme_dev* dev, ems_u32* buffer, unsigned int size,
        ems_u32 addr)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    if (pwrite(info->p_mem, buffer, size, addr)<0) {
        printf("sis3100_mem_write: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int sis3100_mem_read(struct vme_dev* dev, ems_u32* buffer, unsigned int size,
        ems_u32 addr)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    if (pread(info->p_mem, buffer, size, addr)<0) {
        printf("sis3100_mem_read: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

#ifdef DELAYED_READ
int sis3100_mem_read_delayed(struct vme_dev* dev, ems_u32* buffer,
        unsigned int size, ems_u32 addr, ems_u32 nextbuffer)
{
    struct generic_dev* gdev=(struct generic_dev*)dev;
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    int delayed_read=inside_readout && gdev->generic.delayed_read_enabled;

    if (delayed_read) {
        if (sis3100_vme_delay_read(dev, addr, buffer, size)<0) return -1;
        if (pwrite(info->p_mem, &nextbuffer, 4, info->bufferaddr)<0) {
            printf("sis3100_mem_read_delayed: pwrite: %s\n", strerror(errno));
            return -1;
        }
    } else {
        if (pread(info->p_mem, buffer, size, addr)<0) {
            printf("sis3100_mem_read_delayed: pread: %s\n", strerror(errno));
            return -1;
        }
    }
    return 0;
}
#endif

void sis3100_mem_set_bufferstart(struct vme_dev* dev, ems_u32 bufferstart,
        ems_u32 bufferaddr)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;

    info->bufferstart=bufferstart;
    info->bufferaddr=bufferaddr;
}
