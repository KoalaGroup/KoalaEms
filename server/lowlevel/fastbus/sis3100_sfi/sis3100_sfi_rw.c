/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_rw.c
 * 
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_rw.c,v 1.9 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

int
sis3100_sfi_read(struct fastbus_dev* dev, int ofs, volatile ems_u32* data)
{
    struct fastbus_sis3100_sfi_info* info=(struct fastbus_sis3100_sfi_info*)dev->info;
    struct sis1100_vme_req req;
    req.size=4;
    req.am=0x39;
    req.addr=SFI_BASE+ofs;
    if (ioctl(info->p_vme, SIS3100_VME_READ, &req)<0) {
        printf("sfi_read(0x%08x): %s\n", ofs, strerror(errno));
        return -1;
    }
    if (req.error) {
        printf("sfi_read(0x%08x): 0x%x\n", ofs, req.error);
        return -1;
    }
    *data=req.data;
    return 0;
}

int
sis3100_sfi_write(struct fastbus_dev* dev, int ofs, volatile ems_u32 v)
{
    struct fastbus_sis3100_sfi_info* info=(struct fastbus_sis3100_sfi_info*)dev->info;
    struct sis1100_vme_req req;
    req.size=4;
    req.am=0x39;
    req.addr=SFI_BASE+ofs;
    req.data=v;
#if 0
    printf("sfi_write(addr=0x%08x size=%d am=0x%02x val=0x%08x)\n",
        req.addr, req.size, req.am, req.data);
#endif
    if (ioctl(info->p_vme, SIS3100_VME_WRITE, &req)<0) {
        printf("sfi_write(0x%08x, 0x%08x): %s\n", ofs, v, strerror(errno));
        return -1;
    }
    if (req.error) {
        printf("sfi_write(0x%08x, 0x%08x): 0x%x\n", ofs, v, req.error);
        return -1;
    }
    return 0;
}

#ifndef SIS3100_SFI_MMAPPED
int
sis3100_vme_read(struct fastbus_dev* dev, int ofs, volatile ems_u32* data)
{
    struct fastbus_sis3100_sfi_info* info=(struct fastbus_sis3100_sfi_info*)dev->info;
    struct sis1100_ctrl_reg req;
    req.offset=ofs;
    if (ioctl(info->p_vme, SIS1100_CTRL_READ, &req)<0) {
        printf("vme_read(0x%08x): %s\n", ofs, strerror(errno));
        return -1;
    }
    if (req.error) {
        printf("vme_read(0x%08x): 0x%x\n", ofs, req.error);
        return -1;
    }
    *data=req.val;
    return 0;
}

int
sis3100_vme_write(struct fastbus_dev* dev, int ofs, volatile ems_u32 v)
{
    struct fastbus_sis3100_sfi_info* info=(struct fastbus_sis3100_sfi_info*)dev->info;
    struct sis1100_ctrl_reg req;
    req.offset=ofs;
    req.val=v;

    if (ioctl(info->p_vme, SIS1100_CTRL_WRITE, &req)<0) {
        printf("vme_write(0x%08x, 0x%08x): %s\n", ofs, v, strerror(errno));
        return -1;
    }
    if (req.error) {
        printf("vme_write(0x%08x, 0x%08x): 0x%x\n", ofs, v, req.error);
        return -1;
    }
    return 0;
}
#endif
/*****************************************************************************/
/*****************************************************************************/
