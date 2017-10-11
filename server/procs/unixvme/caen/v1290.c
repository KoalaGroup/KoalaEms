/************************************************************************
 *
 * S.Trusov: temporary for spectator tests 
 * trigger matching mode, D32 output buffer readout
 * check of valid data by Data Ready bit of Status Register
 ************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: v1290.c,v 1.8 2015/04/06 21:33:34 wuestner Exp $";

#include <stdio.h>
#include <stdlib.h>

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/unixvme/vme.h"

#include "v1290.h"

extern int wirbrauchen;
extern Modlist *modullist;
extern int *memberlist;
extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/unixvme/caen")

struct vme_dev* dev;

static int WaitDelay(int cycles);
static int WriteOpcode(ml_entry * module, ems_u16 data);
static int SetWindow(ml_entry * module, ems_u16* data);

int v1290readout(ml_entry * module,int memberidx)
{
  int wordcount;
  ems_u32  temp;
  int j;
  ems_u16   status;
  ems_u32* ptr_header;
  dev = module->address.vme.dev;

  /* increment index */
  memberidx += 1;

  // check is data ready
  dev->read_a32d16(dev, module->address.vme.addr + V1290_STATUS_REG, &status);
  if(!(status & 0x1)) {
/*     printf("***** v1290readout: id=%1d not ready, status=0x%1x\n", */
/* 	   memberidx-1, status); */
    return 1;
  }
  /* write header word for the TDC unit */
  *outptr++ = CAEN_V1290;   /* WORD 0 */
  ptr_header = &(*outptr);      /* reference to the module title word */
  wordcount = 0;            /* will be filled later, if needed */
  temp = wordcount;         /* number of data and tdc header words, must be less < 2048 */
  temp += (memberidx<<12);
  temp += (module->address.vme.crate<<18);
  *outptr++ = temp;         /* WORD 1 */

  for(j=0; j<1024; j++) {    
    dev->read_a32d32(dev, module->address.vme.addr + 4*j, &temp);
    if(((temp >> V1290_WTYPE_SHIFT) & 0x1f) == V1290_FILLER) {
      continue;
    }
    *outptr++ = temp;
    wordcount++;

    if(((temp >> V1290_WTYPE_SHIFT) & 0x1f) == V1290_GLOBAL_TRAILER) {
      break;
    }
  } 
  *ptr_header = (*ptr_header) | (wordcount & 0x3ff);
  return 1;
}

int v1290readoutDSP(ml_entry * module,int memberidx)
{
  int wordcount;
  ems_u32  temp;
  int j;
  ems_u16   status;
  ems_u32* ptr_header;
  dev = module->address.vme.dev;

  /* increment index */
  memberidx += 1;

  // check is data ready
  dev->read_a32d16(dev, module->address.vme.addr + V1290_STATUS_REG, &status);
  if(!(status & 0x1)) {
/*     printf("***** v1290readout: id=%1d not ready, status=0x%1x\n", */
/* 	   memberidx-1, status); */
    return 1;
  }
  /* write header word for the TDC unit */
/*   *outptr++ = CAEN_V1290;   /\* WORD 0 *\/ */
  ptr_header = &(*outptr);      /* reference to the module title word */
  wordcount = 0;            /* will be filled later, if needed */
  temp = wordcount;         /* number of data and tdc header words, must be less < 2048 */
  temp += (memberidx<<12);
  temp += (module->address.vme.crate<<18);
  *outptr++ = temp;         /* WORD 1 */

  for(j=0; j<1024; j++) {    
    dev->read_a32d32(dev, module->address.vme.addr + 4*j, &temp);
    if(((temp >> V1290_WTYPE_SHIFT) & 0x1f) == V1290_FILLER) {
      continue;
    }
    *outptr++ = temp;
    wordcount++;

    if(((temp >> V1290_WTYPE_SHIFT) & 0x1f) == V1290_GLOBAL_TRAILER) {
      break;
    }
  } 
  *ptr_header = (*ptr_header) | (wordcount & 0x3ff);
  return 1;
}

