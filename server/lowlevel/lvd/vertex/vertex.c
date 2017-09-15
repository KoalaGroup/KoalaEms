/*
 * lowlevel/lvd/vertex/vertex.c
 * created 2005-Feb-25 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: vertex.c,v 1.57 2011/10/17 22:53:55 wuestner Exp $";

/* If controller is in DDMA(async readout) mode,
 * i.e LVD_SIS1100_MMAPPED is defined
 * (struct lvd_dev* dev->info->ddma_active != 0)
 * lvd_i_r() may return error code. It is not foreseen to use
 * this function (as well as  lvd_i_w()) in async mode.
 */

#define LOWLIB
#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include "../../../commu/commu.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <modultypes.h>
#include <rcs_ids.h>
#include "vertex.h"
#include "../lvd_access.h"
#include "../lvd_initfuncs.h"
#ifdef LVD_SIS1100
 #include "../sis1100/sis1100_lvd.h"
#endif

#ifdef DMALLOC
 #include <dmalloc.h>
#endif


#include "../lvd_vertex_map.h"

/* #define VTX_DEBUG 0
 */
#define iMAX(m, n)  ({ int _m=m, _n=n; _m>_n?m:n; })
#define iMIN(m, n)  ({ int _m=m, _n=n; _m<_n?m:n; })


/* bits of vtx_info[].loaded */
#define SEQ_LOADED    0x1  /* idle loop loaded, no one seq.function can be used */
#define CODE_LOADED   0x2  /* seq program for read-out loaded */
#define DACS_LOADED   0x4  /* DACs of front-end board is loaded */
#define CHIP_LOADED   0x8  /*  */
#define PED_LOADED   0x10
#define THR_LOADED   0x20
#define SEQ_READY  0x8000 /* must be set manually, when loading is completed */

#define IS_SEQ_LOADED(_v)  (((_v) & (SEQ_LOADED | CODE_LOADED)) ? 1 : 0)
#define IS_DACS_LOADED(_v) (((_v) & DACS_LOADED) ? 1 : 0)

int const VTX_INIT_MODE = VTX_CR_RUN    |
                          VTX_CR_RAW    |
                          VTX_CR_IH_LV  | VTX_CR_IH_HV | 
                          VTX_CR_IH_TRG;

int const VTX_STOP_MODE = VTX_CR_RAW    |
                          VTX_CR_IH_LV  | VTX_CR_IH_HV |
                          VTX_CR_IH_TRG;


#define VA32TA2_BITS        175
#define VA32TA2_CHAN         32

#define MATE3_REG_WORDS       6
#define MATE3_CHAN           16
#define MATE3_REG_SHIFT       8
#define MATE3_CHIP_SHIFT     11

/* macros for i2c_data */
#define  SET_I2C_CSR_DATA(_d, _c, _r) ( ((_d) & 0xff) | \
                                        (((_r) & 0x7) << MATE3_REG_SHIFT) | \
                                        (((_c) & 0xf) << MATE3_CHIP_SHIFT) )

#define  GET_I2C_CSR_REG(_d)  ( ((_d) >> MATE3_REG_SHIFT)  & 0x7)
#define  GET_I2C_CSR_CHIP(_d) ( ((_d) >> MATE3_CHIP_SHIFT) & 0xf)
#define  GET_I2C_CSR_DATA(_d) ( (_d) & 0xff)

#define MATE3_ACK_PERIOD_MASK 0x7
#define MATE3_ACK_PERIOD_SHIFT  8
#define  SET_I2C_ACK_PERIOD(_b, _v) (((_b) & (~(MATE3_ACK_PERIOD_MASK << MATE3_ACK_PERIOD_SHIFT))) | \
                                    (((_v) & MATE3_ACK_PERIOD_MASK)   << MATE3_ACK_PERIOD_SHIFT))
#define  GET_I2C_ACK_PERIOD(_b)     (((_b) >> MATE3_ACK_PERIOD_SHIFT) & MATE3_ACK_PERIOD_MASK)

#define MAX_MATE3_ACK_DELAY 0x7

static enum {ACK_MIN=0, ACK_MAX, ACK_OPT, ACK_CHIP, ACK_SCALING , ACK_LEN} ACK_DELAY_IND;
static const ems_u32 def_scaling = 0x1f77; /* for VERTEXM3 */

struct vtx_spec {
  int      seq_addr;          /* start address of sequencer RAM      */
  int      ped_addr;          /* start address of of pedestal  buffer */
  int      thr_addr;          /* start address of of threshold buffer */
  int      adc_offs;          /* poti/poti_comval address offset */
  int      lp_count[2];       /* address of loop counters */
  int      ser_csr;           /* offset to reg_cr/i2c_csr/ser_csr     to load chips */
  int      ser_data;          /* offset to reg_data/i2c_data/ser_data to load chips */
  int      rep_dacs;          /* offset to register to write DACS of repeater card */
  int      aclk_par;          /* offset to register to read/write ADC clock params */
};

typedef plerrcode (*seqinit)(struct lvd_dev*, struct lvd_acard*, int, ems_u32*);
typedef int       (*roprep)(struct lvd_dev*, struct lvd_acard*, int); /* last - idx */

struct vtx_info;

typedef void*      (*allocpar)(int);
typedef void       (*freepar)(void*); /* pointer to vtx_info[].seq_par */
typedef void       (*setropar)(struct vtx_info* info, ems_u32 len, ems_u32* par);
typedef plerrcode  (*fe_reset)(struct lvd_dev*, struct lvd_acard*, int);
typedef plerrcode  (*analogtp)(struct lvd_dev*, struct lvd_acard*, int, ems_u32);
typedef plerrcode  (*seltpchan)(struct lvd_dev*, struct lvd_acard*, int, ems_u32*);
typedef plerrcode  (*loadchips)(struct lvd_dev*, struct lvd_acard*, int, int, ems_u32*);

struct vtx_info {
  int             idx;          /* to have reference to all */
  int             chiptype;
  int             numchips;
  int             numchannels;
  ems_u16         mode;         /* contains "cr reg" bits related to this sequencer  */ 
  int             mon_ch;       /* if mon_ch < 0, no channel for monitoring selected */
  int             tp_all;
  ems_u32         loaded;
  ems_u32         dac[9];       /* dac values, last loaded (with number of dac in MSB bits) */
  ems_u32         scaling;      /* 0 - uninitialized, result of scan... LV/HV. For both could be other... */
  ems_u32         delay;
  struct vtx_spec spec;

  void*           seq_par;      /* either struct va_seq_par* or struct m3_seq_par* */

  freepar         free_par;     
  roprep          prep_ro;      
  setropar        set_ro_par;   /* never dummy */ 
  fe_reset        ro_reset;
  fe_reset        m_reset;      /* master set func, only VA32TA2 chips, NOT MATE3! */ 
  seltpchan       sel_tp_chan;
  analogtp        analog_tp;
  loadchips       load_chips;
  seltpchan       set_scaling; /* set info[idx] and write scaling in module */
};

struct va_seq_par {
  ems_u32   ro_reset;                /* read-out reset */
  ems_u32     ro_len;                /* length of next array */
  ems_u32*    ro_par;                /* pointer to read-out params*/
  ems_u32   prog_len;                /* length of next array */
  ems_u32*      prog;                /* program words to be loaded in sequencer memory */

  ems_u32    m_reset;                /* master reset switch */
  ems_u32    sel_len;                /* length of next array */
  ems_u32* sel_funcs;                /* pointer to select functions */
  ems_u32     va_len;                /* length of next array */
  ems_u32*  va_funcs;                /* VA clk. tp params, first - number */
};

struct m3_seq_par {
  ems_u32   ro_reset;                /* read-out reset */
  ems_u32     ro_len;                /* length of next array */
  ems_u32*    ro_par;                /* pointer to read-out params*/

  ems_u32   prog_len;                /* length of next array */
  ems_u32*      prog;                /* program words to be loaded in sequencer memory */
};


/*
 * Static function prototypes
 */
static plerrcode exec_seq_func(struct lvd_dev* dev, struct lvd_acard* acard, 
			       int idx, ems_u32 sw, int protect);
static int       lvd_vertex_load_seq_(struct lvd_dev* dev, int addr, int seq_addr,
				      int num, ems_u32* data);

/* static void free_seq_par_dummy(void* par); */
static void free_seq_par_va(void* par);
static void free_seq_par_m3(void* par);
static void set_vtx_spec_va(struct vtx_info* info);
static void set_vtx_spec_m3(struct vtx_info* info);
static void set_vtx_spec_un(struct vtx_info* info);

static void set_vtx_va_func(struct vtx_info* info);
static void set_vtx_va_func_un(struct vtx_info* info);
static void set_vtx_m3_func(struct vtx_info* info);
static void set_vtx_m3_func_un(struct vtx_info* info);
static void set_vtx_func_dummy(struct vtx_info* info);

static struct va_seq_par* decode_va_args(ems_u32* par);
static struct m3_seq_par* decode_m3_args(ems_u32* par);
static plerrcode mreset_va(struct lvd_dev* dev, struct lvd_acard* acard, int idx);
static plerrcode ro_reset_va(struct lvd_dev* dev, struct lvd_acard* acard, int idx);
static plerrcode ro_reset_m3(struct lvd_dev* dev, struct lvd_acard* acard, int idx);

static int find_vata_chip_delay(struct lvd_dev* dev, struct lvd_acard* acard,
				int idx, int exp_bits);

static int set_mate3_chip_delay_m3(struct lvd_dev* dev, struct lvd_acard* acard, int idx,
				   int delay);
static int set_mate3_chip_delay_un(struct lvd_dev* dev, struct lvd_acard* acard, int idx,
				   int delay);
static ems_u32 read_mate3_reg(struct lvd_dev* dev, struct lvd_acard* acard,
			      int idx, int chip, int reg, ems_u32* data);

static plerrcode set_scaling_vata_va(struct lvd_dev* dev, struct lvd_acard* acard, int idx, ems_u32* p);
static plerrcode set_scaling_vata_un(struct lvd_dev* dev, struct lvd_acard* acard, int idx, ems_u32* p);
static plerrcode set_scaling_mate3_m3(struct lvd_dev* dev, struct lvd_acard* acard, int idx, ems_u32* p);
static plerrcode set_scaling_mate3_un(struct lvd_dev* dev, struct lvd_acard* acard, int idx, ems_u32* p);
static int       lvd_vertex_count_vata_bits(struct lvd_dev* dev, int addr, int idx,
					    int nrwords);
static int get_seq_csr(struct lvd_dev* dev, ems_u32 addr, int* ddma_in, ems_u32* seq_csr_val);


static void print_csr_regs(struct lvd_dev* dev, int addr, int idx, char* str) {
  ems_u32 cr, sr, seq_csr, rrr=0;
  if(lvd_i_r(dev, addr, vertex.cr, &cr)) rrr = 1;
  if(lvd_i_r(dev, addr, vertex.sr, &sr)) rrr |= 0x2;
  if(lvd_i_r(dev, addr, vertex.seq_csr, &seq_csr)) rrr |= 0x4;
  if(!str) str="";
  if(rrr) {
    printf("%s: addr=0x%1x idx=%1d CR=0x%1x SR=0x%1x SEQ_CSR=0x%1x read_err=0x%1x\n",
	   str, addr, idx, cr, sr, seq_csr, rrr);
  }
  else {
    printf("%s: addr=0x%1x idx=%1d CR=0x%1x SR=0x%1x SEQ_CSR=0x%1x\n",
	   str, addr, idx, cr, sr, seq_csr);
  }
}
/*****************************************************************************
 * set chip bit type in universal VertexADC
 *****************************************************************************/
static plerrcode set_chip_type(struct lvd_dev* dev, struct lvd_acard* acard, int idx) {
  struct vtx_info* info = acard->cardinfo;
  int addr = acard->addr;
  ems_u32 org = 0;
  if(lvd_i_r(dev, addr, vertex.unit_mode, &org) < 0) {
    printf("set_chip_type: addr=0x%1x idx=%1d : error in vertex.unit_mode read\n",
	   addr, idx);
    return plErr_HW;
  }
  if(info[idx].chiptype == VTX_CHIP_VA32TA2) {
    org |= (1<<idx);
  }
  else {
    org &= (~(1<<idx));
  }
  if(lvd_i_w(dev, addr, vertex.unit_mode, org) < 0) {
    printf("set_chip_type: addr=0x%1x idx=%1d : error in vertex.unit_mode write\n",
	   addr, idx);
    return plErr_HW;
  }
  return plOK;
}



/*****************************************************************************
 * delay in micro seconds
 *****************************************************************************/
static void u_delay(int us) {
  struct timeval a, b;
  gettimeofday(&a, 0);
  do {
    gettimeofday(&b, 0);    
  } while(((b.tv_sec-a.tv_sec)*1000000+(b.tv_usec-a.tv_usec)) <us);
}

ems_u32 lvd_vertex_thr_offs(struct lvd_dev* dev, int addr, int idx) {
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  struct vtx_info* info = (acard) ? acard->cardinfo : 0;
  if(!info) {
    printf("lvd_vertex_mode: addr=0x%1x acard=%1p info=%1p\n - error", addr, acard, info);
    return plErr_BadModTyp;
  }
  return (info[idx].spec.thr_addr - info[idx].spec.ped_addr);
}
/*****************************************************************************
 * To set control register "cr" of any VertexADC module,
 * save it also vtx_info[0/1] and set acard->initialized = 1
 * must called only after at least dummy_init was called
 *****************************************************************************/
plerrcode lvd_vertex_mode(struct lvd_dev* dev, ems_u32 addr, ems_u32 mode) {
  plerrcode pres = plOK;
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  struct vtx_info* info = (acard) ? acard->cardinfo : 0;
  if(!info) {
    printf("lvd_vertex_mode: addr=0x%1x\n - error", addr);
    return plErr_BadModTyp;
  }
  acard->daqmode = mode;
  info[0].mode = mode;
  info[1].mode = mode;
  return pres;
}

/*****************************************************************************
 * To set channel number in each chip to send test pulse for monitoring
 * save it also vtx_info[0/1]
 *****************************************************************************/
plerrcode lvd_vertex_mon_chan(struct lvd_dev* dev, ems_u32 addr, ems_u32 unit, ems_u32 ch, ems_u32 all) {
  plerrcode err = plOK;
  if((int)(addr) < 0) {
    int card;
    for (card=0; card<dev->num_acards; card++) {
      if(IsVertexCard(dev, card)) {
	plerrcode pres =  lvd_vertex_mon_chan(dev, dev->acards[card].addr, unit, ch, all);
	printf("pres=%1d addr=0x%1x set mon chan\n", pres, dev->acards[card].addr);
	if(pres != plOK) {
	  err = pres;
	}
      }
    }
  }
  else {
    struct lvd_acard* acard = GetVertexCard(dev, addr);
    struct vtx_info* info = (acard) ? acard->cardinfo : 0;
    if(!info) { // !acard->intitialized
      printf("addr=0x%1x - lvd_vertex_mon_chan null pointers\n", addr);
      return plErr_BadModTyp;
    }
    all = (all) ? 1 : 0; /* to be sure */
    if(unit & 1) {
      info[0].tp_all = all;
      info[0].mon_ch = ch;
      if(ch>=0 && all && (info[0].chiptype == VTX_CHIP_MATE3)) info[0].mon_ch = ch & 0xf;
    }
    if(unit & 2) {
      info[1].tp_all = all;
      info[1].mon_ch = ch;
      if(ch>=0 && all && (info[1].chiptype == VTX_CHIP_MATE3)) info[1].mon_ch = ch & 0xf;
    }
    return plOK;
  }
  return err; 
}

static ems_u32 prep_uw(int dac_num, int dac_value, int chan, int all) {
  ems_u32 uw =0;
  if(chan >=0) {
    uw = dac_value & 0xfff;
    uw |= ((dac_num &0x7) << 12);
    if(all) uw |= (1 <<15);
    uw |= ((chan & 0xff)  << 16);
  }
  return uw;
}

/*****************************************************************************
 * This functions initialize half of module in "idle" mode,
 * loading idle loop only. Struct info must be initialised 
 * depending on the module type (struct spec must be set properly)
 *****************************************************************************/
static int dummy_seq_init(struct lvd_dev* dev, struct lvd_acard* acard, int unit) {
  int res = 0;
#define DUMMY_SIZE  7
  ems_u32 dummy_code[DUMMY_SIZE] = { /* idle loop with halt inside, no switches */
    0xC000C000,
    0x0200C000,
    0x30000400,
    0x48024802,
    0x48024802,
    0x00004802,
    0x00000000
  };
  int num = 2*DUMMY_SIZE;
  int addr = acard->addr;
  struct vtx_info* info = (struct vtx_info*)acard->cardinfo;
  if (unit&1) {
    info[0].loaded &= (~(SEQ_LOADED | CODE_LOADED)); /* clear bits */
    res |= lvd_vertex_load_seq_(dev, addr, info[0].spec.seq_addr, num, dummy_code);
    if (res) {
      printf("dummy_seq_init: error, initialising idle card 0x%1x, LV \n", addr);
    }
    else {
      info[0].loaded |= SEQ_LOADED; /* dummy prog loaded*/
      int sw = 0x88888888;
      res = lvd_i_w(dev, addr, vertex.lv.swreg, sw); /* tsv */
    }
  }
  if(unit&2) {
    info[1].loaded &= (~(SEQ_LOADED | CODE_LOADED)); /* clear bits */
    res |= lvd_vertex_load_seq_(dev, addr, info[1].spec.seq_addr, num, dummy_code);
    if (res) {
        printf("dummy_seq_init: error initialising idle card 0x%1x, idx=1 \n", addr);
    }
    else {
      info[1].loaded |= SEQ_LOADED; /* dummy prog loaded*/
      int sw = 0x88888888;
      res = lvd_i_w(dev, addr, vertex.hv.swreg, sw);
    }
  }
  return res;
}

/**************************************************************************
 * Set of function to initialize any of  
 * VertexADC/VertexADCM3/VertexADCUN module by default, 
 * i.e. to do nothing with front-end card, as we know nothing about it yet.
 * See struct vtx_info... 
 * It is supposed, that before  execution of a  dummy function:
 *  - at least dummy sequencer idle loop is loaded,
 *  - and module is running, i.e C_RUN=1.
 * in other case, the error returned.
 * Most of dummy functions try to start idle loop.
 *
 ***************************************************************************/
static int prep_ro_dummy(struct lvd_dev* dev, struct lvd_acard* acard, int idx) {
  exec_seq_func(dev, acard, idx, 0, 0);
  return plOK;
}

static plerrcode sel_tp_chan_dummy(struct lvd_dev* dev, struct lvd_acard* acard,
				   int idx, ems_u32* par) {
  exec_seq_func(dev, acard, idx, 0, 0);
  return plOK;
}

static plerrcode ro_reset_dummy(struct lvd_dev* dev, struct lvd_acard* acard, 
				int idx) {
  exec_seq_func(dev, acard, idx, 0, 0);
  return plOK;
}
static plerrcode analog_tp_dummy(struct lvd_dev* dev, struct lvd_acard* acard, int idx, ems_u32 par) {
  exec_seq_func(dev, acard, idx, 0, 0);
  return plOK;
}

static void set_dummy_vtx_info(struct vtx_info* info) {
  info->chiptype    = VTX_CHIP_NONE;
  info->mode        = (info->idx==0) ? VTX_CR_IH_LV : VTX_CR_IH_HV;
  info->prep_ro     = prep_ro_dummy;     /* do nothing */
  info->ro_reset    = ro_reset_dummy;    /* do nothing */
  info->m_reset     = ro_reset_dummy;    /* do nothing */
  info->sel_tp_chan = sel_tp_chan_dummy; /* do nothing */
  info->analog_tp   = analog_tp_dummy;   /* do nothing */
  info->free_par    = 0;
}

static struct vtx_info* vtx_info_alloc(int mtype) {
  struct vtx_info *info;
  info = malloc(2*sizeof(struct vtx_info));
  if (!info) {
    printf("alloc_vtx_info: %s\n", strerror(errno));
    return 0;
  }
  memset(info, 0, 2*sizeof(struct vtx_info));
  info[1].idx = 1;
  set_dummy_vtx_info(&info[0]);
  set_dummy_vtx_info(&info[1]);

  if(mtype == ZEL_LVD_ADC_VERTEX) {
    set_vtx_spec_va(info);
  }
  else if(mtype == ZEL_LVD_ADC_VERTEXM3) {    
    set_vtx_spec_m3(info);
  }
  else if(mtype == ZEL_LVD_ADC_VERTEXUN){
    set_vtx_spec_un(info);
  }
  return info;
}
static void vtx_info_free(struct vtx_info* info) {
  if (info) {
    int i;
    for(i=0; i<2; i++) {
      if(info[i].free_par) { /* for dummy seq_par==0, no need to free */
	info[i].free_par(info[i].seq_par);
	info[i].seq_par = 0;
      }
    }
    free(info);
  }
}

/**************************************************************************
 * Set module specific address offsets independent on chip type
 **************************************************************************/
/* set_vtx_lp_count_offs() is common for VertexADC of any type */
static void set_vtx_lp_count_offs(struct vtx_info* info) {
  /* loop counter info, idx=0 */
  info[0].spec.lp_count[0]      = ofs(union lvd_in_card,  vertex.lv.lp_counter[0]);
  info[0].spec.lp_count[1]      = ofs(union lvd_in_card,  vertex.lv.lp_counter[1]);
  info[1].spec.lp_count[0]      = ofs(union lvd_in_card,  vertex.hv.lp_counter[0]);
  info[1].spec.lp_count[1]      = ofs(union lvd_in_card,  vertex.hv.lp_counter[1]);
}

/*set_vtx_vam3_ram_addr() is common RAM bank addresses for VertexADC and VertexADCM3 */
static void set_vtx_vam3_ram_addr(struct vtx_info* info) {
  /* RAM banks addresses, idx=0 */
  info[0].spec.seq_addr = 0;         /* start address of sequencer RAM      */
  info[0].spec.ped_addr = 0x0;       /* start address of of pedestal  buffer */
  info[0].spec.thr_addr = 0x1000;    /* start address of of threshold buffer */
  /* RAM banks addresses, idx=1 */
  info[1].spec.seq_addr = 0x080000;  /* start address of sequencer RAM      */
  info[1].spec.ped_addr = 0x100000;  /* start address of of pedestal  buffer */
  info[1].spec.thr_addr = 0x101000;  /* start address of of threshold buffer */
}
/* set_vtx_un_ram_addr(): set RAM bank addresses for VertexADCUN */
static void set_vtx_un_ram_addr(struct vtx_info* info) {
  /* ----- RAM banks addresses, idx=0 */
  info[0].spec.seq_addr = 0;         /* start address of sequencer RAM      */
  info[0].spec.ped_addr = 0;         /* start address of of pedestal  buffer */
  info[0].spec.thr_addr = 0x1000;    /* start address of of threshold buffer */
  /* ----- RAM banks addresses, idx=1 */
  info[1].spec.seq_addr = 0x040000;  /* start address of sequencer RAM      */
  info[1].spec.ped_addr = 0x080000;  /* start address of of pedestal  buffer */
  info[1].spec.thr_addr = 0x081000;  /* start address of of threshold buffer */
}

/* Addresses of VertexADC for VA32TA2 chips */
static void set_vtx_spec_va(struct vtx_info* info) {
  info[0].free_par    = free_seq_par_va;   /* to escape memory leak */
  info[1].free_par    = free_seq_par_va;   /* to escape memory leak */
  set_vtx_vam3_ram_addr(info);
  set_vtx_lp_count_offs(info);
  /* ADC offset address, idx=0 */
  info[0].spec.adc_offs = ofs(union lvd_in_card, vertex.lv.v.poti);
  info[1].spec.adc_offs = ofs(union lvd_in_card, vertex.hv.v.poti);
  /* register offsets to load chips */
  info[0].spec.ser_csr  = ofs(union lvd_in_card, vertex.lv.v.reg_cr);
  info[0].spec.ser_data = ofs(union lvd_in_card, vertex.lv.v.reg_data);
  info[1].spec.ser_csr  = ofs(union lvd_in_card, vertex.hv.v.reg_cr);
  info[1].spec.ser_data = ofs(union lvd_in_card, vertex.hv.v.reg_data);
  /* register offsets to load repeater DACS */
  info[0].spec.rep_dacs  =  ofs(union lvd_in_card,  vertex.lv.v.dac);
  info[1].spec.rep_dacs  =  ofs(union lvd_in_card,  vertex.hv.v.dac);
  /* offset to register to read/write ADC clock params */
  info[0].spec.aclk_par = ofs(union lvd_in_card, vertex.aclk_par);
  info[1].spec.aclk_par =  info[0].spec.aclk_par;
}

/* Addresses of VertexADCM3 for MATE3 chips */
static void set_vtx_spec_m3(struct vtx_info* info) {
  info[0].free_par    = free_seq_par_m3;   /* to escape memory leak */
  info[1].free_par    = free_seq_par_m3;   /* to escape memory leak */
  set_vtx_vam3_ram_addr(info);
  set_vtx_lp_count_offs(info);
  /* ADC offset address, idx=0 */
  info[0].spec.adc_offs = ofs(union lvd_in_card, vertex.lv.m.poti);
  /* ADC offset address, idx=1 */
  info[1].spec.adc_offs = ofs(union lvd_in_card, vertex.hv.m.poti);
  /* register offsets to load chips */
  info[0].spec.ser_csr  = ofs(union lvd_in_card, vertex.lv.m.i2c_csr);
  info[0].spec.ser_data = ofs(union lvd_in_card, vertex.lv.m.i2c_data);
  info[1].spec.ser_csr  = ofs(union lvd_in_card, vertex.hv.m.i2c_csr);
  info[1].spec.ser_data = ofs(union lvd_in_card, vertex.hv.m.i2c_data);
  /* register offsets to load repeater DACS */
  info[0].spec.rep_dacs  =  ofs(union lvd_in_card,  vertex.lv.m.dac);
  info[1].spec.rep_dacs  =  ofs(union lvd_in_card,  vertex.hv.m.dac);
  /* offset to register to read/write ADC clock params */
  info[0].spec.aclk_par = ofs(union lvd_in_card, vertex.lv.m.aclk_par);
  info[1].spec.aclk_par = ofs(union lvd_in_card, vertex.hv.m.aclk_par);
}

static void set_vtx_spec_un(struct vtx_info* info) {
  /* here should be definition of info[0/1].free_par ??? */
  set_vtx_un_ram_addr(info);
  set_vtx_lp_count_offs(info);
  /* ADC offset address, idx=0 */
  info[0].spec.adc_offs = ofs(union lvd_in_card, vertex.lv.m.poti);
  info[1].spec.adc_offs = ofs(union lvd_in_card, vertex.hv.m.poti);
  /* register offsets to load chips, either VATA or MATE3  */
  info[0].spec.ser_csr  = ofs(union lvd_in_card, vertex.lv.m.i2c_csr);
  info[0].spec.ser_data = ofs(union lvd_in_card, vertex.lv.m.i2c_data);
  info[1].spec.ser_csr  = ofs(union lvd_in_card, vertex.hv.m.i2c_csr);
  info[1].spec.ser_data = ofs(union lvd_in_card, vertex.hv.m.i2c_data);
  /* register offsets to load repeater DACS */
  info[0].spec.rep_dacs  =  ofs(union lvd_in_card, vertex.lv.m.dac);
  info[1].spec.rep_dacs  =  ofs(union lvd_in_card, vertex.hv.m.dac);
  /* offset to register to read/write ADC clock params */
  info[0].spec.aclk_par = ofs(union lvd_in_card, vertex.lv.m.aclk_par);
  info[1].spec.aclk_par = ofs(union lvd_in_card, vertex.hv.m.aclk_par);
}


/* info is a pointer to element of structure, no array of elements! */
static void set_ro_par_va(struct vtx_info* info, ems_u32 len, ems_u32* par) {
  int i;
  struct va_seq_par* seq_par = info->seq_par;
  if(!seq_par) {
    info->seq_par = malloc(sizeof(struct va_seq_par));
    memset(info->seq_par, 0, sizeof(struct va_seq_par));
    seq_par = info->seq_par;
  }
  if(seq_par->ro_len < len) {
    free(seq_par->ro_par);
    seq_par->ro_par = malloc(len*sizeof(ems_u32));
  }
  for(i=0; i<len; i++) {
    seq_par->ro_par[i] = par[i];
  }
  seq_par->ro_len = len;
}

