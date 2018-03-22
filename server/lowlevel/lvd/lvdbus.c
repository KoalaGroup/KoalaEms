/*
 * lowlevel/lvd/lvdbus.c
 * created 13.Dec.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvdbus.c,v 1.58 2017/10/24 17:00:09 wuestner Exp $";

#define LOWLIB
#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#ifndef HAVE_CGETCAP
#include <getcap.h>
#endif
#include <modultypes.h>
#include <xprintf.h>
#include <rcs_ids.h>
#include "lvd_map.h"
#include "lvdbus.h"
#include "lvd_access.h"
#include "../../main/nowstr.h"
#include "../../commu/commu.h"
#include "../../lowlevel/lvd/lvd_sync_statist.h"

#ifdef LVD_SIS1100
#   include "sis1100/sis1100_lvd.h"
#endif
#ifdef DMALLOC
#   include <dmalloc.h>
#endif

typedef int(*initfunctype)(struct lvd_dev*, struct lvd_acard*);
struct cardinitfunc {
    struct cardinitfunc* next;
    ems_u32 module_type;
    initfunctype init;
};

struct cardinitfunc *lvdinitfuncs=0;

RCS_REGISTER(cvsid, "lowlevel/lvd")

/*****************************************************************************/
#ifdef LVD_TRACE
void lvd_settrace(struct lvd_dev* dev, int *old, int trace)
{
    if (old)
        *old=dev->trace;
    if (trace!=-1)
        dev->trace=!!trace;
}
#endif
/*****************************************************************************/
static ems_u32
lvd_cardtype(ems_u32 ident)
{
    switch (LVD_HWtyp(ident)) {
    case LVD_CARDID_CONTROLLER_GPX   : return ZEL_LVD_CONTROLLER_GPX;
    case LVD_CARDID_CONTROLLER_F1    : return ZEL_LVD_CONTROLLER_F1;
    case LVD_CARDID_CONTROLLER_F1GPX : return ZEL_LVD_CONTROLLER_F1GPX;
    case LVD_CARDID_F1               : return ZEL_LVD_TDC_F1;
    case LVD_CARDID_GPX              : return ZEL_LVD_TDC_GPX;
    case LVD_CARDID_VERTEX           : return ZEL_LVD_ADC_VERTEX;
    case LVD_CARDID_VERTEXM3         : return ZEL_LVD_ADC_VERTEXM3;
    case LVD_CARDID_VERTEXUN         : return ZEL_LVD_ADC_VERTEXUN;
    case LVD_CARDID_FQDC             : return ZEL_LVD_FQDC;
    case LVD_CARDID_VFQDC            : return ZEL_LVD_VFQDC;
    case LVD_CARDID_VFQDC_200        : return ZEL_LVD_VFQDC;
    case LVD_CARDID_SQDC             : return ZEL_LVD_SQDC;
    case LVD_CARDID_SYNCH_MASTER     : return ZEL_LVD_MSYNCH;
    case LVD_CARDID_SYNCH_MASTER2    : return ZEL_LVD_MSYNCH;
    case LVD_CARDID_SYNCH_OUTPUT     : return ZEL_LVD_OSYNCH;
    case LVD_CARDID_TRIGGER          : return ZEL_LVD_TRIGGER;
    default:
        printf("lvd_cardtype: unknown type 0x%x\n", ident);
        return ZEL_LVD_UNKNOWN;
    }
}
/*****************************************************************************/
static const char*
cardident(ems_u32 ident, int width)
{
    static char s[128];
    const char* name;
    int HWtyp, HWver, FWver;
    HWtyp=LVD_HWtyp(ident);
    HWver=LVD_HWver(ident);
    FWver=LVD_FWver(ident);

    switch (HWtyp) {
    case LVD_CARDID_CONTROLLER_GPX:
        name="controller (GPX)";
        break;
    case LVD_CARDID_CONTROLLER_F1:
        name="controller (F1)";
        break;
    case LVD_CARDID_CONTROLLER_F1GPX:
        name="controller (F1GPX)";
        break;
    case LVD_CARDID_F1:
        name="F1 TDC";
        break;
    case LVD_CARDID_GPX:
        name="GPX TDC";
        break;
    case LVD_CARDID_VERTEX:
        name="VERTEX ADC";
        break;
    case LVD_CARDID_VERTEXM3:
        name="VERTEX_M3 ADC";
        break;
    case LVD_CARDID_VERTEXUN:
        name="VERTEX_UN ADC";
        break;
    case LVD_CARDID_SYNCH_MASTER:
        name="SYNCH MASTER";
        break;
    case LVD_CARDID_SYNCH_MASTER2:
        name="SYNCH MASTER_2";
        break;
    case LVD_CARDID_SYNCH_OUTPUT:
        name="SYNCH OUTPUT";
        break;
    case LVD_CARDID_FQDC:
        name="FAST QDC";
        break;
    case LVD_CARDID_VFQDC:
    case LVD_CARDID_VFQDC_200:
        name="VERY FAST QDC";
        break;
    case LVD_CARDID_SQDC:
        name="SLOW QDC";
        break;
    case LVD_CARDID_TRIGGER:
        name="TRIGGER";
        break;
    default:
        name="(unknown)";
    }
    snprintf(s, 128, "HWtype=0x%x, HWver=0x%x, FW=%1x.%1x %04x %*s",
            HWtyp, HWver, (FWver>>4)&0xf, FWver&0xf, ident, width, name);
    return s;
}
/*****************************************************************************/
void
lvdbus_register_cardtype(ems_u32 module_type, initfunctype init)
{
    struct cardinitfunc* help;

    help=malloc(sizeof(struct cardinitfunc));
    help->module_type=module_type;
    help->init=init;
    help->next=lvdinitfuncs;
    lvdinitfuncs=help;
}
/*****************************************************************************/
static initfunctype
find_cardinitfunc(ems_u32 module_type)
{
    struct cardinitfunc* help=lvdinitfuncs;
    while (help && help->module_type!=module_type) {
        help=help->next;
    }
    if (help)
        return help->init;
    else
        return 0;
}
/*****************************************************************************/
int
lvd_enumerate(struct lvd_dev* dev, ems_u32* outptr)
{
    int i;

    *outptr++=dev->num_acards;
    for (i=0; i<dev->num_acards; i++) {
        *outptr++=dev->acards[i].addr;
        *outptr++=dev->acards[i].mtype;
        *outptr++=dev->acards[i].id;
        *outptr++=dev->acards[i].serial;
    }
    return dev->num_acards*4+1;
}
/*****************************************************************************/
int
lvd_find_acard(struct lvd_dev* dev, int addr)
{
    int i;

    for (i=0; i<dev->num_acards; i++) {
        if (dev->acards[i].addr==(addr&0xf)) {
            return i;
        }
    }
    return -1;
}
/*****************************************************************************/
int
lvd_a_ident(struct lvd_dev* dev, int addr, ems_u32* ident, ems_u32* serial)
{
    int i;

    i=lvd_find_acard(dev, addr);
    if (i<0)
        return -1;

    *ident=dev->acards[i].id;
    *serial=dev->acards[i].serial;
    return 0;
}
/*****************************************************************************/
int
lvd_a_mtype(struct lvd_dev* dev, int addr, ems_u32* mtype)
{
    int i;

    i=lvd_find_acard(dev, addr);
    if (i<0)
        return -1;

    *mtype=dev->acards[i].mtype;
    return 0;
}
/*****************************************************************************/
int
lvd_c_ident(struct lvd_dev* dev, ems_u32* ident, ems_u32* serial)
{
    int res=0;
    res|=lvd_cc_r(dev, ident, ident);
    res|=lvd_cc_r(dev, serial, serial);
    return res;
}
/*****************************************************************************/
int
lvd_c_mtype(struct lvd_dev* dev, ems_u32* mtype)
{
    *mtype=dev->ccard.mtype;
    return 0;
}
/*****************************************************************************/
plerrcode
lvd_get_eventcount(struct lvd_dev* dev, ems_u32* evcount)
{
    if (lvd_cc_r(dev, event_nr, evcount)<0)
        return plErr_System;
    return plOK;
}
/*****************************************************************************/
plerrcode
lvd_getdaqmode(struct lvd_dev* dev, ems_u32* mode)
{
    *mode=dev->ccard.daqmode;
    return plOK;
}
/*****************************************************************************/
plerrcode
lvd_setdaqmode(struct lvd_dev* dev, int mode)
{
    dev->ccard.daqmode=mode;
    return plOK;
}
/*****************************************************************************/
plerrcode
lvdacq_getdaqmode(struct lvd_dev* dev, int addr, ems_u32* mode)
{
    if (addr<0) {
        int i;
        for (i=0; i<MAXACARDS; i++)
            mode[i]=0;
        for (i=0; i<dev->num_acards; i++) {
            struct lvd_acard *acard=dev->acards+i;
            mode[acard->addr]=acard->daqmode;
        }
    } else {
        int idx=lvd_find_acard(dev, addr);
        if (idx<0) {
            printf("lvdacq_getdaqmode: no card with addr 0x%x\n", addr);
            return plErr_BadHWAddr;
        }
        *mode=dev->acards[idx].daqmode;
    }

    return plOK;
}
/*****************************************************************************/
plerrcode
lvdacq_setdaqmode(struct lvd_dev* dev, int addr, ems_u32 mtype, int mode)
{
    int cards=0;

    if (addr<0) {
        int card;
        for (card=0; card<dev->num_acards; card++) {
            if ((mtype==0) || (dev->acards[card].mtype==mtype)) {
                dev->acards[card].daqmode=mode;
                cards++;
            }
        }
    } else {
        int card;
        card=lvd_find_acard(dev, addr);
        if (card<0) {
            printf("lvdacq_setdaqmode: no card with addr 0x%x\n", addr);
            return plErr_BadHWAddr;
        }
        dev->acards[card].daqmode=mode;
        cards++;
    }
    return cards?plOK:plErr_NotInModList;
}
/*****************************************************************************/
int
lvd_clear_status(struct lvd_dev* dev)
{
    int res=0, i;

    for (i=0; i<dev->num_acards; i++) {
        struct lvd_acard* acard=dev->acards+i;
        if (acard->initialized) {
            if (acard->clear_card)
                acard->clear_card(dev, acard);
        }
    }

    {
        ems_u32 sr;
        res|=lvd_cc_r(dev, sr, &sr);
#if 0
        printf("controllerstatusA=0x%04x\n", sr);
#endif
        res|=lvd_cc_w(dev, sr, sr);
        res|=lvd_cc_r(dev, sr, &sr);
#if 0
        printf("controllerstatusB=0x%04x\n", sr);
#endif
    }
    return res;
}
/*****************************************************************************/
plerrcode
lvd_start(struct lvd_dev* dev, int selftrigger)
{
    int res=0, i;
    plerrcode pres;

    printf("lvd_start, selftrigger=%d\n", selftrigger);

    lvd_sync_statist_clear(dev);

    if (lvd_clear_status(dev)) {
        return plErr_Other;
    }

    if (!selftrigger) {
        /*
         * if the trigger is connected directly to the controller
         * the controller has to be started from the trigger procedure
         */

        if ((pres=lvd_start_controller(dev))!=plOK)
            return pres;
    }

    /* start all acquisition cards but not synchronisation master */
    for (i=0; i<dev->num_acards; i++) {
        struct lvd_acard* acard=dev->acards+i;
        if (acard->mtype!=ZEL_LVD_MSYNCH && acard->initialized) {
            acard->start_card(dev, acard);
        }
    }

    /* start synchronisation master (if it exists) */
    for (i=0; i<dev->num_acards; i++) {
        struct lvd_acard* acard=dev->acards+i;
        if (acard->mtype==ZEL_LVD_MSYNCH && acard->initialized) {
            if (acard->start_card(dev, acard)<0)
                return plErr_Other;
        }
    }

    return res?plErr_Other:plOK;
}
/*****************************************************************************/
plerrcode
lvd_stop(struct lvd_dev* dev, __attribute__((unused)) int selftrigger)
{
    int res=0, i;
    plerrcode pres, presx;

    printf("lvd_stop\n");

    /* stop synchronisation master (if it exists) */
    for (i=0; i<dev->num_acards; i++) {
        struct lvd_acard* acard=dev->acards+i;
        if (acard->mtype==ZEL_LVD_MSYNCH && acard->initialized) {
            res|=acard->stop_card(dev, acard);
        }
    }

    res|=lvd_cc_w(dev, cr, 0);

    for (i=0; i<dev->num_acards; i++) {
        struct lvd_acard* acard=dev->acards+i;
        if (acard->mtype!=ZEL_LVD_MSYNCH && acard->initialized) {
            res|=acard->stop_card(dev, acard);
        }
    }

    pres=res?plErr_Other:plOK;

    if ((presx=lvd_stop_controller(dev))!=plOK) {
        if (pres==plOK) pres=presx;
    }

    return pres;
}
/*****************************************************************************/
int
lvd_statist_interval(struct lvd_dev* dev, int interval)
{
    int old=dev->statist_interval;
    if (interval>=0)
        dev->statist_interval=interval;
    return old;
}
/*****************************************************************************/
int
lvd_silent_regmanipulation(struct lvd_dev* dev, int silent)
{
    int old=dev->silent_regmanipulation;
    if (silent!=-1)
        dev->silent_regmanipulation=silent;
    return old;
}
/*****************************************************************************/
plerrcode
lvd_set_creg(struct lvd_dev* dev, int reg, int size, ems_u32 val)
{
    if (!dev->silent_regmanipulation)
        printf("set_creg   0x%x/%d <-- 0x%x\n", reg, size, val);
    return lvd_c_w_(dev, reg, size, val)?plErr_Other:plOK;
}
/*****************************************************************************/
#if 0
plerrcode
lvd_set_cregB(struct lvd_dev* dev, int reg, int size, ems_u32 val)
{
    if (!dev->silent_regmanipulation)
        printf("set_cregB  all/0x%x/%d <-- 0x%x\n", reg, size, val);
    return lvd_cb_w_(dev, reg, size, val)?plErr_Other:plOK;
}
#endif
/*****************************************************************************/
plerrcode
lvd_get_creg(struct lvd_dev* dev, int reg, int size, ems_u32* val)
{
    int res;

    res=lvd_c_r_(dev, reg, size, val);
    if (!dev->silent_regmanipulation) {
        printf("get_creg   0x%x/%d --> ", reg, size);
        if (!res)
            printf("0x%x\n", *val);
        else
            printf("Error\n");
    }
    return res?plErr_Other:plOK;
}
/*****************************************************************************/
#if 0
plerrcode
lvd_get_cregB(struct lvd_dev* dev, int reg, int size, ems_u32* val)
{
    int res;

    res=lvd_cb_r_(dev, reg, size, val);
    if (!dev->silent_regmanipulation) {
        printf("get_cregB  all/0x%x/%d --> ", reg, size);
        if (!res)
            printf("0x%x\n", *val);
        else
            printf("Error\n");
    }
    return res?plErr_Other:plOK;
}
#endif
/*****************************************************************************/
plerrcode
lvd_set_areg(struct lvd_dev* dev, int addr, int reg, int size, ems_u32 val)
{
    if (!dev->silent_regmanipulation)
        printf("set_areg   0x%x/0x%x/%d <-- 0x%x\n", addr, reg, size, val);
    return lvd_i_w_(dev, addr, reg, size, val)?plErr_Other:plOK;
}
/*****************************************************************************/
plerrcode
lvd_set_aregB(struct lvd_dev* dev, int reg, int size, ems_u32 val)
{
    if (!dev->silent_regmanipulation)
        printf("set_aregB  0x%x/%d <-- 0x%x\n", reg, size, val);
    return lvd_ib_w_(dev, reg, size, val)?plErr_Other:plOK;
}
/*****************************************************************************/
plerrcode
lvd_set_aregs(struct lvd_dev* dev, ems_u32 mtype, int reg, int size,
    ems_u32 val)
{
    int res=0, card;

    for (card=0; card<dev->num_acards; card++) {
        if (!mtype || (dev->acards[card].mtype==mtype))
            res|=lvd_i_w_(dev, dev->acards[card].addr, reg, size, val);
    }
    return res?plErr_Other:plOK;
}
/*****************************************************************************/
plerrcode
lvd_get_areg(struct lvd_dev* dev, int addr, int reg, int size, ems_u32* val)
{
    int res;

    res=lvd_i_r_(dev, addr, reg, size, val);
    if (!dev->silent_regmanipulation) {
        printf("get_areg   0x%x/0x%x/%d --> ", addr, reg, size);
        if (!res)
            printf("0x%x\n", *val);
        else
            printf("Error\n");
    }
    return res?plErr_Other:plOK;
}
/*****************************************************************************/
plerrcode
lvd_get_aregB(struct lvd_dev* dev, int reg, int size, ems_u32* val)
{
    int res;

    res=lvd_ib_r_(dev, reg, size, val);
    if (!dev->silent_regmanipulation) {
        printf("get_aregB  0x%x/%d --> ", reg, size);
        if (!res)
            printf("0x%x\n", *val);
        else
            printf("Error\n");
    }
    return res;
}
/*****************************************************************************/
int
lvd_modstat(struct lvd_dev* dev, unsigned int addr, void *xp, int level)
{
    int i, found=0;

    xprintf(xp ,"\n=== MODULESTAT %s for module 0x%x ===\n", nowstr(), addr);

    for (i=0; i<dev->num_acards; i++) {
        if (dev->acards[i].addr==(addr&0xf)) {
            found=1;
            break;
        }
    }
    if (found) {
        lvd_cardstat_acq(dev, addr, xp, level, 1);
    } else {
        xprintf(xp, "module 0x%x does not exist\n", addr);
    }
    printf("=== END OF MODULESTAT ===\n\n");

    return 0;
}
/*****************************************************************************/
int
lvd_contrstat(struct lvd_dev* dev, void *xp, int level)
{
    xprintf(xp, "\n=== CONTROLLERSTAT %s ===\n", nowstr());
    lvd_controllerstat(dev, xp, level);

#ifdef LVD_SIS1100
    if (dev->lvdtype==lvd_sis1100) {
        struct lvd_sis1100_info* info=(struct lvd_sis1100_info*)dev->info;
        info->mc_stat(dev, 0, xp, level);
        info->sis_stat(dev, 0, xp, level);
        info->plx_stat(dev, 0, xp, level);
    }
#endif

    xprintf(xp, "=== END OF CONTROLLERSTAT ===\n\n");

    return 0;
}
/*****************************************************************************/
/*
 * modmask==-1: all modules
 * modmask==0: all initialized modules
 * modmask>0: all modules in mask
 */
