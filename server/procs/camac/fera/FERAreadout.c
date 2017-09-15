/*
 * camac/fera/FERAreadout.c
 * 
 * created before 28.09.93
 * 02.Aug.2001 PW: multicrate support
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: FERAreadout.c,v 1.16 2011/04/06 20:30:30 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/camac/camac.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../procs.h"
#include "../../procprops.h"
#include "../camac_verify.h"

extern ems_u32* outptr;
extern int *memberlist, wirbrauchen;

RCS_REGISTER(cvsid, "procs/camac/fera")

/*****************************************************************************/

/* Auslesen eines FERA-Speichers */
/* Parameter: n */
plerrcode proc_FERAreadoutM(ems_u32* p)
{
    struct camac_dev* dev;
    ml_entry* n;
    int N;
    ems_u32 count;

    T(FERAreadoutM)

    n=ModulEnt(p[1]);
    dev=n->address.camac.dev;
    N=n->address.camac.slot;

    /* CAMAC-Modus, lesen, zurueck */
    dev->CAMACwrite(dev, dev->CAMACaddrP(N,1,17),1);
    dev->CAMACread(dev, dev->CAMACaddrP(N,0,1), &count);
    count&=0xffff;
    D(D_USER,printf("read %d words\n", count);)
    dev->CAMACwrite(dev, dev->CAMACaddrP(N,0,17),0);

    dev->CAMACblread(dev, dev->CAMACaddrP(N,0,0), count, outptr);
    outptr+=count;

    dev->CAMACwrite(dev, dev->CAMACaddrP(N,0,17),0);
    dev->CAMACwrite(dev, dev->CAMACaddrP(N,1,17),3);

    return(plOK);
}

#define LOOPS 100000

/* Auslesen eines FERA-Speichers, pollt FSC dafuer.
 (Singleevent-Modus) */
plerrcode proc_FERAreadout(ems_u32* p)
/* in memberlist: FSC, M1, M2, ... */
{
    struct camac_dev* dev_fsc;
    struct camac_dev* dev_N;
    int welcher, n;
    ml_entry *fsc, *N;
    camadr_t fscaddr1, fscaddr2;
    ems_u32 count, qx;

    T(FERAreadout)

    fsc=ModulEnt(1);
    dev_fsc=fsc->address.camac.dev;

    fscaddr1=CAMACaddrM(fsc, 0, 10);
    fscaddr2=CAMACaddrM(fsc, 1, 10);
    n=LOOPS;
poll:
    if (dev_fsc->CAMACread(dev_fsc, &fscaddr1, &qx), dev_fsc->CAMACgotQ(qx)){
        welcher=2;
        dev_fsc->CAMACcntl(dev_fsc, &fscaddr1, &qx);
    } else if (dev_fsc->CAMACread(dev_fsc, &fscaddr2, &qx), dev_fsc->CAMACgotQ(qx)) {
        welcher=3;
        dev_fsc->CAMACcntl(dev_fsc, &fscaddr2, &qx);
    } else if (!n--) {
        return(plErr_HW);
    } else {
        goto poll;
    }

    N=ModulEnt(welcher); /* Slotnummer des Speichers */
    D(D_USER, printf("Mem %d in Slot %d\n",welcher-1,n);)
    dev_N=N->address.camac.dev;

    /* CAMAC-Modus, lesen, zurueck */
    dev_N->CAMACwrite(dev_N, CAMACaddrMP(N, 1, 17), 1);
    dev_N->CAMACread(dev_N, CAMACaddrMP(N, 0, 1), &count);
    count&=0xffff;
    D(D_USER,printf("lese %d Worte\n", count);)
    dev_N->CAMACwrite(dev_N, CAMACaddrMP(N, 0, 17), 0);
    outptr+=dev_N->CAMACblread(dev_N, CAMACaddrMP(N, 0, 0), count, outptr);
    dev_N->CAMACwrite(dev_N, CAMACaddrMP(N, 0, 17), 0);
    dev_N->CAMACwrite(dev_N, CAMACaddrMP(N, 1, 17), 3);

    /* FSC-Reset */
    dev_fsc->CAMACwrite(dev_fsc, CAMACaddrMP(fsc, 0, 16), welcher>2?0x25:0x23);

    return(plOK);
}