static void set_ro_par_m3(struct vtx_info* info, ems_u32 len, ems_u32* par) {
  int i;
  struct m3_seq_par* seq_par = info->seq_par;
  if(!seq_par) {
    info->seq_par = malloc(sizeof(struct m3_seq_par));
    memset(info->seq_par, 0, sizeof(struct m3_seq_par));
    seq_par = info->seq_par;
  }
  if(seq_par->ro_len < len) {
    free(seq_par->ro_par);
    seq_par->ro_par = malloc(len*sizeof(ems_u32));
  }
  for(i=0; i<len; i++) {
    seq_par->ro_par[i] = par[i];
  }
  seq_par->ro_len = len;
}




/*
 * Meaning of parameters, 09.04.2008:
 * info[idx].sel_tp[0] - select chan 0 in vata chip 0 				  
 * info[idx].sel_tp[1] - select chan 0 in N chips, N-1 must be in lp_count[0]	  
 * info[idx].sel_tp[2] - select chan 0 and N VA clks, N-1 must be in lp_count[0]
 *
 * info[idx].vaclk_tp[0] - single VA clock in test mode				  
 * info[idx].vaclk_tp[1] - N VA clks, N-1 must be in lp_count[0]                    
 */
static int       get_cr(struct lvd_dev* dev, ems_u32 addr, ems_u32* cr_val);
/* static int       get_cr(struct lvd_dev* dev, ems_u32 addr, int* ddma, ems_u32* cr_val); */
static int       vertex_wait_serial(struct lvd_dev* dev, int addr, int mask);

static plerrcode seq_init(struct lvd_dev* dev,  struct lvd_acard* acard, 
			  int idx, ems_u32* par);
static int       lvd_stop_vertex(struct lvd_dev* dev, struct lvd_acard* acard);
static int       lvd_vertex_count_vata_chips(struct lvd_dev* dev, int addr, 
					     int unit, int max);
static ems_u32   read_scaling(struct lvd_dev* dev, int addr);
static int       write_scaling(struct lvd_dev* dev, int addr,  ems_u32 val);
static int       prep_ro_va32(struct lvd_dev* dev, struct lvd_acard* acard, int idx);
static void      free_seq_par_va(void* par);
static plerrcode sel_vata_chan_tp(struct lvd_dev* dev, struct lvd_acard* acard, 
				  int idx, ems_u32* par);
static plerrcode analog_tp_va(struct lvd_dev* dev, struct lvd_acard* acard, 
			      int idx, ems_u32 ch);
/* ========= MATE3 functions */

/* RETURNS: plOK | plErr_HW, no any bit check! */
static plerrcode clear_i2c_err(struct lvd_dev* dev, struct lvd_acard* acard, int idx);
static plerrcode read_i2c_csr(struct lvd_dev* dev, struct lvd_acard* acard, int idx, ems_u32* val);
/* RETURNS: plOK | plErr_Busy | plErr_HW, no I2C_ERROR check!  */
static plerrcode is_i2c_ready(struct lvd_dev* dev, struct lvd_acard* acard, int idx, ems_u32* val);

/* RETURNS: plOK | plErr_HW | plErr_Timeout, no I2C_ERROR check! */
static plerrcode wait_i2c(struct lvd_dev* dev, struct lvd_acard* acard, int idx, ems_u32 *val);


/* data_in[6] modified with reg(1-6) and chip bits and written to data_out,
   all registers of one chip */
static void set_mate3_reg_chip_bits(int chip, ems_u32* data_in, ems_u32* data_out);

/* Fills array data_out[nchips] by words to be written in "reg" registers of all chips with "data_in"*/
static void set_mate3_reg_all_chips(int nchips, int reg, ems_u32 data_in, ems_u32* data_out);

static plerrcode init_mate3_addresses(struct lvd_dev* dev, int addr, int idx, int* nchips);

/* write array of data[] in MATE3 board. Chip and reg, in data[i] must be set before */
static plerrcode write_mate3_reg_arr(struct lvd_dev* dev, int addr,
				     int idx, int size, ems_u32* data);

static plerrcode sel_mate3_chan_tp(struct lvd_dev* dev, struct lvd_acard* acard, 
				   int idx, ems_u32* par);
static plerrcode load_mate3_chips(struct lvd_dev* dev, struct lvd_acard* acard,
				  int idx, int words, ems_u32* data);
plerrcode        lvd_vertex_load_mate3(struct lvd_dev* dev, int addr, int unit,
				       int words, ems_u32* data);

static int prep_ro_m3(struct lvd_dev* dev, struct lvd_acard* acard, int idx);
static void free_seq_par_m3(void* seq_par);


RCS_REGISTER(cvsid, "lowlevel/lvd/vertex")




/*****************************************************************************/
struct lvd_acard* GetVertexCard(struct lvd_dev* dev, int addr) {
  int idx=lvd_find_acard(dev, addr);
  struct lvd_acard* acard=(idx < 0) ? 0 : dev->acards+idx;
  return (acard && 
	  ((acard->mtype == ZEL_LVD_ADC_VERTEX)   || 
	   (acard->mtype == ZEL_LVD_ADC_VERTEXM3) ||
	   (acard->mtype == ZEL_LVD_ADC_VERTEXUN))) ? acard : 0;
}
/*****************************************************************************/
int IsVertexCard(struct lvd_dev* dev, int card) {
  return ((dev->acards[card].mtype == ZEL_LVD_ADC_VERTEX)  || 
	  (dev->acards[card].mtype == ZEL_LVD_ADC_VERTEXM3)||
	  (dev->acards[card].mtype == ZEL_LVD_ADC_VERTEXUN));
}




/******************************************************************************
 * Sequencer initialization for VTX_CHIP_VA32TA2 chips
 * for parameter list see parameters of seq_init() below.
 * It must be proved before, that "acard" is ZEL_LVD_ADC_VERTEX card.
 *
 * VTX_CHIP_VA32TA2  parameters (par[0]== VTX_CHIP_VA32TA2 == 1):
 *
 * par[1]                    : master reset switch
 * par[2]                    : read-out reset switch
 * par[3]=nro                : number of read-out params
 * par[4]..par[3+nro]        : read-out params
 * p[4+nro] = nsel           : number of select channel sequencer switches
 * p[5+nro]..p[4+nro+nsel]   : switch values for select functions
 * p[5+nro+nsel] = nva                        : number of VA clk switch functions
 * p[6+nro+nsel]..p[5+nro+nsel+nva]           : VA clk switch functions
 * p[6+nro+nsel+nva] = nprog                  : number of 16-bit words to be loaded, seq prog
 * p[7+nro+nsel+nva]..p[6+nro+nsel+nva+nprog] : "sequencer program"

 */


static struct va_seq_par* decode_va_args(ems_u32* par) {
  int i;
  ems_u32* ptr = par;

  struct va_seq_par* seq_par = malloc(sizeof(struct va_seq_par));
  if(seq_par) {
    memset(seq_par, 0, sizeof(struct va_seq_par));
    
    seq_par->m_reset     = par[1];
    seq_par->ro_reset    = par[2];
    /* read-out params */
    seq_par->ro_len = par[3];
    seq_par->ro_par = malloc(seq_par->ro_len*sizeof(ems_u32));
    if(!seq_par->ro_par) {
      free_seq_par_va(seq_par);
      return 0;
    }
    ptr = &par[4];
    for(i=0; i<seq_par->ro_len; i++) {
      seq_par->ro_par[i] = *ptr++;
    }
    /* select params */
    seq_par->sel_len   = *ptr++;
    seq_par->sel_funcs = malloc(seq_par->sel_len*sizeof(ems_u32));
    if(!seq_par->sel_funcs) {
      free_seq_par_va(seq_par);
      return 0;
    }
    for(i=0; i<seq_par->sel_len; i++) {
      seq_par->sel_funcs[i] = *ptr++;
    }
    /* va params */
    seq_par->va_len  = *ptr++;
    seq_par->va_funcs =  malloc(seq_par->va_len*sizeof(ems_u32));
    if(!seq_par->va_funcs) {
      free_seq_par_va(seq_par);
      return 0;
    }
    for(i=0; i<seq_par->va_len; i++) {
      seq_par->va_funcs[i] = *ptr++;
    }
    /* sequencer program, no copy! */
    seq_par->prog_len = *ptr++;
    seq_par->prog = ptr;
  }
  return seq_par;
}


/******************************************************************************
 * May be used only if C_RUN=0 in "cr" of module!
 * This function is foreseen to load sequencer and set
 * parameters for "sequencer" functions. The last ones may depend
 * on the type of chips in front-end card.
 * Any access to front-end card is forbidden here,
 * because DACs must be set before any access! The initialization of front-end
 * electronics must be done later!
 * idx                   : 0 -LV, 1 - HV, other - forbidden
 * par                   : array of parameters, par == 0 - load dummy prog file only!
 * par[0]                : chip type, may be needed, if sequencer code depends on it
 *                       : if chip_type == VTX_CHIP_NONE, load dummy prog file!
 *                       : the rest of par[] ignored!
 * VTX_CHIP_VA32TA2  parameters (par[0]== VTX_CHIP_VA32TA2 == 1):
 *
 * par[1]                    : master reset switch
 * par[2]                    : read-out reset switch
 * par[3]=nro                : number of read-out params
 * par[4]..par[3+nro]        : read-out params
 * p[4+nro] = nsel           : number of select channel sequencer switches
 * p[5+nro]..p[4+nro+nsel]   : switch values for select functions
 * p[5+nro+nsel] = nva                        : number of VA clk switch functions
 * p[6+nro+nsel]..p[5+nro+nsel+nva]           : VA clk switch functions
 * p[6+nro+nsel+nva] = nprog                  : number of 16-bit words to be loaded, seq prog
 * p[7+nro+nsel+nva]..p[6+nro+nsel+nva+nprog] : "sequencer program"
 * 
 *
 * VTX_CHIP_MATE3 parameters (par[0]== VTX_CHIP_MATE3 == 2)
 *
 * par[1]                   : read-out reset switch
 * par[2] = nro             : number of read-out params
 * par[3]..par[2+nro]       : read-out params
 * p[3+nro] = nprog         : number of 16-bit words to be loaded, seq prog
 * p[4+nro]..p[4+nro+nprog] : "sequencer program"
 */
static plerrcode seq_init(struct lvd_dev* dev,  struct lvd_acard* acard, 
			      int idx, ems_u32* par) {
  int*      prog_len  = 0;
  ems_u32** prog = 0;
  plerrcode res = plOK;
  int addr = acard->addr;
  struct vtx_info* info = acard->cardinfo;
  if(idx <0 || idx>1) {
    printf("*** seq_init: addr=0x%1x idx=%1d -invalid\n", addr, idx);
    return plErr_Other;
  }

  /* first of all, check that dummy is loaded in second half of module! */
  int idx_sec = (idx) ? 0 : 1;
  if(!IS_SEQ_LOADED(info[idx_sec].loaded)) { /* not loaded, load dummy */
    if(dummy_seq_init(dev, acard, idx_sec+1)) {
      printf("seq_init: addr=0x%1x idx=%1d -failed load dummy in other part\n",
	     addr, idx);
      return plErr_Other;
    }
    set_vtx_func_dummy(&info[idx_sec]);    
  }
  int chip_type = (par) ? par[0] : VTX_CHIP_NONE;
  /* clear old sequencer parameters, if available */
  if(info[idx].free_par) {
    info[idx].free_par(info[idx].seq_par);   
    info[idx].free_par = 0;
  }
  info[idx].seq_par  = 0;
  /* is front-end available? */
  if(chip_type == VTX_CHIP_NONE) {  /* ---- front-end card is absent or switched off */
    set_dummy_vtx_info(&info[idx]); /* does not matter which module type */
    dummy_seq_init(dev, acard, idx+1);
  }
  else {   /* set vtx_info in correspondence with chip and module types */
    if(acard->mtype == ZEL_LVD_ADC_VERTEX) {
      if(chip_type != VTX_CHIP_VA32TA2) {
	printf("seq_init(): addr=0x%1x idx=%1d: invalid chip type %1d for module\n",
	       acard->addr, idx, chip_type); 
	res = plErr_BadModTyp;
      }
      else {
	info[idx].chiptype = chip_type;
	struct va_seq_par* seq_par = decode_va_args(par);
	if(seq_par) {
	  info[idx].seq_par  = seq_par;
	  prog_len = &seq_par->prog_len;
	  prog     = &seq_par->prog;
	  set_vtx_va_func(&info[idx]);
	}
	else {
	  res = plErr_InvalidUse;
	}
      }
    }
    else if(acard->mtype == ZEL_LVD_ADC_VERTEXM3) {
      if(chip_type != VTX_CHIP_MATE3) {
	printf("seq_init(): addr=0x%1x idx=%1d: invalid chip type %1d for module\n",
	       acard->addr, idx, chip_type); 
	res = plErr_BadModTyp;
      }
      else {
	info[idx].chiptype = chip_type;
	struct m3_seq_par* seq_par = decode_m3_args(par);
	if(seq_par) {
	  info[idx].seq_par  = seq_par;
	  prog_len = &seq_par->prog_len;
	  prog     = &seq_par->prog;
	  set_vtx_m3_func(&info[idx]);
	}
	else {
	  res = plErr_InvalidUse;
	}
      }
    }
    else if(acard->mtype == ZEL_LVD_ADC_VERTEXUN) {
      if(chip_type == VTX_CHIP_VA32TA2) {
	info[idx].chiptype = chip_type;
	info[idx].free_par = free_seq_par_va;   /* to escape memory leak */
	struct va_seq_par* seq_par = decode_va_args(par);
	if(seq_par) {
	  info[idx].seq_par  = seq_par;
	  prog_len = &seq_par->prog_len;
	  prog     = &seq_par->prog;
	  set_vtx_va_func_un(&info[idx]);
	  set_chip_type(dev, acard, idx); 
	}
	else {
	  res = plErr_InvalidUse;
	}
      }
      else if(chip_type == VTX_CHIP_MATE3) {
	info[idx].free_par = free_seq_par_m3;   /* to escape memory leak */
	info[idx].chiptype = chip_type;
	struct m3_seq_par* seq_par = decode_m3_args(par);
	if(seq_par) {
	  info[idx].seq_par  = seq_par;
	  prog_len = &seq_par->prog_len;
	  prog     = &seq_par->prog;
	  set_vtx_m3_func_un(&info[idx]);
	  set_chip_type(dev, acard, idx); 
	}
	else {
	  res = plErr_InvalidUse;
	}
      }
      else {
	printf("seq_init(): addr=0x%1x idx=%1d: invalid chip type %1d for module\n",
	       acard->addr, idx, chip_type); 
	res = plErr_BadModTyp;
      }
    }
  }
  if(res == plOK) {
    /* here we must load sequencer program */
    if((*prog_len > 0) &&  (*prog != 0)) {
      info[idx].loaded &= (~(CODE_LOADED | SEQ_LOADED));    /* clear seq loaded */
      if(lvd_vertex_load_seq_(dev, addr, info[idx].spec.seq_addr, 
			      *prog_len, *prog)) {
	printf("seq_init: addr=%x idx=%1d  lvd_vertex_load_seq_ returns error\n",
	     addr, idx);
	res= plErr_HW; /* if failed, has no large sense load dummy code, probably HW problems */
      }
      else {
	info[idx].loaded |= CODE_LOADED;    /* set program loaded */
      }

      *prog_len = 0; /* in order not to refer invalid memory outside... */
      *prog = 0;
      acard->initialized = 1;
    }
  }
  return res;
}


/*****************************************************************************
 * par                   : array of parameters, par == 0 - load dummy prog file only!
 * par[0]                : chip type: VTX_CHIP_NONE|VTX_CHIP_VA32TA2|VTX_CHIP_MATE3
 *
 * for the rest see seq_init()
 *
 * VTX_CHIP_MATE3  parameters (par[0]== VTX_CHIP_MATE3 == 2):
 *
 * par[1]                   : read-out reset switch
 * par[2] = nro             : number of read-out params
 * par[3]..par[2+nro]       : read-out params
 * p[3+nro] = nprog         : number of 16-bit words to be loaded, seq prog
 * p[4+nro]..p[4+nro+nprog] : "sequencer program"
 *
 * MAY BE CALLED ONLY AFTER lvdbus_init!
 *
 * par[] depends both on module and chip type! The module dependent must be set in call??? 
 */

plerrcode lvd_vertex_seq_init(struct lvd_dev* dev, int addr, int unit, ems_u32* par) {
  plerrcode pres = plOK;
  ems_u32 cr_val=0;
  struct lvd_acard* acard= GetVertexCard(dev, addr);
  struct vtx_info* info = (acard) ? acard->cardinfo : 0;
  if(!info) {
    printf("lvd_vertex_seq_init: addr=0x%1x unut=%1d - acard=%1p or info=%1p is null\n", 
	   addr, unit, acard, info);
    return plErr_BadModTyp;
  }
  int idx = unit - 1;
  /* are sequencers stopped ? */
  int res = get_cr(dev, addr, &cr_val); /* protection inside */
  if(res ||  (cr_val & VTX_CR_RUN)) { /* sequencers are running or error */
    printf("lvd_vertex_seq_init: failed get proper CR value, addr=0x%1x cr_val=0x%1x\n", addr, cr_val);
    pres = (res) ? plErr_HW : plErr_InvalidUse;
  }
  else {      /* initialize vtx_info for idx and load sequencer program */
    pres = seq_init(dev, acard, idx, par); /* if par==0, no front-end card */
  }
  return pres;
}

/*****************************************************************************/
static int is_ddma_active(struct lvd_dev* dev) {
  int ddma = 0;
#ifdef LVD_SIS1100
  if (dev->lvdtype==lvd_sis1100) {
    struct lvd_sis1100_info* cc_info=(struct lvd_sis1100_info*)dev->info;
    ddma = cc_info->ddma_active;
  }
#endif
  return ddma;
}

/*****************************************************************************
 * if(!ddma_in || *ddma_in < 0), it will be checked inside
 */
static int get_cr(struct lvd_dev* dev, ems_u32 addr, ems_u32* cr_val) {
  int i;
  int res = 0;
  int max_count = 10;

  for(i=0; i<max_count; i++) { /* protection loop */
    if((res = lvd_i_r(dev, addr, vertex.cr, cr_val)) == 0) break;
  }
  if(res) {
    printf("get_cr: read error,  addr=0x%1x\n", addr);
  }
  return (res) ? -1 : 0;
}

/*****************************************************************************
RETURNS:
 1 : module has C_RUN == 1
 0 : in other case, or if failed read C_RUN
-1 : error in get_cr
*/
static int is_vertex_running(struct lvd_dev* dev, struct lvd_acard* acard) {
  ems_u32 cr;
  int res = 0;
  res = get_cr(dev, acard->addr, &cr);  /* protection loop inside */
  if(res) return -1;
  return (cr & VTX_CR_RUN) ? 1 : 0;
}

/*****************************************************************************
 * if(ddma < 0), it will be checked inside
 */
static int get_sr(struct lvd_dev* dev, ems_u32 addr, int* ddma, ems_u32* sr_val) {
  int i;
  int res = 0;
  int max_count = 10;
  if(*ddma < 0) 
    *ddma = is_ddma_active(dev);
  if(*ddma) {
    for(i=0; i<max_count; i++) {
      res = lvd_i_r(dev, addr, vertex.sr, sr_val);
      if(res == 0) break;
    }
  }
  else {
    res = lvd_i_r(dev, addr, vertex.sr, sr_val);
    return (res) ? -1 : 0;
  }
  if(res) {
    printf("get_sr: read error, addr=0x%1x\n", addr);
  }
  return (res) ? -1 : 0;
}

/* RETURN:
 *    0 - no error
 *   -1 - failed read status register "sr" 0xE
 *   >0 - content of sr in case of error. 
 */
static int is_seq_error(struct lvd_dev* dev, ems_u32 addr) {
  ems_u32 sr  = 0;
  int ddma=-1;
  int res = 0;

  res = get_sr(dev, addr, &ddma, &sr);
  if(res) {
    printf("is_seq_error: failed get_sr(), addr=0x%1x\n", addr);
    return -1;
  }
  return (sr & (VTX_SR_FAT_ERR | VTX_SR_LV_DAQ_ERR | VTX_SR_HV_DAQ_ERR |
		VTX_SR_RO_ERR)) ? sr : 0;
}


/*****************************************************************************
 * if(ddma < 0), it will be checked inside
 */
static int get_seq_csr(struct lvd_dev* dev, ems_u32 addr, int* ddma_in, ems_u32* seq_csr_val) {
  int i;
  int res = 0;
  int max_count = 10;
  for(i=0; i<max_count; i++) {
    res = lvd_i_r(dev, addr, vertex.seq_csr, seq_csr_val);
    if(res == 0) break;
  }
  return (res) ? -1 : 0;
}


/****************************************************************************
 * unit  : 1 -LV, 2 - HV, 3 - both
*/
static plerrcode seq_halt(struct lvd_dev* dev, int addr, int unit) {
  int i;
  int res = 0;
  ems_u32 seq_csr_val=0;
  int ddma =  is_ddma_active(dev);
  ems_u32 val;
  int max_count = 10;
  ems_u32 mask = 0;

  res = get_seq_csr(dev, addr, &ddma, &seq_csr_val);
  if(res)
    return plErr_HW;
  val = seq_csr_val;
  /*   if sequencer is halted, both VTX_SEQ_CSR_?V_IDLE and  */
  /*   VTX_SEQ_CSR_?V_HALT are set */
  if(unit&1) {
    val  |= VTX_SEQ_CSR_LV_HALT;
    mask |= VTX_SEQ_CSR_LV_IDLE;
  }
  if(unit&2) {
    val  |= VTX_SEQ_CSR_HV_HALT;
    mask |= VTX_SEQ_CSR_HV_IDLE;
  }
  res = lvd_i_w(dev, addr, vertex.seq_csr, val);

  for(i=0; i<max_count; i++) {
    res = get_seq_csr(dev, addr, &ddma, &seq_csr_val);
    if(!res && (seq_csr_val & mask)) return plOK;
  }
  return (res) ? plErr_HW : plErr_Busy; 
}

/* Return: plErr_Timeout - sequencer is busy
 *         plErr_OK      - sequencer bit BUSY is not set
*/
static plerrcode seq_busy_n(struct lvd_dev* dev, int addr, ems_u32 busy_bit) {
  int ddma = -1;
  ems_u32 val = -1;
  plerrcode res = plErr_Timeout;
  /* wait sequencer ready */
  int pass = 10;
  while(pass>0) {
    if(get_seq_csr(dev, addr, &ddma, &val)) { /* protection loop  inside */
      printf("*** seq_busy_n: addr=0x%1x - error in get_seq_csr()\n", addr);
      return res;
    }
    if(!(val & busy_bit)) {
      res = plOK;
      break;
    }
    u_delay(100);
    pass--;
  }
  return res;
}

/* returns 0 - OK */
static int seq_resume_n(struct lvd_dev* dev, int addr, ems_u32 halt_bit, ems_u32 halt_mask) {
  int ddma = -1;
  ems_u32 val = -1;
  printf("***** IGNORE seq_resume_n\n");
  return plOK;
  /* remove halt bit */  
  if(lvd_i_w(dev, addr, vertex.seq_csr, 0)) {
    printf("*** seq_resume_n: addr=0x%1x - error in write seq_csr 0\n", addr);    
  }
  /* wait sequencer resume running */
  int res = -1;
  int pass = 10;
  while(pass>0) {
    if(get_seq_csr(dev, addr, &ddma, &val)) {
      printf("*** seq_resume_n: addr=0x%1x - error in get_seq_csr()\n", addr);
      return res;
    }
    if(val & halt_bit) { /* was not cleaned */
      printf("*** seq_resume_n: addr=0x%1x - repeat write 9\n", addr);
      lvd_i_w(dev, addr, vertex.seq_csr, 0);
    }
    else {
      if(!(val & halt_mask)) { /* resumed */
	res = 0;
	break;
      }
    }
    u_delay(100);
    pass--;
  }
  return res;
}
/****************************************************************************
 * unit : 1 -LV, 2 - HV, 3 - both
*/
static plerrcode seq_resume(struct lvd_dev* dev, int addr, int unit) {
  ems_u32 halt_mask = 0;
  ems_u32 halt_bit  = 0;

  if(unit & 1) {
    halt_mask |= VTX_SEQ_CSR_LV_IDLE;
    halt_bit  |= VTX_SEQ_CSR_LV_HALT;
  }
  if(unit & 2) {
    halt_mask |= VTX_SEQ_CSR_HV_IDLE;
    halt_bit  |= VTX_SEQ_CSR_HV_HALT;
  }
  return (seq_resume_n(dev, addr, halt_bit, halt_mask)) ? plErr_HW : plOK;
}

static int seq_halt_n(struct lvd_dev* dev, int addr, ems_u32 halt_bit, ems_u32 halt_mask) {
  int ddma = -1;
  ems_u32 val = -1;
  int res = -1;
  int pass = 100;
  printf("***** IGNORE seq_halt_n\n");
  return plOK;

  /* set halt bit */
  if(lvd_i_w(dev, addr, vertex.seq_csr, halt_bit)) {
    printf("*** seq_halt_n: addr=0x%1x - error in write seq_csr 0\n", addr);    
  }
  /* wait sequencer halt */
  while(pass>0) {
    if(get_seq_csr(dev, addr, &ddma, &val)) {
      printf("*** seq_halt_n: addr=0x%1x - error in get_seq_csr()\n", addr);
      return res;
    }
    if(!(val & halt_bit)) { /* was not set */
      lvd_i_w(dev, addr, vertex.seq_csr, halt_bit);
      printf("*** seq_halt_n: addr=0x%1x repeat write halt_bit=0x%1x\n", addr, halt_bit);
    }
    else {
      if((val & halt_mask)) { /* halted */
	res = 0;
	break;
      }
    }
    u_delay(100);
    pass--;
  }
  if(res) {
    printf("*** seq_halt_n: addr=0x%1x clear halt bit\n", addr);
    if(lvd_i_w(dev, addr, vertex.seq_csr, 0)) {
      printf("*** seq_halt_n: addr=0x%1x - error in write seq_csr 0\n", addr);    
    }
  }
  return res;
}

/*****************************************************************************
 * This function allows check that sequencer may be halted
 * Now (04.04.2008), if sequencer is prepared for read-out, it cannot be halted!
 * So this function servers to check, that we can execute different procedure
 * by writing of sequencer switch[s].
 *
 * idx: 0 - LV, 1 -HV
 * RETUNS:
 *  0 - cannot be halted / or resumed
 *  1 - can be halted. 
 */
/*****************************************************************************
 * Halt sequencer
 * if value == 0, clear halt bit in seq_csr, resume sequencer run
 *          == 1, halt sequencer
 * 
 */
plerrcode lvd_vertex_seq_halt(struct lvd_dev* dev, int addr, int unit, ems_u32 value)
{
  if (addr<0) {
    int card;
    plerrcode last_err = plOK;
    for (card=0; card<dev->num_acards; card++) {
      if(IsVertexCard(dev, card)) {
	plerrcode pres = lvd_vertex_seq_halt(dev, dev->acards[card].addr, 
					     unit, value);
	if (pres!=plOK) {
	  last_err = pres;
	}
      }
    }
    return last_err;
  } else {
    struct lvd_acard* acard = GetVertexCard(dev, addr);
    if (!acard) {
      printf("lvd_vertex_seq_halt: error, no VERTEX card with address 0x%x known\n",
	     addr);
      return plErr_Program;
    }
    if(acard->initialized == 0) return plOK;
    return (value) ? seq_halt(dev, addr, unit) : seq_resume(dev, addr, unit);
  }
  return plErr_Program; /* to make compiler happy */
}

/*****************************************************************************
 * Wait until sequencer busy bits are cleared.
 *  RETURNS:
 *  0 : sequencer is ready . In "ddma"/async mode may be invalid
 * -1 : hardware error
 *  1 : sequencer is busy.
 */
static int wait_seq_ready(struct lvd_dev* dev, int addr, int unit) {
  int i;
  ems_u32 seq_csr;
  int max_count = 10; /* attempt to read sequencer status */
  int res = 0;
  int mask = 0;
  int ddma = is_ddma_active(dev);
  struct timeval a, b;
  int tdiff;
  const int MAX_WAIT = 100;
  if(unit & 0x1) {
    mask |= VTX_SEQ_CSR_LV_BUSY;
  }
  if(unit & 0x2) {
    mask |= VTX_SEQ_CSR_HV_BUSY;
  }
  gettimeofday(&a, 0);
  for(i=0; i<max_count; i++) {
    res = get_seq_csr(dev, addr, &ddma, &seq_csr);
    gettimeofday(&b, 0);
    tdiff=(b.tv_sec-a.tv_sec)*1000000+(b.tv_usec-a.tv_usec);
    if(res) {
      if(ddma && (tdiff < MAX_WAIT)) continue;
      printf("wait_seq_ready: unexpected error in seq_csr read of addr=0x%1x\n",
	     addr);
      return -1;
    }
    else {
      if((seq_csr & mask)== 0) {
	return 0;
      } else {
	if(tdiff > MAX_WAIT) {
	  return -1; /* time-out */
	}
      }
    }  
  }
  if (res) return -1;
  return ((seq_csr & mask)== 0) ? 0 : 1;
}

