/*
 * procs/unixvme/mesytec/mtdc32.c
 * created 2015-09-04 PeWue
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: mtdc32.c,v 1.4 2017/10/20 23:20:52 wuestner Exp $";

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
#include <unistd.h>

extern ems_u32* outptr;
extern unsigned int *memberlist;

RCS_REGISTER(cvsid, "procs/unixvme/mesytec")

/*
 * mtdc 32-channel ADC
 * A32D16 (registers) A32D32/BLT32 (data buffer)
 * reserves 65536 Byte
 */
/*
 // Address registers
 *0x0000: data fifo (65464 words)
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
 *0x6042: tdc_resolution
 *0x6044: output_format
 // Trigger
 *0x6050: bank0_win_start
 *0x6052: bank1_win_start
 *0x6054: bank0_win_width
 *0x6056: bank1_win_width
 *0x6058: bank0_trig_source
 *0x605a: bank1_trig_source
 *0x605c: first_hit
 // Inputs, outputs
 *0x6060: Negative_edge
 *0x6062: ECL_term
 *0x6064: ECL_trig1_osc
 *0x6068: Trig_select
 *0x606a: NIM_trig1_osc
 *0x606e: NIM_busy
 // Test pulser / Threshold
 *0x6070: pulser_status;
 *0x6070: pulser_pattern
 *0x6070: bank0_input_thr
 *0x6070: bank1_input_thr
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
 *0x60A8: time_0
 *0x60AA: time_1
 *0x60AC: time_2
 *0x60AE: stop_ctr
 // Multiplicity filter
 *0x60b0: high_limit0
 *0x60b0: low_limit0
 *0x60b0: high-limit1
 *0x60b0: low-limit1
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
proc_mtdc32_reg(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_mtdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int res;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mtdc32_reg);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;

        pres=plOK;

        if (p[0]>2) {
            res=dev->write_a32d16(dev, addr+p[2], p[3]);
            if (res!=2) {
                complain("mtdc32_reg(%04x, %04x): res=%d errno=%s",
                        p[2], p[3],
                        res, strerror(errno));
                return plErr_System;
            }
        } else {
            ems_u16 val;
            res=dev->read_a32d16(dev, addr+p[2], &val);
            if (res!=2) {
                complain("mtdc32_reg(%04x): res=%d errno=%s",
                        p[2],
                        res, strerror(errno));
                return plErr_System;
            }
            *outptr++=val;
        }
    }

    return pres;
}

plerrcode test_proc_mtdc32_reg(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_mtdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=2 && p[0]!=3) {
        complain("mtdc32_reg: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("mtdc32_reg: p[1](==%u): no valid VME module", p[1]);
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

char name_proc_mtdc32_reg[] = "mtdc32_reg";
int ver_proc_mtdc32_reg = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==8
 * p[1]: module idx (or -1 for all mtdc32 of this IS)
 * p[2]: module ID
 *         -1: the default should be used
 *         !!! use mtdc32_id to set usefull IDs
 *         >=0: ID is used for the first module and incremented for each
 *              following module
 * p[3]: resolution (2..9) (-1: don't change the default)
 * p[4]: negative_edge (2 bits) (-1: don't change the default)
 * p[5]: marking_type (0: event counter  1: time stamp  3: extended time stamp)
 * p[6]: output format (0: time difference 1: single hit full time stamp)
 * p[7]: bank_operation (0..2)
 * p[8]: first hit only
 */