int
lvd_cratestat(struct lvd_dev* dev, void *xp, int level, int modmask)
{
    int j, all=modmask==-1;

    xprintf(xp, "\n=== CRATESTAT %s ===\n", nowstr());
    lvd_controllerstat(dev, xp, level);
    if (modmask)
        modmask&=dev->a_pat; /* all modules if modulemask==-1 */
    else
        modmask=dev->a_pat;
    for (j=0; j<MAXACARDS; j++) {
        if (modmask&(1<<j)) {
            lvd_cardstat_acq(dev, j, xp, level, all);
        }
    }
#ifdef LVD_SIS1100
    if (dev->lvdtype==lvd_sis1100) {
        struct lvd_sis1100_info* info=(struct lvd_sis1100_info*)dev->info;
        info->mc_stat(dev, 0, xp, level);
        info->sis_stat(dev, 0, xp, level);
    }
#endif
    xprintf(xp, "=== END OF CRATESTAT ===\n\n");

    return 0;
}
/*****************************************************************************/
static void
print_acard(struct lvd_acard* acard)
{
    printf("[%x]: %s", acard->addr, cardident(acard->id, -14));
    if (acard->serial!=-1)
        printf(" serial no. %d", acard->serial);
    printf("\n");
}
/*****************************************************************************/
/*
 * initializes dev->acards[...] using the ident already written
 */