/*****************************************************************************
 * Only after both sequencers are loaded with required programs,
 * threshold and pedestals, ONLY then both sequencers may be started
 * 
 * If module is running (C_RUN==1) we will stop it only if: 
 *     - error bit[s] in sr (0xE) is set
 *     - we cannot execute sequencer "switch" functions of both sequencers
 * If module is stopped (C_RUN==0):
 *   For any chance we must check is  error bit[s] in sr (0xE) clear.
 *   If it is not true, we must stop module.
 *   
*/

plerrcode start_vertex_init_mode(struct lvd_dev* dev, struct lvd_acard* acard) {
  int addr = acard->addr;
  struct vtx_info* info = (struct vtx_info*)acard->cardinfo;
  int res = 0;
  ems_u32 cr = 0;
  int seq_err;
  int req_mode = acard->daqmode;

  /* is it necessary to start module? */
  if ((req_mode & VTX_CR_RUN) == 0) { /* module must not be started! */
    return plOK;
  }
  /* check that we can start module, i.e. are sequencers loaded */
  if ((((info[0].loaded & SEQ_LOADED) || (info[0].loaded & CODE_LOADED)) &&
       ((info[1].loaded & SEQ_LOADED) || (info[1].loaded & CODE_LOADED))) == 0) {
    printf("start_vertex_init_mode: addr=0x%1x info[0].loaded=0x%1x info[1].loaded=0x%1x - error",
	   acard->addr, info[0].loaded, info[1].loaded );
    return plErr_InvalidUse;
  }
  /* can we access module CR (0xC)? */
  res = get_cr(dev, addr, &cr);  /* protection loop inside */
  if(res) {
    printf("start_vertex_init_mode: error in get_cr() for addr=0x%1x\n", addr);
    return plErr_HW;
  }
  /* are error bits set in module ?  Does not matter is module running or not */
  seq_err = is_seq_error(dev, addr);
  if(seq_err) { /* if any error, even read of sr */
    printf("start_vertex_init_mode: error in  is_seq_error() for addr=0x%1x seq_err=0x%1x\n",
	   addr, seq_err);
    printf("start_vertex_init_mode: try reset module, addr=0x%1x\n", addr);
    res= lvd_stop_vertex(dev, acard); /* it is the only way to clear err bits */
    if(res) {
      printf("start_vertex_init_mode: error in lvd_stop_vertex(), addr=0x%1x\n", addr);  
      return plErr_HW;
    }
    /* check that now err bits are clear */
    seq_err = is_seq_error(dev, addr);
    if(seq_err) {
      printf("start_vertex_init_mode: addr=0x%1x seq_err=0x%1x after reset\n", 
	     addr, seq_err);
      return plErr_HW;
    }
  }

  /* check is module running, and if it is so, load idle loop switches */
  if(cr & VTX_CR_RUN) {
    res = 0;
    if (exec_seq_func(dev, acard, 0, 0, 0) != plOK) {
      res = 1;
    } else if (exec_seq_func(dev, acard, 1, 0, 0) != plOK) {
      res = 2;
    }
    if(res != 0) { /* failed set idle loops in both sequencers */
      printf("start_vertex_init_mode: error in exec_seq_func(), addr=0x%1x, res=%1d, reset module\n",
	     addr, res );
      res= lvd_stop_vertex(dev, acard); /* try reset module, i.e. stop it */
      printf("start_vertex_init_mode: addr=0x%1x was running, stop it\n", addr);
      if(res) {
	printf("start_vertex_init_mode: error in lvd_stop_vertex(), addr=0x%1x\n", addr);  
	return plErr_HW;
      } 
    }
  }
  /* start module in init mode (or set proper bits for running module) */
  /*  write idle switches */
  int sw = 0x88888888;
  int i;
  int max_pass = 10;
  res = -1;
  /*  write idle switches, not in loop! */
  res = lvd_i_w(dev, addr, vertex.lv.swreg, sw); /* tsv */
  res |= lvd_i_w(dev, addr, vertex.hv.swreg, sw);
  if(res) {
    printf("start_vertex_init_mode: addr=%1d : write idle loop faked error?\n", addr);
  }
  /* start */
  for(i=0; i<max_pass; i++) {
    if((res = lvd_i_w(dev, addr, vertex.cr, VTX_INIT_MODE)) == 0) break;
  }
  if(res) {
    printf("start_vertex_init_mode: addr=%1d : write CR  faked error?\n", addr);
  }
  /* check resulting cr */
  res = get_cr(dev, addr, &cr); /* protection loop inside */
  if(!res && ((cr & VTX_INIT_MODE) != VTX_INIT_MODE)) {
    res = -1;
    printf("start_vertex_init_mode: addr=0x%1x error\n", addr);
    return plErr_HW;
  }
  return plOK;
}

plerrcode lvd_vertex_start_init_mode(struct lvd_dev* dev, int addr) {
  if (addr<0) {
    int card;
    plerrcode last_err = plOK;
    plerrcode pres;
    for (card=0; card < dev->num_acards; card++) {
      if(IsVertexCard(dev, card)) {
	pres = start_vertex_init_mode(dev, dev->acards + card);
	if(pres != plOK) {
	  last_err = pres;
	}
      }
    }
    return last_err;
  } else {
    struct lvd_acard* acard = GetVertexCard(dev, addr);
    if (!acard) {
      printf("lvd_vertex_start_init_mode: no VERTEX card with addr=0x%x known, error\n",
	     addr);
      return plErr_Program;
    }
    return start_vertex_init_mode(dev, acard);
  }
}

/****************************************************************************
 * This function is used to execute "sequencer functions", i.e. load switch registers.
 * Sequencer must be running, i.e. C_RUN== 1, if not - returns error.
 * Sequencer may be halted, if not - error.
 * The "sw" value must have set bits (0x8 | 0x80 | 0x800 | 0x8000 ....)
 * in order corresponding switches were written.
 */
static plerrcode exec_seq_func(struct lvd_dev* dev, struct lvd_acard* acard, int idx, 
			       ems_u32 sw, int protect) {

  int crun = 0;
  int seq_csr, ddma=0;
  char buf[128];
  int res = 0;
  const int m0 = VTX_SEQ_CSR_LV_OVFL | VTX_SEQ_CSR_LV_UNFL | VTX_SEQ_CSR_LV_OPERR |VTX_SEQ_CSR_LV_BUSY;
  const int m1 = VTX_SEQ_CSR_HV_OVFL | VTX_SEQ_CSR_HV_UNFL | VTX_SEQ_CSR_HV_OPERR;
  ems_u32 err_mask = (idx) ? m1 : m0;
  ems_u32 busy_mask = (idx) ? VTX_SEQ_CSR_HV_BUSY : VTX_SEQ_CSR_LV_BUSY;
  int addr = acard->addr;
  int req_mode = acard->daqmode;

  sprintf(buf,"exec_seq_func(0x%1x) : error", sw);

  if(!(req_mode & VTX_CR_RUN)) { /* sequencer will not run, ignore func */
    printf("exec_seq_func: for not running module addr=0x%1x req_mode=0x%1x sw=0%1x - ignore\n", 
	   addr, req_mode, sw);
    return plOK;
  }
  crun = is_vertex_running(dev, acard); /* protection loop inside */
  if(crun < 0) {
    printf("exec_seq_func: addr=0x%1x idx=%1d sw=0x%1x, error in is_vertex_running()\n",
	   addr, idx, sw);
    return plErr_HW;
  }
  /* is module started, i.e. C_RUN==1 ?*/
  if(!crun) {
    printf("exec_seq_func: addr=0x%1x idx=%1d sw=0x%1x, error: C_RUN=0\n", 
	   addr, idx, sw);
    return plErr_InvalidUse;
  }

  /* is sequencer running and OK */
  if(get_seq_csr(dev, addr, &ddma, &seq_csr)) {  /* protection loop inside */
    printf("exec_seq_func: addr=0x%1x idx=%1d sw=0x%1x, error: failed read seq_csr\n",
	   addr, idx, sw);
    return plErr_HW;
  }

  if(seq_csr & (err_mask | busy_mask)) {
    print_csr_regs(dev, addr, idx, buf);
    return plErr_Other;
  }
  /* now load switch registers */
  int max_pass = 10;
  int i;
  if(sw == 0) sw = 0x88888888;
  if (idx == 0)
    res=lvd_i_w(dev, addr, vertex.lv.swreg, sw);
  else
    res=lvd_i_w(dev, addr, vertex.hv.swreg, sw);
  if(res && protect) {  
    for(i=0; i<max_pass; i++) { /* protection loop  */
      res = (idx == 0) ? lvd_i_w(dev, addr, vertex.lv.swreg, sw) :
                         lvd_i_w(dev, addr, vertex.hv.swreg, sw);
      if(res == 0) break;
    }
  }

  /* wait completition */
  if(seq_busy_n(dev, addr, busy_mask)) {
    print_csr_regs(dev, addr, idx, "exec_seq_func: time-out error");
    return plErr_Timeout;
  }
  /* error in sequencer code ? */
  if(get_seq_csr(dev, addr, &ddma, &seq_csr)) { /* protection loop inside */
    printf("exec_seq_func: addr=0x%1x idx=%1d sw=0x%1x, error: failed read seq_csr\n",
	   addr, idx, sw);
    return plErr_HW;
  }
  if(seq_csr & err_mask) {
    print_csr_regs(dev, addr, idx, buf);
    return plErr_Other;
  }
  return plOK;
}

plerrcode lvd_vertex_exec_seq_func(struct lvd_dev* dev, int addr, int unit, 
			       ems_u32 sw) {
  if (addr<0) {
    int card;
    plerrcode last_err = plOK;
    plerrcode pres;
    for (card=0; card < dev->num_acards; card++) {
      if(dev->acards[card].initialized == 0) continue;
      if(IsVertexCard(dev, card)) { /* probably must be more complicated! */	
	pres = lvd_vertex_exec_seq_func(dev, addr, unit, sw);
	if(pres != plOK) {
	  last_err = pres;
	}
      }
    }
    return last_err;
  } else {
    struct lvd_acard* acard = GetVertexCard(dev, addr);
    if (!acard) {  /* probably must be more complicated! */
      printf("lvd_vertex_exec_seq_func: error, no VERTEX card with address 0x%x known\n",
	     addr);
      return plErr_Program;
    }
    plerrcode last_err = plOK;
    plerrcode pres = plErr_InvalidUse;

    if(unit&1) {
      pres = exec_seq_func(dev, acard, 0, sw, 0);
      if(pres != plOK) {
	last_err = pres;
      }
    }
    if(unit&2) {
      pres = exec_seq_func(dev, acard, 1, sw, 0);
      if(pres != plOK) {
	last_err = pres;
      }
    }
    return last_err;
  }
}

static int is_readout(int req_mode, int idx) {
  int ro = 0;
  if(idx) {
    ro = (req_mode & VTX_CR_IH_HV) ? 0 : 1;
  }
  else {
     ro = (req_mode & VTX_CR_IH_LV) ? 0 : 1;
  }
  return ro;
}

static plerrcode write_aclk_par(struct lvd_dev* dev, int addr, struct vtx_info* info, int par) {
  int i, /*r, */v;
  int max_pass = 10;
  plerrcode res = plErr_HW;
  for(i=0; i< max_pass; i++) { /* safe write/read aclk_par */
    v = 0;
    /*r  =*/ lvd_i_w_(dev, addr, info->spec.aclk_par, 2, par);
    /*r  =*/ lvd_i_r_(dev, addr, info->spec.aclk_par, 2, &v);
    if(v == par) {
      res = plOK;
      break;
    }
  }
  if(res) {
    printf("write_aclk_par: addr=0x%1x idx=%1d aclk_par error\n", addr, info->idx);
  }
  return res;
}
static plerrcode write_lp0(struct lvd_dev* dev, int addr, struct vtx_info* info, int par) {
  int i, /*r, */v;
  int max_pass = 10;
  plerrcode res = plErr_HW;
  for(i=0; i< max_pass; i++) { /* safe write/read aclk_par */
    v = 0;
    /*r  =*/ lvd_i_w_(dev, addr, info->spec.lp_count[0], 2, par);
    /*r  =*/ lvd_i_r_(dev, addr, info->spec.lp_count[0], 2, &v);
    if(v == par) {
      res = plOK;
      break;
    }
  }
  if(res) {
    printf("write_lp0: addr=0x%1x idx=%1d aclk_par error\n", addr, info->idx);
  }
  return res;
}

static plerrcode write_lp1(struct lvd_dev* dev, int addr, struct vtx_info* info, int par) {
  int i, /*r, */v;
  int max_pass = 10;
  plerrcode res = plErr_HW;
  for(i=0; i< max_pass; i++) { /* safe write/read aclk_par */
    v = 0;
    /*r  =*/ lvd_i_w_(dev, addr, info->spec.lp_count[1], 2, par);
    /*r  =*/ lvd_i_r_(dev, addr, info->spec.lp_count[1], 2, &v);
    if(v == par) {
      res = plOK;
      break;
    }
  }
  if(res) {
    printf("write_lp1: addr=0x%1x idx=%1d aclk_par error\n", addr, info->idx);
  }
  return res;
}
static plerrcode write_ro_clk(struct lvd_dev* dev, int addr, struct vtx_info* info, int par) {
  int i, /*r, */v;
  int max_pass = 10;
  plerrcode res = plErr_HW;
  int idx = info->idx;
  for(i=0; i< max_pass; i++) { /* safe write/read aclk_par */
    v = 0;
#if 0
/*
 *  r, the result, is not used.
 *  Therefore the compiler may optimize away the whole construct.
 *  (No, this is not true because the compiler does not know anything
 *  about the side effects of functions; but this is very bad style.)
 */
    r = (idx) ? lvd_i_w(dev, addr, vertex.hv.nr_chan, par) :
                lvd_i_w(dev, addr, vertex.lv.nr_chan, par);
    r = (idx) ? lvd_i_r(dev, addr, vertex.hv.nr_chan, &v) :
                lvd_i_r(dev, addr, vertex.lv.nr_chan, &v);
#endif
    if (idx) {
        lvd_i_w(dev, addr, vertex.hv.nr_chan, par);
        lvd_i_r(dev, addr, vertex.hv.nr_chan, &v);
    } else {
        lvd_i_w(dev, addr, vertex.lv.nr_chan, par);
        lvd_i_r(dev, addr, vertex.lv.nr_chan, &v);
    }

    if(v == par) {
      res = plOK;
      break;
    }
  }
  if(res) {
    printf("write_ro_clk: addr=0x%1x idx=%1d aclk_par error\n", addr, info->idx);
  }
  return res;
}


/* info[idx].spec.prep_ro function of vtx_info structure,  
 * now it does not matter, is it VertexADC or VertexADCUN module */
static int prep_ro_va32(struct lvd_dev* dev, struct lvd_acard* acard, int idx) {
  if(acard->initialized == 0) {
    return 0;
  }
  int addr = acard->addr;
  struct vtx_info* info = (struct vtx_info*)acard->cardinfo;
  struct va_seq_par* va_par = info[idx].seq_par;
  int ro_clk = info[idx].numchips * VA32TA2_CHAN - 1;
  int res = 0;
  /* select channel to monitor by TP */
  ems_u32 d[2];
  ems_u32 uw = 0;
  d[0] = info[idx].mon_ch;
  d[1] = info[idx].tp_all; /* channel per chip */

  uw = prep_uw(4, info[idx].dac[4], d[0], d[1]); /* check (chan >= 0) inside */
  if(info[idx].sel_tp_chan(dev, acard, idx, d) != plOK) {
    uw = 0;
    printf("prep_ro_va32: failed sel mon_ch=%1d at addr=0x%1x idx=%1d\n",
	   d[0], addr, idx);
  }
  lvd_vertex_write_uw(dev, addr, idx+1, uw); /* protection loop inside */
  /* read-ou reset */
  if(info[idx].ro_reset) info[idx].ro_reset(dev, acard, idx);

  /* prepare to start read-out */
  if((res=write_aclk_par(dev, addr, &info[idx], va_par->ro_par[1])) != plOK) return res;
  if((res=write_lp0(dev, addr, &info[idx], va_par->ro_par[2])) != plOK) return res;
  if((res=write_lp1(dev, addr, &info[idx],  info[idx].numchips-1)) != plOK) return res;
  if((res=write_ro_clk(dev, addr, &info[idx], ro_clk)) != plOK) return res;
  return  exec_seq_func(dev, acard, idx, va_par->ro_par[0], 0);
}

/* info[idx].spec.prep_ro function of vtx_info structure,  
 * it  does not matter, is it VertexADCM3 or VertexADCUN module */
static int prep_ro_m3(struct lvd_dev* dev, struct lvd_acard* acard, int idx) {
  if(acard->initialized == 0) {
    return 0;
  }
  int addr = acard->addr;
  struct vtx_info* info = (struct vtx_info*)acard->cardinfo;
  struct m3_seq_par* m3_par = info[idx].seq_par;
  int ro_clk = info[idx].numchips * m3_par->ro_par[3] - 1;
  int res = 0;
  /* select channel to monitor by TP */
  ems_u32 d[2];
  ems_u32 uw = 0;
  d[0] = info[idx].mon_ch;
  d[1] = info[idx].tp_all; /* chanel per chip */

  uw = prep_uw(4, info[idx].dac[4], d[0], d[1]); /* check (chan >= 0) inside */
  if(info[idx].sel_tp_chan(dev, acard, idx, d) != plOK) {
    uw = 0;
    printf("prep_ro_m3: failed sel mon_ch=%1d at addr=0x%1x idx=%1d\n",
	   d[0], addr, idx);
  }
  lvd_vertex_write_uw(dev, addr, idx+1, uw); /* protection loop inside */

  /* read-out reset */
  if(info[idx].ro_reset) info[idx].ro_reset(dev, acard, idx);

  /* prepare to start read-out */
  if((res=write_aclk_par(dev, addr, &info[idx], m3_par->ro_par[1])) != plOK) return res;
  if((res=write_lp0(dev, addr, &info[idx], m3_par->ro_par[2])) != plOK) return res;
  if((res=write_lp1(dev, addr, &info[idx],  info[idx].numchips-1)) != plOK) return res;
  if((res=write_ro_clk(dev, addr, &info[idx], ro_clk)) != plOK) return res;
  return exec_seq_func(dev, acard, idx, m3_par->ro_par[0], 0);
}
 
static int prep_ro(struct lvd_dev* dev, struct lvd_acard* acard, int idx) {
  int i;
  int max_pass = 3;
  if(acard->initialized == 0) {
    return plOK;
  }
  int addr = acard->addr;
  ems_u32 req_mode = acard->daqmode; /* ????? info[idx],daqmode? */
  struct vtx_info* info = (struct vtx_info*)acard->cardinfo;
  int res = 0;
  if(is_readout(req_mode, idx) && (info[idx].prep_ro != 0)) {
    if(!(info[idx].loaded & CHIP_LOADED)) {
      printf("prep_ro: module 0x%1x idx=%1d, chips are not loaded,error\n",
	     addr, idx);
      return plErr_InvalidUse;
    }
    for(i=0; i<max_pass; i++) { /* protection loop, extra */
      if((res = info[idx].prep_ro(dev, acard, idx)) == 0) break; /* protection loop inside */
    }
  }
  return res;
}
/****************************************************************************
 * This function without error must start read-out of  initialized
 * and not initialized modules. 
 * Not properly initialized:
 *    info[idx] == VTX_CHIP_NONE, no read-out
 *    info[idx].spec == 0 or invalid size
 *
 *  
*/
static int
lvd_start_vertex(struct lvd_dev* dev, struct lvd_acard* acard)
{
  int i;
  int max_pass = 10;
  int res = 0;
  ems_u32 cr = 0;
/*   struct vtx_info* info=acard->cardinfo; */
  ems_u32 req_mode = acard->daqmode;

  if(acard->initialized == 0) {
    return 0;
  }
  /* Will we read at least half of module? */
  
  if(((req_mode & VTX_CR_RUN) == 0) ||
     ((req_mode & VTX_CR_IH_LV) && (req_mode & VTX_CR_IH_HV))) {
    for(i=0; i<max_pass; i++) { /* protection loop */
      if((res = lvd_i_w(dev, acard->addr, vertex.cr, req_mode)) == 0) break; /* for any chance */
      /* should be read back and check that it was stopped? ### */
    }
    if(res != 0) {
      printf("lvd_start_vertex: write cr error, addr=0x%1x\n", acard->addr);
    }
    else {
      printf("lvd_start_vertex: no read-out requested at addr 0x%1x\n", acard->addr);
    }
    return 0;  /* no read-out requested, module left as it is (how is it???? - stopped?) */
  }
  /* To proceed futher, module must be started. If not, try to start it */
  res = get_cr(dev, acard->addr, &cr); /* protection loop inside */
  if(res) {
    printf("lvd_start_vertex: failed get_cr, error\n");
    return -1;
  }
  
  /* we will read at least one input */
  if((cr & VTX_CR_RUN) == 0) {
    printf("lvd_start_vertex: try start module, addr=0x%1x\n", acard->addr);
    plerrcode pres= start_vertex_init_mode(dev, acard);
    if(pres != plOK) {
      return -1;
    }
  }
  /*idx = 0;*/
  if(prep_ro(dev, acard, 0)) {
    printf("lvd_start_vertex: addr=0x%1x() idx=0 (lv): error\n", acard->addr);
    return -1;
  }
  /*idx = 1;*/
  if(prep_ro(dev, acard, 1)) {
    printf("lvd_start_vertex: addr=0x%1x() idx=1 (hv): error\n", acard->addr);
    return -1;
  }
  for(i=0; i<max_pass; i++) { /* protection loop */
    if((res = lvd_i_w(dev, acard->addr, vertex.cr, acard->daqmode)) == 0) break; 
    /* should be read back and check that it was started? ### */
  }
  if(res != 0) {
    printf("lvd_start_vertex: write cr error, addr=0x%1x\n", acard->addr);
    return -1;
  }
  return res ? -1 : 0;
}

plerrcode lvd_vertex_module_start(struct lvd_dev* dev, int addr) {
  if (addr<0) {
    int card;
    int res = 0;
    for (card=0; card < dev->num_acards; card++) {
      if(dev->acards[card].initialized && IsVertexCard(dev, card)) {
	res |= lvd_start_vertex(dev, dev->acards+card);
      }
    }
    return (res) ? plErr_HW : plOK;
  } else {
    int res = 0;
    struct lvd_acard* acard = GetVertexCard(dev, addr);
    if (!acard) {
      printf("lvd_vertex_module_start: error, no VERTEX card with address 0x%x known\n",
	     addr);
      return plErr_Program;
    }
    res = (acard->initialized) ? lvd_start_vertex(dev, acard) : 0;
    return (res) ? plErr_HW : plOK;
  }
}


/*****************************************************************************
 */
static int
lvd_stop_vertex(struct lvd_dev* dev, struct lvd_acard* acard)
{
  int i;
  int max_pass = 10;
  int res=0;
  ems_u32 cr_val=0x0;
  int addr = acard->addr;
  if(acard->initialized == 0) {
    return 0;
  }
  /* try get Control/Mode register value */
  res = get_cr(dev, addr, &cr_val); /* protection loop inside */

  if(res < 0)
    cr_val = VTX_STOP_MODE; /* for any chance */

  /* inhibit trigger input of module */
  for(i=0; i<max_pass; i++) { /* protection loop */
    if((res = lvd_i_w(dev, addr, vertex.cr, cr_val|VTX_CR_IH_TRG)) == 0) break;
  }
  if(res) {
    printf("lvd_stop_vertex: error in write VTX_CR_IH_TRG\n");
  }

  /* wait completion of any operation */  
  vertex_wait_serial(dev, addr, VTX_SR_SERIAL_BUSY); /* protection loop inside */
  lvd_vertex_seq_wait(dev, addr, 3);
  for(i=0; i<max_pass; i++) { /* protection loop */
    if((res = lvd_i_w(dev, addr, vertex.cr, VTX_STOP_MODE)) == 0) break;
  }
  if(res) {    /* could it be only false answer ?*/
    printf("lvd_stop_vertex: error in write stop_mode, addr=0x%1x,try check is stopped?\n",
	  addr);
    res = get_cr(dev, addr, &cr_val); /* protection loop inside */
    if(res < 0) {
      printf("lvd_stop_vertex: error in get_cr(),  addr=0x%1x\n", addr);
      return -1;
    }
    if(cr_val & VTX_CR_RUN) {
      printf("lvd_stop_vertex: error, addr=0x%1x cr=0x%1x - running\n", addr, cr_val);
      return -1;
    } 
  }
  return 0;
}

plerrcode lvd_vertex_module_stop(struct lvd_dev* dev, int addr) {
  int res = 0;
  if (addr<0) {
    int card;
    for (card=0; card < dev->num_acards; card++) {
      if( dev->acards[card].initialized && IsVertexCard(dev, card)) {
	res |= lvd_stop_vertex(dev, dev->acards+card);
      }
    }
    return (res) ?  plErr_HW : plOK;
  } else {
    struct lvd_acard* acard = GetVertexCard(dev, addr);
    int res1 = 0;
    if(!acard) {
      printf("lvd_vertex_module_stop: error, no VERTEX card with address 0x%x known\n",
	     addr);
      return plErr_Program;
    }
    res1 = (acard->initialized) ? lvd_stop_vertex(dev, acard) : 0;
    return (res1) ? plErr_HW : plOK;
  }
}

/*****************************************************************************/
static int
lvd_clear_vertex(struct lvd_dev* dev, struct lvd_acard* acard)
{
    return 0;
}
/*****************************************************************************/
/*
 * RAM layout:
 * Bank 1:
 * 0x000000..0x000FFF: 4096*Pedestal LV
 * 0x001000..0x001FFF: 4096*Threshold LV
 * 0x002000..0x07FFFF: unused?
 * 
 * Bank 2:
 * 0x080000..0x0FFFFF: working area
 * 
 * Bank 3:
 * 0x100000..0x100FFF: 4096*Pedestal HV
 * 0x101000..0x101FFF: 4096*Threshold HV
 * 0x102000..0x17FFFF: unused?
 * 
 * Bank 4:
 * 0x180000..0x1FFFFF: working area
 * 
 * Addresses are addressing 16-bit words.
 * Each element of 'data' contains two 16-bit words.
 * In case of an odd start address only the lower 16 bit of the first
 * 32-bit word are used.
 * If the last address is odd (odd start address or odd num) only the
 * lower 16 bit of the last 32-bit word are used.
 */