plerrcode proc_FERAreadoutC(ems_u32* p)
/* in memberlist: FSC, M1, M2, ... */
{
    struct camac_dev* dev_fsc;
    struct camac_dev* dev_N;
    int welcher;
    ml_entry *N, *fsc;
    camadr_t fscaddr1, fscaddr2;
    ems_u32 count, qx, val;

    T(FERAreadoutC)

    fsc=ModulEnt(1);
    dev_fsc=fsc->address.camac.dev;
    fscaddr1=CAMACaddrM(fsc, 0, 10);
    fscaddr2=CAMACaddrM(fsc, 1, 10);

    if (dev_fsc->CAMACread(dev_fsc, &fscaddr1, &qx), dev_fsc->CAMACgotQ(qx)) {
        welcher=2;
        dev_fsc->CAMACcntl(dev_fsc, &fscaddr1, &qx);
    } else if (dev_fsc->CAMACread(dev_fsc, &fscaddr2, &qx), dev_fsc->CAMACgotQ(qx)) {
        welcher=3;
        dev_fsc->CAMACcntl(dev_fsc, &fscaddr2, &qx);
    } else {
        N=ModulEnt(2);
        dev_N=N->address.camac.dev;
        dev_N->CAMACwrite(dev_N, CAMACaddrMP(N, 1, 17), 1);
        dev_N->CAMACread(dev_N, CAMACaddrMP(N, 0, 1), &val);
        *outptr++=val&0xffff;
        dev_N->CAMACwrite(dev_N, CAMACaddrMP(N, 1, 17), 3);
        N=ModulEnt(3);
        dev_N=N->address.camac.dev;
        dev_N->CAMACwrite(dev_N, CAMACaddrMP(N, 1, 17), 1);
        dev_N->CAMACread(dev_N, CAMACaddrMP(N, 0, 1), &val);
        *outptr++=val&0xffff;
        dev_N->CAMACwrite(dev_N, CAMACaddrMP(N, 1, 17), 3);
        return(plOK);
    }

    N=ModulEnt(welcher); /* Slotnummer des Speichers */
    dev_N=N->address.camac.dev;
    D(D_USER,
        printf("Mem %d in Slot %d, Crate %d\n",
            welcher-1,
            N->address.camac.slot,
            N->address.camac.crate);
    )

    /* CAMAC-Modus, lesen, zurueck */
    dev_N->CAMACwrite(dev_N, CAMACaddrMP(N, 1, 17), 1);
    dev_N->CAMACread(dev_N, CAMACaddrMP(N,0,1), &count);
    count&=0xffff;
    D(D_USER,printf("lese %d Worte\n", count);)
    dev_N->CAMACwrite(dev_N, CAMACaddrMP(N, 0, 17), 0);
    outptr+=dev_N->CAMACblread(dev_N, CAMACaddrMP(N, 0, 0), count, outptr);
    dev_N->CAMACwrite(dev_N, CAMACaddrMP(N,0,17), 0);
    dev_N->CAMACwrite(dev_N, CAMACaddrMP(N,1,17), 3);

    /* FSC-Reset */
    dev_fsc->CAMACwrite(dev_fsc, CAMACaddrMP(fsc, 0, 16), welcher>2?0x05:0x03);

    return(plOK);
}

static plerrcode test_ferasystem(void)
{
    ems_u32 modFSC[]={FSC, 0};
    ems_u32 modMEM[]={FERA_MEM_4302, 0};
    plerrcode pres;

    if (!memberlist) return(plErr_NoISModulList);
    if (*memberlist<3) return(plErr_BadISModList);

    if ((pres=test_proc_camac1(memberlist, 1, modFSC))!=plOK) {
        *outptr++=memberlist[1];
        return pres;
    }
    if ((pres=test_proc_camac1(memberlist, 2, modMEM))!=plOK) {
        *outptr++=memberlist[1];
        return pres;
    }
    if ((pres=test_proc_camac1(memberlist, 3, modMEM))!=plOK) {
        *outptr++=memberlist[1];
        return pres;
    }
    return(plOK);
}

plerrcode test_proc_FERAreadoutM(ems_u32* p)
{
    ems_u32 modMEM[]={FERA_MEM_4302, 0};
    plerrcode pres;

    if (*p!=1) return(plErr_ArgNum);
    if ((pres=test_proc_camac1(memberlist, p[1], modMEM))!=plOK) {
        *outptr++=p[1];
        return pres;
    }
    return plOK;
}

plerrcode test_proc_FERAreadout(ems_u32* p)
{
    if (*p) return(plErr_ArgNum);
    return test_ferasystem();
}

plerrcode test_proc_FERAreadoutC(ems_u32* p)
{
    if (*p) return(plErr_ArgNum);
    wirbrauchen=16*1024;
    return(test_ferasystem());
}
#ifdef PROCPROPS
static procprop all_prop={-1, -1, "keine Ahnung", "keine Ahnung"};

procprop* prop_proc_FERAreadoutM()
{
    return(&all_prop);
}

procprop* prop_proc_FERAreadout()
{
    return(&all_prop);
}

procprop* prop_proc_FERAreadoutC()
{
    return(&all_prop);
}
#endif
char name_proc_FERAreadoutM[]="FERAreadoutM";
char name_proc_FERAreadout[]="FERAreadout";
char name_proc_FERAreadoutC[]="FERAreadoutC";

int ver_proc_FERAreadoutM=1;
int ver_proc_FERAreadout=1;
int ver_proc_FERAreadoutC=1;

/*****************************************************************************/
/*****************************************************************************/
