/*   the code provides serial sequence of pulses on a selected output of VPG 517 */
/*   At the moment it may contains extra parameters, for example: */
/*     - if we always use S[0] register, we don't need p[2] */
/*     - if we can readout S[6] without distorbing readout - don't need p[6] */
/*   If we always works with S[0] register - only 8 bits maybe handled, */
/*   So, corresponding check must be provided. */
/*   Bit counts from ZERO to 31 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: VPGbitTest.c,v 1.4 2017/10/09 21:25:38 wuestner Exp $";

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/unixvme/vme.h"

extern Modlist *modullist;
extern int *memberlist;
 
typedef enum {Narg=0, Id, Addr, Bit, Nbits, Data, Exmp} ArgIndex;

RCS_REGISTER(cvsid, "procs/rcard")

/*
 * p[0]: Narg==6
 * p[1]: Id,  index of VPG 517 in modulelist
 * p[2]: Addr, relative address in   VPG 517
 * p[3]: Bit, number of of output bit which will be serial output
 * p[4]: Nbits, number of bits to be loaded in serie from data word
 * p[5]: Data word to be converted in serial output,  LSB comes first 
 * p[6]: Exmp, contains all other output const bit values for the serie
 */
plerrcode proc_VPGbitTest(ems_u32 *p) {
  ml_entry * module;
  struct vme_dev* dev;
  int Obit, mask0, mask1, i, d;

  module = ModulEnt(p[Id]);    
  if(module->modultype != VPG517) return(plErr_ArgRange);
  dev = module->address.vme.dev;
  Obit = 1 << p[Bit];
  mask0 = p[Exmp] & (~Obit); // if 0 in output
  mask1 = p[Exmp] | Obit;    // if 1 in output
  for(i=0; i<p[Nbits]; i++) {
    d = (p[Data] & 1) ?  mask1 :  mask0;
    p[Data] = p[Data] >> 1;
    dev->write_a32d32(dev, module->address.vme.addr + p[Addr], d);
/*     here must be command to access VPG 517 and clock */
  }
  return(plOK);
}

/*
 * p[0]: Narg==6
 * p[1]: Id,  index of VPG 517 in modulelist
 * p[2]: Addr, relative address in   VPG 517
 * p[3]: Bit, number of of output bit which will be serial output
 * p[4]: Nbits, number of bits to be loaded in serie from data word
 * p[5]: Data word to be converted in serial output,  LSB comes first 
 * p[6]: Exmp, contains all other output const bit values for the serie
 */
plerrcode test_proc_VPGbitTest(ems_u32 *p) {
  if(p[0] != 6)  return(plErr_ArgNum);
  if(p[Bit] < 0 || p[Bit] > 31) return(plErr_ArgRange);
  if(p[Nbits] < 0 || p[Nbits] > 31) return(plErr_ArgRange);
  wirbrauchen = 0;
  return(plOK);
}

char name_proc_VPGbitTest[] = "VPGbitTest";
int  ver_proc_VPGbitTest  = 1;








