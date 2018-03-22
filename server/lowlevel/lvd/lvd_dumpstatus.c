/*
 * lowlevel/lvd/lvd_dumpstatus.c
 * created 13.Dec.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_dumpstatus.c,v 1.25 2017/10/20 23:21:31 wuestner Exp $";

#define LOWLIB
#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <modultypes.h>
#include <xprintf.h>
#include <rcs_ids.h>
#include "lvd_map.h"
#include "lvdbus.h"
#include "lvd_access.h"

RCS_REGISTER(cvsid, "lowlevel/lvd")

/*****************************************************************************/
int
lvd_cardstat_acq(struct lvd_dev* dev, unsigned int addr, void *xp, int level, int all)
{
    int i=0;

    while ((i<dev->num_acards) && (dev->acards[i].addr!=addr))
        i++;
    if (dev->acards[i].addr!=addr) {
        /* see comment in lvdbus.c:lvd_cratestat */
        xprintf(xp, "lvd_cardstat_acq: cannot find card 0x%x; program error\n",
            addr);
        return -1;
    }
    xprintf(xp, "ACQcard 0x%03x mtype=0x%x (%sinitialized)\n",
            addr, dev->acards[i].mtype, dev->acards[i].initialized?"":"NOT ");
    if (all||dev->acards[i].initialized)
        return dev->acards[i].cardstat(dev, dev->acards+i, xp, level);
    else
        return 0;
}
/*****************************************************************************/
/*****************************************************************************/
static int
dump_ident(struct lvd_dev* dev, void *xp)
{
    ems_u32 val;

    if (lvd_cc_r(dev, ident, &val)<0) {
        xprintf(xp, "  unable to read ident\n");
        return -1;
    }

    xprintf(xp, "  [00] ident       =0x%04x (type=0x%x version=0x%x)\n",
            val, val&0xff, (val>>8)&0xff);
    return 0;
}
/*****************************************************************************/
static int
dump_serial(struct lvd_dev* dev, void *xp)
{
    ems_u32 val;
    int res;

    dev->silent_errors=1;
    res=lvd_cc_r(dev, serial, &val);
    dev->silent_errors=0;
    if (res<0) {
        xprintf(xp, "  [02] serial      : not set\n");
    } else {
        xprintf(xp, "  [02] serial      =%d\n", val);
    }
    return 0;
}
/*****************************************************************************/
static int
dump_cr(struct lvd_dev* dev, void *xp)
{
    ems_u32 val;

    if (lvd_cc_r(dev, cr, &val)<0) {
        xprintf(xp, "  unable to read \n");
        return -1;
    }

    xprintf(xp, "  [08] cr          =0x%04x MODE=", val);

    switch (LVD_HWtyp(dev->ccard.id)) {
    case LVD_CARDID_CONTROLLER_GPX:
    case LVD_CARDID_CONTROLLER_F1:
        switch (val&LVD_MCR_DAQ) {
        case LVD_MCR_DAQIDLE:    xprintf(xp, "IDLE");     break;
        case LVD_MCR_DAQWINDOW:  xprintf(xp, "WINDOW");   break;
        case LVD_MCR_DAQRAWINP:  xprintf(xp, "INP_RAW");  break;
        case LVD_MCR_DAQRAWTRIG: xprintf(xp, "TRIG_RAW"); break;
        }
        if (val&LVD_MCR_DAQ_TEST)    xprintf(xp, " TEST");
        if (val&LVD_MCR_DAQ_HANDSH)  xprintf(xp, " HANDSH");
        if (val&LVD_MCR_DAQ_LOCEVNR) xprintf(xp, " ALLOW_LOCAL_EVNR");
        if (val&LVD_MCR_F1_MODE)     xprintf(xp, " F1_MODE");
        if (val&LVD_MCR_TRG_BUSY)    xprintf(xp, " TRG_BUSY");
        if (val&LVD_MCR_NOSTART)     xprintf(xp, " NOSTART");
        break;
    case LVD_CARDID_CONTROLLER_F1GPX:
        switch (val&LVD_MCR3_DAQ) {
        case LVD_MCR_DAQIDLE:        xprintf(xp, "IDLE");     break;
        case LVD_MCR_DAQWINDOW:      xprintf(xp, "WINDOW");   break;
        case LVD_MCR_DAQRAWINP:      xprintf(xp, "INP_RAW");  break;
        case LVD_MCR_DAQRAWTRIG:     xprintf(xp, "TRIG_RAW"); break;
        case LVD_MCR3_QDCTRIG:       xprintf(xp, "QDC triggered"); break;
        case LVD_MCR3_DAQTRIGANDRAW: xprintf(xp, "RAW_and_TRIGGERED"); break;
        default: xprintf(xp, "unknown mode %02x", val&LVD_MCR3_DAQ); break;
        }
        if (val&LVD_MCR_DAQ_TEST)    xprintf(xp, " TEST");
        if (val&LVD_MCR_DAQ_HANDSH)  xprintf(xp, " HANDSH");
        if (val&LVD_MCR_DAQ_LOCEVNR) xprintf(xp, " ALLOW_LOCAL_EVNR");
        if (val&LVD_MCR_F1_MODE)     xprintf(xp, " F1_MODE");
        if (val&LVD_MCR_TRG_BUSY)    xprintf(xp, " TRG_BUSY");
        if (LVD_FWver(dev->ccard.id)> 0x5 ) {
            if (val&LVD_MCR3_EXTRA_TRIG) xprintf(xp, " EXTRA_TRIG");
            if (val&LVD_MCR3_RATE_TRIG)  xprintf(xp, " RATE_TRIG");
        }
        break;
    default:
        xprintf(xp, "unknown type %d", LVD_HWtyp(dev->ccard.id));
    }
    xprintf(xp, "\n");

    return 0;
}
/*****************************************************************************/
static int
dump_sr(struct lvd_dev* dev, void *xp)
{
    ems_u32 val;

    if (lvd_cc_r(dev, sr, &val)<0) {
        xprintf(xp, "  unable to read sr\n");
        return -1;
    }

    xprintf(xp, "  [0a] sr          =0x%04x", val);
    if (val&LVD_MSR_DATA_AV)   xprintf(xp, " DATA_AV");
    if (val&LVD_MSR_READY)     xprintf(xp, " READY");
    switch (LVD_HWtyp(dev->ccard.id)) {
    case LVD_CARDID_CONTROLLER_F1:
        if (val&LVD_MSR1_WRBUSY)    xprintf(xp, " WRBUSY");
        if (val&LVD_MSR1_NOINIT)    xprintf(xp, " NOINIT");
        if (val&LVD_MSR1_NOLOCK)    xprintf(xp, " NOLOCK");
        if (val&LVD_MSR1_T_OVL)     xprintf(xp, " T_FIFO_OVL");
        if (val&LVD_MSR1_H_OVL)     xprintf(xp, " H_FIFO_OVL");
        if (val&LVD_MSR1_O_OVL)     xprintf(xp, " O_FIFO_OVL");
        if (val&LVD_MSR1_FATAL)     xprintf(xp, " FATAL");
        if (val&LVD_MSR1_TRG_LOST)  xprintf(xp, " TRG_LOST");
        if (val&LVD_MSR1_EVC_ERR)   xprintf(xp, " NO_VALID_EVNR");
        if (val&LVD_MSR1_FIFO_FULL) xprintf(xp, " FIFO_FULL");
        if (val&LVD_MSR1_F1_SEQERR) xprintf(xp, " SEQERR");
        break;
    case LVD_CARDID_CONTROLLER_GPX:
        if (val&LVD_MSR2_EF0)       xprintf(xp, " EF0");
        if (val&LVD_MSR2_EF1)       xprintf(xp, " EF1");
        if (val&LVD_MSR2_NOLOCK)    xprintf(xp, " NOLOCK");
        if (val&LVD_MSR2_INTFLAG)   xprintf(xp, " INTFLAG");
        if (val&LVD_MSR2_FATAL)     xprintf(xp, " FATAL");
        if (val&LVD_MSR2_TRG_LOST)  xprintf(xp, " TRG_LOST");
        if (val&LVD_MSR2_EVC_ERR)   xprintf(xp, " NO_VALID_EVNR");
        if (val&LVD_MSR2_FIFO_FULL) xprintf(xp, " FIFO_FULL");
        break;
    case LVD_CARDID_CONTROLLER_F1GPX:
        if (val&LVD_MSR3_NOINIT)    xprintf(xp, " NOINIT");
        if (val&LVD_MSR3_NOLOCKF1)  xprintf(xp, " NOLOCK_F1");
        if (val&LVD_MSR3_NOLOCKGPX) xprintf(xp, " NOLOCK_GPX");
        if (val&LVD_MSR3_PHASE_ERR) xprintf(xp, " PHASE_ERR");
        if (val&LVD_MSR3_EVC_ERR)   xprintf(xp, " NO_VALID_EVNR");
        if (val&LVD_MSR3_TRG_LOST)  xprintf(xp, " TRG_LOST");
        if (val&LVD_MSR3_GLB)       xprintf(xp, " GLB_ERR");
        if (val&LVD_MSR3_FATAL)     xprintf(xp, " FATAL");
        if (val&LVD_MSR3_FIFO_FULL) xprintf(xp, " FIFO_FULL");
        break;
    default:
        xprintf(xp, "\n    unknown controller type %d", LVD_HWtyp(dev->ccard.id));
    }
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
dump_evnr(struct lvd_dev* dev, void *xp)
{
    ems_u32 val;

    if (lvd_cc_r(dev, event_nr, &val)<0) {
        xprintf(xp, "  unable to read event_nr\n");
        return -1;
    }

    xprintf(xp, "  [0c] event_nr    =0x%08x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump_delay(struct lvd_dev* dev, void *xp)
{
    ems_u32 val;

    if (lvd_cc_r(dev, ro_delay, &val)<0) {
        xprintf(xp, "  unable to read ro_delay\n");
        return -1;
    }

    xprintf(xp, "  [14] ro_delay    =0x%04x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump_rate_trigger(struct lvd_dev* dev, void *xp)
{

    ems_u32 val;

    if ( (LVD_HWtyp(dev->ccard.id)!= LVD_CARDID_CONTROLLER_F1GPX) ||
            (LVD_FWver(dev->ccard.id)< 0x5 ))
        return 0;

    if (lvd_c3_r(dev, rate_trigger, &val)<0) {
        xprintf(xp, "  unable to read rate_trigger\n");
        return -1;
    }

    xprintf(xp, "  [14] rate_trigger    =0x%04x\n", val );
    xprintf(xp, "       period          =0x%02x\n", (val & 0xff) );
    xprintf(xp, "       count           =0x%02x\n", (val & 0xff00) >> 8 );
    return 0;
}
/*****************************************************************************/
static void
decode_gpx_reg(int reg, ems_u32 val, void *xp)
{
    switch (reg) {
    case 0:
        xprintf(xp, " osc.=%02x", val&1);
        xprintf(xp, " diff. inp.=%02x", (val>>1)&0x3f);
        xprintf(xp, " serv.=%02x", (val>>7)&0x7);
        xprintf(xp, " rising=%x|%02x", (val>>10)&1, (val>>11)&0xff); 
        xprintf(xp, " falling=%x|%02x", (val>>19)&1, (val>>20)&0xff); 
        break;
    case 1:
        xprintf(xp, " (channel adjustment)");
        break;
    case 2:
        if (val&1) xprintf(xp, " G-Mode");
        if (val&2) xprintf(xp, " I-Mode");
        if (val&4) xprintf(xp, " R-Mode");
        xprintf(xp, " disabled chan=%x|%02x", (val>>3)&1, (val>>4)&0xff);
        break;
    case 3: break;
    case 4: break;
    case 5: break;
    case 6: break;
    case 7: break;
    case 8: break;
    case 9: break;
    case 10: break;
    case 11: break;
    case 12: break;
    case 13: break;
    case 14: break;
    }
}
/*****************************************************************************/
static int
dump_gpx_regs(struct lvd_dev* dev, void *xp)
{
    static int regs_to_be_read[]={
        0, 1, 2, 3, 4, 5, 6, 7, 11, 12, 14, -1,
    };
    int i;

    if (LVD_HWtyp(dev->ccard.id)!=LVD_CARDID_CONTROLLER_GPX)
        return 0;

    for (i=0; regs_to_be_read[i]>=0; i++) {
        ems_u32 val;
        int reg=regs_to_be_read[i];

        if (lvd_c2_w(dev, gpx_reg, reg)<0) {
            xprintf(xp, "  unable to write gpx_reg\n");
            return -1;
        }
        if (lvd_c2_r(dev, gpx_data, &val)<0) {
            xprintf(xp, "  unable to read gpx_reg\n");
            return -1;
        }
        xprintf(xp, "  gpx_reg[%2d] =0x%07x", reg, val);
        decode_gpx_reg(reg, val, xp);
        xprintf(xp, "\n");
    }

    return 0;
}
/*****************************************************************************/
static int
dump_gpx_range(struct lvd_dev* dev, void *xp)
{
    ems_u32 val;

    if (LVD_HWtyp(dev->ccard.id)!=LVD_CARDID_CONTROLLER_GPX)
        return 0;

    if (lvd_c2_r(dev, gpx_range, &val)<0) {
        xprintf(xp, "  unable to read gpx_range\n");
        return -1;
    }

    xprintf(xp, "  [28] gpx_range   =0x%04x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump_ready(struct lvd_dev* dev, void *xp)
{
    ems_u32 val;

    if (lvd_ib_r(dev, ro_data, &val)<0) {
        xprintf(xp, "  unable to read ro_data\n");
        return -1;
    }

    val&=0xffff;
    if (val) {
        int i;
        xprintf(xp, "  ACQcards with data:");
        for (i=0; val; i++, val>>=1) {
            if (val&1)
                xprintf(xp, " %d", i);
        }
        xprintf(xp, "\n");
    } else {
        xprintf(xp, "  no ACQcard has data\n");
    }
    return 0;
}
/*****************************************************************************/
static int
dump_error(struct lvd_dev* dev, void *xp)
{
    ems_u32 val;

    if (lvd_ib_r(dev, error, &val)<0) {
        xprintf(xp, "  unable to read error\n");
        return -1;
    }

    val&=0xffff;
    if (val) {
        int i;
        xprintf(xp, "  ACQcards with error:");
        for (i=0; val; i++, val>>=1) {
            if (val&1)
                xprintf(xp, " %d", i);
        }
        xprintf(xp, "\n");
    } else {
        xprintf(xp, "  no ACQcard has an error\n");
    }
    return 0;
}
/*****************************************************************************/
static int
dump_busy(struct lvd_dev* dev, void *xp)
{
    ems_u32 val;

    if (lvd_ib_r(dev, t.busy[0], &val)<0) {
        xprintf(xp, "  unable to read t.busy\n");
        return -1;
    }

    val&=0xffff;
    if (val) {
        int i;
        xprintf(xp, "  ACQcards witch are busy:");
        for (i=0; val; i++, val>>=1) {
            if (val&1)
                xprintf(xp, " %d", i);
        }
        xprintf(xp, "\n");
    } else {
        xprintf(xp, "  no ACQcard is busy\n");
    }
    return 0;
}
/*****************************************************************************/
static int
dump_softinfo(struct lvd_dev* dev, void *xp)
{
    xprintf(xp, "  hsdiv     =%d (0x%x)\n", dev->hsdiv, dev->hsdiv);
    xprintf(xp, "  range     =%d (0x%x)\n", dev->tdc_range, dev->tdc_range);
    xprintf(xp, "  resolution=%.2f ps\n", dev->tdc_resolution);
    xprintf(xp, "  daq_mode  =0x%04x\n", dev->ccard.daqmode);
    if (LVD_HWtyp(dev->ccard.id)==LVD_CARDID_CONTROLLER_F1GPX) {
        xprintf(xp, "  controller mode: (%d) ", dev->contr_mode);
        switch (dev->contr_mode) {
        case contr_none: xprintf(xp, "NONE\n"); break;
        case contr_f1: xprintf(xp, "F1\n"); break;
        case contr_gpx: xprintf(xp, "GPX\n"); break;
        default: xprintf(xp, "illegal\n");
        }
    }
    xprintf(xp, "  trigger edge=%d\n", dev->ccard.trigger_edge);
    if (LVD_HWtyp(dev->ccard.id)!=LVD_CARDID_CONTROLLER_F1) {
        xprintf(xp, "  trigger mask=0x%x", dev->ccard.trigger_mask);
        if (dev->ccard.trigger_mask&1)
            xprintf(xp, " LEMO");
        if (dev->ccard.trigger_mask&2)
            xprintf(xp, " RJ45");
        if (dev->ccard.trigger_mask&4)
            xprintf(xp, " 6pack");
        if (dev->ccard.trigger_mask&8)
            xprintf(xp, " optic");
        xprintf(xp, "\n");
    }
    return 0;
}
/*****************************************************************************/
int
lvd_controllerstat(struct lvd_dev* dev, void *xp, int level)
{
    xprintf(xp, "Controller (");
    switch (LVD_HWtyp(dev->ccard.id)) {
    case LVD_CARDID_CONTROLLER_GPX:
        xprintf(xp, "GPX");
        break;
    case LVD_CARDID_CONTROLLER_F1:
        xprintf(xp, "F1");
        break;
    case LVD_CARDID_CONTROLLER_F1GPX:
        xprintf(xp, "F1GPX");
        break;
    default:
        xprintf(xp, "unknown type %d", LVD_HWtyp(dev->ccard.id));
    }
    xprintf(xp, "):\n");
    switch (level) {
    case 0:
        dump_cr(dev, xp);
        dump_sr(dev, xp);
        dump_ready(dev, xp);
        dump_error(dev, xp);
        dump_busy(dev, xp);
        break;
    case 1:
        dump_ident(dev, xp);
        dump_cr(dev, xp);
        dump_sr(dev, xp);
        dump_evnr(dev, xp);
        dump_delay(dev, xp);
        dump_ready(dev, xp);
        dump_error(dev, xp);
        dump_busy(dev, xp);
        dump_softinfo(dev, xp);
        break;
    case 2:
    default:
        dump_ident(dev, xp);
        dump_serial(dev, xp);
        dump_cr(dev, xp);
        dump_sr(dev, xp);
        dump_evnr(dev, xp);
        dump_delay(dev, xp);
        dump_gpx_regs(dev, xp);
        dump_gpx_range(dev, xp);
        dump_rate_trigger(dev, xp);
#if 0
dump_f1_regs(dev, contr, xp);
#endif
        dump_ready(dev, xp);
        dump_error(dev, xp);
        dump_busy(dev, xp);
        dump_softinfo(dev, xp);
        break;
    }

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
