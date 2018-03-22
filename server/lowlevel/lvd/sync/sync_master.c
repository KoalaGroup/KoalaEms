/*
 * lowlevel/lvd/syncmaster/syncmaster.c
 * created 2006-Feb-07 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sync_master.c,v 1.21 2017/10/20 23:21:31 wuestner Exp $";

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
#include <xprintf.h>
#include <rcs_ids.h>
#include "sync.h"
#include "../lvd_access.h"
#include "../lvd_initfuncs.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

struct msync_info {
    int msync2;
    int trigmask;
    int stopcount;
    int running;
};

RCS_REGISTER(cvsid, "lowlevel/lvd/sync")

/*****************************************************************************/
/*
 * the real start will be done in lvd_syncmaster_start
 */
static int
lvd_start_msynch(__attribute__((unused)) struct lvd_dev* dev, __attribute__((unused)) struct lvd_acard* acard)
{
    return 0;
}
/*****************************************************************************/
/*
 * the real stop should already be done in lvd_syncmaster_stop
 */
static int
lvd_stop_msynch(__attribute__((unused)) struct lvd_dev* dev, __attribute__((unused)) struct lvd_acard* acard)
{
    return 0;
}
/*****************************************************************************/
static int
lvd_clear_msynch(__attribute__((unused)) struct lvd_dev* dev,
        __attribute__((unused)) struct lvd_acard* acard)
{
    return 0;
}
/*****************************************************************************/
static plerrcode
lvd_syncmaster_init_card(struct lvd_dev* dev, int addr, int mode,
    int trig_mask, int sum_mask)
{
    struct lvd_acard* acard;
    struct msync_info* info;
    int idx;

    idx=lvd_find_acard(dev, addr);
    acard=dev->acards+idx;
    if ((idx<0) || (acard->mtype!=ZEL_LVD_MSYNCH)) {
        printf("lvd_syncmaster_init_card: no MSYNC card with address 0x%x known\n",
                addr);
        return plErr_Program;
    }
    info=(struct msync_info*)acard->cardinfo;

    acard->daqmode=mode;
    info->stopcount=0;
    info->running=0;
    info->trigmask=trig_mask;

    printf("initializing (%s) syncmaster card 0x%x\n",
            info->msync2?"new":"old", addr);


    if (info->msync2) {
        if (lvd_i_w(dev, addr, msync2.cr, 0)<0) {
            printf("lvd_syncmaster_init_card: error initialising card 0x%x\n", addr);
            return plErr_System;
        }
        printf("write trig_mask 0x%04x\n", (~trig_mask)&0xffff);
        if (lvd_i_w(dev, addr, msync2.trig_inhibit, ~trig_mask)<0) {
            return plErr_System;
        }
        if (lvd_i_w(dev, addr, msync2.log_inhibit, 0)<0) {
            return plErr_System;
        }
        if (lvd_i_w(dev, addr, msync2.sum_inhibit, ~sum_mask)<0) {
            return plErr_System;
        }
    } else {
        if (lvd_i_w(dev, addr, msync.cr, 0)<0) {
            printf("lvd_syncmaster_init_card: error initialising card 0x%x\n", addr);
            return plErr_System;
        }
        printf("write trig_mask 0x%04x\n", (~trig_mask)&0xffff);
        if (lvd_i_w(dev, addr, msync.trig_inhibit, ~trig_mask)<0) {
            return plErr_System;
        }
        if (lvd_i_w(dev, addr, msync.log_inhibit, 0)<0) {
            return plErr_System;
        }
    }

    printf("lvd_syncmaster_init_card: input card 0x%x initialized.\n", addr);
    acard->initialized=1;

    return plOK;
}
/*****************************************************************************/
/* initialize input cards */
plerrcode
lvd_syncmaster_init(struct lvd_dev* dev, int addr, int daqmode, int trig_mask,
        int sum_mask)
{
    int card;
    plerrcode pres;

    if (addr<0) {
        for (card=0; card<dev->num_acards; card++) {
            if (dev->acards[card].mtype==ZEL_LVD_MSYNCH) {
                pres=lvd_syncmaster_init_card(dev, dev->acards[card].addr, daqmode,
                        trig_mask, sum_mask);
                if (pres!=plOK) {
                    return pres;
                }
            }
        }
    } else {
        /* type was checked in test_proc_lvd_syncmaster_init already */
        pres=lvd_syncmaster_init_card(dev, addr, daqmode, trig_mask, sum_mask);
        if (pres!=plOK) {
            return pres;
        }
    }

    printf("lvd_syncmaster_init: all input cards initialized.\n");

    return plOK;
}
/*****************************************************************************/
plerrcode
lvd_syncmaster_width(struct lvd_dev* dev, int addr, int sel, int val, int save)
{
    int i, help;

    val&=0x1f; /* only 5 bit are valid */
    val|=0x80; /* set bit 7 for writing */

    for (i=0; i<8; i++) {
        help=0;
        if (sel&(1<<(2*i))) {
            help|=val;
        }
        if (sel&(1<<(2*i+1))) {
            help|=val<<8;
        }
        if (lvd_i_w(dev, addr, msync.sel_pw[i], help)<0)
            return plErr_System;
    }
    help=0;
    if (sel&0x10000) {
        help|=val;
    }
    if (sel&0x20000) {
        help|=val<<8;
    }
    if (lvd_i_w(dev, addr, msync.sel_xpw, help)<0)
        return plErr_System;

    help=save?3:1;
    if (lvd_i_w(dev, addr, msync.pw_ctrl, help)<0)
        return plErr_System;
    sleep(1);

    return plOK;
}
/*****************************************************************************/
plerrcode
lvd_syncmaster_evc(struct lvd_dev *dev, int addr, ems_u32 *val)
{
    lvd_i_r(dev, addr, msync.evc, val);
    return plOK;
}
/*****************************************************************************/
plerrcode
lvd_syncmaster_pause(struct lvd_dev *dev, int addr)
{
    struct lvd_acard *acard;
    struct msync_info* info;
    ems_u32 val;
    int res, idx, cc=10;

    idx=lvd_find_acard(dev, addr);
    acard=dev->acards+idx;
    if ((idx<0) || (acard->mtype!=ZEL_LVD_MSYNCH)) {
        printf("lvd_syncmaster_pause: no MSYNC card with address 0x%x known\n",
                addr);
        return plErr_Program;
    }
    info=(struct msync_info*)acard->cardinfo;

    if (!info->running) {
        printf("lvd_syncmaster_pause: not running\n");
        return plOK;
    }
    lvd_i_wblind(dev, addr, msync.trig_inhibit, 0xffff);
    res=lvd_i_r(dev, addr, msync.trig_inhibit, &val);
    while ((res || (val&0xffff)!=0xffff) && cc-->0) {
        printf("lvd_syncmaster_pause: masking failed: res=%d val=0x%x\n",
            res, val);
        if (--cc<0) {
            printf("lvd_syncmaster_pause: giving up\n");
            return plErr_System;
        }
        lvd_i_wblind(dev, addr, msync.trig_inhibit, 0xffff);
        sleep(1);
        res=lvd_i_r(dev, addr, msync.trig_inhibit, &val);
    }
    info->stopcount++;
    return plOK;
}
/*****************************************************************************/
plerrcode
lvd_syncmaster_resume(struct lvd_dev *dev, int addr)
{
    struct lvd_acard *acard;
    struct msync_info* info;
    int idx;

    idx=lvd_find_acard(dev, addr);
    acard=dev->acards+idx;
    if ((idx<0) || (acard->mtype!=ZEL_LVD_MSYNCH)) {
        printf("lvd_syncmaster_resume: no MSYNC card with address 0x%x known\n",
                addr);
        return plErr_Program;
    }
    info=(struct msync_info*)acard->cardinfo;

    if (!info->running) {
        printf("lvd_syncmaster_resume: not running\n");
        return plOK;
    }
    if (info->stopcount>0)
        info->stopcount--;
    if (info->stopcount==0) {
        lvd_i_wblind(dev, addr, msync.trig_inhibit, ~info->trigmask);
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
lvd_syncmaster1_start(struct lvd_dev *dev, struct lvd_acard* acard)
{
    struct msync_info* info=(struct msync_info*)acard->cardinfo;
    int addr=acard->addr, res=0;

    res|=lvd_i_w(dev, addr, msync.cr, acard->daqmode|MSYNC_CR_GO);
    res|=lvd_ib_w(dev, ctrl, LVD_TRG_MRST); /* ? */

    info->stopcount=0;
    info->running=1;

    return res?plErr_System:plOK;
}
/*****************************************************************************/
static plerrcode
lvd_syncmaster2_start(struct lvd_dev *dev, struct lvd_acard* acard)
{
    struct msync_info* info=(struct msync_info*)acard->cardinfo;
    int addr=acard->addr, res=0;

printf("syncmaster2_start: write 0x%x to cr\n", acard->daqmode|MSYNC2_CR_RUN);
    res|=lvd_i_w(dev, addr, msync2.cr, acard->daqmode|MSYNC2_CR_RUN);
printf("syncmaster2_start: broadcast LVD_TRG_MRST\n");
    res|=lvd_ib_w(dev, ctrl, LVD_TRG_MRST);
    //res|=lvd_i_w(dev, addr, msync2.ctrl, MSYNC2_CTRL_TRG_RST);

    info->stopcount=0;
    info->running=1;

    return res?plErr_System:plOK;
}
/*****************************************************************************/
plerrcode
lvd_syncmaster_start(struct lvd_dev *dev, int addr)
{
    struct lvd_acard *acard;
    struct msync_info* info;
    int idx;

    idx=lvd_find_acard(dev, addr);
    acard=dev->acards+idx;
    if ((idx<0) || (acard->mtype!=ZEL_LVD_MSYNCH)) {
        printf("lvd_syncmaster_start: no MSYNC card with address 0x%x known\n",
                addr);
        return plErr_Program;
    }
    info=(struct msync_info*)acard->cardinfo;
    if (info->msync2)
        return lvd_syncmaster2_start(dev, acard);
    else
        return lvd_syncmaster1_start(dev, acard);
}
/*****************************************************************************/
static plerrcode
lvd_syncmaster1_stop(struct lvd_dev *dev, struct lvd_acard* acard)
{
    struct msync_info* info=(struct msync_info*)acard->cardinfo;
    int addr=acard->addr;

    printf("write daqmode 0 to old SYNCH_MASTER card 0x%x\n", addr);
    lvd_i_wblind(dev, addr, msync.cr, 0);
    info->stopcount=0;
    info->running=0;

    return plOK;
}
/*****************************************************************************/
static plerrcode
lvd_syncmaster2_stop(struct lvd_dev *dev, struct lvd_acard* acard)
{
    struct msync_info* info=(struct msync_info*)acard->cardinfo;
    int addr=acard->addr;
    int res=0;

    printf("reset run bit in new SYNCH_MASTER card 0x%x\n", addr);
    res|=lvd_i_wblind(dev, addr, msync2.cr, acard->daqmode&~MSYNC2_CR_RUN);
    info->stopcount=0;
    info->running=0;

    return plOK;
}
/*****************************************************************************/
plerrcode
lvd_syncmaster_stop(struct lvd_dev *dev, int addr)
{
    struct lvd_acard *acard;
    struct msync_info* info;
    int idx;

    idx=lvd_find_acard(dev, addr);
    acard=dev->acards+idx;
    if ((idx<0) || (acard->mtype!=ZEL_LVD_MSYNCH)) {
        printf("lvd_syncmaster_stop: no MSYNC card with address 0x%x known\n",
                addr);
        return plErr_Program;
    }
    info=(struct msync_info*)acard->cardinfo;
    if (info->msync2)
        return lvd_syncmaster2_stop(dev, acard);
    else
        return lvd_syncmaster1_stop(dev, acard);
}
/*****************************************************************************/
/*****************************************************************************/
static int
dump_ident(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, msync.ident, &val)<0) {
        xprintf(xp, "  unable to read ident\n");
        return -1;
    }

    xprintf(xp, "  [00] ident       =0x%04x (type=0x%x version=0x%x)\n",
            val, val&0xff, (val>>8)&0xff);
    return 0;
}
/*****************************************************************************/
static int
dump_serial(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;
    int res;

    dev->silent_errors=1;
    res=lvd_i_r(dev, addr, msync.serial, &val);
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
dump1_sr(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, msync.sr, &val)<0) {
        xprintf(xp, "  unable to read sr\n");
        return -1;
    }

    xprintf(xp, "  [0e] sr          =0x%04x", val);
    if (val&MSYNC_SR_RING_TRG)
        xprintf(xp, " RING_TRG");
    if (val&MSYNC_SR_RING_GO)
        xprintf(xp, " RING_GO");
    if (val&MSYNC_SR_VETO)
        xprintf(xp, " VETO");
    if (val&MSYNC_SR_SI)
        xprintf(xp, " SI");
    if (val&MSYNC_SR_M_INH)
        xprintf(xp, " M_INH");
    if (val&MSYNC_SR_AUX_IN)
        xprintf(xp, " AUX_IN");
    if (val&MSYNC_SR_RING_SI)
        xprintf(xp, " RING_SI");
    if (val&MSYNC_SR_HL_TOUT)
        xprintf(xp, " HL_TOUT");
    if (val&MSYNC_SR_RING_TDT)
        xprintf(xp, " RING_TDT");
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
dump2_sr(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, msync.sr, &val)<0) {
        xprintf(xp, "  unable to read sr\n");
        return -1;
    }

    xprintf(xp, "  [0e] sr          =0x%04x", val);
    if (val&MSYNC2_SR_FERR)
        xprintf(xp, " FERR");
    if (val&MSYNC2_SR_RUN)
        xprintf(xp, " RUN");
    if (val&MSYNC2_SR_ACTIVE)
        xprintf(xp, " ACTIVE");
    if (val&MSYNC2_SR_BUSY)
        xprintf(xp, " BUSY");
    if (val&MSYNC2_SR_BSYALL)
        xprintf(xp, " BSYALL");
    if (val&MSYNC2_SR_VETO)
        xprintf(xp, " VETO");
    if (val&MSYNC2_SR_AUX_IN)
        xprintf(xp, " AUX_IN=0x%x", (val>>6)&3);
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
dump1_cr(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, msync.cr, &val)<0) {
        xprintf(xp, "  unable to read cr\n");
        return -1;
    }

    xprintf(xp, "  [0c] cr          =0x%04x", val);
    if (val&MSYNC_CR_RING)
        xprintf(xp, " RING");
    if (val&MSYNC_CR_GO)
        xprintf(xp, " GO");
    if (val&MSYNC_CR_TAW_ENA)
        xprintf(xp, " TAW_ENA");
    if (val&MSYNC_CR_LOCBUSY)
        xprintf(xp, " LOCAL_BUSY_ENA");
    if (val&MSYNC_CR_AUX)
        xprintf(xp, " AUX=0x%x", (val>>4)&0xf);
    if (val&MSYNC_CR_T4LATCH)
        xprintf(xp, " T4LATCH=0x%x", (val>>8)&0xf);
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
dump2_cr(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, msync.cr, &val)<0) {
        xprintf(xp, "  unable to read cr\n");
        return -1;
    }

    xprintf(xp, "  [0c] cr          =0x%04x", val);
    if (val&MSYNC2_CR_RUN)
        xprintf(xp, " RUN");
    if (val&MSYNC2_CR_LBUSY)
        xprintf(xp, " LBUSY");
    if (val&MSYNC2_CR_TAW_ENA)
        xprintf(xp, " TAW_ENA");
    if (val&MSYNC2_CR_PW)
        xprintf(xp, " PW");
    if (val&MSYNC2_CR_LATCH)
        xprintf(xp, " LATCH");
    if (val&MSYNC2_CR_AUX_OUT)
        xprintf(xp, " AUX_OUT=0x%x", (val>>6)&0x3);
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
dump1_trig_inhibit(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, msync.trig_inhibit, &val)<0) {
        xprintf(xp, "  unable to read trig_inhibit\n");
        return -1;
    }

    xprintf(xp, "  [10] trig_inh    =0x%04x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump2_trig_inhibit(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, msync2.trig_inhibit, &val)<0) {
        xprintf(xp, "  unable to read trig_inhibit\n");
        return -1;
    }

    xprintf(xp, "  [10] trig_inh    =0x%04x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump2_sum_inhibit(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val=0;

    if (lvd_i_r(dev, addr, msync2.sum_inhibit, &val)<0) {
        xprintf(xp, "  unable to read sum_inhibit\n");
        return -1;
    }

    xprintf(xp, "  [12] sum_inh     =0x%04x\n", val&0xffff);
    return 0;
}
/*****************************************************************************/
static int
dump1_log_inhibit(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, msync.log_inhibit, &val)<0) {
        xprintf(xp, "  unable to read log_inhibit\n");
        return -1;
    }

    xprintf(xp, "  [12] log_inh     =0x%04x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump2_log_inhibit(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, msync2.log_inhibit, &val)<0) {
        xprintf(xp, "  unable to read log_inhibit\n");
        return -1;
    }

    xprintf(xp, "  [14] log_inh     =0x%04x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump1_trig_input(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, msync.trig_input, &val)<0) {
        xprintf(xp, "  unable to read trig_input\n");
        return -1;
    }

    xprintf(xp, "  [14] trig_input  =0x%04x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump2_trig_input(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, msync2.trig_input, &val)<0) {
        xprintf(xp, "  unable to read trig_input\n");
        return -1;
    }

    xprintf(xp, "  [16] trig_input  =0x%04x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump1_trig_accepted(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, msync.trig_accepted, &val)<0) {
        xprintf(xp, "  unable to read trig_accepted\n");
        return -1;
    }

    xprintf(xp, "  [16] trig_accept =0x%04x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump2_trig_accepted(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, msync2.trig_accepted, &val)<0) {
        xprintf(xp, "  unable to read trig_accepted\n");
        return -1;
    }

    xprintf(xp, "  [18] trig_accept =0x%04x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump_evc(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    /* msync2.evc is at the same position */
    if (lvd_i_r(dev, addr, msync.evc, &val)<0) {
        xprintf(xp, "  unable to read evc\n");
        return -1;
    }

    xprintf(xp, "  [4c] evc         =0x%08x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump_softinfo(__attribute__((unused)) struct lvd_dev* dev, struct lvd_acard* acard, void *xp)
{
    xprintf(xp, "  daq_mode  =0x%04x\n", acard->daqmode);
    return 0;
}
/*****************************************************************************/
static int
lvd_cardstat_synch_master1(struct lvd_dev* dev, struct lvd_acard* acard,
        void *xp, int level)
{
    int addr=acard->addr;

    xprintf(xp, "ACQcard 0x%03x SYNC MASTER:\n", addr);

    switch (level) {
    case 0:
        dump1_cr(dev, addr, xp);
        dump1_sr(dev, addr, xp);
        break;
    case 1:
        dump_ident(dev, addr, xp);
        dump1_cr(dev, addr, xp);
        dump1_sr(dev, addr, xp);
        dump1_trig_input(dev, addr, xp);
        dump1_trig_accepted(dev, addr, xp);
        dump_evc(dev, addr, xp);
        dump_softinfo(dev, acard, xp);
        break;
    case 2:
    default:
        dump_ident(dev, addr, xp);
        dump_serial(dev, addr, xp);
        dump1_cr(dev, addr, xp);
        dump1_sr(dev, addr, xp);
        dump1_trig_inhibit(dev, addr, xp);
        dump1_log_inhibit(dev, addr, xp);
        dump1_trig_input(dev, addr, xp);
        dump1_trig_accepted(dev, addr, xp);
        dump_evc(dev, addr, xp);
        dump_softinfo(dev, acard, xp);
        break;
    }

    return 0;
}
/*****************************************************************************/
static int
lvd_cardstat_synch_master2(struct lvd_dev* dev, struct lvd_acard* acard,
        void *xp, int level)
{
    int addr=acard->addr;

    xprintf(xp, "ACQcard 0x%03x SYNC MASTER2:\n", addr);

    switch (level) {
    case 0:
        dump2_cr(dev, addr, xp);
        dump2_sr(dev, addr, xp);
        break;
    case 1:
        dump_ident(dev, addr, xp);
        dump2_cr(dev, addr, xp);
        dump2_sr(dev, addr, xp);
        dump2_trig_input(dev, addr, xp);
        dump2_trig_accepted(dev, addr, xp);
        dump_evc(dev, addr, xp);
        dump_softinfo(dev, acard, xp);
        break;
    case 2:
    default:
        dump_ident(dev, addr, xp);
        dump_serial(dev, addr, xp);
        dump2_cr(dev, addr, xp);
        dump2_sr(dev, addr, xp);
        dump2_trig_inhibit(dev, addr, xp);
        dump2_sum_inhibit(dev, addr, xp);
        dump2_log_inhibit(dev, addr, xp);
        dump2_trig_input(dev, addr, xp);
        dump2_trig_accepted(dev, addr, xp);
        dump_evc(dev, addr, xp);
        dump_softinfo(dev, acard, xp);
        break;
    }

    return 0;
}
/*****************************************************************************/
static void
lvd_msync_acard_free(__attribute__((unused)) struct lvd_dev* dev, struct lvd_acard* acard)
{
    free(acard->cardinfo);
    acard->cardinfo=0;
}
/*****************************************************************************/
int
lvd_msync_acard_init(__attribute__((unused)) struct lvd_dev* dev, struct lvd_acard* acard)
{
    struct msync_info *info;

    acard->freeinfo=lvd_msync_acard_free;

    acard->cardinfo=malloc(sizeof(struct msync_info));
    if (!acard->cardinfo) {
        printf("malloc acard->cardinfo(msync): %s\n", strerror(errno));
        return -1;
    }
    info=acard->cardinfo;

    info->msync2=LVD_HWtyp(acard->id)==LVD_CARDID_SYNCH_MASTER2;

    if (info->msync2) {
        acard->clear_card=lvd_clear_msynch;
        acard->start_card=lvd_start_msynch;
        acard->stop_card=lvd_stop_msynch;
        acard->cardstat=lvd_cardstat_synch_master2;
    } else {
        acard->clear_card=lvd_clear_msynch;
        acard->start_card=lvd_start_msynch;
        acard->stop_card=lvd_stop_msynch;
        acard->cardstat=lvd_cardstat_synch_master1;
    }

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
