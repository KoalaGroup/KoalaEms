/*
 * procs/camac/fera/FERAmesspeds.c
 * 
 * created before 28.09.93
 * 02.Aug.2001 PW: multicrate support
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: FERAmesspeds.c,v 1.13 2017/10/20 23:20:52 wuestner Exp $";

/* Setup-Feld fuer FERA-Systeme erzeugen,
  dafuer Input der FERA-ADCs mitteln,
  fuer SILENA und Patternunits Defaultwerte einsetzen */
/* Parameter: FSC-timeout, -delay */

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

#define MAXVSN 20

RCS_REGISTER(cvsid, "procs/camac/fera")

/*****************************************************************************/

#ifdef OSK
#define TIMEOUT 3000
static int to;
static timeout(){
    to=1;
}
#else
#define TIMEOUT 5
#endif

/*****************************************************************************/

/* Setup der ADC-Module,
 sammle die Daten:
 zaehler: Gesamtlaenge des Setup-Feldes
 offset[vsn]: Start des Moduls im Setup-feld
 Return: zu erwartende Datenmenge pro Event */
int SetupFERAmesspeds(ml_entry* n, int index, int* zaehler, int* offset)
{
    struct camac_dev* dev=n->address.camac.dev;
    int N=n->address.camac.slot;
    int j, channels=0;

    switch (n->modultype) {
    case FERA_ADC_4300B:
    case FERA_TDC_4300B:
        dev->CAMACwrite(dev, dev->CAMACaddrP(N,0,16), FERA_ped_stat);
        for (j=0; j<16; j++)
            dev->CAMACwrite(dev, dev->CAMACaddrP(N, j, 17),outptr[*zaehler+j]=0);
        offset[index]= *zaehler;
        *zaehler+=16; /* braucht 16 Setup-Daten */
        channels=16;
        break;
    case SILENA_ADC_4418V:
        dev->CAMACwrite(dev, dev->CAMACaddrP(N,14,20),SILENA_ped_stat);
        dev->CAMACwrite(dev, dev->CAMACaddrP(N,9,20),
                              outptr[*zaehler]=SILENA_def_comthr);
        for (j=0;j<8;j++)
            dev->CAMACwrite(dev, dev->CAMACaddrP(N,j+8,17),
                             outptr[*zaehler+1+j]=SILENA_def_uthr);
        for (j=0;j<8;j++)
            dev->CAMACwrite(dev, dev->CAMACaddrP(N,j,17),
                             outptr[*zaehler+9+j]=SILENA_def_lthr);
        for (j=0;j<8;j++)
            dev->CAMACwrite(dev, dev->CAMACaddrP(N,j,20),
                             outptr[*zaehler+17+j]=SILENA_def_offs);
        offset[index]= -1; /* nicht aufsummieren! */
        *zaehler+=25; /* braucht 25 Setup-Daten */
        channels=8;
        break;
    case BIT_PATTERN_UNIT:
        dev->CAMACwrite(dev, dev->CAMACaddrP(N,0,17), BPU_ped_stat); /*disable*/
        outptr[*zaehler]=BPU_def_chan; /* default-Kanalzahl */
        offset[index]= -1;
        *zaehler+=1;
        channels=0; /* ignoriert */
        break;
    }
    return(channels);
}