static int
lvd_vertex_load_ram_(struct lvd_dev* dev, int addr, int ram_addr,
        int num, ems_u32* data)
{
    DEFINE_LVDTRACE(trace);
    int res=0;

    lvd_settrace(dev, &trace, 0);
    if (LVDTRACE_ACTIVE(trace))
        printf("vertex_load_ram_ (%s:0x%04x): 0x%x..0x%x \n",
            dev->pathname, addr, ram_addr, ram_addr+num-1);

    /* write address */
    res|=lvd_i_w(dev, addr, vertex.table_addr, ram_addr);
    /* write first 16-bit word, if address is odd */
    if (ram_addr&1) {
        res|=lvd_i_w(dev, addr, vertex.table_data.s, *data);
        data++; num--;
    }
    /* write all double-16-bit words */
    while (num>1) {
        res|=lvd_i_w(dev, addr, vertex.table_data.l, *data);
        data++; num-=2;
    }
    /* write remaining 16-bit word (if it exists) */
    if (num) { /* only 0 or 1 is possible here */
        res|=lvd_i_w(dev, addr, vertex.table_data.s, *data);
    }

    lvd_settrace(dev, 0, trace);
    return res;
}
/*****************************************************************************/
static int
lvd_vertex_fill_ram_(struct lvd_dev* dev, int addr, int ram_addr,
        int num, ems_u16 data)
{
    ems_u32 data2=(data<<16)|data;
    int res=0;
    DEFINE_LVDTRACE(trace);

    lvd_settrace(dev, &trace, 0);
    if (LVDTRACE_ACTIVE(trace))
        printf("vertex_fill_ram_ (%s:0x%04x): 0x%x..0x%x <-- 0x%x\n",
            dev->pathname, addr, ram_addr, ram_addr+num-1, data);

    /* write address */
    res|=lvd_i_w(dev, addr, vertex.table_addr, ram_addr);
    /* write first 16-bit word, if address is odd */
    if (ram_addr&1) {
        res|=lvd_i_w(dev, addr, vertex.table_data.s, data);
        data++; num--;
    }
    /* write all double-16-bit words */
    while (num>1) {
        res|=lvd_i_w(dev, addr, vertex.table_data.l, data2);
        data++; num-=2;
    }
    /* write remaining 16-bit word (if it exists) */
    if (num) { /* only 0 or 1 is possible here */
        res|=lvd_i_w(dev, addr, vertex.table_data.s, data);
    }

    lvd_settrace(dev, 0, trace);
    return res;
}
/*****************************************************************************/
static int
lvd_vertex_read_ram_(struct lvd_dev* dev, int addr, int ram_addr,
        int num, ems_u32* data)
{
    DEFINE_LVDTRACE(trace);
    int res=0;

    lvd_settrace(dev, &trace, 0);
    if (LVDTRACE_ACTIVE(trace))
        printf("vertex_read_ram_ (%s:0x%04x): 0x%x..0x%x\n",
            dev->pathname, addr, ram_addr, ram_addr+num-1);

    /* write address */
    res|=lvd_i_w(dev, addr, vertex.table_addr, ram_addr);
    /* read first 16-bit word, if address is odd */
    if (ram_addr&1) {
        res|=lvd_i_r(dev, addr, vertex.table_data.s, data);
        data++; num--;
    }
    /* read all double-16-bit words */
    while (num>1) {
        res|=lvd_i_r(dev, addr, vertex.table_data.l, data);
        data++; num-=2;
    }
    /* read remaining 16-bit word (if it exists) */
    if (num) { /* only 0 or 1 is possible here */
        res|=lvd_i_r(dev, addr, vertex.table_data.s, data);
    }

    lvd_settrace(dev, 0, trace);
    return res;
}
/*****************************************************************************/
plerrcode
lvd_vertex_load_ram(struct lvd_dev* dev, int addr, int unit,
        int ram_addr, int num, ems_u32* data)
{
    if (addr<0) {
        int card;
        for (card=0; card<dev->num_acards; card++) {
	  if(IsVertexCard(dev, addr)) {
	    plerrcode pres;
	    pres=lvd_vertex_load_ram(dev, dev->acards[card].addr,
				     unit, ram_addr, num, data);
	    if (pres!=plOK)
	      return pres;
	  }
        }
        return plOK;
    } else {
        int res=0;
	struct lvd_acard* acard=GetVertexCard(dev, addr);
	struct vtx_info* info = (acard) ? acard->cardinfo : 0;
	if(!info) {
	  printf("lvd_vertex_load_ram(): addr=0x%1x unit=%1d acard=%1p info=%1p - error\n",
		 addr, unit, acard, info);
	  return plErr_InvalidUse;
	}
        if (unit&1) {
	    res|=lvd_vertex_load_ram_(dev, addr, ram_addr+info[0].spec.ped_addr, num, data);
        }
        if (unit&2) {
	    res|=lvd_vertex_load_ram_(dev, addr, ram_addr+info[1].spec.ped_addr, num, data);
        }
        return res?plErr_HW:plOK;
    }
}
/*****************************************************************************/
plerrcode
lvd_vertex_fill_ram(struct lvd_dev* dev, int addr, int unit,
        int ram_addr, int num, ems_u16 data)
{
    if (addr<0) {
        int card;
        for (card=0; card<dev->num_acards; card++) {
            if (dev->acards[card].mtype==ZEL_LVD_ADC_VERTEX) {
                plerrcode pres;
                pres=lvd_vertex_fill_ram(dev, dev->acards[card].addr,
                        unit, ram_addr, num, data);
                if (pres!=plOK)
                    return pres;
            }
        }
        return plOK;
    } else {
      int res=0;
      struct lvd_acard* acard=GetVertexCard(dev, addr);
      struct vtx_info* info = (acard) ? acard->cardinfo : 0;
      if(!info) {
	printf("lvd_vertex_fill_ram(): addr=0x%1x unit=%1d acard=%1p info=%1p - error\n",
	       addr, unit, acard, info);
	return plErr_InvalidUse;
      }      
      if (unit&1) {
	res|=lvd_vertex_fill_ram_(dev, addr, ram_addr+info[0].spec.ped_addr, num, data);
      }
      if (unit&2) {
	res|=lvd_vertex_fill_ram_(dev, addr, ram_addr+info[1].spec.ped_addr, num, data);
      }
      return res?plErr_HW:plOK;
    }
}
/*****************************************************************************/
plerrcode
lvd_vertex_read_ram(struct lvd_dev* dev, int addr, int unit,
        int ram_addr, int num, ems_u32* data)
{
    int res=0;
    struct lvd_acard* acard=GetVertexCard(dev, addr);
    struct vtx_info* info = (acard) ? acard->cardinfo : 0;
    if(!info) {
      printf("lvd_vertex_read_ram(): addr=0x%1x unit=%1d acard=%1p info=%1p - error\n",
	     addr, unit, acard, info);
      return plErr_InvalidUse;
    }
    if (unit==1) {
        res|=lvd_vertex_read_ram_(dev, addr, ram_addr+info[0].spec.ped_addr, num, data);
    } else {
        res|=lvd_vertex_read_ram_(dev, addr, ram_addr+info[1].spec.ped_addr, num, data);
    }
    return res?plErr_HW:plOK;
}
/*****************************************************************************/
/*
 *                       RAM layout:
 *
 * VertexADC or VertexADCM2        VertexADCUN
 *
 *   0x00000..0x7FFFF: LV         0x00000..0x3FFFF
 *   0x80000..0xFFFFF: HV         0x40000..0x7FFFF
 * 
 * Addresses are addressing 16-bit words.
 * Each element of 'data' contains two 16-bit words.
 * In case of an odd start address only the lower 16 bit of the first
 * 32-bit word are used.
 * If the last address is odd (odd start address or odd num) only the
 * lower 16 bit of the last 32-bit word are used.
 */
static int
lvd_vertex_load_seq_(struct lvd_dev* dev, int addr, int seq_addr,
        int num, ems_u32* data)
{
    DEFINE_LVDTRACE(trace);
    int res=0;

    lvd_settrace(dev, &trace, 0);
    if (LVDTRACE_ACTIVE(trace))
        printf("lvd_vertex_load_seq_ (%s:0x%04x): 0x%x..0x%x\n",
            dev->pathname, addr, seq_addr, seq_addr+num-1);

    /* write address */
    res|=lvd_i_w(dev, addr, vertex.seq_addr, seq_addr);
    /* write first 16-bit word, if address is odd */
    if (seq_addr&1) {
        res|=lvd_i_w(dev, addr, vertex.seq_data.s, *data);
        data++; num--;
    }
    /* write all double-16-bit words */
    while (num>1) {
        res|=lvd_i_w(dev, addr, vertex.seq_data.l, *data);
        data++; num-=2;
    }
    /* write remaining 16-bit word (if it exists) */
    if (num) { /* only 0 or 1 is possible here */
        res|=lvd_i_w(dev, addr, vertex.seq_data.s, *data);
    }

    lvd_settrace(dev, 0, trace);
    return res;
}
/*****************************************************************************/
static int
lvd_vertex_fill_seq_(struct lvd_dev* dev, int addr, int seq_addr,
        int num, ems_u16 data)
{
    ems_u32 data2=(data<<16)|data;
    int res=0;
    DEFINE_LVDTRACE(trace);

    lvd_settrace(dev, &trace, 0);
    if (LVDTRACE_ACTIVE(trace))
        printf("vertex_fill_seq_ (%s:0x%04x): 0x%x..0x%x <-- 0x%x\n",
            dev->pathname, addr, seq_addr, seq_addr+num-1, data);

    /* write address */
    res|=lvd_i_w(dev, addr, vertex.seq_addr, seq_addr);
    /* write first 16-bit word, if address is odd */
    if (seq_addr&1) {
        res|=lvd_i_w(dev, addr, vertex.seq_data.s, data);
        data++; num--;
    }
    /* write all double-16-bit words */
    while (num>1) {
        res|=lvd_i_w(dev, addr, vertex.seq_data.l, data2);
        data++; num-=2;
    }
    /* write remaining 16-bit word (if it exists) */
    if (num) { /* only 0 or 1 is possible here */
        res|=lvd_i_w(dev, addr, vertex.seq_data.s, data);
    }

    lvd_settrace(dev, 0, trace);
    return res;
}
/*****************************************************************************/
static int
lvd_vertex_read_seq_(struct lvd_dev* dev, int addr, int seq_addr,
        int num, ems_u32* data)
{
    DEFINE_LVDTRACE(trace);
    int res=0;

    lvd_settrace(dev, &trace, 0);
    if (LVDTRACE_ACTIVE(trace))
        printf("vertex_read_seq_ (%s:0x%04x): 0x%x..0x%x\n",
            dev->pathname, addr, seq_addr, seq_addr+num-1);

    /* write address */
    res|=lvd_i_w(dev, addr, vertex.seq_addr, seq_addr);
    /* read first 16-bit word, if address is odd */
    if (seq_addr&1) {
        res|=lvd_i_r(dev, addr, vertex.seq_data.s, data);
        data++; num--;
    }
    /* read all double-16-bit words */
    while (num>1) {
        res|=lvd_i_r(dev, addr, vertex.seq_data.l, data);
        data++; num-=2;
    }
    /* read remaining 16-bit word (if it exists) */
    if (num) { /* only 0 or 1 is possible here */
        res|=lvd_i_r(dev, addr, vertex.seq_data.s, data);
    }

    lvd_settrace(dev, 0, trace);
    return res;
}
/*****************************************************************************/
plerrcode
lvd_vertex_load_sequencer(struct lvd_dev* dev, int addr, int unit,
        int seq_addr, int num, ems_u32* data)
{
    if (addr<0) {
        int card;
        for (card=0; card<dev->num_acards; card++) {
            if (dev->acards[card].mtype==ZEL_LVD_ADC_VERTEX) {
                plerrcode pres;
                pres=lvd_vertex_load_sequencer(dev, dev->acards[card].addr,
                        unit, seq_addr, num, data);
                if (pres!=plOK)
                    return pres;
            }
        }
        return plOK;
    } else {
        int res=0;

        if (unit&1) {
            res|=lvd_vertex_load_seq_(dev, addr, seq_addr, num, data);
        }
        if (unit&2) {
            res|=lvd_vertex_load_seq_(dev, addr, seq_addr+0x80000, num, data);
        }
        return res?plErr_HW:plOK;
    }
}
/*****************************************************************************/
plerrcode
lvd_vertex_fill_sequencer(struct lvd_dev* dev, int addr, int unit,
        int seq_addr, int num, ems_u16 data)
{
    if (addr<0) {
        int card;
        for (card=0; card<dev->num_acards; card++) {
            if (dev->acards[card].mtype==ZEL_LVD_ADC_VERTEX) {
                plerrcode pres;
                pres=lvd_vertex_fill_sequencer(dev, dev->acards[card].addr,
                        unit, seq_addr, num, data);
                if (pres!=plOK)
                    return pres;
            }
        }
        return plOK;
    } else {
        int res=0;

        if (unit&1) {
            res|=lvd_vertex_fill_seq_(dev, addr, seq_addr, num, data);
        }
        if (unit&2) {
            res|=lvd_vertex_fill_seq_(dev, addr, seq_addr+0x80000, num, data);
        }
        return res?plErr_HW:plOK;
    }
}
/*****************************************************************************/
plerrcode
lvd_vertex_read_sequencer(struct lvd_dev* dev, int addr, int unit,
        int seq_addr, int num, ems_u32* data)
{
    int res=0;

    if (unit==1) {
        res|=lvd_vertex_read_seq_(dev, addr, seq_addr, num, data);
    } else {
        res|=lvd_vertex_read_seq_(dev, addr, seq_addr+0x80000, num, data);
    }
    return res?plErr_HW:plOK;
}
/*****************************************************************************/
plerrcode
lvd_vertex_read_mem_file(struct lvd_dev* dev, int card, int ram, int addr,
        int num, const char* name)
{
    FILE* f;
    off_t reg;
    ems_u32 val;
    int res;
    plerrcode pres=plErr_System;
    DEFINE_LVDTRACE(trace);

    lvd_settrace(dev, &trace, 0);
    if (LVDTRACE_ACTIVE(trace))
        printf("vertex_read_mem_file (%s:0x%04x): ram=%d 0x%x..0x%x\n",
            dev->pathname, card, ram, addr, addr+num-1);

    f=fopen(name, "w");
    if (!f) {
        printf("vertex_read_mem_file: open %s: %s\n", name, strerror(errno));
        lvd_settrace(dev, 0, trace);
        return plErr_System;
    }

    if (ram) { /* pedestals and thresholds */
        if (lvd_i_w(dev, card, vertex.table_data, addr)<0)
            goto error;
        reg=ofs(union lvd_in_card, vertex.table_data.s);
    } else { /* sequencer */
        if (lvd_i_w(dev, card, vertex.seq_addr, addr)<0)
            goto error;
        reg=ofs(union lvd_in_card, vertex.seq_data.s);
    }

    for (; num>=0; num--) {
        res=lvd_i_r_(dev, card, reg, 2, &val);
        if (res!=0) {
            printf("read_mem_file: read error\n");
            pres=plErr_HW;
            goto error;
        }
        fprintf(f, "0x%04x\n", val);
    }
    pres=plOK;

error:
    lvd_settrace(dev, 0, trace);
    fclose(f);
    return pres;
}
/*****************************************************************************/
static int
vertex_wait_serial(struct lvd_dev* dev, int addr, int mask)
{
  int i;
  int max_pass = 10;
    struct timeval a, b;
    ems_u32 val, oval;
    int maxwait=0, tdiff;
    DEFINE_LVDTRACE(trace);
    const int MAXWAIT_ADC_DAC=200;
    const int MAXWAIT_VATA_DAC=2000;
    const int MAXWAIT_VATA_WRITE=6000;
    const int MAXWAIT_VATA_READ=1000;
    
    if (mask&VTX_SR_POTY_BUSY)
        maxwait=iMAX(maxwait, MAXWAIT_ADC_DAC);
    if (mask&VTX_SR_VADAC_BUSY)
        maxwait=iMAX(maxwait, MAXWAIT_VATA_DAC);
    if (mask&VTX_SR_VAREGOUT_BSY)
        maxwait=iMAX(maxwait, MAXWAIT_VATA_WRITE);
    if (mask&VTX_SR_VAREGIN_DVAL)
        maxwait=iMAX(maxwait, MAXWAIT_VATA_READ);

    lvd_settrace(dev, &trace, 0);
    gettimeofday(&a, 0);
    do {
      for(i=0; i< max_pass; i++) { /* protectiorn loop */
        if(!lvd_i_r(dev, addr, vertex.sr, &oval)) break;
      }
      gettimeofday(&b, 0);
      tdiff=(b.tv_sec-a.tv_sec)*1000000+(b.tv_usec-a.tv_usec);
      /* invert the bits for VATA input FIFO */
      val=oval^VTX_SR_VAREGIN_DVAL;
    } while ((val&mask ) && tdiff<=maxwait);
    lvd_settrace(dev, 0, trace);
    if (val&mask) {
        printf("vertex_wait_serial timeout while waiting for 0x%x\n", mask);
        return -1;
    }
    return 0;
}

/*****************************************************************************/
/* set the ADC poti, no read back! */
plerrcode
lvd_vertex_adc_dac(struct lvd_dev* dev, int addr, int unit, int value) {
  ems_u32 sr;
  int res=0, count=0, max_count=100;
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  if (!acard) {
    printf("lvd_vertex_adc_dac: error, no VERTEX card with address 0x%x known\n",
	   addr);
    return plErr_Program;
  }    
  /* write data */
  if (unit&1) {
    if(acard->mtype == ZEL_LVD_ADC_VERTEX) {
      res|=lvd_i_w(dev, addr, vertex.lv.v.poti, value);
    } else {
      res|=lvd_i_w(dev, addr, vertex.lv.m.poti, value);
    }
  } else {
    if(acard->mtype == ZEL_LVD_ADC_VERTEX) {
      res|=lvd_i_w(dev, addr, vertex.hv.v.poti, value);
    } else {
      res|=lvd_i_w(dev, addr, vertex.hv.m.poti, value);
    }
  }
  /* wait for ready */
  do {
    lvd_i_r(dev, addr, vertex.sr, &sr);
    count++;
  } while((sr & VTX_SR_POTY_BUSY) && (count < max_count));
  return res?plErr_HW:plOK;
}

/****************************************************************************
 * 
 */
plerrcode
lvd_vertex_aclk_par_read(struct lvd_dev* dev, int addr, int unit, int* value) {
  int res=0;
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  struct vtx_info* info = (acard) ? acard->cardinfo : 0;
  if(!info) {
    printf("lvd_vertex_aclk_par_read: addr=0%1x unit=%1d acard=%1p info=%1p - error\n",
	   addr, unit, acard, info);
    return plErr_BadModTyp;
  }
  if(unit<1 || unit>2) return plErr_InvalidUse;
  res = lvd_i_r_(dev, addr, info[unit-1].spec.aclk_par, 2, value);
  return res?plErr_HW:plOK;
}


/****************************************************************************
 * 
 */
plerrcode
lvd_vertex_aclk_par_write(struct lvd_dev* dev, int addr, int unit, int value) {
  int res=0;
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  struct vtx_info* info = (acard) ? acard->cardinfo : 0;
  if(!info) {
    printf("lvd_vertex_aclk_par_write: addr=0%1x unit=%1d acard=%1p info=%1p - error\n",
	   addr, unit, acard, info);
    return plErr_BadModTyp;
  }
  if(unit<1 || unit>2) return plErr_InvalidUse;
  res  = lvd_i_w_(dev, addr, info[unit-1].spec.aclk_par, 2, value);
  return res?plErr_HW:plOK;
}
/*****************************************************************************/
plerrcode
lvd_vertex_dgap_len_read(struct lvd_dev* dev, int addr, int unit, int idx, int* value) {
    int res=0;
    struct lvd_acard* acard = GetVertexCard(dev, addr);
    if (!acard) {
      printf("lvd_vertex_dgap_len: error, no VERTEX card with address 0x%x known\n",
	     addr);
      return plErr_Program;
    }    
    if (unit&1) {
      res|=lvd_i_r(dev, addr, vertex.lv.dgap_len[idx], value);
    }
    if (unit&2) {
      res|=lvd_i_r(dev, addr, vertex.hv.dgap_len[idx], value);
    }
    return res?plErr_HW:plOK;
}
plerrcode
lvd_vertex_dgap_len_write(struct lvd_dev* dev, int addr, int unit, int idx, int value) {
    int res=0;
    struct lvd_acard* acard = GetVertexCard(dev, addr);
    if (!acard) {
      printf("lvd_vertex_dgap_len: error, no VERTEX card with address 0x%x known\n",
	     addr);
      return plErr_Program;
    }    
    if (unit&1) {
      res|=lvd_i_w(dev, addr, vertex.lv.dgap_len[idx], value);
    }
    if (unit&2) {
      res|=lvd_i_w(dev, addr, vertex.hv.dgap_len[idx], value);
    }
    return res?plErr_HW:plOK;
}


/*****************************************************************************/
plerrcode
lvd_vertex_seq_loopcount(struct lvd_dev* dev, int addr, int unit, int idx,
    int value) {
#if VTX_DEBUG
  printf("seq_loopcount addr=0x%x unit=%d idx=%d value=%d\n", 
	 addr, unit, idx, value);
#endif
  if (addr<0) {
    int card;
    for (card=0; card<dev->num_acards; card++) {
      if(IsVertexCard(dev, card)) {
	plerrcode pres = lvd_vertex_seq_loopcount(dev, dev->acards[card].addr, unit,
						  idx, value);
	if (pres!=plOK)
	  return pres;
      }
    }
    return plOK;
  } else {
    int res=0;
    struct lvd_acard* acard = GetVertexCard(dev, addr);
    if (!acard) {
      printf("lvd_vertex_seq_loopcount: error, no VERTEX card with address 0x%x known\n",
	     addr);
      return plErr_Program;
    }
    if (unit&1) {
      res|=lvd_i_w(dev, addr, vertex.lv.lp_counter[idx], value);
    }
    if (unit&2) {
      res|=lvd_i_w(dev, addr, vertex.hv.lp_counter[idx], value);
    }
    return res?plErr_HW:plOK;
  }
}
/*****************************************************************************/
plerrcode
lvd_vertex_seq_get_loopcount(struct lvd_dev* dev, int addr, ems_u32 *counts)
{
    int res=0;
    struct lvd_acard* acard = GetVertexCard(dev, addr);
    if (!acard) {
      printf("lvd_vertex_module_start: error, no VERTEX card with address 0x%x known\n",
	     addr);
      return plErr_Program;
    }
    res|=lvd_i_r(dev, addr, vertex.lv.lp_counter[0], counts+0);
    res|=lvd_i_r(dev, addr, vertex.lv.lp_counter[1], counts+1);
    res|=lvd_i_r(dev, addr, vertex.hv.lp_counter[0], counts+2);
    res|=lvd_i_r(dev, addr, vertex.hv.lp_counter[1], counts+3);
    return res ? plErr_HW : plOK;
}
/*****************************************************************************/
plerrcode
lvd_vertex_aclk_delay_write(struct lvd_dev* dev, int addr, int unit, int value) {
  int res=0;
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  if (!acard) {
    printf("lvd_vertex_aclk_delay: error, no VERTEX card with address 0x%x known\n",
	   addr);
    return plErr_Program;
  }
  if (unit&1) {
    res|=lvd_i_w(dev, addr, vertex.lv.clk_delay, value);
  } else {
    res|=lvd_i_w(dev, addr, vertex.hv.clk_delay, value);
  }
  return res ? plErr_HW : plOK;
}

plerrcode
lvd_vertex_aclk_delay_read(struct lvd_dev* dev, int addr, int unit, int* value) {
  int res=0;
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  if (!acard) {
    printf("lvd_vertex_aclk_delay: error, no VERTEX card with address 0x%x known\n",
	   addr);
    return plErr_Program;
  }
  if (unit&1) {
    res|=lvd_i_r(dev, addr, vertex.lv.clk_delay, value);
  } else {
    res|=lvd_i_r(dev, addr, vertex.hv.clk_delay, value);
  }
  return res ? plErr_HW : plOK;
}

/*****************************************************************************/
plerrcode
lvd_vertex_xclk_delay(struct lvd_dev* dev, int addr, int unit, int value) {
  int res=0;
  int idx = unit - 1;
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  struct vtx_info* info = (acard) ? acard->cardinfo : 0;
  if (!info) {
    printf("lvd_vertex_xclk_delay: no VertexADC card with address 0x%1x known, error\n",
	   addr);
    return plErr_BadModTyp;
  }
  if(info[idx].chiptype != VTX_CHIP_MATE3) {
    printf("lvd_vertex_xclk_delay: addr=0x%1x idx=%1d - not MATE3 chips, error\n",
	   addr, idx);
    return plErr_InvalidUse;
  }
  res = lvd_i_w(dev, addr, vertex.lv.m.xclk_del, value);
  return res ? plErr_HW : plOK;
}

/*****************************************************************************/
plerrcode
lvd_vertex_nrchan_write(struct lvd_dev* dev, int addr, int unit, int value) {
  int res=0;
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  if (!acard) {
    printf("lvd_vertex_nrchan: error, no VERTEX card with address 0x%x known\n",
	   addr);
    return plErr_Program;
  } 
  if (unit&1) {
    res|=lvd_i_w(dev, addr, vertex.lv.nr_chan, value);
  } else {
    res|=lvd_i_w(dev, addr, vertex.hv.nr_chan, value);
  }
  return res ? plErr_HW : plOK;
}
/*****************************************************************************/
plerrcode
lvd_vertex_nrchan_read(struct lvd_dev* dev, int addr, int unit, int* value) {
  int res=0;
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  if (!acard) {
    printf("lvd_vertex_nrchan: error, no VERTEX card with address 0x%x known\n",
	   addr);
    return plErr_Program;
  } 
  if (unit&1) {
    res|=lvd_i_r(dev, addr, vertex.lv.nr_chan, value);
  } else {
    res|=lvd_i_r(dev, addr, vertex.hv.nr_chan, value);
  }
  return res ? plErr_HW : plOK;
}
/*****************************************************************************/
plerrcode
lvd_vertex_noisewin_write(struct lvd_dev* dev, int addr, int unit, int value) {
  int res=0;
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  if (!acard) {
    printf("lvd_vertex_noisewin: error, no VERTEX card with address 0x%x known\n",
	   addr);
    return plErr_Program;
  }
  if (unit&1) {
    res|=lvd_i_w(dev, addr, vertex.lv.noise_thr, value);
  } else {
      res|=lvd_i_w(dev, addr, vertex.hv.noise_thr, value);
  }
  return res?plErr_HW:plOK;
}
/*****************************************************************************/
plerrcode
lvd_vertex_noisewin_read(struct lvd_dev* dev, int addr, int unit, int* value) {
  int res=0;
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  if (!acard) {
    printf("lvd_vertex_noisewin: error, no VERTEX card with address 0x%x known\n",
	   addr);
    return plErr_Program;
  }
  if (unit&1) {
    res|=lvd_i_r(dev, addr, vertex.lv.noise_thr, value);
  } else {
    res|=lvd_i_r(dev, addr, vertex.hv.noise_thr, value);
  }
  return res?plErr_HW:plOK;
}

/*****************************************************************************
 * If sequencer is not running - returns immediately,
 * else tries to wait until "VTX_SEQ_CSR_LV_BUSY" and 
 * VTX_SEQ_CSR_HV_BUSY is reset.
*/
plerrcode lvd_vertex_seq_wait(struct lvd_dev* dev, int addr, int unit) {
  if (addr<0) {
    int card;
    int res=0;
    plerrcode ret = plOK; /* last error met, if any */
    for (card=0; card<dev->num_acards; card++) {
      if(IsVertexCard(dev, card)) {
	plerrcode pres = lvd_vertex_seq_wait(dev, dev->acards[card].addr, unit);
	if (pres != plOK) {
	  ret = pres;
	  res++;
	}
      }
    }
    return ret;
  } else {
    ems_u32 cr;
    struct lvd_acard* acard = GetVertexCard(dev, addr);
    if (!acard) {
      printf("lvd_vertex_seq_wait: error, no VERTEX card with address 0x%x known\n",
	     addr);
      return plErr_Program;
    }
    if(acard->initialized ==0) return plOK;
    int res = lvd_i_r(dev, addr, vertex.cr, &cr);
    if(res) 
      return plErr_HW;
    if((cr & VTX_CR_RUN) != VTX_CR_RUN) /* sequencer is not running */
      return plOK;
    res = wait_seq_ready(dev, addr, unit);
    return (res) ? plErr_Busy : plOK;
  }
}

/******************************************************************************
 * USE exec_seq_func() instead of exec_seq_switch()!
 * Set sequencer all switches and start/stop sequencer(enable=1), if it is not running
 * Wait completion of procedure
 * idx: 0 - LV, 1 - HV, others - error
 */
static int exec_seq_switch(struct lvd_dev* dev, int addr,
			   int idx, ems_u32 value, int enable_seq) {
  int unit;
  int res = 0;
  if (idx == 0) {
    unit = 1;
    res|=lvd_i_w(dev, addr, vertex.lv.swreg, value);
    printf("write lv.swreg:  0x%x\n", value);
  }
  else if (idx == 1) {
    unit = 2;
    res|=lvd_i_w(dev, addr, vertex.hv.swreg, value);
    printf("write hv.swreg:  0x%x\n", value);
  }
  else {
    printf("exec_seq_switch: invalid idx=%1d of addr=0x%1x, 0/1 allowed\n", 
	   addr, idx);
    return -1;
  }
  lvd_vertex_seq_wait(dev, addr, unit);

  if (enable_seq) {
    ems_u32 ccc=0;
    /* start activities */
    lvd_i_w(dev, addr, vertex.cr, VTX_CR_RUN|VTX_CR_IH_LV|VTX_CR_IH_HV);
    /* Wait for completion */
    ccc=lvd_vertex_seq_wait(dev, addr, unit);
    /* stop activities */
    lvd_i_w(dev, addr, vertex.cr, 0);
    printf("seq_switch: ccc=%d\n", ccc);
  }
 
  return res;
}

