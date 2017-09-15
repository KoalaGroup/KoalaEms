/*
 * procs/camac/discriminator/lc3420.c
 * created 2010-07-28 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lc3420.c,v 1.4 2011/04/06 20:30:30 wuestner Exp $";

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
extern int wirbrauchen;
extern int *memberlist;

RCS_REGISTER(cvsid, "procs/camac/discriminator")

/*
 * LeCroy 3400
 * 16-channel constant fraction discriminator
 * 
 * X and Q are set for each valid command.
 * F(0)A(0-15)  8 bit read threshold
 * F(1)A(0)    16 bit read mask
 * F(1)A(1)     8 bit read dead time and width
 * F(1)A(2)     8 bit read time to charge scale
 * F(16)A(0-15) 8 bit write threshold
 * F(17)A(0)   16 bit write mask
 * F(17)A(1)    8 bit write dead time and width
 * F(17)A(2)    8 bit write time to charge scale
 * F(25)A(0)      n/a       generate test pulse
 * 
 * ! two different versions in the manual:
 * threshold=-20 mV-val*5.2 mV (maximum: -1.33 mV +- 5%)
 * or
 * threshold=-20 mV-val*5.5 mV (maximum: -1.4 mV)
 * 
 * deadtime=25 ns+(val&0xf)*16 ns
 * width   =25 ns+((val>>4)&0xf)*16 ns
 * deadtime>=width!
 */

/*****************************************************************************/
static int
check_module_type(int n)
{
    ml_entry* module;

    if (n<0) {
        if (!modullist) {
            printf("lc3420.c: no modullist\n");
            return -1;
        }
        /* the real code will iterate over all suitable members of
           memberlist or modullist, thus we do not need further checks */
    } else {
        if (!valid_module(n, modul_camac, 0)) {
            printf("lc3420.c: !valid_module\n");
            return -1;
        }
        module=ModulEnt(n);
        if (module->modultype!=LC_DISC_3420) {
            printf("lc3420.c: modultype=%08x", module->modultype);
            return -1;
        }
    }
    return 0;
}
/*****************************************************************************/
static plerrcode
set_reg(ml_entry* module, int A, int F, int value)
{
    struct camac_dev* dev=module->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(module), A, F);
    ems_u32 stat;

    if (dev->CAMACwrite(dev, &addr, value)<0) {
        printf("lc3420_set_reg(%d, %d, %d): errno=%s\n",
                module->address.camac.slot, A, F, strerror(errno));
        return plErr_System;
    }
    if (dev->CAMACstatus(dev, &stat)<0) {
        printf("lc3420_set_reg(%d, %d, %d): errno=%s\n",
                module->address.camac.slot, A, F, strerror(errno));
        return plErr_System;
    }
    if (dev->CAMACgotQX(stat)!=0xC0000000) {
        printf("lc3420_set_reg(%d, %d, %d): QX=%08x\n",
                module->address.camac.slot, A, F, stat);
        return plErr_HW;
    }

    return plOK;
}
/*****************************************************************************/
static plerrcode
set_threshold(ml_entry* module, int channel, int value)
{
    plerrcode pres=plOK;

    if (channel<0) {
        for (channel=0; channel<16; channel++) {
            pres=set_threshold(module, channel, value);
            if (pres!=plOK)
                return pres;
        }
    } else {
        pres=set_reg(module, channel, /*F*/16, value);
    }

    return pres;
}
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module (-1 for all modules of memberlist or modullist)
 * p[2]: channel (-1 for all channels)
 * p[3]: value, range 0..255 (threshold=value*5.5mV)
 */
plerrcode proc_lc3420_set_threshold(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        int i;
        if (memberlist) { /* iterate over memberlist */
            for (i=1; i<memberlist[0]; i++) {
                module=&modullist->entry[memberlist[i]];
                if (module->modulclass==modul_camac &&
                        module->modultype==LC_DISC_3420) {
                    pres=set_threshold(module, ip[2], p[3]);
                    if (pres!=plOK)
                        return pres;
                }
            }
        } else {          /* iterate over modullist */
            for (i=0; i<modullist->modnum; i++) {
                module=&modullist->entry[i];
                if (module->modulclass==modul_camac &&
                        module->modultype==LC_DISC_3420) {
                    pres=set_threshold(module, ip[2], p[3]);
                    if (pres!=plOK)
                        return pres;
                }
            }
        }
    } else {
        module=ModulEnt(p[1]);
        pres=set_threshold(module, ip[2], p[3]);
    }

    return pres;
}
plerrcode test_proc_lc3420_set_threshold(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    if (p[0]!=3)
        return plErr_ArgNum;
    if (check_module_type(ip[1])<0)
        return plErr_BadModTyp;
    if (ip[2]>15)
        return plErr_ArgRange;
    if (p[3]>0xff)
        return plErr_ArgRange;

    wirbrauchen=0;
    return plOK;
}
char name_proc_lc3420_set_threshold[]="lc3420_set_threshold";
int ver_proc_lc3420_set_threshold=1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module
 */
