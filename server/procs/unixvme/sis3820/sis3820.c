/*
 * procs/unixvme/sis3820/sis3820.c
 * created 2013-02-22 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3820.c,v 1.1 2013/07/03 21:15:09 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../vme_verify.h"
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/unixvme/vme.h"
#include "../../../objects/var/variables.h"
#include "../../../lowlevel/devices.h"
#include "../../../trigger/trigger.h"
#include "sis3820.h"

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

RCS_REGISTER(cvsid, "procs/unixvme/sis3880")

/*****************************************************************************/
/*
MACROS for these addresses are defined in sis3820.h

Offset R/W Mode Function/Register
 0x0   R/W D32 Control/Status register
 0x4   R   D32 Module Id. and firmware revision register
 0x8   R/W D32 Interrupt configuration register
 0xC   R/W D32 Interrupt control/status register

 0x10  R/W D32 Acquisition preset register
 0x14  R   D32 Acquisition count register
 0x18  R/W D32 LNE prescale factor register

 0x20  R/W D32 Preset value register counter group 1 (1 to 16)
 0x24  R/W D32 Preset value register counter group 2 (17 to 32)
 0x28  R/W D32 Preset enable and hit register

 0x30  R/W D32 CBLT/Broadcast setup register
 0x34  R/W D32 SDRAM page register
 0x38  R   D32 FIFO Word count register
 0x3C  R/W D32 FIFO Word count threshold register

 0x40  R/W D32 HISCAL_START_PRESET
 0x44  R   D32 HISCAL_START_COUNTER
 0x48  R   D32 HISCAL_LAST_ACQ_COUNTER

 0x50  R   D32 Encoder Position Counter
 0x54  R   D32 Encoder Error Counter
0x100  R/W D32 (Acquisition) Operation mode register
0x104  R/W D32 Copy disable register
0x108  R/W D32 LNE channel select register (1 of 32)
0x10C  R/W D32 PRESET channel select register ( 2 times 1 out of 16)
0x110  R/W D32 MUX_OUT channel select register (firmware 01 0B, 1 of 32,
               note 2)
0x200  R/W D32 Inhibit/count disable register
0x204  W   D32 Counter Clear register
0x208  R/W D32 Counter Overflow read and clear register
0x210  R   D32 Channel 1/17 Bits 33-48
0x214  R/W D32 Veto external count inhibit register (firmware 01 09, note 2)
0x218  R/W D32 Test pulse mask register (firmware 01 0A, note 2)

0x300  R/W     SDRAM Prom
0x310  R/W     XILINX JTAG_TEST/JTAG_DATA_IN
0x314  W       XILINX JTAG_CONTROL
future use R/W  One wire Id. register

0x400  KA  D32 (broadcast) Key reset
0x404  KA  D32 (broadcast) Key SDRAM/FIFO reset
0x408  KA  D32 (broadcast) Key test pulse
0x40C  KA  D32 (broadcast) Key Counter Clear

0x410  KA  D32 (broadcast) Key VME LNE/clock shadow
0x414  KA  D32 (broadcast) Key operation arm
0x418  KA  D32 (broadcast) Key operation enable
0x41C  KA  D32 (broadcast) Key operation disable

0x420  KA  D32 (broadcast) Key HISCAL_START_PULS
0x424  KA  D32 (broadcast) Key HISCAL_ENABLE_LNE_ARM
0x428  KA  D32 (broadcast) Key HISCAL_ENABLE_LNE_ENABLE
0x42C  KA  D32 (broadcast) Key HISCAL_DISABLE

0x800 to 0x87C R D32/BLT32 Shadow registers
0xA00 to 0xA7C R D32/BLT32 Counter registers

0x800000 to 0xfffffc
       W   D32/BLT32
       R   D32/BLT32/MBLT64/2eVme 
               SDRAM or FIFO space array (address window for page of 8 Mbytes)
*/
/*****************************************************************************/
/*
 * p[0]: argcount==number of modules
 * p[1]: Operation Mode
 * p[2...]: indices in memberlist
 */
