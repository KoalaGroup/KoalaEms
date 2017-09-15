/*
 * procs/lvd/vertex/vertex_init.c
 * created 2005-Feb-23 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: vertex_init.c,v 1.30 2011/09/23 13:09:21 trusov Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/lvd/lvdbus.h"
#include "../../../lowlevel/lvd/vertex/vertex.h"
#include "../../../lowlevel/devices.h"
#include "../lvd_verify.h"


#define MATE3_REG_SHIFT  8
#define MATE3_CHIP_SHIFT 11

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

static ems_u32 mtypes[]={ZEL_LVD_ADC_VERTEX, ZEL_LVD_ADC_VERTEXM3, ZEL_LVD_ADC_VERTEXUN, 0};

RCS_REGISTER(cvsid, "procs/lvd/vertex")


/*****************************************************************************
 * This procedure must load any chip type (in correspondence with module 
 * and chip types set in vtx_info[] by lvd_vertex_seq_init() procedure
 * p[0]: argcount
 * p[1]: branch
 * p[2]: module addr (in [0,15] range)
 * p[3]: unit (1: LV, 2: HV)
 * p[4].. data words, length and contents depends on type and number of chips
 *   VATA  : (175*n_chips) bits packed only 16 bits in 32 bit word
 *   Mate3 : (6 bytes * n_chips), only one byte in 32 bit word
 */
plerrcode
proc_lvd_vertex_load_chips(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);    
    return lvd_vertex_load_chips(dev, p[2], p[3], p[0]-3, p+4);
}

plerrcode
test_proc_lvd_vertex_load_chips(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;
    if (p[0]<5)
      return plErr_ArgNum;

    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
      *outptr++ = 1;
      printf("vertex_load_chips: branch %d is not a valid LVD device\n", p[1]);
      return pres;
    }
    if(p[2] > 15) {
      *outptr++ = 2;
      return plErr_ArgRange;
    }
    if((pres=verify_lvd_ids(dev, p[2], mtypes)) != plOK) {
      *outptr++ = 2;
      return pres;
    }
    if ((p[3] < 1) || (p[3]>2)) {
      *outptr++ = 3;
      return plErr_ArgRange;
    }
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_vertex_load_chips[] = "lvd_vertex_load_chips";
int ver_proc_lvd_vertex_load_chips = 1;

/*****************************************************************************
 * This procedure affects all VertexADC/M3/UN modules:
 * - stops them, 
 * - select channels for TP calibration 
 * - prepares for read-out
 * - starts in read-out mode all of them
 *
 * p[0]: argcount==3 
 * p[1]: branch
 * p[2] - channel to be selected, if (-1) - deselect channels
 * p[3] - if 1, channel p[4] is selected in all chips
 *
 */

plerrcode proc_lvd_vertex_calibr_ch(ems_u32* p) {
  plerrcode pres;
  struct lvd_dev* dev=get_lvd_device(p[1]);
  pres = lvd_vertex_calibr_ch(dev, p[2], p[3]);
  return pres;  
}

plerrcode test_proc_lvd_vertex_calibr_ch(ems_u32* p) {
  plerrcode pres = plOK;
  if(p[0] != 3) {    
    pres = plErr_ArgNum;
  }
  else {
    struct lvd_dev* dev=get_lvd_device(p[1]);
    pres=find_lvd_odevice(p[1], &dev);
    *outptr++ = 1;
  }
  wirbrauchen=0;
  return pres;  
}
char name_proc_lvd_vertex_calibr_ch[] = "lvd_vertex_calibr_ch";
int ver_proc_lvd_vertex_calibr_ch = 1;

/*****************************************************************************
 * p[0]: argcount==3 write vtx_info[0].daqmode and vtx_info[1].daqmode of VertexADC
 * p[1]: branch
 * p[2]: module addr 
 * p[3]: daqmode
 *
 */

plerrcode proc_lvd_vertex_mode(ems_u32* p) {
  plerrcode pres;
  struct lvd_dev* dev=get_lvd_device(p[1]);
  pres = lvd_vertex_mode(dev, p[2], p[3]);
  return pres;  
}

plerrcode test_proc_lvd_vertex_mode(ems_u32* p) {
  plerrcode pres = plOK;
  if(p[0] != 3) {
    pres = plErr_ArgNum;
  }
  else {
    struct lvd_dev* dev=get_lvd_device(p[1]);
    pres=find_lvd_odevice(p[1], &dev);
    *outptr++=1;
  }
  wirbrauchen=0;
  return pres;  
}
char name_proc_lvd_vertex_mode[] = "lvd_vertex_mode";
int ver_proc_lvd_vertex_mode = 1;
/*****************************************************************************
 * p[0]: argcount==4 
 * p[1]: branch
 * p[2]: module addr, -1 means all initialized modules
 * p[3]: unit (1,2,3) 
 * p[4]: chan to send test pulse in parallel with normal data
 * p[5]: is in all chips?
 */

plerrcode proc_lvd_vertex_mon_chan(ems_u32* p) {
  plerrcode pres;
  struct lvd_dev* dev=get_lvd_device(p[1]);
  pres = lvd_vertex_mon_chan(dev, p[2], p[3], p[4], p[5]);
  return pres;  
}

plerrcode test_proc_lvd_vertex_mon_chan(ems_u32* p) {
  plerrcode pres = plOK;
  if(p[0] != 5) {
    return plErr_ArgNum;
  }
  struct lvd_dev* dev=get_lvd_device(p[1]);
  if((pres=find_lvd_odevice(p[1], &dev)) != plOK) {
    *outptr++ = 1;
    return pres;
  }
  if((p[3] < 1) || (p[3] > 3)) {
    *outptr++ = 3;
    return plErr_ArgRange;
  }

  wirbrauchen=0;
  return pres;  
}
char name_proc_lvd_vertex_mon_chan[] = "lvd_vertex_mon_chan";
int ver_proc_lvd_vertex_mon_chan = 1;


/*****************************************************************************
 * p[0]: argcount==3
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * p[3]: unit (1: LV, 2: HV 3: both)
 *
 */

plerrcode proc_lvd_vertex_seq_wait(ems_u32* p) {
  plerrcode pres;
  struct lvd_dev* dev=get_lvd_device(p[1]);
  pres = lvd_vertex_seq_wait(dev, p[2], p[3]);
  return pres;
}

plerrcode test_proc_lvd_vertex_seq_wait(ems_u32* p) {
  struct lvd_dev* dev;
  plerrcode pres;

  if(p[0] != 3) {
    return plErr_ArgNum;
  }
  if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
    return pres;
  }
  if (p[2]<16) {
    pres=verify_lvd_ids(dev, p[2], mtypes);
    if(pres != plOK) return pres; 
  }
  if((p[3] < 1) || (p[3] > 3)) {
    return plErr_ArgRange;
  }
  wirbrauchen=0;
  return plOK;
}

char name_proc_lvd_vertex_seq_wait[] = "lvd_vertex_seq_wait";
int ver_proc_lvd_vertex_seq_wait = 1;


/*****************************************************************************/
/*
 * lvd_vertex_chip_info retrieves chip information (type and number)
 * from the EMS server. It uses the information loaded or generated by
 * lvd_vertex_init, it does not check the hardware itself
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: module addr
 *
 * returns:
 * chiptype LV, chiptype HV, number of chips LV, number of chips HV,
 * number of channels LV, number of channels HV
 */
plerrcode proc_lvd_vertex_chip_info(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;

    if ((pres=lvd_vertex_chip_info(dev, p[2], outptr))==plOK)
        outptr+=6;

    return pres;
}

plerrcode test_proc_lvd_vertex_chip_info(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    if ((pres=verify_lvd_ids(dev, p[2], mtypes))!=plOK)
        return pres;

    wirbrauchen=6;
    return plOK;
}

char name_proc_lvd_vertex_chip_info[] = "lvd_vertex_chip_info";
int ver_proc_lvd_vertex_chip_info = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: branch
 * p[2]: module addr
 * p[3]: 1:LV, 2: HV
 * [p[4]: maximum number of chips (default set in lowlevel/lvd/vertex/vertex.c)]
 * if p[4]<0: -p[4] = number of 16-bit words
 */
plerrcode proc_lvd_vertex_count_chips(ems_u32* p)
{
  plerrcode pres = plOK;
  struct lvd_dev* dev=get_lvd_device(p[1]);
  int maxchips= (p[0] > 3) ? p[4] : 0;
  *outptr = lvd_vertex_count_chips(dev, p[2], p[3]-1, maxchips);  
  if(*outptr <= 0) {
    pres = plErr_Other;
  }
  else {
    outptr++;
  }
  return pres;
}

plerrcode test_proc_lvd_vertex_count_chips(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;
    if((p[0] < 3) || (p[0]>4)) 
      return plErr_ArgNum;

    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
      *outptr++ = 1;
      return pres;
    }
    pres=verify_lvd_ids(dev, p[2], mtypes);
    if (pres!=plOK) {
      *outptr++ = 2;
      return pres;
    }
    if ((p[3]<1) || (p[3]>2)) {
      *outptr++ = 3;
      return plErr_ArgRange;
    }
    wirbrauchen=2;
    return plOK;
}

char name_proc_lvd_vertex_count_chips[] = "lvd_vertex_count_chips";
int ver_proc_lvd_vertex_count_chips = 1;
/*****************************************************************************/
/*
 * p[0]: argcount
 * p[1]: branch
 * p[2]: absolute module addr
 * p[3]: 0 sequencer ram, 1 table ram
 * p[4]: addr
 * p[5]: num
 * p[6..]: filename
 */
plerrcode proc_lvd_vertex_read_mem_file(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    char* name;
    plerrcode pres;

    xdrstrcdup(&name, p+6);

    pres=lvd_vertex_read_mem_file(dev, p[2], p[3], p[4], p[5], name);

    free(name);
    return pres;
}

plerrcode test_proc_lvd_vertex_read_mem_file(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]<6)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    pres=verify_lvd_ids(dev, p[2], mtypes);
    if (pres!=plOK)
        return pres;

    if (xdrstrlen(p+6)+5!=p[0])
        return plErr_ArgNum;

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_vertex_read_mem_file[] = "lvd_vertex_read_mem_file";
int ver_proc_lvd_vertex_read_mem_file = 1;
/*****************************************************************************/
#if 0
/*
 * p[0]: argcount
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * p[3]: 0 sequencer ram, 1 table ram
 * p[4]: ram address
 * p[5]: size
 * p[6]: pattern
 */
plerrcode proc_lvd_vertex_fill_mem(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;

    pres=vertex_fill_mem(dev, p[2], p[3], p[4], p[5], p[6]);

    return pres;
}

