/*
 * procs/unixvme/mesytec/mqdc32.c
 * created 2011-09-22 PeWue
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: mqdc32.c,v 1.3 2017/10/20 23:20:52 wuestner Exp $";

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
 * mqdc 32-channel QDC
 * A32D16 (registers) A32D32/BLT32 (data buffer)
 * reserves 65536 Byte
 */
/*
 // Address registers
 *0x0000..0x0400: data fifo (65464 words)
 *0x6000: address_source
 *0x6002: address_reg
 *0x6004: module_id
 *0x6006: fast_vme
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
 // Gate limit
 *0x6050: limit_bank_0
 *0x6052: limit_bank_1
 *0x6054: exp_trig_delay0
 *0x6056: exp_trig_delay1
 // IO
 *0x6060: input_coupling
 *0x6062: ECL_term
 *0x6064: ECL_gate1_osc
 *0x6066: ECL_fc_res
 *0x6068: gate_select
 *0x606A: NIM_gat1_osc
 *0x606C: NIM_fc_reset
 *0x606E: NIM_busy
 // Testpulser
 *0x6070: pulser_status
 *0x6072: pulser_dac
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
 * p[0]: argcount==3
 * p[1]: module idx (or -1 for all mxdc32 of this IS)
 * p[2]: offset
 * [p[3]: value (16 bit)]
 */
plerrcode
proc_mqdc32_reg(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_mqdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int res;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mqdc32_reg);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;

        pres=plOK;

        if (p[0]>2) {
            res=dev->write_a32d16(dev, addr+p[2], p[3]);
            if (res!=2) {
                complain("mqdc32_reg(%04x, %04x): res=%d errno=%s",
                        p[2], p[3],
                        res, strerror(errno));
                return plErr_System;
            }
        } else {
            ems_u16 val;
            res=dev->read_a32d16(dev, addr+p[2], &val);
            if (res!=2) {
                complain("mqdc32_reg(%04x): res=%d errno=%s",
                        p[2],
                        res, strerror(errno));
                return plErr_System;
            }
            *outptr++=val;
        }
    }

    return pres;
}

