/*
 * procs/unixvme/sis3800/sis3800_shadow.c
 * created 25.Jan.2001 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3800_shadow.c,v 1.19 2015/04/06 21:33:37 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../vme_verify.h"
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/unixvme/vme.h"
#include "../../../objects/var/variables.h"
#include "../../../lowlevel/devices.h"
#include "../../../trigger/trigger.h"

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

#define lumis 21
static int lumi[lumis];

RCS_REGISTER(cvsid, "procs/unixvme/sis3800")

/*****************************************************************************/
/*
 * p[0]: argcount==number of modules
 * p[1]: idx of ems-variable
 * p[2...]: indices in memberlist
 */
plerrcode proc_sis3800ShadowInit(ems_u32* p)
{
ems_u32 *var;
int i, res;

for (i=2; i<=p[0]; i++)
  {
  unsigned int help, id, version;
  ml_entry* module=ModulEnt(p[i]);
  struct vme_dev* dev=module->address.vme.dev;
  ems_u32 addr=module->address.vme.addr;

  /* read module identification */
  res=dev->read_a32d32(dev, addr+4, &help);
  if (res!=4) return plErr_System;
  id=(help>>16)&0xffff;
  version=(help>>12)&0xf;
  if (id!=0x3800)
    {
    printf("sis3800ShadowInit(idx=%d, addr=0x%08x): id=%04x version=%d (0x%08x)\n",
            i, addr, id, version, help);
    return plErr_HWTestFailed;
    }

  /* reset module */
  dev->write_a32d32(dev, module->address.vme.addr+0x60, 0);

  /* clear all counters (really necessary after reset?) */
  dev->write_a32d32(dev, module->address.vme.addr+0x20, 0);

  /* set input mode to '1' */
  dev->write_a32d32(dev, module->address.vme.addr+0x0, 0x4);

  /* enable counting */
  dev->write_a32d32(dev, module->address.vme.addr+0x28, 0);

  /* switch LED on (just for fun) */
  dev->write_a32d32(dev, module->address.vme.addr+0x0, 0x1 /*0x100: off*/);
  if (i-2<lumis) lumi[i-2]=1;
  }
if (var_get_ptr(p[1], &var)!=plOK) return plErr_Program;
for (i=0; i<(p[0]-1)*64; i++) var[i]=0;
return plOK;
}

plerrcode test_proc_sis3800ShadowInit(ems_u32* p)
{
unsigned int vsize;
int i;
plerrcode pcode;

for (i=2; i<=p[0]; i++)
  {
  ml_entry* module;

  if (!valid_module(p[i], modul_vme)) return plErr_ArgRange;
  module=ModulEnt(p[i]);
  if (module->modultype!=SIS_3800) return plErr_BadModTyp;
  }
if ((pcode=var_attrib(p[1], &vsize))!=plOK) return pcode;
if (vsize!=(p[0]-1)*64) return plErr_IllVarSize;
wirbrauchen=0;
return plOK;
}

char name_proc_sis3800ShadowInit[] = "sis3800ShadowInit";
int ver_proc_sis3800ShadowInit = 1;
/*****************************************************************************/
/*
 * all modules of the memberlist have to be sis3800
 * all channels are cleared
 *
 * p[0]: argcount==1
 * p[1]: idx of ems-variable
 */
plerrcode proc_sis3800ShadowClear(ems_u32* p)
{
    ems_u32 *var;
    unsigned int vsize;
    int i;

    var_get_ptr(p[1], &var);
    var_attrib(p[1], &vsize);

    /* clear variable */
    memset(var, 0, vsize*sizeof(ems_u32));

    for (i=0; i<memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i+1);
        struct vme_dev* dev=module->address.vme.dev;
        unsigned int addr=module->address.vme.addr;

        /* disable counting */
        dev->write_a32d32(dev, addr+0x2c, 0);

        /* clear all counters */
        dev->write_a32d32(dev, addr+0x20, 0);

        /* enable counting */
        dev->write_a32d32(dev, addr+0x28, 0);
    }

    return plOK;
}