char name_proc_v1290roWithDSP[] = "v1290roWithDSP";
int  ver_proc_v1290roWithDSP = 1;

plerrcode proc_v1290roWithDSP(ems_u32 *p) {
  int modanz = *memberlist;
  int i;
  ml_entry * module;

  for(i=1;i<=modanz;i++){
    module = ModulEnt(i); 
    /* in a module readout procedure must be idx=memberlist[i]+1; !!*/
    if(module->modultype == CAEN_V1290) {
      if(!v1290readoutDSP(module, memberlist[i])) return(plErr_System);
      v1290clear(module);
    }
  }
  return plOK;
}

plerrcode test_proc_v1290roWithDSP(ems_u32 *p) {
  if(p[0] != 0) return(plErr_ArgNum);
  return(plOK);  
}


void v1290clear(ml_entry * module)
{
  dev = module->address.vme.dev;
  dev->write_a32d16(dev, module->address.vme.addr + V1290_CLEAR_REG, 0x0);
}

/*************************************************
 * Low level programming of  CAEN V1290 TDC
 *
*/
static int WaitDelay(int cycles) {
  int i=0;
  int j;
  double c=0;
  double sum;
  cycles++;
  for(i=0; i<cycles; i++) {
    c = c +1.;
    for(j=0; j<cycles; j++) {
      sum = sum + c*c;
    }
  }
  return 1;
}

static int WriteOpcode(ml_entry * module, ems_u16 data) {
  int count=100;
  int res;
  int i;
  ems_u16 tmp=0;
  ems_u16 dummy;
  dev = module->address.vme.dev;
  for(i=0; i<count; i++) {
    res=dev->read_a32d16(dev, module->address.vme.addr+V1290_MicroHandshake, &tmp);
    if(res != 2) {
      printf("***** CAEN V1290 WriteOpcode: failed read MicroHandshake register\n");
      return 0;
    }
    if((tmp & V1290_WRITE_OK) == V1290_WRITE_OK) {
      break;
    }
    else {
      if((tmp & V1290_READ_OK) == V1290_READ_OK) {
	res=dev->read_a32d16(dev, module->address.vme.addr + V1290_MicroReg, &dummy);
	if(res != 2) {
	  printf("*****  CAEN V1290 WriteOpcode: failed read MicroHandshake register\n");
	  return 0;
	}
      }
    }    
    WaitDelay(10000);
  }
  if((tmp & V1290_WRITE_OK) == V1290_WRITE_OK) {
    res=dev->write_a32d16(dev, module->address.vme.addr + V1290_MicroReg, data);
    if(res !=2) {
      printf("***** CAEN V1290 WriteOpcode: failed write data\n");
      return 0;
    }
    return 1;
  }
  printf("*****  CAEN V1290 WriteOpcode: failed wait WRITE_OK\n");
  return 0;
}


/* Sets TriggerMatching Mode and time window
 * data contains: 
 * 0: Window Width, 
 * 1: Window Offset, 
 * 2: Extra Search Margin, 
 * 3: Reject Margin 
 * 4: 1 - Enable trigger subtraction time, 0 -disable subtraction 
*/
static int SetWindow(ml_entry * module, ems_u16* data) {
  dev = module->address.vme.dev;
  /* Set Trigger Match Mode */ 
  if(WriteOpcode(module, V1290_OC_TRG_MATCH) == 0) return 0;
  /* Window Width  */
  if(WriteOpcode(module, V1290_OC_WIN_WIDTH) == 0) return 0;
  if(WriteOpcode(module, data[0]) == 0) return 0;
  /* Window Offset  */
  if(WriteOpcode(module, V1290_OC_WIN_OFFS) == 0) return 0;
  if(WriteOpcode(module, data[1]) == 0) return 0;
  /* Window Extra Search  */
  if(WriteOpcode(module, V1290_OC_SW_MARGIN) == 0) return 0;
  if(WriteOpcode(module, data[2]) == 0) return 0;
  /* Window Reject Margin  */
  if(WriteOpcode(module, V1290_OC_REJ_MARGIN) == 0) return 0;
  if(WriteOpcode(module, data[3]) == 0) return 0;
  /* Enable subtraction of trigger time */
  if(data[4] != 0) {
    if(WriteOpcode(module, V1290_OC_EN_SUB_TRG) == 0) return 0;
  }
  else {
    if(WriteOpcode(module, V1290_OC_DIS_SUB_TRG) == 0) return 0;
  }
  return 1;
}