plerrcode test_proc_mqdc32_reg(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_mqdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=2 && p[0]!=3) {
        complain("mqdc32_reg: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("mqdc32_reg: p[1](==%u): no valid VME module", p[1]);
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

char name_proc_mqdc32_reg[] = "mqdc32_reg";
int ver_proc_mqdc32_reg = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==7
 * p[1]: module idx (or -1 for all mqdc32 of this IS)
 * p[2]: module ID
 *         -1: the default should be used
 *         !!! use mqdc32_id to set usefull IDs
 *         >=0: ID is used for the first module and incremented for each
 *              following module
 * p[3]: marking_type (0: event counter  1: time stamp  3: extended time stamp)
 * p[4]: sliding_scale off
 * p[5]: bank_operation (0..2)
 * p[6]: skip_oorange (0|1) (-1: don't change the default)
 */

plerrcode
proc_mqdc32_init(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_mqdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    ems_u16 val;
    plerrcode pres;
    int res, i;

printf("proc_mqdc32_init: p[1]=%d, idx=%d\n", ip[1], mxdc_member_idx());

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mqdc32_init);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;
        ems_u16 module_id, marking_type;

        pres=plOK;

        /* read firmware revision */
        res=dev->read_a32d16(dev, addr+0x600e, &val);
        if (res!=2) {
            complain("mqdc32_init: firmware revision: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }
        printf("mqdc32(%08x): fw=%02x.%02x\n", addr, (val>>8)&0xff, val&0xff);

        /* soft reset (there is no hard reset) */
        res=dev->write_a32d16(dev, addr+0x6008, 1);
        if (res!=2) {
            complain("mqdc32_init: soft reset: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }

        /* clear threshold memory */
        for (i=0; i<32; i++) {
            res=dev->write_a32d16(dev, addr+0x4000+2*i, 0);
            if (res!=2) {
                complain("mqdc32_init: clear thresholds: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
        }

        /* set module ID */
        if (ip[2]<0 || ip[2]==0xff) {
            res=dev->write_a32d16(dev, addr+0x6004, 0xff);
            if (res!=2) {
                complain("mqdc32_init: set module ID: res=%d errno=%s",
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
                complain("mqdc32_init: set module ID: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
        }

        /* set marking type */
        marking_type=p[3];
        res=dev->write_a32d16(dev, addr+0x6038, marking_type);
        if (res!=2) {
            complain("mqdc32_init: set marking type: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }

        /* set sliding scale */
        res=dev->write_a32d16(dev, addr+0x6048, p[4]);
        if (res!=2) {
            complain("mqdc32_init: set sliding scale: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }

        /* set bank operation */
        res=dev->write_a32d16(dev, addr+0x6040, p[5]);
        if (res!=2) {
            complain("mqdc32_init: set bank operation: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }

        /* set skip_oorange */
        if (ip[6]>=0) {
            res=dev->write_a32d16(dev, addr+0x604a, p[6]);
            if (res!=2) {
                complain("mqdc32_init: set skip_oorange: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
        }

        mxdc32_init_private(module, module_id, marking_type, -1);
    }

    return pres;
}

plerrcode test_proc_mqdc32_init(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=6) {
        complain("mqdc32_init: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("mqdc32_init: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (module->modultype!=mesytec_mqdc32) {
            complain("mqdc32_init: p[1](==%u): no mesytec mqdc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mqdc32_init[] = "mqdc32_init";
int ver_proc_mqdc32_init = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3 or 33
 * p[1]: module idx (or -1 for all mqdc32 of this IS)
 *
 * A (with p[0]==3):
 * p[2]: channel (-1 for all channels)
 * p[3]: threshold (0x1FFF to disable the channel)
 *
 * B (with p[0]==33)
 * p[2..33]: threshold[0..31]
 */

plerrcode
proc_mqdc32_threshold(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_mqdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mxdc32_threshold);
    } else {
        pres=proc_mxdc32_threshold(p);
    }

    return pres;
}

plerrcode test_proc_mqdc32_threshold(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=3 && p[0]!=33) {
        complain("mqdc32_threshold: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("mqdc32_threshold: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (module->modultype!=mesytec_mqdc32) {
            complain("mqdc32_threshold: p[1](==%u): no mesytec mqdc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mqdc32_threshold[] = "mqdc32_threshold";
int ver_proc_mqdc32_threshold = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module idx (or -1 for all mqdc32 of this IS)
 * p[2]: bank (-1 for both)
 * p[3]: offset
 */
plerrcode
proc_mqdc32_offset(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_mqdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int res;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mqdc32_offset);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;

        pres=plOK;

        if (ip[2]<0 || ip[2]==0) {
            res=dev->write_a32d16(dev, addr+0x6044, p[3]);
            if (res!=2) {
                complain("mqdc32_offset: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
        }

        if (ip[2]<0 || ip[2]==1) {
            res=dev->write_a32d16(dev, addr+0x6046, p[3]);
            if (res!=2) {
                complain("mqdc32_offset: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
        }
    }

    return pres;
}

plerrcode test_proc_mqdc32_offset(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=3) {
        complain("mqdc32_offset: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("mqdc32_offset: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (module->modultype!=mesytec_mqdc32) {
            complain("mqdc32_offset: p[1](==%u): no mesytec mqdc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mqdc32_offset[] = "mqdc32_offset";
int ver_proc_mqdc32_offset = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: module idx (or -1 for all mqdc32 of this IS)
 * p[2]: 0: thresholds enabled 1: thresholds ignored
 */
plerrcode
proc_mqdc32_ignore_threshold(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_mqdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int res;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mqdc32_ignore_threshold);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;

        pres=plOK;

        res=dev->write_a32d16(dev, addr+0x604c, p[2]);
        if (res!=2) {
            complain("mqdc32_ignore_threshold: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }
    }

    return pres;
}

plerrcode test_proc_mqdc32_ignore_threshold(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=2) {
        complain("mqdc32_ignore_threshold: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("mqdc32_ignore_threshold: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (module->modultype!=mesytec_mqdc32) {
            complain("mqdc32_ignore_threshold: p[1](==%u): no mesytec mqdc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mqdc32_ignore_threshold[] = "mqdc32_ignore_threshold";
int ver_proc_mqdc32_ignore_threshold = 1;
/*****************************************************************************/
/*
 * delayed gating with experiment trigger can be enabled for both or non
 * bank. The delay can be different if split banks are used.
 * nim_fc_reset is set to '2' if one of the delays is nonzero.
 * p[0]: argcount: 2 or 3
 * p[1]: module idx (or -1 for all mqdc32 of this IS)
 * p[2]: delay for bank 0  (in terms of ns)
 * [p[3]: delay for bank 1 (in terms of ns)]
 */
plerrcode
proc_mqdc32_gate_delay(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_mqdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int res;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mqdc32_gate_delay);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;

        if (p[1]==0 && p[0]>2 &&  p[2]==0) {
            /* delay for both banks is zero --> disable experiment trigger */
            /* this is not ideal, e.g. ECL input is ignored */
            res=dev->write_a32d16(dev, addr+0x606c, 0);
        } else {
            /* redefine nim_fc_reset as experiment trigger input */
            /* here again: the possibility to use ECL is ignored*/
            res=dev->write_a32d16(dev, addr+0x606c, 2);
        }
        res|=dev->write_a32d16(dev, addr+0x6054, p[2]);
        if (p[0]>2)
            res|=dev->write_a32d16(dev, addr+0x6056, p[3]);
        if (res!=2) {
            complain("mqdc32_gate_delay: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }
    }
    return pres;
}

plerrcode test_proc_mqdc32_gate_delay(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]<2 || p[0]>3) {
        complain("mqdc32_gate_delay: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("mqdc32_gate_delay: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (module->modultype!=mesytec_mqdc32) {
            complain("mqdc32_gate_delay: p[1](==%u): no mesytec mqdc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mqdc32_gate_delay[] = "mqdc32_gate_delay";
int ver_proc_mqdc32_gate_delay = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: module idx (or -1 for all mqdc32 of this IS)
 * p[2]: 0: fast_vme disabled 1: fast_vme enabled
 */
plerrcode
proc_mqdc32_fast_vme(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_mqdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int res;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mqdc32_fast_vme);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;

        pres=plOK;

        res=dev->write_a32d16(dev, addr+0x600e, p[2]);
        if (res!=2) {
            complain("mqdc32_fast_vme: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }
    }

    return pres;
}

plerrcode test_proc_mqdc32_fast_vme(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=2) {
        complain("mqdc32_fast_vme: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("mqdc32_fast_vme: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (module->modultype!=mesytec_mqdc32) {
            complain("mqdc32_fast_vme: p[1](==%u): no mesytec mqdc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mqdc32_fast_vme[] = "mqdc32_fast_vme";
int ver_proc_mqdc32_fast_vme = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: module idx (or -1 for all mqdc32 of this IS)
 * p[2]: val
 */

plerrcode
proc_mqdc32_nimbusy(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_mqdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mxdc32_nimbusy);
    } else {
        pres=proc_mxdc32_nimbusy(p);
    }

    return pres;
}

plerrcode test_proc_mqdc32_nimbusy(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=2) {
        complain("mqdc32_nimbusy: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("mqdc32_nimbusy: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (module->modultype!=mesytec_mqdc32) {
            complain("mqdc32_nimbusy: p[1](==%u): no mesytec mqdc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mqdc32_nimbusy[] = "mqdc32_nimbusy";
int ver_proc_mqdc32_nimbusy = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: module idx
 */

plerrcode
proc_mqdc32_status(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
    ems_u16 val;
    int res;

    /* firmware revision */
    res=dev->read_a32d16(dev, addr+0x600e, &val);
    if (res!=2) {
        complain("mqdc32_status: firmware revision: res=%d errno=%s",
                res, strerror(errno));
        return plErr_System;
    }
    printf("mqdc32(%08x): fw=%04x\n", addr, val);


    return plOK;
}

plerrcode test_proc_mqdc32_status(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=1) {
        complain("mqdc32_status: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (!valid_module(p[1], modul_vme)) {
        complain("mqdc32_status: p[1](==%u): no valid VME module", p[1]);
        return plErr_ArgRange;
    }

    module=ModulEnt(p[1]);
    if (module->modultype!=mesytec_mqdc32) {
        complain("mqdc32_status: p[1](==%u): no mesytec mqdc32", p[1]);
        return plErr_BadModTyp;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mqdc32_status[] = "mqdc32_status";
int ver_proc_mqdc32_status = 1;
/*****************************************************************************/
/*****************************************************************************/
