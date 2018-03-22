/*
 * ems/server/procs/hv/sy527/sy527.c
 * created 2007-08-06 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sy527.c,v 1.3 2017/10/09 21:25:38 wuestner Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../procprops.h"
#include "../../../lowlevel/caenet/caennet.h"
#include "../../../lowlevel/devices.h"
#include "../../../objects/domain/dom_ml.h"

extern ems_u32* outptr;

#define get_device(bus) \
    (struct caenet_dev*)get_gendevice(modul_caenet, (bus))

RCS_REGISTER(cvsid, "procs/hv/sy527")

/*****************************************************************************/
/*
 * p[0]: argnum
 * p[1]: bus
 * p[2]: module_id
 */
plerrcode proc_sy527_write(ems_u32* p)
{
    struct caenet_dev* dev=get_device(p[1]);
    /*int module_id=p[2];*/
    plerrcode pres;

    pres=dev->write(dev, 0, 0);

    return pres;
}

plerrcode test_proc_sy527_write(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_caenet, p[1], 0))!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_sy527_write[]="sy527_write";
int ver_proc_sy527_write=1;
/*****************************************************************************/
/*****************************************************************************/
