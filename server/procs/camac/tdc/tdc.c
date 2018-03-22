/*
 * procs/camac/tdc/tdc.c
 * created before 17.01.95
 * 21.Jan.2001 PW: multicrate support
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: tdc.c,v 1.19 2017/10/20 23:20:52 wuestner Exp $";

#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/camac/camac.h"
#include "../../procs.h"
#include "../../procprops.h"
#include "../../../objects/domain/dom_ml.h"

RCS_REGISTER(cvsid, "procs/camac/tdc")

/*****************************************************************************/

static plerrcode tdc2277modtest(void)
{
    unsigned int i;
    for (i=1; i<=*memberlist; i++) {
        ml_entry* module;
        if (!valid_module(i, modul_camac)) {
            *outptr++=memberlist[i];
            return plErr_BadModTyp;
        }
        module=ModulEnt(i);
        if (module->modultype!=LC_TDC_2277) {
            *outptr++=memberlist[i];
            return plErr_BadModTyp;
        }
    }
    return(plOK);
}
#ifdef PROCPROPS
static procprop all_prop={-1, -1, "keine Ahnung", "keine Ahnung"};
#endif
/*****************************************************************************/

plerrcode proc_TDC2277setup(ems_u32* p)
{
    unsigned int vsn;
    int res;

    for (vsn=1; vsn<=*memberlist; vsn++) {
        ml_entry* module=ModulEnt_m(vsn);
        struct camac_dev* dev=module->address.camac.dev;
        camadr_t addr=dev->CAMACaddr(CAMACslot_e(module), 0, 17);

        res=dev->CAMACwrite(dev, &addr, p[vsn]);
        if (res) {
            printf("TDC2277setup: CAMACwrite slot %d failed\n",
                CAMACslot_e(module));
            return plErr_System;
        }
    }
    return plOK;
}

plerrcode test_proc_TDC2277setup(ems_u32* p)
{
    if (!memberlist) return plErr_NoISModulList;
    if (*p!=*memberlist) return plErr_ArgNum;
    wirbrauchen=0;
    return tdc2277modtest();
}
#ifdef PROCPROPS
procprop* prop_proc_TDC2277setup()
{
return(&all_prop);
}
#endif
char name_proc_TDC2277setup[]="TDC2277setup";
int ver_proc_TDC2277setup=1;

/*****************************************************************************/

plerrcode test_proc_TDC2277readout(ems_u32* p)
{
    if (!memberlist) return plErr_NoISModulList;
    if (*p) return plErr_ArgNum;
    wirbrauchen=257*(*memberlist);
    return tdc2277modtest();
}

plerrcode test_proc_TDC2277gate(ems_u32* p)
{
    if (!memberlist) return plErr_NoISModulList;
    if (*p!=1) return plErr_ArgNum;
    wirbrauchen=257*(*memberlist);
    return tdc2277modtest();
}

plerrcode proc_TDC2277readout(__attribute__((unused)) ems_u32* p)
{
    unsigned int vsn;
    int res;

    for (vsn=1; vsn<=*memberlist; vsn++) {
        ml_entry* module=ModulEnt_m(vsn);
        struct camac_dev* dev=module->address.camac.dev;
        camadr_t addr=dev->CAMACaddr(CAMACslot_e(module), 0, 0);

        res=dev->CAMACblreadQstop(dev, &addr, 256, outptr);
        if (res<0) {
            printf("TDC2277readout: CAMACblreadQstop slot %d failed\n",
                CAMACslot_e(module));
            return plErr_System;
        }
        outptr+=res;
    }
    return(plOK);
}

