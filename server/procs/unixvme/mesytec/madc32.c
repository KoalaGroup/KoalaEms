/*
 * procs/unixvme/mesytec/madc32.c
 * created 2011-07-05 h.xu
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: madc32.c,v 1.7 2017/10/20 23:20:52 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <stdlib.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../objects/var/variables.h"
#include "../../../lowlevel/unixvme/vme.h"
#include "../../../lowlevel/devices.h"
/*#include "../../../lowlevel/perfspect/perfspect.h"*/
#include "../../../trigger/trigger.h"
#include "../../procs.h"
#include "../vme_verify.h"
#include "mesytec_common.h"

extern ems_u32* outptr;
extern unsigned int *memberlist;

RCS_REGISTER(cvsid, "procs/unixvme/mesytec")

/*
 * madc 32-channel ADC
 * A32D16 (registers) A32D32/BLT32 (data buffer)
 * reserves 65536 Byte
 */
/*
 // Address registers
 *0x0000..0x0400: data fifo (1026 words)
 *0x6000: address_source
 *0x6002: address_reg
 *0x6004: module_id
 *0x6008: soft_reset
 *0x600E: firmware_revision
 // IRQ(ROACK)
 *0x6010: irq_level
 *0x6012: irq_vector
 *0x6014: irq_test
 *0x6016: irq_reset
 *0x6018: irq_threshold
 *0x601A: Max_transfer_data
 *0x601C: Withdraw IRQ
 // MCST CBLT
 *0x6020: cblt_mcst_control
 *0x6022: cblt_address
 *0x6024: mcst_address
 // FIFO handling
 *0x6030: buffer_data_length
 *0x6032: data_len_format
 *0x6034: readout_reset
 *0x6036: multievent
 *0x6038: marking_type
 *0x603A: start_acq
 *0x603C: fifo_reset
 *0x603E: data_ready
 // Operation mode
 *0x6040: bank_operation
 *0x6042: adc_resolution
 *0x6044: output_format
 *0x6046: adc_override
 *0x6048: slc_off
 *0x604A: skip_oorange
 // Gate generator
 *0x6050: hold_delay0
 *0x6052: hold_delay1
 *0x6054: hold_width0
 *0x6056: hold_width1
 *0x6058: use_gg
 // IO
 *0x6060: input_range
 *0x6062: ECL_term
 *0x6064: ECL_gate1_osc
 *0x6066: ECL_fc_res
 *0x6068: ECL_busy
 *0x606A: NIM_gat1_osc
 *0x606C: NIM_fc_reset
 *0x606E: NIM_busy
 // Testpulser
 *0x6070: pulser_status
 // MRC
 *0x6080: rc_busno
 *0x6082: rc_modnum
 *0x6084: rc_opcode
 *0x6086: rc_adr
 *0x6088: rc_dat
 *0x608A: send return status
 // CTRA
 *0x6090: Reset_ctr_ab
 *0x6092: evctr_lo
 *0x6094: evctr_hi
 *0x6096: ts_sources
 *0x6098: ts_divisor
 *0x609C: ts_counter_lo
 *0x609E: ts_counter_hi
 // CRTB
 *0x60A0: adc_busy_time_lo
 *0x60A2: adc_busy_time_hi
 *0x60A4: gate1_time_lo
 *0x60A6: gate1_time_hi
 *0x60A8: time_0
 *0x60AA: time_1
 *0x60AC: time_2
 *0x60AE: stop_ctr
 */

#define ANY 0x0815

/*****************************************************************************/
/*
 * p[0]: argcount==0/1
 * [p[1]: index of module; -1: all modules of intrumentation system]
 */

