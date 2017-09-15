/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_open_dev.c
 * created 2003
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_open_dev.c,v 1.8 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <rcs_ids.h>

#include "sis3100_sfi_open_dev.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

static const char *sis1100names[]={"remote", "ram", "ctrl", "dsp"};

int
sis3100_sfi_open_dev(const char* pathname, enum sis1100_subdev subdev)
{
    char pname[MAXPATHLEN];
    enum sis1100_subdev _subdev;
    int p;

    snprintf(pname, MAXPATHLEN, "%s%s", pathname, sis1100names[subdev]);
    p=open(pname, O_RDWR, 0);
    if (p<0) {
        printf("open FASTBUS (sis3100_sfi) %s: %s\n", pname, strerror(errno));
        return -1;
    }
    if (ioctl(p, SIS1100_DEVTYPE, &_subdev)<0) {
        printf("ioctl(fastbus %s, SIS1100_DEVTYPE): %s\n",
                pname, strerror(errno));
    } else {
        if (_subdev!=subdev) {
            printf("%s has wrong device type (%d instead of %d)\n", pname,
                    _subdev, subdev);
            close(p);
            return -1;
        }
    }

    if (fcntl(p, F_SETFD, FD_CLOEXEC)<0) {
        printf("fcntl(fastbus %s, FD_CLOEXEC): %s\n", pname, strerror(errno));
    }

printf("sfi_open: %s open as %d\n", pathname, p);

    return p;
}
/*****************************************************************************/
/*****************************************************************************/