plerrcode test_proc_sis3800ShadowClear(ems_u32* p)
{
    ems_u32 mtypes[]={SIS_3800, 0};
    unsigned int vsize;
    int nr_modules, nr_channels, res;
    plerrcode pcode;

    if (p[0]!=1)
        return plErr_ArgNum;
    if (!memberlist)
        return plErr_NoISModulList;

    nr_modules=memberlist[0];
    nr_channels=nr_modules*32;
    if ((pcode=var_attrib(p[1], &vsize))!=plOK)
        return pcode;
    if (vsize!=nr_channels*2)
        return plErr_IllVarSize;

    if ((res=test_proc_vme(memberlist, mtypes))!=plOK)
        return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_sis3800ShadowClear[] = "sis3800ShadowClear";
int ver_proc_sis3800ShadowClear = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==(number of modules)*2+2
 * p[1]: idx of ems-variable
 * p[2]: number of members
 * p[3...3+p[2]-1]: indices in memberlist
 * p[3+p[2]...3+2*p[2]-1] bitmasks
 * [p[3+2*p[2]] bitmask for latchpulse (via controller sis3100)]
 *   if a latchpulse bitmask is given, but its value is 0 then a soft clock
 *   is used
 */
plerrcode proc_sis3800ShadowUpdate(ems_u32* p)
{
    ems_u32 *var;
    int i, idx;

    var_get_ptr(p[1], &var);

    if (p[0]>2*p[2]+2) {
        ems_u32 latchmask=p[2*p[2]+3];
        if (latchmask) {
            ml_entry* module=ModulEnt(p[3]);
            struct vme_dev* dev=module->address.vme.dev;
            dev->write_controller(dev, 1, 0x80, (latchmask&0x7f)<<24);
        } else {
            for (i=0; i<p[2]; i++) {
                ml_entry* module=ModulEnt(p[i+3]);
                struct vme_dev* dev=module->address.vme.dev;
                unsigned int addr=module->address.vme.addr;

                dev->write_a32d32(dev, addr+0x24, 0);
            }
        }
    }

    *outptr++=p[2];
    idx=0;
    for (i=0; i<p[2]; i++) {
        ml_entry* module=ModulEnt(p[i+3]);
        struct vme_dev* dev=module->address.vme.dev;
        unsigned int addr=module->address.vme.addr, j, mask, n;
        ems_u32 *help;

        mask=p[3+p[2]+i];
        help=outptr++;
        n=0;
        /* read data */
        for (j=0; j<32; j++) {
            ems_u32 val;

            dev->read_a32d32(dev, addr+0x200+j*4, &val);
            /* check for overflow (but ignore bits 0..5) */
            if ((val&0xffffffc0)<(var[idx+1]&0xffffffc0)) {
                ems_u16 oval;

                /* verify overflow (to distinguish from 'clear') */
                dev->read_a32d16(dev, addr+0x380+0x20*(j/8), &oval);
                if ((oval&(1<<(8+j%8)))!=0) {
                    /* clear overflow register */
                    dev->write_a32d32(dev, addr+0x180+4*j, 0);
                    var[idx]++;
                } else {
                    printf("backward jump on sis3800 addr 0x%08x chan %2d\n",
                            addr, j);
                }
            }
            var[idx+1]=val;
            if (mask&1) {
                *outptr++=var[idx];
                *outptr++=var[idx+1];
                n++;
            }
            idx+=2; mask>>=1;
        }
        *help=n;
        /* toggle LED (just for fun) */
        if (i<lumis) {
            lumi[i]++;
            if ((lumi[i]>>i)&1)
                dev->write_a32d32(dev, addr+0x0, 0x1);
            else
                dev->write_a32d32(dev, addr+0x0, 0x100);
        }
    }
    return plOK;
}

plerrcode test_proc_sis3800ShadowUpdate(ems_u32* p)
{
    unsigned int vsize;
    int i;
    plerrcode pcode;

    if (p[0]<2)
        return plErr_ArgNum;
    if (p[1]<1) /* empty list of modules */
        return plErr_ArgRange;
    if ((pcode=var_attrib(p[1], &vsize))!=plOK)
        return pcode;
    if (vsize!=p[2]*64)
        return plErr_IllVarSize;
    if ((p[0]!=2*p[2]+2) && (p[0]!=2*p[2]+3))
        return plErr_ArgNum;

    for (i=0; i<p[2]; i++) {
        ml_entry* module;

        if (!valid_module(p[i+3], modul_vme))
            return plErr_ArgRange;
        module=ModulEnt(p[i+3]);
        if (module->modultype!=SIS_3800)
            return plErr_BadModTyp;
    }

    if (p[0]>2*p[2]+2) {
        /* send latch pulse is only possible with sis3100 controller */
        ml_entry* module=ModulEnt(p[3]);
        struct vme_dev* dev=module->address.vme.dev;
        if (dev->vmetype!=vme_sis3100) {
            printf("sis3800ShadowUpdate: illegal controller for latchpulse\n");
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=p[2]*65+1;
    return plOK;
}

char name_proc_sis3800ShadowUpdate[] = "sis3800ShadowUpdate";
int ver_proc_sis3800ShadowUpdate = 1;
/*****************************************************************************/
/*
 * simple version of sis3800ShadowUpdate
 * all modules of the memberlist have to be sis3800
 * all channels are read out
 */
/*
 * p[0]: argcount==1 or 2
 * p[1]: idx of ems-variable
 * [p[2]: bitmask for latchpulse (via controller sis3100)]
 */
plerrcode proc_sis3800ShadowUpdateAll(ems_u32* p)
{
    ems_u32 *var;
    int i, idx;

    var_get_ptr(p[1], &var);

    if (p[0]>1) {
        if (p[2]) { /* real latch pulse via hardware */
            ems_u32 latchmask=p[2];
            if (latchmask) {
                ml_entry* module=ModulEnt(1);
                struct vme_dev* dev=module->address.vme.dev;
                dev->write_controller(dev, 1, 0x80, (latchmask&0x7f)<<24);
            }
        } else { /* software latch */
            for (i=0; i<memberlist[0]; i++) {
                ml_entry* module=ModulEnt(i+1);
                struct vme_dev* dev=module->address.vme.dev;
                unsigned int addr=module->address.vme.addr;

                dev->write_a32d32(dev, addr+0x24, 0);
            }
        }
    }

    *outptr++=memberlist[0];
    idx=0;
    for (i=0; i<memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i+1);
        struct vme_dev* dev=module->address.vme.dev;
        unsigned int addr=module->address.vme.addr, j;

        *outptr++=32;
        /* read data */
        for (j=0; j<32; j++) {
            ems_u32 val;

            dev->read_a32d32(dev, addr+0x200+j*4, &val);
            /* check for overflow (but ignore bits 0..5) */
            if ((val&0xffffffc0)<(var[idx+1]&0xffffffc0)) {
                ems_u16 oval;

                /* verify overflow (to distinguish from 'clear') */
                dev->read_a32d16(dev, addr+0x380+0x20*(j/8), &oval);
                if ((oval&(1<<(8+j%8)))!=0) {
                    /* clear overflow register */
                    dev->write_a32d32(dev, addr+0x180+4*j, 0);
                    var[idx]++;
                } else {
                    printf("backward jump on sis3800 addr 0x%08x chan %2d\n",
                            addr, j);
                    printf("%08x-->%08x\n", var[idx+1], val);
                }
            }
            var[idx+1]=val;
            *outptr++=var[idx];
            *outptr++=var[idx+1];
            idx+=2;
        }
        /* toggle LED (just for fun) */
        if (i<lumis) {
            lumi[i]++;
            if ((lumi[i]>>i)&1)
                dev->write_a32d32(dev, addr+0x0, 0x1);
            else
                dev->write_a32d32(dev, addr+0x0, 0x100);
        }
    }
    return plOK;
}

plerrcode test_proc_sis3800ShadowUpdateAll(ems_u32* p)
{
    ems_u32 mtypes[]={SIS_3800, 0};
    unsigned int vsize;
    int nr_modules, nr_channels, res;
    plerrcode pcode;

    if ((p[0]<1) && (p[0]>2))
        return plErr_ArgNum;
    if (!memberlist)
        return plErr_NoISModulList;

    nr_modules=memberlist[0];
    nr_channels=nr_modules*32;
    if ((pcode=var_attrib(p[1], &vsize))!=plOK)
        return pcode;
    if (vsize!=nr_channels*2)
        return plErr_IllVarSize;

    if ((res=test_proc_vme(memberlist, mtypes))!=plOK)
        return res;

    if (p[0]>1) {
        /* send latch pulse is only possible with sis3100 controller */
        ml_entry* module=ModulEnt(1);
        struct vme_dev* dev=module->address.vme.dev;
        if (dev->vmetype!=vme_sis3100) {
            printf("sis3800ShadowUpdateAll: illegal controller for latchpulse\n");
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=memberlist[0]*65+1;
    return plOK;
}

char name_proc_sis3800ShadowUpdateAll[] = "sis3800ShadowUpdateAll";
int ver_proc_sis3800ShadowUpdateAll = 1;
/*****************************************************************************/
/*
 * faster version of sis3800ShadowUpdateAll
 * all modules of the memberlist have to be sis3800
 * all channels are read out
 */
/*
 * p[0]: argcount==1 or 2
 * p[1]: idx of ems-variable
 * [p[2]: bitmask for latchpulse (via controller sis3100)]
 */
plerrcode proc_sis3800ShadowUpdateAll2(ems_u32* p)
{
    ems_u32 tmp[32];
    ems_u32 *var;
    int i, j;

    var_get_ptr(p[1], &var);

    if (p[0]>1) {
        if (p[2]) {
            ems_u32 latchmask=p[2];
            if (latchmask) {
                ml_entry* module=ModulEnt(1);
                struct vme_dev* dev=module->address.vme.dev;
                dev->write_controller(dev, 1, 0x80, (latchmask&0x7f)<<24);
            }
        } else {
            for (i=0; i<memberlist[0]; i++) {
                ml_entry* module=ModulEnt(i+1);
                struct vme_dev* dev=module->address.vme.dev;
                unsigned int addr=module->address.vme.addr;

                dev->write_a32d32(dev, addr+0x24, 0);
            }
        }
    }

    /* store number of modules */
    *outptr++=memberlist[0];

    for (i=0; i<memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i+1);
        struct vme_dev* dev=module->address.vme.dev;
        unsigned int addr=module->address.vme.addr;

        /* store number of channels */
        *outptr++=32;

        /* read data */
        if (dev->read_a32(dev, addr+0x200, tmp, 128, 4, 0)<0)
            return plErr_System;

        /* update shadow */
        for (j=0; j<32; j++) {
            /* check for overflow (but ignore bits 0..5) */
            if ((tmp[j]&0xffffffc0)<(var[j*2+1]&0xffffffc0)) {
                ems_u16 oval;
                /* verify overflow (to distinguish from 'clear') */
                dev->read_a32d16(dev, addr+0x380+0x20*(j/8), &oval);
                if ((oval&(1<<(8+j%8)))!=0) {
                    /* clear overflow register */
                    dev->write_a32d32(dev, addr+0x180+4*j, 0);
                    var[j*2]++;
                } else {
                    printf("backward jump on sis3800 addr 0x%08x chan %2d\n",
                            addr, j);
                    printf("tmp=0x%08x var=0x%08x oval=0x%04x\n",
                            tmp[j], var[j*2+1], oval);
                }
            }
            var[j*2+1]=tmp[j];
        }
        memcpy(outptr, var, 64*sizeof(ems_u32));
        outptr+=64;
        var+=64;
    }
    return plOK;
}

plerrcode test_proc_sis3800ShadowUpdateAll2(ems_u32* p)
{
    ems_u32 mtypes[]={SIS_3800, 0};
    unsigned int vsize;
    int nr_modules, nr_channels, res, i;
    plerrcode pcode=plOK;

    if ((p[0]<1) && (p[0]>2))
        return plErr_ArgNum;
    if (!memberlist)
        return plErr_NoISModulList;

    nr_modules=memberlist[0];
    nr_channels=nr_modules*32;
    if ((pcode=var_attrib(p[1], &vsize))!=plOK)
        return pcode;
    if (vsize!=nr_channels*2)
        return plErr_IllVarSize;

    if ((res=test_proc_vme(memberlist, mtypes))!=plOK)
        return res;

    /* check version of sis3800 */
    for (i=0; i<memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i+1);
        struct vme_dev* dev=module->address.vme.dev;
        unsigned int addr=module->address.vme.addr;
        ems_u32 val;
        dev->read_a32d32(dev, addr+4, &val);
        if (((val>>12)&0xf)<5) {
            printf("old version of SIS3800 at address 0x%08x: 0x%08x\n",
                    addr, val);
            pcode=plErr_HWTestFailed;
        }
    }
    if (pcode!=plOK)
        return pcode;

    if (p[0]>1) {
        /* send latch pulse is only possible with sis3100 controller */
        ml_entry* module=ModulEnt(1);
        struct vme_dev* dev=module->address.vme.dev;
        if (dev->vmetype!=vme_sis3100) {
            printf("sis3800ShadowUpdateAll2: illegal controller for latchpulse\n");
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=memberlist[0]*65+1;
    return plOK;
}

char name_proc_sis3800ShadowUpdateAll2[] = "sis3800ShadowUpdateAll2";
int ver_proc_sis3800ShadowUpdateAll2 = 2;
/*****************************************************************************/
/*****************************************************************************/
