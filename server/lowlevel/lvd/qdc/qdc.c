/*
 * lowlevel/lvd/qdc/qdc.c
 * created 2006-Feb-07 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: qdc.c,v 1.42 2013/09/24 14:08:03 wuestner Exp $";

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
#include <rcs_ids.h>
#include "../lvdbus.h"
#include "../lvd_access.h"
#include "../lvd_initfuncs.h"
#include "qdc.h"
#include "qdc_private.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#define CHECKFUN
#ifdef CHECKFUN
#define NUMFUN 9
#endif

typedef plerrcode (*qdcfun)(struct lvd_acard*, ems_u32*, int idx);

static ems_u32 mtypes_SQDC[]={ZEL_LVD_SQDC, 0};
#if 0
static ems_u32 mtypes_FQDC[]={ZEL_LVD_FQDC, 0};
static ems_u32 mtypes_VFQDC[]={ZEL_LVD_VFQDC, 0};
#endif
static ems_u32 mtypes_FQDCs[]={ZEL_LVD_FQDC, ZEL_LVD_VFQDC, 0};

RCS_REGISTER(cvsid, "lowlevel/lvd/qdc")

/*****************************************************************************/
static plerrcode
access_qdcfun(struct lvd_dev* dev, struct lvd_acard *acard, ems_u32 *val,
    int idx,
    plerrcode (**funlist)(struct lvd_acard*, ems_u32*, int),
    int numfun, const char *txt)
{
    struct qdc_info *info=(struct qdc_info*)acard->cardinfo;

#ifdef CHECKFUN
    if (numfun<NUMFUN) {
        complain("qdcfun: not enough FUN for %s", txt);
        return plErr_Program;
    }
#endif

    if (numfun>info->qdcver && funlist[info->qdcver]) {
        return funlist[info->qdcver](acard, val, idx);
    } else {
        return plErr_BadModTyp;
    }
}
/*****************************************************************************/
static plerrcode
access_qdc(struct lvd_dev* dev, int addr, ems_u32 *val,
    plerrcode (**funlist)(struct lvd_acard*, ems_u32*, int),
    int numfun,
    ems_u32 *mtypes, const char *txt)
{
    struct lvd_acard *acard;
    int idx;

    idx=lvd_find_acard(dev, addr);
    acard=dev->acards+idx;
    if ((idx<0) ||
                (acard->mtype!=ZEL_LVD_SQDC &&
                acard->mtype!=ZEL_LVD_FQDC &&
                acard->mtype!=ZEL_LVD_VFQDC)) {
        complain("%s: no QDC card with address 0x%x known",
                txt?txt:"access_qdc",
                addr);
        return plErr_Program;
    }
    if (mtypes && mtypes[0]) {
        int ok=0;
        while (!ok && mtypes[0]) {
            if (acard->mtype==mtypes[0])
                ok=1;
            mtypes++;
        }
        if (!ok)
            return plErr_BadModTyp;
    }

    return access_qdcfun(dev, acard, val, 0, funlist, numfun, txt);
}
/*****************************************************************************/
static plerrcode
access_qdcs(struct lvd_dev* dev, int addr, ems_u32 *val,
    plerrcode (**funlist)(struct lvd_acard*, ems_u32*, int),
    int numfun,
    ems_u32 *mtypes, const char *txt)
{
    plerrcode pres=plOK;

    if (addr<0) {
        int idx;
        for (idx=0; idx<dev->num_acards; idx++) {
            struct lvd_acard *acard=dev->acards+idx;
            if ((acard->mtype==ZEL_LVD_SQDC ||
                    acard->mtype==ZEL_LVD_FQDC ||
                    acard->mtype==ZEL_LVD_VFQDC) &&
                    LVD_FWverH(acard->id)<8) {
                int ok=!mtypes || !mtypes[0];
                while (!ok && mtypes[0]) {
                    if (acard->mtype==mtypes[0])
                        ok=1;
                    mtypes++;
                }
                if (ok) {
                    pres=access_qdcfun(dev, acard, val, acard->addr,
                            funlist, numfun, txt);
                    if (pres!=plOK)
                        return pres;
                }
            }
        }
    } else {
        pres=access_qdc(dev, addr, val, funlist, numfun, mtypes, txt);
    }
    return pres;
}
/*****************************************************************************/
/*
 * The lvd_qdc_cr_* function MUST be used to access the 
 * control/mode register (cr).
 * Problem to be avoided: SEL0 (bit 13) is used to select register sets and
 * to modify runtime behavier.
 * lvd_qdc_cr_set: sets the 'normal' runtime value
 * lvd_qdc_cr_sel: sets the SEL0 and SEL1 bits
 * lvd_qdc_cr_rev: reverts cr to the value before lvd_qdc_cr_sel
 * Code which only wants to set the selection bits (SEL[01]) MUST
 * call lvd_qdc_cr_rev before it returns.
 * The value stored in cr_shadow_ does not reflect the current value of
 * cr and must not be used.
 */
static int
lvd_qdc_cr_set(struct lvd_acard* acard, ems_u32 cr)
{
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    int res;

    res=lvd_i_w(acard->dev, acard->addr, all.cr, cr);
    if (!res)
        info->cr_shadow_=cr;
    return res;
}
int
lvd_qdc_cr_sel(struct lvd_acard* acard, ems_u32 sel)
{
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    int res;

    ems_u32 cr=(info->cr_shadow_&~QDC_CR_SEL)|sel;
    res=lvd_i_w(acard->dev, acard->addr, all.cr, cr);
    return res;
}
int
lvd_qdc_cr_rev(struct lvd_acard* acard)
{
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    int res;

    res=lvd_i_w(acard->dev, acard->addr, all.cr, info->cr_shadow_);
    return res;
}
/*****************************************************************************/
/*
 * sets the DACs which shift the baseline (slow QDC only)
 */
static plerrcode
lvd_qdcX_set_dac(struct lvd_acard* acard, ems_u32 *vals, int xxx)
{
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    int idx, out, val, res=0;

    int channel=vals[0];
    ems_u32 dac=vals[1];

    if (channel<0) {
        for (channel=0; channel<16; channel++) {
            plerrcode pres;
            ems_u32 xvals[2];
            xvals[0]=channel;
            xvals[1]=dac;
            pres=lvd_qdcX_set_dac(acard, xvals, 0);
            if (pres!=plOK)
                return pres;
        }
        return plOK;
    }

    idx=channel>>2;
    out=channel&3;

    val=(out<<14)|(1<<13)|(0<<1)|(dac&0xfff);
    if (info->qdcver>=3)
        res=lvd_i_w(acard->dev, acard->addr, qdc3.ofs_dac[idx], val);
    else
        res=lvd_i_w(acard->dev, acard->addr, qdc1.mo.ofs_dac[idx], val);
    info->dac_shadow[channel]=dac;
    info->dac_shadow_valid|=1<<channel;
    return res?plErr_HW:plOK;
}
static plerrcode
lvd_qdc80_set_dac(struct lvd_acard* acard, ems_u32 *vals, int xxx)
{
    complain("lvd_qdc8_set_dac: it is not known how to set the DACs");
    return plErr_Program;
}

plerrcode
lvd_qdc_set_dac(struct lvd_dev* dev, int addr, int channel, ems_u32 dac)
{
    static qdcfun funs[]={0,
            lvd_qdcX_set_dac,
            lvd_qdcX_set_dac,
            lvd_qdcX_set_dac,
            lvd_qdcX_set_dac,
            lvd_qdcX_set_dac,
            0,
            0,
            lvd_qdc80_set_dac
    };
    ems_u32 val[2];
    val[0]=channel;
    val[1]=dac;
    return access_qdcs(dev, addr, val,
                funs, sizeof(funs)/sizeof(qdcfun),
                mtypes_SQDC,
                "lvd_qdc_set_dac");
}
/*****************************************************************************/
/*
 * sets the baseline shifting DAC values from shadow (slow QDC only)
 */
static plerrcode
lvd_qdcX_get_dac(struct lvd_acard* acard, ems_u32 *dacs, int xxx)
{
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    int i;

    if (info->dac_shadow_valid!=0xffff) {
        printf("lvd_qdcX_get_dac(%d): shadow not valid\n", acard->addr);
        return plErr_InvalidUse;
    }
    for (i=0; i<16; i++)
        *dacs++=info->dac_shadow[i];
    return plOK;
}
plerrcode
lvd_qdc_get_dac(struct lvd_dev* dev, int addr, ems_u32 *dacs)
{
    static qdcfun funs[]={0,
            lvd_qdcX_get_dac,
            lvd_qdcX_get_dac,
            lvd_qdcX_get_dac,
            lvd_qdcX_get_dac,
            lvd_qdcX_get_dac,
            0,
            0,
            0
    };
    return access_qdc(dev, addr, dacs,
                funs, sizeof(funs)/sizeof(qdcfun),
                mtypes_SQDC,
                "lvd_qdc_get_dac");
}
/*****************************************************************************/
/*
 * sets the DAC which defines the amplitude of the test pulse (fast QDC only)
 */
static plerrcode
lvd_qdc80_set_testdac(struct lvd_acard* acard, ems_u32 *vals, int xxx)
{
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    ems_u32 dac=(vals[0]&0xff)<<4;
    int res=0;
    res=lvd_i_w(acard->dev, acard->addr, qdc80.tp_dac, dac);
    info->testdac_shadow=vals[0]&0xff;
    info->testdac_shadow_valid=1;
    return res?plErr_HW:plOK;
}
/*
 * sets the DAC which defines the amplitude of the test pulse (fast QDC only)
 */