plerrcode proc_lc3420_get_threshold(ems_u32* p)
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
                printf("lc3420_get_threshold: QX=%08x\n", val[i]);
                return plErr_HW;
            }
            *outptr++=val[i]&~0xC0000000;
        }
    }
    return plOK;
}
plerrcode test_proc_lc3420_get_threshold(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    if (p[0]!=1)
        return plErr_ArgNum;
    if (ip[1]<0 || check_module_type(ip[1])<0)
        return plErr_BadModTyp;

    wirbrauchen=16;
    return plOK;
}
char name_proc_lc3420_get_threshold[]="lc3420_get_threshold";
int ver_proc_lc3420_get_threshold=1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module (-1 for all modules of memberlist or modullist)
 * p[2]: width
 * p[3]: deadtime
 */
plerrcode proc_lc3420_set_width(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;
    int val=(p[2]<<4)+(p[3]);
    plerrcode pres=plOK;

    if (ip[1]<0) {
        int i;
        if (memberlist) { /* iterate over memberlist */
            for (i=1; i<memberlist[0]; i++) {
                module=&modullist->entry[memberlist[i]];
                if (module->modulclass==modul_camac &&
                        module->modultype==LC_DISC_3420) {
                    pres=set_reg(module, 1, 17, val);
                    if (pres!=plOK)
                        return pres;
                }
            }
        } else {          /* iterate over moullist */
            for (i=0; i<modullist->modnum; i++) {
                module=&modullist->entry[i];
                if (module->modulclass==modul_camac &&
                        module->modultype==LC_DISC_3420) {
                    pres=set_reg(module, 1, 17, val);
                    if (pres!=plOK)
                        return pres;
                }
            }
        }
    } else {
        module=ModulEnt(p[1]);
        pres=set_reg(module, 1, 17, val);
    }

    return pres;
}
plerrcode test_proc_lc3420_set_width(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    if (p[0]!=3)
        return plErr_ArgNum;
    if (check_module_type(ip[1])<0)
        return plErr_BadModTyp;
    if (p[2]>0xf)
        return plErr_ArgRange;
    if (p[3]>0xf)
        return plErr_ArgRange;
    if (p[2]>p[3]) /* width must not be longer than deadtime */
        return plErr_ArgRange;

    wirbrauchen=0;
    return plOK;
}
char name_proc_lc3420_set_width[]="lc3420_set_width";
int ver_proc_lc3420_set_width=1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module
 */
plerrcode proc_lc3420_get_width(ems_u32* p)
{
    ems_u32 val;
    ml_entry* m=ModulEnt(p[1]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), /*A*/1, /*F*/1);
    if (dev->CAMACread(dev, &addr, &val)<0) {
        return plErr_System;
    } else {
        if ((val&0xC0000000)!=0xC0000000) {
            printf("lc3420_get_width: QX=%08x\n", val);
            return plErr_HW;
        }
        *outptr++=(val>>4)&0xf;
        *outptr++=val&0xf;
    }
    return plOK;
}
plerrcode test_proc_lc3420_get_width(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    if (p[0]!=1)
        return plErr_ArgNum;
    if (ip[1]<0 || check_module_type(ip[1])<0)
        return plErr_BadModTyp;

    wirbrauchen=2;
    return plOK;
}
char name_proc_lc3420_get_width[]="lc3420_get_width";
int ver_proc_lc3420_get_width=1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module (-1 for all modules of memberlist or modullist)
 * p[2]: mask
 */