plerrcode test_proc_lvd_vertex_fill_mem(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=6)
        return plErr_ArgNum;
    dev=find_lvd_odevice(p[1], &pres);
    if (pres!=plOK)
        return pres;

    if (p[2]<16) {
        pres=verify_lvd_ids(dev, p[2], mtypes);
        if (pres!=plOK)
            return pres;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_vertex_fill_mem[] = "lvd_vertex_fill_mem";
int ver_proc_lvd_vertex_fill_mem = 1;
#endif
/*****************************************************************************/
#if 0
/*
 * p[0]: argcount
 * p[1]: branch
 * p[2]: absolute module address
 * p[3]: 0 sequencer ram, 1 table ram
 */
plerrcode proc_lvd_vertex_check_mem(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;

    pres=vertex_check_mem(dev, p[2], p[3]);

    return pres;
}

plerrcode test_proc_lvd_vertex_check_mem(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    dev=find_lvd_odevice(p[1], &pres);
    if (pres!=plOK)
        return pres;

    pres=verify_lvd_ids(dev, p[2], mtypes);
    if (pres!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_vertex_check_mem[] = "lvd_vertex_check_mem";
int ver_proc_lvd_vertex_check_mem = 1;
#endif
/*****************************************************************************/
#if 0
/*
 * p[0]: argcount
 * p[1]: branch
 * p[2]: absolute module address
 * p[3]: 0 sequencer ram, 1 table ram
 */
plerrcode proc_lvd_vertex_mem_size(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;

    pres=vertex_mem_size(dev, p[2], p[3], outptr);
    if (pres==plOK)
        outptr++;

    return pres;
}

plerrcode test_proc_lvd_vertex_mem_size(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    dev=find_lvd_odevice(p[1], &pres);
    if (pres!=plOK)
        return pres;

    pres=verify_lvd_ids(dev, p[2], mtypes);
    if (pres!=plOK)
        return pres;

    wirbrauchen=1;
    return plOK;
}

char name_proc_lvd_vertex_mem_size[] = "lvd_vertex_mem_size";
int ver_proc_lvd_vertex_mem_size = 1;
#endif
/*****************************************************************************/
/*
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module addr, in [0,15] range
 * p[3]: unit: 1 or 2
 * [p[4]:] val - if available -> write, else - read
 */
plerrcode
proc_lvd_vertex_aclk_par(ems_u32* p)
{
  plerrcode pres;
  ems_u32 v = 0;
  struct lvd_dev* dev=get_lvd_device(p[1]);
  if(p[0] == 3) {
    pres = lvd_vertex_aclk_par_read(dev, p[2], p[3], &v);
    if(pres==plOK) {
      *outptr++=v;
    }
  }
  else {
    pres = lvd_vertex_aclk_par_write(dev, p[2], p[3], p[4]);
  }
  return pres;
}

plerrcode
test_proc_lvd_vertex_aclk_par(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;
  
    if ((p[0] < 3) || (p[0]>4)) {
        return plErr_ArgNum;
    }
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        printf("vertex_aclk_par: branch %d is not a valid LVD device\n", p[1]);
        *outptr++=1;
        return pres;
    }
    if (p[2]<16) {
        pres=verify_lvd_ids(dev, p[2], mtypes);
        if (pres!=plOK) {
            *outptr++=2;
            return pres;
        }
    }
    if((p[3] < 1) || (p[3]>2)) {
      *outptr++=3;
      return plErr_ArgRange;
    }
    wirbrauchen = (p[0]==3) ? 1 : 0;
    return plOK;
}

char name_proc_lvd_vertex_aclk_par[] = "lvd_vertex_aclk_par";
int ver_proc_lvd_vertex_aclk_par = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module addr , in [0,15] range
 * p[3]: unit (1: LV, 2: HV)
 * p[4]: ind (0/1)
 * p[5]: if available - write, else - read 
 */
plerrcode
proc_lvd_vertex_dgap_len(ems_u32* p)
{
  plerrcode pres;
  struct lvd_dev* dev=get_lvd_device(p[1]);
  if(p[0] == 4) { 
    pres= lvd_vertex_dgap_len_read(dev, p[2], p[3], p[4], outptr);
    if(pres == plOK) {
      outptr++;
    }
  }
  else {
    pres= lvd_vertex_dgap_len_write(dev, p[2], p[3], p[4], p[5]);
  }
  return pres;
}

plerrcode
test_proc_lvd_vertex_dgap_len(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;
    if ((p[0] < 4) || (p[0]>5)) {
        return plErr_ArgNum;
    }
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        *outptr++=1;
        return pres;
    }
    if (p[2]<16) {
        pres=verify_lvd_ids(dev, p[2], mtypes);
        if (pres!=plOK) {
            *outptr++=2;
            return pres;
        }
    }
    if((p[3] < 1) || (p[3]>2)) {
      *outptr++=3;
      return plErr_ArgRange;
    }
    if(p[4] > 1) {
      *outptr++=3;
      return plErr_ArgRange;
    }
    wirbrauchen = (p[0]==3) ? 1 : 0;
    return plOK;
}
char name_proc_lvd_vertex_dgap_len[] = "lvd_vertex_dgap_len";
int ver_proc_lvd_vertex_dgap_len = 1;




/*****************************************************************************/
/*
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * p[3]: unit (1: LV, 2: HV)
 * p[4]: delay on serial bus (either I2C or VA clock to read chip content
 * p[5]: period of cycle on serial bus (to write chip contents, I2C/VA)
 * p[6]: period of cycle to write DACs of repeater cards 
 * p[7]: period of cycle to write ADC offset of VertexADC/M3/UN modules 
 */
plerrcode
proc_lvd_vertex_cycles(ems_u32* p)
{
  struct lvd_dev* dev=get_lvd_device(p[1]);
  return lvd_vertex_cycles(dev, p[2], p[3]-1, &p[4]);
}

plerrcode
test_proc_lvd_vertex_cycles(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;
    if (p[0] != 7) {
      return plErr_ArgNum;
    }
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
      *outptr++=1;
      return pres;
    }
    if(p[2]>15) {
      *outptr++=2;
      return plErr_ArgRange;
    }
    else {
      pres=verify_lvd_ids(dev, p[2], mtypes);
      if (pres!=plOK) {
	*outptr++=2;
	return pres;
      }
    }
    if((p[3] < 1) || (p[3]>2)) {
      *outptr++=3;
      return plErr_ArgRange;
    }
    wirbrauchen = 0;
    return plOK;
}

char name_proc_lvd_vertex_cycles[] = "lvd_vertex_cycles";
int ver_proc_lvd_vertex_cycles = 1;

/*****************************************************************************/
/*
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * p[3]: unit (1: LV, 2: HV 3: both)
 * p[4]: val, if available - write, else - read
 */
plerrcode
proc_lvd_vertex_aclk_delay(ems_u32* p)
{
  struct lvd_dev* dev=get_lvd_device(p[1]);
  plerrcode pres;
  if(p[0] == 3) { /* read */
    pres =  lvd_vertex_aclk_delay_read(dev, p[2], p[3], outptr);
    if(pres == plOK) {
      outptr++;
    }
  } else { /* write */
    pres =  lvd_vertex_aclk_delay_write(dev, p[2], p[3], p[4]);    
  }
  return pres;
}

plerrcode
test_proc_lvd_vertex_aclk_delay(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;
    if ((p[0] < 3) || (p[0]>4)) {
        return plErr_ArgNum;
    }
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        *outptr++=1;
        return pres;
    }
    if (p[2]<16) {
      pres=verify_lvd_ids(dev, p[2], mtypes);
      if (pres!=plOK) {
	*outptr++=2;
	return pres;
      }
    }
    if((p[3] < 1) || (p[3]>2)) {
      *outptr++=3;
      return plErr_ArgRange;
    }
    wirbrauchen = (p[0]==3) ? 1 : 0;
    return plOK;
}

char name_proc_lvd_vertex_aclk_delay[] = "lvd_vertex_aclk_delay";
int ver_proc_lvd_vertex_aclk_delay = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module addr 
 * [p[3]: val] 
 */
plerrcode
proc_lvd_vertex_seq_csr(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;

    if (p[0]>2) {
        pres=lvd_vertex_seq_csr(dev, p[2], p[3]);
    } else {
        pres=lvd_vertex_get_seq_csr(dev, p[2], outptr);
        if (pres==plOK)
            outptr++;
    }
    return pres;
}

plerrcode
test_proc_lvd_vertex_seq_csr(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;
  
    if ((p[0]!=2)&&(p[0]!=3))
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        *outptr++=1;
        return pres;
    }
    if(p[2]>15) {
      return plErr_ArgRange;
    }
    pres=verify_lvd_ids(dev, p[2], mtypes);
    if (pres!=plOK) {
      *outptr++=2;
      return pres;
    }
    wirbrauchen= (p[0]==2) ? 1 : 0;
    return plOK;
}

char name_proc_lvd_vertex_seq_csr[] = "lvd_vertex_seq_csr";
int ver_proc_lvd_vertex_seq_csr = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * p[3]: unit (1: LV, 2: HV 3: both)
 * [p[4]: idx (0..7 or -1)
 *  p[5]: val (4 bit)
 *  p[6]: enable sequencer]
 *
 * if idx==-1 val is a 32-bit value which sets all eight switches simultaneous
 */
plerrcode
proc_lvd_vertex_seq_switch(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;

    if (p[0]>4) {
        pres=lvd_vertex_seq_switch(dev, p[2], p[3], p[4], p[5], p[6]);
    } else {
      pres=lvd_vertex_get_seq_switch(dev, p[2], p[3], outptr);
      if (pres==plOK)
	outptr+=p[3]==3?2:1;
    }
    return pres;
}

plerrcode
test_proc_lvd_vertex_seq_switch(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if ((p[0]!=3)&&(p[0]!=6))
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        printf("vertex_seq_switch: branch %d is not a valid LVD device\n", p[1]);
        *outptr++=1;
        return pres;
    }
    if (p[2] < 16) {
      pres=verify_lvd_ids(dev, p[2], mtypes);
      if (pres!=plOK) {
	*outptr++=2;
	return pres;
      }
    }
    if (p[3]>3) {
      *outptr++=3;
      return plErr_ArgRange;
    }
    if ((p[0]>3) && (p[4]>7)) {
      *outptr++=4;
      return plErr_ArgRange;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_vertex_seq_switch[] = "lvd_vertex_seq_switch";
int ver_proc_lvd_vertex_seq_switch = 1;

/*****************************************************************************/
/*
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module addr (absolute address)
 */
plerrcode
proc_lvd_vertex_seq_count(ems_u32* p)
{
    int pres;
    struct lvd_dev* dev=get_lvd_device(p[1]);
    pres=lvd_vertex_seq_count(dev, p[2], outptr);
    if (pres==plOK)
        outptr+=4;
    return pres;
}

plerrcode
test_proc_lvd_vertex_seq_count(ems_u32* p)
{
  struct lvd_dev* dev;
  plerrcode pres;
  
  if (p[0]!=2)
    return plErr_ArgNum;
  if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
    *outptr++=1;
    return pres;
  }
  pres=verify_lvd_ids(dev, p[2], mtypes);
  if (pres!=plOK) {
    *outptr++=2;
    return pres;
  }

  wirbrauchen=4;
  return plOK;
}