plerrcode proc_FERAmesspeds(ems_u32* p)
/* in memberlist: FSC, M1, M2, DRV, mod... */
{
    struct camac_dev* dev1=ModulEnt(1)->address.camac.dev;
    struct camac_dev* dev2=ModulEnt(2)->address.camac.dev;
    struct camac_dev* dev3=ModulEnt(3)->address.camac.dev;
    int N1=ModulEnt(1)->address.camac.slot;
    int N2=ModulEnt(2)->address.camac.slot;
    int N3=ModulEnt(3)->address.camac.slot;
#ifdef OSK
    int sig;
#endif
    int i;
    int offset[MAXVSN],len[MAXVSN];
    int zaehler,channels,events;
    int modanz= *memberlist-4;
    ems_u32 val, qx;

    /* reset, Timeout und Delay fuer FSC */
    dev1->CAMACcntl(dev1, dev1->CAMACaddrP(N1,0,9), &qx);
    dev1->CAMACwrite(dev1, dev1->CAMACaddrP(N1, 1, 16), *outptr++= *++p);
    dev1->CAMACwrite(dev1, dev1->CAMACaddrP(N1, 2, 16), *outptr++= *++p);
    *outptr++=0;  /* fuer Single-Event-Flag freihalten */

    channels=0;
    zaehler=0;
    for(i=0;i<modanz;i++)channels+=len[i]=
          SetupFERAmesspeds(ModulEnt(i+5),i,&zaehler,offset);

/* memories: reset, ECL port enable */
    dev2->CAMACwrite(dev2, dev2->CAMACaddrP(N2,1,17),1);
    dev2->CAMACwrite(dev2, dev2->CAMACaddrP(N2,0,17),0);
    dev2->CAMACwrite(dev2, dev2->CAMACaddrP(N2,1,17),3);
    dev3->CAMACwrite(dev3, dev3->CAMACaddrP(N3,1,17),1);
    dev3->CAMACwrite(dev3, dev3->CAMACaddrP(N3,0,17),0);
    dev3->CAMACwrite(dev3, dev3->CAMACaddrP(N3,1,17),3);

/* FSC: reset, enable LAM */
    dev1->CAMACwrite(dev1, dev1->CAMACaddrP(N1,0,16),7);
    dev1->CAMACcntl(dev1, dev1->CAMACaddrP(N1,0,26), &qx);

#ifdef OSK
    sig=install_signalhandler(timeout);
#endif

/* remove I */
/*CAMACcntl(CAMACaddr(30,9,24));*/
    dev1->CCCI(dev1, 0);

    events=0;
/* mindestens messped_min_events aufsummieren
(im Ausgabefeld ab outptr) */
    do {
        int eventsnow,count;

/* warte auf Memory 1, read out */
#ifdef OSK
        int alm;
        to=0;
        alm=alm_set(sig,TIMEOUT);
        if(alm==-1) {
            remove_signalhandler(sig);
            return(plErr_Other);
        }
        while ((!to)&&(!CAMACgotQ(CAMACreadX(CAMACaddr(memberlist[1],0,10)))));
        if(to) {
            remove_signalhandler(sig);
            return(plErr_HW);
        }
        alm_delete(alm);
#else
        sleep(TIMEOUT);
        if (dev1->CAMACread(dev1, dev1->CAMACaddrP(N1,0,10), &qx), !dev1->CAMACgotQ(qx))
            return plErr_HW;
#endif
        dev2->CAMACwrite(dev2, dev2->CAMACaddrP(N2,1,17),1);
        dev2->CAMACread(dev2, dev2->CAMACaddrP(N2,0,1), &val);
        count=dev2->CAMACval(val) & 0xffff;
        eventsnow=count/channels;
        if(count!=(eventsnow*channels)) {
#ifdef OSK
            remove_signalhandler(sig);
#endif
            return(plErr_HW);
        }
        D(D_USER,printf("%d Events aus M1 gemessen\n",eventsnow);)
        events+=eventsnow;
        dev2->CAMACwrite(dev2, dev2->CAMACaddrP(N2,0,17),0);
        while(eventsnow--) {
            for(i=0;i<modanz;i++) {
                int j;
                for (j=0; j<len[i]; j++) {
                    int ped;
                    dev2->CAMACread(dev2, dev2->CAMACaddrP(N2,0,0), &val);
                    ped=dev2->CAMACval(val) & 0xfff;
                    if (offset[i]>-1) outptr[offset[i]+j]+=ped;
                }
            }
        }
/* reset */
        dev2->CAMACwrite(dev2, dev2->CAMACaddrP(N2,0,17),0);
        dev2->CAMACwrite(dev2, dev2->CAMACaddrP(N2,1,17),3);
        dev1->CAMACwrite(dev1, dev1->CAMACaddrP(N1,0,16),3);

  /* warte auf Memory 2, read out */
#ifdef OSK
        to=0;
        alm=alm_set(sig,TIMEOUT);
        if (alm==-1) {
            remove_signalhandler(sig);
            return(plErr_Other);
        }
        while((!to)&&(!CAMACgotQ(dev1->CAMACread_(dev1, dev1->CAMACaddr(N1,1,10)))));
        if(to) {
            remove_signalhandler(sig);
            return(plErr_HW);
        }
        alm_delete(alm);
#else
        sleep(TIMEOUT);
        if (dev1->CAMACread(dev1, dev1->CAMACaddrP(N1,1,10), &qx), !dev1->CAMACgotQ(qx))
            return plErr_HW;
#endif
        dev3->CAMACwrite(dev3, dev3->CAMACaddrP(N3,1,17),1);
        dev3->CAMACread(dev3, dev3->CAMACaddrP(N3,0,1), &val);
        count=dev3->CAMACval(val) & 0xffff;
        eventsnow=count/channels;
        if(count!=(eventsnow*channels)) {
#ifdef OSK
            remove_signalhandler(sig);
#endif
            return(plErr_HW);
        }
        D(D_USER,printf("%d Events aus M2 gemessen\n",eventsnow);)
        events+=eventsnow;
        dev3->CAMACwrite(dev3, dev3->CAMACaddrP(N3,0,17),0);
        while(eventsnow--) {
            for(i=0;i<modanz;i++) {
                int j;
                for(j=0; j<len[i]; j++) {
                    int ped;
                    dev3->CAMACread(dev3, dev3->CAMACaddrP(N3,0,0), &val);
                    ped=dev3->CAMACval(val) & 0xfff;
                    if (offset[i]>-1) outptr[offset[i]+j]+=ped;
                }
            }
        }
  /* reset */
        dev3->CAMACwrite(dev3, dev3->CAMACaddrP(N3,0,17),0);
        dev3->CAMACwrite(dev3, dev3->CAMACaddrP(N3,1,17),3);
        dev3->CAMACwrite(dev3, dev3->CAMACaddrP(N1,0,16),5);

    } while(events<messped_min_events);

#ifdef OSK
    remove_signalhandler(sig);
#endif

/* durch Eventzahl teilen (in-place) */
    for(i=0;i<modanz;i++) {
        int j;
        if(offset[i]>-1)
            for(j=0;j<len[i];j++) {
                outptr[offset[i]+j]/=events;
            }
    }

/* Ausgabepointer auf Feldende setzen */
    outptr+=zaehler;

    return(plOK);
}

