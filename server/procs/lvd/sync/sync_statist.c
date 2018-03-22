/*
 * procs/lvd/sync/sysnc_statist.c
 * created: 2007-02-06
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sync_statist.c,v 1.9 2017/10/09 21:25:38 wuestner Exp $";

#include <sconf.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procprops.h"
#include "../../procs.h"
#include "../../../lowlevel/lvd/lvd_sync_statist.h"
#include "../../../lowlevel/lvd/sync/sync.h"

extern ems_u32* outptr;

#define get_device(crate) \
    (struct lvd_dev*)get_gendevice(modul_lvd, (crate))

RCS_REGISTER(cvsid, "procs/lvd/sync")

/*****************************************************************************/
/*
 * p[0]: argcount==0
 * p[1]: device idx
 */
plerrcode
proc_lvd_sync_statist_clear(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    lvd_sync_statist_clear(dev);
    return plOK;
}

plerrcode test_proc_lvd_sync_statist_clear(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_device(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}
char name_proc_lvd_sync_statist_clear[]="lvd_sync_statist_clear";
int ver_proc_lvd_sync_statist_clear=1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: device idx
 */
plerrcode
proc_lvd_sync_statist_dump(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    lvd_sync_statist_dump(dev);
    return plOK;
}

plerrcode test_proc_lvd_sync_statist_dump(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_device(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}
char name_proc_lvd_sync_statist_dump[]="lvd_sync_statist_dump";
int ver_proc_lvd_sync_statist_dump=1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: device idx
 * p[2]: flags:
 *         0x1: request trigger distribution            (2*16)
 *         0x2: request deadtimes per crate             (max. 128)
 *         0x4: request deadtimes per trigger bit       (max. 16)
 *         0x8: request deadtimes per trigger pattern   (max. 65535)
 * p[3]: maximum number of entries for (flag&2)!=0
 */
plerrcode
proc_lvd_get_sync_statist(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int num;
    plerrcode pres;

    pres=lvd_get_sync_statist(dev, outptr+1, &num, p[2], p[3]);
    *outptr=num;
    outptr+=num+1;
    return pres;
}

plerrcode test_proc_lvd_get_sync_statist(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_device(modul_lvd, p[1], 0))!=plOK)
        return pres;
    int flags=p[2];
    wirbrauchen=13; /* eventnr, time (64 bit), accepted, rejected, tdt */
    if (flags&1)
        wirbrauchen+=1+32*6;
    if (flags&2)
        wirbrauchen+=1+32*6;
    if (flags&4)
        wirbrauchen+=1+128*6;
    if (flags&8)
        wirbrauchen+=1+65536;
    return plOK;
}
char name_proc_lvd_get_sync_statist[]="lvd_get_sync_statist";
int ver_proc_lvd_get_sync_statist=1;
/*****************************************************************************/
#if 0
/*
 * p[0]: argcount==0|1
 * p[1]: device idx
 * [p[2]: flags: 0x1: ignore errors]
 */
plerrcode
proc_lvd_sync_statist_flags(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    if (p[0]>1)
        *outptr++=lvd_sync_statist_set_flags(dev, p[2]);
    else
        *outptr++=lvd_sync_statist_get_flags(dev);
    return plOK;
}

plerrcode test_proc_lvd_sync_statist_flags(ems_u32* p)
{
    plerrcode pres;

    if (p[0]<1 || p[0]>2)
        return plErr_ArgNum;
    if ((pres=find_device(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=1;
    return plOK;
}
char name_proc_lvd_sync_statist_flags[]="lvd_sync_statist_flags";
int ver_proc_lvd_sync_statist_flags=1;
#endif
/*****************************************************************************/
/*****************************************************************************/
