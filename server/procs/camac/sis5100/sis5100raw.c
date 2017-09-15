/*
 * procs/camac/sis5100/sis5100raw.
 * created 2006-Jul-29 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis5100raw.c,v 1.2 2011/04/06 20:30:30 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <modulnames.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/devices.h"
#include "../../../lowlevel/camac/camac.h"

#include "dev/pci/sis1100_var.h"

#ifdef DMALLOC
#include dmalloc.h
#endif

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

#define get_camacdevice(crate) \
    (struct camac_dev*)get_gendevice(modul_camac, (crate))
#define ofs(what, elem) ((int)&(((what *)0)->elem))

RCS_REGISTER(cvsid, "procs/camac/sis5100")

/*****************************************************************************/
char name_proc_sis5100_remote_write[] = "sis5100_remote_write";
int ver_proc_sis5100_remote_write = 1;
/*
 * p[0]: argcount==5
 * p[1]: crate ID
 * p[2]: size
 * p[3]: am
 * p[4]: addr
 * p[5]: data
 */
plerrcode proc_sis5100_remote_write(ems_u32* p)
{
    struct camac_dev* dev=get_camacdevice(p[1]);
    struct camac_raw_procs* raw_procs=dev->get_raw_procs(dev);

    if (!raw_procs)
        return plErr_NoSuchProc;
    if (!raw_procs->sis1100_remote_write)
        return plErr_NoSuchProc;

    if (raw_procs->sis1100_remote_write(dev, p[2], p[3], p[4], p[5], outptr))
        return plErr_System;
    outptr++;
    return plOK;
}

plerrcode test_proc_sis5100_remote_write(ems_u32* p)
{
    if (p[0]!=5)
        return plErr_ArgNum;

    wirbrauchen=1;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis5100_remote_read[] = "sis5100_remote_read";
int ver_proc_sis5100_remote_read = 1;
/*
 * p[0]: argcount==4
 * p[1]: crate ID
 * p[2]: size
 * p[3]: am
 * p[4]: addr
 */
plerrcode proc_sis5100_remote_read(ems_u32* p)
{
    struct camac_dev* dev=get_camacdevice(p[1]);
    struct camac_raw_procs* raw_procs=dev->get_raw_procs(dev);

    if (!raw_procs)
        return plErr_NoSuchProc;
    if (!raw_procs->sis1100_remote_read)
        return plErr_NoSuchProc;

    if (raw_procs->sis1100_remote_read(dev, p[2], p[3], p[4], outptr+1, outptr))
        return plErr_System;
    outptr+=2;
    return plOK;
}

plerrcode test_proc_sis5100_remote_read(ems_u32* p)
{
    if (p[0]!=4)
        return plErr_ArgNum;

    wirbrauchen=2;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis5100_local_write[] = "sis5100_local_write";
int ver_proc_sis5100_local_write = 1;
/*
 * p[0]: argcount==3
 * p[1]: crate ID
 * p[2]: offset
 * p[3]: val
 */
plerrcode proc_sis5100_local_write(ems_u32* p)
{
    struct camac_dev* dev=get_camacdevice(p[1]);
    struct camac_raw_procs* raw_procs=dev->get_raw_procs(dev);

    if (!raw_procs)
        return plErr_NoSuchProc;
    if (!raw_procs->sis1100_local_write)
        return plErr_NoSuchProc;

    if (raw_procs->sis1100_local_write(dev, p[2], p[3], outptr))
        return plErr_System;
    outptr++;
    return plOK;
}

plerrcode test_proc_sis5100_local_write(ems_u32* p)
{
    if (p[0]!=3)
        return plErr_ArgNum;

    wirbrauchen=1;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis5100_local_read[] = "sis5100_local_read";
int ver_proc_sis5100_local_read = 1;
/*
 * p[0]: argcount==3
 * p[1]: crate ID
 * p[2]: offset
 */
plerrcode proc_sis5100_local_read(ems_u32* p)
{
    struct camac_dev* dev=get_camacdevice(p[1]);
    struct camac_raw_procs* raw_procs=dev->get_raw_procs(dev);

    if (!raw_procs)
        return plErr_NoSuchProc;
    if (!raw_procs->sis1100_local_read)
        return plErr_NoSuchProc;

    if (raw_procs->sis1100_local_read(dev, p[2], outptr+1, outptr))
        return plErr_System;
    outptr+=2;
    return plOK;
}

plerrcode test_proc_sis5100_local_read(ems_u32* p)
{
    if (p[0]!=2)
        return plErr_ArgNum;

    wirbrauchen=2;
    return plOK;
}
/*****************************************************************************/
/*****************************************************************************/
