/******************************************************************************
*                                                                             *
* gsitrigmod.c for camac                                                      *
*                                                                             *
* OS9/UNIX                                                                    *
*                                                                             *
* created 05.12.94                                                            *
* last changed 06.12.94                                                       *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: gsitrigmod.c,v 1.5 2011/04/06 20:30:35 wuestner Exp $";

#if defined(__unix__)
#include <sys/time.h>
#endif

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../../objects/domain/dom_ml.h"

#include "../../../lowlevel/camac/camac.h"
#include "gsitrigger.h"
#include "gsitrigmod.h"

extern int* memberlist;
static struct camac_dev* dev;
static camadr_t stat_r, stat_w;
static camadr_t ctrl_w, fca_w, cvt_w;
static int nummer;

RCS_REGISTER(cvsid, "trigger/camac/gsi")

/*****************************************************************************/
plerrcode trigmod_init(int slot, int master, int ctime, int fcatime)
{
    ml_entry* m;

    T(trigmod_init)
    if ((unsigned int)slot>30)
        return plErr_BadHWAddr;
    if (!valid_module(slot, modul_camac, 0))
        return plErr_BadModTyp;
    m=ModulEnt(slot);
    dev=m->address.camac.dev;
    if (m->modultype!=GSI_TRIGGER)
        return plErr_BadModTyp;

#ifdef NANO
    p[3]/=100;
    p[4]/=100;
#endif

/* conv. time > fast clear window ? */
    if (!ctime || !fcatime || ((ctime | fcatime)&0xffff0000) || (ctime<=fcatime))
        return(plErr_ArgRange);

    stat_w=dev->CAMACaddr(slot, STATUS, 16);
    stat_r=dev->CAMACaddr(slot, STATUS,  0);
    ctrl_w=dev->CAMACaddr(slot, CONTROL,16);
    fca_w=dev->CAMACaddr(slot,  FCATIME,16);
    cvt_w=dev->CAMACaddr(slot,  CTIME,  16);

    dev->CAMACwrite(dev, &ctrl_w, CLEAR);
    dev->CAMACwrite(dev, &ctrl_w, (master?MASTER:SLAVE));
    dev->CAMACwrite(dev, &ctrl_w, BUS_ENABLE);
    dev->CAMACwrite(dev, &cvt_w, 0x10000-ctime);
    dev->CAMACwrite(dev, &fca_w, 0x10000-fcatime);

    dev->CAMACwrite(dev, &ctrl_w,CLEAR);
    dev->CAMACwrite(dev, &ctrl_w,GO);

    return(plOK);
}
/*****************************************************************************/
plerrcode trigmod_done()
{
    T(trigmod_done)
    dev->CAMACwrite(dev, &ctrl_w, HALT);
    dev->CAMACwrite(dev, &ctrl_w, BUS_DISABLE);
    return(plOK);
}
/*****************************************************************************/
int trigmod_get()
{
    ems_u32 status, trigger;

    T(trigmod_get)

    if (dev->CAMACread(dev, &stat_r, &status)<0)
        return -1;
    if ((short)status>=0) /*EON nicht gesetzt*/
        return(0);

    if ((status & EON)==0) {
        printf("Trigger: EON==0\n");
        return(0);
    }

    trigger=status & 0xf;
    if (trigger==0) {
        printf("Trigger: trigger==0\n");
        return(0);
    }

    if (status & (REJECT | MISMATCH)) {
        if (status & REJECT) {
            printf("Trigger: REJECT\n");
        }
        if (status & MISMATCH) {
            printf("Trigger: MISMATCH\n");
        }
        return(0);
    }
    return(trigger);
}
/*****************************************************************************/
void trigmod_reset()
{
    T(trigmod_reset)
    dev->CAMACwrite(dev, &stat_w,EV_IRQ_CLEAR);
    dev->CAMACwrite(dev, &stat_w,DT_CLEAR);
}
/*****************************************************************************/
plerrcode trigmod_soft_init(int slot, int id, int ctime, int fcatime)
{
    ml_entry* m;

    T(trigmod_soft_init)
    if ((unsigned int)slot>30)
        return(plErr_BadHWAddr);

    if (!valid_module(slot, modul_camac, 0))
        return plErr_BadModTyp;
    m=ModulEnt(slot);
    dev=m->address.camac.dev;
    if (m->modultype!=GSI_TRIGGER)
        return plErr_BadModTyp;

    if ((id<1) || (id>15)) return(plErr_ArgRange);
    nummer=id;
#ifdef NANO
    p[3]/=100;
    p[4]/=100;
#endif

/* conv. time > fast clear window ? */
    if (!ctime || !fcatime || ((ctime | fcatime)&0xffff0000) || (ctime<=fcatime))
        return(plErr_ArgRange);

    stat_w=dev->CAMACaddr(slot, STATUS, 16);
    stat_r=dev->CAMACaddr(slot, STATUS,  0);
    ctrl_w=dev->CAMACaddr(slot, CONTROL,16);
    fca_w=dev->CAMACaddr(slot,  FCATIME,16);
    cvt_w=dev->CAMACaddr(slot,  CTIME,  16);

    dev->CAMACwrite(dev, &ctrl_w, CLEAR);
    dev->CAMACwrite(dev, &ctrl_w, MASTER);
    dev->CAMACwrite(dev, &ctrl_w, BUS_ENABLE);
    dev->CAMACwrite(dev, &cvt_w, 0x10000-ctime);
    dev->CAMACwrite(dev, &fca_w, 0x10000-fcatime);

    dev->CAMACwrite(dev, &ctrl_w,CLEAR);
    dev->CAMACwrite(dev, &ctrl_w,GO);

    return(plOK);
}
/*****************************************************************************/
plerrcode trigmod_soft_done()
{
    T(trigmod_soft_done)
    dev->CAMACwrite(dev, &ctrl_w, HALT);
    dev->CAMACwrite(dev, &ctrl_w, BUS_DISABLE);
    return(plOK);
}
/*****************************************************************************/
int trigmod_soft_get()
{
    ems_u32 status, trigger;

    T(trigmod_soft_get)

    dev->CAMACwrite(dev, &stat_w, nummer);
#ifdef OSK
    tsleep(2);
#endif

    if (dev->CAMACread(dev, &stat_r, &status)<0)
        return plErr_System;
    if ((short)status>=0) /*EON nicht gesetzt*/
        return(0);

    if ((status & EON)==0) {
        printf("Trigger: EON==0\n");
        return(0);
    }

    trigger=status & 0xf;
    if (trigger==0) {
        printf("Trigger: trigger==0\n");
        return(0);
    }

    if (status & (REJECT | MISMATCH)) {
        if (status & REJECT) {
            printf("Trigger: REJECT\n");
        }
        if (status & MISMATCH) {
            printf("Trigger: MISMATCH\n");
        }
        return(0);
    }
    return(trigger);
}
/*****************************************************************************/
void trigmod_soft_reset()
{
    dev->CAMACwrite(dev, &stat_w, EV_IRQ_CLEAR);
    dev->CAMACwrite(dev, &stat_w, DT_CLEAR);
}
/*****************************************************************************/
/*****************************************************************************/