char name_proc_lvd_vertex_seq_count[] = "lvd_vertex_seq_count";
int ver_proc_lvd_vertex_seq_count = 1;
/*****************************************************************************/
/*
 * p[0]: argcount 2 or 5
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * [p[3]: unit (1: LV, 2: HV 3: both)
 *  p[4]: idx (0..1)
 *  p[5]: val]
 */
plerrcode
proc_lvd_vertex_seq_loopcount(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    if (p[0]>2) {
      pres=lvd_vertex_seq_loopcount(dev, p[2], p[3], p[4], p[5]);
    } else {
      pres=lvd_vertex_seq_get_loopcount(dev, p[2], outptr);
      if (pres==plOK)
	outptr+=4;
    }
    return pres;
}

plerrcode
test_proc_lvd_vertex_seq_loopcount(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;
  
    if (p[0]!=2 && p[0]!=5)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        *outptr++=1;
        return pres;
    }
 
    if ( p[2] < 16) {
      pres=verify_lvd_ids(dev, p[2], mtypes);
      if (pres!=plOK) {
	*outptr++=2;
	return pres;
      }
    }
    if (p[0]>2) {
      if (p[3]>3) {
	*outptr++=3;
	return plErr_ArgRange;
      }
      if (p[4]>1) {
	*outptr++=4;
	return plErr_ArgRange;
      }
    }
    wirbrauchen=p[0]>2?0:4;
    return plOK;
}

char name_proc_lvd_vertex_seq_loopcount[] = "lvd_vertex_seq_loopcount";
int ver_proc_lvd_vertex_seq_loopcount = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module addr, in [0,15] range
 * p[3]: unit (1: LV, 2: HV )
 * p[4]: val
 */
plerrcode
proc_lvd_vertex_nrchan(ems_u32* p)
{
  plerrcode pres;
  struct lvd_dev* dev=get_lvd_device(p[1]);
  if(p[0] == 3) { /* read */
    pres =  lvd_vertex_nrchan_read(dev, p[2], p[3], outptr);
    if(pres == plOK) {
      outptr++;
    }
  } else { /* write */
    pres =  lvd_vertex_nrchan_write(dev, p[2], p[3], p[4]);    
  }
  return pres;
}

plerrcode
test_proc_lvd_vertex_nrchan(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;
    if ((p[0] < 3) || (p[0]>4)) {
      return plErr_ArgNum;
    }
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        *outptr++=1;
        return pres;
    }
    if (p[2]<16) {
      pres=verify_lvd_ids(dev, p[2], mtypes);
      if (pres!=plOK) {
	*outptr++=2;
	return pres;
      }
    }
    if((p[3] < 1) || (p[3]>2)) {
      *outptr++=3;
      return plErr_ArgRange;
    }
    wirbrauchen = (p[0]==3) ? 1 : 0;
    return plOK;
}

char name_proc_lvd_vertex_nrchan[] = "lvd_vertex_nrchan";
int ver_proc_lvd_vertex_nrchan = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module address, in [0,15] range
 * p[3]: unit (1: LV, 2: HV )
 * p[4]: val, if available - write, else -read
 */
plerrcode
proc_lvd_vertex_noisewin(ems_u32* p)
{
  plerrcode pres;
  struct lvd_dev* dev=get_lvd_device(p[1]);
  if(p[0] == 3) { /* read */
    pres =lvd_vertex_noisewin_read(dev, p[2], p[3], outptr);
    if(pres == plOK) {
      outptr++;
    }
  } else { /* write */
    pres =  lvd_vertex_noisewin_write(dev, p[2], p[3], p[4]);    
  }
  return pres;
}

plerrcode
test_proc_lvd_vertex_noisewin(ems_u32* p)
{
  struct lvd_dev* dev;
  plerrcode pres;
  if ((p[0] < 3) || (p[0]>4)) {
    return plErr_ArgNum;
  }

  if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
    *outptr++=1;
    return pres;
  }
    
  if (p[2] > 15) {
    *outptr++=2;
    return plErr_ArgRange;
  }
  pres=verify_lvd_ids(dev, p[2], mtypes);
  if (pres!=plOK) {
    *outptr++=2;
    return pres;
  }
  if ((p[3] < 1) || (p[3]>2)) {
    *outptr++=3;
    return plErr_ArgRange;
  }
  wirbrauchen= (p[0]==3) ? 1 : 0;
  return plOK;
}

char name_proc_lvd_vertex_noisewin[] = "lvd_vertex_noisewin";
int ver_proc_lvd_vertex_noisewin = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 */
plerrcode
proc_lvd_vertex_cmodval(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres = lvd_vertex_cmodval(dev, p[2], outptr);
    if (pres==plOK)
      outptr+=2;
    return pres;
}

plerrcode
test_proc_lvd_vertex_cmodval(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;
  
    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        printf("vertex_cmodval: branch %d is not a valid LVD device\n", p[1]);
        *outptr++=1;
        return pres;
    }
    if (p[2]>15) {
      *outptr++=2;
      return plErr_ArgRange;
    }
    pres=verify_lvd_ids(dev, p[2], mtypes);
    if (pres!=plOK) {
      *outptr++=2;
      return pres;
    }
    wirbrauchen=2;
    return plOK;
}

char name_proc_lvd_vertex_cmodval[] = "lvd_vertex_cmodval";
int ver_proc_lvd_vertex_cmodval = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 */
plerrcode
proc_lvd_vertex_nrwinval(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;

    pres=lvd_vertex_nrwinval(dev, p[2], outptr);
    if (pres==plOK)
        outptr+=2;
    return pres;
}

plerrcode
test_proc_lvd_vertex_nrwinval(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;
  
    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        printf("vertex_nrwinval: branch %d is not a valid LVD device\n", p[1]);
        *outptr++=1;
        return pres;
    }
    
    if (p[2] > 15) {
      *outptr++=2;
      return plErr_ArgRange;      
    }
    pres=verify_lvd_ids(dev, p[2], mtypes);
    if (pres!=plOK) {
      *outptr++=2;
      return pres;
    }
    wirbrauchen=2;
    return plOK;
}

char name_proc_lvd_vertex_nrwinval[] = "lvd_vertex_nrwinval";
int ver_proc_lvd_vertex_nrwinval = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module addr, in 0,15] range
 */
plerrcode
proc_lvd_vertex_sample(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres = lvd_vertex_sample(dev, p[2], outptr);
    if (pres==plOK)
        outptr+=2;
    return pres;
}

plerrcode
test_proc_lvd_vertex_sample(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        printf("vertex_sample: branch %d is not a valid LVD device\n", p[1]);
        *outptr++=1;
        return pres;
    }
    if (p[2] > 15) {
      *outptr++=2;
      return plErr_ArgRange;      
    }
    pres=verify_lvd_ids(dev, p[2], mtypes);
    if (pres!=plOK) {
      *outptr++=2;
      return pres;
    }
    wirbrauchen=2;
    return plOK;
}

char name_proc_lvd_vertex_sample[] = "lvd_vertex_sample";
int ver_proc_lvd_vertex_sample = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module addr, in [0,15] range 
 * p[3]: unit (1: LV, 2: HV)
 * p[4]: val (0..255), only write, cannot read back!
 */
plerrcode
proc_lvd_vertex_adc_dac(ems_u32* p)
{
  struct lvd_dev* dev=get_lvd_device(p[1]);
  return lvd_vertex_adc_dac(dev, p[2], p[3], p[4]);    
}

plerrcode
test_proc_lvd_vertex_adc_dac(ems_u32* p)
{
  struct lvd_dev* dev;
  plerrcode pres;
  if (p[0] != 4) {
    return plErr_ArgNum;
  }
  
  if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
    printf("vertex_noisewin: branch %d is not a valid LVD device\n", p[1]);
    *outptr++=1;
    return pres;
  }
    
  if (p[2] > 15) {
    *outptr++=2;
    return plErr_ArgRange;
  }
  pres=verify_lvd_ids(dev, p[2], mtypes);
  if (pres!=plOK) {
    *outptr++=2;
    return pres;
  }
  if ((p[3] < 1) || (p[3]>2)) {
    *outptr++=3;
    return plErr_ArgRange;
  }
  if(p[4] > 255) {
    *outptr++=4;
    return plErr_ArgRange;
  }
  wirbrauchen= 0;
  return plOK;
}

char name_proc_lvd_vertex_adc_dac[] = "lvd_vertex_adc_dac";
int ver_proc_lvd_vertex_adc_dac = 1;
/*****************************************************************************/
/* LOAD repeater card DACS for any type of chip and VertexADC[M3/UN] modules.
 * Left name as it is for backward compartibility...
 * p[0]: argcount (==12)
 * p[1]: branch
 * p[2]: module addr, in [0,15] range
 * p[3]: unit (1: LV, 2: HV)
 * p[4...]: values (16 bit),  9 words
 * NEVER read back, may be conflict with I2C slow control code!
 */
plerrcode
proc_lvd_vertex_vata_dac(ems_u32* p)
{
  struct lvd_dev* dev=get_lvd_device(p[1]);
  return lvd_vertex_vata_dac(dev, p[2], p[3], p[0]-3, p+4);
}

plerrcode
test_proc_lvd_vertex_vata_dac(ems_u32* p)
{
  struct lvd_dev* dev;
  plerrcode pres;
  
  if (p[0] != 12)
    return plErr_ArgNum;
  if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
    *outptr++=1;
    return pres;
  }
  if (p[2]>15) {
    *outptr++=2;
    return plErr_ArgRange;
  }
  pres=verify_lvd_ids(dev, p[2], mtypes);
  if (pres!=plOK) {
    *outptr++=2;
    return pres;
  }
  if ((p[3]<1) || (p[3]>2)) {
    *outptr++=3;
    return plErr_ArgRange;
  }
  wirbrauchen=0;
  return plOK;
}

char name_proc_lvd_vertex_vata_dac[] = "lvd_vertex_vata_dac";
int ver_proc_lvd_vertex_vata_dac = 1;

/******************************************************************************
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module addr, in [0,15] range
 * p[3]: unit (1: LV, 2: HV)
 * p[4]: number of DAC, in [0,7] range
 * p[5]: new DAC value, 16 bit word
 ******************************************************************************/
