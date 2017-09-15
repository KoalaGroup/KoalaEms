/*
 * procs/lvd/lvd_verify.c
 * created 2005-Feb-25 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_verify.c,v 1.3 2011/04/06 20:30:33 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/lvd/lvdbus.h"
#include "../../lowlevel/devices.h"
#include "lvd_verify.h"

RCS_REGISTER(cvsid, "procs/lvd")

plerrcode verify_lvd_id(struct lvd_dev* dev, ems_u32 addr, ems_u32 modultype)
{
    ems_u32 cardtype;
    int res;

    res=lvd_a_mtype(dev, addr, &cardtype);
    if (res<0)
        return plErr_BadHWAddr;

    return cardtype==modultype?plOK:plErr_BadModTyp;
}

plerrcode verify_lvd_ids(struct lvd_dev* dev, ems_u32 addr, ems_u32* modultype)
{
    ems_u32 cardtype;
    int res;

    res=lvd_a_mtype(dev, addr, &cardtype);
    if (res<0)
        return plErr_BadHWAddr;

    while (*modultype!=0) {
        if (cardtype==*modultype)
            return plOK;
        modultype++;
    }
    return plErr_BadModTyp;
}
