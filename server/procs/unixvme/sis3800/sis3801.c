/*
 * procs/unixvme/sis3800/sis3801.c
 * created 24.Jan.2001 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3801.c,v 1.9 2011/04/06 20:30:35 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../vme_verify.h"
#include "../../procs.h"
#include "../../../commu/commu.h"
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
 * p[1...]: indices in memberlist
 */
plerrcode proc_sis3801init(ems_u32* p)
{
int i, res;

for (i=1; i<=p[0]; i++)
  {
  unsigned int help, id, version;
  ml_entry* module=ModulEnt(p[i]);
  struct vme_dev* dev=module->address.vme.dev;

  /* read module identification */
  res=dev->read_a32d32(dev, module->address.vme.addr+4, &help);
  if (res!=4) return plErr_System;
  id=(help>>16)&0xffff;
  version=(help>>12)&0xf;
  if (id!=0x3801)
    {
    printf("sis3801init(idx=%d): id=%04x version=%d\n", i, id, version);
    return plErr_HWTestFailed;
    }
  printf("sis3801init(idx=%d): id=%04x version=%d\n", i, id, version);

  /* reset module */
  dev->write_a32d32(dev, module->address.vme.addr+0x60, 0);

  /* clear FIFO */
  dev->write_a32d32(dev, module->address.vme.addr+0x20, 0);

  /* enable external next */
  dev->write_a32d32(dev, module->address.vme.addr+0x0, 0x10000);

  /* enable next clock logic */
  dev->write_a32d32(dev, module->address.vme.addr+0x28, 0);

  /* dummy clock */
  dev->write_a32d32(dev, module->address.vme.addr+0x24, 0);

  /* switch LED on (just for fun) */
  dev->write_a32d32(dev, module->address.vme.addr+0x0, 0x1 /*0x100: off*/);
  if (i<lumis) lumi[i]=1;
  }
return plOK;
}

plerrcode test_proc_sis3801init(ems_u32* p)
{
int i;

for (i=1; i<=p[0]; i++)
  {
  ml_entry* module;

  if (!valid_module(p[i], modul_vme, 0)) return plErr_ArgRange;
  module=ModulEnt(p[i]);
  if (module->modultype!=SIS_3801) return plErr_BadModTyp;
  }
wirbrauchen=0;
return plOK;
}

char name_proc_sis3801init[] = "sis3801init";
int ver_proc_sis3801init = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==number of modules
 * p[1]: idx of ems-variable
 * p[2...]: indices in memberlist
 */
