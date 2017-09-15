/************************************************************************
 ************************************************************************/            
static const char* cvsid __attribute__((unused))=
    "$ZEL: SIS3801readout.c,v 1.6 2011/04/06 20:30:34 wuestner Exp $";

#include <stdio.h>

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/unixvme/vme.h"

#include "SIS3801readout.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/rcard")

struct vme_dev* dev;

int SIS3801readout(ml_entry * module,int memberidx)
{
  int res;
  int temp;
  int j;
  memberidx += 1;
  // write header word for this Scaler unit
  *outptr++ = SIS_3801;
  
  temp = (memberidx<<12);
  *outptr++ = temp;  
  
  for (j=0;j<32;j++) {
    res = dev->read_a32d32(dev, module->address.vme.addr + 0x100, outptr);
    outptr++;
  }
  
  return(1);	
}

int SIS3800readout(ml_entry * module,int memberidx)
{
  int res;
  int temp;
  int j;
  memberidx += 1;
  // write header word for this Scaler unit
  *outptr++ = SIS_3800;
  
  temp = (memberidx<<12);
  *outptr++ = temp;  
  /* make shadow clock, moves data in output buffer */
  res = dev->write_a32d32(dev, module->address.vme.addr + 0x024, 1);
  if(res != 4) { /* for any chance */
    res = dev->write_a32d32(dev, module->address.vme.addr + 0x024, 1);
  }
  for (j=0;j<32;j++) {
    res = dev->read_a32d32(dev, module->address.vme.addr + 0x200 + j*4, outptr);
    outptr++;
  }
  
  return(1);	
}

