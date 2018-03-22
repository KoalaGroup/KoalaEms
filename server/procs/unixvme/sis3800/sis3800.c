/*
 * procs/unixvme/sis3800/sis3800.c
 * created 24.Jan.2001 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3800.c,v 1.13 2017/10/20 23:20:52 wuestner Exp $";

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

extern ems_u32* outptr;
extern unsigned int *memberlist;

#define lumis 21
static int lumi[lumis];

RCS_REGISTER(cvsid, "procs/unixvme/sis3800")

/*****************************************************************************/
/*
 * p[0]: argcount==number of modules
 * p[1...]: indices in memberlist
 */
plerrcode proc_sis3800init(ems_u32* p)
{
unsigned int i;
int res;

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
  if (id!=0x3800)
    {
    printf("sis3800init(idx=%d): id=%04x version=%d\n", i, id, version);
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
  if (i<lumis) lumi[i]=1;
  }
return plOK;
}

plerrcode test_proc_sis3800init(ems_u32* p)
{
unsigned int i;

for (i=1; i<=p[0]; i++)
  {
  ml_entry* module;

  if (!valid_module(p[i], modul_vme)) return plErr_ArgRange;
  module=ModulEnt(p[i]);
  if (module->modultype!=SIS_3800) return plErr_BadModTyp;
  }
wirbrauchen=0;
return plOK;
}

char name_proc_sis3800init[] = "sis3800init";
int ver_proc_sis3800init = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==number of modules
 * p[1]: indices in memberlist
 */
plerrcode proc_sis3800read(ems_u32* p)
{
unsigned int i;

for (i=1; i<=p[0]; i++)
  {
  ml_entry* module=ModulEnt(p[i]);
  struct vme_dev* dev=module->address.vme.dev;
  unsigned int addr=module->address.vme.addr, j;

  /* read data */
  for (j=0; j<32; j++)
    dev->read_a32d32(dev, addr+0x200+j*4, outptr++);
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

plerrcode test_proc_sis3800read(ems_u32* p)
{
unsigned int i;
for (i=1; i<=p[0]; i++)
  {
  ml_entry* module;

  if (!valid_module(p[i], modul_vme)) return plErr_ArgRange;
  module=ModulEnt(p[i]);
  if (module->modultype!=SIS_3800) return plErr_BadModTyp;
  }
wirbrauchen=p[0]*32;
return plOK;
}

char name_proc_sis3800read[] = "sis3800read";
int ver_proc_sis3800read = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==number of modules
 * p[1...]: indices in memberlist
 */
plerrcode proc_sis3800read_block(ems_u32* p)
{
    unsigned int i;

    for (i=1; i<=p[0]; i++) {
        ml_entry* module=ModulEnt(p[i]);
        struct vme_dev* dev=module->address.vme.dev;
        unsigned int addr=module->address.vme.addr;

        if (dev->read_a32(dev, addr+0x200, outptr, 128, 4, &outptr)<0) {
            return plErr_System;
        }
    }
    return plOK;
}

plerrcode test_proc_sis3800read_block(ems_u32* p)
{
    unsigned int i;
    for (i=1; i<=p[0]; i++) {
        ml_entry* module;

        if (!valid_module(p[i], modul_vme))
            return plErr_ArgRange;
        module=ModulEnt(p[i]);
        if (module->modultype!=SIS_3800)
            return plErr_BadModTyp;
    }
    wirbrauchen=p[0]*32;
    return plOK;
}

char name_proc_sis3800read_block[] = "sis3800read_block";
int ver_proc_sis3800read_block = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==number of modules
 * p[1]: indices in memberlist
 */
plerrcode proc_sis3800clock(ems_u32* p)
{
unsigned int i;

for (i=1; i<=p[0]; i++)
  {
  ml_entry* module=ModulEnt(p[i]);
  struct vme_dev* dev=module->address.vme.dev;

  dev->write_a32d32(dev, module->address.vme.addr+0x24, 0);
  }
return plOK;
}

plerrcode test_proc_sis3800clock(ems_u32* p)
{
unsigned int i;
for (i=1; i<=p[0]; i++)
  {
  ml_entry* module;

  if (!valid_module(p[i], modul_vme)) return plErr_ArgRange;
  module=ModulEnt(p[i]);
  if (module->modultype!=SIS_3800) return plErr_BadModTyp;
  }
wirbrauchen=p[0]+1;
return plOK;
}

char name_proc_sis3800clock[] = "sis3800clock";
int ver_proc_sis3800clock = 1;
/*****************************************************************************/
/*****************************************************************************/