plerrcode
proc_lvd_vertex_vata_dac_change(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    return lvd_vertex_vata_dac_change(dev, p[2], p[3], p[4], p[5]);
}

plerrcode
test_proc_lvd_vertex_vata_dac_change(ems_u32* p)
{
  struct lvd_dev* dev;
  plerrcode pres;
  
  if (p[0] != 5)
    return plErr_ArgNum;
  if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
    *outptr++=1;
    return pres;
  }
  if(p[2] > 15) {
    *outptr++=2;
    return plErr_ArgRange;
  }
  pres=verify_lvd_ids(dev, p[2], mtypes);
  if (pres!=plOK) {
    *outptr++=2;
    return pres;
  }
  if ((p[3]<1) || (p[3]>2)) {
    *outptr++=3;
    return plErr_ArgRange;
  }
  if (p[4] > 7) {
    *outptr++=4;
    return plErr_ArgRange;
  }
  wirbrauchen=0;
  return plOK;
}
char name_proc_lvd_vertex_vata_dac_change[] = "lvd_vertex_vata_dac_change";
int ver_proc_lvd_vertex_vata_dac_change = 1;

/*****************************************************************************
 *
 * p[0]: argcount
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * p[3]: unit (1: LV, 2: HV, 3: both)
 * // p[4]: number of bits
 * p[5]..p[5+nw-1] 16 bit data words
 *        nw = (p[4]+15)/16
 *
 *
 * we use only 16 bit of the 32-bit data words. We will change it later!
 */
plerrcode
proc_lvd_vertex_load_vata(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    return lvd_vertex_load_vata(dev, p[2], p[3], p[0]-3, p+4);
}

plerrcode
test_proc_lvd_vertex_load_vata(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]<5)
        return plErr_ArgNum;

    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        *outptr++=1;
        return pres;
    }
    if(p[2] > 15) {
      *outptr++=2;
      return plErr_ArgRange;
    }
/*
    nw=(p[4]+15)/16;
    if (p[0]!=nw+4)
        return plErr_ArgNum;
*/

    if ((p[3]<1) || (p[3]>2)) {
      *outptr++=2;
      return plErr_ArgRange;
    }
    pres=verify_lvd_id(dev, p[2], ZEL_LVD_ADC_VERTEX);
    if(pres!=plOK) {
       pres=verify_lvd_id(dev, p[2], ZEL_LVD_ADC_VERTEXUN);
    }
    if (pres!=plOK) {
      return pres;
    }
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_vertex_load_vata[] = "lvd_vertex_load_vata";
int ver_proc_lvd_vertex_load_vata = 1;
/*****************************************************************************/
/*
 * p[0]: argcount
 * p[1]: branch
 * p[2]: module addr
 * p[3]: unit (1: LV, 2: HV)
 * returns: number of bits, (#bits+15)/16 datawords
 */
plerrcode
proc_lvd_vertex_read_vata(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    int bits, words;    
    pres=lvd_vertex_read_vata(dev, p[2], p[3], outptr+1, &bits);
    words=(bits+15)/16;
    if (pres==plOK) {
        *outptr++=bits;
        outptr+=words;
    }
    return pres;
}

plerrcode
test_proc_lvd_vertex_read_vata(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;

    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        printf("vertex_read_vata: branch %d is not a valid LVD device\n", p[1]);
        *outptr++=1;
        return pres;
    }
    pres=verify_lvd_id(dev, p[2], ZEL_LVD_ADC_VERTEX);
    if (pres!=plOK) {
      pres=verify_lvd_id(dev, p[2], ZEL_LVD_ADC_VERTEXUN);
    }
    if (pres!=plOK)
      return pres;

    if (p[3]!=1 && p[3]!=2)
      return plErr_ArgRange;
    wirbrauchen=-1;
    return plOK;
}

char name_proc_lvd_vertex_read_vata[] = "lvd_vertex_read_vata";
int ver_proc_lvd_vertex_read_vata = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (>5)
 * p[1]: branch
 * p[2]: module addr, in [0,15] range
 * p[3]: unit (1: LV, 2: HV 3: both)
 * p[4]: addr
 * p[5]: number of 16-bit words
 * p[6...]: values (two words packed in 32 bit)
 */
plerrcode
proc_lvd_vertex_load_sequencer(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    return lvd_vertex_load_sequencer(dev, p[2], p[3], p[4], p[5], p+6);
}

plerrcode
test_proc_lvd_vertex_load_sequencer(ems_u32* p)
{
    struct lvd_dev* dev;
    int nr16, nr32;
    plerrcode pres;

    if (p[0]<5)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        printf("vertex_load_sequencer: branch %d is not a valid LVD device\n", p[1]);
        *outptr++=1;
        return pres;
    }
    if (p[2] > 15) {
      *outptr++=2;
      return plErr_ArgRange;
    }
    pres=verify_lvd_ids(dev, p[2], mtypes);
    if (pres!=plOK) {
      *outptr++=2;
      return pres;
    }
    if ((p[3]<1) || (p[3]>2)) {
      *outptr++=3;
      return plErr_ArgRange;
    }

    nr16=p[5]; /* number of 16-bit words */
    nr32=(nr16+1)/2; /* number of 32-bit words */
    if ((p[4]&1) && !(nr16&1))
      nr32++;
    if (p[0]-5!=nr32) {
      printf("vertex_load_sequencer: p[5]=%d; expected %d data words (not %d)\n",
	     p[5], nr32, p[0]-5);
      return plErr_ArgNum;
    }

/* XXX range check of addr and number is missing here */

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_vertex_load_sequencer[] = "lvd_vertex_load_sequencer";
int ver_proc_lvd_vertex_load_sequencer = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * p[3]: unit (1: LV, 2: HV 3: both)
 * p[4]: value
 */
plerrcode
proc_lvd_vertex_fill_sequencer(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    return lvd_vertex_fill_sequencer(dev, p[2], p[3], 0, 0x80000, p[4]);
}

plerrcode
test_proc_lvd_vertex_fill_sequencer(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        printf("vertex_fill_sequencer: branch %d is not a valid LVD device\n", p[1]);
        *outptr++=1;
        return pres;
    }
    if (p[2] < 16) {
        pres=verify_lvd_ids(dev, p[2], mtypes);
        if (pres!=plOK) {
            *outptr++=2;
            return pres;
        }
    }
    if ((p[3] < 1) || (p[3]>3)) {
      *outptr++=3;
      return plErr_ArgRange;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_vertex_fill_sequencer[] = "lvd_vertex_fill_sequencer";
int ver_proc_lvd_vertex_fill_sequencer = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module addr (absolute address)
 * p[3]: unit (1: LV, 2: HV)
 * p[4]: addr
 * p[5]: number of 16-bit words
 */
plerrcode
proc_lvd_vertex_read_sequencer(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    int n;

    /* num/2 plus one additional word if addr or num are odd */
    n=p[5]/2+((p[4]&1)|(p[5]&1));
    *outptr++=n;
    pres=lvd_vertex_read_sequencer(dev, p[2], p[3], p[4], p[5], outptr);
    if (pres==plOK)
        outptr+=n;
    else
        outptr--;
    return pres;
}

plerrcode
test_proc_lvd_vertex_read_sequencer(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]<5)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        printf("vertex_read_sequencer: branch %d is not a valid LVD device\n", p[1]);
        *outptr++=1;
        return pres;
    }
    pres=verify_lvd_ids(dev, p[2], mtypes);
    if (pres!=plOK) {
        *outptr++=2;
        return pres;
    }
    if ((p[3] < 1) ||(p[3]>2)) {
      *outptr++=3;
      return plErr_ArgRange;
    }
    wirbrauchen=p[5]/2+2;
    return plOK;
}

char name_proc_lvd_vertex_read_sequencer[] = "lvd_vertex_read_sequencer";
int ver_proc_lvd_vertex_read_sequencer = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (>5)
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * p[3]: unit (1: LV, 2: HV 3: both)
 * p[4]: addr, for VERTEX[M3] ALWAYS 0? 
 * p[5]: number of 16-bit words
 * p[6...]: values (32 bit)
 */
plerrcode
proc_lvd_vertex_load_pedestal(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    return lvd_vertex_load_ram(dev, p[2], p[3], p[4], p[5], p+6);
}

plerrcode
test_proc_lvd_vertex_load_pedestal(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]<5)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
      printf("vertex_load_pedestal: branch %d is not a valid LVD device\n", p[1]);
      *outptr++=1;
      return pres;
    }
    if(p[2] > 15) {
      *outptr++=2;
      return plErr_ArgRange;
    }
    pres=verify_lvd_ids(dev, p[2], mtypes);
    if (pres!=plOK) {
      *outptr++=2;
      return pres;
    }
    if ((p[3]<1) || (p[3]>2)) {
      *outptr++=3;
      return plErr_ArgRange;
    }
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_vertex_load_pedestal[] = "lvd_vertex_load_pedestal";
int ver_proc_lvd_vertex_load_pedestal = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * p[3]: unit (1: LV, 2: HV 3: both)
 * p[4]: value
 */
plerrcode
proc_lvd_vertex_fill_pedestal(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    return lvd_vertex_fill_ram(dev, p[2], p[3], 0, 0x1000, p[4]);
}

plerrcode
test_proc_lvd_vertex_fill_pedestal(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        printf("vertex_fill_pedestal: branch %d is not a valid LVD device\n",
                p[1]);
        *outptr++=1;
        return pres;
    }
    if (p[2] < 16) {
        pres=verify_lvd_ids(dev, p[2], mtypes);
        if (pres!=plOK) {
            *outptr++=2;
            return pres;
        }
    }
    if ((p[3] < 1) || (p[3]>3)) {
      *outptr++=3;
      return plErr_ArgRange;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_vertex_fill_pedestal[] = "lvd_vertex_fill_pedestal";
int ver_proc_lvd_vertex_fill_pedestal = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module addr (absolute address)
 * p[3]: unit (1: LV, 2: HV)
 * p[4]: addr
 * p[5]: number of 16-bit words
 */
plerrcode
proc_lvd_vertex_read_pedestal(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    int n;

    /* num/2 plus one additional word if addr or num are odd */
    n=p[5]/2+((p[4]&1)|(p[5]&1));
    *outptr++=n;
    pres=lvd_vertex_read_ram(dev, p[2], p[3], p[4], p[5], outptr);
    if (pres==plOK)
        outptr+=n;
    else
        outptr--;
    return pres;
}

plerrcode
test_proc_lvd_vertex_read_pedestal(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]<5)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        printf("vertex_read_pedestal: branch %d is not a valid LVD device\n", p[1]);
        *outptr++=1;
        return pres;
    }
    if(p[2] > 15) {
      *outptr++=2;
      return pres;
    }
    pres=verify_lvd_ids(dev, p[2], mtypes);
    if (pres!=plOK) {
      *outptr++=2;
      return pres;
    }
    if ((p[3] < 1) || (p[3]>2)) {
      *outptr++=3;
      return plErr_ArgRange;
    }

    wirbrauchen=-1;
    return plOK;
}

