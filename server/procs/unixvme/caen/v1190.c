/*
 * procs/unixvme/caen/v1190.c
 * created 15.Jun.2004 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: v1190.c,v 1.4 2012/09/10 22:55:16 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../commu/commu.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/unixvme/vme.h"
#include "../../../lowlevel/devices.h"
#include "../vme_verify.h"

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

RCS_REGISTER(cvsid, "procs/unixvme/caen")

/*
 * V1190 128-channel 19-bit Multihit TDC
 * A24/32D16 (registers)
 * A24/32D32 (output buffer, event counter, testreg, event fifo, dummy32)
 * reserves 65536 Byte
 */
/* registers
 * 0x000..0xFFC: output buffer (D32)
 * 0x1000: control register
 * 0x1002: status register
 * 0x1004: ADER_32     (VX1190* only)
 * 0x1006: ADER_24     (VX1190* only)
 * 0x1008: enable ADER (VX1190* only)
 * 0x100A: interrupt level
 * 0x100C: interrupt vector
 * 0x100E: geo address
 * 0x1010: MCST/CBLT base address
 * 0x1012: MCST/CBLT control
 * 0x1014: module reset
 * 0x1016: software clear
 * 0x1018: software event reset
 * 0x101A: software trigger
 * 0x101C: event counter (D32)
 * 0x1020: event stored
 * 0x1022: almost full level
 * 0x1024: BLT event number
 * 0x1026: firmware revision
 * 0x1028: testreg (D32)
 * 0x102C: output prog control
 * 0x102E: Micro
 * 0x1030: Micro handshake
 * 0x1032: select flash
 * 0x1034: flash memory
 * 0x1036: Sram page
 * 0x1038: event FIFO (D32)
 * 0x103C: event FIFO stored
 * 0x103E: event FIFO status
 * 0x1200..0x41FE: configuration ROM
 * 0x8000..0x81FE: compensation Sram
 */
/* micro controller opcodes
 * 0x00xx TRG_MATCH
 * 0x01xx CONT_STOR
 * 0x02xx READ_ACQ_MOD
 */
#if 0
#define V1190_OC_TRG_MATCH      0x0000
#define V1190_OC_CONT_STOR      0x0100
#define V1190_OC_LOAD_DEF_CFG   0x0500
#define V1190_OC_LOAD_USER_CFG  0x0700

#define V1190_OC_WIN_WIDTH      0x1000
#define V1190_OC_WIN_OFFS       0x1100
#define V1190_OC_SW_MARGIN      0x1200
#define V1190_OC_REJ_MARGIN     0x1300
#define V1190_OC_EN_SUB_TRG     0x1400
#define V1190_OC_DIS_SUB_TRG    0x1400

#define V1190_OC_EDGE_MODE      0x2200
#define V1190_OC_EDGE_RES       0x2400

#define V1190_OC_EN_HEAD        0x3000
#define V1190_OC_DIS_HEAD       0x3100

#define V1190_MASK_32           0x1f
#define V1190_OC_EN_CHAN        0x4000
#define V1190_OC_DIS_CHAN       0x4100
#endif

/* importand Registers */
#define V1190_MicroReg       0x102E
#define V1190_MicroHandshake 0x1030
/* flags in V1190_MicroHandshake */
#define V1190_WRITE_OK       0x1
#define V1190_READ_OK        0x2

