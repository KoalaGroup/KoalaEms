/*
 * procs/lvd/tdc/f1_init.c
 * created 10.Dec.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: f1_init.c,v 1.4 2011/04/06 20:30:33 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/lvd/lvdbus.h"
#include "../../../lowlevel/lvd/tdc/f1.h"
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
 * p[0]: argcount==6
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * p[3]: f1
 * p[4]: reg
 * p[5]: val
 * p[6]: mask
 *
 * p[2..4]<0: all
 */
plerrcode proc_f1_regbits(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;
    ems_i32* ip=(ems_i32*)p;

    res=f1_regbits(dev, ip[2], ip[3], p[4], p[5], p[6]);
    return res?plErr_Other:plOK;
}

plerrcode test_proc_f1_regbits(ems_u32* p)
{
    struct lvd_dev* dev;
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if (p[0]!=6)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    if (ip[2]>=0) {
        pres=verify_lvd_id(dev, p[2], ZEL_LVD_TDC_F1);
        if (pres!=plOK)
            return pres;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_f1_regbits[] = "f1_regbits";
int ver_proc_f1_regbits = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==5
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * p[3]: f1
 * p[4]: reg
 * p[5]: val
 *
 * p[2..4]<0: all
 */
plerrcode proc_f1_reg(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;
    ems_i32* ip=(ems_i32*)p;

    res=f1_reg(dev, ip[2], ip[3], p[4], p[5]);
    return res?plErr_Other:plOK;
}

plerrcode test_proc_f1_reg(ems_u32* p)
{
    struct lvd_dev* dev;
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if (p[0]!=5)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    if (ip[2]>=0) {
        pres=verify_lvd_id(dev, p[2], ZEL_LVD_TDC_F1);
        if (pres!=plOK)
            return pres;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_f1_reg[] = "f1_reg";
int ver_proc_f1_reg = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==4
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * p[3]: f1          (or -1 for all f1)
 * p[4]: reg         (or -1 for all registers)
 */
plerrcode proc_f1_shadow(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;
    ems_i32* ip=(ems_i32*)p;

    res=f1_get_shadow(dev, ip[2], ip[3], p[4], &outptr);
    return res?plErr_Other:plOK;
}

plerrcode test_proc_f1_shadow(ems_u32* p)
{
    struct lvd_dev* dev;
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if (p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    if (ip[2]>=0) {
        pres=verify_lvd_id(dev, p[2], ZEL_LVD_TDC_F1);
        if (pres!=plOK)
            return pres;
    }

    wirbrauchen=1;
    if (p[2]<0) wirbrauchen*=0x110;
    if (p[3]<0) wirbrauchen*=8;
    if (p[4]<0) wirbrauchen*=16;
    return plOK;
}

char name_proc_f1_shadow[] = "f1_shadow";
int ver_proc_f1_shadow = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: branch
 * p[2]: card
 * p[3]: f1 (0..7)
 */
plerrcode proc_f1_state(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;

    res=f1_state(dev, ((ems_i32*)p)[2], ((ems_i32*)p)[3]);
    return res?plErr_Other:plOK;
}

plerrcode test_proc_f1_state(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    if ((ems_i32)p[2]>=0) {
        pres=verify_lvd_id(dev, p[2], ZEL_LVD_TDC_F1);
        if (pres!=plOK)
            return pres;
    }
    if ((ems_i32)p[3]>7)
        return plErr_ArgRange;
    wirbrauchen=2;
    return plOK;
}

char name_proc_f1_state[] = "f1_state";
int ver_proc_f1_state = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==4
 * p[1]: branch
 * p[2]: card
 * p[3]: f1 (0..7)
 * p[4]: val
 */
plerrcode proc_f1_clear(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;

    res=f1_clear(dev, ((ems_i32*)p)[2], ((ems_i32*)p)[3], p[4]);
    return res?plErr_Other:plOK;
}

plerrcode test_proc_f1_clear(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    if ((ems_i32)p[2]>=0) {
        pres=verify_lvd_id(dev, p[2], ZEL_LVD_TDC_F1);
        if (pres!=plOK)
            return pres;
    }
    if ((ems_i32)p[3]>7)
        return plErr_ArgRange;
    wirbrauchen=2;
    return plOK;
}

char name_proc_f1_clear[] = "f1_clear";
int ver_proc_f1_clear = 1;
/*****************************************************************************/
