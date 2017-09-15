/************************************************************************
 ************************************************************************/                           
static const char* cvsid __attribute__((unused))=
    "$ZEL: RCardUtils.c,v 1.6 2011/04/06 20:30:34 wuestner Exp $";

#include <stdio.h>

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/unixvme/vme.h"
#include "RCardUtils.h"

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

  dev = module->address.vme.dev;

  // write header word for this ADC unit
  *outptr++ = CAEN_V550;

  // channel 0
  
  // read number of entries in FIFO 0
  res = dev->read_a32d16(dev, module->address.vme.addr + 0x0010, &help); // the 16 bit access
  if(res!=2) return(0);
  
  // extract real number of entries in FIFO 0
  wordcount = ((int)help & 0x000007ff);
	  
  // printf("%d\n",wordcount);

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
	  
  // printf("%d\n",wordcount);

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

int CAENV775readout(ml_entry * module,int memberidx)
{
  int res;
  int wordcount;
  int temp;
  int help;
  int j;
  
  // write header word for this TDC unit
  *outptr++ = CAEN_V775;

  res = dev->read_a32d32(dev, module->address.vme.addr + 0x0000, &help); // read header
  if(res!=4) return(0);
  
  if((help & 0x06000000) && !(help & 0x04000000)) { // check if data is valid
    temp = 0;
    temp += (memberidx<<12);
    temp += (module->address.vme.crate<<18);
    *outptr++ = temp;
    return(1);
  }
  
  // extract real number of entries
  wordcount = (((int)help & 0x00003F00)>>8);
  temp = wordcount;
  temp += (memberidx<<12);
  temp += (module->address.vme.crate<<18);
  *outptr++ = temp;
  
  // read the data
  for(j=0;j<wordcount;j++) {
    res = dev->read_a32d32(dev, module->address.vme.addr + 0x0000, outptr);
    if(res==4) {
      outptr++;
    } else {
      *outptr++ = res;
      return(0);
    }
  }

  // read the EOB dataword
  res = dev->read_a32d32(dev, module->address.vme.addr + 0x0000, &help);
  if(res!=4) return(0);

  return(1);	
}

int SIS3801readout(ml_entry * module,int memberidx)
{
  int res;
  int wordcount;
  int temp;
  int help;
  int j;
  
  // write header word for this Scaler unit
  *outptr++ = SIS_3801;
  
  for (j=0;j<32;j++) {
    res = dev->read_a32d32(dev, module->address.vme.addr + 0x100, outptr);
    outptr++;
  }
  
  return(1);	
}
