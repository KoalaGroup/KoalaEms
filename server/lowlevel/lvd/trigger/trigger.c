/*
 * lowlevel/lvd/trig/trigger.c
 * created 2009-Nov-10 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: trigger.c,v 1.6 2013/01/17 22:44:55 wuestner Exp $";

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
#include "../lvdbus.h"
#include "../lvd_access.h"
#include "../lvd_initfuncs.h"
#include "trigger.h"
#include "trig_i2c.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

struct trig_info {
    int trigver;
};

typedef plerrcode (*trigfun)(struct lvd_acard*, ems_u32*, int idx);

RCS_REGISTER(cvsid, "lowlevel/lvd/trigger")

/*****************************************************************************/
static plerrcode
access_trigfun(struct lvd_dev* dev, struct lvd_acard *acard, ems_u32 *val,
    int idx,
    plerrcode (**funlist)(struct lvd_acard*, ems_u32*, int),
    int numfun, const char *txt)
{
    struct trig_info *info=(struct trig_info*)acard->cardinfo;

    if (numfun>info->trigver && funlist[info->trigver]) {
        return funlist[info->trigver](acard, val, idx);
    } else {
        return plErr_BadModTyp;
    }
}
/*****************************************************************************/
static struct lvd_acard*
find_acard(struct lvd_dev* dev, int addr, const char *txt)
{
    struct lvd_acard *acard;
    int idx;

    idx=lvd_find_acard(dev, addr);
    acard=dev->acards+idx;
    if (idx<0 || acard->mtype!=ZEL_LVD_TRIGGER) {
        complain("%s: no trigger card with address 0x%x known",
                txt?txt:"access_trig",
                addr);
        return 0;
    }
    return acard;
}
/*****************************************************************************/
static plerrcode
access_trig(struct lvd_dev* dev, int addr, ems_u32 *val,
    plerrcode (**funlist)(struct lvd_acard*, ems_u32*, int),
    int numfun,
    ems_u32 mtype, const char *txt)
{
    struct lvd_acard *acard;
    int idx;

    idx=lvd_find_acard(dev, addr);
    acard=dev->acards+idx;
    if (idx<0 || acard->mtype!=ZEL_LVD_TRIGGER) {
        complain("%s: no trigger card with address 0x%x known",
                txt?txt:"access_trig",
                addr);
        return plErr_Program;
    }
    if (mtype && acard->mtype!=mtype)
        return plErr_BadModTyp;

    return access_trigfun(dev, acard, val, 0, funlist, numfun, txt);
}
/*****************************************************************************/
static plerrcode
access_trigs(struct lvd_dev* dev, int addr, ems_u32 *val,
    plerrcode (**funlist)(struct lvd_acard*, ems_u32*, int),
    int numfun,
    ems_u32 mtype, const char *txt)
{
    plerrcode pres=plOK;

    if (addr<0) {
        int idx;
        for (idx=0; idx<dev->num_acards; idx++) {
            struct lvd_acard *acard=dev->acards+idx;
            if (acard->mtype==ZEL_LVD_TRIGGER &&
                    (!mtype || acard->mtype==mtype)) {
                pres=access_trigfun(dev, acard, val, acard->addr,
                    funlist, numfun, txt);
                if (pres!=plOK)
                    return pres;
            }
        }
    } else {
        pres=access_trig(dev, addr, val, funlist, numfun, mtype, txt);
    }
    return pres;
}
/*****************************************************************************/
plerrcode
lvd_trig_read_mem(struct lvd_dev* dev, int addr,
    int selector, int layer, int sector, int adr,
    int num, ems_u32 *data)
{
    struct lvd_acard *acard;
    int address, res=0, i;

    if (!(acard=find_acard(dev, addr, "lvd_trig_read_mem")))
        return plErr_Program;

    address=0;
    address|=(selector&1)<<15;
    address|=(layer&0x7)<<12;
    address|=(sector&0x1f)<<7;
    address|=(adr>>1)&0x7f;

#if 0
    printf("trig_read_mem: selector %d layer %d sector 0x%x addr 0x%x\n",
        selector, layer, sector, address);
#endif

    res|=lvd_i_w(acard->dev, acard->addr, trig.mem_select, address);
    for (i=0; i<num; i++)
        res|=lvd_i_r(acard->dev, acard->addr, trig.mem_data_s[0], data+i);

    return res?plErr_HW:plOK;
}
/*****************************************************************************/
plerrcode
lvd_trig_write_mem(struct lvd_dev* dev, int addr,
    int selector, int layer, int sector, int adr,
    int num, ems_u32 *data)
{
    struct lvd_acard *acard;
    int address, res=0, i;

    if (!(acard=find_acard(dev, addr, "lvd_trig_write_mem")))
        return plErr_Program;

    address=0;
    address|=(selector&1)<<15;
    address|=(layer&0x7)<<12;
    address|=(sector&0x1f)<<7;
    address|=(adr>>1)&0x7f;

#if 0
    printf("trig_write_mem: selector %d layer %d sector 0x%x addr 0x%x\n",
        selector, layer, sector, address);
#endif

    res|=lvd_i_w(acard->dev, acard->addr, trig.mem_select, address);
    for (i=0; i<num; i++)
        res|=lvd_i_w(acard->dev, acard->addr, trig.mem_data_s[0], data[i]);

    return res?plErr_HW:plOK;
}
/*****************************************************************************/
plerrcode
lvd_trig_read_pi(struct lvd_dev* dev, int addr,
    int adr, int num, ems_u32 *data)
{
    struct lvd_acard *acard;
    int res=0, i;

    if (!(acard=find_acard(dev, addr, "lvd_trig_read_pi")))
        return plErr_Program;

    res|=lvd_i_w(acard->dev, acard->addr, trig.pi_addr, adr);
    for (i=0; i<num; i++)
        res|=lvd_i_r(acard->dev, acard->addr, trig.pi_data_s[0], data+i);

    return res?plErr_HW:plOK;
}
/*****************************************************************************/
plerrcode
lvd_trig_write_pi(struct lvd_dev* dev, int addr,
    int adr, int num, ems_u32 *data)
{
    struct lvd_acard *acard;
    int res=0, i;

    if (!(acard=find_acard(dev, addr, "lvd_trig_write_pi")))
        return plErr_Program;

    res|=lvd_i_w(acard->dev, acard->addr, trig.pi_addr, adr);
    for (i=0; i<num; i++)
        res|=lvd_i_w(acard->dev, acard->addr, trig.pi_data_s[0], data[i]);

    return res?plErr_HW:plOK;
}
/*****************************************************************************/
static plerrcode
lvd_trig0_rmid(struct lvd_acard *acard, ems_u32* IDs, int xxx)
{
    int res=0, idx=*IDs, i;
    if (idx<0) {
        for (i=0; i<16; i++) {
            res|=lvd_i_w(acard->dev, acard->addr, trig.rm_id, i);
            res|=lvd_i_r(acard->dev, acard->addr, trig.rm_id, IDs+i);
        }
    } else {
        res|=lvd_i_w(acard->dev, acard->addr, trig.rm_id, idx);
        res|=lvd_i_r(acard->dev, acard->addr, trig.rm_id, IDs);
    }
    return res?plErr_HW:plOK;
}
plerrcode
lvd_trig_rmid(struct lvd_dev* dev, int addr, int idx, ems_u32* IDs)
{
    static trigfun funs[]={lvd_trig0_rmid};
    *IDs=idx;
    return access_trig(dev, addr, IDs,
                funs, 1,
                0,
                "lvd_trig_rmid");
}
/*****************************************************************************/
static plerrcode
lvd_trig0_get_sync(struct lvd_acard *acard, ems_u32* pattern, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, trig.rx_sync, pattern+idx)
            ?plErr_HW:plOK;
}
plerrcode
lvd_trig_get_sync(struct lvd_dev* dev, int addr, ems_u32* pattern)
{
    static trigfun funs[]={lvd_trig0_get_sync};
    if (addr<0)
        memset(pattern, 0, 16*sizeof(ems_u32));
    return access_trigs(dev, addr, pattern,
                funs, 1,
                0,
                "lvd_trig_get_sync");
}
/*****************************************************************************/
static plerrcode
lvd_trig0_set_ena(struct lvd_acard *acard, ems_u32 *pattern, int xxx)
{
    return lvd_i_w(acard->dev, acard->addr, trig.tx_ena, *pattern)
            ?plErr_HW:plOK;
}
plerrcode
lvd_trig_set_ena(struct lvd_dev* dev, int addr, ems_u32 pattern)
{
    static trigfun funs[]={lvd_trig0_set_ena};
    return access_trigs(dev, addr, &pattern,
                funs, 1,
                0,
                "lvd_trig_set_ena");
}
/*****************************************************************************/
static plerrcode
lvd_trig0_get_ena(struct lvd_acard *acard, ems_u32* pattern, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, trig.tx_ena, pattern+idx)
            ?plErr_HW:plOK;
}
plerrcode
lvd_trig_get_ena(struct lvd_dev* dev, int addr, ems_u32* pattern)
{
    static trigfun funs[]={lvd_trig0_get_ena};
    if (addr<0)
        memset(pattern, 0, 16*sizeof(ems_u32));
    return access_trigs(dev, addr, pattern,
                funs, 1,
                0,
                "lvd_trig_get_ena");
}
/*****************************************************************************/
static int
lvd_start_trig(struct lvd_dev* dev, struct lvd_acard* acard)
{
    int res;

    res=lvd_i_w(acard->dev, acard->addr, all.cr, acard->daqmode);
    return res;
}
/*****************************************************************************/
static int
lvd_stop_trig(struct lvd_dev* dev, struct lvd_acard* acard)
{
    int res;

    res=lvd_i_w(acard->dev, acard->addr, all.cr, 0);
    return res;
}
/*****************************************************************************/
static int
lvd_clear_trig(struct lvd_dev* dev, struct lvd_acard* acard)
{
    int res=0, addr=acard->addr;
    ems_u32 sr;
    res|=lvd_i_r(dev, addr, all.sr, &sr);
    res|=lvd_i_w(dev, addr, all.sr, sr);
    return res?-1:0;
}
/*****************************************************************************/
/* initialize one trig card */
static plerrcode
lvd_trigX_init(struct lvd_acard* acard, ems_u32 *mode, int xxx)
{
    int res=0, addr=acard->addr;

    printf("initializing trig card 0x%x, mode 0x%04x\n", addr, *mode);

    acard->daqmode=*mode;
    //res|=lvd_i_w(dev, addr, trig.tdc_range, dev->tdc_range);

    if (res)
        return plErr_System;

    //printf("trig_init_card: input card 0x%x initialized.\n", addr);
    acard->initialized=1;

    return plOK;
}

