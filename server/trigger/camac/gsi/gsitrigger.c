/*
 * trigger/camac/gsi/gsitrigger.c
 * created: 24.08.94 or before
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: gsitrigger.c,v 1.22 2011/04/06 20:30:35 wuestner Exp $";

#if defined(__unix__)
#include <sys/time.h>
#endif

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <unsoltypes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../../main/scheduler.h"
#include "../../../objects/pi/readout.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/camac/camac.h"
#include "../../../commu/commu.h"
#include "../../trigprocs.h"
#include "gsitrigger.h"

#ifdef OBJ_VAR
#include "../../../objects/var/variables.h"
#endif

extern ems_u32 eventcnt;
extern int* memberlist;

#ifdef OPTIMIERT
VOLATILE int *stat_r, *stat_w;
#else
camadr_t stat_r, stat_w;
#endif

static struct camac_dev* dev;
static camadr_t ctrl_w, fca_w, cvt_w;
static int nummer, hund;
ems_u32* delayptr;

RCS_REGISTER(cvsid, "trigger/camac/gsi")

/*****************************************************************************/
/*
GSI_Trig
p[1]: Slotnummer
p[2] : Mastermodule (boolean)
p[3] : conversion time
p[4] : fast clear acceptance window
*/
#ifdef SCHED_TRIGGER
plerrcode init_trig_gsi(ems_u32* p, struct trigprocinfo* info)
#else
plerrcode init_trig_gsi(ems_u32* p)
#endif
{
    ml_entry* m;

    T(init_trig_gsi)
    if (p[0]!=4)
        return plErr_ArgNum;
    if (p[1]>30)
        return plErr_BadHWAddr;

    if (!valid_module(p[1], modul_camac, 0))
        return plErr_BadModTyp;
    m=ModulEnt(p[1]);
    dev=m->address.camac.dev;
    if (m->modultype!=GSI_TRIGGER)
        return plErr_BadModTyp;

#ifdef NANO
    p[3]/=100;
    p[4]/=100;
#endif

/* conv. time > fast clear window ? */
    if ((!p[3])||(!p[4])||((p[3]|p[4])&0xffff0000)||(p[3]<=p[4]))
        return(plErr_ArgRange);

    stat_w=dev->CAMACaddr(p[1], STATUS, 16);
    stat_r=dev->CAMACaddr(p[1], STATUS,  0);
    ctrl_w=dev->CAMACaddr(p[1], CONTROL,16);
    fca_w=dev->CAMACaddr(p[1],  FCATIME,16);
    cvt_w=dev->CAMACaddr(p[1],  CTIME,  16);

    dev->CAMACwrite(dev, &ctrl_w, CLEAR);
    dev->CAMACwrite(dev, &ctrl_w, (p[2]?MASTER:SLAVE));
    dev->CAMACwrite(dev, &ctrl_w, BUS_ENABLE);
    dev->CAMACwrite(dev, &cvt_w, 0x10000-p[3]);
    dev->CAMACwrite(dev, &fca_w, 0x10000-p[4]);

#ifdef NANO
p[3]*=100;
p[4]*=100;
#endif

    dev->CAMACwrite(dev, &ctrl_w,CLEAR);

    dev->CAMACwrite(dev, &ctrl_w,GO);

    eventcnt=0;
#ifdef SCHED_TRIGGER
    info->proctype=tproc_poll;
#endif
    return plOK;
}
/*****************************************************************************/
plerrcode done_trig_gsi()
{
    T(done_trig_gsi)
    dev->CAMACwrite(dev, &ctrl_w, HALT);
    dev->CAMACwrite(dev, &ctrl_w, BUS_DISABLE);
    return plOK;
}
/*****************************************************************************/
#ifndef OPTIMIERT
int get_trig_gsi()
{
    ems_u32 status, event, trigger;

    if (dev->CAMACread(dev, &stat_r, &status)<0)
        return plErr_System;
    if ((ems_i16)status>=0) /*EON nicht gesetzt*/
        return(0);

    event=(status & 0x00001f00) >> 8;
    trigger=status&0xf;
    if ((status&EON)==0) {
        printf("Trigger: erwarteter Event: 0x%08x; EON==0\n", eventcnt);
        return(0);
    }
    if (trigger==0) {
        printf("Trigger: erwarteter Event: 0x%08x; trigger==0\n", eventcnt);
        fatal_readout_error();
        return(0);
    }

    if ((status & (REJECT | MISMATCH)) || (event != (eventcnt&0x1f))) {
        ems_u32 buf[3];

        if (status & REJECT) {
            printf("Trigger: REJECT bei eventcnt %x\n", eventcnt);
        }
        if (status & MISMATCH) {
            printf("Trigger: MISMATCH bei eventcnt %x\n", eventcnt);
        }
        if (event!=(eventcnt&0x1f)) {
            printf(
                "Trigger: erwarteter Event: 0x%08x (...%x); gefundener Event: 0x%x\n",
                eventcnt, eventcnt & 0x1f, event);
        }
        buf[0]=rtErr_Trig;
        buf[1]=eventcnt;
        buf[2]=status;
        send_unsolicited(Unsol_RuntimeError, buf, 3);
        fatal_readout_error();
        return(0);
    }
    eventcnt++;
    return(trigger);
}
#endif
/*****************************************************************************/
#ifndef OPTIMIERT
void reset_trig_gsi()
{
    dev->CAMACwrite(dev, &stat_w, EV_IRQ_CLEAR);
    dev->CAMACwrite(dev, &stat_w, DT_CLEAR);
}
#endif
/*****************************************************************************/