static plerrcode
lvd_qdc6_set_testdac(struct lvd_acard* acard, ems_u32 *vals, int xxx)
{
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    ems_u32 dac=(vals[0]&0xff)<<4;
    int res=0;
    /* select register set */
    /* cr<13>=0 set test pulse amplitude*/
    res|=lvd_qdc_cr_sel(acard, 0);

    res=lvd_i_w(acard->dev, acard->addr, qdc6.dac_coinc[0], dac);
    info->testdac_shadow=vals[0]&0xff;
    info->testdac_shadow_valid=1;

    res|=lvd_qdc_cr_rev(acard);
    return res?plErr_HW:plOK;
}
static plerrcode
lvd_qdc3_set_testdac
(struct lvd_acard* acard, ems_u32 *vals, int xxx)
{
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    ems_u32 dac=(vals[0]&0xff)<<4;
    int res=0;

    res=lvd_i_w(acard->dev, acard->addr, qdc3.ofs_dac[0], dac);
    info->testdac_shadow=vals[0]&0xff;
    info->testdac_shadow_valid=1;
    return res?plErr_HW:plOK;
}
plerrcode
lvd_qdc_set_testdac(struct lvd_dev* dev, int addr, ems_u32 dac)
{
    static qdcfun funs[]={0,
            0,
            0,
            lvd_qdc3_set_testdac,
            lvd_qdc3_set_testdac,
            lvd_qdc3_set_testdac,
            lvd_qdc6_set_testdac,
            lvd_qdc6_set_testdac,
            lvd_qdc80_set_testdac
    };
    return access_qdcs(dev, addr, &dac,
                funs, sizeof(funs)/sizeof(qdcfun),
                mtypes_FQDCs,
                "lvd_qdc_set_testdac");
}
/*****************************************************************************/
/*
 * reads the DAC value defining the test pulse amplitude from shadow
 * (fast QDC only)
 */
