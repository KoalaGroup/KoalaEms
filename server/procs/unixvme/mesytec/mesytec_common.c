/*
 * procs/unixvme/mesytec/mesytec_common.c
 * created 2011-09-22 PeWue
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: mesytec_common.c,v 1.4 2017/10/20 23:20:52 wuestner Exp $";

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
#include "../../../trigger/trigger.h"
#include "../../procs.h"
#include "../vme_verify.h"
#include "mesytec_common.h"

extern ems_u32* outptr;
extern unsigned int *memberlist;

RCS_REGISTER(cvsid, "procs/unixvme/mesytec")

/*
 * common registers for MADC32 MQDC32 and MTDC32
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

/*
 * This structure is used for the statistics of individual modules and for the
 * instrumentation systems.
 * There are differences:
 * global:
 *     updated    : the time of the last readout of the IS
 *     superevents: counts how often the readout procedure was called
 *     events     : is the number of all events (sum of events of all modules)
 *     words      : number of all words in all superevents
 *     timestamp* : not set
 *     triggers   : not set
 * modules:
 *     updated    : the time of the last readout of this module
 *     superevents: counts how often the readout procedure was called
 *     events     : is the number of events of this module
 *     words      : number of all words of this module
 *     timestamp_high: only with marking type 3
 *     timestamp_low : only with marking type 1 (==time stamp in footer)
 *     triggers      : only with marking type 0 (==event counter in footer)
 */
struct mxdc32_statist {
    u_int64_t channel[32]; /* number of hits for each channel */
    u_int64_t superevents; /* how often scan_events was called */
    u_int64_t events;      /* number of event headers found */
    u_int64_t words;       /* number of data words */
    struct timeval updated;
    u_int32_t timestamp_high;
    u_int32_t timestamp_low;
    u_int32_t triggers;    /* trigger counter in footer */
};

/*
 * This structure is automatically created for each module and a pointer
 * to it is stored in the module description in the global modules list.
 */
struct mxdc32_private {
    ems_u32 module_id;
    ems_u32 marking_type;
    ems_u32 output_format;
    struct mxdc32_statist statist;
    unsigned long long int current_scan_id;
};

static struct mxdc32_statist mxdc32_statist;

/* Yes, this is an evil static variable, but it makes no harm */
static unsigned long long int scan_id=12345; /* must not be equal to the
        initial value of the corresponding member of mxdc32_private;
        if it is equal the number of superevents may be off by 1;
        this is not a real problem */

/*****************************************************************************/
int
is_mesytecvme(ml_entry* module, ems_u32 *modtypes)
{
    ems_u32 *mtype=modtypes;
    if (module->modulclass!=modul_vme)
        return 0;
    while (*mtype) {
        if (module->modultype==*mtype)
            return 1;
        mtype++;
    }
    return 0;
}
/*****************************************************************************/
static struct mxdc32_private*
find_private_by_id(unsigned int id)
{
    ems_u32 mtypes[]={mesytec_madc32, mesytec_mqdc32, mesytec_mtdc32, 0};
    struct mxdc32_private *private;
    ml_entry *module;
    unsigned int i;

    if (memberlist) { /* iterate over memberlist */
        for (i=1; i<=memberlist[0]; i++) {
            module=&modullist->entry[memberlist[i]];
            if (is_mesytecvme(module, mtypes) &&
                module->private_data &&
                module->private_length==sizeof(struct mxdc32_private)) {
                    private=(struct mxdc32_private*)module->private_data;
                    if (private->module_id==id)
                        return private;
            }
        }
    } else {          /* iterate over modullist */
        for (i=0; i<modullist->modnum; i++) {
            module=&modullist->entry[i];
            if (is_mesytecvme(module, mtypes) &&
                module->private_data &&
                module->private_length==sizeof(struct mxdc32_private)) {
                    private=(struct mxdc32_private*)module->private_data;
                    if (private->module_id==id)
                        return private;
            }
        }
    }
    return 0; /* module not found */
}

static plerrcode
mxdc32_clear_statist(ml_entry *module)
{
#if 0
complain("clear_statist called for %p\n", module);
#endif
    memset(&mxdc32_statist, 0, sizeof(struct mxdc32_statist));
    if (module) {
        struct mxdc32_private *private;
        private=(struct mxdc32_private*)module->private_data;
        if (private && module->private_length==sizeof(struct mxdc32_private))
            memset(&private->statist, 0, sizeof(struct mxdc32_statist));
    }
    return plOK;
}

plerrcode
mxdc32_init_private(ml_entry *module, ems_u32 module_id, ems_u32 marking_type,
        ems_u32 output_format)
{
    struct mxdc32_private *private;

    free(module->private_data);
    module->private_data=calloc(1, sizeof(struct mxdc32_private));
    module->private_length=sizeof(struct mxdc32_private);

    private=(struct mxdc32_private*)module->private_data;
    private->module_id=module_id;
    private->marking_type=marking_type;
    private->output_format=output_format;
    return plOK;
}