static int
init_dev_acard(struct lvd_dev* dev, struct lvd_acard* acard)
{
    initfunctype initfunc;

    acard->mtype=lvd_cardtype(acard->id);

    acard->initialized=0;
    if (acard->freeinfo) /*this is save because dev is allocated using calloc*/
        acard->freeinfo(dev, acard);
    acard->cardinfo=0;
    acard->freeinfo=0;
    acard->clear_card=0;
    acard->start_card=0;
    acard->stop_card=0;

    initfunc=find_cardinitfunc(acard->mtype);
    if (!initfunc) {
        printf("lvd init_dev_acard: no initfunc found for %s\n",
            cardident(acard->id, 0));
        return 0;
    }

    if (initfunc(dev, acard)<0) {
        printf("lvd init_dev_acard: initfunc %s addr 0x%x failed\n",
            cardident(acard->id, 0), acard->addr);
        return -1;
    }

    /* sanity checks */
    if (acard->cardinfo && !acard->freeinfo) {
        printf("init_dev_acard: freeinfo not set for addr 0x%x\n",
            acard->addr);
        return -1;
    }
    if (!acard->clear_card) {
        printf("init_dev_acard: clear_card not set for addr 0x%x\n",
            acard->addr);
        return -1;
    }
    if (!acard->start_card) {
        printf("init_dev_acard: start_card not set for addr 0x%x\n",
            acard->addr);
        return -1;
    }
    if (!acard->stop_card) {
        printf("init_dev_acard: stop_card not set for addr 0x%x\n",
            acard->addr);
        return -1;
    }

    return 0;
}
/*****************************************************************************/
plerrcode
lvd_force_module_id(struct lvd_dev* dev, ems_u32 addr, ems_u32 id,
        ems_u32 mask, int serial)
{
    struct lvd_acard* acard;
    int idx=lvd_find_acard(dev, addr);
    ems_u32 new_id;

printf("lvd_force_module_id: dev=%p addr=%d id=%04x mask=%04x ser=%d\n",
        dev, addr, id, mask, serial);
    if (idx<0)
        return plErr_BadHWAddr;
    acard=dev->acards+idx;

    if (acard->freeinfo)
        acard->freeinfo(dev, acard);

    new_id=((id&mask)|(dev->acards[idx].id&~mask))&0xffff;
    dev->acards[idx].id=new_id;
    if (serial>=0)
        dev->acards[idx].serial=serial;
    init_dev_acard(dev, dev->acards+idx);
    return plOK;
}
/*****************************************************************************/
static plerrcode
init_addresses(struct lvd_dev* dev, unsigned int num_ids, ems_u32* ids)
{
    ems_u32 offline, online;
    unsigned int addr, idx=0;
    int verbose=0;

    if (num_ids && ids[0]==(ems_u32)-1) {
        verbose=1;
        num_ids--;
        ids++;
    }

    if (verbose) {
        unsigned int i;
        printf("init_addresses: num_ids=%d%s", num_ids, num_ids?" :":"");
        for (i=0; i<num_ids; i++)
            printf(" %d", ids[i]);
        printf("\n");
    }

    goto start;
    while (offline && idx<16) {
        /* choose a new address */
        if (lvd_ib_r(dev, card_onl, &online)<0)
            return plErr_HW;
        do {
            if (idx<num_ids)
                addr=ids[idx];
            else
                addr=num_ids?ids[num_ids-1]+idx-num_ids:idx;
            addr&=0xf;
            idx++;
        } while (online&(1<<addr));

        if (verbose)
            printf("online=%04x idx=%d addr=%d\n", online, idx, addr);
        /* write the new address */
        if (lvd_ib_w(dev, card_onl, addr)<0)
            return plErr_HW;

start:  /* check whether there is still a card without address*/
        if (lvd_ib_r(dev, card_offl, &offline)<0)
            return plErr_HW;
    };

    if (offline) {
        complain("lvd_init_addresses: some modules still offline");
        return plErr_HW;
    }

    return plOK;
}
/*****************************************************************************/
static void
print_offline(struct lvd_dev* dev, char *text)
{
    ems_u32 offline, online;
    lvd_ib_r(dev, card_offl, &offline);
    lvd_ib_r(dev, card_onl, &online);
    printf("online=%04x offline=%04x (%s)\n", online, offline, text);
}
/*****************************************************************************/
/*
 * lvdbus_init is called whenever the data
 * 
 * lvdbus_init will give all modules an address (if necessary).
 * Module addresses are assigned in ascending order beginning with 0 unless
 * a different order is given with num_ids and ids.
 * 
 * lvdbus_init will fill the data structures of dev with values
 * corresponding to the hardware.
 * Data will remain untouched if they match the moduletype.
 *
 * code will be:
 *    -1: error
 *     0: was offline
 *     1: still initialised
 *     2: different crate or configuration
 */