/*****************************************************************************/
/*  USE lvd_vertex_exec_seq_func() instead of lvd_vertex_seq_switch()!
 *
 *Sequencer may be started and stoped again by C_RUN,
 * if enable_seq == 1. It may cause problems with halves of modules.
 */
plerrcode
lvd_vertex_seq_switch(struct lvd_dev* dev, int addr, int unit, int idx,
        ems_u32 value, int enable_seq) {
  printf("seq_switch addr=0x%x unit=%d idx=%d value=0x%x enable=%d\n",
	 addr, unit, idx, value, enable_seq);
  if (addr<0) {
    int card;
    plerrcode last_err = plOK;
    for (card=0; card<dev->num_acards; card++) {
      if(IsVertexCard(dev, card)) {
	plerrcode pres = lvd_vertex_seq_switch(dev, dev->acards[card].addr, unit,
					       idx, value, enable_seq);
	if (pres!=plOK) {
	  last_err = pres;
	}
      }
    }
    return last_err;
  } else {
    int res=0;
    plerrcode pres;
    struct lvd_acard* acard = GetVertexCard(dev, addr);
    if (!acard) {
      printf("lvd_vertex_seq_switch: error, no VERTEX card with address 0x%x known\n",
	     addr);
      return plErr_Program;
    }

    /* only for checks, delete them later */
    /* wait sequencer ready, add wait serial here? */
    pres = lvd_vertex_seq_wait(dev, addr, unit);
    if(pres != plOK) {
      printf("***** vertex_seq_switch: sequencer not ready\n");
      return pres;
    }
    
    if (enable_seq) {
      ems_u32 cr, seq_csr;
      /* read cr and seq_csr */
      lvd_i_r(dev, addr, vertex.cr, &cr);
      lvd_i_r(dev, addr, vertex.seq_csr, &seq_csr);
      if (seq_csr&(VTX_SEQ_CSR_LV_BUSY|VTX_SEQ_CSR_HV_BUSY)) {
	printf("vertex_seq_switch: sequencer not idle: "
	       "cr=0x%x seq_csr=0x%x\n", cr, seq_csr);
	return plErr_Busy;
      }
      if (cr&1) {
	printf("vertex_seq_switch: sequencer already enabled\n");
	enable_seq=0;
      }
    }
    
    if (unit&1) {
      if (idx<0) {
	res |= exec_seq_switch(dev, addr, (unit&1)-1, value, 0); /* no exec */
      } else {
	ems_u32 val;
	val=(((value&0xf)|0x8)<<(idx*4));
	printf("write 0x%04x to lv.swreg\n", val);
	res|=lvd_i_w(dev, addr, vertex.lv.swreg, val);
	lvd_vertex_seq_wait(dev, addr, unit&1);
      }
    }
    if (unit&2) {
      if (idx<0) {
	res |= exec_seq_switch(dev, addr, (unit&2)-1, value, 0); /* no exec */
      } else {
	ems_u32 val;
	val=(((value&0xf)|0x8)<<(idx*4));
	printf("write 0x%04x to hv.swreg\n", val);
	res|=lvd_i_w(dev, addr, vertex.hv.swreg, val);
	lvd_vertex_seq_wait(dev, addr, unit&2);
      }
    }

    if (enable_seq) {
      ems_u32 ccc=0;
      /* start activities */
      lvd_i_w(dev, addr, vertex.cr, VTX_CR_RUN|VTX_CR_IH_LV|VTX_CR_IH_HV);
      /* Wait for completion */
      ccc=lvd_vertex_seq_wait(dev, addr, unit);
      lvd_i_w(dev, addr, vertex.cr, 0);
      printf("seq_switch: ccc=%d\n", ccc);
    }
    return res?plErr_HW:plOK;
  }
}


/*****************************************************************************/
plerrcode
lvd_vertex_get_seq_switch(struct lvd_dev* dev, int addr, int unit,
        ems_u32* data) {
  int res=0;

  if (unit&1) {
    res|=lvd_i_r(dev, addr, vertex.lv.swreg, data++);
  }
  if (unit&2) {
    res|=lvd_i_r(dev, addr, vertex.hv.swreg, data++);
  }
  return res?plErr_HW:plOK;
}
/*****************************************************************************/
plerrcode
lvd_vertex_seq_csr(struct lvd_dev* dev, int addr, int value) {
#if VTX_DEBUG
  printf("seq_csr addr=0x%x value=0x%x\n", addr, value);
#endif
  if (addr<0) {
    int card;
    for (card=0; card<dev->num_acards; card++) {
      if(IsVertexCard(dev, card)) {
	plerrcode pres = lvd_vertex_seq_csr(dev, dev->acards[card].addr, value);
	if (pres!=plOK)
	  return pres;
      }
    }
    return plOK;
  } else {
    int res;
    struct lvd_acard* acard = GetVertexCard(dev, addr);
    if (!acard) {
      printf("lvd_vertex_module_start: error, no VERTEX card with address 0x%x known\n",
	     addr);
      return plErr_Program;
    }
    res=lvd_i_w(dev, addr, vertex.seq_csr, value);
    return res?plErr_HW:plOK;
  }
}
/*****************************************************************************/
plerrcode
lvd_vertex_get_seq_csr(struct lvd_dev* dev, int addr, ems_u32* value) {
  return (get_seq_csr(dev, addr, 0, value)) ? plErr_HW:plOK;
}
/*****************************************************************************/
plerrcode
lvd_vertex_seq_count(struct lvd_dev* dev, int addr, ems_u32* data) {
  int res=0;
  res|=lvd_i_r(dev, addr, vertex.lv.counter[0], data++);
  res|=lvd_i_r(dev, addr, vertex.lv.counter[1], data++);
  res|=lvd_i_r(dev, addr, vertex.hv.counter[0], data++);
  res|=lvd_i_r(dev, addr, vertex.hv.counter[1], data++);
  return res?plErr_HW:plOK;
}
/*****************************************************************************/
plerrcode
lvd_vertex_cmodval(struct lvd_dev* dev, int addr, ems_u32* data) {
  int res=0;
  struct lvd_acard* acard=GetVertexCard(dev, addr);
  if(!acard) {
    return plErr_BadModTyp;
  }
  if(acard->mtype == ZEL_LVD_ADC_VERTEX) {
    res|=lvd_i_r(dev, addr, vertex.lv.v.comval, data++);
    res|=lvd_i_r(dev, addr, vertex.hv.v.comval, data++);
  }
  else {
    res|=lvd_i_r(dev, addr, vertex.lv.m.comval, data++);
    res|=lvd_i_r(dev, addr, vertex.hv.m.comval, data++);
  }
  return res ? plErr_HW : plOK;
}
/*****************************************************************************/
plerrcode
lvd_vertex_nrwinval(struct lvd_dev* dev, int addr, ems_u32* data) {
  int res=0;
  struct lvd_acard* acard=GetVertexCard(dev, addr);
  if(!acard) {
    return plErr_BadModTyp;
  }
  if(acard->mtype == ZEL_LVD_ADC_VERTEX) {
    res|=lvd_i_r(dev, addr, vertex.lv.v.comcnt, data++);
    res|=lvd_i_r(dev, addr, vertex.hv.v.comcnt, data++);
  }
  else {
    res|=lvd_i_r(dev, addr, vertex.lv.m.comcnt, data++);
    res|=lvd_i_r(dev, addr, vertex.hv.m.comcnt, data++);
  }
  return res ? plErr_HW : plOK;
}
/*****************************************************************************/
plerrcode
lvd_vertex_sample(struct lvd_dev* dev, int addr, ems_u32* data) {
  int res=0;
  struct lvd_acard* acard=GetVertexCard(dev, addr);
  if(!acard) {
    return plErr_BadModTyp ;
  }
  if(acard->mtype == ZEL_LVD_ADC_VERTEX) {
    res|=lvd_i_r(dev, addr, vertex.lv.v.adc_value, data++);
    res|=lvd_i_r(dev, addr, vertex.hv.v.adc_value, data++);
  }
  else {
    res|=lvd_i_r(dev, addr, vertex.lv.m.adc_value, data++);
    res|=lvd_i_r(dev, addr, vertex.hv.m.adc_value, data++);
  }
  return res?plErr_HW:plOK;
}

static plerrcode load_rep_dacs(struct lvd_dev* dev, struct lvd_acard* acard, int idx,
			       int num, ems_u32* data) {	 
  int i;
  int pass;
  int max_pass=10;
  int res=0;
  ems_u32 mask = (idx) ? VTX_SR_HV_VADAC_BUSY : VTX_SR_LV_VADAC_BUSY;
  struct vtx_info* info = (acard) ? acard->cardinfo : 0;
  if(!info) { /* for any chance */
    printf("load_rep_dacs(): idx=%1d\n acard==%p | info==%p - error\n", idx, acard, 
	   &info[idx]);
    return plErr_BadModTyp;
  }
  int sr=-1;
  int addr = acard->addr;
  if(lvd_i_r(dev, addr, vertex.sr, &sr) < 0) {
    printf("load_rep_dacs: addr=0x%1x idx=%1d : sr read error\n",
	   addr, idx);
    return plErr_HW;
  }
  info[idx].loaded &= (~DACS_LOADED); /* clear signature that DACS is loaded */
  /* write data */
  for(pass=0; pass < max_pass; pass++) {
    res = 0;
    for (i=0; i<num; i++) {
      res|=lvd_i_w_(dev, addr, info[idx].spec.rep_dacs, 2, data[i]);
    }
    if(res) {
      return plErr_HW;
    }
    /* wait completition */
    res |= vertex_wait_serial(dev, addr, mask); /* protection loop inside */
    if(!res) break;
  }
  if(res) {
    printf("load_rep_dacs(): addr=0x%1x idx=%1d: error\n", addr, idx);
    return plErr_HW;
  }
  info[idx].loaded |= DACS_LOADED;
  return plOK;
}

/*****************************************************************************/
/*
 * dev        - device
 * addr       - module addr (absolute address, -1 is forbidden) 
 * unit       - (1: LV, 2: HV, other: error)
 * num        - number of data words to load, now must be 9!
 * data[0..7] -  data words: value|(chan<<12)
 * data[8]    -  control word: normally 0x803c
 */
plerrcode lvd_vertex_vata_dac(struct lvd_dev* dev, int addr, int unit,
			      int num, ems_u32* data) 
{
  int i;  
  int idx= unit-1;
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  struct vtx_info *info = (acard) ?  acard->cardinfo : 0;
  if (!info) {
    printf("lvd_vertex_module_start: error, no VERTEX card with address 0x%x known\n",
	   addr);
    return plErr_BadModTyp;
  }
  /* sanity check */
  int dac_num = sizeof(info[idx].dac) / sizeof(info[idx].dac[0]);
  if(num != dac_num) {
    printf("lvd_vertex_vata_dac: invalid dac_num=%1d, must be %1d, error\n",
	   num, dac_num);
    return plErr_ArgRange;
  }
  /* save dac values and control word in vtx_info */
  for (i=0; i<num-1; i++) {
    info[idx].dac[i] = (i<<12) | (data[i]& 0xfff);
  }
  info[idx].dac[num-1] = data[num-1]; /* control word */
  /* write to front-end card */
  return load_rep_dacs(dev, acard, idx, num, info[idx].dac);
}

/*****************************************************************************
 * Changes value of one DAC. For tuning/calibration only. 
 * dev   : device
 * addr  : module addr (absolute address or -1 for all) 
 * unit  : (1: LV, 2: HV, other: error)
 * num   : DAC number, in [0,7] range
 * value : new DAC value
 * 
 * DACs must be loaded before!!! and info[].dac[1] != 0 !!!
 */
plerrcode
lvd_vertex_vata_dac_change(struct lvd_dev* dev, int addr, int unit,
			   int num, ems_u32 value) {
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  struct vtx_info *info = (acard) ? acard->cardinfo : 0; /* fro any chance */
  int idx = unit -1 ; /* subunit: 0 | 1 */
  if (!info) {
    printf("lvd_vertex_vata_dac_change: addr=0x%1x idx=%1d, either acard or info is null, error\n",
	   addr, idx);
    return plErr_BadModTyp;
  } 
  /* write data */
  if(!IS_DACS_LOADED(info[idx].loaded)) {
    printf("vertex_vata_dac_change: DACs of 0x%1x unit=%1d were not loaded yet, error\n",
	   addr, unit);
    return plErr_InvalidUse;
  }
  info[idx].dac[num] = (num << 12) | (value & 0xfff);
  return load_rep_dacs(dev, acard, idx, 9, info[idx].dac);
}

/*****************************************************************************/
/*
 * see lvd_vertex_rw_vata for arguments
 * 'bits' must not exceed 16*VATA_FIFO_SIZE
 */
static int
lvd_vertex_rw_vata_part(struct lvd_dev* dev, int addr, int idx, int bits,
    ems_u32 *out, ems_u32 *in) {
  int words, i;
  ems_u32 val;
  struct lvd_acard* acard=GetVertexCard(dev, addr);
  struct vtx_info*  info = acard->cardinfo;
  int reg_cr_offs   = info[idx].spec.ser_csr;
  int reg_data_offs = info[idx].spec.ser_data;
  int REGOUT_BSY = idx ? VTX_SR_HV_VAREGOUT_BSY:VTX_SR_LV_VAREGOUT_BSY;
  int REGIN_DVAL =idx ? VTX_SR_HV_VAREGIN_DVAL:VTX_SR_LV_VAREGIN_DVAL;

  words=(bits+15)/16;
  lvd_i_r_(dev, addr, reg_cr_offs, 2, &val);
  lvd_i_w_(dev, addr, reg_cr_offs, 2,(val&0xf000)+bits-1);
  if (out) {
    for (i=0; i<words; i++)
      lvd_i_w_(dev, addr, reg_data_offs, 2, out[i]);
  } else {
    for (i=0; i<words; i++)
      lvd_i_w_(dev, addr, reg_data_offs, 2, 0);
  }
  
  if (vertex_wait_serial(dev, addr, REGOUT_BSY)<0) { /* protection loop inside */
    printf("lvd_vertex_rw_vata:  at addr=0x%1x idx=%1d - waiting for serial output, error\n",
	   addr, idx);
    return -1;
  }

  if (in) {
    for (i=0; i<words; i++) {
      if (vertex_wait_serial(dev, addr, REGIN_DVAL)<0) { /* protection loop inside */
	printf("lvd_vertex_rw_vata: at addr=0x%1x idx=%1d - waiting for serial input, error\n",
	       addr, idx);
	return -1;
      }
      lvd_i_r_(dev, addr, reg_data_offs, 2, in+i);
    }
  }
  return 0;
}
static int clear_vata_fifo(struct lvd_dev* dev, int addr, int idx) {
  struct lvd_acard* acard=GetVertexCard(dev, addr);
  struct vtx_info*  info = acard->cardinfo;
  int ser_data_offs = info[idx].spec.ser_data;

  int REGIN_DVAL =idx ? VTX_SR_HV_VAREGIN_DVAL:VTX_SR_LV_VAREGIN_DVAL;
  ems_u32 sr;
  ems_u32 data;
  int res;

  res = lvd_i_r(dev, addr, vertex.sr, &sr);
  if(res < 0) {
    printf("clear_vata_fifo: addr=0x%1x idx=%1d: failed read SR\n", addr, idx);
    return -1;
  }
  int pass = 0;
  while((sr & REGIN_DVAL) && (pass<100)) { /* there is data in fifo */
    res = lvd_i_r_(dev, addr, ser_data_offs, 2, &data);
    if(res< 0) {
      printf("clear_vata_fifo: addr=0x%1x idx=%1d: failed read data\n", addr, idx);
    }
    res = lvd_i_r(dev, addr, vertex.sr, &sr);
    if(res < 0) {
      printf("clear_vata_fifo: addr=0x%1x idx=%1d: failed read SR, pass=%1d\n", addr, idx, pass);
    }
    pass++;
  }
  res = lvd_i_r(dev, addr, vertex.sr, &sr);
  if(res < 0) {
    printf("clear_vata_fifo: addr=0x%1x idx=%1d: failed read SR\n", addr, idx);
    return -1;
  }  
  return 0;
}

/*****************************************************************************/
/*
 * 'unit' is 0 for LV and 1 for HV
 * 'out' or 'in' (or both) may be NULL
 * 0-bits are written to VATA if 'out' is NULL
 * data read from VATA are discarded if 'in' is NULL
 * only the lower 16 bit of 'out' and 'in' are used
 */
static int
lvd_vertex_rw_vata(struct lvd_dev* dev, int addr, int unit, int bits,
    ems_u32 *out, ems_u32 *in)
{
    const int maxbits=VATA_FIFO_SIZE*16;

    while (bits>maxbits) {
        if (lvd_vertex_rw_vata_part(dev, addr, unit, maxbits, out, in)<0)
            return -1;
        if (in)
            in+=VATA_FIFO_SIZE;
        if (out)
            out+=VATA_FIFO_SIZE;
        bits-=maxbits;
    }
    if (bits) {
      if (lvd_vertex_rw_vata_part(dev, addr, unit, bits, out, in)<0) {
	clear_vata_fifo(dev, addr, unit);
	return -1;
      }
    }
    clear_vata_fifo(dev, addr, unit);
    return 0;
}

/*****************************************************************************
 * Check, is sequencer stopped, if not - inhibit trigger.
 * Attempt chip load with running sequencer may cause conflicts 
 * So, if it running, we inhibit trigger and wait sequencer ready
 * RETURNS:
 *  0 : OK, old_cr contains previous status of register
 * -1 : error, if old_cr == -1, we failed read cr.
 *      in other case, before return we write old_cr in the module!
 */
static int prep_safe_load(struct lvd_dev* dev, struct lvd_acard* acard,
			    int idx, ems_u32* old_cr) {
  int res;
  ems_u32 cr;
  int addr = acard->addr;
  /* read cr */
  *old_cr = -1;
  res = get_cr(dev, addr, &cr); /* protection loop inside */
  if(res) {
    printf("prep_safe_load: get_cr(), addr=0x%1x idx=%1d - error\n",
	   addr, idx);
    return -1;
  }
  /* inhibit trigger */
  *old_cr = cr;
  cr |= VTX_CR_IH_TRG;
  res = lvd_i_w(dev, addr, vertex.cr, cr);
  if(res) {
    printf("prep_safe_load: write cr, addr=0x%1x idx=%1d - error\n",
	   addr, idx);
    lvd_i_w(dev, addr, vertex.cr, *old_cr);
    return -1;
  }
  /* wait sequencer ready, for any chance */
  res = wait_seq_ready(dev, addr, idx+1);
  if(res) {
    printf("prep_safe_load: wait_seq_ready(), addr=0x%1x idx=%1d - busy\n",
	   addr, idx);
    lvd_i_w(dev, addr, vertex.cr, *old_cr);
    return -1;
  }
  return 0;
}

/*****************************************************************************
 * Must be called, ONLY from  lvd_vertex_load_chips() or another procedure,
 * which checks that module and front-end are ready to load chips!
 *
 * Checks delay to read-out correctly the number of chips (based on words number),
 * set delay and number of bits to read-out in correspondence with found values,
 * load chips by data[].
 *
 * It is supposed, that VertexADC and VertexADCUN have the same:
 *   - mreset_va(dev,...) 
 *   - find_vata_chip_delay(dev,..)
 *       - set_vata_chip_delay(dev,..)
 *       - lvd_vertex_count_vata_bits(dev,..)
 *       - set_vata_chip_bits(dev,..)
 *   - lvd_vertex_rw_vata(dev,...)
 *       - lvd_vertex_rw_vata_part()
 *   - vertex.[l/h]v.nr_chan - offset where to write (number of board channels - 1)
 *   - VAREGOUT_BSY and VAREGIN_DVAL of "sr" register
 ******************************************************************************/
static plerrcode load_vata_chips(struct lvd_dev* dev, struct lvd_acard* acard,
				 int idx, int words, ems_u32* data) {
  int res;
  int n_chips = 0;
  int bits;
  int addr = acard->addr;
  struct vtx_info *info = (struct vtx_info*)acard->cardinfo;
  /* is it connected to VA32TA2 chips? */
  if(info[idx].chiptype != VTX_CHIP_VA32TA2) { /* for any chance */
    printf("load_vata_chips: addr=0x%1x, idx=%1d chip type=%1d  - not VA32TA2 chips in info, error\n",
	   addr, idx, info[idx].chiptype);
    return plErr_BadModTyp ;
  }
  /* how many chips expected */
  int exp_chips = words*16 / VA32TA2_BITS;
  /* is number of words correct? */
  if(((words*16) % VA32TA2_BITS) > 15) {
    int corr_nw= (exp_chips*VA32TA2_BITS/16);
    if((exp_chips*VA32TA2_BITS) % 16) corr_nw++;
    printf("load_vata_chips: addr=0x%1x, idx=%1d: nw=%1d, must be %1d, error\n",
	   addr, idx, words, corr_nw);
    return plErr_ArgRange;
  }
  /* check number of chips at current delay and period (scaling) */  
  bits = exp_chips*VA32TA2_BITS; /* all available chips must be initialized */
  int n_bits = lvd_vertex_count_vata_bits(dev, acard->addr, idx, words);
  if(n_bits != bits) { /* failed count proper number of bits */
    /* find delay to read back chip settings */    
    if(find_vata_chip_delay(dev, acard, idx, exp_chips*VA32TA2_BITS) <= 0) {
      printf("load_vata_chips(): addr=0x%1x idx=%1d expected %1d chips, no delay found - error\n",
	     addr, idx, exp_chips);
      return plErr_HW;
    }
  }
  /* ----- load chips */
  n_chips = exp_chips;
  bits = n_chips*VA32TA2_BITS; /* all available chips must be initialized */
  res=lvd_vertex_rw_vata(dev, addr, idx, bits, data, 0);
  if(!res) {
    info[idx].numchips = n_chips;
    info[idx].numchannels=info[idx].numchips*VA32TA2_CHAN;
    res|=lvd_i_w(dev, addr, vertex.lv.nr_chan, info[idx].numchannels-1);
    info[idx].loaded |= CHIP_LOADED;
    return plOK;
  }
  return (res) ? plErr_HW : plOK;
}

/*****************************************************************************/
/*
 * 'unit' is 1 for LV and 2 for HV, other - error!
 *
 * caller has to ensure that addr points to a ZEL_LVD_ADC_VERTEX module
 * address cannot be -1
 *
 * we use only 16 bit of the 32-bit data words. We will change it later!
 * 
 * OBSOLETE, use lvd_vertex_load_chips() instead...
 */
plerrcode
lvd_vertex_load_vata(struct lvd_dev* dev, int addr, int unit, int words,
        ems_u32* data)
{
  struct lvd_acard* acard = GetVertexCard(dev, addr);    
  return load_vata_chips(dev, acard, unit-1, words, data);
}
/*****************************************************************************/
/*
 * 'unit' is 1 for LV and 2 for HV
 * '*num' will be set to number of bits
 *
 * caller has to ensure that addr points to a ZEL_LVD_ADC_VERTEX module
 */
plerrcode
lvd_vertex_read_vata(struct lvd_dev* dev, int addr, int unit,
        ems_u32* data, int* num)
{
    struct lvd_acard* acard = GetVertexCard(dev, addr);
    struct vtx_info *info;
    int res=0;

    /* the two following checks are redundant */
    if(!((acard->mtype == ZEL_LVD_ADC_VERTEX) ||
	 (acard->mtype == ZEL_LVD_ADC_VERTEXUN))) {
      return plErr_BadModTyp;
    }
    info=(struct vtx_info*)acard->cardinfo+(unit-1);
    if (info->chiptype!=VTX_CHIP_VA32TA2) {
      printf("lvd_vertex_read_vata: illegal chiptype %d, error\n", info->chiptype);
      return plErr_HWTestFailed;
    }
    
    *num=info->numchips*VA32TA2_BITS;
    /* read data from VATA and fill with zero bits */
    res=lvd_vertex_rw_vata(dev, addr, unit-1, *num, 0, data);
    /* write data back */
    res|=lvd_vertex_rw_vata(dev, addr, unit-1, *num, data, 0);

    return res?plErr_System:plOK;
}
/*******************************************************************************
   set delay to read  correct vata chip setting 
 * if delay 0, it will be set to 15 by module itself!
 * RETURN:
 *   0  - OK, else - error
 * VertexADC[UN], but different reg_offs
********************************************************************************/
static int set_vata_chip_delay(struct lvd_dev* dev, struct lvd_acard* acard, int idx,
			       int delay) {
  int res = -1;
  ems_u32 org, d;
  int val, delay_org, nb;
  int addr = acard->addr;
  struct vtx_info* info = acard->cardinfo;
  int reg_offs = info[idx].spec.ser_csr;
  org = info[idx].numchannels; /* it is not true */

  lvd_i_r_(dev, addr, reg_offs, 2, &org);
  delay_org = org & 0xf000;
  delay = (delay << 12) & 0xf000;
  if(delay_org == delay) return 0; /*nothing to do */
  nb = org & 0xfff;
  val = delay | nb;
  lvd_i_w_(dev, addr, reg_offs, 2, val);
  lvd_i_r_(dev, addr, reg_offs, 2, &d);
  if(d == val) {
    info[idx].delay = (d >> 12) & 0xf;
    res = 0; /* OK */
  }
  return res;
}

/*******************************************************************************/
/* set number of bits to read vata chip setting correctly
 * 
 * RETURN:
 *   0  - OK, else - error
 * VertexADC 
 ********************************************************************************/
static int set_vata_chip_bits(struct lvd_dev* dev, struct lvd_acard* acard,
				  int idx, int bits) {
  ems_u32 org, d;
  ems_u32 val, rest, nb;
  int addr = acard->addr;
  struct vtx_info* info = acard->cardinfo;
  int reg_offs = info[idx].spec.ser_csr;
  lvd_i_r_(dev, addr, reg_offs, 2, &org);
  rest = org & 0xf000;
  nb = bits & 0x0fff;
  val = rest | nb;
  lvd_i_w_(dev, addr, reg_offs, 2, val);
  lvd_i_r_(dev, addr, reg_offs, 2, &d);
  return (d == val) ? 0 : -1;
}

/* Count number of bits in chain of VATA chips. 
 *   addr:   module address
 *   idx:    0=LV, 1=HV
 *   nwords: related to number of chips: nrwords = (n_chips*VA32TA2_BITS +15) / 16
 *
 * "out" - to write, "in" - read from front-end
 * Returns: 
 *    number of bits (> 0) by VertexADC, NOT by VertADCUN module 
 *    <= 0 - error.
 * formally, it may be up to 1023 channels (32 chips) in correspondence with
 * data format, but only 10 chips (320 channels) is probably works (never checked)
 *
*/
static int
lvd_vertex_count_vata_bits(struct lvd_dev* dev, int addr, int idx,
			      int nrwords) {
  int i, j;
  int words=0, wbits=0;

  ems_u32* in  = alloca(sizeof(ems_u32)*nrwords);
  ems_u32* out = alloca(sizeof(ems_u32)*nrwords);

  if(!in || !out) { /* for any chance */
    printf("lvd_vertex_count_vata_bits: alloca failed for nw=%1d, addr=0x%1x\n",
	   nrwords, addr);
    return -1;
  }
  /* fill the chain with 1-bits, ignore output */
  for (i=0; i<nrwords; i++)
    out[i]=0xffff;
  if (lvd_vertex_rw_vata(dev, addr, idx, nrwords*16, out, 0)<0) {
    printf("lvd_vertex_count_vata_bits(): addr0x%1x idx=%1d -failed write 0xffff, error\n",
	   addr, idx);
    return -1;
  }
  /* shift 0-bits into the chain */
  for (i=0; i<nrwords; i++)
    out[i]=0;
  if (lvd_vertex_rw_vata(dev, addr, idx, nrwords*16, out, in)<0) {
    printf("lvd_vertex_count_vata_bits(): addr0x%1x idx=%1d -failed write/read, error\n",
	   addr, idx);
    return -1;
  }
  /* find first word which contains a 1-bit */  
  for (i=0; i<nrwords; i++) {
    if (in[i]!=0xffff) {
      ems_u16 word=in[i];
      ems_u16 mask;
      /* sanity check */
      if ((~word&(~word+1))&0xffff) {
	printf("lvd_vertex_count_vata_bits: unexpected word at %d: 0x%04x, error\n",
	       i, word);
	return -1;
      }
      words=i+1;
      /* find first 0-bit in the word */
      for (mask=0x8000, j=16; j>0; mask>>=1, j--) {
	if (!(word&mask)) {
	  wbits=j;
	  break;
	}
      }
      break;
    }
  }
  i++;
  /* sanity check */
  for (; i<nrwords; i++) {
    if (in[i]!=0) {
      printf("lvd_vertex_count_vata_bits: unexpected word at %d: 0x%04x, error\n",
	     i, in[i]);
      return -1;
    }
  }
  return (words*16-wbits); /* correct delay in reg_cr must be set */
}
/*****************************************************************************
*/