/* initialize one or more trig cards */
plerrcode
lvd_trig_init(struct lvd_dev* dev, int addr, int daqmode)
{
    static trigfun funs[]={lvd_trigX_init, lvd_trigX_init};
    plerrcode pres;
    ems_u32 mode=daqmode;

    pres=access_trigs(dev, addr, &mode,
                funs, 2,
                0,
                "lvd_trig_init");

    if (pres!=plOK)
        return pres;

    printf("trig_init: all input cards initialized.\n");

    return plOK;
}
/*****************************************************************************/
/*****************************************************************************/
static int
dump_ident(struct lvd_acard *acard, struct trig_info *info, void *xp)
{
    ems_u32 val;
    int res, a=0;

    res=lvd_i_r(acard->dev, acard->addr, all.ident, &val);
    if (res) {
        xprintf(xp, "  unable to read ident\n");
        return -1;
    }

    xprintf(xp, "  [%02x] ident       =0x%04x (type=0x%x version=0x%x)\n",
            a, val, val&0xff, (val>>8)&0xff);
    return 0;
}
/*****************************************************************************/
static int
dump_serial(struct lvd_acard *acard, struct trig_info *info, void *xp)
{
    ems_u32 val;
    int res, a=2;

    acard->dev->silent_errors=1;
    res=lvd_i_r(acard->dev, acard->addr, all.serial, &val);
    acard->dev->silent_errors=0;
    xprintf(xp, "  [%02x] serial      ", a);
    if (res<0) {
        xprintf(xp, ": not set\n");
    } else {
        xprintf(xp, "=%d\n", val);
    }
    return 0;
}
/*****************************************************************************/
static void decode_cr(ems_u32 cr, void *xp)
{
    if (cr&TRIG_CR_ENA)
        xprintf(xp, " ENA");
    if (cr&TRIG_CR_TRG_DATA)
        xprintf(xp, " TRG_DATA");
    if (cr&TRIG_CR_RAW_DATA)
        xprintf(xp, " RAW_DATA");
    if (cr&TRIG_CR_RAW_EXT)
        xprintf(xp, " RAW_EXT");
    if (cr&TRIG_CR_TRG_TST)
        xprintf(xp, " TRG_TST");
}
/*****************************************************************************/
static int
dump_cr(struct lvd_acard *acard, struct trig_info *info, void *xp)
{
    ems_u32 cr;
    int res, a=0xc;

    res=lvd_i_r(acard->dev, acard->addr, all.cr, &cr);
    if (res) {
        xprintf(xp, "  unable to read cr\n");
        return -1;
    }

    xprintf(xp, "  [%02x] cr          =0x%04x", a, cr);
    decode_cr(cr, xp);
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
dump_cr_saved(struct lvd_acard *acard, struct trig_info *info, void *xp)
{
    if (!acard->initialized)
        return 0;
    xprintf(xp, " saved cr          =0x%04x", acard->daqmode);
    decode_cr(acard->daqmode, xp);
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
dump_sr(struct lvd_acard *acard, struct trig_info *info, void *xp)
{
    ems_u32 val;
    int res, a=0xe;

    res=lvd_i_r(acard->dev, acard->addr, all.sr, &val);
    if (res) {
        xprintf(xp, "  unable to read sr\n");
        return -1;
    }

    xprintf(xp, "  [%02x] sr          =0x%04x\n", a, val);
    return 0;
}
/*****************************************************************************/
static int
dump_rx_sync(struct lvd_acard *acard, struct trig_info *info, void *xp)
{
    ems_u32 val;
    int res, a=0x1c;

    res=lvd_i_r(acard->dev, acard->addr, trig.rx_sync, &val);
    if (res) {
        xprintf(xp, "  unable to read rx_sync\n");
        return -1;
    }

    xprintf(xp, "  [%02x] rx_sync     =0x%04x\n", a, val);
    return 0;
}
/*****************************************************************************/
static int
dump_fw_ver(struct lvd_acard *acard, struct trig_info *info, void *xp)
{
    ems_u32 val;
    int res, a=0x1a;

    res=lvd_i_r(acard->dev, acard->addr, trig.fw_ver, &val);
    if (res) {
        xprintf(xp, "  unable to read firmware version\n");
        return -1;
    }

    xprintf(xp, "  [%02x] firmware    =0x%04x (type=0x%x version=0x%x)\n",
            a, val, val&0xff, (val>>8)&0xff);
    return 0;
}
/*****************************************************************************/
static int
dump_rm_id(struct lvd_acard *acard, struct trig_info *info, void *xp)
{
    ems_u32 val;
    int res, a=0x1e, i;

    xprintf(xp, "  [%02x] rm_id       ", a);
    for (i=0; i<16; i++) {
        if (i==8)
            xprintf(xp, "\n                   ");
        res=lvd_i_w(acard->dev, acard->addr, trig.rm_id, i);
        res|=lvd_i_r(acard->dev, acard->addr, trig.rm_id, &val);
        if (res) {
            xprintf(xp, "  unable to read rm_id\n");
            return -1;
        }
        xprintf(xp, " %04x", val);
    }
    xprintf(xp, "\n");

    return 0;
}
/*****************************************************************************/
static int
dump_tx_ena(struct lvd_acard *acard, struct trig_info *info, void *xp)
{
    ems_u32 val;
    int res, a=0x44;

    res=lvd_i_r(acard->dev, acard->addr, trig.tx_ena, &val);
    if (res) {
        xprintf(xp, "  unable to read tx_ena\n");
        return -1;
    }

    xprintf(xp, "  [%02x] tx_ena      =0x%04x\n", a, val);
    return 0;
}
/*****************************************************************************/
static int
dump_port_status(struct lvd_acard *acard, struct trig_info *info, int port,
    void *xp)
{
    struct i2c_addr i2c;
    ems_u32 d0[256];
    ems_u32 d1[256];
    size_t cardoffs;

    cardoffs=ofs(struct lvd_bus1100, in_card[acard->addr&0xf]);
    i2c.dev =acard->dev;
    i2c.csr =cardoffs+ofs(union lvd_in_card, trig.i2c_csr);
    i2c.data=cardoffs+ofs(union lvd_in_card, trig.i2c_data);
    i2c.wr  =cardoffs+ofs(union lvd_in_card, trig.i2c_wr);
    i2c.rd  =cardoffs+ofs(union lvd_in_card, trig.i2c_rd);

    /* select port */
    if (lvd_i_w(acard->dev, acard->addr, trig.i2c_sel, port)<0) {
        xprintf(xp, "  unable to select port\n");
        return -1;
    }

    if (trig_i2c_read(&i2c, 0, 0, 256, d0))
        return -1;
    if (trig_i2c_read(&i2c, 1, 0, 256, d1))
        return -1;

    trig_i2c_decode_mem(d0);
    trig_i2c_decode_dia(d1);

    return 0;
}
/*****************************************************************************/
static int
dump_ports_status(struct lvd_acard *acard, struct trig_info *info, void *xp)
{
    int port;
    for (port=0; port<16; port++)
        dump_port_status(acard, info, port, xp);
    return 0;
}
/*****************************************************************************/
static int
lvd_cardstat_trig(struct lvd_dev* dev, struct lvd_acard* acard, void *xp,
    int level)
{
    struct trig_info *info=(struct trig_info*)acard->cardinfo;

    xprintf(xp, "ACQcard 0x%03x TRIGGER:\n", acard->addr);

    switch (level) {
    case 0:
        dump_cr(acard, info, xp);
        dump_sr(acard, info, xp);
        break;
    case 1:
        dump_ident(acard, info, xp);
        dump_cr(acard, info, xp);
        dump_cr_saved(acard, info, xp);
        dump_sr(acard, info, xp);
        dump_rx_sync(acard, info, xp);
        dump_tx_ena(acard, info, xp);
        break;
    case 2:
    default:
        dump_ident(acard, info, xp);
        dump_serial(acard, info, xp);
        dump_cr(acard, info, xp);
        dump_cr_saved(acard, info, xp);
        dump_sr(acard, info, xp);
        dump_rx_sync(acard, info, xp);
        dump_fw_ver(acard, info, xp);
        dump_rm_id(acard, info, xp);
        dump_tx_ena(acard, info, xp);
        break;
    case 3:
        dump_ident(acard, info, xp);
        dump_serial(acard, info, xp);
        dump_cr(acard, info, xp);
        dump_cr_saved(acard, info, xp);
        dump_sr(acard, info, xp);
        dump_rx_sync(acard, info, xp);
        dump_fw_ver(acard, info, xp);
        dump_rm_id(acard, info, xp);
        dump_tx_ena(acard, info, xp);
        dump_ports_status(acard, info, xp);
        break;
    }

    return 0;
}
/*****************************************************************************/
static void
lvd_trig_acard_free(struct lvd_dev* dev, struct lvd_acard* acard)
{
    free(acard->cardinfo);
    acard->cardinfo=0;
}
/*****************************************************************************/
int
lvd_trig_acard_init(struct lvd_dev* dev, struct lvd_acard* acard)
{
    struct trig_info *info;

    acard->freeinfo=lvd_trig_acard_free;

    acard->cardinfo=malloc(sizeof(struct trig_info));
    if (!acard->cardinfo) {
        printf("malloc acard->cardinfo(trig): %s\n", strerror(errno));
        return -1;
    }
    info=acard->cardinfo;
    /*
        trig versions
        0: prototype
    */
    info->trigver=LVD_FWverH(acard->id);
    info->trigver=0;
    if (info->trigver>0) {
        complain("unknown trigger version 0x%04x addr %x",
            acard->id, acard->addr);
        return -1;
    }

    acard->clear_card=lvd_clear_trig;
    acard->start_card=lvd_start_trig;
    acard->stop_card=lvd_stop_trig;
    acard->cardstat=lvd_cardstat_trig;

    if (lvd_i_w(acard->dev, acard->addr, all.cr, 0))
        return -1;

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
