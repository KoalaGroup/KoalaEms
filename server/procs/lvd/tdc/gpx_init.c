/*
 * procs/lvd/tdc/gpx_init.c
 * created 2006-05-04 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: gpx_init.c,v 1.3 2011/04/06 20:30:33 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/lvd/lvdbus.h"
#include "../../../lowlevel/lvd/tdc/gpx.h"
#include "../../../lowlevel/devices.h"
#include "../lvd_verify.h"

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

#define get_device(branch) \
    (struct lvd_dev*)get_gendevice(modul_lvd, (branch))

RCS_REGISTER(cvsid, "procs/lvd/tdc")

/*****************************************************************************/
/*
 * p[0]: argcount==4
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * p[3]: gpx
 * p[4]: reg
 * [p[5]: val]
 *
 * p[2..4]<0: all
 */
plerrcode proc_gpx_reg(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;
    ems_i32* ip=(ems_i32*)p;

    if (p[0]>4) {
        res=gpx_write_reg(dev, ip[2], ip[3], p[4], p[5]);
    } else {
        res=gpx_read_reg(dev, ip[2], ip[3], p[4], outptr);
        if (res==plOK)
            outptr++;
    }
    return res?plErr_Other:plOK;
}

plerrcode test_proc_gpx_reg(ems_u32* p)
{
    struct lvd_dev* dev;
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if ((p[0]!=4)&&(p[0]!=5))
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    if ((ip[2]>=0)||(p[0]<5)) {
        pres=verify_lvd_id(dev, p[2], ZEL_LVD_TDC_GPX);
        if (pres!=plOK)
            return pres;
    }

    wirbrauchen=p[0]>4?0:1;
    return plOK;
}

char name_proc_gpx_reg[] = "gpx_reg";
int ver_proc_gpx_reg = 1;
/*****************************************************************************/
#if 0
/*
 * p[0]: argcount==3
 * p[1]: branch
 * p[2]: card
 * p[3]: gpx (0..7)
 */
plerrcode proc_gpx_state(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;

    res=gpx_state(dev, ((ems_i32*)p)[2], ((ems_i32*)p)[3]);
    return res?plErr_Other:plOK;
}

plerrcode test_proc_gpx_state(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    if ((ems_i32)p[2]>=0) {
        pres=verify_lvd_id(dev, p[2], ZEL_LVD_TDC_gpx);
        if (pres!=plOK)
            return pres;
    }
    if ((ems_i32)p[3]>7)
        return plErr_ArgRange;
    wirbrauchen=2;
    return plOK;
}

char name_proc_gpx_state[] = "gpx_state";
int ver_proc_gpx_state = 1;
#endif
/*****************************************************************************/
#if 0
/*
 * p[0]: argcount==4
 * p[1]: branch
 * p[2]: card
 * p[3]: gpx (0..7)
 * p[4]: val
 */
plerrcode proc_gpx_clear(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;

    res=gpx_clear(dev, ((ems_i32*)p)[2], ((ems_i32*)p)[3], p[4]);
    return res?plErr_Other:plOK;
}

plerrcode test_proc_gpx_clear(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    if ((ems_i32)p[2]>=0) {
        pres=verify_lvd_id(dev, p[2], ZEL_LVD_TDC_GPX);
        if (pres!=plOK)
            return pres;
    }
    if ((ems_i32)p[3]>7)
        return plErr_ArgRange;
    wirbrauchen=2;
    return plOK;
}

char name_proc_gpx_clear[] = "gpx_clear";
int ver_proc_gpx_clear = 1;
#endif
/*****************************************************************************/
/*
 * p[0]: argcount==5
 * p[1]: branch
 * p[2]: card
 * p[3]: dcm
 * p[4]: abs
 * p[5]: val
 */
plerrcode proc_gpx_dcm_shift(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    return gpx_dcm_shift(dev, ((ems_i32*)p)[2], p[3], p[4], p[5]);
}

plerrcode test_proc_gpx_dcm_shift(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=5)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    if ((ems_i32)p[2]>=0) {
        pres=verify_lvd_id(dev, p[2], ZEL_LVD_TDC_GPX);
        if (pres!=plOK)
            return pres;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_gpx_dcm_shift[] = "gpx_dcm_shift";
int ver_proc_gpx_dcm_shift = 1;
/*****************************************************************************/
/*****************************************************************************/