char name_proc_lvd_vertex_read_pedestal[] = "lvd_vertex_read_pedestal";
int ver_proc_lvd_vertex_read_pedestal = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module addr, in [0,15] range
 * p[3]: unit (1: LV, 2: HV)
 * p[4]: addr
 * p[5]: number of 16-bit words
 * p[6...]: values (32 bit)
 */
plerrcode
proc_lvd_vertex_load_threshold(ems_u32* p)
{
  struct lvd_dev* dev=get_lvd_device(p[1]);
  ems_u32 thr_offs = lvd_vertex_thr_offs(dev, p[2], p[3]-1);
  return lvd_vertex_load_ram(dev, p[2], p[3], p[4]+thr_offs, p[5], p+6);
}

plerrcode
test_proc_lvd_vertex_load_threshold(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]<5)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        printf("vertex_load_threshold: branch %d is not a valid LVD device\n",
                p[1]);
        *outptr++=1;
        return pres;
    }
    if (p[2] > 15) {
      *outptr++=2;
      return pres;
    }
    pres=verify_lvd_ids(dev, p[2], mtypes);
    if (pres!=plOK) {
      *outptr++=2;
      return pres;
    }
    if ((p[3] < 1) || (p[3] > 2)) {
      *outptr++=3;
      return plErr_ArgRange;
    }
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_vertex_load_threshold[] = "lvd_vertex_load_threshold";
int ver_proc_lvd_vertex_load_threshold = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * p[3]: unit (1: LV, 2: HV 3: both)
 * p[4]: value
 */
plerrcode
proc_lvd_vertex_fill_threshold(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    return lvd_vertex_fill_ram(dev, p[2], p[3], 0x1000, 0x1000, p[4]);
}

plerrcode
test_proc_lvd_vertex_fill_threshold(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        printf("vertex_fill_threshold: branch %d is not a valid LVD device\n", p[1]);
        *outptr++=1;
        return pres;
    }
    if (p[2]<16) {
        pres=verify_lvd_ids(dev, p[2], mtypes);
        if (pres!=plOK) {
            *outptr++=2;
            return pres;
        }
    }
    if ((p[3] < 1) || (p[3] > 3)) {
      *outptr++=3;
      return plErr_ArgRange;
    }
    
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_vertex_fill_threshold[] = "lvd_vertex_fill_threshold";
int ver_proc_lvd_vertex_fill_threshold = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module addr (absolute address)
 * p[3]: unit (1: LV, 2: HV)
 * p[4]: addr
 * p[5]: number of 16-bit words
 */
plerrcode
proc_lvd_vertex_read_threshold(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    int n;

    /* num/2 plus one additional word if addr or num are odd */
    n=p[5]/2+((p[4]&1)|(p[5]&1));
    *outptr++=n;
    pres=lvd_vertex_read_ram(dev, p[2], p[3], p[4]+0x1000, p[5], outptr);
    if (pres==plOK)
        outptr+=n;
    else
        outptr--;
    return pres;
}

plerrcode
test_proc_lvd_vertex_read_threshold(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]<5)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        printf("vertex_read_threshold: branch %d is not a valid LVD device\n",
                p[1]);
        *outptr++=1;
        return pres;
    }
    if(p[2] > 15) {
      *outptr++=2;
      return pres;
    }
    pres=verify_lvd_ids(dev, p[2], mtypes);
    if (pres!=plOK) {
        *outptr++=2;
        return pres;
    }
    if ((p[3] < 1) || (p[3] > 2)) {
        *outptr++=3;
        return plErr_ArgRange;
    }

    wirbrauchen=p[5]/2+2;
    return plOK;
}

char name_proc_lvd_vertex_read_threshold[] = "lvd_vertex_read_threshold";
int ver_proc_lvd_vertex_read_threshold = 1;
/*****************************************************************************/
/*****************************************************************************/
/* Write user data word, one cannot read it back.
 * p[0]: argcount (==4)
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * p[3]: unit, (1-LV, 2-HV, 3-both)
 * p[4]: val, 24 bits
 */
