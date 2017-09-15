/*
 * lowlevel/unixvme/sis3100/dsp_sis.c
 * created: 27.Jun.2003
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: dsp_sis.c,v 1.6 2011/04/06 20:30:28 wuestner Exp $";

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
#include "dev/pci/sis3100_map.h"
#include "../vme.h"
#include "vme_sis.h"
#include "dsp_sis.h"
#include "load_dsp.h"

#define ofs(what, elem) ((size_t)&(((what *)0)->elem))

RCS_REGISTER(cvsid, "lowlevel/unixvme/sis3100")

/*****************************************************************************/
int sis3100_dsp_present (struct vme_dev* dev)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_ctrl_reg reg;
    int res;

    reg.offset=ofs(struct sis3100_reg, dsp_sc);
    res=ioctl(info->p_dsp, SIS1100_CTRL_READ, &reg);
    if (res<0) {
        printf("sis3100_dsp_present: %s\n", strerror(errno));
        return -1;
    }
    if (reg.error) {
        printf("sis3100_dsp_present: error=0x%x\n", reg.error);
        return -1;
    }
    return !!(reg.val&sis3100_dsp_available);
}

int sis3100_dsp_reset (struct vme_dev* dev)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_ctrl_reg reg;
    int res;

    reg.offset=ofs(struct sis3100_reg, dsp_sc);
    reg.val=0x800;
    res=ioctl(info->p_dsp, SIS1100_CTRL_WRITE, &reg);
    if (res<0) {
        printf("sis3100_dsp_reset: %s\n", strerror(errno));
        return -1;
    }
    if (reg.error) {
        printf("sis3100_dsp_reset: error=0x%x\n", reg.error);
        return -1;
    }
    reg.offset=ofs(struct sis3100_reg, dsp_sc);
    reg.val=0x1000000;
    res=ioctl(info->p_dsp, SIS1100_CTRL_WRITE, &reg);
    if (res<0) {
        printf("sis3100_dsp_reset: %s\n", strerror(errno));
        return -1;
    }
    if (reg.error) {
        printf("sis3100_dsp_reset: error=0x%x\n", reg.error);
        return -1;
    }
    return 0;
}

int sis3100_dsp_start (struct vme_dev* dev)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_ctrl_reg reg;
    int res;

    reg.offset=ofs(struct sis3100_reg, dsp_sc);
    reg.val=0x100;
    res=ioctl(info->p_dsp, SIS1100_CTRL_WRITE, &reg);
    if (res<0) {
        printf("sis3100_dsp_start: %s\n", strerror(errno));
        return -1;
    }
    if (reg.error) {
        printf("sis3100_dsp_start: error=0x%x\n", reg.error);
        return -1;
    }
    return 0;
}

int sis3100_dsp_load (struct vme_dev* dev, const char* codepath)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    return load_dsp_file(info->p_dsp, codepath);
}