plerrcode
lvdbus_init(struct lvd_dev* dev, int num_ids, ems_u32* ids, int *code)
{
    ems_u32 offline, mask;
    plerrcode pres=plErr_HW;
    unsigned int addr;
    int i, dummy;

    printf("== lowlevel/lvd/lvdbus.c:lvdbus_init ==\n");
    print_offline(dev, "lvdbus_init A");

    if (!code) /* allow code to be a null pointer */
        code=&dummy;

    if (num_ids) {
        int i;
        printf("  explicit list of adresses:");
        for (i=0; i<num_ids; i++)
            printf(" %d", ids[i]);
        printf("\n");
    }

    *code=1;

    /* read ident of the controller card */
    lvd_cc_r(dev, ident, &dev->ccard.id);
    printf("controller id: %s\n", cardident(dev->ccard.id, 0));
    dev->ccard.mtype=lvd_cardtype(dev->ccard.id);
    dev->silent_errors=1;
    if (lvd_cc_r(dev, serial, &dev->ccard.serial)<0) {
        dev->ccard.serial=-1;
        printf("Controller has no serial number\n");
    } else {
        printf("Serial number: %d\n", dev->ccard.serial);
    }
    dev->silent_errors=0;

    /* check whether we need to give cards an address */
    if (lvd_ib_r(dev, card_offl, &offline)<0)
        goto error;
    if (offline) {
        *code=0;
        pres=init_addresses(dev, num_ids, ids);
        if (pres!=plOK)
            goto error;
        pres=plErr_HW; /* prepare for next error :-) */
    }

    /* collect information about all existing cards */
    print_offline(dev, "lvdbus_init B");

    if (lvd_ib_r(dev, card_onl, &dev->a_pat)<0)
        goto error;
    dev->num_acards=0;
    mask=dev->a_pat;
    addr=0;
    for (i=0; i<MAXACARDS; i++) {
        ems_u32 id, serial;
        if (!mask) { /* all cards already checked */
            memset(dev->acards+i, 0, sizeof(dev->acards[i]));
            continue;
        }
        dev->num_acards++;
        while (!(mask&1)) { /* find the next address */
            addr++;
            mask>>=1;
        }
        lvd_i_r(dev, addr, all.ident, &id);
        lvd_i_r(dev, addr, all.serial, &serial);
        if (dev->acards[i].addr!=addr || id!=dev->acards[i].orig_id) {
            if (*code!=0)
                *code=2;
            memset(dev->acards+i, 0, sizeof(dev->acards[i]));
            dev->acards[i].dev=dev;
            dev->acards[i].addr=addr;
            dev->acards[i].id=id;
            dev->acards[i].orig_id=id;
            dev->acards[i].serial=serial;
            init_dev_acard(dev, dev->acards+i);
        } else {
            if (dev->acards[i].id==dev->acards[i].orig_id)
                dev->acards[i].serial=serial;
        }
        print_acard(dev->acards+i);
        addr++;
        mask>>=1;
    }

    dev->parsecaps=0;
    dev->parseflags=0;
    lvd_sync_statist_autosetup(dev);
    dev->parsecaps |= lvd_parse_head; /* always possible */
    dev->parseflags=dev->parsecaps&lvd_parse_sync;

    return plOK;

error:
    *code=-1;
    return pres;
}
/*****************************************************************************/
/*
 * lvdbus_reset is called from proc/lvd/lvd_init.c:lvdbus_init and lvdbus_done
 * It resets the hardware and clears some data structures of dev.
 * 
 * After lvdbus_reset it is necessary to call lvdbus_init.
 */
