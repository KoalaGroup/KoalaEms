/********************************************************************
 * 2006/11/11  It was found that there is inconvinience 
 * in module indexing: in memberlist[] indexes of modules starts from 0,
 * but filter VertexVME expects indexes starting from 1!
 * So, I have decided to move this incrementation inside EACH module 
 * in order not to use different indexes in different readout procedure.
 * For example, call of CAENV550readout() must be the same from 
 * proc_RCardReadout(), and from proc_ReadV550().
 * IT IS IMPORTANT to check, what must be in:
 * proc_WriteBlock() and proc_WriteToFixAddr(). Check all DAQ software!!!
 * At the moment (2006/11/11) I have left it as it was!
 *                                        S.Trusov
  ************************************************************************/            
static const char* cvsid __attribute__((unused))=
    "$ZEL: RCardProcs.c,v 1.16 2011/04/06 20:30:34 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/unixvme/vme.h"

#include "CAENV550readout.h"
#include "CAENV775readout.h"
#include "SIS3801readout.h"
#include "../unixvme/caen/v1290.h"
#include <stdio.h>

extern ems_u32* outptr;
extern int wirbrauchen;
extern Modlist *modullist;
extern int *memberlist;

ml_entry * module;
ml_entry * moduleVPG;
struct vme_dev* dev;

RCS_REGISTER(cvsid, "procs/rcard")

/*
 * p[0]: argcount==2
 * p[1]: number of ADC channels, now dummy
 * p[2]: maximum number of strips, now dummy
 */
plerrcode proc_RCardReadout(ems_u32 *p)
{
  int modanz = *memberlist;
  int i;
  int idx;
  
  for(i=1;i<=modanz;i++){
    module = ModulEnt(i); 
    idx = memberlist[i]; /* in a module readout procedure must be idx=idx+1; !!*/
    switch(module->modultype)
      {	
      case VPG517:
	{
	  moduleVPG = module;
	}
	break;
      case CAEN_V550:
	{
          if(!CAENV550readout(module, idx)) return(plErr_System);
	}
	break;
      case CAEN_V775:
	{
	  if(!CAENV775readout(module, idx)) return(plErr_System);
	}
	break;
      case CAEN_V1290:
	{
	  if(!v1290readout(module,memberlist[i])) return(plErr_System);
	  v1290clear(module);
	}
	break;
      case SIS_3800:
	{
	  if(!SIS3800readout(module, idx)) return(plErr_System);
	}
	break;
      case SIS_3801:
	{
	  if(!SIS3801readout(module, idx)) return(plErr_System);
	}
	break;
      default:
	printf("##### proc_RCardReadout(): ignore unknown module type\n");
	break;
      }
  }
  
  //dev = moduleVPG->address.vme.dev;
  //dev->write_a32d32(dev, moduleVPG->address.vme.addr + 0x1fff4, 0x0000);

  return(plOK);
}      

/*
 * p[0]: argcount==2
 * p[1]: number of ADC channels
 * p[2]: maximum number of strips
 */
plerrcode test_proc_RCardReadout(ems_u32 *p)
{
  if(p[0]!=2) return(plErr_ArgNum);
  wirbrauchen = p[1]*(p[2]+1);
  
  return(plOK);
}

char name_proc_RCardReadout[] = "RCardReadout";
int  ver_proc_RCardReadout    = 1;


/*
 * p[0]: argcount==2
 * p[1]: number of ADC channels, dummy now
 * p[2]: maximum number of strips, dummy now 
 * p[3]: ADC value of pedestal, dummy now
 */
plerrcode proc_RCardReadoutCM(ems_u32 *p)
{
  int modanz = *memberlist;
  int i;
  int idx;
  for(i=1;i<=modanz;i++){
    idx = memberlist[i];
    module = ModulEnt(i);
    switch(module->modultype)
      {	
      case VPG517:
	{
	  moduleVPG = module;
	}
	break;
      case CAEN_V550:
	{
	  if(!CAENV550readoutCM(module, idx, p[3])) return(plErr_System);
	}
	break;
      case CAEN_V775:
	{
	  if(!CAENV775readout(module, idx)) return(plErr_System);
	}
	break;
      case SIS_3801:
	{
	  if(!SIS3801readout(module, idx)) return(plErr_System);
	}
	break;
	default:
	break;
      }
  }
    
  //dev = moduleVPG->address.vme.dev;
  //dev->write_a32d32(dev, moduleVPG->address.vme.addr + 0x1fff4, 0x0000);

  return(plOK);
}      

/*
 * p[0]: argcount==3
 * p[1]: number of ADC channels
 * p[2]: maximum number of strips
 * p[3]: ADC value of pedestal
 */
plerrcode test_proc_RCardReadoutCM(ems_u32 *p)
{
  if(p[0]!=3) return(plErr_ArgNum);
  wirbrauchen = p[1]*(p[2]+1);
  
  return(plOK);
}

char name_proc_RCardReadoutCM[] = "RCardReadoutCM";
int  ver_proc_RCardReadoutCM    = 1;

/*
 * p[0]: argcount==0
 */
plerrcode proc_RCardReset(ems_u32 *p)
{
  int modanz = *memberlist;
  int i;
 
  for(i=1;i<=modanz;i++){
    module = ModulEnt(i);
    switch(module->modultype)
      { 
      case VPG517:
        {
          moduleVPG = module;
        }
        break; 
      case CAEN_V550:
	{
          dev = module->address.vme.dev;
	  dev->write_a32d16(dev, module->address.vme.addr + 0x0006, 0x0000);
	}
	break;
      case CAEN_V775:
	{

	}
	break;
      default:
        break;
      }
  }
  
  dev = moduleVPG->address.vme.dev;
  dev->write_a32d32(dev, module->address.vme.addr + 0x1fff4, 0x0000);

  return(plOK);
}