static int find_vata_chip_delay(struct lvd_dev* dev, struct lvd_acard* acard,
				   int idx, int exp_bits) {
  int i;
  int d1=-1, d2 = -1; /* [d1,d2] - range of good delays, if neither is -1*/
  int nw;
  int res = -1;
  int err = 0;
  if(exp_bits % VA32TA2_BITS) {
    printf("find_vata_chip_delay(): addr=0x%1x idx=%1d, exp_bits=%1d - invalid\n",
	   acard->addr, idx, exp_bits);
    return -1;
  }
  nw =(exp_bits+15) / 16;
  int n_bits;
  for(i=1; i<16; i++) { /* delay 0  will set to 15!!! */
    if(set_vata_chip_delay(dev, acard, idx, i)) { /* error */
      err++;
      continue; /* have no idea what to do yet */
    }
    if((n_bits = lvd_vertex_count_vata_bits(dev, acard->addr, idx, nw)) > 0) {
      if(n_bits == exp_bits) {
	if(d1 < 0) {
	  d1 = i;
	}
	else {
	  d2 = i;
	}
      }
      else {
	if(d1 >= 0) break; 
      }
    }
    else {
      if(d1 >= 0) break;
    }
  }
  /* select the optimum delay in the middle of [d1,d2] range*/
  if(d1 < 0) {/* failed*/
    printf("find_vata_chip_delay():  addr=0x%1x idx=%1d failed\n",
	   acard->addr, idx);
    res = -1;
  }
  else {
    if(d2 < 0) {
      res = d1;
    }
    else {
      res = (d1+d2)/2 + ((d1+d2)%2); /* round to larger value */
    }
  }
  /* set optimum delay and correct number of bits, if found */
  if(res >= 0) {
    if(set_vata_chip_delay(dev, acard, idx, res)) {
      printf("find_vata_chip_delay():  addr=0x%1x idx=%1d, set delay=%1d - error\n",
	     acard->addr, idx, res);
      res =-1;
    }
    else {
      if(set_vata_chip_bits(dev, acard, idx, (exp_bits-1))) {
	printf("find_vata_chip_delay():  addr=0x%1x idx=%1d, set bits=%1d - error\n",
	       acard->addr, idx, (exp_bits-1));
	res = -1;
      }
    }
  }
  if(res < 0) {
    printf("find_vata_chip_delay():  addr=0x%1x idx=%1d failed\n",
	   acard->addr, idx);
  }
  else {
    printf("vata_chip_delay():  addr=0x%1x idx=%1d, [%1d,%1d] delay range, delay=%1d nchips=%1d\n",
	   acard->addr, idx, d1,d2,res, exp_bits/VA32TA2_BITS);
  }
  return res;
}



/*****************************************************************************/
/*
 * count VA32TA2 chips
 * addr: module address
 * idx:  0=LV, 1=HV
 * max:
 *   if (max==0) chips to be checked: defmaxchips
 *   if (max>0)  chips to be checked: max
 *   if (max<0)  16-bit words to be checked: max
 * returns:
 *   if (max>=0) number of chips or -1 in case of error
 *   if (max<0)  number of bits in chain
 */
static int
lvd_vertex_count_vata_chips(struct lvd_dev* dev, int addr, int idx,
        int max) {
  const int defmaxchips=10;
  int chips;
  int nrwords= (max < defmaxchips) ? (defmaxchips*VA32TA2_BITS+15)/16 : (max*VA32TA2_BITS+15)/16;
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  int bits = lvd_vertex_count_vata_bits(dev, acard->addr, idx, nrwords);
  
  chips=bits/VA32TA2_BITS;
  if(bits < 0) {
    printf("vertex_count_vata_chips: unexpected bit count %d(n_chip=%1d), must be %d*n_chip, error\n", 
	   bits, chips, VA32TA2_BITS);
    return -1;
  }
  if (bits%VA32TA2_BITS) {
    printf("vertex_count_vata_chips: unexpected bit count %d(n_chip=%1d), must be %d*n_chip, error\n", 
	   bits, chips, VA32TA2_BITS);
    return -1;
  }
  return chips;
}
/****************************************************************************
 * Here: unit: 0=LV, 1=HV
*/
int lvd_vertex_count_chips(struct lvd_dev* dev, int addr, int idx, int max) {
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  struct vtx_info* info = (acard) ? acard->cardinfo : 0;
  int nchips = -1;
  if(!info) {
    printf("lvd_vertex_count_chips: acard=%1p, info=%1p - error\n", acard, info);
    return -1;
  }
  switch (acard->mtype) {
    case ZEL_LVD_ADC_VERTEX:
      nchips = lvd_vertex_count_vata_chips(dev, addr, idx, max);
      break;
    case ZEL_LVD_ADC_VERTEXM3:
      init_mate3_addresses(dev, addr, idx, &nchips);
      break;
    case ZEL_LVD_ADC_VERTEXUN:
      if(info[idx].chiptype == VTX_CHIP_NONE) {
	nchips = 0;
      }
      else if(info[idx].chiptype == VTX_CHIP_VA32TA2) {
	nchips = lvd_vertex_count_vata_chips(dev, addr, idx, max);
      } else if( info[idx].chiptype == VTX_CHIP_MATE3) {
	init_mate3_addresses(dev, addr, idx, &nchips);
      }
      else {
	printf("lvd_vertex_count_chips:addr=0x%1x idx=%1d type=%1d - invalid chip type, error\n",
	       addr, idx, info[idx].chiptype);
      }
      break;
    default:
      printf("programm error in lvd_vertex_count_chips: wrong card type\n");
      break;
  }
  return nchips;
}
/*****************************************************************************/
/*
 * chipargs[0]: chiptype LV
 *         [1]: chiptype HV
 *         [2]: number of chips LV
 *         [3]: number of chips HV
 *         [4]: number of channels LV
 *         [5]: number of channels HV
 */
plerrcode
lvd_vertex_chip_info(struct lvd_dev* dev, int addr, ems_u32* chipargs) {
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  struct vtx_info* info;
  if (!acard) {
    printf("vertex_chip_info: no VERTEX card with address 0x%x known, error\n",
	   addr);
    return plErr_Program;
  }
  info=(struct vtx_info*)acard->cardinfo;

  *chipargs++=info[0].chiptype;
  *chipargs++=info[1].chiptype;
  *chipargs++=info[0].numchips;
  *chipargs++=info[1].numchips;
  *chipargs++=info[0].numchannels;
  *chipargs++=info[1].numchannels;

  return plOK;
}

/*****************************************************************************/
static void free_seq_par_va(void* par) {
  if(par) { /*we set members to 0 for any chance */
    struct va_seq_par* seq_par = par;
    free(seq_par->ro_par);
    seq_par->ro_par = 0;
    seq_par->ro_len = 0;
    free(seq_par->sel_funcs);
    seq_par->sel_funcs = 0;
    seq_par->sel_len = 0;
    free(seq_par->va_funcs);
    seq_par->va_funcs = 0;
    seq_par->va_len = 0;
    seq_par->prog = 0; /* was not allocated! */
    seq_par->prog_len = 0;
    free(seq_par);
  }
}

/* here not everything may be set, as proper sequencer code is not yet loaded! */
static void set_vtx_va_func(struct vtx_info* info) {
  info->prep_ro     = prep_ro_va32;
  info->set_ro_par  = set_ro_par_va;     /* must be set here? */
  info->ro_reset    = ro_reset_va;
  info->m_reset     = mreset_va;
  info->sel_tp_chan = sel_vata_chan_tp;
  info->analog_tp   = analog_tp_va;
  info->load_chips  = load_vata_chips;
  info->set_scaling = set_scaling_vata_va;
}

static void set_vtx_va_func_un(struct vtx_info* info) {
  info->prep_ro     = prep_ro_va32;
  info->free_par    = free_seq_par_va;   /* must be set here */
  info->set_ro_par  = set_ro_par_va;     /* must be set here? */ 
  info->ro_reset    = ro_reset_va;
  info->m_reset     = mreset_va;
  info->sel_tp_chan = sel_vata_chan_tp;
  info->analog_tp   = analog_tp_va;
  info->load_chips  = load_vata_chips;
  info->set_scaling = set_scaling_vata_un;
}

static void  set_vtx_m3_func(struct vtx_info* info) {
  info->prep_ro     = prep_ro_m3;
  info->free_par    = free_seq_par_m3;
  info->set_ro_par  = set_ro_par_m3;
  info->ro_reset    = ro_reset_m3;
  info->m_reset     = ro_reset_dummy;  /* always for MATE3 chips, nothing to do */
  info->sel_tp_chan = sel_mate3_chan_tp;
  info->analog_tp   = analog_tp_dummy; /* no chance to get analog output for TP */
  info->load_chips  = load_mate3_chips;
  info->set_scaling = set_scaling_mate3_m3;
}

static void  set_vtx_m3_func_un(struct vtx_info* info) {
  info->prep_ro     = prep_ro_m3;
  info->free_par    = free_seq_par_m3;
  info->set_ro_par  = set_ro_par_m3;
  info->ro_reset    = ro_reset_m3;
  info->m_reset     = ro_reset_dummy; /* always for MATE3 chips, nothing to do*/
  info->sel_tp_chan = sel_mate3_chan_tp;
  info->analog_tp   = analog_tp_dummy; /* no chance to get analog output for TP */
  info->load_chips  = load_mate3_chips; /* internal functions differs from VertexADCM3! */
  info->set_scaling = set_scaling_mate3_un;
}

static void  set_vtx_func_dummy(struct vtx_info* info) {
  info->prep_ro     = prep_ro_dummy;
  info->free_par    = 0;
  info->set_ro_par  = 0;
  info->ro_reset    = ro_reset_dummy;
  info->m_reset     = ro_reset_dummy; /* always for MATE3 chips, nothing to do*/
  info->sel_tp_chan = sel_tp_chan_dummy;
  info->analog_tp   = analog_tp_dummy; /* no chance to get analog output for TP */
  info->load_chips  = 0; /* internal functions differs from VertexADCM3! */
  info->set_scaling = 0;
}

/*****************************************************************************/
static void
lvd_vertex_acard_free(struct lvd_dev* dev, struct lvd_acard* acard)
{
  if(acard->cardinfo) {
    vtx_info_free(acard->cardinfo);
  }
  acard->cardinfo=0;
}

/****************************************************************************
 * Arguments of lvd_vertex_acard_init determined by 
 * "struct cardinitfunc" of lvdbus.c!
 * this function is called by lvdbus_init() and lvd_force_module_id()
 * At this moment we know nothing which front-end card connected...
 * Potential memory leak, if acard was not cleared by acard->freeinfo(...)
 * before...
*/
int
lvd_vertex_acard_init(struct lvd_dev* dev, struct lvd_acard* acard)
{
  int res = 0;
  if((acard->mtype == ZEL_LVD_ADC_VERTEX)   || /* for any chance */
     (acard->mtype == ZEL_LVD_ADC_VERTEXM3) ||
     (acard->mtype == ZEL_LVD_ADC_VERTEXUN)) {
    acard->freeinfo   = lvd_vertex_acard_free; /* free vtx_info of module */
    acard->clear_card = lvd_clear_vertex;      /* ?? */
    acard->start_card = lvd_start_vertex;      /* prepare read-out and start sequencers (module) */
    acard->stop_card  = lvd_stop_vertex;       /* un-initialize module, load sequencers by idle loop? */
    acard->cardstat   = lvd_cardstat_vertex;
    acard->daqmode    =  0;
    acard->initialized = 0; /* for any chance */
    acard->cardinfo    = vtx_info_alloc(acard->mtype);
    if(!acard->cardinfo) {
      printf("lvd_vertex_acard_init(): failed allocate cardinfo addr=0x%1x, error\n", acard->addr);
    }
  }
  else { /* for any chance */
    printf("programm error in lvd_vertex_acard_init: invalid card type: 0x%1x\n", acard->mtype);
    res = -1; 
  }
  return res;
}
/*****************************************************************************
 * write user data word
 */
plerrcode
lvd_vertex_write_uw(struct lvd_dev* dev, int addr, int unit, ems_u32 value) {
  if (addr<0) {
    int card;
    plerrcode last_err = plOK;
    for (card=0; card<dev->num_acards; card++) {
      if (IsVertexCard(dev, card)) {
	plerrcode pres = lvd_vertex_write_uw(dev, dev->acards[card].addr, 
					     unit, value);
	if (pres!=plOK) {
	  last_err = pres;
	}
      }
    }
    return last_err;
  } else {
    int i;
    int max_pass = 10;
    int res = 0;
    struct lvd_acard* acard = GetVertexCard(dev, addr);
    if (!acard) {
      printf("lvd_vertex_write_uw: no VERTEX card with address 0x%x known, error\n",
	     addr);
      return plErr_Program;
    }
    
    ems_u16 h =(value>>16) & 0xff;
    ems_u16 l = value & 0xffff;

    for(i=0; i<max_pass; i++) { /* protection loop */
      res = 0;
      if(acard->mtype == ZEL_LVD_ADC_VERTEX) {
	if(unit&1) {
	  res|=lvd_i_w(dev, addr, vertex.lv.v.user_data, h);
	  res|=lvd_i_w(dev, addr, vertex.lv.v.user_data, l);
	}
	if(unit&2) {
	  res|=lvd_i_w(dev, addr, vertex.hv.v.user_data, h);
	  res|=lvd_i_w(dev, addr, vertex.hv.v.user_data, l);
	}
      }
      else {
	if(unit&1) {
	  res|=lvd_i_w(dev, addr, vertex.lv.m.user_data, h);
	  res|=lvd_i_w(dev, addr, vertex.lv.m.user_data, l);
	}
	if(unit&2) {
	  res|=lvd_i_w(dev, addr, vertex.hv.m.user_data, h);
	  res|=lvd_i_w(dev, addr, vertex.hv.m.user_data, l);
	}
      }
      if(!res) break;
    }
    return (res) ? plErr_HW : plOK;
  }
}


/*****************************************************************************
 * set vtx_info.spec.ro_par[]
 * must be: par[VTX_INFO_RO_PAR_NUM]!
 * len    : number of parameters, must be <= VTX_INFO_RO_PAR_NUM
 * par[0] : switch reg value to start readout
 * par[1] : aclk_par, must correspond to par[0], see manual
 * par[2] : pause between channel 0 and 1 in each chip
 * par[3] : number of read-out clocks, for MATE3 only
 */
plerrcode
lvd_vertex_info_ro_par_set(struct lvd_dev* dev, int addr, int unit, 
			   ems_u32 len, ems_u32* par) {
  if(!par)
    return plErr_ArgNum;
  if (addr<0) {
    int card;
    plerrcode last_err = plOK;
    for (card=0; card<dev->num_acards; card++) {
      if (IsVertexCard(dev, card)) {
	plerrcode pres = lvd_vertex_info_ro_par_set(dev, dev->acards[card].addr, 
					     unit, len, par);
	if (pres!=plOK) {
	  last_err = pres;
	}
      }
    }
    return last_err;
  } else {
    struct vtx_info* info;
    struct lvd_acard* acard = GetVertexCard(dev, addr);
    if (!acard) {
      printf("lvd_vertex_info_ro_par_set: no VERTEX card with address 0x%x known, error\n",
	     addr);
      return plErr_Program;
    }
    info=(struct vtx_info*)acard->cardinfo;
    if(unit&1) {
      info[0].set_ro_par(&info[0], len, par);
    }
    if(unit&2) {
      info[1].set_ro_par(&info[1], len, par);
    }
    return plOK;
  }
}

/*****************************************************************************
 * Prepare sequencer to start readout.
 * If par == 0, use vtx_info[idx].ro_par[],
 * else - set vtx_info[idx].ro_par[]. Then prepare sequencer to start read-out
 * len    : number of parameters, must be <= VTX_INFO_RO_PAR_NUM
 * par[0] : switch reg value to start readout
 * par[1] : aclk_par, must correspond to par[0], see manual
 * par[2] : pause between channel 0 and 1 in each chip
 * par[3] :  number of read-out clocks, for MATE3 only
 */
plerrcode
lvd_vertex_seq_prep_ro(struct lvd_dev* dev, int addr, int unit, 
		       ems_u32 len, ems_u32* par) {
  if (addr<0) {
    int card;
    plerrcode last_err = plOK;
    for (card=0; card<dev->num_acards; card++) {
      if (IsVertexCard(dev, card) && dev->acards[card].initialized) {
	plerrcode pres = lvd_vertex_seq_prep_ro(dev, dev->acards[card].addr, 
						unit, len, par);
	if (pres!=plOK) {
	  last_err = pres;
	}
      }
    }
    return last_err;
  } 
  else {
    int res = 0;
    struct lvd_acard* acard = GetVertexCard(dev, addr);
    if (!acard) {
      printf("lvd_vertex_seq_prep_ro: no VERTEX card with address 0x%x known, error\n",
	     addr);
      return plErr_Program;
    }
    if(acard->initialized == 0) {
      return 0;
    }
    int crun = is_vertex_running(dev, acard);
    if(crun == 0) { /* start in init mode, for any chance */
      plerrcode pres = start_vertex_init_mode(dev, acard);
      if(pres != plOK) {
	printf("lvd_vertex_seq_prep_ro: error in start_vertex_init_mode(), addr=0x%1x\n",
	       acard->addr);
      }
      return pres;
    }
    struct vtx_info* info = acard->cardinfo;

    if((len !=0) && par) {             /* initialize vtx_info[i].ro_par */
      if(unit & 1) {
	info[0].set_ro_par(&info[0], len, par);	
      }
      if(unit & 2) {
	info[1].set_ro_par(&info[1], len, par);	
      }
    }
    if(unit & 1) {
      res |= info[0].prep_ro(dev, acard, 0);	
    }
    if(unit & 2) {
      res |= info[1].prep_ro(dev, acard, 1);
    }
    return (res) ? plErr_InvalidUse : plOK;
  } 
}

static plerrcode analog_tp_va(struct lvd_dev* dev, struct lvd_acard* acard, 
			      int idx, ems_u32 ch) {
  plerrcode pres = plOK;
  struct vtx_info* info = acard->cardinfo;
  struct vtx_info* half = &info[idx];
  if(half->chiptype != VTX_CHIP_VA32TA2 || (half->numchips <= 0)) return plOK; /* for any chance */
  if(half->seq_par == 0) {
    printf("analog_tp_va: addr=0x%1x idx=%1d: seq. params not set!\n",
	   acard->addr, idx);
    return plErr_InvalidUse;
  }
  struct va_seq_par* seq_par = half->seq_par;
  /* Master Reset, for chips to deselect and clean all chips */
  if(seq_par->m_reset) {    
    if((pres = exec_seq_func(dev, acard, idx, seq_par->m_reset, 1)) != plOK) {
      printf("analog_tp_va: addr=0x%1x idx=%1d: master reset call failed!\n",
	     acard->addr, idx);
    }
  }
  else {
    pres = plErr_InvalidUse;
    printf("analog_tp_va: addr=0x%1x idx=%1d: seq_par->m_reset not set!\n",
	   acard->addr, idx);
  }
  if(pres != plOK || ((int)ch < 0)) return pres; /* error, or no channel selection required */
  /* try select channel */
  if(seq_par->sel_funcs == 0) { /* is selection functions set? */
    printf("analog_tp_va: addr=0x%1x idx=%1d: seq_par->sel_funcs not set!\n",
	   acard->addr, idx);
    pres = plErr_InvalidUse;
  }
  else {
    /*select chan 0 in chip 0 */
    if((pres = exec_seq_func(dev, acard, idx, seq_par->sel_funcs[0], 0)) != plOK) {
      printf("analog_tp_va: addr=0x%1x idx=%1d: failed select ch: 0!\n",
	     acard->addr, idx);      
    }
    else {
      if((int)ch > 0) {
	if(idx == 0) {
	  if(lvd_i_w(dev, acard->addr, vertex.lv.lp_counter[0], ch-1) < 0) pres = plErr_HW;
	}
	else {
	  if(lvd_i_w(dev, acard->addr, vertex.hv.lp_counter[0], ch-1) < 0) pres = plErr_HW;
	}
	if(pres != plOK) {
	  printf("analog_tp_va: addr=0x%1x idx=%1d: failed load counter 0!\n",
	     acard->addr, idx);
	}
	if((pres = exec_seq_func(dev, acard, idx, seq_par->va_funcs[1], 0)) != plOK) {
	  printf("analog_tp_va: addr=0x%1x idx=%1d: failed exec va clks!\n",
		 acard->addr, idx);
	}
      }
    }
  }
  /* NO readout reset with this function! */
  return pres;
}

/*****************************************************************************
 * Select channel[s] in vertex vata32 chips for TestPulse analog output
 */
static plerrcode
vertex_analog_tp(struct lvd_dev* dev, struct lvd_acard* acard, int idx, ems_u32 val) {
  struct vtx_info* info=(acard) ? acard->cardinfo : 0;
  if(!info) {
    return plErr_BadModTyp;
  }
  if(idx<0 || idx>1) return plErr_InvalidUse;
  return info[idx].analog_tp(dev, acard, idx, val);
}


/*****************************************************************************
 * Select channel[s] in vertex vata32 chips for TestPulse calibration
 * idx:    0 - LV, 1 - HV, other - error!
 * par[0] - channel to be selected, if (-1) - deselect channels 
 * par[1] - if 1, channel=par[0] is selected in all chips
 */

static plerrcode sel_vata_chan_tp(struct lvd_dev* dev, struct lvd_acard* acard, 
				       int idx, ems_u32* par) {
  int i;
  int max_pass=10;
  int res = plOK;
  struct vtx_info* info=(acard) ? acard->cardinfo : 0;
  if( !info || !info[idx].loaded || (acard->initialized == 0)) {
    return plOK; /* nothing to do for uninitialized sequencer */
  }
  int addr = acard->addr;
  int nchips = info[idx].numchips;
  if(nchips <= 0) {
    printf("sel_vata_chan_tp: nchips <= 0, addr=0x%1x idx=%1d, error\n",
	   addr, idx);
    return plErr_InvalidUse;;
  }
  /* module specifics, always available here? */
  ems_u32 lp_count0 = info[idx].spec.lp_count[0];
  /* chip type specifics */
  struct va_seq_par* seq_par = (struct va_seq_par*)info[idx].seq_par;
  if(seq_par == 0) {  /* for any chance */
    printf("sel_vata_chan_tp: seq_par == 0 addr=0x%1x idx=%1d, error\n",
	   addr, idx);
    return plErr_InvalidUse;;
  }
  ems_u32  sel_len  = seq_par->sel_len;
  ems_u32* sel_tp   = seq_par->sel_funcs;
  ems_u32* vaclk_tp = seq_par->va_funcs;   /* VA clk. tp params, first - number */
  if((sel_len) < 3 || (sel_tp==0)) { /* for any chance */
    printf("sel_vata_chan_tp: unknown seq.funcs to select TP at addr=0x%1x idx=%1d error\n",
	   addr, idx);
    return plErr_InvalidUse;
  }
  
  /* Master Reset for chips */  
  if(info[idx].m_reset) {
    if((res=info[idx].m_reset(dev, acard, idx)) != plOK) {  /* protection loop inside */
      printf("sel_vata_chan_tp: addr=0x%1x idx=%1d m_reset error\n",
	     addr, idx);
      return res;
    }
  }
  else {
    printf("sel_vata_chan_tp: addr=0x%1x idx=%1d, (info[idx].m_reset()=0, error\n",
	   addr, idx);
    return plErr_InvalidUse;
  }
  /* is it deselect all channels only? */
  if((int)(par[0]) < 0) { /* yes, no selection */ 
    return plOK;
  }
  if(par[1]) { /* in all available chips */
    /* load number of chips */
    for(i=0; i<max_pass; i++) {  /* protection loop */
      if((res=lvd_i_w_(dev, addr, lp_count0, 2, nchips - 1)) == 0) break;
    }
    if(res) {
      printf("*** sel_vata_chan_tp: addr=0x%1x idx=%1d: error write %1d chips to lp_count0 at 0x%1x", 
	     addr, idx, (nchips - 1), lp_count0);
      return plErr_HW;
    }
    res = exec_seq_func(dev, acard, idx, sel_tp[1], 1);  /* select chan 0 in all chips, protection loop inside */
    if(res) {
      print_csr_regs(dev, addr, idx, "sel_vata_chan_tp error sel 0 ch in all");
      return plErr_Other;
    }
    if(par[0] > 0) {                                 /* if channel to be selected > 0 */
      for(i=0; i<max_pass; i++) { /* protection loop  */
	if((res=lvd_i_w_(dev, addr, lp_count0, 2,  par[0] - 1)) == 0) break;  
      }
      if(res) {
	printf("*** sel_vata_chan_tp: addr=0x%1x idx=%1d: error write %1d chan. to lp_count0 at 0x%1x", 
	       addr, idx, (par[0] - 1), lp_count0);
	return plErr_HW;
      }
      res = exec_seq_func(dev, acard, idx, vaclk_tp[1], 0);  /* VA clocks,  NO protection loop inside! */
      if(res) {
	print_csr_regs(dev, addr, idx, "sel_vata_chan_tp: error in VA clock func");
	printf("sel_vata_chan_tp: addr=0x%1x idx=%1d switch=0x%1x - failed exec\n", 
	       addr, idx,  vaclk_tp[1]);
	return plErr_Other;
      }
    }
    if(res != plOK) {
      printf("sel_vata_chan_tp: addr=0x%1x, idx=%1d, error sel chan %1d in all chips\n",
	     addr, idx, par[0]);
      return plErr_InvalidUse;
    }
  }
  else { /* only one chan in board, par[1] == 0 */
    if(par[0] == 0) { /*select chan 0 in chip 0 */
      res |= exec_seq_func(dev, acard, idx, sel_tp[0], 1); /* protection loop inside */
      if(res) {
	printf("sel_vata_chan_tp: addr=0x%1x, idx=%1d, error sel chan 0 chip 0\n",
	       addr, idx);
	return plErr_InvalidUse;
      }
    }
    else {
      int r=0;
      for(i=0; i<max_pass; i++) {
	if((r=lvd_i_w_(dev, addr, lp_count0, 2, par[0])) == 0) break;
      }
      res |= r;
      res |= exec_seq_func(dev, acard, idx, sel_tp[2], 0);
    }
    if(res) {
      printf("sel_vata_chan_tp: addr=0x%1x, idx=%1d, error sel chan %1d in board\n",
	     addr, idx, par[0]);
      return plErr_Other;
    }
  }
  /* readout reset */
  res |= exec_seq_func(dev, acard, idx, seq_par->ro_reset, 1); /* protection loop inside */
  if(res) {
    printf("sel_vata_chan_tp: addr=0x%1x, idx=%1d, error in RO reset\n",
	   addr, idx);
    return plErr_InvalidUse;
  }
  return plOK;
}

/*****************************************************************************
 * Select channel[s] in vertex front-end card for TestPulse calibration
 * idx:    0 - LV, 1 - HV, other - error!
 */
static plerrcode vertex_sel_chan_tp(struct lvd_dev* dev, struct lvd_acard* acard, int idx, ems_u32* par) {
  if(acard->initialized == 0) {
    return 0;
  }
  plerrcode res = plOK;
  struct vtx_info* info=(struct vtx_info*)acard->cardinfo;
  if(info[idx].chiptype == VTX_CHIP_NONE) {
    printf("vertex_sel_chan_tp: addr=0x%1x id=%1d - no chips defined, ignore\n",
	     acard->addr, idx);
  }
  else {
    res = info[idx].sel_tp_chan(dev, acard, idx, par);
    if(res != plOK) {
      printf("vertex_sel_chan_tp: at addr=0x%1x idx=%1d - error\n", acard->addr, idx);
    }
  }
  return res;
}

/*****************************************************************************
 * Select channel[s]  chips for TestPulse calibration
 * 
 */