static void
mxdc32_scan_events(ems_u32 *data, int words)
{
    struct mxdc32_private *private=0;
    struct timeval now;
    int i;

    gettimeofday(&now, 0);
    scan_id++;
    mxdc32_statist.updated=now;
    mxdc32_statist.superevents++;

    DV(D_USER,printf("scan_events(total:%d, residual:%d): words=%d\n",mxdc32_statist.superevents, mxdc32_statist.superevents%34,words);)
    for (i=0; i<words; i++) {
        ems_u32 d=data[i];
        DV(D_USER, if(i==0 && ((d>>30)&3) != 0x1) { printf("Error: first word is not Header\n");} )
        switch ((d>>30)&3) {
        case 0x1: { /* header */
                int id=(d>>16)&0xff;
                int len=d&0xfff;
                private=find_private_by_id(id);
                DV(D_USER,printf("Header: module_id=%d, word_index=%d, remaining_words=%d\n",id, i+1, len); )
                mxdc32_statist.events++;
                mxdc32_statist.words+=len;

                if (private) {
                    if (private->current_scan_id!=scan_id) {
                        private->statist.superevents++;
                        private->current_scan_id=scan_id;
                    }
                    private->statist.updated=now;
                    private->statist.events++;
                    private->statist.words+=len;
                } else {
                  DV(D_USER, printf("mxdc32header %08x: no private module data for module %02x, word_index=%d\n", d, id, i+1); )
                }
            }
            break;
        case 0x0: { /* data, timestamp or dummy */
                switch ((d>>22)&0xff) {
                case    0: /* dummy */
                    DV(D_USER, printf("dummy word, word_index=%d\n",i+1); )
                    break;
                case 0x10: { /* data */
                        int channel=(d>>16)&0x1f;
                        //int value=d&0xffff;
                        if (private) {
                            private->statist.channel[channel]++;
                        }
                        DV(D_USER, printf("data word, channel=%d, word_index=%d\n",channel, i+1); )
                    }
                    break;
                case 0x12: { /* extended time stamp */
                        int stamp=d&0xffff;
                        if (private) {
                            private->statist.timestamp_high=stamp;
                        }
                        DV(D_USER, printf("extended timestamp, word_index=%d\n",i+1); )
                    }
                    break;
                default:
                    printf("mxdc32 data: unexpected word %08x\n", d);
                }
            }
            break;
        case 0x3: { /* end of event */
          DV(D_USER, printf("End of Event, word_index=%d\n", i+1); )
                if (private) {
                    int stamp=d&0x3fffffff;
                    if (private->marking_type==0) { /* event counter */
                        private->statist.triggers=stamp;
                    } else {                        /* time stamp */
                        private->statist.timestamp_low=stamp;
                    }
                    private=0;
                } else {
                  DV(D_USER, printf("mxdc32eoe %08x: no private module data, word_index=%d\n", d,i+1); )
                }
            }
            break;
        default:  { /* illegal */
                printf("mxdc32: illegal word %08x\n", d);
            }
            break;
        }
    }
}
/*****************************************************************************/
/*
 * 'for_each_member' calls 'proc' for each member of the IS modullist or
 * (if the IS modullist does not exist) for each member of the global
 * modullist.
 * p[1] has to be the module index (given as -1). It will be replaced by
 * the actual module idx.
 *
 * Warning: because of the static variable (for_each_member_idx) this
 * function ist not reenrant. But ems server does not use threads...
 */

static int mxdc_member_idx_=-1;
int mxdc_member_idx(void)
{
    return mxdc_member_idx_;
}

plerrcode
for_each_mxdc_member(ems_u32 *p, ems_u32 *modtypes, plerrcode(*proc)(ems_u32*))
{
    ml_entry* module;
    ems_u32 *pc;
    plerrcode pres=plOK;
    int psize;
    unsigned int i;

    if (!modullist)
        return plOK;

    /* create a copy of p */
    psize=4*(p[0]+1);
    pc=malloc(psize);
    if (!pc) {
        complain("mxdc32:for_each: %s", strerror(errno));
        return plErr_NoMem;
    }
    memcpy(pc, p, psize);
    mxdc_member_idx_=-1;

    if (memberlist) { /* iterate over memberlist */
        for (i=1; i<=memberlist[0]; i++) {
            module=&modullist->entry[memberlist[i]];
            if (is_mesytecvme(module, modtypes)) {
                pc[1]=i;
                mxdc_member_idx_++;
                pres=proc(pc);
                if (pres!=plOK)
                    break;
            }
        }
    } else {          /* iterate over modullist */
        for (i=0; i<modullist->modnum; i++) {
            module=&modullist->entry[i];
            if (is_mesytecvme(module, modtypes)) {
                pc[1]=i;
                mxdc_member_idx_++;
                pres=proc(pc);
                if (pres!=plOK)
                    break;
            }
        }
    }

    mxdc_member_idx_=-1;
    free(pc);
    return pres;
}
/*****************************************************************************/
/*
 * 'for_each_module' calls 'proc' for each member of the IS modullist or
 * (if the IS modullist does not exist) for each member of the global
 * modullist.
 */
