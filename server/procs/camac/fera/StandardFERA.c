/*
 * procs/camac/fera/StandardFERA.c
 * created before 27.07.94
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: StandardFERA.c,v 1.16 2011/04/06 20:30:30 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <unistd.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/camac/camac.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../procs.h"
#include "../../procprops.h"
#include "../camac_verify.h"
#include "fera.h"

extern ems_u32* outptr;
extern int* memberlist;

#define MAXVSN 20

RCS_REGISTER(cvsid, "procs/camac/fera")

/*****************************************************************************/

#ifdef OSK
#define TIMEOUT 3000
static int to;
static timeout()
  {
  to=1;
  }
#else
#define TIMEOUT 5
#endif

/*****************************************************************************/

plerrcode proc_StandardFERAsetup(ems_u32* p)
/* in memberlist: M, DRV, mod... */
{
    ml_entry* m=ModulEnt(1);
    struct camac_dev* dev=m->address.camac.dev;
    ems_u32* data;
    int N=m->address.camac.slot;
    int i, vsn;
    int modanz= *memberlist;

    /* setup der ADC-Module */
    vsn=1;
    data= &p[1];
    for(i=3;i<=modanz;i++)
        data=SetupFERAmodul(m, data, &vsn);

/* memory: reset, ECL port enable */
    dev->CAMACwrite(dev, dev->CAMACaddrP(N, 1, 17), 1);
    dev->CAMACwrite(dev, dev->CAMACaddrP(N, 0, 17), 0);
    dev->CAMACwrite(dev, dev->CAMACaddrP(N, 1, 17), 3);

    return(plOK);
}

/*****************************************************************************/

plerrcode proc_StandardFERAmesspeds(ems_u32* p){
/* in memberlist: M, DRV, mod... */
#ifdef OSK
    int sig;
#endif
    int i;
    int offset[MAXVSN],len[MAXVSN];
    int zaehler,channels,events;
    int modanz= *memberlist-2, N;
    ml_entry* n;
    struct camac_dev* dev;
    ems_u32 qx;

    channels=0;
    zaehler=0;
    for(i=0;i<modanz;i++)channels+=len[i]=
          SetupFERAmesspeds(ModulEnt(i+3),i,&zaehler,offset);

    n=ModulEnt(1);
    dev=n->address.camac.dev;
    N=n->address.camac.slot;

    /* memory: reset, ECL port enable, LAM enable */
    dev->CAMACwrite(dev, dev->CAMACaddrP(N, 1, 17), 1);
    dev->CAMACwrite(dev, dev->CAMACaddrP(N, 0, 17), 0);
    dev->CAMACwrite(dev, dev->CAMACaddrP(N, 1, 17), 3);
    dev->CAMACcntl(dev, dev->CAMACaddrP(N, 0, 26), &qx);

#ifdef OSK
    sig=install_signalhandler(timeout);
#endif

    /* remove I */
    dev->CCCI(dev, 0);

    events=0;
    /* mindestens messped_min_events aufsummieren
     (im Ausgabefeld ab outptr) */
    do {
        ems_u32 count;
        int eventsnow;

        /* warte auf Memory, read out */
#ifdef OSK
        int alm;
        to=0;
        alm=alm_set(sig,TIMEOUT);
        if (alm==-1) {
            remove_signalhandler(sig);
            return(plErr_Other);
        }
        while ((!to)&&(!CAMACgotQ(CAMACread(CAMACaddr(n,0,10)))));
        if (to) {
            remove_signalhandler(sig);
            return(plErr_HW);
        }
        alm_delete(alm);
#else
        sleep(TIMEOUT);
        dev->CAMACread(dev, dev->CAMACaddrP(N, 0, 10), &qx);
        if (!dev->CAMACgotQ(qx))
                return plErr_HW;
#endif
        dev->CAMACwrite(dev, dev->CAMACaddrP(N, 1, 17), 1);
        dev->CAMACread(dev, dev->CAMACaddrP(N, 0, 1), &count);
        count&=0xffff;
        eventsnow=count/channels;
        if (count!=(eventsnow*channels)) {
#ifdef OSK
            remove_signalhandler(sig);
#endif
            return(plErr_HW);
        }
        events+=eventsnow;
        dev->CAMACwrite(dev, dev->CAMACaddrP(N, 0, 17), 0);
        while (eventsnow--) {
            for (i=0;i<modanz;i++) {
                int j;
                for (j=0;j<len[i];j++) {
                    ems_u32 ped;
                    dev->CAMACread(dev, dev->CAMACaddrP(N, 0, 0), &ped);
                    if (offset[i]>-1) outptr[offset[i]+j]+=ped&0xfff;
                }
            }
        }
        /* reset */
        dev->CAMACwrite(dev, dev->CAMACaddrP(N, 0, 17), 0);
        dev->CAMACwrite(dev, dev->CAMACaddrP(N, 1, 17), 3);
        dev->CAMACcntl(dev, CAMACaddrMP(ModulEnt(2), 0, 9), &qx); /* DRV */
    } while (events<messped_min_events);

#ifdef OSK
    remove_signalhandler(sig);
#endif
    dev->CAMACcntl(dev, dev->CAMACaddrP(N, 0, 24), &qx);

    /* durch Eventzahl teilen (in-place) */
    for(i=0;i<modanz;i++) {
        int j;
        if (offset[i]>-1) {
            for (j=0;j<len[i];j++) {
                outptr[offset[i]+j]/=events;
            }
        }
    }

    /* Ausgabepointer auf Feldende setzen */
    outptr+=zaehler;

    return(plOK);
}