plerrcode proc_lc3420_set_mask(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        int i;
        if (memberlist) { /* iterate over memberlist */
            for (i=1; i<memberlist[0]; i++) {
                module=&modullist->entry[memberlist[i]];
                if (module->modulclass==modul_camac &&
                        module->modultype==LC_DISC_3420) {
                    pres=set_reg(module, 0, 17, p[2]);
                    if (pres!=plOK)
                        return pres;
                }
            }
        } else {          /* iterate over moullist */
            for (i=0; i<modullist->modnum; i++) {
                module=&modullist->entry[i];
                if (module->modulclass==modul_camac &&
                        module->modultype==LC_DISC_3420) {
                    pres=set_reg(module, 0, 17, p[2]);
                    if (pres!=plOK)
                        return pres;
                }
            }
        }
    } else {
        module=ModulEnt(p[1]);
        pres=set_reg(module, 0, 17, p[2]);
    }

    return pres;
}
plerrcode test_proc_lc3420_set_mask(ems_u32* p)
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
char name_proc_lc3420_set_mask[]="lc3420_set_mask";
int ver_proc_lc3420_set_mask=1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module
 */
plerrcode proc_lc3420_get_mask(ems_u32* p)
{
    ems_u32 val;
    ml_entry* m=ModulEnt(p[1]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), /*A*/0, /*F*/1);
    if (dev->CAMACread(dev, &addr, &val)<0) {
        return plErr_System;
    } else {
        if ((val&0xC0000000)!=0xC0000000) {
            printf("lc3420_get_mask: QX=%08x\n", val);
            return plErr_HW;
        }
        *outptr++=val&~0xC0000000;
    }
    return plOK;
}
plerrcode test_proc_lc3420_get_mask(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    if (p[0]!=1)
        return plErr_ArgNum;
    if (ip[1]<0 || check_module_type(ip[1])<0)
        return plErr_BadModTyp;

    wirbrauchen=1;
    return plOK;
}
char name_proc_lc3420_get_mask[]="lc3420_get_mask";
int ver_proc_lc3420_get_mask=1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module (-1 for all modules of memberlist or modullist)
 * p[2]: tqc
 */
plerrcode proc_lc3420_set_tqc(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        int i;
        if (memberlist) { /* iterate over memberlist */
            for (i=1; i<memberlist[0]; i++) {
                module=&modullist->entry[memberlist[i]];
                if (module->modulclass==modul_camac &&
                        module->modultype==LC_DISC_3420) {
                    pres=set_reg(module, 2, 17, p[2]);
                    if (pres!=plOK)
                        return pres;
                }
            }
        } else {          /* iterate over moullist */
            for (i=0; i<modullist->modnum; i++) {
                module=&modullist->entry[i];
                if (module->modulclass==modul_camac &&
                        module->modultype==LC_DISC_3420) {
                    pres=set_reg(module, 2, 17, p[2]);
                    if (pres!=plOK)
                        return pres;
                }
            }
        }
    } else {
        module=ModulEnt(p[1]);
        pres=set_reg(module, 2, 17, p[2]);
    }

    return pres;
}
plerrcode test_proc_lc3420_set_tqc(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    if (p[0]!=2)
        return plErr_ArgNum;
    if (check_module_type(ip[1])<0)
        return plErr_BadModTyp;
    if (p[2]>0xff)
        return plErr_ArgRange;

    wirbrauchen=0;
    return plOK;
}
char name_proc_lc3420_set_tqc[]="lc3420_set_tqc";
int ver_proc_lc3420_set_tqc=1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module
 */
plerrcode proc_lc3420_get_tqc(ems_u32* p)
{
    ems_u32 val;
    ml_entry* m=ModulEnt(p[1]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), /*A*/2, /*F*/1);
    if (dev->CAMACread(dev, &addr, &val)<0) {
        return plErr_System;
    } else {
        if ((val&0xC0000000)!=0xC0000000) {
            printf("lc3420_get_tqc: QX=%08x\n", val);
            return plErr_HW;
        }
        *outptr++=val&~0xC0000000;
    }
    return plOK;
}
plerrcode test_proc_lc3420_get_tqc(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    if (p[0]!=1)
        return plErr_ArgNum;
    if (ip[1]<0 || check_module_type(ip[1])<0)
        return plErr_BadModTyp;

    wirbrauchen=1;
    return plOK;
}
char name_proc_lc3420_get_tqc[]="lc3420_get_tqc";
int ver_proc_lc3420_get_tqc=1;
/*****************************************************************************/
/*****************************************************************************/
