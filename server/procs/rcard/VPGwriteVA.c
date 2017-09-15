static const char* cvsid __attribute__((unused))=
    "$ZEL: VPGwriteVA.c,v 1.4 2011/04/06 20:30:34 wuestner Exp $";

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

#include "VPGbits.h" // contains A.Mussgill's definitions
unsigned int actSYNC(unsigned int d);
unsigned int passSYNC(unsigned int d);

RCS_REGISTER(cvsid, "procs/rcard")

/***************************************************************************
 *   Bit setting is done for modifyed flat cable:
 *   - it is cross cable!
 *   - bits 6 <-> 7 on the side of VME module, made by S.Merzliakov, 
 *   - bit 7 -inversed poliarity
 * 
 * Defines for this configuration is:
 * #define OUTB   (1<<5)  // or (SHIFT_IN_TA>>16) from VPGbits.h
 * #define CLKta  (1<<6)  // or (CLK_TA>>16)
 * #define TESTon (1<<7)  // or (TEST_ON>>16)
 * #define StrobeON 0     
 * #define StrobeOF TESTon
 *
 * StrobeON and StrobeOF serves to set active level to 0.
 * it is evident, that change in polarity of TESTon will change values 
 * StrobeON <-> StrobeOF
 ***************************************************************************/
/* that is for S6 VPG517 register and new PCB boards */
#define OUTB   TEST_PULSE 
#define CLKta  CLK_TA

#define CLKdac CLK
#define OUTDAC SHIFT_IN

#define DACSYNCact  TEST_ON      
#define DACSYNCpass       0 

/* that is for S0 VPG517 register and old PCB Board */
/* 
 * #define OUTB   (SHIFT_IN_TA>>16)
 * #define CLKta  (CLK_TA>>16)
 * #define TESTon (TEST_ON>>16)
*/

extern int wirbrauchen;
extern Modlist *modullist;
extern int *memberlist;
extern int *outptr; 
typedef enum {Narg=0, Id, Nbits, Data, Exmp} ArgIndex;

/****************************************************************************
 * this function writes a pointed number of bits from 32 bit word 
 * throw 0xffe4 Register on SHIFT_IN_TA output for VA32TA2 chips loading 
 * first we write the LSB bit
 * all bit inversions, and changing bit order and meanning MUST be 
 * done in client call
 ****************************************************************************/


/*
 * p[0]: Narg==4
 * p[1]: Id,  index of VPG 517 in modulelist
 * p[2]: Nbits, number of bits to be loaded in serie from data word
 * p[3]: Data word to be converted in serial output,  LSB comes first 
 * p[4]: Exmp, contains all other output const bit values for the serie
 */
plerrcode proc_VPGwriteVA(ems_u32 *p) {
/*  static const unsigned int Reg0=0x1FFE4;
*/
  static const unsigned int Reg0=0x1fffc;
  ml_entry * module;
  struct vme_dev* dev;
  unsigned int RegAddr;
  unsigned int v = p[Data];
  unsigned int addr  =  p[Exmp];
  unsigned int mask0 = (p[Exmp] & (~OUTB));  // if 0 in output
  unsigned int mask1 = (p[Exmp] | OUTB); // if 1 in output
  int i, d;

  module = ModulEnt(p[Id]);    
  if(module->modultype != VPG517) return(plErr_ArgRange);
  dev = module->address.vme.dev;
  RegAddr = module->address.vme.addr + Reg0;

  dev->write_a32d32(dev, RegAddr, addr);       /* chip select */
  for(i=0; i<p[Nbits]; i++) {
    d = (v & 1) ?  mask1 :  mask0;
    dev->write_a32d32(dev, RegAddr, d);
    dev->write_a32d32(dev, RegAddr, d | CLKta);
    dev->write_a32d32(dev, RegAddr, d );    
    v = v >> 1;
  }
  dev->write_a32d32(dev, RegAddr, addr);       /* for the last data bit */
  return(plOK);
}

/*
 * p[0]: Narg==4
 * p[1]: Id,  index of VPG 517 in modulelist
 * p[2]: Nbits, number of bits to be loaded in serie from data word
 * p[3]: Data word to be converted in serial output,  LSB comes first 
 * p[4]: Exmp, contains all other output const bit values for the serie
 */
