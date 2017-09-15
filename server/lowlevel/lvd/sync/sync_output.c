/*
 * lowlevel/lvd/syncoutput/syncoutput.c
 * created 2006-Feb-07 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sync_output.c,v 1.18 2013/01/17 22:44:54 wuestner Exp $";

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

#define OSYNCH_RUNBIT 1
#define OSYNCH_PULSE 2

struct osynch_info {
    int flags;
};

RCS_REGISTER(cvsid, "lowlevel/lvd/sync")

/*****************************************************************************/
static int
lvd_start_osynch(struct lvd_dev* dev, struct lvd_acard* acard)
{
    struct osynch_info *info=acard->cardinfo;
    int addr=acard->addr;
    ems_u32 val;
    int res=0;

    val=acard->daqmode;
    if (info->flags&OSYNCH_RUNBIT)
        val|=OSYNC_CR_RUN;
    res|=lvd_i_w(dev, addr, osync.cr, val);
    res|=lvd_i_r(dev, addr, osync.trg_in, &val);
    if (val) {
        printf("lvd_start(OSYNCH 0x%x): trigger input not clear: "
                "0x%02x\n", addr, val);
        return -1;
    }
    res|=lvd_i_r(dev, addr, osync.sr, &val);
    if (val&0x300) {
        printf("lvd_start(OSYNCH 0x%x): invalid status: "
                "0x%03x\n", addr, val);
        /*return -1;*/
    }

    return res?-1:0;
}
/*****************************************************************************/
static int
lvd_stop_osynch(struct lvd_dev* dev, struct lvd_acard* acard)
{
    int res=0;

    res|=lvd_i_w(dev, acard->addr, osync.cr, 0);

    return res?-1:0;
}
/*****************************************************************************/
static int
lvd_clear_osynch(struct lvd_dev* dev, struct lvd_acard* acard)
{
    return 0;
}
/*****************************************************************************/
static plerrcode
lvd_syncoutput_init_card(struct lvd_dev* dev, struct lvd_acard* acard, int mode,
        int tpat)
{
    struct osynch_info *info=acard->cardinfo;
    int addr=acard->addr;
    ems_u32 val;

    printf("initializing syncoutput card 0x%x\n", acard->addr);

    if (tpat && LVD_FWverH(acard->id)<1) {
        printf("lvd_syncoutput_init[%d]: trigger pattern not supported\n",
                acard->addr);
        return plErr_BadModTyp;
    }

    if (lvd_i_w(dev, acard->addr, osync.cr, 0xffff)<0)
        return plErr_System;
    if (lvd_i_r(dev, acard->addr, osync.cr, &val)<0)
        return plErr_System;
    if (lvd_i_w(dev, acard->addr, osync.cr, 0)<0)
        return plErr_System;

    if (val==0xffff)     /* new version with pulse capability */
        info->flags|=OSYNCH_PULSE;
    else if (val==0x1ff) /* old version with run bit */
        info->flags|=OSYNCH_RUNBIT;
    else if (val!=0xff)  /* 'normal' new version */
        printf("lvd_syncoutput_init[%d]: illegal cr: 0x%04x\n", acard->addr, val);

    if (info->flags&OSYNCH_RUNBIT)
        printf("syncoutput[%d] needs run bit\n", acard->addr);
    if (info->flags&OSYNCH_PULSE)
        printf("syncoutput[%d] has pulse capability\n", acard->addr);

    acard->daqmode=mode;
    if (!(info->flags&OSYNCH_PULSE))
        acard->daqmode&=0xff;

    if (lvd_i_w(dev, addr, osync.bsy_tmo, 0)<0) {
        return plErr_System;
    }
    if (LVD_FWverH(acard->id)>=1) {
        if (lvd_i_w(dev, addr, osync.tpat, tpat)<0) {
            return plErr_System;
        }
    }

    printf("lvd_syncoutput_init_card: card 0x%x initialized.\n", addr);
    acard->initialized=1;

    return plOK;
}
/*****************************************************************************/
/* initialize input cards */
plerrcode
lvd_syncoutput_init(struct lvd_dev* dev, int addr, int daqmode, int tpat)
{
    struct lvd_acard *acard;
    plerrcode pres;

    if (addr<0) {
        int card;
        for (card=0; card<dev->num_acards; card++) {
            acard=dev->acards+card;
            if (acard->mtype==ZEL_LVD_OSYNCH) {
                pres=lvd_syncoutput_init_card(dev, acard, daqmode, tpat);
                if (pres!=plOK) {
                    return pres;
                }
            }
        }
        printf("lvd_syncoutput_init: all cards initialized.\n");
    } else {
        int idx=lvd_find_acard(dev, addr);
        acard=dev->acards+idx;
        if ((idx<0) || (acard->mtype!=ZEL_LVD_OSYNCH)) {
            printf("lvd_syncoutput_init_card: no OSYNC card with address 0x%x known\n", addr);
            return plErr_Program;
        }

        pres=lvd_syncoutput_init_card(dev, acard, daqmode, tpat);
        if (pres!=plOK) {
            return pres;
        }
    }

    return plOK;
}
/*****************************************************************************/
plerrcode
lvd_syncoutput_evc(struct lvd_dev *dev, int addr, ems_u32 *val)
{
    lvd_i_r(dev, addr, osync.evc, val);
    return plOK;
}
/*****************************************************************************/
plerrcode
lvd_syncoutput_sram_fill(struct lvd_dev* dev, int idx, unsigned int start,
        int num, int val)
{
    int i, end=65536;

    if (start>=56536)
        return plErr_ArgRange;
    if (num>=0)
        end=start+num;
    if (end>65536)
        return plErr_ArgRange;

#if 0
    printf("sram_fill: start=0x%x, num=0x%x val=0x%x\n", start, num, val);
#endif
    if (lvd_i_w(dev, idx, osync.sr_addr, start)<0)
        return plErr_System;
    for (i=start; i<end; i++) {
        if (lvd_i_w(dev, idx, osync.sr_data, val)<0) {
            printf("sram_fill: data=0x%02x failed\n", val);
            return plErr_System;
        }
        start++;
    }
    return plOK;
}
/*****************************************************************************/
plerrcode
lvd_syncoutput_sram_set(struct lvd_dev* dev, int idx, unsigned int addr, int val)
{
    if (addr>=65536)
        return plErr_ArgRange;

    if (lvd_i_w(dev, idx, osync.sr_addr, addr)<0)
        return plErr_System;
    if (lvd_i_w(dev, idx, osync.sr_data, val)<0)
        return plErr_System;
    return plOK;
}
/*****************************************************************************/
static plerrcode
sram_setbits(struct lvd_dev* dev, int idx, unsigned int addr, int bits)
{
    ems_u32 val;

    if (lvd_i_w(dev, idx, osync.sr_addr, addr)<0)
        return plErr_System;
    if (lvd_i_r(dev, idx, osync.sr_data, &val)<0)
        return plErr_System;

    val|=bits;
    if (lvd_i_w(dev, idx, osync.sr_addr, addr)<0)
        return plErr_System;
    if (lvd_i_w(dev, idx, osync.sr_data, val)<0)
        return plErr_System;
    return plOK;
}
/*****************************************************************************/
plerrcode
lvd_syncoutput_sram_setbits(struct lvd_dev* dev, int idx, int addr, int bits)
{
    plerrcode pres=plOK;

    if (addr>=65536)
        return plErr_ArgRange;

    if (addr<0) {
        int i;
        for (i=0; i<65536; i++) {
            if ((pres=sram_setbits(dev, idx, i, bits))!=plOK)
                return pres;
        }
    } else {
        pres=sram_setbits(dev, idx, addr, bits);
    }
    return pres;
}
/*****************************************************************************/
static plerrcode
sram_resbits(struct lvd_dev* dev, int idx, unsigned int addr, int bits)
{
    ems_u32 val;

    if (lvd_i_w(dev, idx, osync.sr_addr, addr)<0)
        return plErr_System;
    if (lvd_i_r(dev, idx, osync.sr_data, &val)<0)
        return plErr_System;

    val&=~bits;
    if (lvd_i_w(dev, idx, osync.sr_addr, addr)<0)
        return plErr_System;
    if (lvd_i_w(dev, idx, osync.sr_data, val)<0)
        return plErr_System;
    return plOK;
}
/*****************************************************************************/
plerrcode
lvd_syncoutput_sram_resbits(struct lvd_dev* dev, int idx, int addr, int bits)
{
    plerrcode pres=plOK;

    if (addr>=65536)
        return plErr_ArgRange;

    if (addr<0) {
        int i;
        for (i=0; i<65536; i++) {
            if ((pres=sram_resbits(dev, idx, i, bits))!=plOK)
                return pres;
        }
    } else {
        pres=sram_setbits(dev, idx, addr, bits);
    }
    return pres;
}
/*****************************************************************************/
plerrcode
lvd_syncoutput_sram_bitmasks(struct lvd_dev* dev, int idx, ems_u32* masks)
{
    plerrcode pres=plErr_System;
    ems_u8* buf;
    int bit, i;

    buf=(ems_u8*)calloc(65536, 1);
    if (!buf)
        return plErr_NoMem;

    for (bit=0; bit<8; bit++) {
        ems_u32 mask=masks[bit];
        ems_u8  pat=1<<bit;
        for (i=0; i<65536; i++) {
            if (mask&i)
                buf[i]|=pat;
        }
    }
    if (lvd_i_w(dev, idx, osync.sr_addr, 0)<0)
        goto error;
    for (i=0; i<65536; i++) {
        if (lvd_i_w(dev, idx, osync.sr_data, buf[i])<0)
            goto error;
    }
    pres=plOK;

error:
    free(buf);
    return pres;
}
/*****************************************************************************/
plerrcode
lvd_syncoutput_sram_get(struct lvd_dev* dev, int idx, unsigned int addr,
        ems_u32* val)
{
    if (addr>=65536)
        return plErr_ArgRange;

    if (lvd_i_w(dev, idx, osync.sr_addr, addr)<0)
        return plErr_System;
    if (lvd_i_r(dev, idx, osync.sr_data, val)<0)
        return plErr_System;
    return plOK;
}
/*****************************************************************************/
static plerrcode
sram_check1(struct lvd_dev* dev, int idx, int val)
{
    ems_u32 rval;
    int i;

    if (lvd_i_w(dev, idx, osync.sr_addr, 0)<0)
        return plErr_System;
    for (i=0; i<65536; i++) {
        if (lvd_i_w(dev, idx, osync.sr_data, val)<0)
            return plErr_System;
    }
    if (lvd_i_w(dev, idx, osync.sr_addr, 0)<0)
        return plErr_System;
    for (i=0; i<65536; i++) {
        if (lvd_i_r(dev, idx, osync.sr_data, &rval)<0)
            return plErr_System;
        if (rval!=val) {
            printf("sram_check1: addr=0x%04x: 0x%04x ==> 0x%04x\n", i,
                    i, rval);
            return plErr_HW;
        }
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
sram_check2(struct lvd_dev* dev, int idx)
{
    ems_u32 rval;
    int i;

    if (lvd_i_w(dev, idx, osync.sr_addr, 0)<0)
        return plErr_System;
    for (i=0; i<65536; i++) {
        if (lvd_i_w(dev, idx, osync.sr_data, i)<0)
            return plErr_System;
    }

    if (lvd_i_w(dev, idx, osync.sr_addr, 0)<0)
        return plErr_System;
    for (i=0; i<65536; i++) {
        if (lvd_i_r(dev, idx, osync.sr_data, &rval)<0)
            return plErr_System;
        if (rval!=i) {
            printf("sram_check2: addr=0x%04x: 0x%04x ==> 0x%04x\n", i,
                    i, rval);
            return plErr_HW;
        }
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
sram_check3(struct lvd_dev* dev, int idx)
{
    ems_u32 rval;
    int i;

    for (i=65535; i>=0; i--) {
        if (lvd_i_w(dev, idx, osync.sr_addr, i)<0)
            return plErr_System;
        if (lvd_i_w(dev, idx, osync.sr_data, 65535-i)<0)
            return plErr_System;
    }

    if (lvd_i_w(dev, idx, osync.sr_addr, 0)<0)
        return plErr_System;
    for (i=0; i<65536; i++) {
        if (lvd_i_r(dev, idx, osync.sr_data, &rval)<0)
            return plErr_System;
        if (rval!=65535-i) {
            printf("sram_check3: addr=0x%04x: 0x%04x ==> 0x%04x\n", i,
                    65535-i, rval);
            return plErr_HW;
        }
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
sram_check4(struct lvd_dev* dev, int idx)
{
    ems_u32 rval;
    int i;

    if (lvd_i_w(dev, idx, osync.sr_addr, 0)<0)
        return plErr_System;
    for (i=0; i<65536; i++) {
        if (lvd_i_w(dev, idx, osync.sr_data, i)<0)
            return plErr_System;
    }

    for (i=65535; i>=0; i--) {
        if (lvd_i_w(dev, idx, osync.sr_addr, i)<0)
            return plErr_System;
        if (lvd_i_r(dev, idx, osync.sr_data, &rval)<0)
            return plErr_System;
        if (rval!=i) {
            printf("sram_check4: addr=0x%04x: 0x%04x ==> 0x%04x\n", i,
                    i, rval);
            return plErr_HW;
        }
    }
    return plOK;
}
/*****************************************************************************/
plerrcode
lvd_syncoutput_sram_check(struct lvd_dev* dev, int idx)
{
    int vals[]={0, 0xffff, 0xaaaa, 0x5555};
    int i;
    plerrcode pres;

    for (i=0; i<sizeof(vals)/sizeof(int); i++) {
        if ((pres=sram_check1(dev, idx, vals[i]))!=plOK)
            return pres;
    }

    if ((pres=sram_check2(dev, idx))!=plOK)
        return pres;
    if ((pres=sram_check3(dev, idx))!=plOK)
        return pres;
    if ((pres=sram_check4(dev, idx))!=plOK)
        return pres;

    return plOK;
}
/*****************************************************************************/
plerrcode
lvd_syncoutput_sram_dump(struct lvd_dev* dev, int idx, unsigned int start,
        unsigned int num)
{
    ems_u32 val;
    int i, end;


    if (start>=65536)
        return plErr_ArgRange;
    end=start+num;
    if (end>65536)
        end=65536;
    printf("syncoutput_sram_dump: addr=%d\n", idx);
    printf("  start=%u end=%d num=%u\n", start, end, num);
    if (lvd_i_w(dev, idx, osync.sr_addr, start)<0) {
        return plErr_System;
    }
    for (i=start; i<end; i++) {
        if (lvd_i_r(dev, idx, osync.sr_data, &val)<0) {
            return plErr_System;
        }
        printf("  [%02x] %04x\n", i, val);
    }
    return plOK;
}
/*****************************************************************************/
/*****************************************************************************/
static int
dump_ident(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, osync.ident, &val)<0) {
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
    res=lvd_i_r(dev, addr, osync.serial, &val);
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
dump_busy(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val, cr;

    if (lvd_i_r(dev, addr, osync.busy, &val)<0) {
        xprintf(xp, "  unable to read busy\n");
        return -1;
    }
    if (lvd_i_r(dev, addr, osync.cr, &cr)<0) {
        xprintf(xp, "  unable to read cr\n");
        return -1;
    }
    cr&=OSYNC_CR_TENA;

    xprintf(xp, "  [06] busy        =0x%04x", val);
    xprintf(xp, " input:0x%02x local:0x%02x\n", (val>>8)&cr, val&cr);
    return 0;
}
/*****************************************************************************/
static int
dump_sr(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, osync.sr, &val)<0) {
        xprintf(xp, "  unable to read sr\n");
        return -1;
    }

    xprintf(xp, "  [0e] sr          =0x%04x", val);
    if (val&OSYNC_SR_TRG_SUM)
        xprintf(xp, " TRG_SUM");
    if (val&OSYNC_SR_SEQ_ERROR)
        xprintf(xp, " SEQ_ERROR");
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
dump_cr(struct lvd_dev* dev, struct lvd_acard* acard, void *xp)
{
    struct osynch_info *info=acard->cardinfo;
    ems_u32 val;

    if (lvd_i_r(dev, acard->addr, osync.cr, &val)<0) {
        xprintf(xp, "  unable to read cr\n");
        return -1;
    }

    xprintf(xp, "  [0c] cr          =0x%04x", val);
    if (val&OSYNC_CR_TENA)
        xprintf(xp, " TENA=0x%02x", val&OSYNC_CR_TENA);
    if (info->flags&OSYNCH_RUNBIT && val&OSYNC_CR_RUN)
        xprintf(xp, " RUN");
    if (info->flags&OSYNCH_PULSE && val&OSYNC_CR_PULSE)
        xprintf(xp, " PULSE=0x%02x", (val&OSYNC_CR_PULSE)>>8);
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
dump_trg_in(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, osync.trg_in, &val)<0) {
        xprintf(xp, "  unable to read trg_in\n");
        return -1;
    }

    xprintf(xp, "  [10] trg_in      =0x%04x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump_tpat(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, osync.tpat, &val)<0) {
        xprintf(xp, "  unable to read tpat\n");
        return -1;
    }

    xprintf(xp, "  [10] tpat        =0x%04x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump_evc(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, osync.evc, &val)<0) {
        xprintf(xp, "  unable to read evc\n");
        return -1;
    }

    xprintf(xp, "  [1c] evc         =0x%08x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump_sr_addr(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, osync.sr_addr, &val)<0) {
        xprintf(xp, "  unable to read sr_addr\n");
        return -1;
    }

    xprintf(xp, "  [12] sr_addr     =0x%04x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump_bsy_tmo(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, osync.bsy_tmo, &val)<0) {
        xprintf(xp, "  unable to read bsy_tmo\n");
        return -1;
    }

    xprintf(xp, "  [04] bsy_tmo     =0x%04x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump_softinfo(struct lvd_dev* dev, struct lvd_acard* acard, void *xp)
{
    xprintf(xp, "  daq_mode  =0x%04x\n", acard->daqmode);
    return 0;
}
/*****************************************************************************/
static int
lvd_cardstat_synch_output(struct lvd_dev* dev, struct lvd_acard* acard,
        void *xp, int level)
{
    int addr=acard->addr;

    xprintf(xp, "ACQcard 0x%03x SYNC OUTPUT:\n", addr);

    switch (level) {
    case 0:
        dump_busy(dev, addr, xp);
        dump_cr(dev, acard, xp);
        dump_sr(dev, addr, xp);
        dump_trg_in(dev, addr, xp);
        dump_tpat(dev, addr, xp);
        dump_evc(dev, addr, xp);
        break;
    case 1:
        dump_ident(dev, addr, xp);
        dump_busy(dev, addr, xp);
        dump_cr(dev, acard, xp);
        dump_sr(dev, addr, xp);
        dump_trg_in(dev, addr, xp);
        dump_tpat(dev, addr, xp);
        dump_evc(dev, addr, xp);
        dump_softinfo(dev, acard, xp);
        break;
    case 2:
    default:
        dump_ident(dev, addr, xp);
        dump_serial(dev, addr, xp);
        dump_bsy_tmo(dev, addr, xp);
        dump_busy(dev, addr, xp);
        dump_cr(dev, acard, xp);
        dump_sr(dev, addr, xp);
        dump_trg_in(dev, addr, xp);
        dump_sr_addr(dev, addr, xp);
        dump_tpat(dev, addr, xp);
        dump_evc(dev, addr, xp);
        dump_softinfo(dev, acard, xp);
        break;
    }

    return 0;
}
/*****************************************************************************/
static void
lvd_osync_acard_free(struct lvd_dev* dev, struct lvd_acard* acard)
{
    free(acard->cardinfo);
    acard->cardinfo=0;
}
/*****************************************************************************/
int
lvd_osync_acard_init(struct lvd_dev* dev, struct lvd_acard* acard)
{
    struct osynch_info *info;

    acard->freeinfo=lvd_osync_acard_free;
    acard->clear_card=lvd_clear_osynch;
    acard->start_card=lvd_start_osynch;
    acard->stop_card=lvd_stop_osynch;
    acard->cardstat=lvd_cardstat_synch_output;
    acard->cardinfo=malloc(sizeof(struct osynch_info));
    if (!acard->cardinfo) {
        printf("malloc acard->cardinfo(osync): %s\n", strerror(errno));
        return -1;
    };
    info=acard->cardinfo;
    info->flags=0;

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
