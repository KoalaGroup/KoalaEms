/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_multi_write.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_multi_write.c,v 1.6 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

int
sis3100_sfi_multi_write(struct fastbus_dev* dev, int num, ems_u32* list)
{
    struct fastbus_sis3100_sfi_info* info=(struct fastbus_sis3100_sfi_info*)dev->info;
    struct sis1100_writepipe req;

    req.num=num;
    req.am=0x39;
    req.data=list;

SIS3100_CHECK_BALANCE(dev, "sfi_multi_write_0");
    if (ioctl(info->p_vme, SIS1100_WRITE_PIPE, &req)<0) {
        printf("sfi_multi_write: %s\n", strerror(errno));
        return -1;
    }
    if (req.error) {
        printf("sfi_multi_write: 0x%x\n", req.error);
        return -1;
    }
SIS3100_CHECK_BALANCE(dev, "sfi_multi_write_1");
    return 0;
}
/*****************************************************************************/