static plerrcode
madc32reset_module(int idx)
{
    ml_entry* module=ModulEnt(idx);
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
//    ems_u16 stat;
    int res, i;

    // reset module and start to be readout//
       res=dev->write_a32d16(dev,addr+0x603A,0x0); 
       printf("proc_madc32reset: stop acq!\n");
       if(res!=2) return plErr_System;

       res=dev->write_a32d16(dev,addr+0x6036,0x0); 
       printf("proc_madc32reset: Single event readout!\n");
       if(res!=2) return plErr_System;
 
       res=dev->write_a32d16(dev,addr+0x6012,0x0);
       printf("proc_madc32reset: set IRQ Vector to 0!\n");
       if(res!=2) return plErr_System;

       res=dev->write_a32d16(dev,addr+0x6010,0x1);
       printf("proc_madc32reset: set IRQ level to 1!\n");
       if(res!=2) return plErr_System;

       res=dev->write_a32d16(dev,addr+0x6034,0x0);
       printf("proc_madc32reset: Reset fifo!\n");
       if(res!=2) return plErr_System;

       res=dev->write_a32d16(dev,addr+0x6044,0x1);
       printf("proc_madc32reset: Set bit resolution as 4k!\n");
       if(res!=2) return plErr_System;

       res=dev->write_a32d16(dev,addr+0x603A,0x1); 
       printf("proc_madc32reset:start_acq!\n");
       if(res!=2) return plErr_System;

    // check amnesia and write GEO address if necessary//
    printf("read addr 0x%x offs 0x600e\n", addr);


    // clear threshold memory //
    for (i=0; i<32; i++) {
        res=dev->write_a32d16(dev, addr+0x4000+2*i, 0);
//printf("madc32reset: clear threshold memory!\n");
        if (res!=2) return plErr_System;
    }
    return plOK;
}

plerrcode proc_madc32reset(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    int i, pres=plOK;
    if (p[0] && (ip[1]>-1)) {
        pres=madc32reset_module(p[1]);
//printf("proc_madc32reset is OK4\n");
    } else {
        for (i=memberlist[0]; i>0; i--) {
//printf("proc_madc32reset: OK44\n");
            pres=madc32reset_module(i);
            if (pres!=plOK)
                break;
        }
    }
    return pres;
}

plerrcode test_proc_madc32reset(ems_u32* p)
{
#if 0
    plerrcode res;
#endif
 
    if ((p[0]!=0) && (p[0]!=1))
        return plErr_ArgNum;
  //   printf("test_proc_madc32reset: OK6\n");
  //   printf("memberlist=%p, mtypes=%p\n",memberlist,mtypes);
#if 0
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) {
        printf("test_proc_madc32reset: test_proc_vme failed!,return res=%d\n",res);
        return res;
    }
#endif
  //   printf("test_proc_v792reset  OK7\n");
  //   printf("res is: %d\n",res);

    wirbrauchen=0;
//printf("test_proc_madc32reset  OK8\n");
    return plOK;
}

char name_proc_madc32reset[] = "madc32reset";
int ver_proc_madc32reset = 1;

/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_madc32read_one(__attribute__((unused)) ems_u32* p)
{
    unsigned int i;
    int res;

    *outptr++=memberlist[0];
    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;
        ems_u32 data, *help;
        ems_u16 datalength;

        help=outptr++;
/*
       res=dev->write_a32d16(dev,addr+0x603A,0x0); 
       printf("proc_madc32read_one: stop acq!\n");
       if(res!=2) return plErr_System;

       res=dev->write_a32d16(dev,addr+0x6036,0x0); 
       printf("proc_madc32read_one: Single event readout!\n");
       if(res!=2) return plErr_System;
 
       res=dev->write_a32d16(dev,addr+0x6012,0x0);
       printf("proc_madc32read_one: set IRQ Vector to 0!\n");
       if(res!=2) return plErr_System;

       res=dev->write_a32d16(dev,addr+0x6010,0x1);
       printf("proc_madc32read_one: set IRQ level to 1!\n");
       if(res!=2) return plErr_System;

       res=dev->write_a32d16(dev,addr+0x6034,0x0);
       printf("proc_madc32read_one: Reset fifo!\n");
       if(res!=2) return plErr_System;

       res=dev->write_a32d16(dev,addr+0x603A,0x1); 
       printf("proc_madc32read_one: start_acq!\n");
       if(res!=2) return plErr_System;
*/

       res=dev->read_a32d16(dev,addr+0x6030,&datalength);
      if(res!=2) return plErr_System;
       printf("datalength is:0x%d\n",datalength);

        /* read until code==4 (trailer) */
        do {
printf("proc_madc32read_one: start to read_1!\n");
            res=dev->read_a32d32(dev, addr, &data);
printf("proc_madc32read_one: res1 is %d\n", res);
printf("data is:0x%d\n",data);
            if (res!=4) {
                *help=outptr-help-1;
                return plErr_System;
            }
            *outptr++=data;
           printf("data is:0x%d\n",data);

        } while ((data&0x7000000)!=0x4000000);
        *help=outptr-help-1;

       res=dev->write_a32d16(dev,addr+0x6034,0x0);
       printf("proc_madc32read_one: readout reset!\n");
       if(res!=2) return plErr_System;

        /* read until code==6 (invalid word) and discard data */

    }
    return plOK;
}

