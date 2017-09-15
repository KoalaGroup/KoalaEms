/*
 * lowlevel/lvd/vertex/vertex_dump.c
 * created 2008-Jun-06 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: vertex_dump.c,v 1.5 2013/01/17 22:44:55 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <stdio.h>
#include <errno.h>

#include <modultypes.h>
#include <xprintf.h>
#include <rcs_ids.h>
#include "vertex.h"
#include "../lvd_access.h"

RCS_REGISTER(cvsid, "lowlevel/lvd/vertex")

/*****************************************************************************/
static void
dump_ident(struct lvd_acard* acard, void *xp)
{
    ems_u32 val;
    int a=0;

    if (lvd_i_r(acard->dev, acard->addr, vertex.ident, &val)<0) {
        xprintf(xp, "  unable to read ident\n");
        return;
    }
    xprintf(xp, "  [%02x] ident    =0x%04x (type=0x%x version=0x%x)\n",
            a, val, val&0xff, (val>>8)&0xff);
}
/*****************************************************************************/
static void
dump_serial(struct lvd_acard* acard, void *xp)
{
    ems_u32 val;
    int res, a=2;

    acard->dev->silent_errors=1;
    res=lvd_i_r(acard->dev, acard->addr, vertex.serial, &val);
    acard->dev->silent_errors=0;
    xprintf(xp, "  [%02x] serial   ", a);
    if (res<0) {
        xprintf(xp, ": not set\n");
    } else {
        xprintf(xp, "=%d\n", val);
    }
}
/*****************************************************************************/
static void
dump_seq_csr(struct lvd_acard* acard, void *xp)
{
    static const char *seq_csr_bits[]={
        "LV_HALT",
        "LV_HALTED",
        "LV_STOVFL",
        "LV_STUNFL",
        "LV_OPERR",
        "LV_FEMPTY",
        "LV_DIGTST_POL",
        "LV_BUSY",
        "HV_HALTED",
        "HV_IDLE",
        "HV_STOVFL",
        "HV_STUNFL",
        "HV_OPERR",
        "HV_FEMPTY",
        "HV_DIGTST_POL"
        "HV_BUSY"
    };
    ems_u32 val;
    int i, a=4;

    if (lvd_i_r(acard->dev, acard->addr, vertex.seq_csr, &val)<0) {
        xprintf(xp, "  unable to read seq_csr\n");
        return;
    }

    xprintf(xp, "  [%02x] seq_csr  =0x%04x", a, val);
    for (i=0; i<16; i++) {
        if (val&(1<<i))
            xprintf(xp, " %s", seq_csr_bits[i]);
    }
    xprintf(xp, "\n");
}
/*****************************************************************************/
static void
dump_aclk_par(struct lvd_acard *acard, void *xp)
/* VATA only */
{
    ems_u32 val;
    int a=0x06;

    if (acard->mtype == ZEL_LVD_ADC_VERTEXM3)
        return;

    if (lvd_i_r(acard->dev, acard->addr, vertex.aclk_par, &val)<0) {
        xprintf(xp, "  unable to read aclk_par\n");
        return;
    }
    xprintf(xp, "  [%02x] aclk_par =0x%04x\n", a, val);
}
/*****************************************************************************/
static void
dump_cr(struct lvd_acard *acard, void *xp)
{
    static const char *cr_bits[]={
        "RUN",
        "RAW",
        "VERBOSE",
        "TM_MODE",
        "LV_INH",
        "HV_INH",
        "TRG_INH",
        "VA_TRG",
        "MON_0",
        "MON_1",
        "MON_2",
        "UDW",
        "NOCOM",
        "MAINT_I2C"
    };
    ems_u32 val;
    int i, a=0xc;

    if (lvd_i_r(acard->dev, acard->addr, vertex.cr, &val)<0) {
        xprintf(xp, "  unable to read cr\n");
        return;
    }

    xprintf(xp, "  [%02x] cr       =0x%04x", a, val);
    for (i=0; i<8; i++) {
        if ((val&0x38ff)&(1<<i))
            xprintf(xp, " %s", cr_bits[i]);
    }
    if (val&VTX_CR_MON)
        xprintf(xp, " MON=%0x", (val&VTX_CR_MON)>>8);
    xprintf(xp, "\n");
}
/*****************************************************************************/
static void
dump_sr(struct lvd_acard *acard, void *xp)
{
    static const char *sr_bits[]={
        "FAT_ERR",
        "RO_ERR",
        "LV_DR_ERR",
        "HV_DR_ERR",
        "LV_DAQ_ERR0",
        "LV_DAQ_ERR1",
        "HV_DAQ_ERR0",
        "HV_DAQ_ERR1",
        "LV_POTY_BUSY",
        "HV_POTY_BUSY",
        "LV_DAC_BUSY",
        "HV_DAC_BUSY",
        "LV_VAREGOUT_BSY",
        "HV_VAREGOUT_BSY",
        "LV_I2C_DATA",
        "HV_I2C_DATA"
    };
    ems_u32 val;
    int i, a=0xe;

    if (lvd_i_r(acard->dev, acard->addr, vertex.sr, &val)<0) {
        xprintf(xp, "  unable to read sr\n");
        return;
    }

    xprintf(xp, "  [%02x] sr       =0x%04x", a, val);
    for (i=0; i<16; i++) {
        if (val&(1<<i))
            xprintf(xp, " %s", sr_bits[i]);
    }
    xprintf(xp, "\n");
}
/*****************************************************************************/
static void
dump_cyclic_raw(struct lvd_acard* acard, void *xp)
{
    ems_u32 val;
    int a=0x4a;

    if (lvd_i_r(acard->dev, acard->addr, vertex.cyclic_raw, &val)<0) {
        xprintf(xp, "  unable to read cyclic_raw\n");
        return;
    }
    xprintf(xp, "  [%02x] cycl_raw  =0x%04x\n", a, val);
}
/*****************************************************************************/
static void
dump_scaling(struct lvd_acard* acard, void *xp)
{
    ems_u32 val;
    int a=0x4c;

    if (lvd_i_r(acard->dev, acard->addr, vertex.scaling, &val)<0) {
        xprintf(xp, "  unable to read scaling\n");
        return;
    }
    xprintf(xp, "  [%02x] scaling   =0x%04x\n", a, val);
}
/*****************************************************************************/
static void
dump_dig_tst(struct lvd_acard* acard, void *xp)
{
    ems_u32 val;
    int a=0x43;

    if (lvd_i_r(acard->dev, acard->addr, vertex.dig_tst, &val)<0) {
        xprintf(xp, "  unable to read dig_tst\n");
        return;
    }
    xprintf(xp, "  [%02x] dig_tst   =0x%04x\n", a, val);
}
/*****************************************************************************/
static void
dump_xv_switchreg(struct lvd_acard* acard, void *xp, int idx)
{
    ems_u32 val;
    int res, a=idx?0x50:0x20;

    if (idx)
        res=lvd_i_r(acard->dev, acard->addr, vertex.hv.swreg, &val);
    else
        res=lvd_i_r(acard->dev, acard->addr, vertex.lv.swreg, &val);
    if (res<0) {
        xprintf(xp, "  unable to read switchreg\n");
        return;
    }

    xprintf(xp, "  [%02x] switchreg=0x%04x\n", a, val);
}
/*****************************************************************************/
static void
dump_xv_count(struct lvd_acard* acard, void *xp, int idx)
{
    ems_u32 val[2];
    int res, a=idx?0x54:0x24;

    if (idx) {
        res =lvd_i_r(acard->dev, acard->addr, vertex.hv.counter[0], val+0);
        res|=lvd_i_r(acard->dev, acard->addr, vertex.hv.counter[1], val+1);
    } else {
        res =lvd_i_r(acard->dev, acard->addr, vertex.lv.counter[0], val+0);
        res|=lvd_i_r(acard->dev, acard->addr, vertex.lv.counter[1], val+1);
    }
    if (res) {
        xprintf(xp, "  unable to read count\n");
        return;
    }

    xprintf(xp, "  [%02x] count0   =0x%04x\n", a, val[0]);
    xprintf(xp, "  [%02x] count1   =0x%04x\n", a+2, val[1]);
}
/*****************************************************************************/
static void
dump_xv_loopcnt(struct lvd_acard* acard, void *xp, int idx)
{
    ems_u32 val[2];
    int res, a=idx?0x58:0x28;

    if (idx) {
        res =lvd_i_r(acard->dev, acard->addr, vertex.hv.lp_counter[0], val+0);
        res|=lvd_i_r(acard->dev, acard->addr, vertex.hv.lp_counter[1], val+1);
    } else {
        res =lvd_i_r(acard->dev, acard->addr, vertex.lv.lp_counter[0], val+0);
        res|=lvd_i_r(acard->dev, acard->addr, vertex.lv.lp_counter[1], val+1);
    }
    if (res) {
        xprintf(xp, "  unable to read loop_count\n");
        return;
    }

    xprintf(xp, "  [%02x] lp_count0=0x%04x\n", a, val[0]);
    xprintf(xp, "  [%02x] lp_count1=0x%04x\n", a+2, val[1]);
}
/*****************************************************************************/
static void
dump_xv_dgap_len(struct lvd_acard* acard, void *xp, int idx)
{
    ems_u32 val[2];
    int res, a=idx?0x5c:0x2c;

    if (idx) {
        res =lvd_i_r(acard->dev, acard->addr, vertex.hv.dgap_len[0], val+0);
        res|=lvd_i_r(acard->dev, acard->addr, vertex.hv.dgap_len[1], val+1);
    } else {
        res =lvd_i_r(acard->dev, acard->addr, vertex.lv.dgap_len[0], val+0);
        res|=lvd_i_r(acard->dev, acard->addr, vertex.lv.dgap_len[1], val+1);
    }
    if (res) {
        xprintf(xp, "  unable to read dgap_len\n");
        return;
    }

    xprintf(xp, "  [%02x] dgap_len0=0x%04x\n", a, val[0]);
    xprintf(xp, "  [%02x] dgap_len1=0x%04x\n", a+2, val[1]);
}
/*****************************************************************************/
static void
dump_xv_aclk_delay(struct lvd_acard* acard, void *xp, int idx)
{
    ems_u32 val;
    int res, a=idx?0x60:0x30;

    if (idx)
        res=lvd_i_r(acard->dev, acard->addr, vertex.hv.clk_delay, &val);
    else
        res=lvd_i_r(acard->dev, acard->addr, vertex.lv.clk_delay, &val);
    if (res<0) {
        xprintf(xp, "  unable to read aclk_delay\n");
        return;
    }

    xprintf(xp, "  [%02x] delay    =0x%04x\n", a, val);
}
/*****************************************************************************/
static void
dump_xv_nrchan(struct lvd_acard* acard, void *xp, int idx)
{
    ems_u32 val;
    int res, a=idx?0x62:0x32;

    if (idx)
        res=lvd_i_r(acard->dev, acard->addr, vertex.hv.nr_chan, &val);
    else
        res=lvd_i_r(acard->dev, acard->addr, vertex.lv.nr_chan, &val);
    if (res<0) {
        xprintf(xp, "  unable to read nrchan\n");
        return;
    }

    xprintf(xp, "  [%02x] nrchan   =0x%04x\n", a, val);
}
/*****************************************************************************/
static void
dump_xv_noisewin(struct lvd_acard* acard, void *xp, int idx)
{
    ems_u32 val;
    int res, a=idx?0x64:0x34;

    if (idx)
        res=lvd_i_r(acard->dev, acard->addr, vertex.hv.noise_thr, &val);
    else
        res=lvd_i_r(acard->dev, acard->addr, vertex.lv.noise_thr, &val);
    if (res<0) {
        xprintf(xp, "  unable to read noisewin\n");
        return;
    }

    xprintf(xp, "  [%02x] noisewin =0x%04x\n", a, val);
}
/*****************************************************************************/
static void
dump_xv_cmodval(struct lvd_acard* acard, void *xp, int idx)
{
  ems_u32 val;
  int res, a=idx?0x66:0x36;

  if(acard->mtype == ZEL_LVD_ADC_VERTEX) {
    if (idx)
        res=lvd_i_r(acard->dev, acard->addr, vertex.hv.v.comval, &val);
    else
        res=lvd_i_r(acard->dev, acard->addr, vertex.lv.v.comval, &val);
    if (res<0) {
        xprintf(xp, "  unable to read cmodval\n");
        return;
    }
  } else {
    if (idx)
        res=lvd_i_r(acard->dev, acard->addr, vertex.hv.m.comval, &val);
    else
        res=lvd_i_r(acard->dev, acard->addr, vertex.lv.m.comval, &val);
    if (res<0) {
        xprintf(xp, "  unable to read cmodval\n");
        return;
    }
  }
    xprintf(xp, "  [%02x] cmodval  =0x%04x\n", a, val);
}
/*****************************************************************************/
static void
dump_xv_nrwinval(struct lvd_acard* acard, void *xp, int idx)
{
  ems_u32 val;
  int res, a=idx?0x68:0x38;

  if(acard->mtype == ZEL_LVD_ADC_VERTEX) {
    if (idx)
        res=lvd_i_r(acard->dev, acard->addr, vertex.hv.v.comcnt, &val);
    else
        res=lvd_i_r(acard->dev, acard->addr, vertex.lv.v.comcnt, &val);
    if (res<0) {
        xprintf(xp, "  unable to read nrwinval\n");
        return;
    }
  } else {
    if (idx)
        res=lvd_i_r(acard->dev, acard->addr, vertex.hv.m.comcnt, &val);
    else
        res=lvd_i_r(acard->dev, acard->addr, vertex.lv.m.comcnt, &val);
    if (res<0) {
        xprintf(xp, "  unable to read nrwinval\n");
        return;
    }
  }
  xprintf(xp, "  [%02x] nrwinval =0x%04x\n", a, val);
}
/*****************************************************************************/
static void
dump_xv_reg_cr(struct lvd_acard* acard, void *xp, int idx)
/* VATA only */
{
  ems_u32 val;
  int res, a=idx?0x6e:0x3e;

  if(acard->mtype == ZEL_LVD_ADC_VERTEX) {
    if (idx)
        res=lvd_i_r(acard->dev, acard->addr, vertex.hv.v.reg_cr, &val);
    else
        res=lvd_i_r(acard->dev, acard->addr, vertex.lv.v.reg_cr, &val);
    if (res<0) {
        xprintf(xp, "  unable to read reg_cr\n");
        return;
    }
    xprintf(xp, "  [%02x] reg_cr   =0x%04x\n", a, val);
  }
}
/*****************************************************************************/
static void
dump_xv_otr_counter(struct lvd_acard* acard, void *xp, int idx)
{
  ems_u32 val;
  int res, a;

  if(acard->mtype == ZEL_LVD_ADC_VERTEX) {
    a=idx?0x74:0x44;
    if (idx)
        res=lvd_i_r(acard->dev, acard->addr, vertex.hv.v.otr_counter, &val);
    else
        res=lvd_i_r(acard->dev, acard->addr, vertex.lv.v.otr_counter, &val);
    if (res<0) {
        xprintf(xp, "  unable to read otr_counter\n");
        return;
    }
  } else {
    a=idx?0x70:0x40;
    if (idx)
        res=lvd_i_r(acard->dev, acard->addr, vertex.hv.m.otr_counter, &val);
    else
        res=lvd_i_r(acard->dev, acard->addr, vertex.lv.m.otr_counter, &val);
    if (res<0) {
        xprintf(xp, "  unable to read otr_counter\n");
        return;
    }
  }
  xprintf(xp, "  [%02x] otr_cnt  =0x%04x\n", a, val);
}
/*****************************************************************************/
static void
dump_xv_i2c_csr(struct lvd_acard* acard, void *xp, int idx)
/* MATA3 only */
{
  ems_u32 val;
  int res, a=idx?0x6a:0x3a;

  if(acard->mtype == ZEL_LVD_ADC_VERTEXM3) {
    if (idx)
        res=lvd_i_r(acard->dev, acard->addr, vertex.hv.m.i2c_csr, &val);
    else
        res=lvd_i_r(acard->dev, acard->addr, vertex.lv.m.i2c_csr, &val);
    if (res<0) {
        xprintf(xp, "  unable to read i2c_csr\n");
        return;
    }
    xprintf(xp, "  [%02x] i2c_csr  =0x%04x\n", a, val);
  }
}
/*****************************************************************************/
static void
dump_xv_aclk_par(struct lvd_acard* acard, void *xp, int idx)
/* MATA3 only */
{
  ems_u32 val;
  int res, a=idx?0x72:0x42;

  if(acard->mtype == ZEL_LVD_ADC_VERTEXM3) {
    if (idx)
        res=lvd_i_r(acard->dev, acard->addr, vertex.hv.m.aclk_par, &val);
    else
        res=lvd_i_r(acard->dev, acard->addr, vertex.lv.m.aclk_par, &val);
    if (res<0) {
        xprintf(xp, "  unable to read aclk_par\n");
        return;
    }
    xprintf(xp, "  [%02x] aclk_par =0x%04x\n", a, val);
  }
}
/*****************************************************************************/
static void
dump_xv_xclk_del(struct lvd_acard* acard, void *xp, int idx)
/* MATA3 only */
{
  ems_u32 val;
  int res, a=idx?0x74:0x44;

  if(acard->mtype == ZEL_LVD_ADC_VERTEXM3) {
    if (idx)
        res=lvd_i_r(acard->dev, acard->addr, vertex.hv.m.xclk_del, &val);
    else
        res=lvd_i_r(acard->dev, acard->addr, vertex.lv.m.xclk_del, &val);
    if (res<0) {
        xprintf(xp, "  unable to read xclk_del\n");
        return;
    }
    xprintf(xp, "  [%02x] xclk_del =0x%04x\n", a, val);
  }
}
/*****************************************************************************/
static void
dump_xv_adc_sample(struct lvd_acard* acard, void *xp, int idx)
{
  ems_u32 val;
  int res, a;

  if(acard->mtype == ZEL_LVD_ADC_VERTEX) {
    a=idx?0x72:0x42;
    if (idx)
        res=lvd_i_r(acard->dev, acard->addr, vertex.hv.v.adc_value, &val);
    else
        res=lvd_i_r(acard->dev, acard->addr, vertex.lv.v.adc_value, &val);
    if (res<0) {
        xprintf(xp, "  unable to read adc_sample\n");
        return;
    }
  } else {
    a=idx?0x6e:0x3e;
    if (idx)
        res=lvd_i_r(acard->dev, acard->addr, vertex.hv.m.adc_value, &val);
    else
        res=lvd_i_r(acard->dev, acard->addr, vertex.lv.m.adc_value, &val);
    if (res<0) {
        xprintf(xp, "  unable to read adc_sample\n");
        return;
    }
  }
    xprintf(xp, "  [%02x] adc_sample=0x%04x\n", a, val);
}
/*****************************************************************************/
static void
dump_block(struct lvd_acard* acard, void *xp, int level, int idx)
{
    xprintf(xp, " %s:\n", idx?"HV":"LV");
    switch (level) {
    case 0:
        dump_xv_switchreg(acard, xp, idx);
        dump_xv_count(acard, xp, idx);
        dump_xv_loopcnt(acard, xp, idx);
        dump_xv_cmodval(acard, xp, idx);
        dump_xv_nrwinval(acard, xp, idx);
        dump_xv_adc_sample(acard, xp, idx);
    case 1:
    case 2:
    default:
        dump_xv_switchreg(acard, xp, idx);
        dump_xv_count(acard, xp, idx);
        dump_xv_loopcnt(acard, xp, idx);
        dump_xv_dgap_len(acard, xp, idx);
        dump_xv_aclk_delay(acard, xp, idx);
        dump_xv_nrchan(acard, xp, idx);
        dump_xv_noisewin(acard, xp, idx);
        dump_xv_cmodval(acard, xp, idx);
        dump_xv_nrwinval(acard, xp, idx);
        switch (acard->mtype) {
        case ZEL_LVD_ADC_VERTEX:
            dump_xv_reg_cr(acard, xp, idx);
            dump_xv_otr_counter(acard, xp, idx);
            dump_xv_adc_sample(acard, xp, idx);
            break;
        case ZEL_LVD_ADC_VERTEXM3:
            dump_xv_i2c_csr(acard, xp, idx);
            dump_xv_adc_sample(acard, xp, idx);
            dump_xv_otr_counter(acard, xp, idx);
            dump_xv_aclk_par(acard, xp, idx);
            dump_xv_xclk_del(acard, xp, idx);
            break;
        }
        break;
    }
}
/*****************************************************************************/
int
lvd_cardstat_vertex(struct lvd_dev* dev, struct lvd_acard* acard, void *xp,
    int level)
{
    int addr=acard->addr;

    xprintf(xp, "ACQcard 0x%03x VERTEX ", addr);
    switch (LVD_HWtyp(acard->id)) {
    case 0x40:
        xprintf(xp, "(VA32TA2):\n");
        break;
    case 0x48:
        xprintf(xp, "(MATE3):\n");
        break;
    default:
        xprintf(xp, "(unknown type of chip):\n");
    }

    switch (level) {
    case 0:
        dump_seq_csr(acard, xp);
        dump_cr(acard, xp);
        dump_sr(acard, xp);
        dump_block(acard, xp, level, 0);
        dump_block(acard, xp, level, 1);
        break;
    case 1:
        dump_ident(acard, xp);
        dump_seq_csr(acard, xp);
        dump_aclk_par(acard, xp);
        dump_cr(acard, xp);
        dump_sr(acard, xp);
        dump_block(acard, xp, level, 0);
        dump_block(acard, xp, level, 1);
        dump_scaling(acard, xp);
        dump_dig_tst(acard, xp);
        break;
    case 2:
    default:
        dump_ident(acard, xp);
        dump_serial(acard, xp);
        dump_seq_csr(acard, xp);
        dump_aclk_par(acard, xp);
        dump_cr(acard, xp);
        dump_sr(acard, xp);
        dump_block(acard, xp, level, 0);
        dump_block(acard, xp, level, 1);
        dump_cyclic_raw(acard, xp);
        dump_scaling(acard, xp);
        dump_dig_tst(acard, xp);
        break;
    }
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
