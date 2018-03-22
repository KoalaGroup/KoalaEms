/*
 * lowlevel/lvd/qdc/qdc.c
 * created 2012-Sep-11 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: cardstat_qdc.c,v 1.4 2017/10/22 22:30:50 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <stdio.h>

#include <modultypes.h>
#include <xprintf.h>
#include <rcs_ids.h>
#include "../lvdbus.h"
#include "../lvd_access.h"
#include "qdc.h"
#include "qdc_private.h"

RCS_REGISTER(cvsid, "lowlevel/lvd/qdc")

/*****************************************************************************/
static int
dump_ident(struct lvd_acard *acard,
        __attribute__((unused)) struct qdc_info *info, void *xp)
{
    ems_u32 val;
    int res, a=0;

    res=lvd_i_r(acard->dev, acard->addr, all.ident, &val);
    if (res) {
        xprintf(xp, "  unable to read ident\n");
        return -1;
    }

    xprintf(xp, "  [%02x] ident       =0x%04x (type=0x%x version=%x.%x)\n",
            a, val, val&0xff, LVD_FWverH(val), LVD_FWverL(val));
    return 0;
}
/*****************************************************************************/
static int
dump_serial(struct lvd_acard *acard,
        __attribute__((unused)) struct qdc_info *info, void *xp)
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
static int
dump_inhibit(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 val;
    int res, a;

    if (info->qdcver>=3) {
        res=lvd_i_r(acard->dev, acard->addr, qdc3.cha_inh, &val);
        a=0x10;
    } else {
        res=lvd_i_r(acard->dev, acard->addr, qdc1.cha_inh, &val);
        a=0x18;
    }
    if (res) {
        xprintf(xp, "  unable to read inhibit\n");
        return -1;
    }

    xprintf(xp, "  [%02x] inhibit_chan=0x%04x", a, val);
    if (val) {
        int i, n=0;
        xprintf(xp, "(");
        for (i=0; i<16; i++) {
            if (val&(1<<i)) {
                xprintf(xp, "%s%d", n?" ":"", i);
                n++;
            }
        }
        xprintf(xp, ")");
    }
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
dump_raw(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 val;
    int res, a;

    if (info->qdcver>=3) {
        res=lvd_i_r(acard->dev, acard->addr, qdc3.cha_raw, &val);
        a=0x12;
    } else {
        res=lvd_i_r(acard->dev, acard->addr, qdc1.cha_raw, &val);
        a=0x1a;
    }
    if (res) {
        xprintf(xp, "  unable to read cha_raw\n");
        return -1;
    }

    xprintf(xp, "  [%02x] raw_chan    =0x%04x", a, val);
    if (val) {
        int i, n=0;
        xprintf(xp, " (");
        for (i=0; i<16; i++) {
            if (val&(1<<i)) {
                xprintf(xp, "%s%d", n?" ":"", i);
                n++;
            }
        }
        xprintf(xp, ")");
    }
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
dump_iw_start(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 val;
    ems_i16 sval;
    int res, a;
    float clk;

    if (info->qdcver>=3) {
        res=lvd_i_r(acard->dev, acard->addr, qdc3.iw_start, &val);
        a=0x14;
        clk=info->base_clk;
    } else {
        res=lvd_i_r(acard->dev, acard->addr, qdc1.iw_start, &val);
        a=0x1c;
        clk=info->adc_clk;
    }
    if (res) {
        xprintf(xp, "  unable to read iw_start\n");
        return -1;
    }

    sval=val;
    xprintf(xp, "  [%02x] iw_start    =0x%04x (%6.1f ns)\n", a, val, sval*clk);
    return 0;
}
/*****************************************************************************/
static int
dump_iw_length(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 val;
    int res, a;
    float clk;

    if (info->qdcver>=3) {
        res=lvd_i_r(acard->dev, acard->addr, qdc3.iw_length, &val);
        a=0x16;
        clk=info->base_clk;
    } else {
        res=lvd_i_r(acard->dev, acard->addr, qdc1.iw_length, &val);
        a=0x1e;
        clk=info->adc_clk;
    }
    if (res) {
        xprintf(xp, "  unable to read iw_length\n");
        return -1;
    }

    xprintf(xp, "  [%02x] iw_length   =0x%04x (%6.1f ns)\n", a, val, val*clk);
    return 0;
}
/*****************************************************************************/
static int
dump_sw_start(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 val;
    ems_i16 sval;
    int res, a;
    float clk=info->base_clk;

    if (info->qdcver>=3) {
        res=lvd_i_r(acard->dev, acard->addr, qdc3.sw_start, &val);
        a=0x18;
    } else {
        res=lvd_i_r(acard->dev, acard->addr, qdc1.latency, &val);
        a=0x4;
    }
    if (res) {
        xprintf(xp, "  unable to read sw_start\n");
        return -1;
    }

    sval=val;
    xprintf(xp, "  [%02x] sw_start    =0x%04x (%6.1f ns)\n", a, val, sval*clk);
    return 0;
}
/*****************************************************************************/
static int
dump_sw_length(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 val;
    int res, a;
    float clk=info->base_clk;

    if (info->qdcver>=3) {
        res=lvd_i_r(acard->dev, acard->addr, qdc3.sw_length, &val);
        a=0x1a;
    } else {
        res=lvd_i_r(acard->dev, acard->addr, qdc1.window, &val);
        a=0x6;
    }
    if (res) {
        xprintf(xp, "  unable to read sw_length\n");
        return -1;
    }

    xprintf(xp, "  [%02x] sw_length   =0x%04x (%6.1f ns)\n", a, val, val*clk);
    return 0;
}
/*****************************************************************************/
static int
dump_sw_ilength(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 val;
    int res, a;
    float clk;

    if (info->qdcver>=3) {
        res=lvd_i_r(acard->dev, acard->addr, qdc3.sw_ilength, &val);
        clk=info->base_clk;
        a=0x1c;
    } else {
        res=lvd_i_r(acard->dev, acard->addr, qdc1.int_length, &val);
        clk=info->adc_clk;
        a=0x4c;
    }
    if (res) {
        xprintf(xp, "  unable to read sw_ilength\n");
        return -1;
    }

    xprintf(xp, "  [%02x] sw_ilength  =0x%04x (%6.1f ns)\n", a, val, val*clk);
    return 0;
}
/*****************************************************************************/
static void
decode_cr(ems_u32 cr, void *xp)
{
    if (cr&QDC_CR_ENA)
        xprintf(xp, " ENA");
    if (cr&QDC_CR_LTRIG)
        xprintf(xp, " LOCAL_TRIGGER");
    if (cr&QDC_CR_LEVTRIG)
        xprintf(xp, " LEVEL_TRIGGER");
    if (cr&QDC_CR_VERBOSE)
        xprintf(xp, " VERBOSE");
    if (cr&QDC_CR_ADCPOL)
        xprintf(xp, " INV_ADC");
    if (cr&QDC_CR_SGRD)
        xprintf(xp, " SGRD");
    if (cr&QDC_CR_TST_SIG)
        xprintf(xp, " TST_SIG");
    if (cr&QDC_CR_ADC_PWR)
        xprintf(xp, " ADC_PWR_OFF");
    if (cr&QDC_CR_F1MODE)
        xprintf(xp, " F1MODE");
    if (cr&QDC_CR_LOCBASE)
        xprintf(xp, " LOCBASE");
    if (cr&QDC_CR_NOPS)
        xprintf(xp, " NOPS");
    if (cr&QDC_CR_LMODE)
        xprintf(xp, " LMODE");
    if (cr&QDC_CR_TRAW)
        xprintf(xp, " TRAW");
}
/*****************************************************************************/
static int
dump_cr(struct lvd_acard *acard, __attribute__((unused)) struct qdc_info *info,
        void *xp)
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
dump_cr_saved(struct lvd_acard *acard,
        __attribute__((unused)) struct qdc_info *info, void *xp)
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
dump_sr(struct lvd_acard *acard, __attribute__((unused)) struct qdc_info *info,
        void *xp)
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
dump_trig_level(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 val;
    int res, a;

    if (info->qdcver>=3) {
        res=lvd_i_r(acard->dev, acard->addr, qdc3.trig_level, &val);
        a=0x4;
    } else {
        res=lvd_i_r(acard->dev, acard->addr, qdc1.trig_level, &val);
        a=0x10;
    }
    if (res) {
        xprintf(xp, "  unable to read trig_level\n");
        return -1;
    }

    xprintf(xp, "  [%02x] trig_level  =0x%04x\n", a, val);
    return 0;
}
/*****************************************************************************/
static int
dump_anal_ctrl(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 val;
    int res, a;

    if (info->qdcver>=3) {
        res=lvd_i_r(acard->dev, acard->addr, qdc3.anal_ctrl, &val);
        a=0x1e;
    } else {
        res=lvd_i_r(acard->dev, acard->addr, qdc1.anal_ctrl, &val);
        a=0x12;
    }
    if (res) {
        xprintf(xp, "  unable to read anal_ctrl\n");
        return -1;
    }

    xprintf(xp, "  [%02x] anal_ctrl   =0x%04x\n", a, val);
    return 0;
}
/*****************************************************************************/
static int
dump_aclk_shift(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 val;
    int res, a;

    if (info->qdcver>=3) {
        res=lvd_i_r(acard->dev, acard->addr, qdc3.aclk_shift, &val);
        a=0x6;
    } else {
        res=lvd_i_r(acard->dev, acard->addr, qdc1.aclk_shift, &val);
        a=0x16;
    }
    if (res) {
        xprintf(xp, "  unable to read aclk_shift\n");
        return -1;
    }

    xprintf(xp, "  [%02x] aclk_shift  =0x%04x\n", a, val);
    return 0;
}
/*****************************************************************************/
static int
dump_noise_level(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 val[16];
    int i, res=0, a=0x20;

    switch (info->qdcver) {
    case 1:
    case 2:
        for (i=0; i<16; i++)
            res|=lvd_i_r(acard->dev, acard->addr, qdc1.nq.noise_level[i],
                    val+i);
        break;
    case 3:
    case 4:
        for (i=0; i<16; i++)
            res|=lvd_i_r(acard->dev, acard->addr, qdc3.noise_qthr[i], val+i);
        break;
    case 5:
        res|=lvd_qdc_cr_sel(acard, 0);
        for (i=0; i<16; i++)
            res|=lvd_i_r(acard->dev, acard->addr, qdc3.noise_qthr[i], val+i);
        res|=lvd_qdc_cr_rev(acard);
        break;
    case 6:
    case 7:
        res|=lvd_qdc_cr_sel(acard, 0);
        for (i=0; i<16; i++)
            res|=lvd_i_r(acard->dev, acard->addr, qdc6.noise_qthr_coi[i],
                    val+i);
        res|=lvd_qdc_cr_rev(acard);
        break;
    }

    if (res) {
        xprintf(xp, "  unable to read noise_level\n");
        return -1;
    }

    xprintf(xp, "  [%02x] noise_level =", a);
    for (i=0; i<16; i++) {
        if (!(i&7))
            xprintf(xp, "\n    ");
        else if (!(i&3))
            xprintf(xp, " ");
        xprintf(xp, "%5x ", val[i]);
    }
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
dump_auto_baseline(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 val[16];
    int i, res=0, a;

    switch (info->qdcver) {
    case 1:
    case 2:
        for (i=0; i<16; i++)
            res|=lvd_i_r(acard->dev, acard->addr, qdc1.mo.mean_level[i], val+i);
        a=0x60;
        break;
    case 3:
    case 4:
        for (i=0; i<16; i++)
            res|=lvd_i_r(acard->dev, acard->addr, qdc3.bline_thlevel[i], val+i);
        a=0x40;
        break;
    case 5:
        res|=lvd_qdc_cr_sel(acard, 0);
        for (i=0; i<16; i++)
            res|=lvd_i_r(acard->dev, acard->addr, qdc3.bline_thlevel[i], val+i);
        res|=lvd_qdc_cr_rev(acard);
        a=0x40;
        break;
    case 6:
    case 7:
        res|=lvd_qdc_cr_sel(acard, 0);
        for (i=0; i<16; i++)
            res|=lvd_i_r(acard->dev, acard->addr, qdc6.bline_thlevel[i], val+i);
        res|=lvd_qdc_cr_rev(acard);
        a=0x40;
        break;
    default:
        xprintf(xp, "  unknown version %d of QDC\n", info->qdcver);
        return 0;
    }

    if (res) {
        xprintf(xp, "  unable to read auto_bline\n");
        return -1;
    }

    xprintf(xp, "  [%02x] auto_baseline =", a);
    for (i=0; i<16; i++) {
        if (!(i&7))
            xprintf(xp, "\n    ");
        else if (!(i&3))
            xprintf(xp, " ");
        if (info->qdcver<3)
            xprintf(xp, "%5x ", val[i]);
        else
            xprintf(xp, "%x.%1x ", val[i]/16, val[i]%16);
    }
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
dump_fixed_baseline(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 val[16];
    int i, res=0, a=0x40;

    if (info->qdcver<5)
        return 0;

    res|=lvd_qdc_cr_sel(acard, QDC_CR_BL_QTH);

    for (i=0; i<16; i++)
        res|=lvd_i_r(acard->dev, acard->addr, qdc3.bline_thlevel[i], val+i);

    res|=lvd_qdc_cr_rev(acard);

    xprintf(xp, "  [%02x] fixed_baseline =", a);
    for (i=0; i<16; i++) {
        if (!(i&7))
            xprintf(xp, "\n    ");
        else if (!(i&3))
            xprintf(xp, " ");
        xprintf(xp, "%5x ", val[i]);
    }
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
dump_thrh_level(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 val[16];
    u_int16_t valid=0;
    int i, a=0x40, res=0;

    switch (info->qdcver) {
    case 1:
    case 2:
    case 3:
        return 0;
    case 4:
        for (i=0; i<16; i++)
            val[i]=info->thrh_shadow[i];
        valid=info->thrh_shadow_valid;
        break;
    case 5:
        res|=lvd_qdc_cr_sel(acard, QDC_CR_SEL);
        for (i=0; i<16; i++)
            res|=lvd_i_r(acard->dev, acard->addr, qdc3.bline_thlevel[i], val+i);
        res|=lvd_qdc_cr_rev(acard);
        valid=0xffff;
        break;
    case 6:
    case 7:
        res|=lvd_qdc_cr_sel(acard, QDC_CR_SEL|QDC_CR_BL_QTH);
        for (i=0; i<16; i++)
            res|=lvd_i_r(acard->dev, acard->addr, qdc6.bline_thlevel[i], val+i);
        res|=lvd_qdc_cr_rev(acard);
        valid=0xffff;
        break;
    }

    xprintf(xp, "  [%02x] thrh_level =", a);
    for (i=0; i<16; i++) {
        if (!(i&7))
            xprintf(xp, "\n    ");
        else if (!(i&3))
            xprintf(xp, " ");
        if (valid&(1<<i))
            xprintf(xp, "%5x ", val[i]);
        else
            xprintf(xp, "    ? ");
    }
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
dump_tdcrange(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 val;
    int res, a;

    if (info->qdcver>=3) {
        res=lvd_i_r(acard->dev, acard->addr, qdc3.tdc_range, &val);
        a=0x60;
    } else {
        res=lvd_i_r(acard->dev, acard->addr, qdc1.tdc_range, &val);
        a=0x50;
    }
    if (res) {
        xprintf(xp, "  unable to read tdcrange\n");
        return -1;
    }

    xprintf(xp, "  [%02x] tdcrange    =0x%04x\n", a, val);
    return 0;
}
/*****************************************************************************/
static int
dump_dacs(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    int channel, a;

    a=info->qdcver>=3?0x70:0x60;

    if (LVD_HWtyp(acard->id)==LVD_CARDID_SQDC) {
        xprintf(xp, "  [%02x] dacs =", a);
        for (channel=0; channel<16; channel++) {
            if (!(channel&7))
                xprintf(xp, "\n    ");
            else if (!(channel&3))
                xprintf(xp, " ");
            if (info->dac_shadow_valid&(1<<channel))
                xprintf(xp, "%5x ", info->dac_shadow[channel]);
            else
                xprintf(xp, "    ? ");
        }
        xprintf(xp, "\n");
    } else {
        if (info->testdac_shadow_valid)
            xprintf(xp, "  [%02x] testdac     = 0x%x\n", a, info->testdac_shadow);
        else
            xprintf(xp, "  [%02x] testdac     = shadow not valid\n", a);
    }
    return 0;
}
/*****************************************************************************/
static int
dump_qthreshold(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 val[16];
    u_int16_t valid=0;
    int channel, a=0x20, res=0;

    switch (info->qdcver) {
    case 1:
    case 2:
    case 3:
    case 4:
        for (channel=0; channel<16; channel++)
            val[channel]=info->threshold_shadow[channel];
        valid=info->threshold_shadow_valid;
        break;
    case 5:
        res|=lvd_qdc_cr_sel(acard, QDC_CR_BL_QTH);
        for (channel=0; channel<16; channel++)
            res|=lvd_i_r(acard->dev, acard->addr, qdc3.noise_qthr[channel],
                    val+channel);
        lvd_qdc_cr_rev(acard);
        if (res) {
            xprintf(xp, "  unable to read qthr[%d]\n", channel);
            return -1;
        }
        valid=0xffff;
        break;
    case 6:
    case 7:
        res|=lvd_qdc_cr_sel(acard, QDC_CR_BL_QTH);
        for (channel=0; channel<16; channel++)
            res|=lvd_i_r(acard->dev, acard->addr, qdc6.noise_qthr_coi[channel],
                    val+channel);
        lvd_qdc_cr_rev(acard);
        if (res) {
            xprintf(xp, "  unable to read qthr[%d]\n", channel);
            return -1;
        }
        valid=0xffff;
    }

    xprintf(xp, "  [%02x] Q-threshold =", a);
    for (channel=0; channel<16; channel++) {
        if (!(channel&7))
            xprintf(xp, "\n    ");
        else if (!(channel&3))
            xprintf(xp, " ");
        if (valid&(1<<channel))
            xprintf(xp, "%5x ", val[channel]);
        else
            xprintf(xp, "    ? ");
    }
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
dump_rawcycle(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 val;
    int res, a=0x64;

    if (info->qdcver<3)
        return 0;

    res=lvd_i_r(acard->dev, acard->addr, qdc3.traw_cycle, &val);
    if (res) {
        xprintf(xp, "  unable to read traw_cycle\n");
        return -1;
    }

    xprintf(xp, "  [%02x] raw_cycle   =0x%04x", a, val);
    if (val)
        xprintf(xp, " (%d)\n", 1<<(val+7));
    else
        xprintf(xp, " (off)\n");
    
    return 0;
}
/*****************************************************************************/
static int
dump_ovr(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 val;
    int res, a=0x66;

    if (info->qdcver<3)
        return 0;

    res=lvd_i_r(acard->dev, acard->addr, qdc3.ovr, &val);
    if (res) {
        xprintf(xp, "  unable to read ovr\n");
        return -1;
    }

    xprintf(xp, "  [%02x] ovr         =0x%04x\n", a, val);
    
    return 0;
}
/*****************************************************************************/
static int
dump_bline_adjust(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 val;
    int res, a=0x68;

    if (info->qdcver<3 || info->qdcver>5)
        return 0;

    res=lvd_i_r(acard->dev, acard->addr, qdc3.bline_adjust, &val);
    if (res) {
        xprintf(xp, "  unable to read bline_adjust\n");
        return -1;
    }

    xprintf(xp, "  [%02x] bline_adj   =0x%04x\n", a, val);
    
    return 0;
}
/*****************************************************************************/
int
lvd_cardstat_qdc(__attribute__((unused)) struct lvd_dev* dev,
        struct lvd_acard* acard, void *xp,
    int level)
{
    struct qdc_info *info=(struct qdc_info*)acard->cardinfo;

    xprintf(xp, "ACQcard 0x%03x %sQDC:\n", 
            acard->addr,
            acard->mtype==ZEL_LVD_SQDC?"S":
            acard->mtype==ZEL_LVD_FQDC?"F":"VF");

    switch (level) {
    case 0:
        dump_cr(acard, info, xp);
        dump_sr(acard, info, xp);
        dump_noise_level(acard, info, xp);
        dump_ovr(acard, info, xp);
        dump_bline_adjust(acard, info, xp);
        break;
    case 1:
        dump_ident(acard, info, xp);
        dump_trig_level(acard, info, xp);
        dump_cr(acard, info, xp);
        dump_cr_saved(acard, info, xp);
        dump_sr(acard, info, xp);
        dump_inhibit(acard, info, xp);
        dump_raw(acard, info, xp);
        dump_iw_start(acard, info, xp);
        dump_iw_length(acard, info, xp);
        dump_sw_start(acard, info, xp);
        dump_sw_length(acard, info, xp);
        dump_sw_ilength(acard, info, xp);
        dump_anal_ctrl(acard, info, xp);
        dump_qthreshold(acard, info, xp);
        dump_noise_level(acard, info, xp);
        dump_auto_baseline(acard, info, xp);
        dump_fixed_baseline(acard, info, xp);
        dump_thrh_level(acard, info, xp);
        dump_ovr(acard, info, xp);
        dump_bline_adjust(acard, info, xp);
        break;
    case 2:
    default:
        dump_ident(acard, info, xp);
        dump_serial(acard, info, xp);
        dump_trig_level(acard, info, xp);
        dump_aclk_shift(acard, info, xp);
        dump_cr(acard, info, xp);
        dump_cr_saved(acard, info, xp);
        dump_sr(acard, info, xp);
        dump_inhibit(acard, info, xp);
        dump_raw(acard, info, xp);
        dump_iw_start(acard, info, xp);
        dump_iw_length(acard, info, xp);
        dump_sw_start(acard, info, xp);
        dump_sw_length(acard, info, xp);
        dump_sw_ilength(acard, info, xp);
        dump_anal_ctrl(acard, info, xp);
        dump_qthreshold(acard, info, xp);
        dump_noise_level(acard, info, xp);
        dump_auto_baseline(acard, info, xp);
        dump_fixed_baseline(acard, info, xp);
        dump_thrh_level(acard, info, xp);
        dump_tdcrange(acard, info, xp);
        dump_rawcycle(acard, info, xp);
        dump_ovr(acard, info, xp);
        dump_bline_adjust(acard, info, xp);
        dump_dacs(acard, info, xp);
        break;
    }

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