plerrcode
lvd_vertex_sel_chan_tp(struct lvd_dev* dev, int addr, int unit, ems_u32* par) {
  if (addr<0) {
    int card;
    plerrcode last_err = plOK;
    for (card=0; card<dev->num_acards; card++) {
      if (IsVertexCard(dev, card)) {
	plerrcode pres = lvd_vertex_sel_chan_tp(dev, dev->acards[card].addr, 
						unit, par);
	if (pres!=plOK) {
	  last_err = pres;
	}
      }
    }
    return last_err;
  } else {
    plerrcode pres;
    plerrcode last_err = plOK;
    struct lvd_acard* acard = GetVertexCard(dev, addr);
    if (!acard) {
      printf("lvd_vertex_sel_chan_tp: no VERTEX card with address 0x%x known, error\n",
	     addr);
      return plErr_Program;
    }
    if(acard->initialized == 0) {
      return 0;
    }

    int idx = 0;
    if(unit & 1) { /* may be different types of chips, so never both! */
      pres = vertex_sel_chan_tp(dev, acard, idx, par);
      if(pres != plOK) last_err = pres;
    }
    if(unit & 2) { /* may be different types of chips, so never both! */
      idx = 1;
      pres = vertex_sel_chan_tp(dev, acard, idx, par);
      if(pres != plOK) last_err = pres;
    }
    return last_err;
  }
}

/* master reset for VA chips */
static plerrcode mreset_va(struct lvd_dev* dev, struct lvd_acard* acard, int idx) {
  struct vtx_info* info = (struct vtx_info*)acard->cardinfo;
  plerrcode pres;  
  if(info[idx].m_reset) {
    struct va_seq_par* seq_par = info[idx].seq_par;
    pres = exec_seq_func(dev, acard, idx, seq_par->m_reset, 1);  /* protection loop inside */
  }
  else {
    pres = OK;
    printf("mreset_va: addr=0x%1x idx=%1d - ignore master reset, warning\n",
	   acard->addr, idx);
  }
  return pres;
}

/*********************************************************************
 * Function to make master reset for front-end electronics
 * ONLY for VA32TA2 chips, for MATE3 - do nothing, as MRESET for MATE3
 * clear chip settings!
 */
plerrcode lvd_vertex_mreset(struct lvd_dev* dev, int addr, int unit) {
  plerrcode last_err = plOK;
  plerrcode pres = plOK;
  if (addr<0) {
    int card;
    plerrcode pres;
    for (card=0; card < dev->num_acards; card++) {
      struct lvd_acard* acard=dev->acards+card;
      if (IsVertexCard(dev, card)) {
	pres = lvd_vertex_mreset(dev, acard->addr, unit);
	if(pres != plOK) {
	  last_err = pres;
	  return last_err;
	}
      }
    }
  } else {
    struct lvd_acard* acard=GetVertexCard(dev, addr);
    if (!acard) {
      printf("lvd_vertex_mreset: no VERTEX card with addr=0x%1x known, error\n",
                addr);
      return plErr_Program;
    }
    struct vtx_info* info = acard->cardinfo;
    int idx = 0;
    if((unit & 1) && (info[idx].m_reset)) {
      pres = info[idx].m_reset(dev, acard, idx);
      if(pres != plOK) last_err = pres;
    }
    idx = 1;
    if((unit & 2) && (info[idx].m_reset)) {
      pres = info[idx].m_reset(dev, acard, idx);
      if(pres != plOK) last_err = pres;
    }
  }
  return last_err;
}

/* makes read-out reset for VATA chips, it must be always possible to execute */
static plerrcode ro_reset_va(struct lvd_dev* dev, struct lvd_acard* acard, int idx)  {
  struct vtx_info*  info  = acard->cardinfo;
  plerrcode pres;
  struct va_seq_par* seq_par = info[idx].seq_par;
  if(seq_par) {
    pres = exec_seq_func(dev, acard, idx, seq_par->ro_reset, 1);
  }
  else {
    pres = plErr_InvalidUse;
    printf("ro_reset_va(): failed at addr=0x%1x idx=%1d\n", acard->addr, idx);
  }
  return pres;
}

static plerrcode ro_reset_m3(struct lvd_dev* dev, struct lvd_acard* acard, int idx)  {
  struct vtx_info*  info  = acard->cardinfo;
  plerrcode pres;
  struct m3_seq_par* seq_par = info[idx].seq_par;
  if(seq_par) {
    pres = exec_seq_func(dev, acard, idx, seq_par->ro_reset, 1);
  }
  else {
    pres = plErr_InvalidUse;
    printf("ro_reset_m3(): failed at addr=0x%1x idx=%1d\n", acard->addr, idx);
  }
  return pres;
}

/*********************************************************************
 * Function to make read-out reset for front-end electronics
 */
plerrcode lvd_vertex_ro_reset(struct lvd_dev* dev, int addr, int unit) {
  if (addr<0) {
    plerrcode last_err = plOK;
    plerrcode pres;
    int card;
    for (card=0; card < dev->num_acards; card++) {
      struct lvd_acard* acard=dev->acards+card;
      if (IsVertexCard(dev, card)) {
	pres = lvd_vertex_ro_reset(dev, acard->addr, unit);
	if(pres != plOK) {
	  last_err = pres;
	}
      }
    }
    return last_err;
  } 
  else {
    plerrcode last_err = plOK;
    plerrcode pres;
    struct lvd_acard* acard=GetVertexCard(dev, addr);
    if(acard) {
      struct vtx_info*  info  = acard->cardinfo;
      int idx = 0;
      if((unit & 1)) {
	pres = info[idx].ro_reset(dev, acard, idx);
	if(pres != plOK) last_err = pres;
      } 
      idx = 1;
      if((unit & 2)) {
	pres = info[idx].ro_reset(dev, acard, idx);
	if(pres != plOK) last_err = pres;
      } 
    }
    else {
      printf("lvd_vertex_ro_reset(): at addr=0x%1x is not any VertexADC, error\n",
	     acard->addr);
      last_err = plErr_InvalidUse;
    }
    return last_err;
  }
}

/*****************************************************************************
 * Select channel[s]  chips for TestPulse analog 
 * 
 */
plerrcode
lvd_vertex_analog_chan_tp(struct lvd_dev* dev, int addr, int unit, ems_u32 val) {
  if (addr<0) {
    int card;
    plerrcode last_err = plOK;
    for (card=0; card<dev->num_acards; card++) {
      if (dev->acards[card].mtype==ZEL_LVD_ADC_VERTEX) { /* only for VATA chips */
	plerrcode pres = lvd_vertex_analog_chan_tp(dev, dev->acards[card].addr, 
						unit, val);
	if (pres!=plOK) {
	  last_err = pres;
	}
      }
    }
    return last_err;
  } else {
    struct lvd_acard* acard=0;
    plerrcode pres;
    int idx=lvd_find_acard(dev, addr);
    if(idx < 0) {
      printf("lvd_vertex_analog_chan_tp: no VERTEX card with address 0x%x known,error\n",
                addr);
      return plErr_Program;
    }
    acard=dev->acards+idx;
    if(acard->mtype!=ZEL_LVD_ADC_VERTEX) {
      printf("lvd_vertex_analog_chan_tp: no  VERTEX card at address 0x%x, error\n",
	     addr);
      return plErr_Program;
    }
    if(unit&1) { /* may be different types of chips, so never both! */
      pres = vertex_analog_tp(dev, acard, 0, val);
      if(pres != plOK) {
	printf("lvd_vertex_analog_chan_tp addr=0x%1x unit=1 - error\n",
	       acard->addr);
      }
    }
    if(unit&2) {
      pres = vertex_analog_tp(dev, acard, 1, val);
      if(pres != plOK) {
	printf("lvd_vertex_analog_chan_tp: addr=0x%1x unit=2 - error\n",
	       acard->addr);
      }
    }
  }
  return plOK;
}

/***********************************************************************
 *============================= MATE3 functions =======================
 */

/****************************************************************************
 */
struct lvd_acard* GetVertexMate3Card(struct lvd_dev* dev, int addr) {
  int idx=lvd_find_acard(dev, addr);
  struct lvd_acard* acard=(idx < 0) ? 0 : dev->acards+idx;
  return (acard && (acard->mtype==ZEL_LVD_ADC_VERTEXM3)) ? acard : 0; 
}

/****************************************************************************
 */
static plerrcode read_i2c_csr(struct lvd_dev* dev, struct lvd_acard* acard, 
			      int idx, ems_u32* val) {
  int i;
  int addr = acard->addr;
  struct vtx_info* info = acard->cardinfo;
  int max_pass = 10;
  int res = 0;
  for(i=0; i<max_pass; i++) { /* protection loop */
    if((res  = lvd_i_r_(dev, addr, info[idx].spec.ser_csr, 2, val)) == 0) break;
  }
  if(res) {
      printf("read_i2c_csr: at addr=0x%1x max_pass=%1d, lvd_i_r_() error\n",
	     addr, max_pass);
  }
  return res ? plErr_HW : plOK;
}

/****************************************************************************
 * RETURNS: plOK       - if bit I2C_BUSY is 0. 
 *          plErr_Busy - if bitI2C_BUSY is 1.
 *          plErr_HW   - in case of lvd_i_r_() error
 *
*/
static plerrcode  is_i2c_ready(struct lvd_dev* dev, struct lvd_acard* acard, int idx, ems_u32* val) {
  if(read_i2c_csr(dev, acard, idx, val) != plOK) return plErr_HW;  /* protection loop inside */
  return (*val & VTX_I2R_BSY_MRST) ?  plErr_Busy : plOK;
}

/****************************************************************************
 * Check BUSY bit only
 * RETURNS: plOK | plErr_HW | plErr_Timeout
 */
static plerrcode wait_i2c(struct lvd_dev* dev, struct lvd_acard* acard, int idx, ems_u32 *val) {
  int maxwait = 100;
  int max_wpass = 100;
  plerrcode pres;
  DEFINE_LVDTRACE(trace);
  lvd_settrace(dev, &trace, 0);
  int i;
  for(i=0; i<max_wpass; i++) {
    pres =  read_i2c_csr(dev, acard, idx, val); /* protection loop inside */
    if((pres != plOK) || (*val & VTX_I2R_ERR_RST)) {
      clear_i2c_err(dev, acard, idx); /* protection loop inside */
      return plErr_HW;
    }
    if(!(*val & VTX_I2R_BSY_MRST)) { /* ready */
      return pres;
    }
    u_delay(maxwait);
  }
  lvd_settrace(dev, 0, trace);
  printf("wait_i2c: timeout expired\n");
  return plErr_Timeout;
}

/****************************************************************************
 * Check I2C_IN_DATA and I2C_ERROR bits 
 * RETURNS: plOK | plErr_HW | plErr_Timeout
 *          plOK - there is availbale data in FIFO and no error
 */
static plerrcode wait_i2c_data(struct lvd_dev* dev, struct lvd_acard* acard, int idx, ems_u32 *val) {
  int maxwait = 100; /* micro sec */
  int max_wpass = 100; /* total wait time = maxwait*max_wpass*1e-6 sec */
  plerrcode pres;
  DEFINE_LVDTRACE(trace);
  *val = -1;
  int i;
  for (i=0; i<max_wpass; i++) {
    pres =  read_i2c_csr(dev, acard, idx, val); /* protection loop inside */
    if((pres != plOK) || (*val & VTX_I2R_ERR_RST)) {
      clear_i2c_err(dev, acard, idx); /* protection loop inside */
      return plErr_HW;
    }
    if(*val & VTX_I2R_BSY_MRST) {
	u_delay(maxwait);
    }
    if(*val & VTX_I2R_DAV_IFRST) {
      return plOK;
    }
  }
  lvd_settrace(dev, 0, trace);
  printf("wait_i2c_data: timeout expired\n");
  return plErr_Timeout;
}

/*************************************************************************
 * Function to clear error bit, State Machine and FIFO  of VERTEXM3 module.
 **************************************************************************/
static plerrcode clear_i2c_err(struct lvd_dev* dev, struct lvd_acard* acard, int idx) {
  int i;
  int addr = acard->addr;
  int res;
  plerrcode pres = plOK;
  struct vtx_info* info = acard->cardinfo;  
  int max_pass = 10;
  for(i=0; i<max_pass; i++) {
    if((res = lvd_i_w_(dev, addr, info[idx].spec.ser_csr, 2, VTX_I2R_BSY_MRST)) == 0) break;
  }
  if(res < 0) {
     printf("clear_i2c_err: lvd_i_w_() at addr=0x%1x - error\n",
	   addr);
    return plErr_HW;
  }

#if 0
  int ddma =  is_ddma_active(dev);
  if((res < 0) && ddma) { /* not good solution... temporary */
    int max_count = 10;
    do {
      res = lvd_i_w_(dev, addr, info[idx].spec.ser_csr, 2, VTX_I2R_BSY_MRST);
      if(res >= 0) break;
      max_count--;
    } while(max_count>=0);
  }
  if(res < 0) {
     printf("clear_i2c_err: lvd_i_w_() at addr=0x%1x - error\n",
	   addr);
    return plErr_HW;
  }
#endif
  /* wait completition for any chance, do not call wait_i2c() as it cause recursive calls! */
  int maxwait = 100;
  int max_wpass = 100;
  int v;
  for(i=0; i<max_wpass; i++) { /* wait clear completed */
    if(((pres=read_i2c_csr(dev, acard, idx, &v)) != plOK) || (v & VTX_I2R_ERR_RST)) { /* protection loop inside */
      return plErr_HW;
    }
    if(!(v & VTX_I2R_BSY_MRST)) {
      return plOK;
    }
    u_delay(maxwait);
  }
  printf("clear_i2c_err: timeout expired\n");
  return plErr_Timeout;
}

/*************************************************************************
 * Return content of VERTEX[M3] "scaling " register"
 *, if (-1) - error
 */
static ems_u32 read_scaling(struct lvd_dev* dev, int addr) {
  ems_u32 val=0;
  int res = lvd_i_r(dev, addr, vertex.scaling, &val);
  if(res<0) {
    printf("read_scaling: addr=0x%1x, lvd_i_r() error\n", addr);
    return (ems_u32)(-1);
  }
  return val;
}

/* RETURNS: 0 -OK, <0 - error */
static int write_scaling(struct lvd_dev* dev, int addr, ems_u32 val) { 
  int i;
  int max_pass = 10;
  int res;
  for(i=0; i<max_pass; i++) {
    if((res = lvd_i_w(dev, addr, vertex.scaling, val)) == 0) break; /* protection loop */
  }
  if(res<0) {
    printf("write_scaling: addr=0x%1x, lvd_i_r() error\n", addr);
    return -1;
  }
  return 0;
}

/*************************************************************************
 * Function to change I2C acknowledge delay, all chip registers are lost!
 * Here unix = 1 or 2
 */
plerrcode lvd_vertex_i2c_delay(struct lvd_dev* dev, int addr, int unit, ems_u32 del) {
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  struct vtx_info* info = (acard) ? acard->cardinfo : 0;
  int idx = unit-1;
  if(!info) {
    printf("lvd_vertex_i2c_delay: addr=0x%1x idx=%1d :  acard=%1p info=%1p - error\n",
	   addr, idx, acard, info);
    return plErr_InvalidUse;
  }
  plerrcode pres = plErr_InvalidUse;
  if(!info || info[idx].chiptype != VTX_CHIP_MATE3) 
    return plErr_BadModTyp;
  if(acard->mtype == ZEL_LVD_ADC_VERTEXM3) 
    pres = set_mate3_chip_delay_m3(dev, acard, idx, del);
  else if(acard->mtype == ZEL_LVD_ADC_VERTEXUN) 
    pres = set_mate3_chip_delay_un(dev, acard, idx, del); 
  return pres;
}

/* p[0]: delay on  VA clock to read chip registers
 * p[1]: period of cycle to write chip registers
 * p[2]: period of cycle to write DACs of repeater cards 
 * p[3]: period of cycle to write ADC offset of VertexADC/M3/UN modules 
 */
static plerrcode set_scaling_vata_va(struct lvd_dev* dev, struct lvd_acard* acard, int idx, ems_u32* p) {
  int addr = acard->addr;
  struct vtx_info* info = acard->cardinfo;
  p[0] &= 0xf; 
  if(p[1] > 0x7) p[1] = 0x7;
  if(p[2] > 0x7) p[2] = 0x7;
  if(p[3] > 0x7) p[3] = 0x7;
  info[idx].scaling = p[3] | (p[2] << 4) | (p[1] << 8);
  info[idx].delay = p[0];
  if(write_scaling(dev, addr, info[idx].scaling) < 0) {  /* protection loop inside */
    return plErr_HW;
  }
  if(set_vata_chip_delay(dev, acard, idx, p[0]) < 0) { 
     return plErr_HW;
  }
  return plOK;
}
/* p[0]: delay on serial bus (either I2C or VA clock to read chip content)
 * p[1]: period of cycle on serial bus (to write chip contents, I2C/VA)
 * p[2]: period of cycle to write DACs of repeater cards 
 * p[3]: period of cycle to write ADC offset of VertexADC/M3/UN modules 
 */
static plerrcode set_scaling_vata_un(struct lvd_dev* dev, struct lvd_acard* acard, int idx, ems_u32* p) {
  int addr = acard->addr;
  struct vtx_info*  info = acard->cardinfo;
  ems_u32 m3 = 0x1e00; /* slowest write to I2C regs */
  int idx_sec = (idx) ? 0 : 1; /* another module half index */ 
  if(p[0] > 0xf) p[0] = 0xf;
  if(p[1] > 0xf) p[1] = 0xf;
  if(p[2] > 0x7) p[2] = 0x7;
  if(p[3] > 0x7) p[3] = 0x7;
  if(info[idx_sec].chiptype == VTX_CHIP_MATE3) { 
    int old = read_scaling(dev, addr); 
    m3 = old & 0xfe00;
    if(m3 < 1) m3 = 0x1e00; /* not set yet, set to maximum */
  }
  info[idx].scaling = p[3] | (p[2] << 3) | (p[1] << 6) | m3;
  info[idx].delay = p[0];
  if(write_scaling(dev, addr, info[idx].scaling) < 0) {  /* protection loop inside */
    printf("set_scaling_vata_un error in write_scaling() addr=0x%1x idx=%1d\n",
	   addr, idx);
    return plErr_HW;
  }
  return plOK;
}

/* p[0]: delay on serial bus (either I2C or VA clock to read chip content
 * p[1]: period of cycle on serial bus (to write chip contents, I2C/VA)
 * p[2]: period of cycle to write DACs of repeater cards 
 * p[3]: period of cycle to write ADC offset of VertexADC/M3/UN modules 
 */
static plerrcode set_scaling_mate3_m3(struct lvd_dev* dev, struct lvd_acard* acard, int idx, ems_u32* p) {
  int addr = acard->addr;
  struct vtx_info*  info = acard->cardinfo;
  
  p[0] &= 0x7;
  if(p[1] > 0xf) p[1] = 0xf;
  if(p[2] > 0x7) p[2] = 0x7;
  if(p[3] > 0x7) p[3] = 0x7;

  info[idx].scaling = p[3] | (p[2] << 4) | (p[1] << 8) | (p[0] <<12);
  info[idx].delay = p[0];
  if(write_scaling(dev, addr, info[idx].scaling) < 0) {  /* protection loop inside */
    return plErr_HW;
  }
  return plOK;
}

/* p[0]: delay on serial bus (either I2C or VA clock to read chip content
 * p[1]: period of cycle on serial bus (to write chip contents, I2C/VA)
 * p[2]: period of cycle to write DACs of repeater cards 
 * p[3]: period of cycle to write ADC offset of VertexADC/M3/UN modules 
 */
static plerrcode set_scaling_mate3_un(struct lvd_dev* dev, struct lvd_acard* acard, int idx, ems_u32* p) {
  struct vtx_info*  info = (acard) ? acard->cardinfo : 0;
  if(!info) {
    printf("set_scaling_mate3_un: acard=%1p info=%1p -error \n", acard, info);
    return plErr_InvalidUse;
  }
  int addr = acard->addr;
  ems_u32 va = 0x1C0; /* slowest write to VA regs */
  int idx_sec = (idx) ? 0 : 1; /* another module half index */ 
  p[0] &= 0x7;
  if(p[1] > 0xf) p[1] = 0xf;
  if(p[2] > 0x7) p[2] = 0x7;
  if(p[3] > 0x7) p[3] = 0x7;
  if(info[idx_sec].chiptype == VTX_CHIP_VA32TA2) { 
    int old = read_scaling(dev, addr); 
    va = old & 0x1C0;
    if(va < 1) va = 0x1C0; /* not set yet, set to maximum */
  }
  info[idx].scaling = p[3] | (p[2] << 3) | (p[1] << 9) | (p[0] <<13) | va;
  info[idx].delay = p[0];
  if(write_scaling(dev, addr, info[idx].scaling) < 0) {  /* protection loop inside */
    return plErr_HW;
  }
  return plOK;
}

plerrcode lvd_vertex_cycles(struct lvd_dev* dev, int addr, int idx, ems_u32* p) {
  struct lvd_acard* acard= GetVertexCard(dev, addr);
  struct vtx_info* info = (acard) ? acard->cardinfo : 0;
  if(!info) {
    printf("lvd_vertex_cycles: addr=0x%1x idx=%1d acard=%1p info=%1p  - error\n", 
	   addr, idx, acard, info);
    return plErr_BadModTyp;
  }
  return (info[idx].set_scaling) ? info[idx].set_scaling(dev, acard, idx, p) : plErr_InvalidUse;
}

plerrcode lvd_vertex_i2c_get_period_m3(struct lvd_dev* dev, int addr, ems_u32* p) {
  ems_u32 v=0;
  /* read v "scaling" reg value */
  if(lvd_i_r(dev, addr, vertex.scaling, &v) < 0) {
    printf("lvd_vertex_i2c_period_m3: addr=0x%1x, lvd_i_r() error\n",
	   addr);
    return plErr_HW;
  }
  *p = (v>>8) & 0xf;
  return plOK;
}


plerrcode lvd_vertex_i2c_period_m3(struct lvd_dev* dev, int addr, ems_u32 p) {
  ems_u32 v=0;
  plerrcode pres = plErr_HW;
  /* read v "scaling" reg value */
  if(lvd_i_r(dev, addr, vertex.scaling, &v) < 0) {
    printf("lvd_vertex_i2c_period_m3: addr=0x%1x, lvd_i_r() error\n",
	   addr);
  }
  else {
    /* write new value */
    v = (v & (~0x0f00)) | ((p<<8) & 0x0f00);
    if(lvd_i_w(dev, addr, vertex.scaling, v) < 0) {
      printf("i2c_period: addr=0c%1x, lvd_i_w() error\n",
	     addr);
    }
    else {
      pres = plOK;
    }
  }
  return pres;
}
plerrcode lvd_vertex_i2c_get_period_un(struct lvd_dev* dev, int addr, ems_u32* p) {
  ems_u32 v=0;
  /* read v "scaling" reg value */
  if(lvd_i_r(dev, addr, vertex.scaling, &v) < 0) {
    printf("lvd_vertex_i2c_period_un: addr=0x%1x, lvd_i_r() error\n",
	   addr);
    return plErr_HW;
  }
  *p = (v>>9) & 0xf;
  return plOK;
}


plerrcode lvd_vertex_i2c_period_un(struct lvd_dev* dev, int addr, ems_u32 p) {
  ems_u32 v=0;
  plerrcode pres = plErr_HW;
  /* read v "scaling" reg value */
  if(lvd_i_r(dev, addr, vertex.scaling, &v) < 0) {
    printf("lvd_vertex_i2c_period_m3: addr=0x%1x, lvd_i_r() error\n",
	   addr);
  }
  else {
    /* write new value */
    v = (v & (~0x1e00)) | ((p<<9) & 0x1e00);
    if(lvd_i_w(dev, addr, vertex.scaling, v) < 0) {
      printf("i2c_period: addr=0c%1x, lvd_i_w() error\n",
	     addr);
    }
    else {
      pres = plOK;
    }
  }
  return pres;
}

/*************************************************************************
 * Function to to initialize addresses of MATE3 chips,
 * all chip setting will be lost.
 **************************************************************************/
static plerrcode init_mate3_addresses(struct lvd_dev* dev, int addr, int idx, int* nchips) {
  int res;
  ems_u32 csr_val;
  plerrcode pres;
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  struct vtx_info* info = (acard) ? acard->cardinfo : 0;
  int ddma =  is_ddma_active(dev);
  *nchips = 0;
  if(!info || info[idx].chiptype != VTX_CHIP_MATE3) {
    return plErr_BadModTyp;
  }
  info[idx].loaded &= (~CHIP_LOADED); /* chips lost settings after VTX_I2R_SETUP! */
  /* write in csr, no wait for ready before, as it is for MRESET mainly */
  res = lvd_i_w_(dev, addr, info[idx].spec.ser_csr, 2, VTX_I2R_SETUP);
  if((res < 0) && ddma) { /* not good solution... temporary */
    int max_count = 10;
    do {
      res = lvd_i_w_(dev, addr, info[idx].spec.ser_csr, 2, VTX_I2R_SETUP);
      if(res >= 0) break;
      max_count--;
    } while(max_count>=0);
  }
  /* wait completition, for any chance */
  pres = wait_i2c(dev, acard, idx, &csr_val); /* protection*/
  if(res < 0) {
     printf("write_i2c_csr: lvd_i_w_() at addr=0x%1x - error\n",
	   addr);
    return plErr_HW;
  }
  if(pres != plOK) {
    return pres;
  }
  *nchips = csr_val & VTX_I2C_CHIPS;
  return pres;
}

/******************************************************************************
 * Sequencer initialization for VTX_CHIP_MATE3 chips
 * for parameter list see parameters of seq_init() below.
 * It must be proved before, that "acard" is ZEL_LVD_ADC_VERTEXM3 card.
 * It must be proved before, that "acard" is ZEL_LVD_ADC_VERTEXM3 card.
 *
 * VTX_CHIP_MATE3  parameters (par[0]== VTX_CHIP_MATE3 == 2):
 *
 * par[1]                   : read-out reset switch
 * par[2] = nro             : number of read-out params
 * par[3]..par[2+nro]       : read-out params
 * p[3+nro] = nprog         : number of 16-bit words to be loaded, seq prog
 * p[4+nro]..p[4+nro+nprog] : "sequencer program"
 */

static void free_seq_par_m3(void* par) {
  if(par) { /*we set members to 0 to prevent invalid use (for any chance) */
    struct m3_seq_par* seq_par = par;
    free(seq_par->ro_par);
    seq_par->ro_len = 0;
    seq_par->prog = 0; /* was not allocated! */
    seq_par->prog_len = 0;
    free(seq_par);
  }
}

static struct m3_seq_par* decode_m3_args(ems_u32* par) {
  int i;
  ems_u32* ptr = par;
  struct m3_seq_par* seq_par = malloc(sizeof(struct m3_seq_par));
  memset(seq_par, 0, sizeof(struct m3_seq_par));
  if(seq_par) {
    memset(seq_par, 0, sizeof(struct m3_seq_par));
    seq_par->ro_reset    = par[1];
    /* read-out params */
    seq_par->ro_len = par[2];
    seq_par->ro_par = malloc(seq_par->ro_len*sizeof(ems_u32));
    if(!seq_par->ro_par) {
      free_seq_par_m3(seq_par);
      return 0;
    }
    ptr = &par[3];
    for(i=0; i<seq_par->ro_len; i++) {
      seq_par->ro_par[i] = *ptr++;
    }
    /* sequencer program, no copy! */
    seq_par->prog_len = *ptr++;
    seq_par->prog = ptr;
  }
  return seq_par;
}


/****************************************************************************
 * 
 */
static plerrcode read_i2c_data(struct lvd_dev* dev,  struct lvd_acard* acard, int idx, ems_u32* val) {
  int addr = acard->addr; /* in this function must be NO protection loop */
  struct vtx_info* info = acard->cardinfo;
  return (lvd_i_r_(dev, addr, info[idx].spec.ser_data, 2, val)) ? plErr_HW : plOK;
#if 0
/*   int ddma =  */
  is_ddma_active(dev);
  int res  = lvd_i_r_(dev, addr, info[idx].spec.ser_data, 2, val);
  if((res < 0)) { /* not good solution... temporary  && ddma */
    int max_count = 10;
    do {
      res = lvd_i_r_(dev, addr, info[idx].spec.ser_data, 2, val);
      max_count--;
    } while((res<0) && (max_count>=0));
  }
  return res ? plErr_HW : plOK;
#endif
}
/* ======================================*/
/*********************************************************************
 * Function to write data[] array of "size" values to MATE3 chip registers
 * 'idx' is 0 for LV and 1 for HV, other - error!
 * Only 15 bits of data[i] is used.
 * 
 */

