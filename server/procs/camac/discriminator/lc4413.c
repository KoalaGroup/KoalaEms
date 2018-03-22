/*
 * procs/camac/discriminator/lc4413.c
 * created 2010-12-17 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lc4413.c,v 1.10 2017/10/20 23:20:52 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../lowlevel/camac/camac.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../procprops.h"

extern ems_u32* outptr;
extern unsigned int *memberlist;

RCS_REGISTER(cvsid, "procs/camac/discriminator")

/*
 * LeCroy 4413
 * 16-channel discriminator
 * 
 * X and Q are set for each valid command.
 * in local and remote mode:
 * F(0)A(0)    16 bit read mask, Q in remote mode only
 * F(1)A(0)    10/11 bit read threshold
 * F(26)A(0)   set remote mode
 *
 * in remote mode only:
 * F(16)A(0)   16 bit write mask
 * F(17)A(0)   10/11 bit write threshold, bit 10==1: screwdriver mode
 * F(24)A(0)   local mode
 * F(25)A(0)   generates test (for all channels which are not masked)
 */

/*****************************************************************************/
static int
check_module_type(int n)
{
    ml_entry* module;

    if (n<0) {
        if (!modullist) {
            printf("lc4413.c: no modullist\n");
            return -1;
        }
        /* the real code will iterate over all suitable members of
           memberlist or modullist, thus we do not need further checks */
    } else {
        if (!valid_module(n, modul_camac)) {
            printf("lc4413.c: !valid_module\n");
            return -1;
        }
        module=ModulEnt(n);
        if (module->modultype!=LC_DISC_4413) {
            printf("lc4413.c: modultype=%08x", module->modultype);
            return -1;
        }
    }
    return 0;
}
/*****************************************************************************/
static plerrcode
set_reg(ml_entry* module, int F, ems_u32 value)
{
    struct camac_dev* dev=module->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(module), 0, F);
    ems_u32 stat;

    if (dev->CAMACwrite(dev, &addr, value)<0) {
        printf("lc4413_set_reg(%d, %d, %d): errno=%s\n",
                module->address.camac.slot, 0, F, strerror(errno));
        return plErr_System;
    }
    if (dev->CAMACstatus(dev, &stat)<0) {
        printf("lc4413_set_reg(%d, %d, %d): errno=%s\n",
                module->address.camac.slot, 0, F, strerror(errno));
        return plErr_System;
    }
    if (dev->CAMACgotQX(stat)!=0xC0000000) {
        printf("lc4413_set_reg(%d, %d, %d): QX=%08x\n",
                module->address.camac.slot, 0, F, stat);
        return plErr_HW;
    }

    return plOK;
}
/*****************************************************************************/
static plerrcode
cntl(ml_entry* module, int F, ems_u32 expected)
{
    struct camac_dev* dev=module->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(module), 0, F);
    ems_u32 stat;

    if (dev->CAMACcntl(dev, &addr, &stat)<0) {
        printf("lc4413_cntl(%d, %d, %d): errno=%s\n",
                module->address.camac.slot, 0, F, strerror(errno));
        return plErr_System;
    }
    if ((dev->CAMACgotQX(stat)&0xC0000000)!=expected) {
        printf("lc4413_cntl(%d, %d, %d): QX=%08x\n",
                module->address.camac.slot, 0, F, stat);
        return plErr_HW;
    }

    return plOK;
}
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: module (-1 for all modules of memberlist or modullist)
 * p[2]: value, range 0..1023 or 0x400
 *       0: -15 mV, 1023: -1 V
 *       0x8??: screwdriver mode
 */