plerrcode test_proc_madc32read_one(ems_u32* p)
{
#if 0
    plerrcode res;
#endif
    if (p[0])
        return plErr_ArgNum;
#if 0
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK)
        return res;
#endif
    wirbrauchen=memberlist[0]*34*32;
// printf("test_proc_madc32read_one: skipped\n");
    return plOK;
}

char name_proc_madc32read_one[] = "madc32read_one";
int ver_proc_madc32read_one = 1;

/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module idx (or -1 for all mxdc32 of this IS)
 * p[2]: offset
 * [p[3]: value (16 bit)]
 */
plerrcode
proc_madc32_reg(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int res;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_madc32_reg);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;

        pres=plOK;

        if (p[0]>2) {
            res=dev->write_a32d16(dev, addr+p[2], p[3]);
            if (res!=2) {
                complain("madc32_reg(%04x, %04x): res=%d errno=%s",
                        p[2], p[3],
                        res, strerror(errno));
                return plErr_System;
            }
        } else {
            ems_u16 val;
            res=dev->read_a32d16(dev, addr+p[2], &val);
            if (res!=2) {
                complain("madc32_reg(%04x): res=%d errno=%s",
                        p[2],
                        res, strerror(errno));
                return plErr_System;
            }
            *outptr++=val;
        }
    }

    return pres;
}

