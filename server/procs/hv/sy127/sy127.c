/*
 * ems/server/procs/hv/sy127/sy127.c
 * created 2007-08-06 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sy127.c,v 1.2 2011/04/06 20:30:33 wuestner Exp $";

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
extern int wirbrauchen;

#define get_device(bus) \
    (struct caenet_dev*)get_gendevice(modul_caenet, (bus))

RCS_REGISTER(cvsid, "procs/hv/sy127")

/*****************************************************************************/
/*
 * p[0]: argnum
 * p[1]: bus
 * p[2]: module_id
 */
plerrcode proc_sy127_write(ems_u32* p)
{
    struct caenet_dev* dev=get_device(p[1]);
    /*int module_id=p[2];*/
    plerrcode pres;

    pres=dev->write(dev, 0, 0);

    return pres;
}

plerrcode test_proc_sy127_write(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_caenet, p[1], 0))!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_sy127_write[]="sy127_write";
int ver_proc_sy127_write=1;
/*****************************************************************************/
/*****************************************************************************/
