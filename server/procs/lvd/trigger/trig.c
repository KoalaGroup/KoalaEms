/*
 * procs/lvd/trigger/trig.c
 * created 2009-Nov-10 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: trig.c,v 1.6 2011/04/15 19:51:00 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../commu/commu.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/lvd/lvdbus.h"
#include "../../../lowlevel/lvd/trigger/trigger.h"
#include "../../../lowlevel/devices.h"
#include "../lvd_verify.h"

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

RCS_REGISTER(cvsid, "procs/lvd/trigger")

/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: branch
 * p[2]: module addr  (absolute address or -1 for all modules)
 * p[3]: daqmode      (see manual)
 */
plerrcode proc_lvd_trig_init(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);

    return lvd_trig_init(dev, p[2], p[3]);
}

plerrcode test_proc_lvd_trig_init(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_trig_init[] = "lvd_trig_init";
int ver_proc_lvd_trig_init = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: branch
 * p[2]: module addr  (absolute address or -1 for all modules)
 * [p[3]: pattern]
 */
plerrcode proc_lvd_trig_ena(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;

    if (p[0]==3) {
        pres=lvd_trig_set_ena(dev, p[2], p[3]);
    } else {
        pres=lvd_trig_get_ena(dev, p[2], outptr);
        if (pres==plOK)
            outptr++;
    }
    return pres;
}

plerrcode test_proc_lvd_trig_ena(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=2 && p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=p[0]==3?0:1;
    return plOK;
}

char name_proc_lvd_trig_ena[] = "lvd_trig_ena";
int ver_proc_lvd_trig_ena = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: module addr
 */
plerrcode proc_lvd_trig_sync(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;

    pres=lvd_trig_get_sync(dev, p[2], outptr);
    if (pres==plOK)
        outptr++;
    return pres;
}

plerrcode test_proc_lvd_trig_sync(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_trig_sync[] = "lvd_trig_sync";
int ver_proc_lvd_trig_sync = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: branch
 * p[2]: module addr
 * p[3]: port idx
 */
plerrcode proc_lvd_trig_rmid(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;

    pres=lvd_trig_rmid(dev, p[2], p[3], outptr);
    if (pres==plOK)
        outptr++;
    return pres;
}

plerrcode test_proc_lvd_trig_rmid(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=1;
    return plOK;
}

char name_proc_lvd_trig_rmid[] = "lvd_trig_rmid";
int ver_proc_lvd_trig_rmid = 1;
/*****************************************************************************/
/*
 * p[0]: argcount>=5
 * p[1]: branch
 * p[2]: module addr
 * p[3]: selector (0..1)
 * p[4]: layer    (0..6)
 * p[5]: sector   (0..23)
 * p[6..]: data
 */
plerrcode proc_lvd_trig_mem_write(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;

    pres=lvd_trig_write_mem(dev, p[2], p[3], p[4], p[5], 0, p[0]-5, p+6);
    return pres;
}

plerrcode test_proc_lvd_trig_mem_write(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;
    int num;

    if (p[0]<5)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;
    if (p[3]>1)
        return plErr_ArgRange;
    if (p[4]>6)
        return plErr_ArgRange;
    if (p[5]>31)
        return plErr_ArgRange;
    num=p[0]-5;
    if (p[3]) {
        if (num!=32) {
            complain("trig_mem_write(%d %d %d): wrong number of arguments: "
                "%d instead of 32", p[3], p[4], p[5], num);
            return plErr_ArgNum;
        }
    } else {
        if (num!=256) {
            complain("trig_mem_write(%d %d %d): wrong number of arguments: "
                "%d instead of 256", p[3], p[4], p[5], num);
            return plErr_ArgNum;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_trig_mem_write[] = "lvd_trig_mem_write";
int ver_proc_lvd_trig_mem_write = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==5
 * p[1]: branch
 * p[2]: module addr
 * p[3]: selector
 * p[4]: layer
 * p[5]: sector
 */
plerrcode proc_lvd_trig_mem_read(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;

    pres=lvd_trig_read_mem(dev, p[2], p[3], p[4], p[5], 0, p[3]?32:256, outptr);
    if (pres==plOK)
        outptr+=p[3]?32:256;
    return pres;
}

plerrcode test_proc_lvd_trig_mem_read(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=5)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;
    if (p[3]>1)
        return plErr_ArgRange;
    if (p[4]>6)
        return plErr_ArgRange;
    if (p[5]>31)
        return plErr_ArgRange;

    wirbrauchen=p[3]?32:256;
    return plOK;
}

char name_proc_lvd_trig_mem_read[] = "lvd_trig_mem_read";
int ver_proc_lvd_trig_mem_read = 1;
/*****************************************************************************/
/*
 * p[0]: argcount>=3
 * p[1]: branch
 * p[2]: module addr
 * p[3]: memaddr (0..65535)
 * p[4..]: data
 */
plerrcode proc_lvd_trig_pi_write(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;

    pres=lvd_trig_write_pi(dev, p[2], p[3], p[0]-3, p+4);
    return pres;
}

plerrcode test_proc_lvd_trig_pi_write(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;
    int num;

    if (p[0]<3)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;
    if (p[3]>65536)
        return plErr_ArgRange;
    num=p[0]-3;
    if (p[3]+num>65536) {
        complain("trig_pi_write(addr=%d num=%d): wrong number of arguments",
                p[3], num);
        return plErr_ArgNum;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_trig_pi_write[] = "lvd_trig_pi_write";
int ver_proc_lvd_trig_pi_write = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==4
 * p[1]: branch
 * p[2]: module addr
 * p[3]: memaddr (0..65535)
 * p[4]: num
 */
plerrcode proc_lvd_trig_pi_read(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;

    pres=lvd_trig_read_pi(dev, p[2], p[3], p[4], outptr);
    if (pres==plOK)
        outptr+=p[3]?32:256;
    return pres;
}

plerrcode test_proc_lvd_trig_pi_read(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;
    if (p[3]>65536)
        return plErr_ArgRange;
    if (p[3]+p[4]>65536) {
        complain("trig_pi_write(addr=%d num=%d): wrong number of arguments",
                p[3], p[4]);
        return plErr_ArgNum;
    }

    wirbrauchen=p[4];
    return plOK;
}

char name_proc_lvd_trig_pi_read[] = "lvd_trig_pi_read";
int ver_proc_lvd_trig_pi_read = 1;
/*****************************************************************************/
/*****************************************************************************/
