/*
 * procs/lvd/sync/sync_init.c
 * created 2007-01-17 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sync_init.c,v 1.8 2011/04/06 20:30:33 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/lvd/lvdbus.h"
#include "../../../lowlevel/lvd/sync/sync.h"
#include "../../../lowlevel/devices.h"
#include "../lvd_verify.h"

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

#define get_device(branch) \
    (struct lvd_dev*)get_gendevice(modul_lvd, (branch))

RCS_REGISTER(cvsid, "procs/lvd/sync")

/*****************************************************************************/
/*
 * p[0]: argcount==4..5
 * p[1]: branch
 * p[2]: module addr  (absolute address or -1 for all modules)
 * p[3]: daqmode      (see manual)
 * p[4]: trigger mask
 * [p[5]: sum mask]
 */
plerrcode proc_msync_init(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    return lvd_syncmaster_init(dev, p[2], p[3], p[4], p[0]>4?p[5]:0);
}

plerrcode test_proc_msync_init(ems_u32* p)
{
    struct lvd_dev* dev;
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if (p[0]<4 || p[0]>5)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    if (ip[2]>=0) {
        pres=verify_lvd_id(dev, p[2], ZEL_LVD_MSYNCH);
        if (pres!=plOK)
            return pres;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_msync_init[] = "msync_init";
int ver_proc_msync_init = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==5
 * p[1]: branch
 * p[2]: module addr
 * p[3]: selection (bit 0..15: trigger 0..15, bit 16: sum trigger, bit 17: TAW)
 * p[4]: value
 * p[5]: save 
 */
plerrcode proc_msync_width(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    return lvd_syncmaster_width(dev, p[2], p[3], p[4], p[5]);
}
plerrcode test_proc_msync_width(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=5)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    pres=verify_lvd_id(dev, p[2], ZEL_LVD_MSYNCH);
    if (pres!=plOK)
        return pres;

    if (p[3]&~0x3ff)
        return plErr_ArgRange;
    if (p[4]>31)
        return plErr_ArgRange;

    wirbrauchen=0;
    return plOK;
}
char name_proc_msync_width[] = "msync_width";
int ver_proc_msync_width = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: module addr
 */
plerrcode proc_msync_pause(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    return lvd_syncmaster_pause(dev, p[2]);
}
plerrcode test_proc_msync_pause(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    pres=verify_lvd_id(dev, p[2], ZEL_LVD_MSYNCH);
    if (pres!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}
char name_proc_msync_pause[] = "msync_pause";
int ver_proc_msync_pause = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: module addr
 */
plerrcode proc_msync_resume(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    return lvd_syncmaster_resume(dev, p[2]);
}
plerrcode test_proc_msync_resume(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    pres=verify_lvd_id(dev, p[2], ZEL_LVD_MSYNCH);
    if (pres!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}
char name_proc_msync_resume[] = "msync_resume";
int ver_proc_msync_resume = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: module addr
 */
plerrcode proc_msync_start(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    return lvd_syncmaster_start(dev, p[2]);
}
plerrcode test_proc_msync_start(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    pres=verify_lvd_id(dev, p[2], ZEL_LVD_MSYNCH);
    if (pres!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}
char name_proc_msync_start[] = "msync_start";
int ver_proc_msync_start = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: module addr
 */
plerrcode proc_msync_stop(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    return lvd_syncmaster_stop(dev, p[2]);
}
plerrcode test_proc_msync_stop(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    pres=verify_lvd_id(dev, p[2], ZEL_LVD_MSYNCH);
    if (pres!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}
char name_proc_msync_stop[] = "msync_stop";
int ver_proc_msync_stop = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: module addr
 */
plerrcode proc_msync_evc(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    plerrcode pres;

    pres=lvd_syncmaster_evc(dev, p[2], outptr);
    if (pres==plOK)
        outptr++;
    return pres;
}
plerrcode test_proc_msync_evc(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    pres=verify_lvd_id(dev, p[2], ZEL_LVD_MSYNCH);
    if (pres!=plOK)
        return pres;

    wirbrauchen=1;
    return plOK;
}
char name_proc_msync_evc[] = "msync_evc";
int ver_proc_msync_evc = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: module addr
 */
plerrcode proc_osync_evc(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    plerrcode pres;

    pres=lvd_syncoutput_evc(dev, p[2], outptr);
    if (pres==plOK)
        outptr++;
    return pres;
}
plerrcode test_proc_osync_evc(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    pres=verify_lvd_id(dev, p[2], ZEL_LVD_OSYNCH);
    if (pres!=plOK)
        return pres;

    wirbrauchen=1;
    return plOK;
}
char name_proc_osync_evc[] = "osync_evc";
int ver_proc_osync_evc = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==6
 * p[1]: branch
 * p[2]: module addr  (absolute address or -1 for all modules)
 * p[3]: daqmode      (see manual)
 * [p[4]: tpat; ports which should use trigger pattern]
 */
plerrcode proc_osync_init(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    return lvd_syncoutput_init(dev, p[2], p[3], p[0]>3?p[4]:0);
}

plerrcode test_proc_osync_init(ems_u32* p)
{
    struct lvd_dev* dev;
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if (p[0]<3 || p[0]>4)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    if (ip[2]>=0) {
        pres=verify_lvd_id(dev, p[2], ZEL_LVD_OSYNCH);
        if (pres!=plOK)
            return pres;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_osync_init[] = "osync_init";
int ver_proc_osync_init = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==6
 * p[1]: branch
 * p[2]: module addr
 * p[3]: value
 * [p[4]: start (default=0)
 * [[p[5]: number of bytes (default=-1 (==all))]]
 */
plerrcode proc_osync_sram_fill(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int start=0, num=-1;

    if (p[0]>3)
        start=p[4];
    if (p[0]>4)
        num=p[5];
    return lvd_syncoutput_sram_fill(dev, p[2], start, num, p[3]);
}

plerrcode test_proc_osync_sram_fill(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]<3 || p[0]>5)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    pres=verify_lvd_id(dev, p[2], ZEL_LVD_OSYNCH);
    if (pres!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_osync_sram_fill[] = "osync_sram_fill";
int ver_proc_osync_sram_fill = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==6
 * p[1]: branch
 * p[2]: module addr
 * p[3]: addr or -1 for all
 * p[4]: bitmask
 */
plerrcode proc_osync_sram_setbits(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    ems_i32* ip=(ems_i32*)p;

    return lvd_syncoutput_sram_setbits(dev, p[2], ip[3], p[4]);
}
plerrcode test_proc_osync_sram_setbits(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    pres=verify_lvd_id(dev, p[2], ZEL_LVD_OSYNCH);
    if (pres!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}
char name_proc_osync_sram_setbits[] = "osync_sram_setbits";
int ver_proc_osync_sram_setbits = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==6
 * p[1]: branch
 * p[2]: module addr
 * p[3]: addr or -1 for all
 * p[4]: bitmask
 */
plerrcode proc_osync_sram_resbits(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    ems_i32* ip=(ems_i32*)p;

    return lvd_syncoutput_sram_resbits(dev, p[2], ip[3], p[4]);
}
plerrcode test_proc_osync_sram_resbits(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    pres=verify_lvd_id(dev, p[2], ZEL_LVD_OSYNCH);
    if (pres!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}
char name_proc_osync_sram_resbits[] = "osync_sram_resbits";
int ver_proc_osync_sram_resbits = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==6
 * p[1]: branch
 * p[2]: module addr
 * p[3..10]: bitmasks
 */
plerrcode proc_osync_sram_bitmasks(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    return lvd_syncoutput_sram_bitmasks(dev, p[2], p+3);
}
plerrcode test_proc_osync_sram_bitmasks(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=10)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    pres=verify_lvd_id(dev, p[2], ZEL_LVD_OSYNCH);
    if (pres!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}
char name_proc_osync_sram_bitmasks[] = "osync_sram_bitmasks";
int ver_proc_osync_sram_bitmasks = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==6
 * p[1]: branch
 * p[2]: module addr
 * p[3]: addr
 * p[4]: value
 */
plerrcode proc_osync_sram_set(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    return lvd_syncoutput_sram_set(dev, p[2], p[3], p[4]);
}
plerrcode test_proc_osync_sram_set(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    pres=verify_lvd_id(dev, p[2], ZEL_LVD_OSYNCH);
    if (pres!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}
char name_proc_osync_sram_set[] = "osync_sram_set";
int ver_proc_osync_sram_set = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==6
 * p[1]: branch
 * p[2]: module addr
 * p[3]: addr
 */
plerrcode proc_osync_sram_get(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    plerrcode pres;

    pres=lvd_syncoutput_sram_get(dev, p[2], p[3], outptr);
    if (pres==plOK)
        outptr++;
    return pres;
}
plerrcode test_proc_osync_sram_get(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    pres=verify_lvd_id(dev, p[2], ZEL_LVD_OSYNCH);
    if (pres!=plOK)
        return pres;

    wirbrauchen=1;
    return plOK;
}
char name_proc_osync_sram_get[] = "osync_sram_get";
int ver_proc_osync_sram_get = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==6
 * p[1]: branch
 * p[2]: module addr
 */
plerrcode proc_osync_sram_check(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    return lvd_syncoutput_sram_check(dev, p[2]);
}
plerrcode test_proc_osync_sram_check(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    pres=verify_lvd_id(dev, p[2], ZEL_LVD_OSYNCH);
    if (pres!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}
char name_proc_osync_sram_check[] = "osync_sram_check";
int ver_proc_osync_sram_check = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==6
 * p[1]: branch
 * p[2]: module addr
 * [p[3]: start addr
 * [[p[4]: number of bytes]]
 */
plerrcode proc_osync_sram_dump(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int start=0, num=16;

    if (p[0]>2)
        start=p[3];
    if (p[0]>3)
        num=p[4];
    return lvd_syncoutput_sram_dump(dev, p[2], start, num);
}

plerrcode test_proc_osync_sram_dump(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]<2 || p[0]>4)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    pres=verify_lvd_id(dev, p[2], ZEL_LVD_OSYNCH);
    if (pres!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_osync_sram_dump[] = "osync_sram_dump";
int ver_proc_osync_sram_dump = 1;
/*****************************************************************************/
/*****************************************************************************/