plerrcode
proc_mtdc32_init(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_mtdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    ems_u16 val;
    plerrcode pres;
    int res;

printf("proc_mtdc32_init: p[1]=%d, idx=%d\n", ip[1], mxdc_member_idx());

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mtdc32_init);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;
        ems_u16 module_id, marking_type, output_format;

        pres=plOK;

        /* read firmware revision */
        res=dev->read_a32d16(dev, addr+0x600e, &val);
        if (res!=2) {
            complain("mtdc32_init: firmware revision: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }
        printf("mtdc32(%08x): fw=%02x.%02x\n", addr, (val>>8)&0xff, val&0xff);

        /* soft reset (there is no hard reset) */
        res=dev->write_a32d16(dev, addr+0x6008, 1);
        if (res!=2) {
            complain("mtdc32_init: soft reset: res=%d errno=%s",
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

        /* set module ID */
        if (ip[2]<0 || ip[2]==0xff) {
            res=dev->write_a32d16(dev, addr+0x6004, 0xff);
            if (res!=2) {
                complain("mtdc32_init: set module ID: res=%d errno=%s",
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
                complain("mtdc32_init: set module ID: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
        }

        /* set resolution */
        if (ip[3]>=0) {
            res=dev->write_a32d16(dev, addr+0x6042, p[3]);
            if (res!=2) {
                complain("mtdc32_init: set resolution: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
        }

        /* set negative edge */
        if (ip[4]>=0) {
            res=dev->write_a32d16(dev, addr+0x6060, p[4]);
            if (res!=2) {
                complain("mtdc32_init: set negative edge: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
        }

        /* set marking type */
        marking_type=p[5];
        res=dev->write_a32d16(dev, addr+0x6038, marking_type);
        if (res!=2) {
            complain("mtdc32_init: set marking type: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }

        /* set output format */
        output_format=p[6];
        res=dev->write_a32d16(dev, addr+0x6044, p[6]);
        if (res!=2) {
            complain("mtdc32_init: set output format: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }

        /* set bank operation */
        res=dev->write_a32d16(dev, addr+0x6040, p[7]);
        if (res!=2) {
            complain("mtdc32_init: set bank operation: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }

        /* set first_hit */
        res=dev->write_a32d16(dev, addr+0x605c, p[8]);
        if (res!=2) {
            complain("mtdc32_init: set first_hit: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }

        mxdc32_init_private(module, module_id, marking_type, output_format);
    }

    return pres;
}

plerrcode test_proc_mtdc32_init(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=8) {
        complain("mtdc32_init: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("mtdc32_init: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (module->modultype!=mesytec_mtdc32) {
            complain("mtdc32_init: p[1](==%u): no mesytec mtdc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mtdc32_init[] = "mtdc32_init";
int ver_proc_mtdc32_init = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: module idx (or -1 for all mtdc32 of this IS)
 * p[2]: val
 */

plerrcode
proc_mtdc32_nimbusy(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_mtdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mxdc32_nimbusy);
    } else {
        pres=proc_mxdc32_nimbusy(p);
    }

    return pres;
}

plerrcode test_proc_mtdc32_nimbusy(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=2) {
        complain("mtdc32_nimbusy: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("mtdc32_nimbusy: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (module->modultype!=mesytec_mtdc32) {
            complain("mtdc32_nimbusy: p[1](==%u): no mesytec mtdc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mtdc32_nimbusy[] = "mtdc32_nimbusy";
int ver_proc_mtdc32_nimbusy = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: module idx (or -1 for all mtdc32 of this IS)
 * p[2]: selection   (0x6068, 0: NIM 1: ECL)
 * p[3]: termination (0x6062)
 */

plerrcode
proc_mtdc32_trigger(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_mtdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int res;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mtdc32_trigger);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;

        pres=plOK;

        res=dev->write_a32d16(dev, addr+0x6068, p[2]);
        if (res!=2) {
            complain("mtdc32_trigger: set selction: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }

        res=dev->write_a32d16(dev, addr+0x6062, p[2]);
        if (res!=2) {
            complain("mtdc32_trigger: set ecl_term: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }
    }

    return pres;
}

plerrcode test_proc_mtdc32_trigger(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=3) {
        complain("mtdc32_trigger: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("mtdc32_trigger: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (module->modultype!=mesytec_mtdc32) {
            complain("mtdc32_trigger: p[1](==%u): no mesytec mtdc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mtdc32_trigger[] = "mtdc32_trigger";
int ver_proc_mtdc32_trigger = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module idx (or -1 for all mtdc32 of this IS)
 * p[2]: bank (-1 for both)
 * p[3]: value
 */
plerrcode
proc_mtdc32_threshold(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_mtdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int res;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mtdc32_threshold);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;

        pres=plOK;

        if (ip[2]<0 || ip[2]==0) {
            res=dev->write_a32d16(dev, addr+0x6078, p[3]);
            if (res!=2) {
                complain("mtdc32_threshold: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
        }

        if (ip[2]<0 || ip[2]==1) {
            res=dev->write_a32d16(dev, addr+0x607a, p[3]);
            if (res!=2) {
                complain("mtdc32_threshold: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
        }
    }

    return pres;
}

plerrcode test_proc_mtdc32_threshold(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=3) {
        complain("mtdc32_threshold: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("mtdc32_threshold: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (module->modultype!=mesytec_mtdc32) {
            complain("mtdc32_threshold: p[1](==%u): no mesytec mtdc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mtdc32_threshold[] = "mtdc32_threshold";
int ver_proc_mtdc32_threshold = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==5
 * p[1]: module idx (or -1 for all mtdc32 of this IS)
 * p[2]: bank (-1 for both)
 * p[3]: start
 * p[4]: width
 * p[5]: source
 */
plerrcode
proc_mtdc32_window(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_mtdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int res;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mtdc32_window);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;

        pres=plOK;

        if (ip[2]<0 || ip[2]==0) {
            res=dev->write_a32d16(dev, addr+0x6050, p[3]);
            if (res!=2) {
                complain("mtdc32_window: start: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
            res=dev->write_a32d16(dev, addr+0x6054, p[4]);
            if (res!=2) {
                complain("mtdc32_window: width: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
            res=dev->write_a32d16(dev, addr+0x6058, p[5]);
            if (res!=2) {
                complain("mtdc32_window: source: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
        }

        if (ip[2]<0 || ip[2]==1) {
            res=dev->write_a32d16(dev, addr+0x6052, p[3]);
            if (res!=2) {
                complain("mtdc32_window: start: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
            res=dev->write_a32d16(dev, addr+0x6056, p[4]);
            if (res!=2) {
                complain("mtdc32_window: width: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
            res=dev->write_a32d16(dev, addr+0x605a, p[5]);
            if (res!=2) {
                complain("mtdc32_window: source: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
        }
    }

    return pres;
}

plerrcode test_proc_mtdc32_window(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=5) {
        complain("mtdc32_window: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("mtdc32_window: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (module->modultype!=mesytec_mtdc32) {
            complain("mtdc32_window: p[1](==%u): no mesytec mtdc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mtdc32_window[] = "mtdc32_window";
int ver_proc_mtdc32_window = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: module idx
 */

plerrcode
proc_mtdc32_status(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
    ems_u16 val;
    int res;

    /* firmware revision */
    res=dev->read_a32d16(dev, addr+0x600e, &val);
    if (res!=2) {
        complain("mtdc32_status: firmware revision: res=%d errno=%s",
                res, strerror(errno));
        return plErr_System;
    }
    printf("mtdc32(%08x): fw=%04x\n", addr, val);


    return plOK;
}

plerrcode test_proc_mtdc32_status(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=1) {
        complain("mtdc32_status: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (!valid_module(p[1], modul_vme)) {
        complain("mtdc32_status: p[1](==%u): no valid VME module", p[1]);
        return plErr_ArgRange;
    }

    module=ModulEnt(p[1]);
    if (module->modultype!=mesytec_mtdc32) {
        complain("mtdc32_status: p[1](==%u): no mesytec mtdc32", p[1]);
        return plErr_BadModTyp;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mtdc32_status[] = "mtdc32_status";
int ver_proc_mtdc32_status = 1;
/*****************************************************************************/
/*****************************************************************************/
