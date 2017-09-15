/*
 * lowlevel/unixvme/unixvme_jtag.c
 * created 2005-Aug-22 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: unixvme_jtag.c,v 1.5 2011/04/06 20:30:28 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "vme.h"
#include "unixvme_jtag.h"
#include "../jtag/jtag_lowlevel.h"
#include "dev/pci/sis1100_var.h"
#include "dev/pci/sis1100_map.h"

#ifdef UNIXVME_SIS3100
#include "sis3100/vme_sis.h"
#endif
#ifdef DMALLOC
#include <dmalloc.h>
#endif


struct opaque_data_sis1100 {
    int p;
};

RCS_REGISTER(cvsid, "lowlevel/unixvme")

/****************************************************************************/
static int
jt_action_sis1100(struct jtag_dev* jdev, int tms, int tdi, int* tdo)
{
    struct opaque_data_sis1100* opaque=
            (struct opaque_data_sis1100*)jdev->opaque;
    ems_u32 csr, tdo_;

    csr=SIS1100_JT_C|(tms?SIS1100_JT_TMS:0)|(tdi?SIS1100_JT_TDI:0);

    if (ioctl(opaque->p, SIS1100_JTAG_PUT, &csr))
        return -1;
    if (ioctl(opaque->p, SIS1100_JTAG_GET, &csr))
        return -1;
    tdo_=!!(csr&SIS1100_JT_TDO);

    if (tdo!=0)
        *tdo=tdo_;
    return 0;
}
/****************************************************************************/
static int
jt_data_sis1100(struct jtag_dev* jdev, ems_u32* v)
{
    struct opaque_data_sis1100* opaque=
            (struct opaque_data_sis1100*)jdev->opaque;
    int res;
    res=ioctl(opaque->p, SIS1100_JTAG_DATA, v);
    return res;
}
/****************************************************************************/
static void
jt_free(struct jtag_dev* jdev)
{
    free(jdev->opaque);
    jdev->opaque=0;
}
/****************************************************************************/
plerrcode
unixvme_init_jtag_dev(struct generic_dev* gdev, void* jdev_)
{
    struct vme_dev* dev=(struct vme_dev*)gdev;
    struct jtag_dev* jdev=(struct jtag_dev*)jdev_;
    void* opaque=0;
    plerrcode pres=plOK;

    switch (jdev->ci) {
    case 2: { /* sis1100 */
        struct vme_sis_info *info;
        if (dev->vmetype!=vme_sis3100) {
            printf("unixvme_init_jdev: unusable vmetype %d\n", dev->vmetype);
            pres=plErr_BadModTyp;
            goto error;
        }
        if (jdev->addr!=0) {
            printf("unixvme_init_jdev: illegal sis1100 address %d\n", jdev->addr);
            pres=plErr_ArgRange;
            goto error;
        }
        info=(struct vme_sis_info*)dev->info;
        jdev->jt_data=jt_data_sis1100;
        jdev->jt_action=jt_action_sis1100;
        jdev->jt_free=jt_free;
        opaque=malloc(sizeof(struct opaque_data_sis1100));
        if (opaque==0) {
            printf("lowlevel/unixvme/unixvme_jtag.c:init_jtag_dev: %s\n",
                strerror(errno));
            return plErr_System;
        }
        ((struct opaque_data_sis1100*)opaque)->p=info->p_ctrl;
      } break;
    default:
        printf("unixvme_init_jdev: invalid CIcode %d\n", jdev->ci);
        pres=plErr_ArgRange;
        goto error;
    }
    jdev->opaque=opaque;
    return plOK;

error:
    free(opaque);
    jdev->opaque=0;
    return pres;
}
/*****************************************************************************/
/*****************************************************************************/
