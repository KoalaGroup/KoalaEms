/*
 * lowlevel/unixvme/sis3153_usb/vme_sis_usb.c
 * created 2016-01-21 PeWue
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: vme_sis_usb.c,v 1.1 2016/02/29 17:33:10 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include "../../../commu/commu.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <rcs_ids.h>

#include "../vme.h"
#include "../vme_address_modifiers.h"
#include "vme_sis_usb.h"

RCS_REGISTER(cvsid, "lowlevel/unixvme/sis3153_usb")

/*****************************************************************************/

/*****************************************************************************/
errcode
vme_init_sis3153_usb(struct vme_dev* dev)
{
    printf("vme_init_sis3100_usb: path=%s\n", dev->pathname);
    printf("vme_init_sis3100_usb not implemeted yet\n");

    return Err_NotImpl;
}
/*****************************************************************************/
/*****************************************************************************/
