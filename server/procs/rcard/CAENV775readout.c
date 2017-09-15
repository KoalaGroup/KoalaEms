/*
 * procs/rcard/CAENV775readout.c
 * created before 2001-Aug-18 A. Mussgiller
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: CAENV775readout.c,v 1.7 2011/04/06 20:30:34 wuestner Exp $";

#include <stdio.h>

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/unixvme/vme.h"

#include "CAENV775readout.h"

extern ems_u32* outptr;

struct vme_dev* dev;

RCS_REGISTER(cvsid, "procs/rcard")

static void ReadRegsCAENV775(ml_entry * module){
  int res;
  ems_u16 data;
  int i;
  int adrr[17] = {0x1006, 0x1008, 0x100a, 0x100e, 0x1010, 0x1020, 0x1022,
                0x1024, 0x1026, 0x102c, 0x102e, 0x1032, 0x103c, 0x1060,
                0x1070, 0x1072, 0x1080}; 
  dev = module->address.vme.dev;
  for(i=0; i<17; i++) {
    res = dev->read_a32d16(dev, module->address.vme.addr+adrr[i], &data); 
    printf("####### ReadRegsCAENV775(): addr=%x res=%1d  data=%x\n",
	   adrr[i], res, data);

  }
}

static int GetStatus(ml_entry * module) {
  int res;
  ems_u16 mask1 = 0x3;
  ems_u16 mask2 = 0x4; /* busy bit */
  ems_u16 status;
  int j; 
  int n=10;
  int delay=0;
  int tmp;

  dev = module->address.vme.dev;
  for(j=0; j<n; j++) {
    res = dev->read_a32d16(dev, module->address.vme.addr + 0x100e, &status);
    if((res == 2) && 
       ((status & mask2) == 0) &&
       ((status & mask1) == mask1)) return 1;
    {
      delay=0;
      while(delay < 1000) {
	delay++;
	tmp = delay - j;
      }
    }
  }
  if(res != 2) {
    printf("####### ReadStatusCAENV775(): addr=0x100E invalid read: res=%1d  ", res); 
  }
  printf("invalid status: 0x%1x\n", status);
  return 0;
}


/* static void CAENV775clear(ml_entry * module) { */
/*   int res; */
/*   dev = module->address.vme.dev; */
/*   res = dev->write_a32d16(dev, module->address.vme.addr + 0x1032, 4); */
/*   res = dev->write_a32d16(dev, module->address.vme.addr + 0x1034, 4); */
/* } */



int CAENV775readout(ml_entry * module,int memberidx)
{
  int res;
  int wordcount;
  int temp;
  ems_u32 help;
  int j;
  short int status=0;     
  static int ncall=10; /* to provide debug prints */

  memberidx += 1;      /* increase by 1 for off-line handling */
  if(ncall>0) ncall--;
  dev = module->address.vme.dev;
  if(ncall > 8) {
    printf("####### CAENV775readout(): addr=%x\n", module->address.vme.addr);
    ReadRegsCAENV775(module);
  }
  status = GetStatus(module);
  if(status == 0) {
    int  header;
    int  wordcount;
    int  data;

    header = 0x02000000;
    wordcount = 1;
    data = 0x06000000;
    
    *outptr++ = CAEN_V775;
    *outptr++ = header | (wordcount << 8);
    *outptr++ = data;
    printf("####### CAENV775readout(): dummy event\n");
    return(1);
  }
  // write header word for this TDC unit

  res = dev->read_a32d32(dev, module->address.vme.addr + 0x0000, &help); // read header
  if(ncall > 0) {
    printf("####### CAENV775readout(): res=%1d header=%x\n", res, help);
  }
  if(res!=4) {
    int  header;
    int  wordcount;
    int  data;

    header = 0x02000000;
    wordcount = 1;
    data = 1 << 14;

    *outptr++ = CAEN_V775;
    *outptr++ = header | (wordcount << 8);
    *outptr++ = data;
    printf("####### CAENV775readout(): dummy event\n");
    return(1);
  }
  
  if ((help & 0x02000000) && !(help & 0x04000000)) {
    if(ncall > 0) {
      printf("####### CAENV775readout(): header= detected\n");
    }
    *outptr++ = CAEN_V775;
    wordcount = (((int)help & 0x00003F00)>>8);
    temp = wordcount;
    temp += (memberidx<<12);
    temp += (module->address.vme.crate<<18);
    if(ncall > 0) {
      printf("####### CAENV775readout(): header=%x wordcount=%1d memberidx=%1d crate=%1d\n", 
	     help, wordcount, memberidx, module->address.vme.crate);
    }

    *outptr++ = temp;
  
    // read the data
    for(j=0;j<wordcount;j++) {
      res = dev->read_a32d32(dev, module->address.vme.addr + 0x0000, outptr);
      if(ncall > 0) {
	printf("####### CAENV775readout(): data=0x%x  res=%1d\n", *outptr, res);
      }
      if(res==4) {
        outptr++;
      } else {
        *outptr++ = 1 << 14;
      }
    }

    // read the EOB dataword
    res = dev->read_a32d32(dev, module->address.vme.addr + 0x0000, &help);
    if(res!=4) {
      res = dev->read_a32d32(dev, module->address.vme.addr + 0x0000, &help);
      return(1); /* must be 0 */
    }
    if(ncall > 0) {
      printf("####### CAENV775readout(): EOB=%x ev_count=%1d\n",
	     help, help&0xffffff);
      res = dev->read_a32d32(dev, module->address.vme.addr + 0x0000, &help);
      printf("####### CAENV775readout():next 1 after EOB: %x  -  b26-24=%x  ch?=%1d v?=%1d\n",
	     help, (help>>24)&0x7, (help>>8)&0x3f, help&0xfff);
      res = dev->read_a32d32(dev, module->address.vme.addr + 0x0000, &help);
      printf("####### CAENV775readout():next 2 after EOB: %x  - b26-24=%x  ch?=%1d v?=%1d\n",
	     help, (help>>24)&0x7, (help>>8)&0x3f, help&0xfff);
      res = dev->read_a32d32(dev, module->address.vme.addr + 0x0000, &help);
      printf("####### CAENV775readout():next 3 after EOB: %x  -  b26-24=%x  ch?=%1d v?=%1d\n",
	     help, (help>>24)&0x7, (help>>8)&0x3f, help&0xfff);
    }

    return(1);
  }
  else {
    int j;
    printf("####### CAENV775readout(): not Header!!!\n");
    for(j=0; j<32; j++) {
      res = dev->read_a32d32(dev, module->address.vme.addr + 0x0000, &help);
      if((help & 0x06000000) == 0x06000000) break;
    }    
  }

/*   if ((help & 0x04000000) && (help & 0x02000000)) { */
/*     printf("CAENV775readout: invalid datum 0x%08x\n",help); */
/*     temp = 0; */
/*     temp += (memberidx<<12); */
/*     temp += (module->address.vme.crate<<18); */
/*     *outptr++ = temp; */
/*     return(1); */
/*   } */
  return(1);	
}
