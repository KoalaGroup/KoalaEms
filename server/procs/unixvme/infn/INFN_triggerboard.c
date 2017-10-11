#include <sconf.h>
#include <stdlib.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <modultypes.h>
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/unixvme/vme.h"
#include "../../../lowlevel/devices.h"


extern ems_u32* outptr;
extern int* memberlist;
extern int wirbrauchen;


//*******ADDRESSES***********
const int FCSR             = 0x0;         // PAX_CONTROLLER -> "Functional Colntrol Status Register" register address
const int CMCU             = 0x10;        // PAX_CONTROLLER -> "Command to MCU" register address
const int DMCU             = 0x14;        // PAX_CONTROLLER -> "Data to MCU" register address
//***************************

//*******COMMANDS************
const int CTRL_WRITE       = 0x80000000;  // PAX_CONTROLLER -> write command
const int CTRL_READ        = 0x90000000;  // PAX_CONTROLLER -> read command
//***************************

//**********MASKS************
const int MOD_ADDR_MASK    = 0xf000000;   
const int DATA_MASK        = 0xffff;      
const int TEST_MASK        = 0xffffff;    
//***************************

//*******BITS SHIFTS*********
const int BOARD_ADR_SHIFT  = 24;          // pax commands   -> board's address bit shift 
const int REG_ADR_SHIFT    = 16;          // pax commands   -> registers' address bit shift
const int D_READY_SHIFT    = 31;          // pax commands   -> data ready flag's bit shift
const int MCU_BSY_SHIFT    = 30;          // pax commands   -> MCU busy flag's bit shift
const int NIOS_TR_SHIFT    = 27;          // pax commands   -> NIOS transaction flag's bit shift
const int DT_LINE_SHIFT    = 26;          // pax commands   -> PAX_SPI data line flag's bit shift
//***************************

//****TEST PARAMETERS********
const int TB_TEST_W        = 0x154fed;  
const int PS_TEST_W        = 0x10fede;  
const int TB_TEST_R        = 0x150000;  
const int PS_TEST_R        = 0x100000;  
//***************************

/*****************************************************************************/
static plerrcode
test_vmeparm(ems_u32* p, int n)
{
  if (p[0]!=n) return(plErr_ArgNum);
  if (!valid_module(p[1], modul_vme)) return plErr_ArgRange;
  wirbrauchen = 1;
  return plOK;
}
/*****************************************************************************/


/*
 *****************************************************************************
 *
 * p[0]: argcount==4
 * p[1]: pax controller address
 * p[2]: pax module address
 * p[3]: offset
 * p[4]: data to write
 *
 */

plerrcode proc_INFN_areg(ems_u32* p)
{

  int res;
  int dataReady;
  int mcuBusy;
  int niosTrans;
  int paxSpi;
  int dataCounter=0;
  ml_entry* module=ModulEnt(p[1]);
  struct vme_dev* dev=module->address.vme.dev;

  if (p[0]==3)
    { 
      res=dev->write_a32d32(dev, module->address.vme.addr+CMCU, (CTRL_READ | p[2]<<BOARD_ADR_SHIFT | p[3]<<REG_ADR_SHIFT));
      
      do
	{
	  int i = 0;
	  do
	    {
	      i++;
	    }
	  while (i<10000);
	  res=dev->read_a32d32(dev, module->address.vme.addr+FCSR, outptr);
	  dataReady = (*outptr>>D_READY_SHIFT) & 1;
	  mcuBusy   = (*outptr>>MCU_BSY_SHIFT) & 1;
	  niosTrans = (*outptr>>NIOS_TR_SHIFT) & 1;
	  paxSpi    = (*outptr>>DT_LINE_SHIFT) & 1;
	  dataCounter++;
	} while (((dataReady == 0)||(mcuBusy == 1)||(niosTrans == 1)||(paxSpi == 0))&&(dataCounter<1000000));
    }

  else if (p[0]==4)
    {
      res=dev->write_a32d32(dev, module->address.vme.addr+CMCU, (CTRL_WRITE | p[2]<<BOARD_ADR_SHIFT | p[3]<<REG_ADR_SHIFT | p[4]));
      
      do
	{
	  int i = 0;
	  do
	    {
	      i++;
	    }
	  while (i<10000);
	  res=dev->read_a32d32(dev, module->address.vme.addr+FCSR, outptr);
	  dataReady = (*outptr>>D_READY_SHIFT) & 1;
	  mcuBusy   = (*outptr>>MCU_BSY_SHIFT) & 1;
	  niosTrans = (*outptr>>NIOS_TR_SHIFT) & 1;
	  paxSpi    = (*outptr>>DT_LINE_SHIFT) & 1;
	  dataCounter++;
	} while (((dataReady == 0)||(mcuBusy == 1)||(niosTrans == 1)||(paxSpi == 0))&&(dataCounter<1000000)); 
    }
  
  else return(plErr_ArgRange);

  if (res==4) {
    outptr++;
    return(plOK);
  } else {
    *outptr++ = res;
    return(plErr_System);
  }
  
}

