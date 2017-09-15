/*
 * procs/lvd/sis1100/lvd_sis1100.c
 * created 2005-Aug-06 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_sis1100.c,v 1.9 2013/01/17 22:40:13 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <xprintf.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/lvd/sis1100/sis1100_lvd.h"
#include "../../../lowlevel/devices.h"

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

#define get_device(branch) \
    (struct lvd_dev*)get_gendevice(modul_lvd, (branch))

RCS_REGISTER(cvsid, "procs/lvd/sis1100")

/*****************************************************************************/
/*
 * p[0]: argcount==2|3
 * p[1]: branch
 * p[2]: link
 * p[3]: reg
 * [p[4]: val]
 */
plerrcode proc_lvd_plxreg(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    plerrcode pres;

    if (p[0]==3) {
        pres=info->plxread(dev, p[2], p[3], outptr);
        if (pres==plOK)
            outptr++;
    } else {
        pres=info->plxwrite(dev, p[2], p[3], p[4]);
    }
    return pres;
}

plerrcode test_proc_lvd_plxreg(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if ((p[0]!=3) && (p[0]!=4))
        return plErr_ArgNum;
    if ((pres=find_device(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;
    if (dev->lvdtype!=lvd_sis1100)
        return plErr_BadModTyp;

    wirbrauchen=p[0]==3?1:0;
    return plOK;
}

char name_proc_lvd_plxreg[] = "lvd_plxreg";
int ver_proc_lvd_plxreg = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2|3
 * p[1]: branch
 * p[2]: link
 * p[3]: reg
 * [p[4]: val]
 */
plerrcode proc_lvd_sisreg(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    plerrcode pres;

    if (dev->lvdtype!=lvd_sis1100) {
        return plErr_BadModTyp;
    }

    if (p[0]==3) {
        pres=info->sisread(dev, p[2], p[3], outptr);
        if (pres==plOK)
            outptr++;
    } else {
        pres=info->siswrite(dev, p[2], p[3], p[4]);
    }
    return pres;
}

plerrcode test_proc_lvd_sisreg(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if ((p[0]!=3) && (p[0]!=4))
        return plErr_ArgNum;
    if ((pres=find_device(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;
    if (dev->lvdtype!=lvd_sis1100)
        return plErr_BadModTyp;

    wirbrauchen=p[0]==3?1:0;
    return plOK;
}

char name_proc_lvd_sisreg[] = "lvd_sisreg";
int ver_proc_lvd_sisreg = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2|3
 * p[1]: branch
 * p[2]: link
 * p[3]: reg
 * [p[4]: val]
 */
plerrcode proc_lvd_mcreg(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    plerrcode pres;

    if (dev->lvdtype!=lvd_sis1100) {
        return plErr_BadModTyp;
    }

    if (p[0]==3) {
        pres=info->mcread(dev, p[2], p[3], outptr);
        if (pres==plOK) outptr++;
    } else {
        pres=info->mcwrite(dev, p[2], p[3], p[4]);
    }
    return pres;
}

plerrcode test_proc_lvd_mcreg(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if ((p[0]!=3) && (p[0]!=4))
        return plErr_ArgNum;
    if ((pres=find_device(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;
    if (dev->lvdtype!=lvd_sis1100)
        return plErr_BadModTyp;

    wirbrauchen=p[0]==3?1:0;
    return plOK;
}

char name_proc_lvd_mcreg[] = "lvd_mcreg";
int ver_proc_lvd_mcreg = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==5
 * p[1]: branch
 * p[2]: link
 * p[3]: level
 *       level&0x30 ==   0: print to stdout
 *       level&0x30 ==0x10: output string to outptr
 *       level&0x30 ==0x30: do both of above
 */
plerrcode proc_lvd_sisstat(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    if (dev->lvdtype==lvd_sis1100) {
        struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
        plerrcode pres;
        void *xp=0;

        if (p[3]&0x10)
            xprintf_init(&xp);

        pres=info->sis_stat(dev, p[2], xp, p[3]);

        if (xp) {
            const char *p=xprintf_get(xp);
            if (p[3]&0x20)
                printf("%s", p);
            if (p[3]&0x10) {
                outptr=outstring(outptr, p);
            }
            xprintf_done(&xp);
        }

        return pres;
    } else {
        return plErr_BadModTyp;
    }
}

plerrcode test_proc_lvd_sisstat(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_device(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;
    if (dev->lvdtype!=lvd_sis1100)
        return plErr_BadModTyp;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_sisstat[] = "lvd_sisstat";
int ver_proc_lvd_sisstat = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==5
 * p[1]: branch
 * p[2]: link
 * p[3]: level
 *       level&0x30 ==   0: print to stdout
 *       level&0x30 ==0x10: output string to outptr
 *       level&0x30 ==0x30: do both of above
 */
plerrcode proc_lvd_mcstat(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    if (dev->lvdtype==lvd_sis1100) {
        struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
        void *xp=0;
        plerrcode pres;

        if (p[3]&0x10)
            xprintf_init(&xp);

        pres=info->mc_stat(dev, p[2], xp, p[3]);

        if (xp) {
            const char *p=xprintf_get(xp);
            if (p[3]&0x20)
                printf("%s", p);
            if (p[3]&0x10) {
                outptr=outstring(outptr, p);
            }
            xprintf_done(&xp);
        }

        return pres;
    } else {
        return plErr_BadModTyp;
    }
}

plerrcode test_proc_lvd_mcstat(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_device(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;
    if (dev->lvdtype!=lvd_sis1100)
        return plErr_BadModTyp;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_mcstat[] = "lvd_mcstat";
int ver_proc_lvd_mcstat = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==5
 * p[1]: branch
 * [p[2]: level]
 *       level&0x30 ==   0: print to stdout
 *       level&0x30 ==0x10: output string to outptr
 *       level&0x30 ==0x30: do both of above
 */
plerrcode proc_lvd_linkstat(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    if (dev->lvdtype==lvd_sis1100) {
        struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
        void *xp=0;
        plerrcode pres;

        if (p[3]&0x10)
            xprintf_init(&xp);

        pres=info->link_stat(dev, xp);

        if (xp) {
            const char *p=xprintf_get(xp);
            if (p[3]&0x20)
                printf("%s", p);
            if (p[3]&0x10) {
                outptr=outstring(outptr, p);
            }
            xprintf_done(&xp);
        }

        return pres;
    } else {
        return plErr_BadModTyp;
    }
}

plerrcode test_proc_lvd_linkstat(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_device(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;
    if (dev->lvdtype!=lvd_sis1100)
        return plErr_BadModTyp;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_linkstat[] = "lvd_linkstat";
int ver_proc_lvd_linkstat = 1;
/*****************************************************************************/
#if 0
/*
 * p[0]: argcount==5
 * p[1]: branch
 * p[2]: level
 */
plerrcode proc_lvd_ddmastat(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    if (dev->lvdtype==lvd_sis1100) {
        struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
        return info->ddma_stat(dev, p[2]);
    } else {
        return plErr_BadModTyp;
    }
}

plerrcode test_proc_lvd_ddmastat(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_ddmastat[] = "lvd_ddmastat";
int ver_proc_lvd_ddmastat = 1;
#endif
/*****************************************************************************/
#if 0
/*
 * p[0]: argcount==5
 * p[1]: branch
 */
plerrcode proc_lvd_ddmaflush(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    if (dev->lvdtype==lvd_sis1100) {
        struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
        return info->ddma_flush(dev);
    } else {
        return plErr_BadModTyp;
    }
}

plerrcode test_proc_lvd_ddmaflush(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_ddmaflush[] = "lvd_ddmaflush";
int ver_proc_lvd_ddmaflush = 1;
#endif
/*****************************************************************************/
/*****************************************************************************/