plerrcode proc_sis3820init(ems_u32* p)
{
    int i, res;
    ems_u32 opmode=p[1];

    for (i=2; i<=p[0]; i++) {
        unsigned int help, id, version;
        ml_entry* module=ModulEnt(p[i]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr, mode;

        /* read module identification */
        res=dev->read_a32d32(dev, module->address.vme.addr+4, &help);
        if (res!=4)
            return plErr_System;
        id=(help>>16)&0xffff;
        version=(help>>12)&0xf;
        if (id!=0x3820) {
            printf("sis3820init(idx=%d): id=%04x version=%d\n", i, id, version);
            return plErr_HWTestFailed;
        }

        /* reset module */
        dev->write_a32d32(dev, addr+SIS3820_KEY_RESET, 0);

        /* clear all counters (really necessary after reset?) */
        dev->write_a32d32(dev, addr+SIS3820_COUNTER_CLEAR, 0xffffffff);

        /* set Operation Mode register (0x100) */
#if 0
        mode=0;
        mode|=1<<0; /* do not clear with LNE */
        mode|=1<<4; /* LNE source bit: Front panel */
        mode|=6<<16; /* input mode '6' */
#else
        mode=opmode;
#endif
        dev->write_a32d32(dev, addr+SIS3820_OPERATION_MODE, mode);

        /* enable counting */
        dev->write_a32d32(dev, addr+SIS3820_KEY_OPERATION_ENABLE, 0);

        /* switch LED on (just for fun) */
        dev->write_a32d32(dev, addr+SIS3820_CONTROL_STATUS,
                0x1 /*0x10000: off*/);
    }
    return plOK;
}

plerrcode test_proc_sis3820init(ems_u32* p)
{
    int i;

    if (p[0]<2)
        return plErr_ArgNum;

    for (i=2; i<=p[0]; i++) {
        ml_entry* module;

        if (!valid_module(p[i], modul_vme, 0))
            return plErr_ArgRange;
        module=ModulEnt(p[i]);
        if (module->modultype!=SIS_3820)
            return plErr_BadModTyp;
    }
    wirbrauchen=0;
    return plOK;
}

char name_proc_sis3820init[] = "sis3820init";
int ver_proc_sis3820init = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==number of modules
 * p[1...]: indices in memberlist
 */
plerrcode proc_sis3820clear(ems_u32* p)
{
    int i;

    for (i=1; i<=p[0]; i++) {
        ml_entry* module=ModulEnt(p[i]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;

        /* clear all counters (really necessary after reset?) */
        dev->write_a32d32(dev, addr+SIS3820_COUNTER_CLEAR, 0xffffffff);
    }
    return plOK;
}

plerrcode test_proc_sis3820clear(ems_u32* p)
{
    int i;

    if (p[0]<1)
        return plErr_ArgNum;

    for (i=1; i<=p[0]; i++) {
        ml_entry* module;

        if (!valid_module(p[i], modul_vme, 0))
            return plErr_ArgRange;
        module=ModulEnt(p[i]);
        if (module->modultype!=SIS_3820)
            return plErr_BadModTyp;
    }
    wirbrauchen=0;
    return plOK;
}

char name_proc_sis3820clear[] = "sis3820clear";
int ver_proc_sis3820clear = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==number of modules
 * p[1]: 1: send LNE, 0: don't send LNE
 * p[2..]: indices in memberlist
 */
plerrcode proc_sis3820read(ems_u32* p) {
    int i;

    if (p[1]) {
    /* LNE */
        for (i=2; i<=p[0]; i++) {
            ml_entry* module=ModulEnt(p[i]);
            struct vme_dev* dev=module->address.vme.dev;
            ems_u32 addr=module->address.vme.addr;

            dev->write_a32d32(dev, addr+SIS3820_KEY_LNE_PULS, 0);
        }
    }

    /* read data */
    for (i=2; i<=p[0]; i++) {
        ml_entry* module=ModulEnt(p[i]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;
        unsigned j;

        /* read data */
        for (j=0; j<32; j++)
            dev->read_a32d32(dev, addr+SIS3820_COUNTER_SHADOW_CH1+j*4,
                    outptr++);
    }
    return plOK;
}

plerrcode test_proc_sis3820read(ems_u32* p)
{
    int i;

    if (p[0]<2)
        return plErr_ArgNum;

    for (i=2; i<=p[0]; i++) {
        ml_entry* module;

        if (!valid_module(p[i], modul_vme, 0))
            return plErr_ArgRange;
        module=ModulEnt(p[i]);
        if (module->modultype!=SIS_3820)
            return plErr_BadModTyp;
    }
    wirbrauchen=(p[0]-1)*32;
    return plOK;
}

char name_proc_sis3820read[] = "sis3820read";
int ver_proc_sis3820read = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==number of modules
 * p[1]: 1: send LNE, 0: don't send LNE
 * p[2...]: indices in memberlist
 */
plerrcode proc_sis3820read_block(ems_u32* p) {
    int i;

    if (p[1]) {
    /* LNE */
        for (i=2; i<=p[0]; i++) {
            ml_entry* module=ModulEnt(p[i]);
            struct vme_dev* dev=module->address.vme.dev;
            ems_u32 addr=module->address.vme.addr;

            dev->write_a32d32(dev, addr+SIS3820_KEY_LNE_PULS, 0);
        }
    }

    /* read data */
    for (i=2; i<=p[0]; i++) {
        ml_entry* module=ModulEnt(p[i]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;

        if (dev->read_a32(dev, addr+SIS3820_COUNTER_SHADOW_CH1,
                outptr, 128, 4, &outptr)<0) {
            return plErr_System;
        }
    }
    return plOK;
}

plerrcode test_proc_sis3820read_block(ems_u32* p) {
    int i;

    if (p[0]<2)
        return plErr_ArgNum;

    for (i=2; i<=p[0]; i++) {
        ml_entry* module;

        if (!valid_module(p[i], modul_vme, 0))
            return plErr_ArgRange;
        module=ModulEnt(p[i]);
        if (module->modultype!=SIS_3820)
            return plErr_BadModTyp;
    }
    wirbrauchen=(p[0]-1)*32;
    return plOK;
}

char name_proc_sis3820read_block[] = "sis3820read_block";
int ver_proc_sis3820read_block = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==number of modules
 * p[1]: indices in memberlist
 */
plerrcode proc_sis3820lne(ems_u32* p) {
    int i;

    for (i=1; i<=p[0]; i++) {
        ml_entry* module=ModulEnt(p[i]);
        struct vme_dev* dev=module->address.vme.dev;

        dev->write_a32d32(dev,
                module->address.vme.addr+SIS3820_KEY_LNE_PULS, 0);
    }
    return plOK;
}

plerrcode test_proc_sis3820lne(ems_u32* p) {
    int i;

    if (p[0]<1)
        return plErr_ArgNum;

    for (i=1; i<=p[0]; i++) {
        ml_entry* module;

        if (!valid_module(p[i], modul_vme, 0))
            return plErr_ArgRange;
        module=ModulEnt(p[i]);
        if (module->modultype!=SIS_3820)
            return plErr_BadModTyp;
    }
    wirbrauchen=0;
    return plOK;
}

char name_proc_sis3820lne[] = "sis3820lne";
int ver_proc_sis3820lne = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==number of modules
 * p[1]: idx of ems-variable
 * p[2...]: indices in memberlist
 */
plerrcode proc_sis3820ShadowInit(ems_u32* p)
{
    ems_u32 *var, mode;
    int i, res;

    for (i=2; i<=p[0]; i++) {
        unsigned int help, id, version;
        ml_entry* module=ModulEnt(p[i]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;

        /* read module identification */
        res=dev->read_a32d32(dev, addr+4, &help);
        if (res!=4)
            return plErr_System;
        id=(help>>16)&0xffff;
        version=(help>>12)&0xf;
        if (id!=0x3820) {
            printf("sis3800ShadowInit(idx=%d, addr=0x%08x): id=%04x "
                "version=%d (0x%08x)\n",
                        i, addr, id, version, help);
            return plErr_HWTestFailed;
        }

        /* reset module */
        dev->write_a32d32(dev, module->address.vme.addr+SIS3820_KEY_RESET, 0);

        /* clear all counters (really necessary after reset?) */
        dev->write_a32d32(dev, module->address.vme.addr+SIS3820_COUNTER_CLEAR,
                0xffffffff);

        /* set Operation Mode register (0x100) */
        mode=0;
        
        /*  select non clearing mode <0> */
        mode|=1<<0; /* do not clear with LNE */

        /* data format bit 0 und 1 <3..2> */
        /* 00: 32 bit
           01: 24 bit with user bit and channel information
           10: 16-bit (MCS only)
           11: 8-bit (MCS only) */
        /* default (32 bit) is OK */

        /* LNE source bit 0 bis 2 <6..4> */
        /* 000 VME key address only (ignore front panel signals)
           001 Front panel control signal
           010 10 MHz internal pulser
           011 Channel N (ChN)
           100 Preset Scaler N (MCS only)
           101 Encoder pulse */
        mode|=1<<4; /* Front panel */

        /* Arm/Enable source bit 0 und 1 <9..8> */
        /* 00 LNE Front panel control signal
           01 Channel N (ChN)
           10 reserved
           11 reserved */
        /* we use the default because we do not know what the
           purpose of arm/enable is */

        /* select SDRAM mode <12> */
        /* select SDRAM add mode <13> */
        /* 00 FIFO emulation
           01 SDRAM
           10 SDRAM
           11 SDRAM */
        /* we just want to read the content of all scaler channels
           in simple scaler mode, so FIFO mode is OK for us */
        
        /* HISCAL_START_SOURCE_BIT <14> */
        /* we do not use HISCAL */

        /* Control input mode bit 0 bis 2 <18..16> */
        /* we need input mode '6' to enable input 3 for "clear all counters" */
        mode|=6<<16;

        /* Control inputs inverted <19> */
        /* no, we don't need this */

        /* Control output mode bit 0 und 1 <21..20> */
        /* not necessary at the moment */

        /* Control outputs inverted <23> */
        /* no */

        /* Operation Mode bit 0 bis 2 <30..28> */
        /* 000 Scaler/Counter, Latching Scaler, Preset Scaler
           010 Multi channel Scaler
           111 SDRAM/FIFO VME write test mode
           (all others reserved) */
        /* we need 000, the default */

        /* now set the mode */
        dev->write_a32d32(dev,
            module->address.vme.addr+SIS3820_OPERATION_MODE, mode);

        /* enable counting */
        dev->write_a32d32(dev,
            module->address.vme.addr+SIS3820_KEY_OPERATION_ENABLE,
            0);

        /* switch LED on (just for fun) */
        dev->write_a32d32(dev, addr+SIS3820_CONTROL_STATUS, 0x1);
    }

    if (var_get_ptr(p[1], &var)!=plOK)
        return plErr_Program;
    for (i=0; i<(p[0]-1)*64; i++)
        var[i]=0;
    return plOK;
}

plerrcode test_proc_sis3820ShadowInit(ems_u32* p)
{
    unsigned int vsize;
    int i;
    plerrcode pcode;

    if (p[0]<2)
        return plErr_ArgNum;

    for (i=2; i<=p[0]; i++) {
        ml_entry* module;

        if (!valid_module(p[i], modul_vme, 0))
            return plErr_ArgRange;
        module=ModulEnt(p[i]);
        if (module->modultype!=SIS_3820)
            return plErr_BadModTyp;
    }

    if ((pcode=var_attrib(p[1], &vsize))!=plOK)
        return pcode;

    if (vsize!=(p[0]-1)*64)
        return plErr_IllVarSize;

    wirbrauchen=0;
    return plOK;
}

char name_proc_sis3820ShadowInit[] = "sis3820ShadowInit";
int ver_proc_sis3820ShadowInit = 1;
/*****************************************************************************/
/*
 * all modules of the memberlist have to be sis3820
 * all channels are cleared
 *
 * p[0]: argcount==1
 * p[1]: idx of ems-variable
 */
plerrcode proc_sis3820ShadowClear(ems_u32* p)
{
    ems_u32 *var;
    unsigned int vsize;
    int i;

    var_get_ptr(p[1], &var);
    var_attrib(p[1], &vsize);

    /* clear variable */
    memset(var, 0, vsize*sizeof(ems_u32));

    for (i=0; i<memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i+1);
        struct vme_dev* dev=module->address.vme.dev;
        unsigned int addr=module->address.vme.addr;

        /* disable counting */
        dev->write_a32d32(dev, addr+SIS3820_KEY_OPERATION_DISABLE, 0);

        /* clear all counters */
        dev->write_a32d32(dev,addr+SIS3820_COUNTER_CLEAR, 0xffffffff);

        /* enable counting */
        dev->write_a32d32(dev, addr+SIS3820_KEY_OPERATION_ENABLE, 0);
    }

    return plOK;
}

plerrcode test_proc_sis3820ShadowClear(ems_u32* p)
{
    ems_u32 mtypes[]={SIS_3820, 0};
    unsigned int vsize;
    int nr_modules, nr_channels, res;
    plerrcode pcode;

    if (p[0]!=1)
        return plErr_ArgNum;

    if (p[0]!=1)
        return plErr_ArgNum;
    if (!memberlist)
        return plErr_NoISModulList;

    nr_modules=memberlist[0];
    nr_channels=nr_modules*32;

    if ((pcode=var_attrib(p[1], &vsize))!=plOK)
        return pcode;
    if (vsize!=nr_channels*2)
        return plErr_IllVarSize;

    if ((res=test_proc_vme(memberlist, mtypes))!=plOK)
        return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_sis3820ShadowClear[] = "sis3820ShadowClear";
int ver_proc_sis3820ShadowClear = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==(number of modules)*2+2
 * p[1]: idx of ems-variable
 * p[2]: number of members
 * p[3...3+p[2]-1]: indices in memberlist
 * p[3+p[2]...3+2*p[2]-1] bitmasks
 * [p[3+2*p[2]] bitmask for latchpulse (via controller sis3100)]
 *   if a latchpulse bitmask is given, but its value is 0 then a soft clock
 *   is used
 */
plerrcode proc_sis3820ShadowUpdate(ems_u32* p)
{
    ems_u32 *var;
    int mod, idx;

    var_get_ptr(p[1], &var);

    /* generate LNE */
    if (p[0]>2*p[2]+2) {
        ems_u32 latchmask=p[2*p[2]+3];
        if (latchmask) {
            ml_entry* module=ModulEnt(p[3]);
            struct vme_dev* dev=module->address.vme.dev;
            dev->write_controller(dev, 1, 0x80, (latchmask&0x7f)<<24);
        } else {
            for (mod=0; mod<p[2]; mod++) {
                ml_entry* module=ModulEnt(p[mod+3]);
                struct vme_dev* dev=module->address.vme.dev;
                ems_u32 addr=module->address.vme.addr;

                dev->write_a32d32(dev, addr+SIS3820_KEY_LNE_PULS, 0);
            }
        }
    }

    *outptr++=p[2];
    for (mod=0; mod<p[2]; mod++) {
        ml_entry* module=ModulEnt(p[mod+3]);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;
        unsigned int mask, chan, n;
        ems_u32 vals[32];
        ems_u32 *mvar=var+mod*32*2; /* start of shadow for this module */
        ems_u32 *help;

        mask=p[3+p[2]+mod];
        help=outptr++;
        n=0;

        /* read data */
        if (dev->read_a32(dev, addr+SIS3820_COUNTER_SHADOW_CH1,
                vals, 128, 4, 0)<0) {
            return plErr_System;
        }

        /* overflow handling:
           if the value of one channel is smaller than the last one either
           an overflow has occured or the channel was reset.
           The SIS3820_COUNTER_OVERFLOW register tells us whether it was really
           an overflow.
           But the value itself and the COUNTER_OVERFLOW register are not
           read at the same time. Therefore it is possible that the overflow
           bit is set but we have the value just before the overflow. In this
           case we ignore the overflow bit.
           It is possible that an overflow has occured and the new value is
           already larger than the old one. This cannot be distinguished from
           the previous case and is ignored. It can be avoided is the scaler
           is read out more often than every 17 seconds (2**32/250MHz)
        */

        ems_u32 ovf_soft=0, ovf_hard;
        /* first we check whether there is any evidence for overflow
           without asking the hardware */
        for (chan=0; chan<32; chan++) {
            if ((vals[chan]&0xfffffffe)<(mvar[chan*2+1]&0xfffffffe))
                ovf_soft|=1<<chan;
        }
        /* if there is any evidence we ask the hardware */
        if (ovf_soft) {
            /* read the overflow pattern */
            dev->read_a32d32(dev, addr+SIS3820_COUNTER_OVERFLOW, &ovf_hard);
            /* correct it with our soft pattern */
            ovf_hard&=ovf_soft;
            /* and clear it */
            dev->write_a32d32(dev, addr+SIS3820_COUNTER_OVERFLOW, ovf_hard);
            for (chan=0; chan<32; chan++) {
                if ((1<<chan)&ovf_hard)
                    mvar[chan*2]++;
            }
        }

        /* write data to var and outptr */
        for (chan=0; chan<32; chan++) {
            mvar[chan*2+1]=vals[chan];
            if (mask&1) {
                *outptr++=mvar[chan*2];
                *outptr++=mvar[chan*2+1];
                n++;
            }
            idx+=2; mask>>=1;
        }

        *help=n;
    }
    return plOK;
}

plerrcode test_proc_sis3820ShadowUpdate(ems_u32* p)
{
    unsigned int vsize;
    int i;
    plerrcode pcode;

    if (p[0]<2)
        return plErr_ArgNum;
    if (p[1]<1) /* empty list of modules */
        return plErr_ArgRange;
    if ((pcode=var_attrib(p[1], &vsize))!=plOK)
        return pcode;
    if (vsize!=p[2]*64)
        return plErr_IllVarSize;
    if ((p[0]!=2*p[2]+2) && (p[0]!=2*p[2]+3))
        return plErr_ArgNum;

    for (i=0; i<p[2]; i++) {
        ml_entry* module;

        if (!valid_module(p[i+3], modul_vme, 0))
            return plErr_ArgRange;
        module=ModulEnt(p[i+3]);
        if (module->modultype!=SIS_3820)
            return plErr_BadModTyp;
    }

    if (p[0]>2*p[2]+2) {
        /* send latch pulse is only possible with sis3100 controller */
        ml_entry* module=ModulEnt(p[3]);
        struct vme_dev* dev=module->address.vme.dev;
        if (dev->vmetype!=vme_sis3100) {
            printf("sis3800ShadowUpdate: illegal controller for latchpulse\n");
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=p[2]*65+1;
    return plOK;
}

char name_proc_sis3820ShadowUpdate[] = "sis3820ShadowUpdate";
int ver_proc_sis3820ShadowUpdate = 1;
/*****************************************************************************/
/*****************************************************************************/