plerrcode test_proc_FERAmesspeds(ems_u32* p)
{
    ems_u32 modFSC[]={FSC, 0};
    ems_u32 modMEM[]={FERA_MEM_4302, 0};
    ems_u32 modDRV[]={FERA_DRV_4301, 0};
    ems_u32 modADC[]={FERA_ADC_4300B, SILENA_ADC_4418V, BIT_PATTERN_UNIT, 0};
    plerrcode pres;
    unsigned int i;

    if (*p!=2) return plErr_ArgNum;
    if (!memberlist) return plErr_NoISModulList ;
    if (*memberlist<5) return plErr_BadISModList;
    if ((pres=test_proc_camac1(memberlist, 1, modFSC))!=plOK) {
        *outptr++=memberlist[1];
        return pres;
    }
    if ((pres=test_proc_camac1(memberlist, 2, modMEM))!=plOK) {
        *outptr++=memberlist[2];
        return pres;
    }
    if ((pres=test_proc_camac1(memberlist, 3, modMEM))!=plOK) {
        *outptr++=memberlist[3];
        return pres;
    }
    if ((pres=test_proc_camac1(memberlist, 4, modDRV))!=plOK) {
        *outptr++=memberlist[4];
        return pres;
    }
    for (i=5; i<=*memberlist; i++) {
        if ((pres=test_proc_camac1(memberlist, i, modADC))!=plOK) {
            *outptr++=memberlist[i];
            return pres;
        }
    }
    return(plOK);
}
#ifdef PROCPROPS
static procprop all_prop={-1, -1, "keine Ahnung", "keine Ahnung"};

procprop* prop_proc_FERAmesspeds()
{
    return(&all_prop);
}
#endif
char name_proc_FERAmesspeds[]="FERAmesspeds";

int ver_proc_FERAmesspeds=1;

/*****************************************************************************/
/*****************************************************************************/