static plerrcode
lvd_qdc3_get_testdac(struct lvd_acard* acard, ems_u32 *dac, int idx)
{
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;

    if (!info->testdac_shadow_valid) {
        printf("lvd_qdc3_get_testdac(%d): shadow not valid\n", acard->addr);
        return plErr_InvalidUse;
    }
    dac[idx]=info->testdac_shadow;
    return plOK;
}
plerrcode
lvd_qdc_get_testdac(struct lvd_dev* dev, int addr, ems_u32 *dac)
{
    static qdcfun funs[]={0,
            0,
            0,
            lvd_qdc3_get_testdac,
            lvd_qdc3_get_testdac,
            lvd_qdc3_get_testdac,
            lvd_qdc3_get_testdac,
            lvd_qdc3_get_testdac,
            0
    };
    if (addr<0)
        memset(dac, 0, 16*sizeof(ems_u32));
    return access_qdcs(dev, addr, dac,
                funs, sizeof(funs)/sizeof(qdcfun),
                mtypes_FQDCs,
                "lvd_qdc_get_testdac");
}
/*****************************************************************************/
static plerrcode
lvd_qdc6_test_pulse(struct lvd_acard* acard, ems_u32* vvv, int xxx)
{
    int res;
    res=lvd_i_w(acard->dev, acard->addr, qdc6.ctrl, 8);
    return res?plErr_HW:plOK;
}
static plerrcode
lvd_qdc6s_test_pulse(struct lvd_acard* acard, ems_u32* vvv, int xxx)
{
    int res;
    res=lvd_i_w(acard->dev, acard->addr, qdc6s.ctrl_bline_adjust, 8);
    return res?plErr_HW:plOK;
}
plerrcode
lvd_qdc_test_pulse(struct lvd_dev* dev, int addr)
{
    static qdcfun funs[]={0,
            0, /* some older versions can generate testpulses too */
            0,
            0,
            0,
            0,
            lvd_qdc6_test_pulse,
            lvd_qdc6s_test_pulse,
            0
            };
    return access_qdcs(dev, addr, 0,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_test_pulse");
}
/*****************************************************************************/
/*
 * sets the level at which the internal trigger will fire
 * does it still exist in verson 6?
 */
static plerrcode
lvd_qdc1_set_trglevel(struct lvd_acard* acard, ems_u32 *level, int xxx)
{
    return lvd_i_w(acard->dev, acard->addr, qdc1.trig_level, *level)
            ?plErr_HW:plOK;
}
static plerrcode
lvd_qdc3_set_trglevel(struct lvd_acard* acard, ems_u32 *level, int xxx)
{
    return lvd_i_w(acard->dev, acard->addr, qdc3.trig_level, *level)
            ?plErr_HW:plOK;
}
plerrcode
lvd_qdc_set_trglevel(struct lvd_dev* dev, int addr, ems_u32 level)
{
    static qdcfun funs[]={0,
            lvd_qdc1_set_trglevel,
            lvd_qdc1_set_trglevel,
            lvd_qdc3_set_trglevel,
            lvd_qdc3_set_trglevel,
            lvd_qdc3_set_trglevel,
            0,
            0,
            0
    };
    return access_qdcs(dev, addr, &level,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_set_trglevel");
}
/*****************************************************************************/
/*
 * reads the level at which the internal trigger will fire
 * does it still exist in verson 6?
 */
static plerrcode
lvd_qdc1_get_trglevel(struct lvd_acard* acard, ems_u32 *level, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc1.trig_level, level+idx)
            ?plErr_HW:plOK;
}
static plerrcode
lvd_qdc3_get_trglevel(struct lvd_acard* acard, ems_u32 *level, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc3.trig_level, level+idx)
            ?plErr_HW:plOK;
}
plerrcode
lvd_qdc_get_trglevel(struct lvd_dev* dev, int addr, ems_u32 *level)
{
    static qdcfun funs[]={0,
            lvd_qdc1_get_trglevel,
            lvd_qdc1_get_trglevel,
            lvd_qdc3_get_trglevel,
            lvd_qdc3_get_trglevel,
            lvd_qdc3_get_trglevel,
            0,
            0,
            0
    };
    if (addr<0)
        memset(level, 0, 16*sizeof(ems_u32));
    return access_qdcs(dev, addr, level,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_get_trglevel");
}
/*****************************************************************************/
/*
 * This sets a fixed baseline
 */
static plerrcode
lvd_qdc3_set_ped(struct lvd_acard* acard, ems_u32 *vals, int xxx)
{
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    int res=0;

    int channel=vals[0];
    ems_u32 ped=vals[1];

    if (channel<0) {
        for (channel=0; channel<16; channel++) {
            res|=lvd_i_w(acard->dev, acard->addr, qdc3.bline_thlevel[channel],
                    ped);
            info->base_shadow[channel]=ped;
        }
        info->base_shadow_valid=0xffff;
    } else {
        res=lvd_i_w(acard->dev, acard->addr, qdc3.bline_thlevel[channel], ped);
        info->base_shadow[channel]=ped;
        info->base_shadow_valid|=1<<channel;
    }

    return res?plErr_HW:plOK;
}
static plerrcode
lvd_qdc5_set_ped(struct lvd_acard* acard, ems_u32 *vals, int xxx)
{
    int res=0;

    int channel=vals[0];
    ems_u32 ped=vals[1];

    /* select register set */
    res|=lvd_qdc_cr_sel(acard, 0);

    if (channel<0) {
        for (channel=0; channel<16; channel++) {
            res|=lvd_i_w(acard->dev, acard->addr, qdc3.bline_thlevel[channel],
                    ped);
        }
    } else {
        res=lvd_i_w(acard->dev, acard->addr, qdc3.bline_thlevel[channel], ped);
    }

    res|=lvd_qdc_cr_rev(acard);
    return res?plErr_HW:plOK;
}
static plerrcode
lvd_qdc6_set_ped(struct lvd_acard* acard, ems_u32 *vals, int xxx)
{
    int res=0;

    int channel=vals[0];
    ems_u32 ped=vals[1];

    /* select register set */
    res|=lvd_qdc_cr_sel(acard, 0);

    if (channel<0) {
        for (channel=0; channel<16; channel++) {
            res|=lvd_i_w(acard->dev, acard->addr, qdc6.bline_thlevel[channel],
                    ped);
        }
    } else {
        res=lvd_i_w(acard->dev, acard->addr, qdc6.bline_thlevel[channel], ped);
    }

    res|=lvd_qdc_cr_rev(acard);
    return res?plErr_HW:plOK;
}
plerrcode
lvd_qdc_set_ped(struct lvd_dev* dev, int addr, int channel, ems_u32 ped)
{
    static qdcfun funs[]={0,
        0,
        0,
        lvd_qdc3_set_ped,
        0, /* yes, no fixed baseline in version 4 */
        lvd_qdc5_set_ped,
        lvd_qdc6_set_ped,
        lvd_qdc6_set_ped,
        0
    };
    ems_u32 val[2];
    val[0]=channel;
    val[1]=ped;
    return access_qdcs(dev, addr, val,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_set_ped");
}
/*****************************************************************************/
/*
 * This reads the value of the fixed baseline
 */
static plerrcode
lvd_qdc3_get_ped(struct lvd_acard* acard, ems_u32 *peds, int xxx)
{
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    int i;

    if (info->base_shadow_valid!=0xffff) {
        printf("lvd_qdc3_get_ped(%d): shadow not valid\n", acard->addr);
        return plErr_InvalidUse;
    }
    for (i=0; i<16; i++)
        *peds++=info->base_shadow[i];

    return plOK;
}
static plerrcode
lvd_qdc5_get_ped(struct lvd_acard* acard, ems_u32 *peds, int xxx)
{
    int channel, res=0;

    /* select register set */
    res|=lvd_qdc_cr_sel(acard, QDC_CR_BL_QTH);

    for (channel=0; channel<16; channel++) {
        res|=lvd_i_r(acard->dev, acard->addr, qdc3.bline_thlevel[channel],
                peds++);
    }

    res|=lvd_qdc_cr_rev(acard);
    return res?plErr_HW:plOK;
}
static plerrcode
lvd_qdc6_get_ped(struct lvd_acard* acard, ems_u32 *peds, int xxx)
{
    int channel, res=0;

    /* select register set */
    res|=lvd_qdc_cr_sel(acard, QDC_CR_BL_QTH);

    for (channel=0; channel<16; channel++) {
        res|=lvd_i_r(acard->dev, acard->addr, qdc6.bline_thlevel[channel],
                peds++);
    }

    res|=lvd_qdc_cr_rev(acard);
    return res?plErr_HW:plOK;
}
plerrcode
lvd_qdc_get_ped(struct lvd_dev* dev, int addr, ems_u32 *peds)
{
    static qdcfun funs[]={0,
        0,
        0,
        lvd_qdc3_get_ped,
        0, /* yes, no fixed baseline in version 4 */
        lvd_qdc5_get_ped,
        lvd_qdc6_get_ped,
        lvd_qdc6_get_ped,
        0
    };
    return access_qdc(dev, addr, peds,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_get_ped");
}
/*****************************************************************************/
/*
 * sets the Q-threshold
 */
static plerrcode
lvd_qdcX_set_threshold(struct lvd_acard* acard, ems_u32* vals, int xxx)
{
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    int res;

    int channel=vals[0];
    ems_u32 threshold=vals[1];

    if (channel<0) {
        for (channel=0; channel<16; channel++) {
            plerrcode pres;
            ems_u32 xvals[2];
            xvals[0]=channel;
            xvals[1]=threshold;
            pres=lvd_qdcX_set_threshold(acard, xvals, 0);
            if (pres!=plOK)
                return pres;
        }
        return plOK;
    }

    if (info->qdcver>=3)
        res=lvd_i_w(acard->dev, acard->addr, qdc3.noise_qthr[channel],
                threshold);
    else
        res=lvd_i_w(acard->dev, acard->addr, qdc1.nq.q_threshold[channel],
                threshold);
    if (info->qdcver<5) {
        info->threshold_shadow[channel]=threshold;
        info->threshold_shadow_valid|=1<<channel;
    }
    return res?plErr_HW:plOK;
}
static plerrcode
lvd_qdc6_set_threshold(struct lvd_acard* acard, ems_u32* vals, int xxx)
{
    ems_u32 threshold=vals[1];
    int channel=vals[0];
    int res=0;

    /* select register set */
    /* cr<13>=0 cr<14>=0,1 set q_treshold*/
    lvd_qdc_cr_sel(acard, 0);

    if (channel<0) {
        for (channel=0; channel<16; channel++) {
            res|=lvd_i_w(acard->dev, acard->addr, qdc6.noise_qthr_coi[channel],
                threshold);
       }
    } else {
        res|=lvd_i_w(acard->dev, acard->addr, qdc6.noise_qthr_coi[channel],
                threshold);
    }
     
    lvd_qdc_cr_rev(acard);
    return res?plErr_HW:plOK;
}
plerrcode
lvd_qdc_set_threshold(struct lvd_dev* dev, int addr, int channel,
        ems_u32 threshold)
{
    static qdcfun funs[]={0,
        lvd_qdcX_set_threshold,
        lvd_qdcX_set_threshold,
        lvd_qdcX_set_threshold,
        lvd_qdcX_set_threshold,
        lvd_qdcX_set_threshold,
        lvd_qdc6_set_threshold,
        lvd_qdc6_set_threshold,
        0
    };
    ems_u32 val[2];
    val[0]=channel;
    val[1]=threshold;
    return access_qdcs(dev, addr, val,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_set_threshold");
}
/*****************************************************************************/
/*
 * reads the Q-threshold
 */
static plerrcode
lvd_qdcX_get_threshold(struct lvd_acard* acard, ems_u32* thresholds, int xxx)
{
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    int i;

    if (info->threshold_shadow_valid!=0xffff) {
        printf("lvd_qdcX_get_threshold(%d): shadow not valid\n", acard->addr);
        return plErr_InvalidUse;
    }
    for (i=0; i<16; i++)
        *thresholds++=info->threshold_shadow[i];
    return plOK;
}
static plerrcode
lvd_qdc5_get_threshold(struct lvd_acard* acard, ems_u32 *thresholds, int xxx)
{
    int channel, res=0;

    /* select register set */
    res|=lvd_qdc_cr_sel(acard, QDC_CR_BL_QTH);

    for (channel=0; channel<16; channel++)
        res|=lvd_i_r(acard->dev, acard->addr, qdc3.noise_qthr[channel],
                thresholds++);

    res|=lvd_qdc_cr_rev(acard);
    return res?plErr_HW:plOK;
}
static plerrcode
lvd_qdc6_get_threshold(struct lvd_acard* acard, ems_u32 *thresholds, int xxx)
{
    int channel, res=0;

    /* select register set */
    res|=lvd_qdc_cr_sel(acard, QDC_CR_BL_QTH);

    for (channel=0; channel<16; channel++)
        res|=lvd_i_r(acard->dev, acard->addr, qdc6.noise_qthr_coi[channel],
            thresholds++);

    res|=lvd_qdc_cr_rev(acard);
    return res?plErr_HW:plOK;
}
plerrcode
lvd_qdc_get_threshold(struct lvd_dev* dev, int addr, ems_u32 *thresholds)
{
    static qdcfun funs[]={0,
        lvd_qdcX_get_threshold,
        lvd_qdcX_get_threshold,
        lvd_qdcX_get_threshold,
        lvd_qdcX_get_threshold,
        lvd_qdc5_get_threshold,
        lvd_qdc6_get_threshold,
        lvd_qdc6_get_threshold,
        0
    };
    return access_qdc(dev, addr, thresholds,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_get_threshold");
}
/*****************************************************************************/
/*
 * sets the level threshold used for coincidences and data selection
 */
static plerrcode
lvd_qdc4_set_thrh(struct lvd_acard* acard, ems_u32 *vals, int xxx)
{
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    int res=0;

    ems_i32 channel=vals[0];
    ems_u32 level=vals[1];

    if (channel<0) {
        for (channel=0; channel<16; channel++) {
            res|=lvd_i_w(acard->dev, acard->addr, qdc3.bline_thlevel[channel],
                    level);
            info->thrh_shadow[channel]=level;
        }
        info->thrh_shadow_valid=0xffff;
    } else {
        res=lvd_i_w(acard->dev, acard->addr, qdc3.bline_thlevel[channel], level);
        info->thrh_shadow[channel]=level;
        info->thrh_shadow_valid|=1<<channel;
    }

    return res?plErr_HW:plOK;
}
static plerrcode
lvd_qdc5_set_thrh(struct lvd_acard* acard, ems_u32 *vals, int xxx)
{
    int res=0;

    ems_i32 channel=vals[0];
    ems_u32 level=vals[1];

    /* select register set */
    res|=lvd_qdc_cr_sel(acard, QDC_CR_LTHRES);
    if (channel<0) {
        for (channel=0; channel<16; channel++) {
            res|=lvd_i_w(acard->dev, acard->addr, qdc3.bline_thlevel[channel],
                    level);
        }
    } else {
        res=lvd_i_w(acard->dev, acard->addr, qdc3.bline_thlevel[channel], level);
    }

    res|=lvd_qdc_cr_rev(acard);
    return res?plErr_HW:plOK;
}
static plerrcode
lvd_qdc6_set_thrh(struct lvd_acard* acard, ems_u32 *vals, int xxx)
{
    int res=0;

    ems_i32 channel=vals[0];
    ems_u32 level=vals[1];

    /* select register set */
    res|=lvd_qdc_cr_sel(acard, QDC_CR_LTHRES);
    if (channel<0) {
        for (channel=0; channel<16; channel++) {
            res|=lvd_i_w(acard->dev, acard->addr, qdc6.bline_thlevel[channel],
                    level);
        }
    } else {
        res=lvd_i_w(acard->dev, acard->addr, qdc6.bline_thlevel[channel], level);
    }

    res|=lvd_qdc_cr_rev(acard);
    return res?plErr_HW:plOK;
}
plerrcode
lvd_qdc_set_thrh(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 thrh)
{
    static qdcfun funs[]={0,
        0,
        0,
        0,
        lvd_qdc4_set_thrh,
        lvd_qdc5_set_thrh,
        lvd_qdc6_set_thrh,
        lvd_qdc6_set_thrh,
        0
    };
    ems_u32 val[2];
    val[0]=channel;
    val[1]=thrh;
    return access_qdcs(dev, addr, val,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "qdc_set_thrh");
}
/*****************************************************************************/
/*
 * reads the level threshold used for coincidences and data selection
 */
static plerrcode
lvd_qdc4_get_thrh(struct lvd_acard* acard, ems_u32 *thrh, int xxx)
{
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    int i;

    if (info->thrh_shadow_valid!=0xffff) {
        printf("lvd_qdc4_get_thrh(%d): shadow not valid\n", acard->addr);
        return plErr_InvalidUse;
    }
    for (i=0; i<16; i++)
        *thrh++=info->thrh_shadow[i];

    return plOK;
}
static plerrcode
lvd_qdc5_get_thrh(struct lvd_acard* acard, ems_u32 *thrh, int xxx)
{
    int channel, res=0;

    /* select register set */
    res|=lvd_qdc_cr_sel(acard, QDC_CR_LTHRES);

    for (channel=0; channel<16; channel++) {
        res|=lvd_i_r(acard->dev, acard->addr, qdc3.bline_thlevel[channel],
                thrh++);
    }

    res|=lvd_qdc_cr_rev(acard);
    return res?plErr_HW:plOK;
}
static plerrcode
lvd_qdc6_get_thrh(struct lvd_acard* acard, ems_u32 *thrh, int xxx)
{
    int channel, res=0;

    /* select register set */
    res|=lvd_qdc_cr_sel(acard, QDC_CR_LTHRES);

    for (channel=0; channel<16; channel++) {
        res|=lvd_i_r(acard->dev, acard->addr, qdc6.bline_thlevel[channel],
                thrh++);
    }

    res|=lvd_qdc_cr_rev(acard);
    return res?plErr_HW:plOK;
}
plerrcode
lvd_qdc_get_thrh(struct lvd_dev* dev, int addr, ems_u32 *thrh)
{
    static qdcfun funs[]={0,
        0,
        0,
        0,
        lvd_qdc4_get_thrh,
        lvd_qdc5_get_thrh,
        lvd_qdc6_get_thrh,
        lvd_qdc6_get_thrh,
        0
    };
    return access_qdc(dev, addr, thrh,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_get_thrh");
}
/*****************************************************************************/
static plerrcode
lvd_qdc1_noise(struct lvd_acard* acard, ems_u32 *noise, int xxx)
{
    int i;
    for (i=0; i<16; i++) {
        if (lvd_i_r(acard->dev, acard->addr, qdc1.nq.noise_level[i], noise))
            return plErr_HW;
        noise++;
    }
    return plOK;
}
static plerrcode
lvd_qdc3_noise(struct lvd_acard* acard, ems_u32 *noise, int xxx)
{
    int i;
    for (i=0; i<16; i++) {
        if (lvd_i_r(acard->dev, acard->addr, qdc3.noise_qthr[i], noise))
            return plErr_HW;
        noise++;
    }
    return plOK;
}
static plerrcode
lvd_qdc5_noise(struct lvd_acard* acard, ems_u32 *noise, int xxx)
{
    int i;
    /* select register set */
    lvd_qdc_cr_sel(acard, 0);

    for (i=0; i<16; i++) {
        if (lvd_i_r(acard->dev, acard->addr, qdc3.noise_qthr[i], noise))
            return plErr_HW;
        noise++;
    }
    lvd_qdc_cr_rev(acard);
    return plOK;
}
static plerrcode
lvd_qdc6_noise(struct lvd_acard* acard, ems_u32 *noise, int xxx)
{
    int i;
    /* select register set */
    lvd_qdc_cr_sel(acard, 0);
    for (i=0; i<16; i++) {
        if (lvd_i_r(acard->dev, acard->addr, qdc6.noise_qthr_coi[i], noise))
            return plErr_HW;
        noise++;
    }
    lvd_qdc_cr_rev(acard);
    return plOK;
}
plerrcode
lvd_qdc_noise(struct lvd_dev* dev, int addr, ems_u32 *noise)
{
    static qdcfun funs[]={0,
        lvd_qdc1_noise,
        lvd_qdc1_noise,
        lvd_qdc3_noise,
        lvd_qdc3_noise,
        lvd_qdc5_noise,
        lvd_qdc6_noise,
        lvd_qdc6_noise,
        0
    };
    return access_qdc(dev, addr, noise,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_noise");
}
/*****************************************************************************/
static plerrcode
lvd_qdc1_mean(struct lvd_acard* acard, ems_u32 *mean, int xxx)
{
    int i;
    for (i=0; i<16; i++) {
        if (lvd_i_r(acard->dev, acard->addr, qdc1.mo.mean_level[i], mean))
            return plErr_HW;
        mean++;
    }
    return plOK;
}
static plerrcode
lvd_qdc3_mean(struct lvd_acard* acard, ems_u32 *mean, int xxx)
{
    int i;

    for (i=0; i<16; i++) {
        if (lvd_i_r(acard->dev, acard->addr, qdc3.bline_thlevel[i], mean))
            return plErr_HW;
        mean++;
    }

    return plOK;
}
static plerrcode
lvd_qdc5_mean(struct lvd_acard* acard, ems_u32 *mean, int xxx)
{
    int i;

    /* select register set */
    lvd_qdc_cr_sel(acard, 0);
  
    for (i=0; i<16; i++) {
        if (lvd_i_r(acard->dev, acard->addr, qdc3.bline_thlevel[i], mean))
            return plErr_HW;
        mean++;
    }

    lvd_qdc_cr_rev(acard);
    return plOK;
}
static plerrcode
lvd_qdc6_mean(struct lvd_acard* acard, ems_u32 *mean, int xxx)
{
    int i;

    /* select register set */
    lvd_qdc_cr_sel(acard, 0);
  
    for (i=0; i<16; i++) {
        if (lvd_i_r(acard->dev, acard->addr, qdc6.bline_thlevel[i], mean))
            return plErr_HW;
        mean++;
    }

    lvd_qdc_cr_rev(acard);
    return plOK;
}
plerrcode
lvd_qdc_mean(struct lvd_dev* dev, int addr, ems_u32 *mean)
{
    static qdcfun funs[]={0,
        lvd_qdc1_mean,
        lvd_qdc1_mean,
        lvd_qdc3_mean,
        lvd_qdc3_mean,
        lvd_qdc5_mean,
        lvd_qdc6_mean,
        lvd_qdc6_mean,
        0
    };
    return access_qdc(dev, addr, mean,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_mean");
}
/*****************************************************************************/
static plerrcode
lvd_qdc3_ovr(struct lvd_acard* acard, ems_u32 *ovr, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc3.ovr, ovr+idx)
            ?plErr_HW:plOK;
}
plerrcode
lvd_qdc_ovr(struct lvd_dev* dev, int addr, ems_u32 *ovr)
{
    static qdcfun funs[]={0,
            0,
            0,
            lvd_qdc3_ovr,
            lvd_qdc3_ovr,
            lvd_qdc3_ovr,
            lvd_qdc3_ovr,
            lvd_qdc3_ovr,
            0
    };
    if (addr<0)
        memset(ovr, 0, 16*sizeof(ems_u32));
    return access_qdcs(dev, addr, ovr,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_ovr");
}
/*****************************************************************************/
static plerrcode
lvd_qdc3_bline_adjust(struct lvd_acard* acard, ems_u32 *bline_adjust, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc3.bline_adjust,
            bline_adjust+idx)?plErr_HW:plOK;
}
static plerrcode
lvd_qdc6_bline_adjust(struct lvd_acard* acard, ems_u32 *bline_adjust, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc6.bline_adjust,
            bline_adjust+idx)?plErr_HW:plOK;
}
static plerrcode
lvd_qdc6s_bline_adjust(struct lvd_acard* acard, ems_u32 *bline_adjust, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc6s.ctrl_bline_adjust,
            bline_adjust+idx)?plErr_HW:plOK;
}
plerrcode
lvd_qdc_bline_adjust(struct lvd_dev* dev, int addr, ems_u32 *bline_adjust)
{
    static qdcfun funs[]={0,
            0,
            0,
            lvd_qdc3_bline_adjust,
            lvd_qdc3_bline_adjust,
            lvd_qdc3_bline_adjust,
            lvd_qdc6_bline_adjust,
            lvd_qdc6s_bline_adjust,
            0
    };
    if (addr<0)
        memset(bline_adjust, 0, 16*sizeof(ems_u32));
    return access_qdcs(dev, addr, bline_adjust,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_bline_adjust");
}
/*****************************************************************************/
static plerrcode
lvd_qdc1_set_inhibit(struct lvd_acard *acard, ems_u32* mask, int xxx)
{
    printf("qdc1_set_inhibit(%x): %04x\n", acard->addr, *mask);
    return lvd_i_w(acard->dev, acard->addr, qdc1.cha_inh, *mask)?plErr_HW:plOK;
}
static plerrcode
lvd_qdc3_set_inhibit(struct lvd_acard *acard, ems_u32* mask, int xxx)
{
    printf("qdc1_set_inhibit(%x): %04x\n", acard->addr, *mask);
    return lvd_i_w(acard->dev, acard->addr, qdc3.cha_inh, *mask)?plErr_HW:plOK;
}
plerrcode
lvd_qdc_set_inhibit(struct lvd_dev* dev, int addr, ems_u32 mask)
{
    static qdcfun funs[]={0,
            lvd_qdc1_set_inhibit,
            lvd_qdc1_set_inhibit,
            lvd_qdc3_set_inhibit,
            lvd_qdc3_set_inhibit,
            lvd_qdc3_set_inhibit,
            lvd_qdc3_set_inhibit,
            lvd_qdc3_set_inhibit,
            0
    };
    return access_qdcs(dev, addr, &mask,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_set_inhibit");
}
/*****************************************************************************/
static plerrcode
lvd_qdc1_get_inhibit(struct lvd_acard *acard, ems_u32* mask, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc1.cha_inh, mask+idx)
            ?plErr_HW:plOK;
}
static plerrcode
lvd_qdc3_get_inhibit(struct lvd_acard *acard, ems_u32* mask, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc3.cha_inh, mask+idx)
            ?plErr_HW:plOK;
}
plerrcode
lvd_qdc_get_inhibit(struct lvd_dev* dev, int addr, ems_u32* mask)
{
    static qdcfun funs[]={0,
            lvd_qdc1_get_inhibit,
            lvd_qdc1_get_inhibit,
            lvd_qdc3_get_inhibit,
            lvd_qdc3_get_inhibit,
            lvd_qdc3_get_inhibit,
            lvd_qdc3_get_inhibit,
            lvd_qdc3_get_inhibit,
            0
    };
    if (addr<0)
        memset(mask, 0, 16*sizeof(ems_u32));
    return access_qdcs(dev, addr, mask,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_get_inhibit");
}
/*****************************************************************************/
static plerrcode
lvd_qdc1_set_raw(struct lvd_acard *acard, ems_u32* mask, int xxx)
{
    return lvd_i_w(acard->dev, acard->addr, qdc1.cha_raw, *mask)?plErr_HW:plOK;
}
static plerrcode
lvd_qdc3_set_raw(struct lvd_acard *acard, ems_u32* mask, int xxx)
{
    return lvd_i_w(acard->dev, acard->addr, qdc3.cha_raw, *mask)?plErr_HW:plOK;
}
plerrcode
lvd_qdc_set_raw(struct lvd_dev* dev, int addr, ems_u32 mask)
{
    static qdcfun funs[]={0,
            lvd_qdc1_set_raw,
            lvd_qdc1_set_raw,
            lvd_qdc3_set_raw,
            lvd_qdc3_set_raw, 
            lvd_qdc3_set_raw,
            lvd_qdc3_set_raw,
            lvd_qdc3_set_raw,
            0
    };
    return access_qdcs(dev, addr, &mask,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_set_raw");
}
/*****************************************************************************/
static plerrcode
lvd_qdc1_get_raw(struct lvd_acard *acard, ems_u32* mask, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc1.cha_raw, mask+idx)
            ?plErr_HW:plOK;
}
static plerrcode
lvd_qdc3_get_raw(struct lvd_acard *acard, ems_u32* mask, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc3.cha_raw, mask+idx)
            ?plErr_HW:plOK;
}
plerrcode
lvd_qdc_get_raw(struct lvd_dev* dev, int addr, ems_u32* mask)
{
    static qdcfun funs[]={0,
            lvd_qdc1_get_raw,
            lvd_qdc1_get_raw,
            lvd_qdc3_get_raw,
            lvd_qdc3_get_raw, 
            lvd_qdc3_get_raw,
            lvd_qdc3_get_raw, 
            lvd_qdc3_get_raw,
            0
    };
    if (addr<0)
        memset(mask, 0, 16*sizeof(ems_u32));
    return access_qdcs(dev, addr, mask,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_get_raw");
}
/*****************************************************************************/
static plerrcode
lvd_qdc1_set_sw_start(struct lvd_acard *acard, ems_u32* start, int xxx)
{
    return lvd_i_w(acard->dev, acard->addr, qdc1.latency, *start)?plErr_HW:plOK;
}
static plerrcode
lvd_qdc3_set_sw_start(struct lvd_acard *acard, ems_u32* start, int xxx)
{
    return lvd_i_w(acard->dev, acard->addr, qdc3.sw_start, *start)?plErr_HW:plOK;
}
plerrcode
lvd_qdc_set_sw_start(struct lvd_dev* dev, int addr, ems_u32 start)
{
    static qdcfun funs[]={0,
            lvd_qdc1_set_sw_start,
            lvd_qdc1_set_sw_start,
            lvd_qdc3_set_sw_start,
            lvd_qdc3_set_sw_start,
            lvd_qdc3_set_sw_start,
            lvd_qdc3_set_sw_start,
            lvd_qdc3_set_sw_start,
            0
    };
    return access_qdcs(dev, addr, &start,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_set_sw_start");
}
/*****************************************************************************/
static plerrcode
lvd_qdc1_get_sw_start(struct lvd_acard *acard, ems_u32* start, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc1.latency, start+idx)
            ?plErr_HW:plOK;
}
static plerrcode
lvd_qdc3_get_sw_start(struct lvd_acard *acard, ems_u32* start, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc3.sw_start, start+idx)
            ?plErr_HW:plOK;
}
plerrcode
lvd_qdc_get_sw_start(struct lvd_dev* dev, int addr, ems_u32* start)
{
    static qdcfun funs[]={0,
            lvd_qdc1_get_sw_start,
            lvd_qdc1_get_sw_start,
            lvd_qdc3_get_sw_start,
            lvd_qdc3_get_sw_start,
            lvd_qdc3_get_sw_start,
            lvd_qdc3_get_sw_start,
            lvd_qdc3_get_sw_start,
            0
    };
    if (addr<0)
        memset(start, 0, 16*sizeof(ems_u32));
    return access_qdcs(dev, addr, start,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_get_sw_start");
}
/*****************************************************************************/
static plerrcode
lvd_qdc1_set_sw_length(struct lvd_acard *acard, ems_u32* len, int xxx)
{
    return lvd_i_w(acard->dev, acard->addr, qdc1.window, *len)?plErr_HW:plOK;
}
static plerrcode
lvd_qdc3_set_sw_length(struct lvd_acard *acard, ems_u32* len, int xxx)
{
    return lvd_i_w(acard->dev, acard->addr, qdc3.sw_length, *len)?plErr_HW:plOK;
}
plerrcode
lvd_qdc_set_sw_length(struct lvd_dev* dev, int addr, ems_u32 length)
{
    static qdcfun funs[]={0,
            lvd_qdc1_set_sw_length,
            lvd_qdc1_set_sw_length,
            lvd_qdc3_set_sw_length,
            lvd_qdc3_set_sw_length,
            lvd_qdc3_set_sw_length,
            lvd_qdc3_set_sw_length,
            lvd_qdc3_set_sw_length,
            0
    };
    return access_qdcs(dev, addr, &length,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_set_sw_length");
}
/*****************************************************************************/
static plerrcode
lvd_qdc1_get_sw_length(struct lvd_acard *acard, ems_u32* len, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc1.window, len+idx)
            ?plErr_HW:plOK;
}
static plerrcode
lvd_qdc3_get_sw_length(struct lvd_acard *acard, ems_u32* len, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc3.sw_length, len+idx)
            ?plErr_HW:plOK;
}
plerrcode
lvd_qdc_get_sw_length(struct lvd_dev* dev, int addr, ems_u32* length)
{
    static qdcfun funs[]={0,
            lvd_qdc1_get_sw_length,
            lvd_qdc1_get_sw_length,
            lvd_qdc3_get_sw_length,
            lvd_qdc3_get_sw_length,
            lvd_qdc3_get_sw_length,
            lvd_qdc3_get_sw_length,
            lvd_qdc3_get_sw_length,
            0
    };
    if (addr<0)
        memset(length, 0, 16*sizeof(ems_u32));
    return access_qdcs(dev, addr, length,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_get_sw_length");
}
/*****************************************************************************/
static plerrcode
lvd_qdc1_set_sw_ilength(struct lvd_acard *acard, ems_u32* len, int xxx)
{
    return lvd_i_w(acard->dev, acard->addr, qdc1.int_length, *len)?plErr_HW:plOK;
}
static plerrcode
lvd_qdc3_set_sw_ilength(struct lvd_acard *acard, ems_u32* len, int xxx)
{
    return lvd_i_w(acard->dev, acard->addr, qdc3.sw_ilength, *len)?plErr_HW:plOK;
}
plerrcode
lvd_qdc_set_sw_ilength(struct lvd_dev* dev, int addr, ems_u32 length)
{
    static qdcfun funs[]={0,
            lvd_qdc1_set_sw_ilength,
            lvd_qdc1_set_sw_ilength,
            lvd_qdc3_set_sw_ilength,
            lvd_qdc3_set_sw_ilength,
            lvd_qdc3_set_sw_ilength,
            lvd_qdc3_set_sw_ilength,
            lvd_qdc3_set_sw_ilength,
            0
    };
    return access_qdcs(dev, addr, &length,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_set_sw_ilength");
}
/*****************************************************************************/
static plerrcode
lvd_qdc1_get_sw_ilength(struct lvd_acard *acard, ems_u32* len, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc1.int_length, len+idx)
            ?plErr_HW:plOK;
}
static plerrcode
lvd_qdc3_get_sw_ilength(struct lvd_acard *acard, ems_u32* len, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc3.sw_ilength, len+idx)
            ?plErr_HW:plOK;
}
plerrcode
lvd_qdc_get_sw_ilength(struct lvd_dev* dev, int addr, ems_u32* length)
{
    static qdcfun funs[]={0,
            lvd_qdc1_get_sw_ilength,
            lvd_qdc1_get_sw_ilength,
            lvd_qdc3_get_sw_ilength,
            lvd_qdc3_get_sw_ilength,
            lvd_qdc3_get_sw_ilength,
            lvd_qdc3_get_sw_ilength,
            lvd_qdc3_get_sw_ilength,
            0
    };
    if (addr<0)
        memset(length, 0, 16*sizeof(ems_u32));
    return access_qdcs(dev, addr, length,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_get_sw_ilength");
}
/*****************************************************************************/
static plerrcode
lvd_qdc1_set_iw_start(struct lvd_acard *acard, ems_u32* start, int xxx)
{
    return lvd_i_w(acard->dev, acard->addr, qdc1.iw_start, *start)?plErr_HW:plOK;
}
static plerrcode
lvd_qdc3_set_iw_start(struct lvd_acard *acard, ems_u32* start, int xxx)
{
    return lvd_i_w(acard->dev, acard->addr, qdc3.iw_start, *start)?plErr_HW:plOK;
}
plerrcode
lvd_qdc_set_iw_start(struct lvd_dev* dev, int addr, ems_u32 start)
{
    static qdcfun funs[]={0,
            lvd_qdc1_set_iw_start,
            lvd_qdc1_set_iw_start,
            lvd_qdc3_set_iw_start,
            lvd_qdc3_set_iw_start,
            lvd_qdc3_set_iw_start,
            lvd_qdc3_set_iw_start,
            lvd_qdc3_set_iw_start,
            0
    };
    return access_qdcs(dev, addr, &start,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_set_iw_start");
}
/*****************************************************************************/
static plerrcode
lvd_qdc1_get_iw_start(struct lvd_acard *acard, ems_u32* start, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc1.iw_start, start+idx)
            ?plErr_HW:plOK;
}
static plerrcode
lvd_qdc3_get_iw_start(struct lvd_acard *acard, ems_u32* start, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc3.iw_start, start+idx)
            ?plErr_HW:plOK;
}
plerrcode
lvd_qdc_get_iw_start(struct lvd_dev* dev, int addr, ems_u32* start)
{
    static qdcfun funs[]={0,
            lvd_qdc1_get_iw_start,
            lvd_qdc1_get_iw_start,
            lvd_qdc3_get_iw_start,
            lvd_qdc3_get_iw_start,
            lvd_qdc3_get_iw_start,
            lvd_qdc3_get_iw_start,
            lvd_qdc3_get_iw_start,
            0
    };
    if (addr<0)
        memset(start, 0, 16*sizeof(ems_u32));
    return access_qdcs(dev, addr, start,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_get_iw_start");
}
/*****************************************************************************/
static plerrcode
lvd_qdc1_set_iw_length(struct lvd_acard *acard, ems_u32* len, int xxx)
{
    return lvd_i_w(acard->dev, acard->addr, qdc1.iw_length, *len)?plErr_HW:plOK;
}
static plerrcode
lvd_qdc3_set_iw_length(struct lvd_acard *acard, ems_u32* len, int xxx)
{
    return lvd_i_w(acard->dev, acard->addr, qdc3.iw_length, *len)?plErr_HW:plOK;
}
plerrcode
lvd_qdc_set_iw_length(struct lvd_dev* dev, int addr, ems_u32 length)
{
    static qdcfun funs[]={0,
            lvd_qdc1_set_iw_length,
            lvd_qdc1_set_iw_length,
            lvd_qdc3_set_iw_length,
            lvd_qdc3_set_iw_length,
            lvd_qdc3_set_iw_length,
            lvd_qdc3_set_iw_length,
            lvd_qdc3_set_iw_length,
            0
    };
    return access_qdcs(dev, addr, &length,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_set_iw_length");
}
/*****************************************************************************/
static plerrcode
lvd_qdc1_get_iw_length(struct lvd_acard *acard, ems_u32* len, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc1.iw_length, len+idx)
            ?plErr_HW:plOK;
}
static plerrcode
lvd_qdc3_get_iw_length(struct lvd_acard *acard, ems_u32* len, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc3.iw_length, len+idx)
            ?plErr_HW:plOK;
}
plerrcode
lvd_qdc_get_iw_length(struct lvd_dev* dev, int addr, ems_u32* length)
{
    static qdcfun funs[]={0,
            lvd_qdc1_get_iw_length,
            lvd_qdc1_get_iw_length,
            lvd_qdc3_get_iw_length,
            lvd_qdc3_get_iw_length,
            lvd_qdc3_get_iw_length,
            lvd_qdc3_get_iw_length,
            lvd_qdc3_get_iw_length,
            0
    };
    if (addr<0)
        memset(length, 0, 16*sizeof(ems_u32));
    return access_qdcs(dev, addr, length,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_get_iw_length");
}
/*****************************************************************************/
static plerrcode
lvd_qdc1_set_anal(struct lvd_acard *acard, ems_u32* anal, int xxx)
{
    return lvd_i_w(acard->dev, acard->addr, qdc1.anal_ctrl, *anal)
            ?plErr_HW:plOK;
}
static plerrcode
lvd_qdc3_set_anal(struct lvd_acard *acard, ems_u32* anal, int xxx)
{
    return lvd_i_w(acard->dev, acard->addr, qdc3.anal_ctrl, *anal)
            ?plErr_HW:plOK;
}
plerrcode
lvd_qdc_set_anal(struct lvd_dev* dev, int addr, ems_u32 anal)
{
    static qdcfun funs[]={0,
            lvd_qdc1_set_anal,
            lvd_qdc1_set_anal,
            lvd_qdc3_set_anal,
            lvd_qdc3_set_anal,
            lvd_qdc3_set_anal,
            lvd_qdc3_set_anal,
            lvd_qdc3_set_anal,
            0
    };
    return access_qdcs(dev, addr, &anal,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_set_anal");
}
/*****************************************************************************/
static plerrcode
lvd_qdc1_get_anal(struct lvd_acard *acard, ems_u32* anal, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc1.anal_ctrl, anal+idx)
            ?plErr_HW:plOK;
}
static plerrcode
lvd_qdc3_get_anal(struct lvd_acard *acard, ems_u32* anal, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc3.anal_ctrl, anal+idx)
            ?plErr_HW:plOK;
}
plerrcode
lvd_qdc_get_anal(struct lvd_dev* dev, int addr, ems_u32* anal)
{
    static qdcfun funs[]={0,
            lvd_qdc1_get_anal,
            lvd_qdc1_get_anal,
            lvd_qdc3_get_anal,
            lvd_qdc3_get_anal,
            lvd_qdc3_get_anal,
            lvd_qdc3_get_anal,
            lvd_qdc3_get_anal,
            0
    };
    if (addr<0)
        memset(anal, 0, 16*sizeof(ems_u32));
    return access_qdcs(dev, addr, anal,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_get_anal");
}
/*****************************************************************************/
static plerrcode
lvd_qdc3_set_rawcycle(struct lvd_acard *acard, ems_u32* rawcycle, int xxx)
{
    return lvd_i_w(acard->dev, acard->addr, qdc3.traw_cycle, *rawcycle)?
            plErr_HW:plOK;
}
static plerrcode
lvd_qdc80_set_rawcycle(struct lvd_acard *acard, ems_u32* rawcycle, int xxx)
{
    int res=0;
    u_int32_t coinmin_traw;
    res|=lvd_i_r(acard->dev, acard->addr, qdc80.coinmin_traw, &coinmin_traw);
    coinmin_traw=(coinmin_traw & 0xf0) | (*rawcycle & 0x0f);
    res|=lvd_i_w(acard->dev, acard->addr, qdc80.coinmin_traw, coinmin_traw);
    return res?plErr_HW:plOK;
}
plerrcode
lvd_qdc_set_rawcycle(struct lvd_dev* dev, int addr, ems_u32 rawcycle)
{
    static qdcfun funs[]={0,
            0,
            0,
            lvd_qdc3_set_rawcycle,
            lvd_qdc3_set_rawcycle,
            lvd_qdc3_set_rawcycle,
            lvd_qdc3_set_rawcycle,
            lvd_qdc3_set_rawcycle,
            lvd_qdc80_set_rawcycle
    };
    return access_qdcs(dev, addr, &rawcycle,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_set_rawcycle");
}
/*****************************************************************************/
static plerrcode
lvd_qdc3_get_rawcycle(struct lvd_acard *acard, ems_u32* rawcycle, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc3.traw_cycle, rawcycle+idx)
            ?plErr_HW:plOK;
}
static plerrcode
lvd_qdc80_get_rawcycle(struct lvd_acard *acard, ems_u32* rawcycle, int idx)
{
    int res;
    u_int32_t coinmin_traw;
    res=lvd_i_r(acard->dev, acard->addr, qdc80.coinmin_traw, &coinmin_traw);
    rawcycle[idx]=coinmin_traw & 0xf;
    return res?plErr_HW:plOK;
}
plerrcode
lvd_qdc_get_rawcycle(struct lvd_dev* dev, int addr, ems_u32* rawcycle)
{
    static qdcfun funs[]={0,
            0,
            0,
            lvd_qdc3_get_rawcycle,
            lvd_qdc3_get_rawcycle,
            lvd_qdc3_get_rawcycle,
            lvd_qdc3_get_rawcycle,
            lvd_qdc3_get_rawcycle,
            lvd_qdc80_get_rawcycle
    };
    if (addr<0)
        memset(rawcycle, 0, 16*sizeof(ems_u32));
    return access_qdcs(dev, addr, rawcycle,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_get_rawcycle");
}
/*****************************************************************************/
static plerrcode
lvd_qdc6_set_grp_coinc(struct lvd_acard* acard, ems_u32 *pattern, int xxx)
{
    return lvd_i_w(acard->dev, acard->addr, qdc6.grp_coinc, *pattern)
            ?plErr_HW:plOK;
}
static plerrcode
lvd_qdc80_set_grp_coinc(struct lvd_acard* acard, ems_u32 *pattern, int xxx)
{
    return lvd_i_w(acard->dev, acard->addr, qdc80.grp_coinc, *pattern)
            ?plErr_HW:plOK;
}

plerrcode
lvd_qdc_set_grp_coinc(struct lvd_dev* dev, int addr, ems_u32 pattern)
{
    static qdcfun funs[]={0,
            0,
            0,
            0,
            0,
            0,
            lvd_qdc6_set_grp_coinc,
            lvd_qdc6_set_grp_coinc,
            lvd_qdc80_set_grp_coinc
    };
    return access_qdcs(dev, addr, &pattern,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_set_grp_coinc");
}
/*****************************************************************************/
static plerrcode
lvd_qdc6_get_grp_coinc(struct lvd_acard* acard, ems_u32 *pattern, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc6.grp_coinc, pattern+idx)
            ?plErr_HW:plOK;
}
static plerrcode
lvd_qdc80_get_grp_coinc(struct lvd_acard* acard, ems_u32 *pattern, int idx)
{
    return lvd_i_r(acard->dev, acard->addr, qdc80.grp_coinc, pattern+idx)
            ?plErr_HW:plOK;
}

plerrcode
lvd_qdc_get_grp_coinc(struct lvd_dev* dev, int addr, ems_u32 *pattern)
{
    static qdcfun funs[]={0,
            0,
            0,
            0,
            0,
            0,
            lvd_qdc6_get_grp_coinc,
            lvd_qdc6_get_grp_coinc,
            lvd_qdc80_get_grp_coinc
            };
    if (addr<0)
        memset(pattern, 0, 16*sizeof(ems_u32));
    return access_qdcs(dev, addr, pattern,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_get_grp_coinc");
}
/*****************************************************************************/
static plerrcode
lvd_qdc6_set_coinc(struct lvd_acard* acard, ems_u32 *val, int xxx)
{
    ems_i32 group=val[0];
    ems_u32 pattern=val[1];
    int res=0;

    /* select register set  cr<13>=1 */
    lvd_qdc_cr_sel(acard, QDC_CR_LTHRES);

    if (group<0) {
        for (group=0; group<4; group++)
            res|=lvd_i_w(acard->dev, acard->addr,
                    qdc6.dac_coinc[group], pattern);
    } else {
        res|=lvd_i_w(acard->dev, acard->addr, qdc6.dac_coinc[group], pattern);
    }

    lvd_qdc_cr_rev(acard);
    return res?plErr_HW:plOK;
}
static plerrcode
lvd_qdc80_set_coinc(struct lvd_acard* acard, ems_u32 *val, int xxx)
{
    ems_i32 group=val[0];
    ems_u32 pattern=val[1];
    int res=0;

    if (group<0)
        group=0x10;

    /* select FPGA (0x10 selects all four FPGAs) */
    res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, group);
    /* write data */
    res|=lvd_i_w(acard->dev, acard->addr, qdc80.coinc_tab, pattern);

    return res?plErr_HW:plOK;
}
plerrcode
lvd_qdc_set_coinc(struct lvd_dev* dev, int addr, ems_i32 group, ems_u32 pattern)
{
    static qdcfun funs[]={0,
            0,
            0,
            0,
            0,
            0,
            lvd_qdc6_set_coinc,
            lvd_qdc6_set_coinc,
            lvd_qdc80_set_coinc
           };
    ems_u32 val[2];
    val[0]=group;
    val[1]=pattern;
    return access_qdcs(dev, addr, val,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_set_coinc");
}
/*****************************************************************************/
/*
 * Here we have a little problem: we need the group and the destination pointer.
 * We only have one ems_u32 pointer.
 * For now we misuse it a little bit, but we should find a general and clear
 * solution at a later time. (Just a void pointer instead of ems_u32?) 
 * static plerrcode
 */
struct get_coinc_data {
    ems_u32 *dest;
    int group;
};
static plerrcode
lvd_qdc6_get_coinc(struct lvd_acard* acard, ems_u32 *data_, int idx)
{
    int res=0;
    struct get_coinc_data *data=(struct get_coinc_data*)data_;

    /* select register set  cr<13>=1 */
    lvd_qdc_cr_sel(acard, QDC_CR_LTHRES);

    if (data->group<0) {
        int group;
        for (group=0; group<4; group++)
            res|=lvd_i_r(acard->dev, acard->addr, qdc6.dac_coinc[group],
                    data->dest+4*idx+group);
    } else {
        res|=lvd_i_r(acard->dev, acard->addr, qdc6.dac_coinc[data->group],
                data->dest+idx);
    }

    lvd_qdc_cr_rev(acard);
    return res?plErr_HW:plOK;
}
static plerrcode
lvd_qdc80_get_coinc(struct lvd_acard* acard, ems_u32 *data_, int idx)
{
    int res=0;
    struct get_coinc_data *data=(struct get_coinc_data*)data_;

    if (data->group<0) {
        int group;
        for (group=0; group<4; group++) {
            /* select FPGA */
            res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->group);
            /* read data */
            res|=lvd_i_r(acard->dev, acard->addr, qdc80.coinc_tab,
                    data->dest+4*idx+group);
        }
    } else {
        res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->group);
        res|=lvd_i_r(acard->dev, acard->addr, qdc80.coinc_tab,
                data->dest+idx);
    }

    return res?plErr_HW:plOK;
}
plerrcode
lvd_qdc_get_coinc(struct lvd_dev* dev, int addr, int group, ems_u32* pattern)
{
    static qdcfun funs[]={0,
            0,
            0,
            0,
            0,
            0,
            lvd_qdc6_get_coinc,
            lvd_qdc6_get_coinc,
            lvd_qdc80_get_coinc
            };
    struct get_coinc_data data;
    data.dest=pattern;
    data.group=group;
    if (addr<0)
        memset(pattern, 0, 64*sizeof(ems_u32));
    return access_qdcs(dev, addr, (ems_u32*)&data,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_get_coinc");
}
/*****************************************************************************/
static plerrcode
lvd_qdc6s_set_sc_rate(struct lvd_acard* acard, ems_u32 *val, int xxx)
{
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    int res;

    res=lvd_i_w(acard->dev, acard->addr, qdc6s.scaler_rate, *val);
    if (res) {
        return plErr_HW;
    } else {
        info->scaler_rate_shadow=*val;
        info->scaler_rate_shadow_valid=1;
        return plOK;
    }
}
static plerrcode
lvd_qdc80_set_sc_rate(struct lvd_acard* acard, ems_u32 *val, int xxx)
{
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    int res;

    res=lvd_i_w(acard->dev, acard->addr, qdc80.scaler_rout, *val);
    if (res) {
        return plErr_HW;
    } else {
        info->scaler_rate_shadow=*val;
        info->scaler_rate_shadow_valid=1;
        return plOK;
    }
}
plerrcode
lvd_qdc_set_sc_rate(struct lvd_dev* dev, int addr, ems_u32 val)
{
    static qdcfun funs[]={0,
            0,
            0,
            0,
            0,
            0,
            0,
            lvd_qdc6s_set_sc_rate,
            lvd_qdc80_set_sc_rate
    };
    return access_qdcs(dev, addr, &val,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_set_sc_rate");
}
/*****************************************************************************/
static plerrcode
lvd_qdc6_get_sc_rate(struct lvd_acard* acard, ems_u32 *val, int idx)
{
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    if (!info->scaler_rate_shadow_valid) {
        printf("lvd_qdc6_get_sc_rate(%d): shadow not valid\n",
                acard->addr);
        return plErr_InvalidUse;
    } else {
        val[idx]=info->scaler_rate_shadow;
        return plOK;
    }
}
static plerrcode
lvd_qdc80_get_sc_rate(struct lvd_acard* acard, ems_u32 *val, int idx)
{
    int res;
    res=lvd_i_r(acard->dev, acard->addr, qdc80.scaler_rout, val+idx);
    return res?plErr_HW:plOK;
}
plerrcode
lvd_qdc_get_sc_rate(struct lvd_dev* dev, int addr, ems_u32 *val)
{
    static qdcfun funs[]={0,
            0,
            0,
            0,
            0,
            0,
            0,
            lvd_qdc6_get_sc_rate,
            lvd_qdc80_get_sc_rate
            };
    if (addr<0)
        memset(val, 0, 16*sizeof(ems_u32));
    return access_qdcs(dev, addr, val,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_get_sc_rate");
}
/*****************************************************************************/
/* lvd_qdc_sc_init initializes the scaler function of QDCs
 * the shadow memory is filled with zeros
 * and the hardware is cleared
 */
static plerrcode
lvd_qdc6s_sc_init(struct lvd_acard *acard, ems_u32* val, int xxx)
{
    int res;
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    /* clear shadow data */
    info->scaler_enabled=0;
    memset(info->scaler_u, 0, sizeof(info->scaler_u));
    memset(info->scaler_l, 0, sizeof(info->scaler_l));
    /* reset scaler hardware */
    res=lvd_i_w(acard->dev, acard->addr, qdc6s.scaler, 0x8000);
    if (res==0 && val[0]<=val[1]) {
        info->scaler_first=val[0];
        info->scaler_last=val[1];
        info->scaler_enabled=1;
    }
    return res?plErr_HW:plOK;
}
static plerrcode
lvd_qdc80_sc_init(struct lvd_acard *acard, ems_u32* val, int xxx)
{
    int res;
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    /* clear shadow data */
    info->scaler_enabled=0;
    memset(info->scaler_u, 0, sizeof(info->scaler_u));
    memset(info->scaler_l, 0, sizeof(info->scaler_l));
    /* reset scaler hardware */
    res=lvd_i_w(acard->dev, acard->addr, qdc80.scaler, 0x8000);
    if (res==0 && val[0]<=val[1]) {
        info->scaler_first=val[0];
        info->scaler_last=val[1];
        info->scaler_enabled=1;
    }
    return res?plErr_HW:plOK;
}
plerrcode
lvd_qdc_sc_init(struct lvd_dev* dev, int addr, int first, int last)
{
    static qdcfun funs[]={0,
            0,
            0,
            0,
            0,
            0,
            0,
            lvd_qdc6s_sc_init,
            lvd_qdc80_sc_init
    };
    ems_u32 val[2];
    val[0]=first;
    val[1]=last;
    return access_qdcs(dev, addr, val,
                funs, sizeof(funs)/sizeof(qdcfun),
                mtypes_FQDCs,
                "lvd_qdc_sc_init");
}
/*****************************************************************************/
/* lvd_qdc_sc_clear clears all scalers channels
 * the shadow memory is filled with zeros
 * and the hardware is cleared
 */
static plerrcode
lvd_qdc6s_sc_clear(struct lvd_acard *acard, ems_u32* vvv, int xxx)
{
    int res;
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    /* clear shadow data */
    memset(info->scaler_u, 0, sizeof(info->scaler_u));
    memset(info->scaler_l, 0, sizeof(info->scaler_l));
    /* reset scaler hardware */
    res=lvd_i_w(acard->dev, acard->addr, qdc6s.scaler, 0x8000);
    return res?plErr_HW:plOK;
}
static plerrcode
lvd_qdc80_sc_clear(struct lvd_acard *acard, ems_u32* vvv, int xxx)
{
    int res;
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    /* clear shadow data */
    memset(info->scaler_u, 0, sizeof(info->scaler_u));
    memset(info->scaler_l, 0, sizeof(info->scaler_l));
    /* reset scaler hardware */
    res=lvd_i_w(acard->dev, acard->addr, qdc80.scaler, 0x8000);
    return res?plErr_HW:plOK;
}
plerrcode
lvd_qdc_sc_clear(struct lvd_dev* dev, int addr)
{
    static qdcfun funs[]={0,
            0,
            0,
            0,
            0,
            0,
            0,
            lvd_qdc6s_sc_clear,
            lvd_qdc80_sc_clear
    };
    return access_qdcs(dev, addr, 0,
                funs, sizeof(funs)/sizeof(qdcfun),
                mtypes_FQDCs,
                "lvd_qdc_sc_clear");
}
/*****************************************************************************/
/* lvd_qdc_sc_update updates the shadow registers for all enabled channels
 */
static plerrcode
lvd_qdc6s_sc_update(struct lvd_acard *acard, ems_u32* ooo, int xxx)
{
    int res=0, i;
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;

    if (!info->scaler_enabled)
        return plOK;

    /* set index register */
    res|=lvd_i_w(acard->dev, acard->addr, qdc6s.scaler, info->scaler_first);
    /* read the scalers */
    for (i=info->scaler_first; i<=info->scaler_last; i++) {
        ems_u32 val;
        /* read the hardware (32 bit only) */
        res|=lvd_i_r(acard->dev, acard->addr, qdc6s.scaler, &val);
        /* check for overflow */
        if (val<info->scaler_l[i])
            info->scaler_u[i]++;
        /* store the new value */
        info->scaler_l[i]=val;
    }

    return res?plErr_HW:plOK;
}
static plerrcode
lvd_qdc80_sc_update(struct lvd_acard *acard, ems_u32* ooo, int xxx)
{
    int res=0, i;
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;

    if (!info->scaler_enabled)
        return plOK;

    /* set index register */
    res|=lvd_i_w(acard->dev, acard->addr, qdc80.scaler, info->scaler_first);
    /* read the scalers */
    for (i=info->scaler_first; i<=info->scaler_last; i++) {
        ems_u32 val;
        /* read the hardware (32 bit only) */
        res|=lvd_i_r(acard->dev, acard->addr, qdc80.scaler, &val);
        /* check for overflow */
        if (val<info->scaler_l[i])
            info->scaler_u[i]++;
        /* store the new value */
        info->scaler_l[i]=val;
    }

    return res?plErr_HW:plOK;
}
plerrcode
lvd_qdc_sc_update(struct lvd_dev* dev)
{
    static qdcfun funs[]={0,
            0,
            0,
            0,
            0,
            0,
            0,
            lvd_qdc6s_sc_update,
            lvd_qdc80_sc_update
    };
    return access_qdcs(dev, -1, 0,
            funs, sizeof(funs)/sizeof(qdcfun),
            mtypes_FQDCs,
            "lvd_qdc_sc_ini");
}
/*****************************************************************************/
/* lvd_qdc_sc_get does not touch the hardware
 * lvd_qdc_sc_update has to be called before
 * only data for scalers which are both enabled and selected in the
 * given mask are transfered
 * WARNING:
 * It is not yet known in which order the data will appear if -1 is given
 * as module address. (but we will write the modules address to the
 * data stream)
 */
struct sc_get_data {
    ems_u32* dest;
    ems_u32  mask;
};
static plerrcode
lvd_qdc6_sc_get(struct lvd_acard *acard, ems_u32* data_, int addr)
{
    int i, idx;
    struct qdc_info* info=(struct qdc_info*)acard->cardinfo;
    struct sc_get_data *data=(struct sc_get_data*)data_;

    if (!info->scaler_enabled)
        return plOK;

    idx=2; /* space for module address and channel counter */

    /* copy the data */
    for (i=info->scaler_first; i<=info->scaler_last; i++) {
        if (data->mask&(1<<i)) {
            data->dest[idx++]=info->scaler_u[i];
            data->dest[idx++]=info->scaler_l[i];
        }
    }
    /* store address and channel counter and update destination pointer */
    if (idx>2) { /* change dest only if we have real data */
        data->dest[0]=addr;
        data->dest[1]=(idx-2)/2;
        data->dest+=idx;
    }

    return plOK;
}
plerrcode
lvd_qdc_sc_get(struct lvd_dev* dev, int addr, ems_u32 mask,
        ems_u32 *dest, int *num)
{
    plerrcode pres;
    static qdcfun funs[]={0,
            0,
            0,
            0,
            0,
            0,
            0,
            lvd_qdc6_sc_get,
            lvd_qdc6_sc_get
    };
    struct sc_get_data data;
    data.dest=dest;
    data.mask=mask;
    pres=access_qdcs(dev, addr, (ems_u32*)&data,
            funs, sizeof(funs)/sizeof(qdcfun),
            mtypes_FQDCs,
            "lvd_qdc_sc_get");
    if (pres==plOK)
        *num=data.dest-dest;
    return pres;
}

/*****************************************************************************/
/*****************************************************************************/
static int
lvd_start_qdc(struct lvd_dev* dev, struct lvd_acard* acard)
{
    struct qdc_info *info=(struct qdc_info*)acard->cardinfo;
    return lvd_qdc_cr_set(acard, acard->daqmode|info->f1flag);
}
/*****************************************************************************/
static int
lvd_stop_qdc(struct lvd_dev* dev, struct lvd_acard* acard)
{
    return lvd_qdc_cr_set(acard, 0);
}
/*****************************************************************************/
static int
lvd_clear_qdc(struct lvd_dev* dev, struct lvd_acard* acard)
{
    int res=0, addr=acard->addr;
    ems_u32 sr;
    res|=lvd_i_r(dev, addr, all.sr, &sr);
    res|=lvd_i_w(dev, addr, all.sr, sr);
    return res?-1:0;
}
/*****************************************************************************/
/* initialize one QDC card */
static plerrcode
lvd_qdcX_init(struct lvd_acard* acard, ems_u32 *mode, int xxx)
{
    struct qdc_info *info=(struct qdc_info*)acard->cardinfo;
    struct lvd_dev* dev=acard->dev;
    int res=0, addr=acard->addr;
    plerrcode pres;

    printf("initializing QDC card 0x%x, mode 0x%04x\n", addr, *mode);

    if (!dev->hsdiv) {
        complain("lvd_qdc_init_card: controller %d not initialised", addr>>4);
        return plErr_ArgRange;
    }

    switch (dev->contr_mode) {
    case contr_f1:
        info->f1flag=QDC_CR_F1MODE;
        break;
    case contr_gpx:
        info->f1flag=0;
        break;
    default:
        complain("lvd_qdc_init: illegal controller mode %d", dev->contr_mode);
        return plErr_BadModTyp;
    }

    /* try to set the F1-flag if desired */
    if (info->f1flag) {
        ems_u32 val;
        res|=lvd_qdc_cr_set(acard, info->f1flag);
        res|=lvd_i_r(dev, addr, all.cr, &val);
        res|=lvd_qdc_cr_set(acard, 0);
        if (val!=info->f1flag) {
            complain("lvd_qdc_init_card(0x%x): cannot set F1-flag; firmware "
                    "too old (or module broken)", addr);
            return plErr_BadModTyp;
        }
    }

    acard->daqmode=*mode;
    if (info->qdcver>=3)
        res|=lvd_i_w(dev, addr, qdc3.tdc_range, dev->tdc_range);
    else
        res|=lvd_i_w(dev, addr, qdc1.tdc_range, dev->tdc_range);

    if (res)
        return plErr_System;

    if (acard->mtype==ZEL_LVD_SQDC) {
        /* set all DACs and shadow to 0 */
        if ((pres=lvd_qdc_set_dac(dev, addr, -1, 0))!=plOK)
            return pres;
    } else { /* FQDC or VFQDC */
        /* set testdac and shadow to 0 */
        if (info->qdcver>=4) {
            if ((pres=lvd_qdc_set_testdac(dev, addr, 0))!=plOK)
                return pres;
        }
    }
    if (info->qdcver>=3) {
        /* set baseline and shadow to 0x800 */
        if ((pres=lvd_qdc_set_ped(dev, addr, -1, 0x800))!=plOK)
            return pres;
    }
    if (info->qdcver>=3) {
        /* set threshold and shadow to 0 */
        if ((pres=lvd_qdc_set_threshold(dev, addr, -1, 0))!=plOK)
            return pres;
    }
    if (info->qdcver>=4) {
        /* set thrh_level and shadow to 0x10 */
        if ((pres=lvd_qdc_set_thrh(dev, addr, -1, 0x10))!=plOK)
            return pres;
    }
    if (info->qdcver>=7) {
        /* disable scaler readout */
        if ((pres=lvd_qdc_set_sc_rate(dev, addr, 0))!=plOK)
            return pres;
        info->scaler_enabled=0;
    }

    printf("qdc_init_card: input card 0x%x initialized.\n", addr);
    acard->initialized=1;

    return plOK;
}

static plerrcode
lvd_qdc80_init(struct lvd_acard* acard, ems_u32 *mode, int xxx)
{
    struct qdc_info *info=(struct qdc_info*)acard->cardinfo;
    struct lvd_dev* dev=acard->dev;
    int res=0, addr=acard->addr;

    printf("initializing QDC card 0x%x, mode 0x%04x\n", addr, *mode);

    if (!dev->hsdiv) {
        complain("lvd_qdc_init_card: controller %d not initialised", addr>>4);
        return plErr_ArgRange;
    }

    switch (dev->contr_mode) {
    case contr_f1:
        info->f1flag=QDC_CR_F1MODE;
        break;
    case contr_gpx:
        info->f1flag=0;
        break;
    default:
        complain("lvd_qdc_init: illegal controller mode %d", dev->contr_mode);
        return plErr_BadModTyp;
    }

    /* set the F1-flag if desired */
    if (info->f1flag) {
        ems_u32 val;
        res|=lvd_qdc_cr_set(acard, info->f1flag);
        res|=lvd_i_r(dev, addr, all.cr, &val);
        res|=lvd_qdc_cr_set(acard, 0);
        if (val!=info->f1flag) {
            complain("lvd_qdc_init_card(0x%x): error setting F1-flag; "
                    "module broken", addr);
            return plErr_HWTestFailed;
        }
    printf("qdc_init_card: input card 0x%x f1flag set.\n", addr);
    }

    acard->daqmode=*mode;
    res|=lvd_i_w(dev, addr, qdc80.tdc_range, dev->tdc_range);

    if (res)
        return plErr_System;

    
    /* disabled scaler readout */
    info->scaler_enabled=0;

    printf("qdc_init_card: input card 0x%x initialized.\n", addr);
    acard->initialized=1;

    return plOK;
}

/* initialize one or more QDC cards */
plerrcode
lvd_qdc_init(struct lvd_dev* dev, int addr, int daqmode)
{
    static qdcfun funs[]={0,
            lvd_qdcX_init,
            lvd_qdcX_init,
            lvd_qdcX_init,
            lvd_qdcX_init,
            lvd_qdcX_init,
            lvd_qdcX_init,
            lvd_qdcX_init,
            lvd_qdc80_init
    };
    plerrcode pres;
    ems_u32 mode=daqmode;

    pres=access_qdcs(dev, addr, &mode,
                funs, sizeof(funs)/sizeof(qdcfun),
                0,
                "lvd_qdc_init");

    if (pres!=plOK)
        return pres;

    printf("qdc_init: all input cards initialized.\n");

    return plOK;
}
/*****************************************************************************/
static void
lvd_qdc_acard_free(struct lvd_dev* dev, struct lvd_acard* acard)
{
    free(acard->cardinfo);
    acard->cardinfo=0;
}
/*****************************************************************************/
int
lvd_qdc_acard_init(struct lvd_dev* dev, struct lvd_acard* acard)
{
    struct qdc_info *info;

    acard->freeinfo=lvd_qdc_acard_free;

    acard->cardinfo=calloc(1, sizeof(struct qdc_info));
    if (!acard->cardinfo) {
        printf("malloc acard->cardinfo(qdc): %s\n", strerror(errno));
        return -1;
    }
    info=acard->cardinfo;
    /*
        qdc versions
        1: uralt
        2: 'normal'
        3: with fixed (manual) baseline
        4: with separate trigger level for each channel
        5: 3 and 4 merged
        6:  feature extraction for trigger and many more for 240MHz
        6.2 : additional self triggering modes
        6.3? Scaler function
        8.* registers completely reordered
    */
    info->qdcver=LVD_FWverH(acard->id);  /* ==1, 2, 3, 4, 5, 6 or 8*/
    info->qdcverL=LVD_FWverL(acard->id); /* needed if qdcver==6 only */
    /* Firmawre        qdcver
     * 
     *    6.0              5
     *    6.1              5
     *    6.2              6
     *    6.3              7
     *    8.*              8
     */
    if (info->qdcver==6) { // version 6.2
        if (info->qdcverL>2)
            info->qdcver=7; // version 6.3 QDC with Scaler functionality
        if (info->qdcverL<2)
            info->qdcver=5;
        /* qdcverL==2: info->qdcver=6 */
    }
    if (info->qdcver>8) {
        complain("unknown QDC version 0x%04x addr %x",
                 acard->id, acard->addr);
        return -1;
    }

    lvd_qdc_cr_set(acard, 0);

#if 0
    switch (acard->mtype) {
    case ZEL_LVD_SQDC:
        info->base_clk=12.5;
        info->adc_clk=12.5;
        break;
    case ZEL_LVD_FQDC:
        info->base_clk=12.5;
        info->adc_clk=6.25;
        break;
    case ZEL_LVD_VFQDC:
        info->base_clk=12.5;
        info->adc_clk=1000./240.;
        break;
    default:
        printf("lvd_qdc_acard_init: unknown QDC type 0x%x\n", acard->mtype);
        return -1;
    }
#else
    switch (LVD_HWtyp(acard->id)) {
    case LVD_CARDID_SQDC:
        info->base_clk=12.5;
        info->adc_clk=1000./80.;
        break;
    case LVD_CARDID_FQDC:
        info->base_clk=12.5;
        info->adc_clk=1000./160.;
        break;
    case LVD_CARDID_VFQDC_200:
        info->base_clk=12.5;
        info->adc_clk=1000./200.;
        break;
    case LVD_CARDID_VFQDC:
        info->base_clk=12.5;
        info->adc_clk=1000./240.;
        break;
    default:
        printf("lvd_qdc_acard_init: unknown QDC type 0x%x\n", acard->mtype);
        return -1;
    }
#endif

    acard->clear_card=lvd_clear_qdc;
    acard->start_card=lvd_start_qdc;
    acard->stop_card=lvd_stop_qdc;
    if (info->qdcver<8)
        acard->cardstat=lvd_cardstat_qdc;
    else
        acard->cardstat=lvd_cardstat_qdc80;

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
