/*
 * lowlevel/lvd/tdc/f1_reg.c
 * created 25.Mar.2005 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: f1_reg.c,v 1.8 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include "../../../commu/commu.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <modultypes.h>
#include <rcs_ids.h>
#include "f1.h"
#include "../lvd_access.h"

RCS_REGISTER(cvsid, "lowlevel/lvd/tdc")

/*****************************************************************************/
#ifndef LVD_TRACE
static int
f1_reg_wait(struct lvd_dev* dev, int addr)
{
    ems_u32 val;
    int res=0;

    do {
        res|=lvd_i_r(dev, addr, f1.f1_state[0], &val);
    } while (!res && (val&F1_WRBUSY));

    return res;
}
#else
static int
f1_reg_wait(struct lvd_dev* dev, int addr)
{
    DEFINE_LVDTRACE(trace);
    ems_u32 val;
    int weiter, res=0;
    int n=0;

    lvd_settrace(dev, &trace, -1);
    do {
        if (n)
            lvd_settrace(dev, 0, 0);
        res|=lvd_i_r(dev, addr, f1.f1_state[0], &val);
        weiter=val&F1_WRBUSY;
        n++;
    } while (!res && weiter);
    if (LVDTRACE_ACTIVE(trace))
        printf("f1.c: f1_reg_wait: %d\n", n);
    lvd_settrace(dev, 0, trace);
    return res;
}
#endif
/*****************************************************************************/
static int
set_f1_addr(struct lvd_dev* dev, struct lvd_acard* acard, unsigned int f1)
{
    struct f1_info* info=(struct f1_info*)acard->cardinfo;
    if (info->last_f1!=(int)f1) {
        if (lvd_i_w(dev, acard->addr, f1.f1_addr, f1)<0)
            return -1;
        info->last_f1=f1;
    }
    return 0;
}
/*****************************************************************************/
int
f1_reg_w(struct lvd_dev* dev, struct lvd_acard* acard, unsigned int f1,
    int unsigned reg, int val)
{
    struct f1_info* info=(struct f1_info*)acard->cardinfo;
    int reg_ofs;

    if (acard->mtype!=ZEL_LVD_TDC_F1) {
        printf("f1_reg_w: card with addr 0x%x is not a F1 card\n", acard->addr);
        return -1;
    }

    if ((f1>8)||(reg>15)) {
        printf("lowlevel/lvd/f1/f1_reg.c: "
                "f1_reg_w(addr=%d f1=%d reg=%d val=0x%x): "
                "wrong arguments\n",
                acard->addr, f1, reg, val);
        return -1;
    }

    if (set_f1_addr(dev, acard, f1)<0)
        return -1;

    reg_ofs=ofs(union lvd_in_card, f1.f1_regX[0])+2*reg;
    if (lvd_i_w_(dev, acard->addr, reg_ofs, 2, val)<0)
        return -1;
    if (f1_reg_wait(dev, acard->addr))
        return -1;

    if (f1==8) {
        int i;
        for (i=0; i<8; i++)
            info->shadow[i][reg]=val;
    } else {
        info->shadow[f1][reg]=val;
    }
    return 0;
}
/*****************************************************************************/
int
f1_reg_r(__attribute__((unused)) struct lvd_dev* dev, struct lvd_acard* acard, unsigned int f1,
    int unsigned reg, int* val)
{
    struct f1_info* info=(struct f1_info*)acard->cardinfo;
    if (acard->mtype!=ZEL_LVD_TDC_F1) {
        printf("f1_reg_w: card with addr 0x%x is not a F1 card\n", acard->addr);
        return -1;
    }

    if ((f1>7)||(reg>15)) {
        printf("lowlevel/lvd/f1/f1_reg.c: "
                "f1_reg_r(addr=%d f1=%d reg=%d): wrong arguments\n",
                acard->addr, f1, reg);
        return -1;
    }

    *val=info->shadow[f1][reg];

    return 0;
}
/*****************************************************************************/
int
f1_reset_chips(struct lvd_dev* dev, struct lvd_acard* acard)
{
    struct f1_info* info=(struct f1_info*)acard->cardinfo;
    int i, j;
    /*printf("f1_reset_chips card 0x%x F1\n", acard->addr);*/
    if (lvd_i_w(dev, acard->addr, f1.ctrl, LVD_PURES)<0)
        return -1;

    for (i=0; i<8; i++) {
        for (j=0; j<16; j++) {
            info->shadow[i][j]=0;
        }
    }
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