plerrcode
proc_lvd_vertex_write_uw(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    return lvd_vertex_write_uw(dev, p[2], p[3], p[4]);
}
plerrcode
test_proc_lvd_vertex_write_uw(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;
  
    if (p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        printf("proc_lvd_vertex_write_uw: branch %d is not a valid LVD device\n", p[1]);
        *outptr++=1;
        return pres;
    }
    if (p[2]<16) {
        pres=verify_lvd_ids(dev, p[2], mtypes);
        if (pres!=plOK) {
            *outptr++=2;
            return pres;
        }
    }
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_vertex_write_uw[] = "lvd_vertex_write_uw";
int ver_proc_lvd_vertex_write_uw = 1;
/*****************************************************************************/
/* Set parameters to prepare sequencer to start read-out
 * p[0]: argcount (==6 / 7)
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * p[3]: unit, (1-LV, 2-HV, 3-both)
 * p[4]: switch reg value to start readout, must not be 0!
 * p[5]: aclk_par, must correspond to par[4], see manual
 * p[6]: pause between channel 0 and 1 in each chip
 * p[7]: number of read-out clocks, for MATE3 only
 */
plerrcode
proc_lvd_vertex_info_ro_par_set(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    int len = p[0] - 3;
    return lvd_vertex_info_ro_par_set(dev, p[2], p[3], len, &p[3]);
}
plerrcode
test_proc_lvd_vertex_info_ro_par_set(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;
  
    if (p[0]!=6) {
        return plErr_ArgNum;
    }
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        printf("proc_lvd_vertex_info_ro_par_set: branch %d is not a valid LVD device\n", p[1]);
        *outptr++=1;
        return pres;
    }
    if (p[2]<16) {
        pres=verify_lvd_ids(dev, p[2], mtypes);
        if (pres!=plOK) {
	  printf("proc_lvd_vertex_info_ro_par_set: is not ZEL_LVD_ADC_VERTEX[M3]\n");
            *outptr++=2;
            return pres;
        }
    }
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_vertex_info_ro_par_set[] = "lvd_vertex_info_ro_par_set";
int ver_proc_lvd_vertex_info_ro_par_set = 1;
/*****************************************************************************/
/* Set parameters to prepare sequencer to start read-out
 * p[0]: argcount (==3/6), if 3 use vtx_info[idx].ro_par[]
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * p[3]: unit, (1-LV, 2-HV, 3-both)
 * p[4]: switch reg value to start readout, must not be 0!
 * p[5]: aclk_par, must correspond to par[4], see manual
 * p[6]: pause between channel 0 and 1 in each chip
 * p[7]: number of read-out clocks, MATE3 only
 */
plerrcode
proc_lvd_vertex_seq_prep_ro(ems_u32* p) {
  struct lvd_dev* dev=get_lvd_device(p[1]);
  
  if(p[0] == 3) {
    return lvd_vertex_seq_prep_ro(dev, p[2], p[3], 0, 0);
  }
  else if(p[0] < 6) {
    return plErr_ArgNum;
  }
  return lvd_vertex_seq_prep_ro(dev, p[2], p[3], p[0]-3, &p[4]);
}
plerrcode
test_proc_lvd_vertex_seq_prep_ro(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;
    
    if (p[0]!=3 && p[0] < 6)
        return plErr_ArgNum;
    
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        printf("proc_lvd_vertex_seq_prep_ro: branch %d is not a valid LVD device\n", p[1]);
        *outptr++=1;
        return pres;
    }
    if (p[2]<16) {
        pres=verify_lvd_ids(dev, p[2], mtypes);
        if (pres!=plOK) {
	  printf("test_proc_lvd_vertex_seq_prep_ro is not ZEL_LVD_ADC_VERTEX[M3]\n");
            *outptr++=2;
            return pres;
        }
    }
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_vertex_seq_prep_ro[] = "lvd_vertex_seq_prep_ro";
int ver_proc_lvd_vertex_seq_prep_ro = 1;
/*****************************************************************************/
/* 
 * p[0]: argcount == 5
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * p[3]: unit, (1-LV, 2-HV, 3-both)
 * p[4] - channel to be selected, if (-1) - deselect channels
 * p[5] - if 1, channel p[4] is selected in all chips
 *
 *  For VA32TA2 chips switch values before execute this function the vtx_info structure
 *  members must be correctly set:
 *  info[idx].sel_tp[0]   - select chan 0 in vata chip 0
 *  info[idx].sel_tp[1]   - select chan 0 in N vata chips, N-1 must be in lp_count[0]
 *  info[idx].sel_tp[2]   - select chan 0 and N VA clks, N-1 must be in lp_count[0]
 *
 *  info[idx].vaclk_tp[0] - single VA clock in test mode 
 *  info[idx].vaclk_tp[1] - N VA clks, N-1 must be set in lp_count[0]
 * Zero in any above member means that it is not set!
 *  
 * For MATE3 chips it is not yet implemented.
 */
plerrcode
proc_lvd_vertex_sel_chan_tp(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    return lvd_vertex_sel_chan_tp(dev, p[2], p[3], &p[4]);
}
plerrcode
test_proc_lvd_vertex_sel_chan_tp(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;
  
    if (p[0] != 5) {
      printf("test_proc_lvd_vertex_sel_chan_tp: plErr_ArgNum\n");
      return plErr_ArgNum;
    }
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        printf("proc_lvd_vertex_sel_chan_tp: branch %d is not a valid LVD device\n", p[1]);
        *outptr++=1;
        return pres;
    }
    if (p[2]<16) {
        pres=verify_lvd_ids(dev, p[2], mtypes);
        if (pres!=plOK) {
	  printf("test_proc_lvd_vertex_sel_chan_tp is not ZEL_LVD_ADC_VERTEX[M3]\n");
            *outptr++=2;
            return pres;
        }
    }
    if((p[3] < 1) || (p[3]>2)) {
      *outptr++=3;
      return pres;
    }
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_vertex_sel_chan_tp[] = "lvd_vertex_sel_chan_tp";
int ver_proc_lvd_vertex_sel_chan_tp = 1;

/*****************************************************************************
 * par             : array of parameters, par == 0 - load dummy prog file only!
 * p[0]            : argcount ()
 * p[1]            : branch
 * p[2]            : module addr (absolute address or -1 for all modules)
 * p[3]            : unit, (1-LV, 2-HV, 3-both)
 * p[4]            : chip type: VTX_CHIP_NONE|VTX_CHIP_VA32TA2|VTX_CHIP_MATE3
 *
 *  for VTX_CHIP_VA32TA2:
 * p[5]            : master reset switch
 * p[6]            : read-out reset switch
 *      {len, ro[len]}
 * p[7] = nro      : number of read-out params, now 3
 * p[8]..p[7+nr0]  : read-out params: sw, aclk, gap between chips 
 *      {len, sel[len]}
 * p[8+nro] = nsel                           : number of select channel sequencer switches
 * p[9+nro]..p[8+nro+nsel]                   : switch values for select functions
 *      {len, va[len]}
 * p[9+nro+nsel] = nva                       : number of VA clk switch functions
 * p[10+nro+nsel]..p[9+nro+nsel+nva]         : VA clk switch functions
 *      {len, prog[len]}
 * p[10+nro+nsel+nva]= np                    : number of 16-bit words to be loaded, seq program
 * p[11+nro+nsel+nva]..p[10+nro+nsel+nva+np] : "sequencer program"
 *
 * for VTX_CHIP_MATE3 boards:
 *
 * p[5]            : read-out reset switch
 *      {len, ro[len]}
 * p[6] = nro      : number of read-out params, now 3
 * p[7]..p[6+nr0]  : read-out params: sw, aclk, gap between chips (p[7], p[8], p[9])
 *      {len, prog[len]}
 * p[7+nro]= np    : number of 16-bit words to be loaded, seq program (p[10])
 * p[8+nro]..p[8+nro+np+1] : "sequencer program" (p[11] -...)



*/

/* {len, ro[len]}   : ro params, len=3 : p[7] - p[10] 
 * {len, sel[len]}  : sel_tp params, len =3, p[11] - p[14]
 * {len, va[len]}   : vaclk, len=2, p[15] - p[17]
 * {len, prog[len]} : len=xxxx, p[18] - ....
*/


plerrcode proc_lvd_vertex_seq_init(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    if(p[0] < 4) { /* dummy init, for unused modules */
      return lvd_vertex_seq_init(dev, p[2], p[3], 0);
    }
    return lvd_vertex_seq_init(dev, p[2], p[3], &p[4]);
}

plerrcode test_proc_lvd_vertex_seq_init(ems_u32* p)
{ 
  struct lvd_dev* dev;
  plerrcode pres;
  int ro_parms  = 4;
  int sel_funcs = 3;
  int va_funcs  = 2; 
  int sel_ind   = 0;
  int va_ind    = 0;
  int prog_ind  = 0;

  if ((p[0] == 3) || ((p[0] == 4) && (p[4]==0))) { /* dummy/unused unit */
    p[0] = 3;
    return plOK;
  }
  if(p[0] < 11) { /* arbitrary limit, nothing precise could be here */
      printf("test_proc_lvd_vertex_seq_init: too short arg list\n");
      return plErr_ArgNum;
  }
  if((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
    *outptr++ = 1;
    return pres;
  }
    
  if((p[3] < 1) || (p[3] > 2)) {
    printf("test_proc_lvd_vertex_seq_init: unit must be 1 or 2, not: %1d", p[3]);
    return plErr_ArgRange;
  }

  if(p[4] == 1) { /* VA32TA2 chips */
    pres=verify_lvd_id(dev, p[2], ZEL_LVD_ADC_VERTEX);
    if (pres!=plOK) {
      pres=verify_lvd_id(dev, p[2], ZEL_LVD_ADC_VERTEXUN);
    }
    if (pres!=plOK)
        return pres;

    if(p[5]==0) {
      printf("test_proc_lvd_vertex_seq_init: master reset switch must be not 0, error\n");
      return plErr_ArgRange;
    }
    if(p[6]==0) {
      printf("test_proc_lvd_vertex_seq_init: read-out  reset switch must be not 0, err in arg 6\n");
      return plErr_ArgRange;
    }
    if(p[7] > ro_parms) {
      printf("test_proc_lvd_vertex_seq_init: read-out param. number %1d > %1d, err in arg 7\n",
	     p[7], ro_parms);
      return plErr_ArgRange;
    }
    if(!p[8]) {
      printf("test_proc_lvd_vertex_seq_init: read-out switch must be not 0, err in arg_9 == 0\n");
      return plErr_ArgRange;
    }

    sel_ind = 8 + p[7]; /* 11 */
    if(p[sel_ind] != sel_funcs) {
      printf("test_proc_lvd_vertex_seq_init: sel_funcs num. %1d != %1d, err in arg %1d\n",
	     p[sel_ind], sel_funcs, sel_ind);
      return plErr_ArgRange;
    }
    va_ind = sel_ind + sel_funcs + 1; /* 15 */ 
    if(p[va_ind] != va_funcs) {
      printf("test_proc_lvd_vertex_seq_init: va_funcs num. %1d != %1d, err in arg %1d\n",
	     p[va_ind], va_funcs, va_ind);
      return plErr_ArgRange;
    }
    prog_ind = va_ind + va_funcs + 1; /*18, contains number of words to be loaded in seq. */
    if(p[prog_ind] < 1000) {
      printf("test_proc_lvd_vertex_seq_init: length of prog. %1d <1000, err in arg %1d\n",
	     p[prog_ind], prog_ind);
      return plErr_ArgRange;
    }
  }
  else if(p[4] == 2) { /* MATE3 chips */
    pres=verify_lvd_id(dev, p[2], ZEL_LVD_ADC_VERTEXM3);
    if (pres!=plOK) {
      pres=verify_lvd_id(dev, p[2], ZEL_LVD_ADC_VERTEXUN);
    }
    if (pres!=plOK)
        return pres;
    if(p[5]==0) {
      printf("test_proc_lvd_vertex_seq_init: read-out  reset switch must be not 0, err in arg 5\n");
      return plErr_ArgRange;
    }
    if(p[6] > ro_parms) {
      printf("test_proc_lvd_vertex_seq_init: read-out param. number %1d > %1d, err in arg 6\n",
	     p[6], ro_parms);
      return plErr_ArgRange;
    }
    if(!p[7]) {
      printf("test_proc_lvd_vertex_seq_init: read-out switch must be not 0, err in arg 7 == 0\n");
      return plErr_ArgRange;
    }
    prog_ind = 7+p[6]; /*10, contains number of words to be loaded in seq. */
    if(p[prog_ind] < 1000) {
      printf("test_proc_lvd_vertex_seq_init: length of prog. %1d <1000, err in arg %1d\n",
	     p[prog_ind], prog_ind);
      return plErr_ArgRange;
    }
  } else {
    printf("test_proc_lvd_vertex_seq_init: load idle\n");
  }
  wirbrauchen=0;
  return plOK;
}

char name_proc_lvd_vertex_seq_init[] = "lvd_vertex_seq_init";
int ver_proc_lvd_vertex_seq_init = 1;

plerrcode proc_lvd_vertex_module_start(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    return lvd_vertex_module_start(dev, p[2]);
}

plerrcode test_proc_lvd_vertex_module_start(ems_u32* p)
{  
  struct lvd_dev* dev;
  plerrcode pres;
  if (p[0] != 2) {
    return plErr_ArgNum;
  }
  if((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
    *outptr++ = 1;
    return pres;
  }
  if(p[2] < 16) {
    pres=verify_lvd_ids(dev, p[2], mtypes);
    if (pres!=plOK)
      return pres;
  }
  wirbrauchen=0;
  return plOK;
}

char name_proc_lvd_vertex_module_start[] = "lvd_vertex_module_start";
int ver_proc_lvd_vertex_module_start = 1;

/*****************************************************************************
 * p[0]            : argcount ()
 * p[1]            : branch
 * p[2]            : module addr (absolute address or -1 for all modules)
*/

plerrcode proc_lvd_vertex_start_init_mode(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    return lvd_vertex_start_init_mode(dev, p[2]);
}

plerrcode test_proc_lvd_vertex_start_init_mode(ems_u32* p)
{  
  struct lvd_dev* dev;
  plerrcode pres;
  if (p[0] != 2) {
    return plErr_ArgNum;
  }
  if((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
    *outptr++ = 1;
    return pres;
  }
  if(p[2] < 16) {
    pres=verify_lvd_ids(dev, p[2], mtypes);
    if (pres!=plOK)
      return pres;
  }
  wirbrauchen=0;
  return plOK;
}

char name_proc_lvd_vertex_start_init_mode[] = "lvd_vertex_start_init_mode";
int ver_proc_lvd_vertex_start_init_mode = 1;

/*****************************************************************************
 * p[0]            : argcount ()
 * p[1]            : branch
 * p[2]            : module addr (absolute address or -1 for all modules)
*/

plerrcode proc_lvd_vertex_module_stop(ems_u32* p)
{
  plerrcode res;
  struct lvd_dev* dev=get_lvd_device(p[1]);
  res = lvd_vertex_module_stop(dev, p[2]);
  return res;
}

plerrcode test_proc_lvd_vertex_module_stop(ems_u32* p)
{  
  struct lvd_dev* dev;
  plerrcode pres;
  if (p[0] != 2) {
    return plErr_ArgNum;
  }
  if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
    *outptr++=1;
    return pres;
  }
  if(p[2] < 16) {
    pres=verify_lvd_ids(dev, p[2], mtypes);
    if (pres!=plOK)
      return pres;
  }
  
  wirbrauchen=0;
  return plOK;
}

char name_proc_lvd_vertex_module_stop[] = "lvd_vertex_module_stop";
int ver_proc_lvd_vertex_module_stop = 1;

/*****************************************************************************
 * p[0] : argcount ()
 * p[1] : branch
 * p[2] : module addr (absolute address or -1 for all modules)
 * p[3] : unit, 1 - LV, 2 - HV, 3 - both
*/

plerrcode proc_lvd_vertex_mreset(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    return lvd_vertex_mreset(dev, p[2], p[3]);
}

plerrcode test_proc_lvd_vertex_mreset(ems_u32* p)
{  
  struct lvd_dev* dev;
  plerrcode pres;

  if (p[0] != 3) {
    return plErr_ArgNum;
  }
  if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
    *outptr++=1;
    return pres;
  }
  pres=verify_lvd_ids(dev, p[2], mtypes);
  if (pres!=plOK) {
    *outptr++ = 1;
    return pres;
  }  
  if(p[2] < 16) {
    pres=verify_lvd_ids(dev, p[2], mtypes);
    if (pres!=plOK)
      *outptr++ = 2;
      return pres;
  }
  if((p[3]<1) || (p[3]>3)) {
    *outptr++ = 3;
    return plErr_ArgRange;
  }
  wirbrauchen=0;
  return plOK;
}
char name_proc_lvd_vertex_mreset[] = "lvd_vertex_mreset";
int ver_proc_lvd_vertex_mreset = 1;
/*****************************************************************************
 * p[0] : argcount ()
 * p[1] : branch
 * p[2] : module addr (absolute address or -1 for all modules)
 * p[3] : unit, 1 - LV, 2 - HV, 3 - both
*/

plerrcode proc_lvd_vertex_ro_reset(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    return lvd_vertex_ro_reset(dev, p[2], p[3]);
}

plerrcode test_proc_lvd_vertex_ro_reset(ems_u32* p)
{  
  struct lvd_dev* dev;
  plerrcode pres;
    if (p[0] != 3) {
      return plErr_ArgNum;
    }
    if((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
      *outptr++ = 1;
      return pres;
    }
    if(p[2] < 16) {
      pres=verify_lvd_ids(dev, p[2], mtypes);
      if (pres!=plOK)
	*outptr++ = 2;
	return pres;
    }
    if((p[3]<1) || (p[3]>3)) {
      *outptr++ = 3;
      return plErr_ArgRange;
    }
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_vertex_ro_reset[] = "lvd_vertex_ro_reset";
int ver_proc_lvd_vertex_ro_reset = 1;

/*****************************************************************************
 * p[0] : argcount ()
 * p[1] : branch
 * p[2] : module addr (absolute address or -1 for all modules)
 * p[3] : unit, 1 - LV, 2 - HV, 3 - both
 * p[4] : switch value to be loaded and executed
*/

plerrcode proc_lvd_vertex_exec_seq_func(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    return lvd_vertex_exec_seq_func(dev, p[2], p[3], p[4]);
}

plerrcode test_proc_lvd_vertex_exec_seq_func(ems_u32* p)
{  
  struct lvd_dev* dev;
  plerrcode pres;
  if (p[0] != 4) {
    return plErr_ArgNum;
  }
  if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
    *outptr++=1;
    return pres;
  }  
  if(p[2] < 16) {
    pres=verify_lvd_ids(dev, p[2], mtypes);
    if (pres!=plOK)
      *outptr++=2;
      return pres;
  }
  if((p[3]<1) || (p[3]>3)) {
    *outptr++ = 3;
    return plErr_ArgRange;
  }
  wirbrauchen=0;
  return plOK;
}

char name_proc_lvd_vertex_exec_seq_func[] = "lvd_vertex_exec_seq_func";
int ver_proc_lvd_vertex_exec_seq_func = 1;

/*****************************************************************************
 * Do nothing. Added as start_proclist cannot be empty now
 * p[0] : argcount ()
 * p[1] : branch
*/

plerrcode proc_lvd_vertex_dummy(ems_u32* p)
{
  if(!p) return plOK;
  return plOK;;
}

plerrcode test_proc_lvd_vertex_dummy(ems_u32* p)
{  
  if (p[0] != 1) {
    return plErr_ArgNum;
  }
  wirbrauchen=0;
  return plOK;
}

char name_proc_lvd_vertex_dummy[] = "lvd_vertex_dummy";
int ver_proc_lvd_vertex_dummy = 1;

/*****************************************************************************/
/* 
 * p[0]: argcount (==3/6), if 3 use vtx_info[idx].ro_par[]
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * p[3]: unit, (1-LV, 2-HV, 3-both)
 * p[4] - channel to be selected, if (-1) - deselect channels
 *
 *  For VA32TA2 chips switch values before execute this function the vtx_info structure
 *  members must be correctly set:
 *  info[idx].sel_tp[0]   - select chan 0 in vata chip 0
 *  info[idx].sel_tp[1]   - select chan 0 in N vata chips, N-1 must be in lp_count[0]
 *  info[idx].sel_tp[2]   - select chan 0 and N VA clks, N-1 must be in lp_count[0]
 *
 *  info[idx].vaclk_tp[0] - single VA clock in test mode 
 *  info[idx].vaclk_tp[1] - N VA clks, N-1 must be set in lp_count[0]
 * Zero in any above member means that it is not set!
 *  
 * For MATE3 chips it is not yet implemented.
 */
plerrcode
proc_lvd_vertex_analog_chan_tp(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    return lvd_vertex_analog_chan_tp(dev, p[2], p[3], p[4]);
}
plerrcode
test_proc_lvd_vertex_analog_chan_tp(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;
  
    if (p[0] < 4) {
      printf("test_proc_lvd_vertex_analog_chan_tp: plErr_ArgNum\n");
      return plErr_ArgNum;
    }
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        printf("proc_lvd_vertex_analog_chan_tp: branch %d is not a valid LVD device\n", p[1]);
        *outptr++=1;
        return pres;
    }
    if (p[2]<16) {
        pres=verify_lvd_id(dev, p[2], ZEL_LVD_ADC_VERTEX);
        if (pres!=plOK) {
	  pres=verify_lvd_id(dev, p[2], ZEL_LVD_ADC_VERTEXUN);
	}
	if(pres != plOK) { 
	  printf("test_proc_lvd_vertex_analog_chan_tp is not ZEL_LVD_ADC_VERTEX\n");
	  *outptr++=2;
	  return pres;
	}
    }
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_vertex_analog_chan_tp[] = "lvd_vertex_analog_chan_tp";
int ver_proc_lvd_vertex_analog_chan_tp = 1;


/*============================ MATE3 specific procedures
 */

/*****************************************************************************
 * p[0] : argcount ()
 * p[1] : branch
 * p[2] : module address, from [0,15] range
 * p[3] : unit: 1 -LV, 2 -HV other - forbidden
 * p[4] : chip number, [0,15] range
 * p[5] : chip register, [1,6] for MATE3, formally in [0,7] range
*/

plerrcode proc_lvd_vertex_read_mate3_reg(ems_u32* p) {
  struct lvd_dev* dev = get_lvd_device(p[1]);
  plerrcode pres = lvd_vertex_read_mate3_reg(dev, p[2], p[3]-1, p[4], p[5], outptr);
  if(pres == plOK) {
    outptr++;
  }
  return pres;
}

plerrcode test_proc_lvd_vertex_read_mate3_reg(ems_u32* p) {  
  struct lvd_dev* dev;
  plerrcode pres;

  if (p[0] != 5) {
    return plErr_ArgNum;
  }
  if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
    printf("lvd_vertex_read_mate3_reg: branch %d is not a valid LVD device\n", p[1]);
    *outptr++=1;
    return pres;
  }
  if(p[2] > 15) {
    printf("lvd_vertex_read_mate3_reg: module addr=0x%1x - error\n", p[2]);
    *outptr++=2;
    return plErr_ArgRange;  
  }
  if((pres=verify_lvd_id(dev, (p[2]&0xf), ZEL_LVD_ADC_VERTEXM3)) != plOK) {
    pres=verify_lvd_id(dev, (p[2]&0xf), ZEL_LVD_ADC_VERTEXUN);
  }
  if(pres!=plOK) {
    printf("lvd_vertex_read_mate3_reg is not ZEL_LVD_ADC_VERTEXM3/UN\n");
    *outptr++=3;
    return pres;
  } 
  if((p[3] < 1) || (p[3] > 2)) {
    printf("lvd_vertex_read_mate3_reg: invalid unit %1d in module\n", p[3]);
    *outptr++=4;
    return plErr_ArgRange;
  }
  if(p[4] > 15) {
    printf("lvd_vertex_read_mate3_reg: chip addr>15\n");
    *outptr++=5;
    return plErr_ArgRange;
  }
  if(p[5] < 1 || p[5] >6) {
    printf("lvd_vertex_read_mate3_reg: invalid register number: %1d\n",
	   p[5]);
    *outptr++=6;
    return plErr_ArgRange;
  }
  wirbrauchen=1;
  return plOK;
}

char name_proc_lvd_vertex_read_mate3_reg[] = "lvd_vertex_read_mate3_reg";
int ver_proc_lvd_vertex_read_mate3_reg = 1;



/*****************************************************************************
 * p[0] : argcount ()
 * p[1] : branch
 * p[2] : module address, from [0,15] range
 * p[3] : unit: 1 -LV, 2 -HV, other - forbidden
 * RETURNS data array:
 *  0 - number of register data words
 *  1 - number of invalid data words, inv. one == -1
 *  2... [0] - register data words
*/

plerrcode proc_lvd_vertex_read_mate3_board_reg(ems_u32* p) {
  struct lvd_dev* dev = get_lvd_device(p[1]);
  ems_u32 buf[128]; /* for output data, maximum size*/
  buf[0] = 126;
  plerrcode pres = lvd_vertex_read_mate3_board_reg(dev, p[2], p[3]-1, buf);
  if(pres == plOK) {
    int i;
    *outptr++ =buf[0];
    *outptr++ =buf[1];
    for(i=2; i<buf[0]+2; i++) { 
      *outptr++ = buf[i];
    }
  }
  return pres;
}

plerrcode test_proc_lvd_vertex_read_mate3_board_reg(ems_u32* p) {  
  struct lvd_dev* dev;
  plerrcode pres;

  if (p[0] != 3) {
    return plErr_ArgNum;
  }
  if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
    *outptr++=1;
    return pres;
  }
  if(p[2] > 15) {
    *outptr++=2;
    return plErr_ArgRange;  
  }
  pres=verify_lvd_id(dev, p[2], ZEL_LVD_ADC_VERTEXM3);
  if (pres!=plOK) {
    pres=verify_lvd_id(dev, p[2], ZEL_LVD_ADC_VERTEXUN);
  }
  if(pres!=plOK) {
    return pres;
  }
  if((p[3] < 1) || (p[3]>2)) {
    *outptr++=3;
    return plErr_ArgRange; 
  } 
  wirbrauchen= -1;
  return plOK;
}
char name_proc_lvd_vertex_read_mate3_board_reg[] = "lvd_vertex_read_mate3_board_reg";
int ver_proc_lvd_vertex_read_mate3_board_reg = 1;

/*****************************************************************************
 * The called function make MRESET for MATE3 board, i.e
 * re-assign addresses. In case of error returned, chips may be not set
 * at all!
 * p[0] : argcount ()
 * p[1] : branch
 * p[2] : module address, from [0,15] range
 * p[3] : unit: 1 -LV, 2 -HV other - forbidden
 * p[4]... : content of regs: {r1,..,r6}, {r1,..,r6}, ....
 *           only (rX & 0xff) is valid. No chip and register numbers
 *           required now in data word. Everything will be done 
 *           in a function. Later, probably we will pack 4 valid bytes
 *           in one 32 bit word...
*/

plerrcode proc_lvd_vertex_init_mate3(ems_u32* p) {
  struct lvd_dev* dev = get_lvd_device(p[1]);
  plerrcode pres = lvd_vertex_init_mate3(dev, p[2], p[3], p[0]-3, p+4);
  return pres;
}

plerrcode test_proc_lvd_vertex_init_mate3(ems_u32* p) {  
  struct lvd_dev* dev;
  plerrcode pres;

  if (p[0] < 10) {
    printf("lvd_vertex_init_mate3: at least 10 args needed, here: %1d\n", p[0]);
    return plErr_ArgNum;
  }
  if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
    printf("lvd_vertex_init_mate3: branch %d is not a valid LVD device\n", p[1]);
    *outptr++=1;
    return pres;
  }
  if(p[2] > 15) {
    printf("lvd_vertex_init_mate3: module addr <0 not yet implemented\n");
    *outptr++=2;
    return plErr_ArgRange;  
  }
  else {
    if(p[2] > 15) {
      printf("lvd_vertex_init_mate3: module addr > 15, will use addr&0xf = 0x%1x\n",
	     (p[2] & 0xf));
    }
    pres=verify_lvd_id(dev, (p[2]&0xf), ZEL_LVD_ADC_VERTEXM3);
    if (pres!=plOK) {
      printf("test_proc_lvd_vertex_init_mate3: at addr: 0x%1x  VERTEXM3 not found\n",
	     (p[2]&0xf));
      *outptr++=3;
      return pres;
    } 
  }
  if((p[3] < 1) || (p[3] > 2)) {
    printf("lvd_vertex_init_mate3: invalid unit %1d in module, 1 or 2 allowed!\n", p[3]);
    *outptr++=4;
    return plErr_ArgRange;
  }
  if((p[0]-3) < 6) {
    printf("lvd_vertex_init_mate3: for one chip at least 6 words needed, here %1d\n", (p[0]-3));
    *outptr++=5;
    return plErr_ArgRange;
  }

  wirbrauchen=0;
  return plOK;
}