plerrcode
for_each_mxdc_module(ems_u32 *modtypes, plerrcode(*proc)(ml_entry*))
{
    ml_entry* module;
    plerrcode pres=plOK;
    unsigned int i;

    if (memberlist) { /* iterate over memberlist */
        for (i=1; i<=memberlist[0]; i++) {
            module=&modullist->entry[memberlist[i]];
            if (is_mesytecvme(module, modtypes)) {
                pres=proc(module);
                if (pres!=plOK)
                    break;
            }
        }
    } else {          /* iterate over modullist */
        for (i=0; i<modullist->modnum; i++) {
            module=&modullist->entry[i];
            if (is_mesytecvme(module, modtypes)) {
                pres=proc(module);
                if (pres!=plOK)
                    break;
            }
        }
    }

    return pres;
}
/*****************************************************************************/
int
nr_mxdc_modules(ems_u32 *modtypes)
{
    ml_entry* module;
    unsigned int i;
    int count=0;

    if (memberlist) { /* iterate over memberlist */
        for (i=1; i<=memberlist[0]; i++) {
            module=&modullist->entry[memberlist[i]];
            if (is_mesytecvme(module, modtypes)) {
                count++;
            }
        }
    } else {          /* iterate over modullist */
        for (i=0; i<modullist->modnum; i++) {
            module=&modullist->entry[i];
            if (is_mesytecvme(module, modtypes)) {
                count++;
            }
        }
    }

    return count;
}
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module idx (or -1 for all mxdc32 of this IS)
 * p[2]: offset
 * [p[3]: value (16 bit)]
 */
plerrcode
proc_mxdc32_reg(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, mesytec_mqdc32, mesytec_mtdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int res;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mxdc32_reg);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;

        pres=plOK;

        if (p[0]>2) {
            res=dev->write_a32d16(dev, addr+p[2], p[3]);
            if (res!=2) {
                complain("mxdc32_reg(%04x, %04x): res=%d errno=%s",
                        p[2], p[3],
                        res, strerror(errno));
                return plErr_System;
            }
        } else {
            ems_u16 val;
            res=dev->read_a32d16(dev, addr+p[2], &val);
            if (res!=2) {
                complain("mxdc32_reg(%04x): res=%d errno=%s",
                        p[2],
                        res, strerror(errno));
                return plErr_System;
            }
            *outptr++=val;
        }
    }

    return pres;
}

