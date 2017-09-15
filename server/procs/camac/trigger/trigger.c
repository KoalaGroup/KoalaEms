/*
 * procs/camac/trigger/trigger.c
 * 
 * created before 15.10.93
 * 02.Aug.2001 PW: multicrate support
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: trigger.c,v 1.9 2011/04/06 20:30:30 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/camac/camac.h"
#include "../../../trigger/camac/gsi/gsitrigger.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../procs.h"
#include "../../procprops.h"

extern int *memberlist;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/camac/trigger")

/*****************************************************************************/

plerrcode proc_ResetTrigger(ems_u32* p)
{
    ml_entry* n=ModulEnt(p[1]);
    struct camac_dev* dev=n->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(n), CONTROL, 16);

    dev->CAMACwrite(dev, &addr, HALT);
    dev->CAMACwrite(dev, &addr, SLAVE);
    dev->CAMACwrite(dev, &addr, BUS_ENABLE);
    dev->CAMACwrite(dev, &addr, CLEAR);
    dev->CAMACwrite(dev, &addr, BUS_DISABLE);
    return plOK;
}

plerrcode test_proc_ResetTrigger(ems_u32* p)
{
    if (p[0]!=1) return(plErr_ArgNum);
    if (!valid_module(p[1], modul_camac, 0)) return plErr_BadModTyp;
    if (ModulEnt(p[1])->modultype!=GSI_TRIGGER) return plErr_BadModTyp;
    return plOK;
}

#ifdef PROCPROPS
static procprop ResetTrigger_prop={0, 0, "slot",
    "setzt den GSI-Trigger zurueck"};

procprop* prop_proc_ResetTrigger()
{
    return(&ResetTrigger_prop);
}
#endif

char name_proc_ResetTrigger[]="ResetTrigger";
int ver_proc_ResetTrigger=1;

/*****************************************************************************/
/*****************************************************************************/