/*
 * p[0]: argcount=0
 */
plerrcode test_proc_RCardReset(ems_u32 *p)
{
  if(p[0]!=0) return(plErr_ArgNum);
  wirbrauchen = 0;
 
  return(plOK);
} 

char name_proc_RCardReset[] = "RCardReset";
int  ver_proc_RCardReset    = 1;


/*
 * p[0]: argcount==2
 * p[1]: moduleindex
 * p[2]: memberindex
 */
plerrcode proc_ReadV550(ems_u32 *p)
{
  module = &modullist->entry[p[1]];
  if(module->modultype!=CAEN_V550) return(plErr_System);  
  if(!CAENV550readout(module,p[2])) return(plErr_System); /* here may be +1 for p[2] */  
  return(plOK);
}

/*
 * p[0]: argcount=2
 * p[1]: moduleindex
 * p[2]: memberindex
 */
plerrcode test_proc_ReadV550(ems_u32 *p)
{
  if(p[0]!=2) return(plErr_ArgNum);
  wirbrauchen = 0;
 
  return(plOK);
} 

char name_proc_ReadV550[] = "ReadV550";
int  ver_proc_ReadV550    = 1;


/*
 * p[0]: argcount== 3 + ndata 
 * p[1]: moduleindex
 * p[2]: offset
 * p[3]: number of data (ndata)
 * p[4]: data
 */
plerrcode proc_WriteBlock(ems_u32 *p)
{
  int l,res;
  ml_entry* module=ModulEnt(p[1]);
  struct vme_dev* dev=module->address.vme.dev;

  for(l=0;l<p[3];l++) {
    res=dev->write_a32d32(dev,l*4+module->address.vme.addr+p[2],p[4+l]);
    if (res!=4) {
      *outptr++ = res;
      return(plErr_System);
    }  
  }
  return(plOK);
}

/*
 * p[0]: argcount== 3 + ndata 
 * p[1]: moduleindex
 * p[2]: offset
 * p[3]: number of data (ndata)
 * p[4]: data
 */
plerrcode test_proc_WriteBlock(ems_u32 *p)
{
  //printf("WriteBlock %d %d %d %d\n",p[0],p[1],p[2],p[3]);

  if(p[0]<3) return(plErr_ArgNum);
  if((p[3] + 3)!=p[0]) return(plErr_ArgNum);
  wirbrauchen = 0;
 
  return(plOK);
} 

char name_proc_WriteBlock[] = "WriteBlock";
int  ver_proc_WriteBlock    = 1;
/* ----------------------------------------------------------------
 * Write block of data at the same address
 * probably we cannot use more than p[4]?!
 */

/*
 * p[0]: argcount== 3 + ndata 
 * p[1]: moduleindex
 * p[2]: offset
 * p[3]: number of data (ndata)
 * p[4]: data : p[4] - state, p[5] - time in cycles, ...
 */
plerrcode proc_WriteToFixAddr(ems_u32 *p)
{
  int l,res,k, delay;
  ml_entry* module=ModulEnt(p[1]);
  struct vme_dev* dev=module->address.vme.dev;

  for(l=0;l<p[3]/2;l++) {
    res=dev->write_a32d32(dev, module->address.vme.addr+p[2],p[4+2*l]);
    if (res!=4) {
      *outptr++ = res;
      return(plErr_System);
    }
    delay = 0;
    for(k=0;k<p[5+2*l]; k++) {
	delay++;
    }
  }
  return(plOK);
}

/*
 * p[0]: argcount== 4 + ndata 
 * p[1]: moduleindex
 * p[2]: offset
 * p[3]: number of data (ndata)
 * p[4]: data : p[4] - state, p[5] - time in cycles, ...
 */
plerrcode test_proc_WriteToFixAddr(ems_u32 *p)
{
  //printf("WriteToFixAddr %d %d %d %d\n",p[0],p[1],p[2],p[3],p[4]);

  if(p[0]<3) return(plErr_ArgNum);
  if((p[3] + 3)!=p[0]) return(plErr_ArgNum);
  if((p[3] % 2) != 0) return(plErr_ArgNum); // must be pairs of (data,delay) words
  wirbrauchen = 0;
 
  return(plOK);
} 

char name_proc_WriteToFixAddr[] = "WriteToFixAddr";
int  ver_proc_WriteToFixAddr    = 1;

/*--------- end of  WriteToFixAddr ----------------------------------------*/

/*
 * p[0]: argcount==2
 * p[1]: moduleindex
 * p[2]: memberindex
 */
plerrcode proc_ReadSIS3801(ems_u32 *p)
{
  module = &modullist->entry[p[1]];
  if(module->modultype!=SIS_3801) return(plErr_System);  
  if(!SIS3801readout(module,p[2])) return(plErr_System);  
  return(plOK);
}

/*
 * p[0]: argcount=2
 * p[1]: moduleindex
 * p[2]: memberindex
 */
plerrcode test_proc_ReadSIS3801(ems_u32 *p)
{
  if(p[0]!=2) return(plErr_ArgNum);
  wirbrauchen = 0;
 
  return(plOK);
} 

char name_proc_ReadSIS3801[] = "ReadSIS3801";
int  ver_proc_ReadSIS3801    = 1;