plerrcode test_proc_mxdc32_reg(ems_u32* p)
{
  ems_u32 mtypes[]={mesytec_madc32, mesytec_mqdc32, mesytec_mtdc32, vme_mcst, vme_cblt, 0};
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=2 && p[0]!=3) {
        complain("mxdc32_reg: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("mxdc32_reg: p[1](==%u): no valid VME module", p[1]);
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

char name_proc_mxdc32_reg[] = "mxdc32_reg";
int ver_proc_mxdc32_reg = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==4
 * p[1]: module idx (or -1 for all mxdc32 of this IS)
 * p[2]: level
 * p[3]: vector
 *         <0: -vector is used for all modules
 *         >=0: vector is used for the first module and incremented for each
 *              following module
 * p[4]: irq threshold (maximimum 956)
 */

plerrcode
proc_mxdc32_irq(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, mesytec_mqdc32, mesytec_mtdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int res, vector;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mxdc32_irq);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;

        pres=plOK;

        /* level */
        res=dev->write_a32d16(dev, addr+0x6010, p[2]);
        if (res!=2) {
            complain("mxdc32_irq: set level: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }

        /* vector */
        if (ip[3]<0) {
            vector=-ip[3];
        } else {
            vector=ip[3];
            if (mxdc_member_idx()>=0)
                vector+=mxdc_member_idx();
        }
        res=dev->write_a32d16(dev, addr+0x6012, vector);
        if (res!=2) {
            complain("mxdc32_irq: set vector: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }

        /* irq threshold */
        res=dev->write_a32d16(dev, addr+0x6018, p[4]);
        if (res!=2) {
            complain("mxdc32_irq: set irq threshold: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }
 
 #if 0
 /*TEST*/
        /* don't withdraw IRQ */
        res=dev->write_a32d16(dev, addr+0x601c, 0);
        if (res!=2) {
            complain("mxdc32_irq: disable withdraw IRQ: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }
#endif
    }

    return pres;
}

plerrcode test_proc_mxdc32_irq(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, mesytec_mqdc32, mesytec_mtdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=4) {
        complain("mxdc32_irq: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("mxdc32_irq: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (!is_mesytecvme(module, mtypes)) {
            complain("mxdc32_irq: p[1](==%u): no mesytec mxdc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mxdc32_irq[] = "mxdc32_irq";
int ver_proc_mxdc32_irq = 1;
/*****************************************************************************/
/* this procedure is valid for madc32 and mqdc32
 * p[0]: argcount==3 or 33
 * p[1]: module idx (or -1 for all madc32 and mqdc32 of this IS)
 *
 * A (with p[0]==3):
 * p[2]: channel (-1 for all channels)
 * p[3]: threshold (0x1FFF to disable the channel)
 *
 * B (with p[0]==33)
 * p[2..33]: threshold[0..31]
 */

plerrcode
proc_mxdc32_threshold(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, mesytec_mqdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int res, i;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mxdc32_threshold);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;

        pres=plOK;

        if (p[0]==33) {       /* 32 individual channels */
             for (i=0; i<32; i++) {
                res=dev->write_a32d16(dev, addr+0x4000+2*i, p[i+2]);
                if (res!=2) {
                    complain("mxdc32_threshold: res=%d errno=%s",
                            res, strerror(errno));
                    return plErr_System;
                }
           }
       } else if (ip[2]<0) { /* argcount=3 and all channels */
            for (i=0; i<32; i++) {
                res=dev->write_a32d16(dev, addr+0x4000+2*i, p[3]);
                if (res!=2) {
                    complain("mxdc32_threshold: res=%d errno=%s",
                            res, strerror(errno));
                    return plErr_System;
                }
           }
        } else {              /* argcount=3 and one channel */
            res=dev->write_a32d16(dev, addr+0x4000+2*p[2], p[3]);
            if (res!=2) {
                complain("mxdc32_threshold: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
        }
    }

    return pres;
}

plerrcode test_proc_mxdc32_threshold(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=3 && p[0]!=33) {
        complain("mxdc32_threshold: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("mxdc32_threshold: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (module->modultype!=mesytec_madc32 &&
            module->modultype!=mesytec_mqdc32) {
            complain("mxdc32_threshold: p[1](==%u): no mesytec m(aq)dc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mxdc32_threshold[] = "mxdc32_threshold";
int ver_proc_mxdc32_threshold = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: module idx (or -1 for all mxdc32 of this IS)
 * p[2]: val
 */

plerrcode
proc_mxdc32_nimbusy(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, mesytec_mqdc32, mesytec_mtdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int res;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mxdc32_nimbusy);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;

        pres=plOK;

        res=dev->write_a32d16(dev, addr+0x606e, p[2]);
        if (res!=2) {
            complain("mxdc32_nimbusy: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }
    }

    return pres;
}

plerrcode test_proc_mxdc32_nimbusy(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, mesytec_mqdc32, mesytec_mtdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=2) {
        complain("mxdc32_nimbusy: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("mxdc32_nimbusy: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (!is_mesytecvme(module, mtypes)) {
            complain("mxdc32_nimbusy: p[1](==%u): no mesytec mxdc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mxdc32_nimbusy[] = "mxdc32_nimbusy";
int ver_proc_mxdc32_nimbusy = 1;
/*****************************************************************************/
/*
 * mxdc32_init_cblt enables (or disables) multicast and chained block readout
 * It operates over all mxdc32 modules of the instrumentation system.
 * It requires that all modules are in the same crate.
 * 
 * p[0]: argcount (0..2)
 * [p[1]: mcst_module   if not given: multicast disabled
 * [p[2]: cblt_module]] if not given: cblt disabled
 */
plerrcode
proc_mxdc32_init_cblt(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, mesytec_mqdc32, mesytec_mtdc32, 0};
    ml_entry *mcst_mod=0;
    ml_entry *cblt_mod=0;
    ml_entry *module_first=0;
    ml_entry *module_last=0;
    int idx_start, idx_end, i;
    ems_u16 cblt_mcst_control=0;

    if (p[0]>0) { /* enable mcst */
        cblt_mcst_control|=1<<7;
        mcst_mod=ModulEnt(p[1]);
    } else {
        cblt_mcst_control|=1<<6;
    }

    if (p[0]>1) { /* enable cblt */
        cblt_mcst_control|=1<<1;
        cblt_mod=ModulEnt(p[2]);
    } else {
        cblt_mcst_control|=1<<0;
    }

    if (memberlist) {
        idx_start=1;
        idx_end=memberlist[0];
    } else {
        idx_start=0;
        idx_end=modullist->modnum-1;
    }

    printf("idx_start=%d idx_end=%d\n", idx_start, idx_end);
    for (i=idx_start; i<=idx_end; i++) {
        ml_entry *module=ModulEnt(i);
        struct vme_dev *dev;
        ems_u32 addr;
        int res;

        if (!is_mesytecvme(module, mtypes)) {
            printf("module %d skipped\n", i);
            continue;
        }

        dev=module->address.vme.dev;
        addr=module->address.vme.addr;

        if (module_first==0 || addr<module_first->address.vme.addr)
            module_first=module;
        if (module_last==0 || addr>module_last->address.vme.addr)
            module_last=module;
        printf("actual module=%p addr=%08x\n", module, addr);
        printf("module_first=%p module_last=%p\n", module_first, module_last);

        /* write multicast address */
        if (mcst_mod) {
            res=dev->write_a32d16(dev, addr+0x6024,
                    mcst_mod->address.vme.addr>>24);
            if (res!=2) {
                complain("mxdc32_cblt: mcst_address: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
        }

        /* write cblt address */
        if (cblt_mod) {
            res=dev->write_a32d16(dev, addr+0x6022,
                    cblt_mod->address.vme.addr>>24);
            if (res!=2) {
                complain("mxdc32_cblt: cblt_address: res=%d errno=%s",
                        res, strerror(errno));
                return plErr_System;
            }
        }

        /* disable all mcst and cblt features */
        res=dev->write_a32d16(dev, addr+0x6020, 0x55);
        if (res!=2) {
            complain("mxdc32_cblt: cblt_control: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }
        /* enable requested features */
        res=dev->write_a32d16(dev, addr+0x6020, cblt_mcst_control);
        if (res!=2) {
            complain("mxdc32_cblt: cblt_control: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }
    }

    /* set first and last cblt module */
    if (p[0]>1) {
        /* sanity check */
        if (!module_first || !module_last) {
            complain("mxdc32_cblt: no modules");
            return plErr_BadISModList;
        }
        /* no error check here, because we already wrote to the register */
        module_first->address.vme.dev->write_a32d16(
                module_first->address.vme.dev,
                module_first->address.vme.addr+0x6020, 1<<5);
        module_last->address.vme.dev->write_a32d16(
                module_last->address.vme.dev,
                module_last->address.vme.addr+0x6020, 1<<3);
    }

    return plOK;
}

plerrcode
test_proc_mxdc32_init_cblt(ems_u32* p)
{
    ml_entry *module;
    if (p[0]>2) {
        complain(": illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (p[0]>0) { /* mcst address given */
        if (!valid_module(p[1], modul_vme)) {
            complain("mxdc32_init_cblt: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (module->modultype!=vme_mcst) {
            complain("mxdc32_init_cblt: p[1](==%u): no mcst module", p[1]);
            return plErr_BadModTyp;
        }
    }

    if (p[0]>1) { /* cblt address given */
        if (!valid_module(p[2], modul_vme)) {
            complain("mxdc32_init_cblt: p[2](==%u): no valid VME module", p[2]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[2]);
        if (module->modultype!=vme_cblt) {
            complain("mxdc32_init_cblt: p[2](==%u): no cblt module", p[2]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mxdc32_init_cblt[] = "mxdc32_init_cblt";
int ver_proc_mxdc32_init_cblt = 1;
/*****************************************************************************/
static plerrcode
read_time_counters(ml_entry *module)
{
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;
        int res;
        ems_u16 c_lo, c_hi, id;

        res=dev->read_a32d16(dev, addr+0x609c, &c_lo);
        if (res!=2) {
            complain("mxdc32_start: read counter lo: res=%d errno=%s",
                res, strerror(errno));
            return plErr_System;
        }
        res=dev->read_a32d16(dev, addr+0x609e, &c_hi);
        if (res!=2) {
            complain("mxdc32_start: read counter hi: res=%d errno=%s",
                res, strerror(errno));
            return plErr_System;
        }
        res=dev->read_a32d16(dev, addr+0x6004, &id);
        if (res!=2) {
            complain("mxdc32_start: read module_id: res=%d errno=%s",
                res, strerror(errno));
            return plErr_System;
        }
        printf("module %d: ts_lo=%04x ts_hi=%04x\n", id, c_lo, c_hi);
        return plOK;
}
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module idx
 * p[2]: multievent
 * p[3]: max_transfer_data
 */
plerrcode
proc_mxdc32_start_simple(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, mesytec_mqdc32, mesytec_mtdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    /* yes, this will be called multiple times, this is not harmfull */
    mxdc32_clear_statist(0);

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mxdc32_start_simple);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;
        int res;

        pres=plOK;
        mxdc32_clear_statist(module);

        /* set multievent */
        res=dev->write_a32d16(dev, addr+0x6036, p[2]);
        if (res!=2) {
            complain("mxdc32_start_simple: set multievent: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }

        /* set max_transfer_data (956 is maximum) */
        res=dev->write_a32d16(dev, addr+0x601a, p[3]);
        if (res!=2) {
            complain("mxdc32_start_simple: set multievent: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }

        /* FIFO reset */
        res=dev->write_a32d16(dev, addr+0x603c, ANY);
        if (res!=2) {
            complain("mxdc32_start_simple: reset FIFO: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }
 
        /* reset counters */
        /* step 1: stop counters */
        res=dev->write_a32d16(dev, addr+0x60ae, 3);
        if (res!=2) {
          complain("mxdc32_start_simple: stop counters: res=%d errno=%s",
                   res, strerror(errno));
          return plErr_System;
        }
        /* step 2: reset counters */
        /* TODO: b1100 single shot reset ? */
        res=dev->write_a32d16(dev, addr+0x6090, 3);
        if (res!=2) {
            complain("mxdc32_start_simple: reset counters: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }
        usleep(10);
        /* step 3: start counters */
        res=dev->write_a32d16(dev, addr+0x60ae, 0);
        if (res!=2) {
          complain("mxdc32_start_simple: start counters: res=%d errno=%s",
                   res, strerror(errno));
          return plErr_System;
        }


        /* start acq */
        res=dev->write_a32d16(dev, addr+0x603a, 1);
        if (res!=2) {
            complain("mxdc32_start_simple: start acq: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }

  	/* readout reset */
        res=dev->write_a32d16(dev, addr+0x6034, ANY);
        if (res!=2) {
          complain("mxdc32_start_simple: readout reset: res=%d errno=%s",
                   res, strerror(errno));
          return plErr_System;
        }
    }

    return pres;
}

plerrcode test_proc_mxdc32_start_simple(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, mesytec_mqdc32, mesytec_mtdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=3) {
        complain("mxdc32_start_simple: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("mxdc32_start_simple: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (!is_mesytecvme(module, mtypes)) {
            complain("mxdc32_start_simple: p[1](==%u): no mesytec mxdc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mxdc32_start_simple[] = "mxdc32_start_simple";
int ver_proc_mxdc32_start_simple = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: mcst_module
 * p[2]: multievent (0: single, 3: multi (2 not usable))
 * p[3]: max_transfer_data
 */
plerrcode
proc_mxdc32_start_cblt(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, mesytec_mqdc32, mesytec_mtdc32, 0};
    ml_entry* mcst_module=ModulEnt(p[1]);
    struct vme_dev* dev=mcst_module->address.vme.dev;
    ems_u32 maddr=mcst_module->address.vme.addr;
    int res;

    mxdc32_clear_statist(0);
    for_each_mxdc_module(mtypes, mxdc32_clear_statist);

    /* set multievent */
    res=dev->write_a32d16(dev, maddr+0x6036, p[2]);
    if (res!=2) {
        complain("mxdc32_start_cblt: set multievent: res=%d errno=%s",
                res, strerror(errno));
        return plErr_System;
    }

    /*
     * set max_transfer_data (956 is maximum)
     * 0 (infinite) is also possible, but dangerous (transfer may never end)
     */
    res=dev->write_a32d16(dev, maddr+0x601a, p[3]);
    if (res!=2) {
        complain("mxdc32_start_cblt: set max_transfer_data: res=%d errno=%s",
                res, strerror(errno));
        return plErr_System;
    }

    /* FIFO reset */
    res=dev->write_a32d16(dev, maddr+0x603c, ANY);
    if (res!=2) {
        complain("mxdc32_start_cblt: reset FIFO: res=%d errno=%s",
                res, strerror(errno));
        return plErr_System;
    }

    /* reset counters */
    /* step 1: stop counters */
    res=dev->write_a32d16(dev, maddr+0x60ae, 3);
    if (res!=2) {
      complain("mxdc32_start_cblt: stop counters: res=%d errno=%s",
               res, strerror(errno));
      return plErr_System;
    }
    /* step 2: reset counters */
    /* TODO: b1100 single shot reset ? */
    res=dev->write_a32d16(dev, maddr+0x6090, 3);
    if (res!=2) {
        complain("mxdc32_start_cblt: reset counters: res=%d errno=%s",
                res, strerror(errno));
        return plErr_System;
    }
    usleep(10);
    /* step 3: start counters */
    res=dev->write_a32d16(dev, maddr+0x60ae, 0);
    if (res!=2) {
      complain("mxdc32_start_cblt: start counters: res=%d errno=%s",
               res, strerror(errno));
      return plErr_System;
    }

#if 0
    /* step 4: read counters (possible error ignored) */
    res=dev->write_a32d16(dev, maddr+0x60ae, 3);
    if (res!=2) {
      complain("mxdc32_start_cblt: stop counters: res=%d errno=%s",
               res, strerror(errno));
      return plErr_System;
    }
    for_each_mxdc_module(mtypes, read_time_counters);
    res=dev->write_a32d16(dev, maddr+0x60ae, 0);
    if (res!=2) {
      complain("mxdc32_start_cblt: start counters: res=%d errno=%s",
               res, strerror(errno));
      return plErr_System;
    }
#endif

    /* start acq */
    res=dev->write_a32d16(dev, maddr+0x603a, 1);
    if (res!=2) {
        complain("mxdc32_start_cblt: start acq: res=%d errno=%s",
                res, strerror(errno));
        return plErr_System;
    }

    /* readout reset */
    res=dev->write_a32d16(dev, maddr+0x6034, ANY);
    if (res!=2) {
      complain("mxdc32_start_cblt: reset FIFO: res=%d errno=%s",
               res, strerror(errno));
      return plErr_System;
    }

    return plOK;
}

plerrcode test_proc_mxdc32_start_cblt(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=3) {
        complain("mxdc32_start_cblt: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (!valid_module(p[1], modul_vme)) {
        complain("mxdc32_start_cblt: p[1](==%u): no valid VME module", p[1]);
        return plErr_ArgRange;
    }

    module=ModulEnt(p[1]);
    if (module->modultype!=vme_mcst) {
        complain("mxdc32_start_cblt: p[1](==%u): no mcst module", p[1]);
        return plErr_BadModTyp;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mxdc32_start_cblt[] = "mxdc32_start_cblt";
int ver_proc_mxdc32_start_cblt = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: mcst_module
 */
plerrcode
proc_mxdc32_stop_cblt(ems_u32* p)
{
    ml_entry* mcst_module=ModulEnt(p[1]);
    struct vme_dev* dev=mcst_module->address.vme.dev;
    ems_u32 maddr=mcst_module->address.vme.addr;
    int res;

    /* stop acq */
    res=dev->write_a32d16(dev, maddr+0x603a, 0);
    if (res!=2) {
        complain("mxdc32_stop_cblt: res=%d errno=%s",
                res, strerror(errno));
        return plErr_System;
    }

    return plOK;
}

plerrcode test_proc_mxdc32_stop_cblt(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=1) {
        complain("mxdc32_stop_cblt: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (!valid_module(p[1], modul_vme)) {
        complain("mxdc32_stop_cblt: p[1](==%u): no valid VME module", p[1]);
        return plErr_ArgRange;
    }

    module=ModulEnt(p[1]);
    if (module->modultype!=vme_mcst) {
        complain("mxdc32_stop_cblt: p[1](==%u): no mcst module", p[1]);
        return plErr_BadModTyp;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mxdc32_stop_cblt[] = "mxdc32_stop_cblt";
int ver_proc_mxdc32_stop_cblt = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: module idx
 */
plerrcode
proc_mxdc32_stop_simple(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, mesytec_mqdc32, mesytec_mtdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mxdc32_stop_simple);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;
        int res;

        pres=plOK;

        res=dev->write_a32d16(dev, addr+0x603a, 0);
        if (res!=2) {
            complain("stop_simple: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }
    }

    return pres;
}

plerrcode test_proc_mxdc32_stop_simple(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, mesytec_mqdc32, mesytec_mtdc32, 0};
    ems_i32 *ip=(ems_i32*)p;

    if (p[0]!=1) {
        complain("stop_simple: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        ml_entry* module;
        if (!valid_module(p[1], modul_vme)) {
            complain("mxdc32_stop_simple: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (!is_mesytecvme(module, mtypes)) {
            complain("mxdc32_stop_simple: p[1](==%u): no mesytec mxdc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_mxdc32_stop_simple[] = "mxdc32_stop_simple";
int ver_proc_mxdc32_stop_simple = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: module idx
 * p[2]: max_words (per module)
 */
plerrcode
proc_mxdc32_read_simple(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, mesytec_mqdc32, mesytec_mtdc32, 0};
    ems_u32 *oldoutptr=outptr;
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_mxdc_member(p, mtypes, proc_mxdc32_read_simple);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;
        /*ems_u16 val;*/
        int res;

        pres=plOK;

        /* read data */
        res=dev->read(dev, addr+0, 0xb, outptr, /*4*val*/4*p[2], 4, &outptr);
        if (res<0) {
            complain("mxdc32_read_simple: read: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }

        /* clear busy */
        res=dev->write_a32d16(dev, addr+0x6034, 0);
        if (res!=2) {
            complain("mxdc32_read_simple: clear busy: res=%d errno=%s",
                    res, strerror(errno));
            return plErr_System;
        }
        mxdc32_scan_events(oldoutptr, outptr-oldoutptr);
        DV(D_USER, printf("read_simple: stop\n"); )
    }

    return pres;
}

plerrcode test_proc_mxdc32_read_simple(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, mesytec_mqdc32, mesytec_mtdc32, 0};
    ems_i32 *ip=(ems_i32*)p;
    ml_entry* module;

    if (p[0]!=2) {
        complain("mxdc32_read_simple: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[1]>=0) {
        if (!valid_module(p[1], modul_vme)) {
            complain("mxdc32_read_simple: p[1](==%u): no valid VME module", p[1]);
            return plErr_ArgRange;
        }
        module=ModulEnt(p[1]);
        if (!is_mesytecvme(module, mtypes)) {
            complain("mxdc32_read_simple: p[1](==%u): no mesytec mxdc32", p[1]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=(ip[1]>=0?1:nr_mxdc_modules(mtypes))*p[2];
    return plOK;
}

char name_proc_mxdc32_read_simple[] = "mxdc32_read_simple";
int ver_proc_mxdc32_read_simple = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: mcst_module
 * p[2]: max_words (per module)
 */
plerrcode
proc_mxdc32_read_mcst(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, mesytec_mqdc32, mesytec_mtdc32, 0};
    ml_entry *module, *mcst_module=ModulEnt(p[1]);
    struct vme_dev *dev, *mdev=mcst_module->address.vme.dev;
    ems_u32 addr, maddr=mcst_module->address.vme.addr;
    ems_u32 *oldoutptr=outptr;
    unsigned int i;
    int res;

//printf("mxdc32_read_mcst\n");

    DV(D_USER, printf("\nread_mcst: start\n"); )
    if (memberlist) { /* iterate over memberlist */
        for (i=1; i<=memberlist[0]; i++) {
            module=&modullist->entry[memberlist[i]];
//printf("member %d %08x\n", i, module->modultype);
            if (is_mesytecvme(module, mtypes)) {
                dev=module->address.vme.dev;
                addr=module->address.vme.addr;
                res=dev->read(dev, addr, 0xb, outptr, 4*p[2], 4, &outptr);
                if (res<0) {
                    complain("mxdc32_read_mcst: %s", strerror(errno));
                    return plErr_System;
                }
            }
        }
    } else {          /* iterate over modullist */
        for (i=0; i<modullist->modnum; i++) {
            module=&modullist->entry[i];
            if (is_mesytecvme(module, mtypes)) {
                dev=module->address.vme.dev;
                addr=module->address.vme.addr;
                res=dev->read(dev, addr, 0xb, outptr, 4*p[2], 4, &outptr);
                if (res<0) {
                    complain("mxdc32_read_mcst: %s", strerror(errno));
                    return plErr_System;
                }
            }
        }
    }

    /* clear busy */
    res=mdev->write_a32d16(dev, maddr+0x6034, 0);
    if (res!=2) {
        complain("mxdc32_read_mcst: clear busy: res=%d errno=%s",
                res, strerror(errno));
        return plErr_System;
    }

    mxdc32_scan_events(oldoutptr, outptr-oldoutptr);
    DV(D_USER, printf("read_mcst: stop\n"); )

    return plOK;
}

plerrcode test_proc_mxdc32_read_mcst(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, mesytec_mqdc32, mesytec_mtdc32, 0};
    ml_entry* module;

    if (p[0]!=2) {
        complain("mxdc32_read_mcst: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (!valid_module(p[1], modul_vme)) {
        complain("mxdc32_read_mcst: p[1](==%u): no valid VME module", p[1]);
        return plErr_ArgRange;
    }

    module=ModulEnt(p[1]);
    if (module->modultype!=vme_mcst) {
        complain("mxdc32_read_mcst: p[1](==%u): no mcst module", p[1]);
        return plErr_BadModTyp;
    }

    wirbrauchen=nr_mxdc_modules(mtypes)*p[2];
    return plOK;
}

char name_proc_mxdc32_read_mcst[] = "mxdc32_read_mcst";
int ver_proc_mxdc32_read_mcst = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: mcst_module
 * p[2]: cblt_module
 * p[3]: max words (should be equal to max_transfer_data in
 *       mxdc32_start_cblt times number of modules)
 */

plerrcode
proc_mxdc32_read_cblt(ems_u32* p)
{
  DV(D_USER, printf("\nreab_cblt start:\n"); )
    ml_entry* mcst_module=ModulEnt(p[1]);
    ml_entry* cblt_module=ModulEnt(p[2]);
    /* dev of mcst and cblt should be identical */
    struct vme_dev* dev=cblt_module->address.vme.dev;
    ems_u32 maddr=mcst_module->address.vme.addr;
    ems_u32 caddr=cblt_module->address.vme.addr;
    ems_u32 *oldoutptr=outptr;
    int res;

    /* read data */
    res=dev->read(dev, caddr+0, 0xb, outptr, 4*p[3], 4, &outptr);
    //printf("data read: %d\n", res/4);

    /* clear busy */
    res=dev->write_a32d16(dev, maddr+0x6034, 0);
    if (res!=2) {
        complain("mxdc32_read_cblt: clear busy: res=%d errno=%s",
                res, strerror(errno));
        return plErr_System;
    }

    mxdc32_scan_events(oldoutptr, outptr-oldoutptr);
    DV(D_USER, printf("reab_cblt end:\n"); )

    return plOK;
}

plerrcode test_proc_mxdc32_read_cblt(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=3) {
        complain("mxdc32_read_cblt: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (!valid_module(p[1], modul_vme)) {
        complain("mxdc32_read_cblt: p[1](==%u): no valid VME module", p[1]);
        return plErr_ArgRange;
    }
    module=ModulEnt(p[1]);
    if (module->modultype!=vme_mcst) {
        complain("mxdc32_read_cblt: p[1](==%u): no mcst module", p[1]);
        return plErr_BadModTyp;
    }

    if (!valid_module(p[2], modul_vme)) {
        complain("mxdc32_read_cblt: p[2](==%u): no valid VME module", p[2]);
        return plErr_ArgRange;
    }
    module=ModulEnt(p[2]);
    if (module->modultype!=vme_cblt) {
        complain("mxdc32_read_cblt: p[2](==%u): no cblt module", p[2]);
        return plErr_BadModTyp;
    }

    wirbrauchen=p[3];
    return plOK;
}

char name_proc_mxdc32_read_cblt[] = "mxdc32_read_cblt";
int ver_proc_mxdc32_read_cblt = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: module idx (0: global statistics only
 *                   1: global and all modules)
 * 
 * outptr (repeated for all modules)
 *   [0]: number of following words for this module or global statistics
 *   [1[: module ID (-1 for global statistics)
 *   [2]: updated: tv.tv_sec
 *   [3]: updated: tv.tv_usec
 *   [4..5]: scanned 
 *   [4]: events (64 bit)
 *   [5]: words (64 bit)
 *   [6]: timestamp: high
 *   [7]: timestamp: low
 *   [8]: triggers
 */

static plerrcode
mxdc32_put_statist(ml_entry* module) {
    struct mxdc32_statist *statist=0;
    ems_u32 *help;
    int id;

    if (module) {
        struct mxdc32_private *private;
        private=(struct mxdc32_private*)module->private_data;
        if (private && module->private_length==sizeof(struct mxdc32_private)) {
            statist=&private->statist;
            id=private->module_id;
        }
    } else {
        statist=&mxdc32_statist;
        id=-1;
    }

#if 0
    if (!statist) {
        complain("mxdc32: no statistics for module %08x found",
                module->address.vme.addr);
        return plErr_Program;
    }
#else
    if (!statist) {
        complain("mxdc32: no statistics for module %08x found",
                module->address.vme.addr);
        return plOK;
    }
#endif

    help=outptr++;
    *outptr++=id;
    *outptr++=statist->updated.tv_sec;
    *outptr++=statist->updated.tv_usec;
    *outptr++=statist->superevents>>32;
    *outptr++=statist->superevents&0xffffffff;
    *outptr++=statist->events>>32;
    *outptr++=statist->events&0xffffffff;
    *outptr++=statist->words>>32;
    *outptr++=statist->words&0xffffffff;
    *outptr++=statist->timestamp_high;
    *outptr++=statist->timestamp_low;
    *outptr++=statist->triggers;
    *help=outptr-help-1;

    return plOK;
}

plerrcode
proc_mxdc32_statist(ems_u32* p)
{
    ems_u32 mtypes[]={mesytec_madc32, mesytec_mqdc32, mesytec_mtdc32, 0};
    mxdc32_put_statist(0);
    if (p[1]>0) {
        for_each_mxdc_module(mtypes, mxdc32_put_statist);
    }
    return plOK;
}

plerrcode test_proc_mxdc32_statist(ems_u32* p)
{
    if (p[0]!=1) {
        complain("mxdc32_evnr: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    wirbrauchen=-1;
    return plOK;
}

char name_proc_mxdc32_statist[] = "mxdc32_statist";
int ver_proc_mxdc32_statist = 1;
/*****************************************************************************/
/*****************************************************************************/