char name_proc_lvd_vertex_init_mate3[] = "lvd_vertex_init_mate3";
int ver_proc_lvd_vertex_init_mate3 = 1;

/*****************************************************************************
 * The called function make MRESET for MATE3 board, i.e
 * re-assign addresses. In case of error returned, chips may be not set
 * at all!
 * p[0] : argcount ()
 * p[1] : branch
 * p[2] : module address, from [0,15] range
 * p[3] : unit, 1 - LV, 2 - HV, 3 - error
 * p[4] : acknowledge delay from [0,7] range
*/

plerrcode proc_lvd_vertex_i2c_delay(ems_u32* p) {
  struct lvd_dev* dev = get_lvd_device(p[1]);
  return lvd_vertex_i2c_delay(dev, p[2], p[3], p[4]);
}

plerrcode test_proc_lvd_vertex_i2c_delay(ems_u32* p) {  
  struct lvd_dev* dev;
  plerrcode pres;

  if (p[0] != 4) {
    printf("lvd_vertex_changei2c_ackw_delay: 4 args needed, here: %1d\n", p[0]);
    return plErr_ArgNum;
  }
  if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
    printf("lvd_vertex_i2c_delay: branch %d is not a valid LVD device\n", p[1]);
    return pres;
  }
  if(p[2] >15) {
    printf("lvd_vertex_i2c_delay: module addr <0 not yet implemented\n");
    return plErr_ArgRange;  
  }
  else {
    if(p[2] > 15) {
      printf("lvd_vertex_i2c_delay: module addr > 15, will use addr&0xf = 0x%1x\n",
	     (p[2] & 0xf));
    }
    pres=verify_lvd_id(dev, (p[2]&0xf), ZEL_LVD_ADC_VERTEXM3);
    if (pres!=plOK) {
      printf("test_proc_lvd_vertex_i2c_delay: at addr: 0x%1x  VERTEXM3 not found\n",
	     (p[2]&0xf));
      return pres;
    } 
  }
  if((p[3] < 1) || (p[3] > 2)) {
    printf("lvd_vertex_i2c_delay: invalid unit %1d in module, 1 or 2 allowed!\n", p[3]);
    *outptr++=4;
    return plErr_ArgRange;
  }
  wirbrauchen=0;
  return plOK;
}