/*****************************************************************************/
static plerrcode
waitMC(ml_entry* module, u_int16_t flags)
{
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
    struct timeval tv0, tv1;
    u_int16_t val;
    int res, diff;
    const int timeout=2000; /* in terms of ms */

    gettimeofday(&tv0, 0);
    do {
        gettimeofday(&tv1, 0);
        diff=(tv1.tv_sec-tv0.tv_sec)*1000+
             (tv1.tv_usec-tv0.tv_usec)/1000;
        if ((res=dev->read_a32d16(dev, addr+V1190_MicroHandshake, &val))!=2)
            return plErr_System;
        if ((val&flags)==flags) {
#if 0
            printf("diff=%d\n", diff);
#endif
            return plOK;
        }
    } while (diff<timeout);

    complain("CAEN V1190: timeout waiting for %d in waitMC", flags);
    return plErr_HW;
}
/*****************************************************************************/
static plerrcode
wMC(ml_entry* module, u_int32_t code, u_int32_t *data, int num)
{
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
    plerrcode pres;
    int i;

    if ((pres=waitMC(module, V1190_WRITE_OK))!=plOK)
        return pres;
    if (dev->write_a32d16(dev, addr+V1190_MicroReg, code)!=2)
        return plErr_System;
    for (i=0; i<num; i++) {
        if ((pres=waitMC(module, V1190_WRITE_OK))!=plOK)
            return pres;
        if (dev->write_a32d16(dev, addr+V1190_MicroReg, data[i])!=2)
            return plErr_System;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
rMC(ml_entry* module, u_int32_t code, u_int32_t *data, int num)
{
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
    plerrcode pres;
    int i;

    if ((pres=waitMC(module, V1190_WRITE_OK))!=plOK)
        return pres;
    if (dev->write_a32d16(dev, addr+V1190_MicroReg, code)!=2)
        return plErr_System;
    for (i=0; i<num; i++) {
        u_int16_t val;
        if ((pres=waitMC(module, V1190_READ_OK))!=plOK)
            return pres;
        if (dev->read_a32d16(dev, addr+V1190_MicroReg, &val)!=2)
            return plErr_System;
        data[i]=val;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
test_mod_type(ems_u32 idx)
{
    ems_u32 mtypes[]={CAEN_V1190, 0};
    ml_entry* module;

    if (memberlist) {
        if (idx>memberlist[0])
            return plErr_ArgRange;
        idx=memberlist[idx];
    }
    if (!modullist)
        return plErr_NoDomain;
    if (idx>=modullist->modnum)
        return plErr_BadISModList;

    module=modullist->entry+idx;
    return test_proc_vmemodule(module, mtypes);
}
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: module idx
 */
plerrcode proc_v1190reset(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
    int res;

    res=dev->write_a32d16(dev, addr+0x1014, 0);
    if (res!=2)
        return plErr_System;

    return plOK;
}

plerrcode test_proc_v1190reset(ems_u32* p)
{
    plerrcode res;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((res=test_mod_type(p[1]))!=plOK)
        return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v1190reset[] = "v1190reset";
int ver_proc_v1190reset = 1;
/*****************************************************************************/
/*
 * p[0]: argcount: 1|2
 * p[1]: module idx
 * [p[2]: control word]
 */
plerrcode proc_v1190control(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
    ems_u16 val;
    int res;

    res=dev->read_a32d16(dev, addr+0x1000, &val);
    if (res!=2)
        return plErr_System;
    *outptr++=val;
    if (p[0]>1) {
        res=dev->write_a32d16(dev, addr+0x1000, p[2]);
        if (res!=2)
            return plErr_System;
    }

    return plOK;
}

plerrcode test_proc_v1190control(ems_u32* p)
{
    plerrcode res;

    if (p[0]!=1 && p[0]!=2)
        return plErr_ArgNum;
    if ((res=test_mod_type(p[1]))!=plOK)
        return res;

    wirbrauchen=p[0]==1;
    return plOK;
}

char name_proc_v1190control[] = "v1190control";
int ver_proc_v1190control = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: module idx
 */
plerrcode proc_v1190status(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
    ems_u16 val;
    int res;

    res=dev->read_a32d16(dev, addr+0x1002, &val);
    if (res!=2)
        return plErr_System;
    *outptr++=val;

    return plOK;
}

plerrcode test_proc_v1190status(ems_u32* p)
{
    plerrcode res;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((res=test_mod_type(p[1]))!=plOK)
        return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v1190status[] = "v1190status";
int ver_proc_v1190status = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (2..4)
 * p[1]: module idx
 * p[2]: level (level==0 disables IRQ)
 * [p[3]: vector
 * [p[4]: almost full level (default 64, min. 1, max. 32735)]]
 */
plerrcode proc_v1190irq(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
    int res;

    res=dev->write_a32d16(dev, addr+0x100A, p[2]); /* level */
    if (res!=2)
        return plErr_System;

    if (p[0]>2) {
        res=dev->write_a32d16(dev, addr+0x100C, p[3]); /* vector */
        if (res!=2)
            return plErr_System;
    }
    if (p[0]>3) {
        res=dev->write_a32d16(dev, addr+0x1022, p[4]); /* almost full level */
        if (res!=2)
            return plErr_System;
    }
        
    return plOK;
}

plerrcode test_proc_v1190irq(ems_u32* p)
{
    plerrcode res;

    if (p[0]<2 || p[0]>4)
        return plErr_ArgNum;
    if ((res=test_mod_type(p[1]))!=plOK)
        return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v1190irq[] = "v1190irq";
int ver_proc_v1190irq = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1|2
 * p[1]: module idx
 * [p[2]: 0: continuous storage mode 1: trigger matching mode]
 */
plerrcode proc_v1190mode(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    plerrcode pres;

    if (p[0]>1) { /* write mode */
        pres=wMC(module, p[2]?0x0000:0x0100, 0, 0);
    } else {      /* read mode */
        pres=rMC(module, 0x0200, outptr, 1);
        if (pres==plOK)
            outptr++;
    }

    return pres;
}

plerrcode test_proc_v1190mode(ems_u32* p)
{
    plerrcode res;

    if (p[0]!=1 && p[0]!=2)
        return plErr_ArgNum;
    if ((res=test_mod_type(p[1]))!=plOK)
        return res;

    wirbrauchen=p[0]>1?0:1;
    return plOK;
}

char name_proc_v1190mode[] = "v1190mode";
int ver_proc_v1190mode = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1|2
 * p[1]: module idx
 * [p[2]]: 0: pairs, 1: trailing, 2: leading, 3: both
 */
plerrcode proc_v1190edges(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    plerrcode pres;

    if (p[0]>1) { /* set edge mode */
        pres=wMC(module, 0x2200, p+2, 1);
    } else {      /* read edge mode */
        pres=rMC(module, 0x2300, outptr, 1);
        if (pres==plOK)
            outptr++;
    }

    return pres;
}

plerrcode test_proc_v1190edges(ems_u32* p)
{
    plerrcode res;

    if (p[0]!=1 && p[0]!=2)
        return plErr_ArgNum;
    if ((res=test_mod_type(p[1]))!=plOK)
        return res;

    wirbrauchen=p[0]>1?0:1;
    return plOK;
}

char name_proc_v1190edges[] = "v1190edges";
int ver_proc_v1190edges = 1;
/*****************************************************************************/
/*
 * this function must be called after setting DAQ mode
 * p[0]: argcount==1|2
 * p[1]: module idx
 * [p[2]]: resolution: 0: 800 ps 1: 200 ps 2: 100 ps
 */
plerrcode proc_v1190resolution(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    ems_u32 edges;
    plerrcode pres;

    if (p[0]>1) { /* set resolution */
        if ((pres=rMC(module, 0x2300, &edges, 1))!=plOK)
            return pres;
        if (edges==0) { /* pairs */
            pres=wMC(module, 0x2500, p+2, 1);
        } else {        /* leading/trailing */
            pres=wMC(module, 0x2400, p+2, 1);
        }
    } else {      /* read resolution */
        pres=rMC(module, 0x2600, outptr, 1);
        if (pres==plOK)
            outptr++;
    }

    return pres;
}

plerrcode test_proc_v1190resolution(ems_u32* p)
{
    plerrcode res;

    if (p[0]!=1 && p[0]!=2)
        return plErr_ArgNum;
    if ((res=test_mod_type(p[1]))!=plOK)
        return res;

    wirbrauchen=p[0]>1?0:1;
    return plOK;
}

char name_proc_v1190resolution[] = "v1190resolution";
int ver_proc_v1190resolution = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==(2..3)
 * p[1]: module idx
 * p[2]: channel (0..127)
 * [p[3]: 0: disable, 1: enable (default)]
 */
plerrcode proc_v1190enablechannel(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    plerrcode pres;

    if (p[0]>2 && p[3]==0) { /* disable channel */
        pres=wMC(module, 0x4100+p[2], 0, 0);
    } else {                 /* enable channels */
        pres=wMC(module, 0x4000+p[2], 0, 0);
    }

    return pres;
}

plerrcode test_proc_v1190enablechannel(ems_u32* p)
{
    plerrcode res;

    if (p[0]!=2 && p[0]!=3)
        return plErr_ArgNum;
    if ((res=test_mod_type(p[1]))!=plOK)
        return res;
    if (p[2]>127)
        return plErr_ArgRange;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v1190enablechannel[] = "v1190enablechannel";
int ver_proc_v1190enablechannel = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: module idx
 * p[2]: 0: disable all channels, 1: enable all channels
 */
plerrcode proc_v1190enable(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    plerrcode pres;

    if (p[2]) { /* enable all channels */
        pres=wMC(module, 0x4200, 0, 0);
    } else {    /* disable all channels */
        pres=wMC(module, 0x4300, 0, 0);
    }

    return pres;
}

plerrcode test_proc_v1190enable(ems_u32* p)
{
    plerrcode res;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((res=test_mod_type(p[1]))!=plOK)
        return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v1190enable[] = "v1190enable";
int ver_proc_v1190enable = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1|2
 * p[1]: module idx
 */
plerrcode proc_v1190readout(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
    plerrcode pres=plOK;
    ems_u32 *help=outptr++;
    int res;

    /* the output buffer occupies only 4096 bytes of address space */
    do {
        res=dev->read_a32(dev, addr, outptr, 1024, 4, &outptr);
    } while (res==1024);
    *help=outptr-help-1;

    return pres;
}

plerrcode test_proc_v1190readout(ems_u32* p)
{
    plerrcode res;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((res=test_mod_type(p[1]))!=plOK)
        return res;

    wirbrauchen=1025;
    return plOK;
}

char name_proc_v1190readout[] = "v1190readout";
int ver_proc_v1190readout = 1;
/*****************************************************************************/
/*****************************************************************************/