plerrcode
lvdbus_reset(struct lvd_dev* dev)
{
    ems_u32 online;
    plerrcode pres=plErr_HW;
    int i;

    printf("== lowlevel/lvd/lvdbus.c:lvdbus_reset ==\n");
    dev->silent_errors=1;
    /* stop controller */
    lvd_cc_w(dev, cr, 0);
    /* reset all cards */
    lvd_ib_w(dev, ctrl, LVD_MRESET);

    /* clear some data structures */
    dev->ccard.daqmode=0;
    dev->ccard.contr_type=contr_none;
    dev->contr_mode=contr_none;
    for (i=0; i<16; i++)
        dev->ccard.f1_shadow[i]=0;
    for (i=0; i<MAXACARDS; i++) {
        if (dev->acards[i].freeinfo)
            dev->acards[i].freeinfo(dev, dev->acards+i);
        memset(dev->acards+i, 0, sizeof(dev->acards[i]));
    }
    dev->num_acards=0;
    dev->a_pat=0;
    dev->parseflags=dev->parsecaps=0;
    dev->sync_statist=0;

    /* check whether reset was successfull */
    if (lvd_ib_r(dev, card_onl, &online)<0) {
        printf("lvd_reset: error reading online status\n");
        goto error;
    } else {
        if (online) {
            printf("lvd_reset: input cards still online after reset: 0x%04x\n",
                online);
            goto  error;
        }
    }

    pres=plOK;
error:
    dev->silent_errors=0;
    return pres;
}
/*****************************************************************************/
plerrcode
lvdbus_done(struct lvd_dev* dev)
{
    printf("== lowlevel/lvd/lvdbus.c:lvdbus_done ==\n");

    lvdbus_reset(dev); /* errors are ignored */

    if (dev->sync_statist) {
        free(dev->sync_statist);
        dev->sync_statist=0;
    }
    while (lvdinitfuncs) {
        struct cardinitfunc* help;
        help=lvdinitfuncs;
        lvdinitfuncs=lvdinitfuncs->next;
        free(help);
    };

    return plOK;
}
/*****************************************************************************/
/*****************************************************************************/