plerrcode test_proc_VPGwriteVA(ems_u32 *p) {
  if(p[0] != 4)  return(plErr_ArgNum);
  if(p[Nbits] <= 0 || p[Nbits] > 32) return(plErr_ArgRange);
  wirbrauchen = 0;
  return(plOK);
}

char name_proc_VPGwriteVA[] = "VPGwriteVA";
int  ver_proc_VPGwriteVA  = 1;

/****************************************************************************
 * This function writes a 32 or first 16 bits from 32 bit word 
 * throw 0xffe4 VPG 517 register on output bit OUTB for AD5328 chip loading 
 * Writing done by two portions of 16 bits if Nbits = 2, LSB bit loaded first.
 * All bit inversions, and changes in  bit order, and bit assignments for data word
 * MUST be done in client call.
 * Assignments for  CLOCK_TA, TEST_ON, and bit serial number for data output
 * as an active level of TEST_ON signal is done by #define in this file, see above. 
 * In p[Exmp] TEST_ON must be low!!!
 ****************************************************************************/

/*
 * p[0]: Narg==4
 * p[1]: Id,  index of VPG 517 in modulelist
 * p[2]: Data word to be converted in serial output,  LSB comes first 
 * p[3]: Nbits, number of write operations : 1 or 2 only by 16 bits
 * p[4]: Exmp, contains all other output const bit values for the serie
 */
unsigned int actSYNC(unsigned int d) {
  return (DACSYNCact) ? (d | TEST_ON) : (d & (~(TEST_ON)));
}
unsigned int passSYNC(unsigned int d) {
  return (DACSYNCact) ? (d & (~(TEST_ON))) : (d | TEST_ON);
}

plerrcode proc_VPGwriteAD(ems_u32 *p) {
  int i, j, d;
  static const unsigned int Reg0=0x1FFFC;
  ml_entry * module;
  struct vme_dev* dev;
  unsigned int RegAddr;
  unsigned int v = p[Data];

  int addr  = passSYNC(p[Exmp]) ;
  int waddr = actSYNC(p[Exmp]);         // Start write strobe
  unsigned int mask0 = waddr & (~OUTDAC);  // if 0 in output
  unsigned int mask1 = waddr | OUTDAC;     // if 1 in output
 
  module = ModulEnt(p[Id]);    
  if(module->modultype != VPG517) return(plErr_ArgRange);
  dev = module->address.vme.dev;
  RegAddr = module->address.vme.addr + Reg0;


  dev->write_a32d32(dev, RegAddr, addr);        /* chip select */
  for(j=0; j<p[Nbits]; j++) {
    dev->write_a32d32(dev, RegAddr, waddr);       // address + write strobe
    for(i=0; i<16; i++) {
      d = (v & 1) ?  mask1 :  mask0;
      dev->write_a32d32(dev, RegAddr, d);
      dev->write_a32d32(dev, RegAddr, d | CLKdac );
      dev->write_a32d32(dev, RegAddr, d); 
      v = v >> 1;
    }
    dev->write_a32d32(dev, RegAddr, waddr);  // keep strobe
    dev->write_a32d32(dev, RegAddr, addr);         // Clear strobe, write
  }
  dev->write_a32d32(dev, RegAddr, addr);       /* complete everything */
                                               /* chip still selected */
  return(plOK);
}

/*
 * p[0]: Narg==4
 * p[1]: Id,  index of VPG 517 in modulelist
 * p[2]: Nbits, number of bits to be loaded in serie from data word,1 ->16, 2->32 bits
 * p[3]: Data word to be converted in serial output,  LSB comes first 
 * p[4]: Exmp, contains all other output const bit values for the serie
 */
plerrcode test_proc_VPGwriteAD(ems_u32 *p) {
  if(p[0] != 4)  return(plErr_ArgNum);
  if(p[Nbits] !=1 && p[Nbits] != 2) return(plErr_ArgRange);
  wirbrauchen = 0;
  return(plOK);
}

char name_proc_VPGwriteAD[] = "VPGwriteAD";
int  ver_proc_VPGwriteAD  = 1;