plerrcode proc_sis3801ShadowInit(ems_u32* p)
{
    ems_u64 *var;
    ems_u32 *var_;
    int i;

    for (i=2; i<=p[0]; i++) {
        ml_entry* module=ModulEnt(p[i]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;

        /* reset module */
        dev->write_a32d32(dev, addr+0x60, 0);

        /* reset FIFO */
        dev->write_a32d32(dev, addr+0x20, 0);

        /* set input mode to '1' */
        dev->write_a32d32(dev, addr+0x0, 0x4);

        /* enable external next */
        dev->write_a32d32(dev, addr+0x0, 0x10000);

        /* enable next clock logic */
        dev->write_a32d32(dev, addr+0x28, 0);

        /* dummy clock */
        dev->write_a32d32(dev, addr+0x24, 0);

        /* switch LED on (just for fun) */
        dev->write_a32d32(dev, addr+0x0, 0x1 /*0x100: off*/);
        if (i-2<lumis)
            lumi[i-2]=1;
    }
    if (var_get_ptr(p[1], &var_)!=plOK)
        return plErr_Program;
    var=(ems_u64*)var_;
    for (i=0; i<(p[0]-1)*32; i++)
        var[i]=0;
    return plOK;
}

plerrcode test_proc_sis3801ShadowInit(ems_u32* p)
{
    unsigned int vsize;
    int i;
    plerrcode pcode;

    for (i=2; i<=p[0]; i++) {
        struct vme_dev* dev;
        ml_entry* module;
        ems_u32 addr;
        unsigned int help, id, version;

        if (!valid_module(p[i], modul_vme, 0))
            return plErr_ArgRange;
        module=ModulEnt(p[i]);
        if (module->modultype!=SIS_3801)
            return plErr_BadModTyp;
        dev=module->address.vme.dev;

        /* read module identification */
        addr=module->address.vme.addr;
        if (dev->read_a32d32(dev, addr+4, &help)!=4)
            return plErr_System;
        id=(help>>16)&0xffff;
        version=(help>>12)&0xf;
        if (id!=0x3801) {
            printf("sis3801ShadowInit(idx=%d, addr=0x%08x): id=%04x version=%d (0x%08x)\n",
                    i, addr, id, version, help);
            return plErr_HWTestFailed;
        }
    }
    if ((pcode=var_attrib(p[1], &vsize))!=plOK)
        return pcode;
    if (vsize!=(p[0]-1)*64)
        return plErr_IllVarSize;
    wirbrauchen=0;
    return plOK;
}

char name_proc_sis3801ShadowInit[] = "sis3801ShadowInit";
int ver_proc_sis3801ShadowInit = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==number of modules
 * p[1]: indices in memberlist
 */
plerrcode proc_sis3801read(ems_u32* p)
{
int i;

for (i=1; i<=p[0]; i++)
  {
  unsigned int help, *hlp, count, hh;
  ml_entry* module=ModulEnt(p[i]);
  struct vme_dev* dev=module->address.vme.dev;
  unsigned int addr=module->address.vme.addr;

  hlp=outptr++; count=0;
  /* fifo not empty? */
  dev->read_a32d32(dev, addr+0, &help);
  if (help&0x100)
    {
    printf("sis3801read(idx=%d): fifo empty\n", i);
    return plErr_HW;
    }
  /* read data */
  do
    {
    dev->read_a32d32(dev, addr+0x100, outptr++);
    count++;
    }
  while (dev->read_a32d32(dev, addr+0, &hh), !(hh&0x100));
  *hlp=count;
  /* toggle LED on (just for fun) */
  if (i<lumis)
    {
    lumi[i]++;
    if ((lumi[i]>>(i-1))&1)
      dev->write_a32d32(dev, addr+0x0, 0x1);
    else
      dev->write_a32d32(dev, addr+0x0, 0x100);
    }
  }
return plOK;
}

plerrcode test_proc_sis3801read(ems_u32* p)
{
int i;
for (i=1; i<=p[0]; i++)
  {
  ml_entry* module;

  if (!valid_module(p[i], modul_vme, 0)) return plErr_ArgRange;
  module=ModulEnt(p[i]);
  if (module->modultype!=SIS_3801) return plErr_BadModTyp;
  }
wirbrauchen=p[0]+1;
return plOK;
}

char name_proc_sis3801read[] = "sis3801read";
int ver_proc_sis3801read = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==number of modules
 * p[1]: indices in memberlist
 */
plerrcode proc_sis3801clock(ems_u32* p)
{
int i;

for (i=1; i<=p[0]; i++)
  {
  ml_entry* module=ModulEnt(p[i]);
  struct vme_dev* dev=module->address.vme.dev;

  dev->write_a32d32(dev, module->address.vme.addr+0x24, 0);
  }
return plOK;
}

plerrcode test_proc_sis3801clock(ems_u32* p)
{
int i;
for (i=1; i<=p[0]; i++)
  {
  ml_entry* module;

  if (!valid_module(p[i], modul_vme, 0)) return plErr_ArgRange;
  module=ModulEnt(p[i]);
  if (module->modultype!=SIS_3801) return plErr_BadModTyp;
  }
wirbrauchen=p[0]+1;
return plOK;
}

char name_proc_sis3801clock[] = "sis3801clock";
int ver_proc_sis3801clock = 1;
/*****************************************************************************/
/*
 * all modules of the memberlist have to be sis3801
 * all channels are read out
 */
/*
 * p[0]: argcount==1 or 2
 * p[1]: idx of ems-variable
 * [p[2]: bitmask for latchpulse (via controller sis3100)]
 */
plerrcode proc_sis3801ShadowUpdate(ems_u32* p)
{
    ems_u32 tmp[65], *var_;
    ems_u64 *var;
    int res, i, j;

    var_get_ptr(p[1], &var_);
    var=(ems_u64*)var_;

    if (p[0]>1) {
        ems_u32 latchmask=p[2];
        if (latchmask) {
            ml_entry* module=ModulEnt(1);
            struct vme_dev* dev=module->address.vme.dev;
            dev->write_controller(dev, 1, 0x80, (latchmask&0x7f)<<24);
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
        if ((res=dev->read_a32_fifo(dev, addr+0x100, tmp, 260, 4, 0))<0)
            return plErr_System;
        if (res!=32*4) {
            char s1[128], s2[128];
            snprintf(s1, 128, "sis3801ShadowUpdate: read_fifo: %d words "
                    "(instead of 32)", res/4);
            snprintf(s2, 128, "crate %d addr 0x%x",
                    module->address.vme.crate, addr);
            printf("%s %s\n", s1, s2);
            send_unsol_text(s1, s2, 0);
        }

        /* update shadow */
        for (j=0; j<32; j++) {
            var[j]+=tmp[j];
        }
        memcpy(outptr, var, 32*sizeof(ems_u64));
        outptr+=64;
        var+=32;
    }
    return plOK;
}

plerrcode test_proc_sis3801ShadowUpdate(ems_u32* p)
{
    ems_u32 mtypes[]={SIS_3801, 0};
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

    /* check version of sis3801 */
    for (i=0; i<memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i+1);
        struct vme_dev* dev=module->address.vme.dev;
        unsigned int addr=module->address.vme.addr;
        ems_u32 val;
        dev->read_a32d32(dev, addr+4, &val);
        if (((val>>12)&0xf)<0xb) {
            printf("old version of SIS3801 at address 0x%08x: 0x%08x\n",
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
            printf("sis3801ShadowUpdateBlock: illegal controller for latchpulse\n");
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=memberlist[0]*65+1;
    return plOK;
}

char name_proc_sis3801ShadowUpdate[] = "sis3801ShadowUpdate";
int ver_proc_sis3801ShadowUpdate = 1;
/*****************************************************************************/
/*
 * all modules of the memberlist have to be sis3801
 * all channels are cleared
 */
/*
 * p[0]: argcount==1 or 2
 * p[1]: idx of ems-variable
 * [p[2]: bitmask for latchpulse (via controller sis3100)]
 */
plerrcode proc_sis3801ShadowClear(ems_u32* p)
{
    ems_u32 tmp[65], *var;
    int i;

    var_get_ptr(p[1], &var);

    if (p[0]>1) {
        ems_u32 latchmask=p[2];
        if (latchmask) {
            ml_entry* module=ModulEnt(1);
            struct vme_dev* dev=module->address.vme.dev;
            dev->write_controller(dev, 1, 0x80, (latchmask&0x7f)<<24);
        }
    }

    for (i=0; i<memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i+1);
        struct vme_dev* dev=module->address.vme.dev;
        unsigned int addr=module->address.vme.addr;

        /* switch banks if no latchpulse was sent */
        if (p[0]<2)
            dev->write_a32d32(dev, addr+0x24, 0);
        /* read unwanted data */
        dev->read_a32_fifo(dev, addr+0x100, tmp, 260, 4, 0);
    }

    /* clear shadow */
    var_clear(p[1]);
    return plOK;
}

plerrcode test_proc_sis3801ShadowClear(ems_u32* p)
{
    ems_u32 mtypes[]={SIS_3801, 0};
    unsigned int vsize;
    int nr_modules, nr_channels, res;
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

    if (p[0]>1) {
        /* send latch pulse is only possible with sis3100 controller */
        ml_entry* module=ModulEnt(1);
        struct vme_dev* dev=module->address.vme.dev;
        if (dev->vmetype!=vme_sis3100) {
            printf("sis3801ShadowClearBlock: illegal controller for latchpulse\n");
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_sis3801ShadowClear[] = "sis3801ShadowClear";
int ver_proc_sis3801ShadowClear = 1;
/*****************************************************************************/
/*****************************************************************************/