plerrcode test_proc_madc32_reg(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=2 && p[0]!=3) {
        complain("madc32_reg: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("madc32_reg: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (!is_mesytecvme(module, mtypes)) {
            complain("mxdc32_irq: p[1](==%u): no mesytec mxdc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=ip[1]>=0?1:nr_mxdc_modules(mtypes);
    return plOK;
}

char name_proc_madc32_reg[] = "madc32_reg";
int ver_proc_madc32_reg = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==7
 * p[1]: module idx (or -1 for all madc32 of this IS)
 * p[2]: module ID
 *         -1: the default should be used
 *         !!! use madc32_id to set usefull IDs
 *         >=0: ID is used for the first module and incremented for each
 *              following module
 * p[3]: resolution (0..4) (-1: don't change the default)
 * p[4]: input_range (0..2) (-1: don't change the default)
 * p[5]: marking_type (0: event counter  1: time stamp  3: extended time stamp)
 * p[6]: sliding_scale off
 * p[7]: bank_operation (0..2)
 * p[8]: skip_oorange (0|1) (-1: don't change the default)
 */

plerrcode
proc_madc32_init(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    ems_u16 val;
    plerrcode pres;
    int res, i;

printf("proc_madc32_init: p[1]=%d, idx=%d\n", ip[1], mxdc_member_idx());

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_madc32_init);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;
        ems_u16 module_id, marking_type;

        pres=plOK;

        /* read firmware revision */
        res=dev->read_a32d16(dev, addr+0x600e, &val);
        if (res!=2) {
            complain("madc32_init: firmware revision: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }
        printf("madc32(%08x): fw=%02x.%02x\n", addr, (val>>8)&0xff, val&0xff);

        /* soft reset (there is no hard reset) */
        res=dev->write_a32d16(dev, addr+0x6008, 1);
        if (res!=2) {
            complain("madc32_init: soft reset: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }
        usleep(100);

        /* disable readout to get a definite state */
        res=dev->write_a32d16(dev, addr+0x603a, 0);
        if (res!=2) {
          complain("madc32_init: disable readout: res=%d errno=%s",
                   res, strerror(errno));
          return plErr_System;
        }

        /* clear threshold memory */
        for (i=0; i<32; i++) {
            res=dev->write_a32d16(dev, addr+0x4000+2*i, 0);
            if (res!=2) {
                complain("madc32_init: clear thresholds: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
        }

        /* set module ID */
        if (ip[2]<0 || ip[2]==0xff) {
            res=dev->write_a32d16(dev, addr+0x6004, 0xff);
            if (res!=2) {
                complain("madc32_init: set module ID: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
            module_id=(addr>>24)&0xff;
        } else {
            module_id=ip[2];
            if (mxdc_member_idx()>=0)
                module_id+=mxdc_member_idx();
            res=dev->write_a32d16(dev, addr+0x6004, module_id);
            if (res!=2) {
                complain("madc32_init: set module ID: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
        }

        /* set resolution */
        if (ip[3]>=0) {
            res=dev->write_a32d16(dev, addr+0x6042, p[3]);
            if (res!=2) {
                complain("madc32_init: set resolution: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
        }

        /* set input range */
        if (ip[4]>=0) {
            res=dev->write_a32d16(dev, addr+0x6060, p[4]);
            if (res!=2) {
                complain("madc32_init: set input range: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
        }

        /* set marking type */
        marking_type=p[5];
        res=dev->write_a32d16(dev, addr+0x6038, marking_type);
        if (res!=2) {
            complain("madc32_init: set marking type: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }

        /* set sliding scale */
        res=dev->write_a32d16(dev, addr+0x6048, p[6]);
        if (res!=2) {
            complain("madc32_init: set sliding scale: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }

        /* set bank operation */
        res=dev->write_a32d16(dev, addr+0x6040, p[7]);
        if (res!=2) {
            complain("madc32_init: set bank operation: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }

        /* set skip_oorange */
        if (ip[8]>=0) {
            res=dev->write_a32d16(dev, addr+0x604a, p[8]);
            if (res!=2) {
                complain("madc32_init: set skip_oorange: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
        }

        /* FIFO reset */
        /* res=dev->write_a32d16(dev, addr+0x603c, ANY); */
        /* if (res!=2) { */
        /*   complain("madc32_init: reset FIFO: res=%d errno=%s", */
        /*            res, strerror(errno)); */
        /*   return plErr_System; */
        /* } */

        mxdc32_init_private(module, module_id, marking_type, -1);
    }

    return pres;
}

plerrcode test_proc_madc32_init(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=8) {
        complain("madc32_init: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("madc32_init: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (module->modultype!=mesytec_madc32) {
            complain("madc32_init: p[1](==%u): no mesytec madc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_madc32_init[] = "madc32_init";
int ver_proc_madc32_init = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3 or 33
 * p[1]: module idx (or -1 for all madc32 of this IS)
 *
 * A (with p[0]==3):
 * p[2]: channel (-1 for all channels)
 * p[3]: threshold (0x1FFF to disable the channel)
 *
 * B (with p[0]==33)
 * p[2..33]: threshold[0..31]
 */

plerrcode
proc_madc32_threshold(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mxdc32_threshold);
    } else {
        pres=proc_mxdc32_threshold(p);
    }

    return pres;
}

plerrcode test_proc_madc32_threshold(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=3 && p[0]!=33) {
        complain("madc32_threshold: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("madc32_threshold: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (module->modultype!=mesytec_madc32) {
            complain("madc32_threshold: p[1](==%u): no mesytec madc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_madc32_threshold[] = "madc32_threshold";
int ver_proc_madc32_threshold = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2 or 5
 * p[1]: module idx (or -1 for all madc32 of this IS)
 * p[2]: bank (-1 for both)
 * [p[3]: delay
 *  p[4]: width
 * ]
 */
plerrcode
proc_madc32_gate(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int res;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_madc32_gate);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;
        ems_u16 use_gg, bankpattern;

        pres=plOK;

        bankpattern=ip[2]<0?3:p[2]+1; /* -1 --> b11, 0 --> b01, 1 --> b10 */

        /* read old use_gg */
        if (ip[2]>=0) { /* only one gate changed, we need the current value */
            res=dev->read_a32d16(dev, addr+0x6058, &use_gg);
            if (res!=2) {
                complain("madc32_gate: read use_gg: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
        } else {
            use_gg=0; /* no old value needed */
        }

        /* calculate new use_gg */
        if (p[0]<3) { /* disable gate(s) */
            use_gg&=!bankpattern;
        } else {      /* enable gate(s) */
            use_gg|=bankpattern;
            if (bankpattern&1) {
                res=dev->write_a32d16(dev, addr+0x6050, p[3]);
                if (res!=2) {
                    complain("madc32_gate: res=%d errno=%s",
                            res, strerror(errno));
                    return plErr_System;
                }
                res=dev->write_a32d16(dev, addr+0x6054, p[4]);
                if (res!=2) {
                    complain("madc32_gate: res=%d errno=%s",
                            res, strerror(errno));
                    return plErr_System;
                }
            }
            if (bankpattern&2) {
                res=dev->write_a32d16(dev, addr+0x6052, p[3]);
                if (res!=2) {
                    complain("madc32_gate: res=%d errno=%s",
                            res, strerror(errno));
                    return plErr_System;
                }
                res=dev->write_a32d16(dev, addr+0x6056, p[4]);
                if (res!=2) {
                    complain("madc32_gate: res=%d errno=%s",
                            res, strerror(errno));
                    return plErr_System;
                }
            }
        }

        res=dev->write_a32d16(dev, addr+0x6058, use_gg);
        if (res!=2) {
            complain("madc32_gate: write use_gg: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }
    }

    return pres;
}

plerrcode test_proc_madc32_gate(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=2 && p[0]!=4) {
        complain("madc32_gate: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("madc32_gate: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (module->modultype!=mesytec_madc32) {
            complain("madc32_gate: p[1](==%u): no mesytec madc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_madc32_gate[] = "madc32_gate";
int ver_proc_madc32_gate = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: module idx (or -1 for all madc32 of this IS)
 * p[2]: val
 */

plerrcode
proc_madc32_nimbusy(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mxdc32_nimbusy);
    } else {
        pres=proc_mxdc32_nimbusy(p);
    }

    return pres;
}

plerrcode test_proc_madc32_nimbusy(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=2) {
        complain("madc32_nimbusy: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("madc32_nimbusy: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (module->modultype!=mesytec_madc32) {
            complain("madc32_nimbusy: p[1](==%u): no mesytec madc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_madc32_nimbusy[] = "madc32_nimbusy";
int ver_proc_madc32_nimbusy = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: module idx
 */

plerrcode
proc_madc32_status(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
    ems_u16 val;
    int res;

    /* firmware revision */
    res=dev->read_a32d16(dev, addr+0x600e, &val);
    if (res!=2) {
        complain("madc32_status: firmware revision: res=%d errno=%s",
                res, strerror(errno));
        return plErr_System;
    }
    printf("madc32(%08x): fw=%04x\n", addr, val);

    dev->read_a32d16(dev, addr+0x6000, &val);
    printf("6000 addr source   : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6002, &val);
    printf("6002 addr reg      : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6010, &val);
    printf("6010 irq level     : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6012, &val);
    printf("6012 irq vector    : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6018, &val);
    printf("6018 irq threshold : %04x\n", val);

    dev->read_a32d16(dev, addr+0x601a, &val);
    printf("601a max trans data: %04x\n", val);

    dev->read_a32d16(dev, addr+0x601c, &val);
    printf("601c withdraw irq  : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6020, &val);
    printf("6020 cblt control  : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6022, &val);
    printf("6022 cblt address  : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6024, &val);
    printf("6024 mcst address  : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6030, &val);
    printf("6030 buff data len : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6032, &val);
    printf("6032 data len form : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6036, &val);
    printf("6036 multievent    : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6038, &val);
    printf("6038 marking type  : %04x\n", val);

    dev->read_a32d16(dev, addr+0x603a, &val);
    printf("603a start acq     : %04x\n", val);

    dev->read_a32d16(dev, addr+0x603e, &val);
    printf("603e data ready    : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6040, &val);
    printf("6040 bank operation: %04x\n", val);

    dev->read_a32d16(dev, addr+0x6042, &val);
    printf("6042 resolution    : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6044, &val);
    printf("6044 output format : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6046, &val);
    printf("6046 override      : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6048, &val);
    printf("6048 slc off       : %04x\n", val);

    dev->read_a32d16(dev, addr+0x604a, &val);
    printf("604a skip oorange  : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6050, &val);
    printf("6050 hold delay0   : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6052, &val);
    printf("6052 hold delay1   : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6054, &val);
    printf("6054 hold width0   : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6056, &val);
    printf("6056 hold width1   : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6058, &val);
    printf("6058 use gg        : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6060, &val);
    printf("6060 input range   : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6062, &val);
    printf("6062 ecl term      : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6064, &val);
    printf("6064 ecl gate1 osc : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6066, &val);
    printf("6066 ecl fc res    : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6068, &val);
    printf("6068 ecl busy      : %04x\n", val);

    dev->read_a32d16(dev, addr+0x606a, &val);
    printf("606a nim gat1 osc  : %04x\n", val);

    dev->read_a32d16(dev, addr+0x606c, &val);
    printf("606c nim fc reset  : %04x\n", val);

    dev->read_a32d16(dev, addr+0x606e, &val);
    printf("606e nim busy      : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6070, &val);
    printf("6070, pulser status : %04x\n", val);

    /* counters A */
    dev->read_a32d16(dev, addr+0x6090, &val);
    printf("6090 reset ctr ab  : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6092, &val);
    printf("6092 evctr lo      : %04x\n", val);
    dev->read_a32d16(dev, addr+0x6094, &val);
    printf("6094 evctr hi      : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6096, &val);
    printf("6096 ts sources    : %04x\n", val);

    dev->read_a32d16(dev, addr+0x6098, &val);
    printf("6098 ts divisor    : %04x\n", val);

    dev->read_a32d16(dev, addr+0x609c, &val);
    printf("609c ts counter lo : %04x\n", val);
    dev->read_a32d16(dev, addr+0x609e, &val);
    printf("609e ts counter hi : %04x\n", val);

    /* counters B */
    dev->read_a32d16(dev, addr+0x60a0, &val);
    printf("60a0 adc bysy time lo: %04x\n", val);
    dev->read_a32d16(dev, addr+0x60a2, &val);
    printf("60a2 adc bysy time hi: %04x\n", val);

    dev->read_a32d16(dev, addr+0x60a4, &val);
    printf("60a4 gate1 time lo : %04x\n", val);
    dev->read_a32d16(dev, addr+0x60a6, &val);
    printf("60a6 gate1 time hi : %04x\n", val);

    dev->read_a32d16(dev, addr+0x60a8, &val);
    printf("60a8 time 0        : %04x\n", val);
    dev->read_a32d16(dev, addr+0x60aa, &val);
    printf("60aa time 1        : %04x\n", val);
    dev->read_a32d16(dev, addr+0x60ac, &val);
    printf("60ac time 2        : %04x\n", val);

    dev->read_a32d16(dev, addr+0x60ae, &val);
    printf("60ae stop ctr: %04x\n", val);


    return plOK;
}

plerrcode test_proc_madc32_status(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=1) {
        complain("madc32_status: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (!valid_module(p[1], modul_vme)) {
        complain("madc32_status: p[1](==%u): no valid VME module", p[1]);
        return plErr_ArgRange;
    }

    module=ModulEnt(p[1]);
    if (module->modultype!=mesytec_madc32) {
        complain("madc32_status: p[1](==%u): no mesytec madc32", p[1]);
        return plErr_BadModTyp;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_madc32_status[] = "madc32_status";
int ver_proc_madc32_status = 1;
/*****************************************************************************/
/*****************************************************************************/
