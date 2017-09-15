/*
 * ratentrigger.c
 * created: 30.03.1998 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: ratentrigger.c,v 1.13 2011/04/06 20:30:36 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#ifdef OBJ_VAR
#include "../../objects/var/variables.h"
#endif
#include "../../main/scheduler.h"
#include "../trigger.h"
#include "../trigprocs.h"
#include <sys/time.h>

struct private {
    int trigger;
    ems_u32* xrate;
    ems_u32 rate;
    int count;
    int startsec, startusec;
};

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "trigger/general")

/*****************************************************************************/
static void gettime(int* sec, int* usec)
{
#if defined(__unix__) || defined(Lynx) || defined(__Lynx__)
    struct timeval t;
    gettimeofday(&t, 0);
    *sec=t.tv_sec;
    *usec=t.tv_usec;
#else
#ifdef OSK
    int sekunden, tage, hundertstel;
    short wochentag;
    _sysdate(3, &sekunden, &tage, &wochentag, &hundertstel);
    *sec=tage*86400+sekunden;
    *usec=hundertstel*10000;
#else
  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#endif
#endif
}
/*****************************************************************************/
static plerrcode
done_trig_rate(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    free(tinfo->private);
    tinfo->private=0;
    return plOK;
}
/*****************************************************************************/
static int
get_trig_rate(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    int sec, usec, soll;
    float diff;

    if (priv->xrate && (*priv->xrate!=priv->rate)) {
        gettime(&priv->startsec, &priv->startusec);
        priv->count=0;
        priv->rate=*priv->xrate;
    }
    gettime(&sec, &usec);
    diff=(float)(sec-priv->startsec)+(float)(usec-priv->startusec)/1000000.;
    soll=diff*priv->rate;
    if (soll>priv->count) {
        trinfo->eventcnt++;
        priv->count++;
        return(priv->trigger);
    } else {
        return 0;
    }
}
/*****************************************************************************/
static void
reset_trig_rate(struct triggerinfo* trinfo)
{}
/*****************************************************************************/
plerrcode
init_trig_rate(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv;

    if ((p[0]<2) || (p[0]>3))
        return plErr_ArgNum;
    if (p[1]>=MAX_TRIGGER) {
        *outptr++=MAX_TRIGGER-1;
        return plErr_ArgRange;
    }

    priv=calloc(1, sizeof(struct private));
    if (!priv) {
        printf("init_trig_rate: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    tinfo->private=priv;

    priv->trigger=p[1];
    priv->rate=p[2];

    if (p[0]>2) {
#ifndef OBJ_VAR
        free(priv);
        return plErr_IllVar;
#else
        if (p[3]>MAX_VAR)
            return plErr_IllVar;
        if (var_list[p[3]].len!=1)
            return plErr_IllVarSize;
        priv->xrate=&var_list[p[3]].var.val;
        *priv->xrate=priv->rate;
#endif
    } else {
        priv->xrate=0;
    }
    trinfo->eventcnt=0;
    priv->count=0;
    gettime(&priv->startsec, &priv->startusec);

    tinfo->get_trigger=get_trig_rate;
    tinfo->reset_trigger=reset_trig_rate;
    tinfo->done_trigger=done_trig_rate;

    tinfo->proctype=tproc_poll;

    return plOK;
}
/*****************************************************************************/

char name_trig_rate[]="Rate";
int ver_trig_rate=1;

/*****************************************************************************/
/*****************************************************************************/
