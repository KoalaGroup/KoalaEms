/*
 * procs/unixvme/sis3800/sis3600.c
 * created 24.Jan.2001 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3600.c,v 1.12 2015/04/06 21:33:37 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/unixvme/vme.h"
#include "../../../lowlevel/devices.h"
#include "../vme_verify.h"

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

RCS_REGISTER(cvsid, "procs/unixvme/sis3100")

#define lumis 21
static int lumi[lumis];

/*****************************************************************************/
/*
 * p[0]: argcount==number of modules
 * p[1...]: indices in memberlist
 */
plerrcode proc_sis3600init(ems_u32* p)
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
  if (id!=0x3600)
    {
    printf("sis3600init(idx=%d): id=%04x version=%d\n", i, id, version);
    printf("                     read 0x%08x\n", help);
    return plErr_HWTestFailed;
    }
  printf("sis3600init(idx=%d): id=%04x version=%d\n", i, id, version);

  /* reset module */
  dev->write_a32d32(dev, module->address.vme.addr+0x60, 0);

  /* clear FIFO */
  dev->write_a32d32(dev, module->address.vme.addr+0x20, 0);

  /* enable next clock logic */
  dev->write_a32d32(dev, module->address.vme.addr+0x28, 0);

  /* enable external next */
  dev->write_a32d32(dev, module->address.vme.addr+0, 0x10000);

  /* dummy clock */ /* probably wrong */
  /* dev->write_a32d32(dev, module->address.vme.addr+0x24, 0); */

  /* switch LED on (just for fun) */
  dev->write_a32d32(dev, module->address.vme.addr+0x0, 0x1 /*0x100: off*/);
  if (i<lumis) lumi[i]=1;
  }
return plOK;
}

plerrcode test_proc_sis3600init(ems_u32* p)
{
int i;

for (i=1; i<=p[0]; i++)
  {
  ml_entry* module;

  if (!valid_module(p[i], modul_vme)) return plErr_ArgRange;
  module=ModulEnt(p[i]);
  if (module->modultype!=SIS_3600) return plErr_BadModTyp;
  }
wirbrauchen=0;
return plOK;
}

char name_proc_sis3600init[] = "sis3600init";
int ver_proc_sis3600init = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==number of modules
 * p[1]: indices in memberlist
 */
plerrcode proc_sis3600read(ems_u32* p)
{
    int i;

    *outptr++=p[0];
    for (i=1; i<=p[0]; i++) {
        ems_u32* help;
        ems_u32 hh;
        ml_entry* module=ModulEnt(p[i]);
        struct vme_dev* dev=module->address.vme.dev;
        unsigned int addr=module->address.vme.addr;

        help=outptr++;
        while (dev->read_a32d32(dev, addr+0, &hh), (hh&0x100)==0) {
            /* fifo not empty */
            dev->read_a32d32(dev, addr+0x100, outptr++); /* read data */
        }
        *help=outptr-help-1;

        /* toggle LED (just for fun) */
        if (i<lumis) {
            lumi[i]++;
            if ((lumi[i]>>(i-1))&1)
                dev->write_a32d32(dev, addr+0x0, 0x1);
            else
                dev->write_a32d32(dev, addr+0x0, 0x100);
        }
    }
    return plOK;
}

plerrcode test_proc_sis3600read(ems_u32* p)
{
int i;
for (i=1; i<=p[0]; i++)
  {
  ml_entry* module;

  if (!valid_module(p[i], modul_vme)) return plErr_ArgRange;
  module=ModulEnt(p[i]);
  if (module->modultype!=SIS_3600) return plErr_BadModTyp;
  }
wirbrauchen=p[0]+1;
return plOK;
}

char name_proc_sis3600read[] = "sis3600read";
int ver_proc_sis3600read = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==number of modules
 * p[1]: indices in memberlist
 */
plerrcode proc_sis3600clock(ems_u32* p)
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

plerrcode test_proc_sis3600clock(ems_u32* p)
{
int i;
for (i=1; i<=p[0]; i++)
  {
  ml_entry* module;

  if (!valid_module(p[i], modul_vme)) return plErr_ArgRange;
  module=ModulEnt(p[i]);
  if (module->modultype!=SIS_3600) return plErr_BadModTyp;
  }
wirbrauchen=p[0]+1;
return plOK;
}

char name_proc_sis3600clock[] = "sis3600clock";
int ver_proc_sis3600clock = 1;
/*****************************************************************************/
/*
 * p[0]: argcount=3
 * p[1]: index in memberlist
 * p[2]: value to be waited for
 * p[3]: mask
 */
plerrcode proc_sis3600wait(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
    ems_u32 val;
    int nochmal, loopcount=0;

#if 0
    /* switch LED on (just for fun) */
    dev->write_a32d32(dev, addr+0x0, 0x1 /*0x100: off*/);
#endif

    do {
        ems_u32 hh;
        int fifocount=0;
        /* clock */
        dev->write_a32d32(dev, module->address.vme.addr+0x24, 0);
        /* read fifo until empty */
        while (dev->read_a32d32(dev, addr+0, &hh), (hh&0x100)==0) {
            dev->read_a32d32(dev, addr+0x100, &val);
            fifocount++;
        }
        if (fifocount!=1) {
            printf("fifocount=%d\n", fifocount);
        }
        loopcount++;
        /* check last value */
        nochmal=(val^p[2])&p[3];
    } while (nochmal);

#if 0
    /* switch LED off (just for fun) */
    dev->write_a32d32(dev, addr+0x0, 0x100);
#endif
    return plOK;
}
plerrcode test_proc_sis3600wait(ems_u32* p)
{
    ml_entry* module;
    plerrcode res;

    if (p[0]!=3) return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme)) return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=SIS_3600) return plErr_BadModTyp;
    if ((res=verify_vme_module(module, 1))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_sis3600wait[] = "sis3600wait";
int ver_proc_sis3600wait = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==number of modules
 * p[1]: index in memberlist
 */
plerrcode proc_sis3600initwait(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;

    /* reset module */
    dev->write_a32d32(dev, module->address.vme.addr+0x60, 0);

    /* clear FIFO */
    dev->write_a32d32(dev, module->address.vme.addr+0x20, 0);

    /* write output mode 2 */
    dev->write_a32d32(dev, module->address.vme.addr+0x00, 0x8);

    /* enable next clock logic */
    dev->write_a32d32(dev, module->address.vme.addr+0x28, 0);

    /* dummy clock */ /* probably wrong */
    /* dev->write_a32d32(dev, module->address.vme.addr+0x24, 0); */

    return plOK;
}
plerrcode test_proc_sis3600initwait(ems_u32* p)
{
    ml_entry* module;
    plerrcode res;

    if (p[0]!=1)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=SIS_3600)
        return plErr_BadModTyp;
    if ((res=verify_vme_module(module, 1))!=plOK)
        return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_sis3600initwait[] = "sis3600initwait";
int ver_proc_sis3600initwait = 1;
/*****************************************************************************/
/*****************************************************************************/