static plerrcode write_mate3_reg_arr(struct lvd_dev* dev, int addr,
				     int idx, int size, ems_u32* data) {
  int i;
  int max_rep = 10; /* for each word to escape I2C errors */
  plerrcode pres = plOK;
  ems_u32 i2c_val;
  int err = 0;
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  struct vtx_info*  info = (acard) ? acard->cardinfo : 0;
  if(!info) { /* for any chance */
    printf("write_mate3_reg_arr(): addr=0x%1x idx=%1d - invalid module or chip types\n",
	   addr, idx);
    return plErr_BadModTyp;
  }
  /* wait completition of previous operation, if error - clear it */
  pres = wait_i2c(dev, acard, idx, &i2c_val); /* protection */
  if(pres != plOK) {
    printf("write_mate3_reg_arr:  addr=0x%1x idx=%1d wait_i2c() i2c_val=0x%1x at enter  returns error\n",
	   addr, idx, i2c_val);
    return pres;
  }
  write_scaling(dev, addr, info[idx].scaling);  /* protection loop inside */
  /* write all data words */
  for(i=0; i<size; i++) {
    ems_u32 tmp;
    int chip = (data[i] >> MATE3_CHIP_SHIFT) & 0xf;
    int reg  = (data[i] >> MATE3_REG_SHIFT) & 0x7;
    int rep;
    int err_key=0;    
    for(rep=0; rep<max_rep; rep++) { /* loop to recover I2C error */
       /* write_i2c_data */
      if(lvd_i_w_(dev, addr,  info[idx].spec.ser_data, 2, (data[i] &0x7fff))) {
        printf("write_mate3_reg_arr: at addr: 0x%1x idx=%1d, data[%1d]=0x%1x, lvd_i_w_() error\n",
               addr, idx, i, data[i]);
        pres = plErr_HW;
        continue;
      }
      /* read word back, wait_i2c inside before read */
      if((pres = read_mate3_reg(dev, acard, idx, chip, reg, &tmp)) != plOK) {  /* read_mate3_reg: must have no loop inside!*/
        printf("write_mate3_reg_arr: addr=0x%1x idx=%1d chip=%1d reg=%1d read error\n",
               addr, idx, chip, reg);
        continue;
      }
      if((data[i]&0xff) != (tmp &0xff)) {
        printf("write_mate3_reg_arr: addr=0x%1x idx=%1d chip=%1d reg=%1d write=0x%1x read=0x%1x, error \n",
               addr, idx, chip, reg, (data[i]&0xff), (tmp &0xff));
        pres = plErr_HW;
	err_key=1;
        continue;
      }
      else { /* OK, read == write*/
	if(err_key) {
	  printf("write_mate3_reg_arr: addr=0x%1x idx=%1d chip=%1d reg=%1d write=0x%1x read=0x%1x pass=%1d OK\n",
		 addr, idx, chip, reg, (data[i]&0xff), (tmp &0xff), rep);	 
	  err_key = 0;
	}
        break;
      }      
    }                                /* end of loop to recover I2C error */
  }
  if(err) {
    printf("write_mate3_reg_arr: addr=0x%1x idx=%1d err_num=%1d, ignore errors\n",  addr, idx, err);
    if(err == size) {
      return plErr_HW;
    }
  }
  return plOK;
}

/* Must be provided by calling functions:
 * acard - points either to VertexADCM3 or VertexADCUN
 * chip - in [0,15] range
 * reg  - in [1, 6] range 
 */
static ems_u32 read_mate3_reg(struct lvd_dev* dev, struct lvd_acard* acard,
			      int idx, int chip, int reg, ems_u32* data) {
  int i;
  ems_u32 val = 0;
  plerrcode pres;
  int addr = acard->addr;
  struct vtx_info* info = acard->cardinfo;
  /* wait  I2C intreface to be ready for the operation */
  for(i=0; i<2; i++) { /* protection loop */
    if((pres = wait_i2c(dev, acard, idx, &val)) == plOK) break; /* protection loop inside */
  }
  if(pres != plOK) {
    printf("read_mate3_reg: at addr: 0x%1x idx=%1d, chip:%1d reg:%1d - not ready error\n",
	   addr, idx, chip, reg);
    *data = 0;
    return pres;
  }
  /* request data, no loop after that for FIFO data regs! */
  val = (chip << MATE3_CHIP_SHIFT) | (reg << MATE3_REG_SHIFT) |  0x8000; /* read */
  int r = lvd_i_w_(dev, addr, info[idx].spec.ser_data, 2, val);
  if(r) pres = plErr_HW;
  /* wait data arriving */
  val = 0;
  for(i=0; i<2; i++) { /* protection loop */
    if((pres = wait_i2c_data(dev, acard, idx, &val)) == plOK) break; /* protection loop inside */
    printf("read_mate3_reg: at addr: 0x%1x idx=%1d: failed wait data at read\n",
	   addr, idx);
  }
  if(pres != plOK) {
    printf("read_mate3_reg: at addr: 0x%1x idx=%1d, data=0x%1x, wait_i2c_data() error\n",
	   addr, idx, *data);
    r |= -1;
  }
  /* read data */
  pres = read_i2c_data(dev, acard, idx, data);
  if(pres != plOK) {
    *data = 0;
    printf("read_mate3_reg: at addr: 0x%1x idx=%1d: failed read\n", addr, idx);
  } else {
    *data = (*data) & 0xff; /* one byte */
  }
  return pres;
}

/*
 * data[size] must be allocated and has enough size: 6*chips+2, max_chips =15
 */
static ems_u32 read_mate3_board_reg(struct lvd_dev* dev, struct lvd_acard* acard,
				    int idx, int* size, int* err, ems_u32* data) {
  ems_u32 val = 0;
  plerrcode pres;
  int addr = acard->addr;
  *err = 0;
  /* wait  I2C intreface to be ready for the operation */
  if((pres=wait_i2c(dev, acard, idx, &val)) != plOK) { /* protection loop inside */
    printf("read_mate3_board_reg: at addr: 0x%1x idx=%1d - not ready error\n",
	   addr, idx);
    return pres;
  }
  /* how many chips we have ? */
  int max_chips = val & VTX_I2C_CHIPS;
  if(*size < max_chips*MATE3_REG_WORDS) {
    printf("read_mate3_board_reg(): addr=0x%1x idx=%1d size=%1d, but required %1d, error\n",
	   addr, idx, *size,  max_chips*MATE3_REG_WORDS);
    return plErr_InvalidUse;
  }
  int chip, reg;
  int w=0;
  for(chip=0; chip<max_chips; chip++) {
    for(reg=1; reg<=MATE3_REG_WORDS; reg++) {
      int max_count = 3;
      int res = -1;
      do { /* loop to recover acknowledge errors */
	res = read_mate3_reg(dev, acard, idx, chip, reg, &data[w]);
	if (res != plOK) {
	  data[w] = -1;
	  clear_i2c_err(dev, acard, idx);  /* protection loop inside */
	  max_count--;
	}
      } while((res !=plOK ) && (max_count > 0));  
      if(res != plOK) {
	*err += 1;
	printf("read_mate3_board_reg(): addr=0x%1x idx=%1d chip=%1d reg=%1d - error\n",
	       addr, idx, chip, reg);
      }
      w++;
    }
  }
  *size = w;
  return plOK; /* but some data MAY BE INVALID! to see which one later */
}


int lvd_vertex_read_mate3_chip_number(struct lvd_dev* dev, int addr, int idx) {
  ems_u32 val = 0;
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  struct vtx_info*  info = (acard) ? acard->cardinfo : 0;
  if(!info || info[idx].chiptype != VTX_CHIP_MATE3) {
    printf("lvd_vertex_read_mate3_chip_number(): addr=0x%1x idx=%1d - invalid module or chip types\n",
	   addr, idx);
    return 0;
  }
  /* wait  I2C intreface to be ready for the operation */
  if(wait_i2c(dev, acard, idx, &val) != plOK) { /* protection loop inside */
    printf("lvd_vertex_read_mate3_chip_number: at addr: 0x%1x idx=%1d - not ready, error\n",
	   addr, idx);
    return 0;
  }
  return (val & VTX_I2C_CHIPS);
}


/* In call:  data[1] - length of expected reg. data words
  in returned data:
   data[0] -  number of register data words (starting from data[2]!
   data[1] -number of invalid data words
   data[2], ... ,data[2+data[1]]
 */
plerrcode lvd_vertex_read_mate3_board_reg(struct lvd_dev* dev, int addr,
				    int idx, ems_u32* data) {
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  struct vtx_info*  info = (acard) ? acard->cardinfo : 0;
  if(!info || info[idx].chiptype != VTX_CHIP_MATE3) {
    printf("read_mate3_board_reg(): addr=0x%1x idx=%1d - invalid module or chip types\n",
	   addr, idx);
    return plErr_BadModTyp;
  }
  return read_mate3_board_reg(dev, acard, idx, &data[0], &data[1], &data[2]);
}
/*********************************************************************
 * Function to read data of MATE3 chip register
 * idx -  is 0 for LV and 1 for HV, other - error!
 *
 */

plerrcode lvd_vertex_read_mate3_reg(struct lvd_dev* dev, int addr,
				    int idx, int chip, int reg, ems_u32* data) {
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  struct vtx_info* info = (acard) ? acard->cardinfo : 0;
  if(!info || info[idx].chiptype != VTX_CHIP_MATE3) {
    printf("lvd_vertex_read_mate3_reg(): addr=0x%1x idx=%1d - invalid module or chip type\n",
	   addr, idx);
    return plErr_BadModTyp;
  }
  return read_mate3_reg(dev, acard, idx, chip, reg, data);
}


/************************************************************************
 * The same  for VertexADCM3 and  VertexADCUN:
 *   - vertex.scaling
 * Different:
 ************************************************************************/
static int set_mate3_chip_delay_m3(struct lvd_dev* dev, struct lvd_acard* acard, int idx,
                                  int delay) {
  int res;
  ems_u32 org, val;
  struct vtx_info* info = (acard) ?  acard->cardinfo : 0;
  if(!info) return -1; /* for any chance*/
  int addr = acard->addr;
  org = info[idx].scaling;
  val = (org & 0xfff) | ((delay & 0x7) << 12);
  res  = lvd_i_w(dev, addr, vertex.scaling, val);
  res |= lvd_i_r(dev, addr, vertex.scaling, &org);
  if(!res && (org == val)) {
    info[idx].scaling = val;
  }
  return res;
}
/************************************************************************
 * For VertexADCUN: delay in[0,7] range
 *   - vertex.scaling as for VertexADCM3
 ************************************************************************/
static int set_mate3_chip_delay_un(struct lvd_dev* dev, struct lvd_acard* acard, int idx,
                                  int delay) {
  int res = 0;
  ems_u32 val;
  int addr = acard->addr;
  struct vtx_info* info = (acard) ?  acard->cardinfo : 0;
  if(!info) return -1;
  ems_u32 org = info[idx].scaling;
  val = (org & 0x1fff) | ((delay & 7)<< 13);
  res  = lvd_i_w(dev, addr, vertex.scaling, val);
  res |= lvd_i_r(dev, addr, vertex.scaling, &org);
  if(!res && (org == val)) {
    info[idx].scaling = org;
  }
  else {
    res = -1;
  }
  return res;
}

/************************************************************************
 * Finds delay of acknowledge on i2c bus of VertexADCM3 and  VertexADCUN
 * (based on expected number of chips)
 * For both modules must be the same:
 *   - clear_i2c_err(dev, addr, lh_offs), lh_offs - offset to half of module
 *   - wait_i2c(...)
 * DIFFERENT:
 * set_mate3_chip_delay_m3() - VertexADCM3
 * set_mate3_chip_delay_un() - VertexADCUN
 ************************************************************************/

static int find_mate3_chip_delay(struct lvd_dev* dev, struct lvd_acard* acard,
                                   int idx, int exp_chips) {
  typedef int (*smate3d)(struct lvd_dev*, struct lvd_acard*, int, int);
  int i;
  int err=0;
  int d1 = -1;
  int d2 = -1;
  int nchips;
  plerrcode pres;
  int res = -1;
  int delay = 0;
  int addr = acard->addr;
  smate3d set_del = 0;
  if(acard->mtype == ZEL_LVD_ADC_VERTEXM3) 
    set_del = set_mate3_chip_delay_m3;
  else if(acard->mtype == ZEL_LVD_ADC_VERTEXUN) 
    set_del = set_mate3_chip_delay_un;
  else
    return plErr_BadModTyp;
  for(i=0; i<8; i++) {
    if((*set_del)(dev, acard, idx, i)) {
      err++;
      continue;
    }
    if((pres = init_mate3_addresses(dev, addr, idx, &nchips)) == plOK) {
      if(exp_chips == nchips) {
	if(d1 < 0) {
	  d1 =i;
	}
	else {
	  d2 = i;
	}
      }
      else {
	if(d1 >= 0) break;
      }
    }
    else {
      if(d1 >= 0) break;
    }
  }
  /* select the optimum delay in the middle of [d1,d2] range*/
  if(d1 < 0) { /* failed */
    delay = -1;
    printf("chip_delay_m3():  addr=0x%1x idx=%1d failed\n", addr, idx);
  }
  else {
    if(d2 < 0) {
      delay = d1;
    }
    else {
      delay = (d1+d2)/2;              /* round to a smaller value */
    }
  }
  /* set optimum delay, if found */
  res = -1;
  if(delay >= 0) { /* delay was found for expected number of chips: exp_chips */
    res = 0;
    if((res=(*set_del)(dev, acard, idx, delay))) {
      printf("chip_delay_m3(): addr=0x%1x idx=%1d, delay = %1d - failed set\n",
             delay, acard->addr, idx);
      res =-1;
    }
    else {
      nchips = exp_chips;
      if((pres = init_mate3_addresses(dev, addr, idx, &nchips))  != plOK) {
	res=-1;
      }
      else if(exp_chips != nchips) {
	  res = -1;
      }
      clear_i2c_err(dev, acard, idx);  /* protection loop inside */
    }
  }
  if(res < 0) {
    printf("mate3_chip_delay(): addr=0x%1x idx=%1d: failed, [%1d, %1d]\n",
	   addr, idx, d1,d2);
  }
  else {
    printf("mate3_chip_delay(): addr=0x%1x idx=%1d, [%1d,%1d] delay range, delay=%1d nchips=%1d\n",
	   addr, idx, d1, d2, delay, nchips);
  }
  
  return res; 
}

/*********************************************************************
 * Function to initialize MATE3 chips.
 * At first tries to find out optimal I2C acknowledge delay
 * (I2C "frequency" bits of "scaling" register must be set earlier.
 *
 * Makes MRESET, set chip addresses (starting from zero) 
 * and load chip registers.
 *  
 * 'idx' is 0 for LV and 1 for HV, other - error!
 * data: array for (words/6) chips: {reg_1,...reg_6}, {reg_1,...reg_6},...
 * The chip and register number will be set here!
 */
static plerrcode load_mate3_chips(struct lvd_dev* dev, struct lvd_acard* acard,
				 int idx, int words, ems_u32* data) {
  plerrcode  pres;
  int        addr = acard->addr;
  struct vtx_info *info = (struct vtx_info*)acard->cardinfo;
  /* is it connected to MATE3 chips? */
  if((info[idx].chiptype != VTX_CHIP_MATE3)) {
    printf("load_mate3_chips: addr=0x%1x, idx=%1d - not MATE3 chips, error\n",
	   addr, idx);
    return plErr_BadModTyp;
  }
  /* is lentgth of data correct? */
  if((words<=0) || (words % MATE3_REG_WORDS)) {
    printf("load_mate3_chips(): addr=0x%1x idx=%1d, words=%1d - invalid\n",
	   addr, idx, words);
    return plErr_ArgRange;
  }
  /* set delay and scaling from info */
  write_scaling(dev, addr, info[idx].scaling);  /* protection loop inside */
  
  /* try init addresses */
  int exp_chips = words / MATE3_REG_WORDS;
  int nchips;
  if(((pres = init_mate3_addresses(dev, addr, idx, &nchips)) != plOK) ||
     (nchips != exp_chips)) {
    /* find I2C acknowledge delay and number of chips, will reset registers of chips in (idx) only! */
    clear_i2c_err(dev, acard, idx);  /* protection loop inside */
    if(find_mate3_chip_delay(dev, acard, idx, exp_chips) < 0) {
      return plErr_InvalidUse;
    }
  }

  /* load chips */
  int w;
  for(w=0; w<words; w++) {
    int chip = (w / MATE3_REG_WORDS) & 0xf;
    int reg  = (w % MATE3_REG_WORDS) + 1;
    data[w] = SET_I2C_CSR_DATA(data[w], chip, reg) & 0x7fff;
  }
  pres = write_mate3_reg_arr(dev, addr, idx, words, data);
  if(pres != plOK) {
    printf("load_mate3_chips: addr=0x%1x idx=%1d, write_mate3_reg_arr() error\n",
	   addr, idx);
    return pres;
  }
  info[idx].numchips    = exp_chips;
  info[idx].numchannels = exp_chips*MATE3_CHAN;
  info[idx].loaded |= CHIP_LOADED;
  return plOK;
}

/*********************************************************************
 * Function to initialize MATE3 chips. Makes MRESET, set chip addresses
 * (starting from zero) and load chip registers.
 *  
 *  unit:   is 1 for LV and 2 for HV, 3 - both!
 *  words:  length of data[]
 *  data[]: array of 32-bit words, which contains groups of 6 words per chip:
 *          { }
 */

plerrcode lvd_vertex_init_mate3(struct lvd_dev* dev, int addr, int unit,
			       int words, ems_u32* data) {
  struct lvd_acard* acard = GetVertexMate3Card(dev, addr);
  if (!acard) {
    printf("lvd_vertex_load_mate3: no Vertex Mate3 Card at address: 0x%1x unit=%1d, error\n",
	   addr, unit);
    return plErr_Program;
  }
  return lvd_vertex_load_chips(dev, addr, unit, words, data);
}


/*****************************************************************************
 * data_in : array[6] of words(only data) to be loaded in reg.1 -reg.6
 * data_out: starting address where put modified data, at least 6 words length
 * Fills  "Reg" and "Chip" bits in data_out
 */
static void set_mate3_reg_chip_bits(int chip, ems_u32* data_in, ems_u32* data_out) {
  int i=0;
  for(i=0; i<6; i++) {
    int reg = i+1;
    data_out[i] = SET_I2C_CSR_DATA(data_in[i], chip, reg);
  }
}

/*****************************************************************************
 * data_in : word(only data) to be loaded in reg of MATE3, reg must be in [1,6] range
 * data_out: starting address where put modified data, at least "nchips" words length
 * Fills  "Reg" and "Chip" bits in data_out
 */
static void set_mate3_reg_all_chips(int nchips, int reg, ems_u32 data_in, ems_u32* data_out) {
  int chip;
  for(chip=0; chip<nchips; chip++) {
    data_out[chip] = SET_I2C_CSR_DATA(data_in, chip, reg);
  }
}

/*****************************************************************************
 * The function does NOT make MRESET for MATE3 board. 
 * It only re-write registers of chip(s).
 * DACs must be loaded and chip addresses must be set before. 
 */
plerrcode lvd_vertex_load_mate3_chip(struct lvd_dev* dev, int addr, int unit,
				     int chip, int words, ems_u32* data) {
  plerrcode  pres;
  int         res;
  ems_u32  old_cr;
  int     n_chips = -1;
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  struct vtx_info *info = (acard) ? acard->cardinfo : 0;
  int idx = unit - 1;

  if(words != 6) { /* is words really needed as args here? */
    printf("lvd_vertex_load_mate3: at address: 0x%1x unit=%1d arg5(words)!=6, error\n",
	   addr, unit);
    return plErr_InvalidUse;
  }
  if(!info) {
    printf("lvd_vertex_load_mate3_chip: addr=0x%1x idx=%1d: acard or info is null\n", addr, idx);
    return plErr_BadModTyp;
  }
  n_chips = info->numchips;
  /* is it connected to MATE3 chips? */
  if(info[idx].chiptype != VTX_CHIP_MATE3) {
    printf("lvd_vertex_load_mate3_chip: addr=0x%1x, idx=%1d - not MATE3 chips in info or not VERTEXM3, error\n",
	   addr, idx);
    return plErr_BadModTyp;
  }
/* check chip number */
  if(n_chips < 1) {
    printf("lvd_vertex_load_mate3_chip: addr=0x%1x, idx=%1d in info %1d chips, error\n",
	   addr, idx, n_chips);
    return plErr_InvalidUse;
  }
  if((chip>=0) && (chip>=n_chips)) {
    printf("lvd_vertex_load_mate3_chip: addr=0x%1x, idx=%1d in info %1d chips,but write to %1d, error\n",
	   addr, idx, n_chips, chip);
    return plErr_InvalidUse;
  }
  /* is DAC loaded ? If not, it has no sense to load chips */
  if(!IS_DACS_LOADED(info[idx].loaded)) {
    printf("lvd_vertex_load_mate3_chip: addr=0x%1x, idx=%1d - DACs not loaded,error\n",
	   addr, idx);
    return plErr_InvalidUse;
  }
  /* if sequencer is running, inhibit trigger */
  res = prep_safe_load(dev, acard, idx, &old_cr);
  if(res) {
    printf("lvd_vertex_load_mate3_chip: addr=0x%1x, idx=%1d, prep_safe_load() error\n",
	   addr, idx);
    return plErr_HW;
  }

  /* prepare words for loading in one chip */
  int nf  = (chip < 0) ? 0 : chip;          /* start loading from chip number nf */
  int nl  = (chip < 0) ? n_chips - 1 : chip;    /* last load to the chip number nl */
  ems_u32 tmp[(nl-nf+1)*6];
  int c;
  for(c=nf; c<=nl; c++) {
    set_mate3_reg_chip_bits(c, data, &tmp[(c-nf)*6]);
  }
  pres = write_mate3_reg_arr(dev, addr, idx, (nl-nf+1)*6, tmp);
  if(pres != plOK) {
    printf("lvd_vertex_load_mate3_chip: addr=0x%1x, idx=%1d, write_mate3_reg_arr() error\n",
	   addr, idx);
    lvd_i_w(dev, addr, vertex.cr, old_cr); /* restore cr reg */
    return pres;
  }

  return plOK;
}

/*****************************************************************************
 * Select channel[s] in vertex vata32 chips for TestPulse calibration
 * idx:    0 - LV, 1 - HV, other - error!
 * par[0] - channel to be selected
 * par[1] - if 1, channel par[0] is selected in all chips
 * 
 */
static plerrcode sel_mate3_chan_tp(struct lvd_dev* dev, struct lvd_acard* acard, int idx, ems_u32* par) {
  if(acard->initialized == 0) {
    return 0;
  }
  struct vtx_info* info=(struct vtx_info*)acard->cardinfo;
  int nchips = info[idx].numchips;
  int addr = acard->addr;
  if(nchips <= 0) {
    printf("sel_mate3_chan_tp: nchips <= 0, addr=0x%1x unit=%1d, error\n",
	   addr, idx+1);
    return plErr_InvalidUse;
  }
  ems_u32  sel_ch[nchips];  /* content of REGCAPIJECT registers */
  if((int)par[0] < 0) {
    set_mate3_reg_all_chips(nchips, 5, 0, sel_ch); /* no selected channel, par[0]<0 case, no HW r/w */
  }
  else {
    if(par[1] != 0) { /* chan. par[0] in all available chips */
      set_mate3_reg_all_chips(nchips, 5, ((par[0]%16)+1) & 0x1f, sel_ch); /*  no HW r/w  */
    }
    else { /* chan p[0] only */
      set_mate3_reg_all_chips(nchips, 5, 0, sel_ch); /* clear all reg.5,  no HW r/w  */
      int chip = par[0] / 16;                        /* set only in needed chip */
      if(chip > nchips) chip = nchips - 1;
      sel_ch[chip] |= (par[0] % 16) + 1;
    }
  }
  return write_mate3_reg_arr(dev, addr, idx, nchips, sel_ch); /* protection */
}

/* NEVER call this function, it is to make compiler happy */
void lvd_vertex_never_call(struct lvd_dev* dev, int addr, int unit) { /* for unused static funcs */
  is_i2c_ready(dev, 0, 0, 0);
  set_mate3_reg_all_chips(1, 1, 0, 0);
  read_scaling(dev, addr);
  write_scaling(dev, addr, 0X741); /* protection loop inside */
  find_vata_chip_delay(dev, 0, unit-1, 875);
  seq_halt_n(dev, 0, 0, 0);
  printf("lvd_vertex_never_call: MUST NOT be called! error=%1d",  ACK_DELAY_IND);
}

/* for all initialised VertexADC modules of any type*/
plerrcode lvd_vertex_calibr_ch(struct lvd_dev* dev, int chan, int all) {
  int card;
  int res = 0;
  all = (all) ? 1 : 0; /* to be sure */
  for (card=0; card<dev->num_acards; card++) {
    if(IsVertexCard(dev, card)) {
      /* stop module, if it is in read-out  */
      struct lvd_acard* acard = &(dev->acards[card]);
      if((acard->initialized != 0) && ((acard->daqmode & VTX_CR_RUN ) != 0)) { /* will run */
	ems_u32 cr = 0;
	int addr = acard->addr;
	res  =  get_cr(dev, addr, &cr); /* protection loop inside */
	if(res) {
	  printf("lvd_vertex_calibr_ch: error in get_cr() for addr=0x%1x\n", addr);
	  return plErr_HW;
	}
	if(cr & VTX_CR_RUN) { /* running */
	  res= lvd_stop_vertex(dev, acard); /* it is the only way to clear err bits */
	  if(res) {
	    return plErr_HW;
	  }
	}
	/* set channel numbers to be selected */
	struct vtx_info* info = acard->cardinfo;
	if((chan>=0) && all) {	  
	  info[0].mon_ch = (info[0].chiptype == VTX_CHIP_MATE3) ? (chan & 0xf) : chan;
	  info[1].mon_ch = (info[1].chiptype == VTX_CHIP_MATE3) ? (chan & 0xf) : chan;
	}
	else  {
	  info[0].mon_ch = chan;
	  info[1].mon_ch = chan;
	}
	info[0].tp_all = all;
	info[1].tp_all = all;
	/* start module in read-out mode mode */	
	res |= lvd_start_vertex(dev, acard);
      }
    }
  }
  if(res) {
    printf("lvd_vertex_calibr_ch: lvd_start_vertex(): error\n");
    return plErr_HW;
  }
  return plOK;
}
/**********************************************************************************
*/
plerrcode lvd_vertex_load_chips(struct lvd_dev* dev, int addr, int unit, int nw, ems_u32* data) {
  struct lvd_acard* acard = GetVertexCard(dev, addr);
  struct vtx_info* info = (acard) ? acard->cardinfo : 0;
  int idx = unit - 1;
  plerrcode pres = plOK;
  if(!info) {
    printf("lvd_vertex_load_chips(): addr=0x%1x idx=%1d - no VertexADC card or info, error\n", 
	   addr, idx);
    return plErr_BadModTyp;
  }
  if(info[idx].load_chips == 0) { /* is function to load chips known? */
    printf("lvd_vertex_load_chips(): no function known  for addr=0x%1x idx=%1d\n", addr, idx);
    return plErr_InvalidUse;
  }
  if(((info[idx].loaded & CODE_LOADED) == 0) || !IS_DACS_LOADED(info[idx].loaded)) {
    printf("lvd_vertex_load_chips(): addr=0x%1x idx=%1d info[%1d].loaded=0x%1x - not ready load chips\n",
	   addr, idx, idx, info[idx].loaded);
    return plErr_InvalidUse;
  }
  /* Inhibit trigger for loaded module - it is the minimum we can make here */
  ems_u32 curr_cr = -1; 
  if(prep_safe_load(dev, acard, idx, &curr_cr)) {
    return plErr_HW;
  }
  /* load chips */
  info[idx].numchips    = 0;
  info[idx].numchannels = 0;
  info[idx].loaded &= (~CHIP_LOADED);
  if(!info[idx].load_chips) {
    printf("lvd_vertex_load_chips(): addr=0x%1x idx=%1d info[idx].load_chips==0, error\n",
	   addr, idx);
    return plErr_InvalidUse;
  }
  pres = info[idx].load_chips(dev, acard, idx, nw, data);
  /* restore previous module status */
  if(lvd_i_w(dev, addr, vertex.cr, curr_cr)) {   /* restore cr reg */
    printf("lvd_vertex_load_chips(): addr=0x%1x idx=%1d - failed restore module status\n",
	   addr, idx);
    return plErr_HW;
  }
  printf("lvd_vertex_load_chips(): addr=0x%1x idx=%1d nchips=%1d\n", addr, idx, info[idx].numchips);
  return pres;
}
 
