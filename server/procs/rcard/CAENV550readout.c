/************************************************************************
 ************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: CAENV550readout.c,v 1.6 2011/04/06 20:30:34 wuestner Exp $";

#include <stdio.h>

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/unixvme/vme.h"

#include "CAENV550readout.h"

extern ems_u32* outptr;

struct vme_dev* dev;

RCS_REGISTER(cvsid, "procs/rcard")

int CAENV550readout(ml_entry * module,int memberidx)
{
  int res;
  int wordcount;
  int temp;
  unsigned short help;
  int j;
  memberidx += 1;
  dev = module->address.vme.dev;

  /* write header word for this ADC unit */
  *outptr++ = CAEN_V550;

  /* channel 0 */
  
  /* read number of entries in FIFO 0 */
  res = dev->read_a32d16(dev, module->address.vme.addr + 0x0010, &help); // the 16 bit access
  if(res!=2) return(0);
  
  // extract real number of entries in FIFO 0
  wordcount = ((int)help & 0x000007ff);
  /* write header word for this ADC subchannel and memberindex */
  temp = wordcount;
  temp += (0<<11);
  temp += (memberidx<<12);
  temp += (module->address.vme.crate<<18);
  *outptr++ = temp;

  /* read the data from FIFO 0 */
  for(j=0;j<wordcount;j++) {
    res = dev->read_a32d32(dev, module->address.vme.addr + 0x0008, outptr);
    if(res==4) {
      outptr++;
    } else {
      *outptr++ = res;
      return(0);
    }
  }
  
  /* read number of entries in FIFO 1 */
  res = dev->read_a32d16(dev, module->address.vme.addr + 0x0012, &help); // the 16 bit access
  if(res!=2) return(0);
  
  // extract real number of entries in FIFO 1
  wordcount = ((int)help & 0x000007ff);

  //write header word for this ADC subchannel and memberindex
  temp = wordcount;
  temp += (1<<11);
  temp += (memberidx<<12);
  temp += (module->address.vme.crate<<18);
  *outptr++ = temp;

  // read the data from FIFO 1
  for(j=0;j<wordcount;j++) {
    res = dev->read_a32d32(dev, module->address.vme.addr + 0x000c, outptr);
    if(res==4) {
      outptr++;
    } else {
      *outptr++ = res;
      return(0);
    }
  }

  return(1);	
}

int CAENV550readoutCM(ml_entry * module,int memberidx,int pedestal)
{
  int res;
  int wordcount;
  int temp;
  unsigned short help;
  int j;
  memberidx += 1;
  dev = module->address.vme.dev;

  // write header word for this ADC unit
  *outptr++ = CAEN_V550;

  // read number of entries in FIFO 0
  res = dev->read_a32d16(dev, module->address.vme.addr + 0x0010, &help); // the 16 bit access
  if(res!=2) return(0);

  // extract real number of entries in FIFO 0
  wordcount = ((int)help & 0x000007ff);
	  
  //write header word for this ADC subchannel and memberindex
  temp = wordcount;
  temp += (0<<11);
  temp += (memberidx<<12);
  temp += (module->address.vme.crate<<18);
  *outptr++ = temp;

  // read the data from FIFO 0
  for(j=0;j<wordcount;j++) {
    res = dev->read_a32d32(dev, module->address.vme.addr + 0x0008, outptr);
    if(res==4) {
      outptr++;
    } else {
      *outptr++ = res;
      return(0);
    }
  }

  // read number of entries in FIFO 1
  res = dev->read_a32d16(dev, module->address.vme.addr + 0x0012, &help); // the 16 bit access
  if(res!=2) return(0);

  // extract real number of entries in FIFO 1
  wordcount = ((int)help & 0x000007ff);
	  
  //write header word for this ADC subchannel and memberindex
  temp = wordcount;
  temp += (1<<11);
  temp += (memberidx<<12);
  temp += (module->address.vme.crate<<18);
  *outptr++ = temp;

  // read the data from FIFO 1
  for(j=0;j<wordcount;j++) {
    res = dev->read_a32d32(dev, module->address.vme.addr + 0x000c, outptr);
    if(res==4) {
      outptr++;
    } else {
      *outptr++ = res;
      return(0);
    }
  }

  return(1);	
}