plerrcode proc_lc4413_set_threshold(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        unsigned int i;
        if (memberlist) { /* iterate over memberlist */
            for (i=1; i<=memberlist[0]; i++) {
                module=&modullist->entry[memberlist[i]];
                if (module->modulclass==modul_camac &&
                        module->modultype==LC_DISC_4413) {
                    set_reg(module, /*F*/17, p[2]);
                    if (pres!=plOK)
                        return pres;
                }
            }
        } else {          /* iterate over modullist */
            for (i=0; i<modullist->modnum; i++) {
                module=&modullist->entry[i];
                if (module->modulclass==modul_camac &&
                        module->modultype==LC_DISC_4413) {
                    set_reg(module, /*F*/17, p[2]);
                    if (pres!=plOK)
                        return pres;
                }
            }
        }
    } else {
        module=ModulEnt(p[1]);
        set_reg(module, /*F*/17, p[2]);
    }

    return pres;
}
plerrcode test_proc_lc4413_set_threshold(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    if (p[0]!=2)
        return plErr_ArgNum;
    if (check_module_type(ip[1])<0)
        return plErr_BadModTyp;
    if (p[2]>1024)
        return plErr_ArgRange;

    wirbrauchen=0;
    return plOK;
}
char name_proc_lc4413_set_threshold[]="lc4413_set_threshold";
int ver_proc_lc4413_set_threshold=1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: module
 */
plerrcode proc_lc4413_get_threshold(ems_u32* p)
{
    ems_u32 val[16];
    ml_entry* m=ModulEnt(p[1]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), /*A*/0, /*F*/0);
    if (dev->CAMACblreadAddrScan(dev, &addr, 16, val)<0) {
        return plErr_System;
    } else {
        int i;
        for (i=0; i<16; i++) {
            if ((val[i]&0xC0000000)!=0xC0000000) {
                printf("lc4413_get_threshold: QX=%08x\n", val[i]);
                return plErr_HW;
            }
            *outptr++=val[i]&~0xC0000000;
        }
    }
    return plOK;
}
plerrcode test_proc_lc4413_get_threshold(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    if (p[0]!=1)
        return plErr_ArgNum;
    if (ip[1]<0 || check_module_type(ip[1])<0)
        return plErr_BadModTyp;

    wirbrauchen=16;
    return plOK;
}
char name_proc_lc4413_get_threshold[]="lc4413_get_threshold";
int ver_proc_lc4413_get_threshold=1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: module (-1 for all modules of memberlist or modullist)
 * p[2]: mask
 */
plerrcode proc_lc4413_set_mask(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        unsigned int i;
        if (memberlist) { /* iterate over memberlist */
            for (i=1; i<=memberlist[0]; i++) {
                module=&modullist->entry[memberlist[i]];
                if (module->modulclass==modul_camac &&
                        module->modultype==LC_DISC_4413) {
                    pres=set_reg(module, 16, p[2]);
                    if (pres!=plOK)
                        return pres;
                }
            }
        } else {          /* iterate over modullist */
            for (i=0; i<modullist->modnum; i++) {
                module=&modullist->entry[i];
                if (module->modulclass==modul_camac &&
                        module->modultype==LC_DISC_4413) {
                    pres=set_reg(module, 16, p[2]);
                    if (pres!=plOK)
                        return pres;
                }
            }
        }
    } else {
        module=ModulEnt(p[1]);
        pres=set_reg(module, 16, p[2]);
    }

    return pres;
}
plerrcode test_proc_lc4413_set_mask(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    if (p[0]!=2)
        return plErr_ArgNum;
    if (check_module_type(ip[1])<0)
        return plErr_BadModTyp;
    if (p[2]>0xffff)
        return plErr_ArgRange;

    wirbrauchen=0;
    return plOK;
}
char name_proc_lc4413_set_mask[]="lc4413_set_mask";
int ver_proc_lc4413_set_mask=1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: module
 */
plerrcode proc_lc4413_get_mask(ems_u32* p)
{
    ems_u32 val;
    ml_entry* m=ModulEnt(p[1]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), /*A*/0, /*F*/1);
    if (dev->CAMACread(dev, &addr, &val)<0) {
        return plErr_System;
    } else {
        if ((val&0xC0000000)!=0xC0000000) {
            printf("lc4413_get_mask: QX=%08x\n", val);
            return plErr_HW;
        }
        *outptr++=val&~0xC0000000;
    }
    return plOK;
}
plerrcode test_proc_lc4413_get_mask(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    if (p[0]!=1)
        return plErr_ArgNum;
    if (ip[1]<0 || check_module_type(ip[1])<0)
        return plErr_BadModTyp;

    wirbrauchen=1;
    return plOK;
}
char name_proc_lc4413_get_mask[]="lc4413_get_mask";
int ver_proc_lc4413_get_mask=1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: module (-1 for all modules of memberlist or modullist)
 */