char name_proc_lvd_vertex_i2c_delay[] = "lvd_vertex_i2c_delay";
int ver_proc_lvd_vertex_i2c_delay = 1;
/*==========*/

/*****************************************************************************
 * Affects both parts of modules VertexADCM3 and VertexADCUN!
 * p[0] : argcount ()
 * p[1] : branch
 * p[2] : module address, from [0,15] range
 * p[3] : period of I2C clocks, in  [0,15] range, if absent - read,decode and print
*/

plerrcode proc_lvd_vertex_i2c_period(ems_u32* p) {
  plerrcode pres;
  struct lvd_dev* dev = get_lvd_device(p[1]);
  if ((pres=verify_lvd_id(dev, p[2], ZEL_LVD_ADC_VERTEXM3)) == plOK) {
    if(p[0] > 2) { /* write */
      pres = lvd_vertex_i2c_period_m3(dev, p[2], p[3]);
    }
    else {         /* read */
      pres = lvd_vertex_i2c_get_period_m3(dev, p[2], outptr);
      if(pres == plOK)
	outptr++;
    }
  }
  else {  
    if((pres=verify_lvd_id(dev, p[2], ZEL_LVD_ADC_VERTEXUN))== plOK) {
      if(p[0] > 2) { /* write */
	pres = lvd_vertex_i2c_period_un(dev, p[2], p[3]);
      }
      else {  /* read */
      pres = lvd_vertex_i2c_get_period_un(dev, p[2], outptr);
      if(pres == plOK)
	outptr++;
      }
    }
    else {
      printf("test_proc_lvd_vertex_i2c_period: addr: 0x%1x not for MATE3! error\n",
	     p[2]);
      pres = plErr_BadModTyp;
    }
  }
  return pres;
}

plerrcode test_proc_lvd_vertex_i2c_period(ems_u32* p) {  
  struct lvd_dev* dev;
  plerrcode pres;

  if ((p[0] < 2) || (p[0] > 3)) {
    return plErr_ArgNum;
  }
  if ((pres=find_lvd_odevice(p[1], &dev)) != plOK) {
    return pres;
  }
  if((p[2] < 0) || (p[2] > 15)) {
    return plErr_ArgRange;  
  }
  wirbrauchen=(p[0]==2) ? 1 : 0;
  return plOK;
}

char name_proc_lvd_vertex_i2c_period[] = "lvd_vertex_i2c_period";
int ver_proc_lvd_vertex_i2c_period = 1;

/*****************************************************************************
 * The called function does NOT make MRESET for MATE3 board. 
 * It only re-write register of chip(s).
 * p[0]        : argcount ()
 * p[1]        : branch
 * p[2]        : module address, from [0,15] range
 * p[3]        : unit: 1 -LV, 2 -HV other - forbidden
 * p[4]        : chip number, if (-1) - all chips in the board
 * p[5]...p[10]: content of regs: {r1,..,r6}
 *           only (rX & 0xff) is valid. No chip and register numbers
 *           required now in data word. Everything will be done 
 *           in a function. Later, probably we will pack 4 valid bytes
 *           in one 32 bit word...
*/

plerrcode proc_lvd_vertex_load_mate3_chip(ems_u32* p) {
  struct lvd_dev* dev = get_lvd_device(p[1]);
  plerrcode pres = lvd_vertex_load_mate3_chip(dev, p[2], p[3], p[4], p[0]-4, p+5);
  return pres;
}

plerrcode test_proc_lvd_vertex_load_mate3_chip(ems_u32* p) {  
  struct lvd_dev* dev;
  plerrcode pres;

  if (p[0] < 10) {
    printf("lvd_vertex_load_mate3_chip: at least 9 args needed, here: %1d\n", p[0]);
    return plErr_ArgNum;
  }
  if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
    printf("lvd_vertex_load_mate3_chip: branch %d is not a valid LVD device\n", p[1]);
    *outptr++=1;
    return pres;
  }
  if(p[2] >15) {
    printf("lvd_vertex_load_mate3_chip: module addr <0 not yet implemented\n");
    *outptr++=2;
    return plErr_ArgRange;  
  }
  if((pres=verify_lvd_id(dev, p[2], ZEL_LVD_ADC_VERTEXM3)) != plOK) {
    pres=verify_lvd_id(dev, p[2], ZEL_LVD_ADC_VERTEXUN);
  }
  if(pres != plOK) {
    printf("test_proc_lvd_vertex_analog_chan_tp: at addr: 0x%1x  VERTEXM3 not found\n",
	   (p[2]&0xf));
    *outptr++=3;
    return pres;
  } 
  if((p[3] < 1) || (p[3] > 2)) {
    printf("lvd_vertex_load_mate3_chip: invalid unit %1d in module, 1 or 2 allowed!\n", p[3]);
    *outptr++=4;
    return plErr_ArgRange;
  }
  if(p[4]>15) {
    printf("lvd_vertex_load_mate3_chip: invalid chip number %1d in module 0x%1x!\n", 
	   p[4], p[2]);
    *outptr++=5;
    return plErr_ArgRange;
  }

  if((p[0]-4) < 6) {
    printf("lvd_vertex_load_mate3_chip: for chip at least 6 words needed, here %1d, addr 0x%1x\n",
	   (p[0]-3), p[2]);
    *outptr++=6;
    return plErr_ArgRange;
  }

  wirbrauchen=0;
  return plOK;
}

char name_proc_lvd_vertex_load_mate3_chip[] = "lvd_vertex_load_mate3_chip";
int ver_proc_lvd_vertex_load_mate3_chip = 1;

/*****************************************************************************/
/*
 * p[0]: argcount (==5)
 * p[1]: branch
 * p[2]: module addr (in [0,15] range)
 * p[3]: unit (1: LV, 2: HV)
 * p[4]: val
 */
plerrcode
proc_lvd_vertex_xclk_delay(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    return lvd_vertex_xclk_delay(dev, (p[2]&0xf), p[3], p[4]);
}

plerrcode
test_proc_lvd_vertex_xclk_delay(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;
  
    if (p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK) {
        printf("vertex_xclk_delay: branch %d is not a valid LVD device\n", p[1]);
        *outptr++=1;
        return pres;
    }
    if(p[2] > 15) {
      *outptr++=2;
      return plErr_ArgRange;
    }
    pres=verify_lvd_id(dev, (p[2]&0xf), ZEL_LVD_ADC_VERTEXM3);
    if(pres != plOK) {
      pres=verify_lvd_id(dev, (p[2]&0xf), ZEL_LVD_ADC_VERTEXUN);	  
      
    }
    if (pres!=plOK) {
      *outptr++=2;
      return pres;
    }
    if ((p[3]) < 1 || (p[3]>2)) {
      *outptr++=3;
      return plErr_ArgRange;
    }
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_vertex_xclk_delay[] = "lvd_vertex_xclk_delay";
int ver_proc_lvd_vertex_xclk_delay = 1;
