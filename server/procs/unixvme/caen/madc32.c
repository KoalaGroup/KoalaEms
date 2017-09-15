/*
 * procs/unixvme/caen/madc32.c
 * created 30.09,2011 h.xu
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: madc32.c,Exp $";
     //"$ZEL: v785_792.c,v 1.8 2011/04/06 20:30:35 wuestner Exp $";

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

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

RCS_REGISTER(cvsid, "procs/unixvme/caen")

/*
 * madc 32-channel ADC
 * A32D16 (registers) A32D32/BLT32 (data buffer)
 * reserves 65536 Byte
 */
/*
 // Address registers
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
 *0x604C: Ignore Thresholds
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
 *
 */

//#undef USE_ECOUNTS

//#ifdef USE_ECOUNTS /* wozu zum Teufel soll das gut sein? */
struct madc32_private {
    ems_i32 event;
};

static ems_u32 mtypes[]={mesytec_madc32, 0};



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
    ems_u16 stat;
    int res, i;

    // reset module //
    // res=dev->write_a32d16(dev, addr+0x1016, 0); should not be used 
    printf("start to reset the modules!\n");
    res=dev->write_a32d16(dev, addr+0x1006, 0x80);
    if (res!=2) return plErr_System;
    res=dev->write_a32d16(dev, addr+0x1008, 0x80);
    if (res!=2) return plErr_System;

    // check amnesia and write GEO address if necessary//
    printf("read addr 0x%x offs 0x100e\n", addr);
    res=dev->read_a32d16(dev, addr+0x100e, &stat);
    if (res!=2) return plErr_System;
    if (stat&0x10) {
        res=dev->write_a32d16(dev, addr+0x1002, idx);
        if (res!=2) return plErr_System;
        // reset module again to use this address //
        res=dev->write_a32d16(dev, addr+0x1006, 0x80);
        if (res!=2) return plErr_System;
        res=dev->write_a32d16(dev, addr+0x1008, 0x80);
        if (res!=2) return plErr_System;
    }

    // clear threshold memory //
    for (i=0; i<32; i++) {
        res=dev->write_a32d16(dev, addr+0x1080+2*i, 0);
        if (res!=2) return plErr_System;
    }

    return plOK;
}

plerrcode proc_madc32reset(ems_u32* p)
{ printf("proc_madc32reset is OK1\n");
    int i, pres=plOK;
printf("proc_madc32reset is OK2\n");
    if (p[0] && (p[1]>-1)) {
printf("proc_madc32reset is OK3\n");
        pres=madc32reset_module(p[1]);
printf("proc_madc32reset is OK4\n");
    } else {
        for (i=memberlist[0]; i>0; i--) {
printf("proc_madc32reset: OK44\n");
            pres=madc32reset_module(i);
printf("proc_madc32reset: pres is=%d\n",pres);
            if (pres!=plOK)
                break;
printf("proc_madc32reset: OK5\n");
        }
    }
    return pres;
}

plerrcode test_proc_madc32reset(ems_u32* p)
{
    plerrcode res;
 
    if ((p[0]!=0) && (p[0]!=1))
        return plErr_ArgNum;
     printf("test_proc_madc32reset: OK6\n");
  //   printf("memberlist=%p, mtypes=%p\n",memberlist,mtypes);
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK)
printf("test_proc_madc32reset: test_proc_vme failed!,return res=%d\n",res);
    //    return plOK;
        return res;
  //   printf("test_proc_v792reset  OK7\n");
  //   printf("res is: %d\n",res);

    wirbrauchen=0;
printf("test_proc_madc32reset  OK8\n");
    return plOK;

}

char name_proc_madc32reset[] = "madc32reset";
int ver_proc_madc32reset = 1;