/* data contains: 
 * 0  Narg: min 3, max=6
 * 1  module ID
 * 2: Window Width, 25 ns cycles 
 * 3: Window Offset,  25 ns cycles ,      [-2048, 40-(Window Width)?-(Extra Search Margin)]
 * 4: Extra Search Margin, default 0x08,  [0, 50(4095 possible)]
 * 5: Reject Margin,       default 0x04,  [1(0 possible), 4095] 
 * 6: Enable trigger subtraction time, default 0x1 = enabled
*/

plerrcode proc_v1290SetWindow(ems_u32 *p) {
  ems_u16 arg[5];
  ml_entry * module;
  int i;

  module = ModulEnt(p[1]);    
  arg[0] = p[2] &0xfff;
  arg[1] = p[3] &0xfff;
  arg[2] = 0x8;
  arg[3] = 0x4;
  arg[4] = 0x1;
  for(i=2; i<=p[0]; i++) {
    arg[i-2] = p[i] & 0xfff;
  }
  if(SetWindow(module, arg) == 0) {
  printf("***** v1290SetWindow failed\n");
    return(plErr_Busy);
  }
  return(plOK);  
}

plerrcode test_proc_v1290SetWindow(ems_u32 *p) {
  if(p[0] < 3) return(plErr_ArgNum);
  if (!valid_module(p[1], modul_vme)) return plErr_ArgRange;
  if(ModulEnt(p[1])->modultype != CAEN_V1290) return(plErr_BadModTyp);
  if(p[2] < 1 || p[2] > 4095) return plErr_ArgRange;
  if(p[0] > 3) {
    int a = p[3];
    if(a < -2048 || (a + p[2]) > 40) return plErr_ArgRange;
  }
  if(p[0] > 4) {
    int a = p[4];
    if(a < 0)    p[4] = 8;
    if(a > 50)  p[4] = 50;
  }
  if(p[0] > 5) {
    if(p[5] < 1)    p[5] = 1;
    if(p[5] > 4095) p[5] = 4;    
  }
  wirbrauchen = 1;
  return(plOK);
}

char name_proc_v1290SetWindow[] = "v1290SetWindow";
int  ver_proc_v1290SetWindow = 1;

plerrcode proc_v1290CheckHandshake(ems_u32 *p) {
  int res;
  ems_u16 tmp=0;
  ml_entry * module;
  module = ModulEnt(p[1]);    
  dev = module->address.vme.dev;
  res=dev->read_a32d16(dev, module->address.vme.addr+V1290_MicroHandshake, &tmp);
  printf("#### Handshake bits: 0x%1x  res=%1d res=0x%1x\n", tmp, res, (tmp & V1290_WRITE_OK));
  
  return(plOK);  
}

plerrcode test_proc_v1290CheckHandshake(ems_u32 *p) {
  if (!valid_module(p[1], modul_vme)) return plErr_ArgRange;
  if(ModulEnt(p[1])->modultype != CAEN_V1290) return(plErr_BadModTyp);  
  wirbrauchen = 1;
  return(plOK); 
}

char name_proc_v1290CheckHandshake[] = "v1290CheckHandshake";
int  ver_proc_v1290CheckHandshake = 1;

/* data contains: 
 * 0  Narg: min 1, max=2
 * 1  module ID
 * 2: leading/trailing edge, trailing=1, leading=2, both=3, 0 forbidden, default=2 
*/