/*---------------------------------------------------------------------------*/

plerrcode test_proc_StandardFERAsetup(ems_u32* p)
{
    ems_u32 modMEM[]={FERA_MEM_4302, 0};
    ems_u32 modDRV[]={FERA_DRV_4301, 0};
    int i,datause= *p;
    plerrcode pres;
    if (!memberlist) return(plErr_NoISModulList);
    if (*memberlist<3) return(plErr_BadISModList);

    if ((pres=test_proc_camac1(memberlist, 1, modMEM))!=plOK) {
        *outptr++=memberlist[1];
        return pres;
    }
    if ((pres=test_proc_camac1(memberlist, 2, modDRV))!=plOK) {
        *outptr++=memberlist[2];
        return pres;
    }
    for (i=3; i<=*memberlist; i++) {
        switch (modullist->entry[memberlist[i]].modultype) {
        case FERA_ADC_4300B:
            datause-=16;
            break;
        case SILENA_ADC_4418V:
            datause-=25;
            break;
        case BIT_PATTERN_UNIT:
            datause--;
            break;
        case CNRS_QDC_1612F:
            datause-=18;
            break;
        case CNRS_TDC_812F:
            datause-=9;
            break;
        default: *outptr++=memberlist[i];
            return plErr_BadModTyp;
        }
    }
    if (datause)
            return plErr_ArgNum;
    return plOK;
}

/*****************************************************************************/

plerrcode test_proc_StandardFERAmesspeds(ems_u32* p)
{
    int i;
    if (*p) return(plErr_ArgNum);
    if (!memberlist) return(plErr_NoISModulList);
    if (*memberlist<3) return(plErr_BadISModList);
    if (modullist->entry[memberlist[1]].modultype!=FERA_MEM_4302){
        *outptr++=memberlist[1];
        return(plErr_BadModTyp);
    }
    for(i=3;i<=*memberlist;i++){
        switch(modullist->entry[memberlist[i]].modultype){
            case FERA_ADC_4300B: break;
            case SILENA_ADC_4418V: break;
            case BIT_PATTERN_UNIT: break;
            default: *outptr++=memberlist[i];
                return(plErr_BadModTyp);
        }
    }
    return(plOK);
}

/*****************************************************************************/
#ifdef PROCPROPS
static procprop all_prop={-1, -1, "keine Ahnung", "keine Ahnung"};

procprop* prop_proc_StandardFERAsetup()
{
return(&all_prop);
}

procprop* prop_proc_StandardFERAmesspeds()
{
return(&all_prop);
}
#endif
/*****************************************************************************/

char name_proc_StandardFERAsetup[]="StandardFERAsetup";
char name_proc_StandardFERAmesspeds[]="StandardFERAmesspeds";

int ver_proc_StandardFERAsetup=1;
int ver_proc_StandardFERAmesspeds=1;

/*****************************************************************************/
/*****************************************************************************/