plerrcode proc_lc4413_test(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        unsigned int i;
        if (memberlist) { /* iterate over memberlist */
            for (i=1; i<=memberlist[0]; i++) {
                module=&modullist->entry[memberlist[i]];
                if (module->modulclass==modul_camac &&
                        module->modultype==LC_DISC_4413) {
                    pres=cntl(module, 25, 0xc0000000);
                    if (pres!=plOK)
                        return pres;
                }
            }
        } else {          /* iterate over modullist */
            for (i=0; i<modullist->modnum; i++) {
                module=&modullist->entry[i];
                if (module->modulclass==modul_camac &&
                        module->modultype==LC_DISC_4413) {
                    pres=cntl(module, 25, 0xc0000000);
                    if (pres!=plOK)
                        return pres;
                }
            }
        }
    } else {
        module=ModulEnt(p[1]);
        pres=cntl(module, 25, 0xc0000000);
    }

    return pres;
}
plerrcode test_proc_lc4413_test(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    if (p[0]!=1)
        return plErr_ArgNum;
    if (check_module_type(ip[1])<0)
        return plErr_BadModTyp;

    wirbrauchen=0;
    return plOK;
}
char name_proc_lc4413_test[]="lc4413_test";
int ver_proc_lc4413_test=1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: module (-1 for all modules of memberlist or modullist)
 */
plerrcode proc_lc4413_remote(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        unsigned int i;
        if (memberlist) { /* iterate over memberlist */
            for (i=1; i<=memberlist[0]; i++) {
                module=&modullist->entry[memberlist[i]];
                if (module->modulclass==modul_camac &&
                        module->modultype==LC_DISC_4413) {
                    pres=cntl(module, 26, 0x40000000);
                    if (pres!=plOK)
                        return pres;
                }
            }
        } else {          /* iterate over modullist */
            for (i=0; i<modullist->modnum; i++) {
                module=&modullist->entry[i];
                if (module->modulclass==modul_camac &&
                        module->modultype==LC_DISC_4413) {
                    pres=cntl(module, 26, 0x40000000);
                    if (pres!=plOK)
                        return pres;
                }
            }
        }
    } else {
        module=ModulEnt(p[1]);
        pres=cntl(module, 26, 0x40000000);
    }

    return pres;
}
plerrcode test_proc_lc4413_remote(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    if (p[0]!=1)
        return plErr_ArgNum;
    if (check_module_type(ip[1])<0)
        return plErr_BadModTyp;

    wirbrauchen=0;
    return plOK;
}
char name_proc_lc4413_remote[]="lc4413_remote";
int ver_proc_lc4413_remote=1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: module (-1 for all modules of memberlist or modullist)
 */
plerrcode proc_lc4413_local(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        unsigned int i;
        if (memberlist) { /* iterate over memberlist */
            for (i=1; i<=memberlist[0]; i++) {
                module=&modullist->entry[memberlist[i]];
                if (module->modulclass==modul_camac &&
                        module->modultype==LC_DISC_4413) {
                    pres=cntl(module, 24, 0x40000000);
                    if (pres!=plOK)
                        return pres;
                }
            }
        } else {          /* iterate over modullist */
            for (i=0; i<modullist->modnum; i++) {
                module=&modullist->entry[i];
                if (module->modulclass==modul_camac &&
                        module->modultype==LC_DISC_4413) {
                    pres=cntl(module, 24, 0x40000000);
                    if (pres!=plOK)
                        return pres;
                }
            }
        }
    } else {
        module=ModulEnt(p[1]);
        pres=cntl(module, 24, 0x40000000);
    }

    return pres;
}
plerrcode test_proc_lc4413_local(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    if (p[0]!=1)
        return plErr_ArgNum;
    if (check_module_type(ip[1])<0)
        return plErr_BadModTyp;

    wirbrauchen=0;
    return plOK;
}
char name_proc_lc4413_local[]="lc4413_local";
int ver_proc_lc4413_local=1;
/*****************************************************************************/
/*****************************************************************************/