#ifdef OPTIMIERT
#ifdef DRIVERCAMAC
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx asm-code geht nur gemappt
#endif
#endif

/*****************************************************************************/

char name_trig_gsi[]="GSI_Trig";
int ver_trig_gsi=1;

/*****************************************************************************/
#ifdef OSK
static int gethund(int start)
{
    int date, time, tick;
    short day;
    int help;

    if (_sysdate(3, &time, &date, &day, &tick)==-1) {
        printf("_sysdate schlug fehl\n");
    }
    help=(date*86400+time)*100+tick;
    /* date*86400 gibt Ueberlauf, macht aber nichts */
    return(help-start);
}
#else
static int gethund(int start)
{
    struct timeval tv;
    struct timezone tz;
    int hund;
    gettimeofday(&tv,&tz);
    hund=tv.tv_sec*100+tv.tv_usec/10000;
    return(hund-start);
}
#endif
/*****************************************************************************/
/*
GSI_soft_Trig
p[1] : Slotnummer
p[2] : nummer
p[3] : conversion time
p[4] : fast clear acceptance window
[p[5]: delayvariable ]
*/
#ifdef SCHED_TRIGGER
plerrcode init_trig_gsi_soft(ems_u32* p, struct trigprocinfo* info)
#else
plerrcode init_trig_gsi_soft(ems_u32* p)
#endif
{
    ml_entry* m;

    T(init_trig_gsi_soft)
    if ((p[0]!=4) && (p[0]!=5))
        return plErr_ArgNum;

    delayptr=0;

#ifndef OBJ_VAR
    if (p[0]==5)
        return(plErr_IllVar);
#else
    if (p[0]==5) {
        plerrcode res;
        unsigned int size;
        if ((res=var_attrib(p[5], &size))!=plOK)
            return(res);
        if (size!=1)
            return(plErr_IllVarSize);
        delayptr=&(var_list[p[5]].var.val);
    }
#endif

    if (p[1]>30)
        return(plErr_BadHWAddr);

    if (!valid_module(p[1], modul_camac, 0))
        return plErr_BadModTyp;
    m=ModulEnt(p[1]);
    dev=m->address.camac.dev;
    if (m->modultype!=GSI_TRIGGER)
        return plErr_BadModTyp;

    if ((p[2]<1) || (p[2]>15))
        return(plErr_ArgRange);
    nummer=p[2];

#ifdef NANO
    p[3]/=100;
    p[4]/=100;
#endif

/* conv. time > fast clear window ? */
    if ((!p[3])||(!p[4])||((p[3]|p[4])&0xffff0000)||(p[3]<=p[4]))
        return(plErr_ArgRange);

    stat_w=dev->CAMACaddr(p[1], STATUS, 16);
    stat_r=dev->CAMACaddr(p[1], STATUS,  0);
    ctrl_w=dev->CAMACaddr(p[1], CONTROL,16);
    fca_w=dev->CAMACaddr(p[1],  FCATIME,16);
    cvt_w=dev->CAMACaddr(p[1],  CTIME,  16);

    dev->CAMACwrite(dev, &ctrl_w, CLEAR);
    dev->CAMACwrite(dev, &ctrl_w, (p[2]?MASTER:SLAVE));
    dev->CAMACwrite(dev, &ctrl_w, BUS_ENABLE);
    dev->CAMACwrite(dev, &cvt_w, 0x10000-p[3]);
    dev->CAMACwrite(dev, &fca_w, 0x10000-p[4]);

#ifdef NANO
    p[3]*=100;
    p[4]*=100;
#endif

    dev->CAMACwrite(dev, &ctrl_w, CLEAR);

    dev->CAMACwrite(dev, &ctrl_w, GO);

    eventcnt=0;

    hund=gethund(0);
#ifdef SCHED_TRIGGER
    info->proctype=tproc_poll;
#endif
    return plOK;
}
/*****************************************************************************/
plerrcode done_trig_gsi_soft()
{
    dev->CAMACwrite(dev, &ctrl_w,HALT);
    dev->CAMACwrite(dev, &ctrl_w,BUS_DISABLE);
    return(plOK);
}
/*****************************************************************************/
int get_trig_gsi_soft()
{
    ems_u32 status, event;

    if (gethund(hund)<(delayptr?*delayptr:20))
        return(0);

    dev->CAMACwrite(dev, &stat_w, nummer);
#ifdef OSK
    tsleep(2);
#endif

    if (dev->CAMACread(dev, &stat_r, &status)<0)
        return plErr_System;

    if ((ems_i16)status>=0) /*EON nicht gesetzt*/
        return(0);
    event=(status & 0x00001f00) >> 8;
    if ((status&(REJECT|MISMATCH))||(event != (eventcnt&0x1f))) {
        ems_u32 buf[3];
        int oldcnt;

        oldcnt=eventcnt;
        if (status & REJECT) {
            printf("Trigger: REJECT bei eventcnt %x\n", eventcnt);
        }
        if (status & MISMATCH) {
            printf("Trigger: MISMATCH bei eventcnt %x\n", eventcnt);
        }
        if (event!=(eventcnt&0x1f)) {
            printf(
                "Trigger: erwarteter Event: 0x%08x (...%x); gefundener Event: 0x%x\n",
                eventcnt, eventcnt & 0x1f, event);
            while (event!=(eventcnt&0x1f))
                eventcnt++;
            printf("eventcnt auf 0x%x (+%d) korrigiert\n",
                    eventcnt, eventcnt-oldcnt);
        }
        buf[0]=rtErr_Trig;
        buf[1]=oldcnt;
        buf[2]=status;
        send_unsolicited(Unsol_RuntimeError, buf, 3);
/*      return(0);*/
    }
    eventcnt++;
    hund=gethund(0);
    return(status&0xf);
}
/*****************************************************************************/
void reset_trig_gsi_soft()
{
    dev->CAMACwrite(dev, &stat_w,EV_IRQ_CLEAR);
    dev->CAMACwrite(dev, &stat_w,DT_CLEAR);
}
/*****************************************************************************/
char name_trig_gsi_soft[]="GSI_soft_Trig";
int ver_trig_gsi_soft=1;
/*****************************************************************************/
/*****************************************************************************/