plerrcode test_proc_INFN_areg(ems_u32* p)
{
  
  if (p[0]==3)
    {
      return test_vmeparm(p, 3);
    }
  else if (p[0]==4)
    {
      return test_vmeparm(p, 4);
    }
  else return(plErr_System);

}

char name_proc_INFN_areg[] = "INFN_areg";
int ver_proc_INFN_areg = 1;


/*
 *****************************************************************************
 *
 * p[0]      : argcount>=2 / argcount<=47
 * p[1]      : pax controller address
 * p[2]      : module type [0->TB; 1->PS]
 * p[3]..p[n]: PAX_SPI command 
 *
 */

plerrcode proc_INFN_load(ems_u32* p)
{

  //***********************
  //****IN PROGRESS********
  //***********************

  int res=0;
  int dataReady;
  int mcuBusy;
  int niosTrans;
  int paxSpi;
  int Counter=0;
  int dataCounter=0;
  int upLimit=0;
  int readTestRegister=0;
  int writeTestRegister=0;
  int keepTestRegister=0;
  int keepValue=0;
  int readBackValue=0;
  
  ml_entry* module=ModulEnt(p[1]);
  struct vme_dev* dev=module->address.vme.dev;
  upLimit = p[0] + 2;
  if (p[2]) 
    {
      readTestRegister= CTRL_READ | (p[3]&MOD_ADDR_MASK) | PS_TEST_R;
      writeTestRegister= CTRL_WRITE | (p[3]&MOD_ADDR_MASK) | PS_TEST_W;
    }
  else
    {
      readTestRegister= CTRL_READ | (p[3]&MOD_ADDR_MASK) | TB_TEST_R;
      writeTestRegister= CTRL_WRITE | (p[3]&MOD_ADDR_MASK) | TB_TEST_W;
    }

  res=dev->write_a32d32(dev, module->address.vme.addr+CMCU, readTestRegister);
  do
    {
      int i = 0;
      do
	{
	  i++;
	}
      while (i<10000);
      res=dev->read_a32d32(dev, module->address.vme.addr+FCSR, outptr);
      dataReady = (*outptr>>D_READY_SHIFT) & 1;
      mcuBusy   = (*outptr>>MCU_BSY_SHIFT) & 1;
      niosTrans = (*outptr>>NIOS_TR_SHIFT) & 1;
      paxSpi    = (*outptr>>DT_LINE_SHIFT) & 1;
      dataCounter++;
    } while (((dataReady == 0)||(mcuBusy == 1)||(niosTrans == 1)||(paxSpi == 0))&&(dataCounter<1000000));
  keepValue=(*outptr & TEST_MASK);
  keepTestRegister= CTRL_WRITE | (p[3]&MOD_ADDR_MASK) | keepValue;
  res=dev->write_a32d32(dev, module->address.vme.addr+CMCU, writeTestRegister);
  do
    {
      int i = 0;
      do
	{
	  i++;
	}
      while (i<10000);
      res=dev->read_a32d32(dev, module->address.vme.addr+FCSR, outptr);
      dataReady = (*outptr>>D_READY_SHIFT) & 1;
      mcuBusy   = (*outptr>>MCU_BSY_SHIFT) & 1;
      niosTrans = (*outptr>>NIOS_TR_SHIFT) & 1;
      paxSpi    = (*outptr>>DT_LINE_SHIFT) & 1;
      dataCounter++;
    } while (((dataReady == 0)||(mcuBusy == 1)||(niosTrans == 1)||(paxSpi == 0))&&(dataCounter<1000000));
  readBackValue=(*outptr & DATA_MASK);
  if(!(readBackValue==(writeTestRegister&DATA_MASK))) 
    {
      return(plErr_ArgRange);
    }
  else
    {
      res=dev->write_a32d32(dev, module->address.vme.addr+CMCU, keepTestRegister);
      for (Counter=3; Counter<upLimit; Counter++)
	{
	  res=dev->write_a32d32(dev, module->address.vme.addr+CMCU, p[Counter]);
	  
	  do
	    {
	      int i = 0;
	      do
		{
		  i++;
		}
	      while (i<10000);
	      res=dev->read_a32d32(dev, module->address.vme.addr+FCSR, outptr);
	      dataReady = (*outptr>>D_READY_SHIFT) & 1;
	      mcuBusy   = (*outptr>>MCU_BSY_SHIFT) & 1;
	      niosTrans = (*outptr>>NIOS_TR_SHIFT) & 1;
	      paxSpi    = (*outptr>>DT_LINE_SHIFT) & 1;
	      dataCounter++;
	    } while (((dataReady == 0)||(mcuBusy == 1)||(niosTrans == 1)||(paxSpi == 0))&&(dataCounter<1000000));
	}
      
      if (res==4) {
	outptr++;
	return(plOK);
      } else {
	*outptr++ = res;
	return(plErr_System);
      }
    }
}

plerrcode test_proc_INFN_load(ems_u32* p)
{
  
/*   if ((p[0]<2) || (p[0]>48)) return plErr_ArgNum; */
  if (p[0]<2) return plErr_ArgNum;

  else return plOK; 

}

char name_proc_INFN_load[] = "INFN_load";
int ver_proc_INFN_load = 1;