plerrcode proc_v1290SetVHRedge(ems_u32 *p ) {
  ml_entry * module;
  ems_u16 data=0x2;
  module = ModulEnt(p[1]);    
  dev = module->address.vme.dev;
  if(p[0] > 1) data = p[2];
  /* Set edge mode */ 
  if(WriteOpcode(module, V1290_OC_EDGE_MODE) == 0) return (plErr_Busy);
  if(WriteOpcode(module, data) == 0) return (plErr_Busy);
  /* Set Very High Resolution */
  if(WriteOpcode(module, V1290_OC_EDGE_RES) == 0) return (plErr_Busy);
  if(WriteOpcode(module, V1290_EDGE_025) == 0) return (plErr_Busy);
  return  (plOK); 
}

plerrcode test_proc_v1290SetVHRedge(ems_u32 *p ) {
  if(p[0] < 1) return(plErr_ArgNum);
  if (!valid_module(p[1], modul_vme)) return plErr_ArgRange;
  if(ModulEnt(p[1])->modultype != CAEN_V1290) return(plErr_BadModTyp);
  if(p[0]>1 && (p[2] > V1290_BOTH_EDGE || p[2] == V1290_PAIR_EDGE))  return plErr_ArgRange;
  wirbrauchen = 1;
  return(plOK); 
}

char name_proc_v1290SetVHRedge[] = "v1290SetVHRedge";
int  ver_proc_v1290SetVHRedge = 1;

/* data contains: 
 * 0  Narg: min 1, max=2
 * 1  module ID
 * 2 0(or absent) - default cfg, 1 - user saved cfg, 
*/
plerrcode proc_v1290LoadCfg(ems_u32 *p ) {
  ml_entry * module;
  module = ModulEnt(p[1]);    
  dev = module->address.vme.dev;
  /* Load default configuration */ 
  if(p[0] >= 2 && (p[2] != 0)) {
    if(WriteOpcode(module, V1290_OC_LOAD_USER_CFG) == 0) return (plErr_Busy);
  }
  else {
    if(WriteOpcode(module, V1290_OC_LOAD_DEF_CFG) == 0) return (plErr_Busy);
  }
  return(plOK); 
}

plerrcode test_proc_v1290LoadCfg(ems_u32 *p ) {
  if(p[0] < 1) return(plErr_ArgNum);
  if (!valid_module(p[1], modul_vme)) return plErr_ArgRange;
  if(ModulEnt(p[1])->modultype != CAEN_V1290) return(plErr_BadModTyp);
  wirbrauchen = 1;
  return(plOK); 
}

char name_proc_v1290LoadCfg[] = "v1290LoadCfg";
int  ver_proc_v1290LoadCfg = 1;

/* Enable/Disable TDC chip header & Trailer in output buffer 
* data[0] = 0 - disable header/trailer, anything else - enable*/
static int EnableTDCHeader(ml_entry * module, ems_u16 data) {
  if(!module) return 0;
  if(data != 0) {
    if(WriteOpcode(module, V1290_OC_EN_HEAD) == 0) return 0;
  }
  else {
    if(WriteOpcode(module, V1290_OC_DIS_HEAD) == 0) return 0;
  }
  wirbrauchen = 1;
  return 1;
}

/* p[] contains: 
 * 0  Narg: min 1, max=2
 * 1(0)  module ID
 * 2(1)  TDC header/trailer enabled [0,1],             default=1 - enabled
 */
plerrcode proc_v1290EnableChipHeader(ems_u32 *p) {
  ml_entry * module;
  ems_u16 data;
  data = 0;
  module = ModulEnt(p[1]);
  if(p[0] > 1) data = (ems_u16)(p[2] & 0xffff);
  if(EnableTDCHeader(module, data) == 0) {
    return(plErr_Busy);
  }
  return  (plOK); 
}
plerrcode test_proc_v1290EnableChipHeader(ems_u32 *p ) {
  if(p[0] < 1) return(plErr_ArgNum);
  if (!valid_module(p[1], modul_vme)) return plErr_ArgRange;
  if(ModulEnt(p[1])->modultype != CAEN_V1290) return(plErr_BadModTyp);
  return  (plOK); 
}