plerrcode proc_TDC2277gate(ems_u32* p)
{
    unsigned int vsn;
    p++;
    for (vsn=1; vsn<=*memberlist; vsn++) {
        ml_entry* module=ModulEnt_m(vsn);
        struct camac_dev* dev=module->address.camac.dev;
        ems_u32 val;
	int space;
	camadr_t addr;

	space=256;
	addr=dev->CAMACaddr(CAMACslot_e(module), 0, 0);

        while ((dev->CAMACread(dev, &addr, &val), dev->CAMACgotQ(val)) &&
               (--space)) {
	    if ((val&0x0000ffff)<=*p) *outptr++=val;
        }
	if (!space) {
	    space=1000;
	    while (dev->CAMACread(dev, &addr, &val), dev->CAMACgotQ(val));
	    *outptr++=0;
	} else
            *outptr++=val;
    }
    return plOK;
}
#ifdef PROCPROPS
procprop* prop_proc_TDC2277readout()
{
    return(&all_prop);
}

procprop* prop_proc_TDC2277gate()
{
    return(&all_prop);
}
#endif
char name_proc_TDC2277readout[]="TDC2277readout";
char name_proc_TDC2277gate[]="TDC2277gate";
int ver_proc_TDC2277readout=1;
int ver_proc_TDC2277gate=1;

/*****************************************************************************/
plerrcode test_proc_TDC2228readout(ems_u32* p)
{
    unsigned int i;
    if (!memberlist) return plErr_NoISModulList;
    if (*p) return plErr_ArgNum;
    for (i=1; i<=*memberlist; i++) {
        ml_entry* module=ModulEnt_m(i);
        if ((module->modulclass!=modul_camac) ||
            ((module->modultype!=LC_TDC_2228) &&
             (module->modultype!=LC_TDC_2229))) {
            *outptr++=memberlist[i];
            return plErr_BadModTyp;
        }
    }
    wirbrauchen=8*(*memberlist);
    return plOK;
}

plerrcode proc_TDC2228readout(__attribute__((unused)) ems_u32* p)
{
    unsigned int vsn;
    int res;

    for (vsn=1; vsn<=*memberlist; vsn++) {
        ml_entry* module=ModulEnt_m(vsn);
        struct camac_dev* dev=module->address.camac.dev;
        camadr_t addr=dev->CAMACaddr(CAMACslot_e(module), 0, 2);

        res=dev->CAMACblreadAddrScan(dev, &addr, 8, outptr);
        if (res<0) {
            printf("TDC2228readout: CAMACblreadAddrScan slot %d failed\n",
                module->address.camac.slot);
            return plErr_System;
        }
        outptr+=res;
    }
    return plOK;
}
#ifdef PROCPROPS
procprop* prop_proc_TDC2228readout()
{
    return(&all_prop);
}
#endif
char name_proc_TDC2228readout[]="TDC2228readout";
int ver_proc_TDC2228readout=1;

/*****************************************************************************/
plerrcode test_proc_Scaler2551readout(ems_u32* p)
{
    unsigned int i;
    if (!memberlist) return plErr_NoISModulList;
    if (*p) return plErr_ArgNum;
    for (i=1; i<=*memberlist; i++) {
        ml_entry* module=ModulEnt_m(i);
        if ((module->modulclass!=modul_camac) ||
            (module->modultype!=LC_SCALER_2551)) {
            *outptr++=memberlist[i];
            return plErr_BadModTyp;
        }
    }
    wirbrauchen=12*(*memberlist);
    return plOK;
}

plerrcode proc_Scaler2551readout(__attribute__((unused)) ems_u32* p)
{
    unsigned int vsn;
    int res;

    for (vsn=1; vsn<=*memberlist; vsn++) {
        ml_entry* module=ModulEnt_m(vsn);
        struct camac_dev* dev=module->address.camac.dev;
        camadr_t addr=dev->CAMACaddr(CAMACslot_e(module), 0, 2);

        res=dev->CAMACblreadAddrScan(dev, &addr, 12, outptr);
        if (res<0) {
            printf("Scaler2551readout: CAMACblreadAddrScan slot %d failed\n",
                module->address.camac.slot);
            return plErr_System;
        }
        outptr+=res;
    }
    return plOK;
}
#ifdef PROCPROPS
procprop* prop_proc_Scaler2551readout()
{
    return(&all_prop);
}
#endif
char name_proc_Scaler2551readout[]="Scaler2551readout";
int ver_proc_Scaler2551readout=1;

/*****************************************************************************/
/*****************************************************************************/
