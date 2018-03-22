/*
 * procs/camac/fera/fsc_FERA.c
 * 
 * 02.Aug.2001 PW: multicrate support
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fsc_FERA.c,v 1.9 2017/10/20 23:20:52 wuestner Exp $";

/* FERA-System fuer Readout initialisieren,
  Daten aus einem Setup-Feld verwenden */
/* Parameter: FSCtimeout, -delay, Single-Event-Flag, Pedestrals */

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

RCS_REGISTER(cvsid, "procs/camac/fera")

plerrcode proc_FERAsetup(ems_u32* p)
/* in memberlist: FSC, M1, M2, DRV, mod... */
{
    ml_entry* m1=ModulEnt(1);
    ml_entry* m2=ModulEnt(2);
    ml_entry* m3=ModulEnt(3);
    struct camac_dev* dev1=m1->address.camac.dev;
    struct camac_dev* dev2=m2->address.camac.dev;
    struct camac_dev* dev3=m3->address.camac.dev;
    int i,vsn,singleevent;
    int modanz= *memberlist-4;
    ems_u32 qx, *data;

    /* reset, Timeout und Delay fuer FSC */
    dev1->CAMACcntl(dev1, CAMACaddrMP(m1, 0, 9), &qx);
    dev1->CAMACwrite(dev1, CAMACaddrMP(m1, 1, 16), p[1]);
    dev1->CAMACwrite(dev1, CAMACaddrMP(m1, 2, 16), p[2]);

    singleevent=p[3];

    /* setup der ADC-Module */
    vsn=1;
    data= &p[4];
    for(i=0;i<modanz;i++)
        data=SetupFERAmodul(ModulEnt(i+5),data,&vsn);

/* memories: reset, ECL port enable */
    dev2->CAMACwrite(dev2, CAMACaddrMP(m2, 1, 17), 1);
    dev2->CAMACwrite(dev2, CAMACaddrMP(m2, 0, 17), 0);
    dev2->CAMACwrite(dev2, CAMACaddrMP(m2, 1, 17), 3);
    dev3->CAMACwrite(dev3, CAMACaddrMP(m3, 1, 17), 1);
    dev3->CAMACwrite(dev3, CAMACaddrMP(m3, 0, 17), 0);
    dev3->CAMACwrite(dev3, CAMACaddrMP(m3, 1, 17), 3);

/* FSC: reset, enable LAM */
    dev1->CAMACwrite(dev1, CAMACaddrMP(m1, 0, 16),singleevent?0x27:7);
    dev1->CAMACcntl(dev1, CAMACaddrMP(m1, 0, 26), &qx);

    return(plOK);
}

plerrcode test_proc_FERAsetup(ems_u32* p)
{
    ems_u32 modFSC[]={FSC, 0};
    ems_u32 modMEM[]={FERA_MEM_4302, 0};
    ems_u32 modDRV[]={FERA_DRV_4301, 0};
    unsigned int i;
    ems_u32* data = p+4;
    plerrcode pres;

    if (!memberlist) return(plErr_NoISModulList);
    if (*memberlist<5) return(plErr_BadISModList);

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
    for (i=5;i<=*memberlist;i++) {
        plerrcode res;

        res = test_SetupFERA(ModulEnt(i), &data);
        if (res) {
	    *outptr++= memberlist[i];
	    return(res);
        }
    }

    if (data != &p[p[0] + 1])
            return(plErr_ArgNum);

    return(plOK);
}
#ifdef PROCPROPS
static procprop all_prop={-1, -1, "keine Ahnung", "keine Ahnung"};

procprop* prop_proc_FERAsetup()
{
    return(&all_prop);
}
#endif
char name_proc_FERAsetup[]="FERAsetup";

int ver_proc_FERAsetup=1;