char name_proc_v1290EnableChipHeader[] = "v1290EnableChipHeader";
int  ver_proc_v1290EnableChipHeader = 1;

/* Arguments:
 *  data[0] = [0, 0x1F] - channel number 
 *  data[1] = 0/1 disable/enable channel
*/
static int EnableChan(ml_entry * module, ems_u16* data) {
  if(!module) return 0;
  if(data[1] != 0) {
    if(WriteOpcode(module, V1290_OC_EN_CHAN | data[0]) == 0) return 0;
  }
  else {
    if(WriteOpcode(module, V1290_OC_DIS_CHAN | data[0]) == 0) return 0;
  }
  wirbrauchen = 1;
  return 1;
}

char name_proc_v1290EnableChan[] = "v1290EnableChan";
int  ver_proc_v1290EnableChan = 1;
/* p[] contains: 
 * 0  Narg: 3
 * 1(0)  module id
 * 2(1)  channel number
 * 3(2)  disabled/enabled (0/1)
 */
plerrcode proc_v1290EnableChan(ems_u32 *p) {
  ml_entry * module;
  ems_u16 data[2];

  module = ModulEnt(p[1]);

  data[0] = (ems_u16)(p[2] & V1290_MASK_32);
  data[1] = p[3];
  if(EnableChan(module, data) == 0) {
    printf("v1290EnableChan(%1d,%1d) failed\n", data[0], data[1]);
    return(plErr_Busy);
  }
  return  (plOK); 
}
plerrcode test_proc_v1290EnableChan(ems_u32 *p ) {
  
  if(p[0] != 3 ) return plErr_ArgNum;
  if (!valid_module(p[1], modul_vme)) return plErr_ArgRange;
  if(ModulEnt(p[1])->modultype != CAEN_V1290) return(plErr_BadModTyp);
  if(p[1] > 31) return plErr_ArgRange;
  return  (plOK); 
}

/* data contains: 
 * 0  Narg: min 1, max=2
 * 1  module ID
 * 2  TDC header/trailer enabled [0,1],             default=1 - enabled
 * 3  TDC error mark enabled, [0,1],                default=1 - enabled
 * 5  enable TDC bypass if global error,            default
*/



/* data contains: 
 * 0  Narg: min 1, max=2
 * 1  module ID
 * 2  TDC header/trailer enabled [0,1],             default=1 - enabled
 * 3  TDC error mark enabled, [0,1],                default=1 - enabled
 * 5  enable TDC bypass if global error,            default
*/
/*  plerrcode proc_SetDOutV1290(ems_u32 *p ) {  */
/*    int res;  */
/*    ems_u16 tmp=0;  */
/*    ml_entry * module;  */
/*    module = ModulEnt(p[1]);      */
/*    dev = module->address.vme.dev;  */
/*    /\\* *\\/   */
/*    if(WriteOpcode(module, V1290_OC_LOAD_DEF_CFG) == 0) return (plErr_Busy);  */
/*    return(plOK);   */
/*  }  */

/*  plerrcode test_proc_SetTDCOutV1290(ems_u32 *p ) {  */
/*    if(p[0] < 1) return(plErr_ArgNum);  */
/*    if (!valid_module(p[1], modul_vme)) return plErr_ArgRange;  */
/*    if(ModulEnt(p[1])->modultype != CAEN_V1290) return(plErr_BadModTyp);  */
/*    wirbrauchen = 1;  */
/*    return(plOK);   */
/*  }  */

/* char name_proc_SetDOutV1290[] = "SetDOutV1290";  */
/* int  ver_proc_SetDOutV1290 = 1;  */

