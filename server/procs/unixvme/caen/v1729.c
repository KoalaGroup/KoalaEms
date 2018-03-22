/*
 * procs/unixvme/caen/v1729.c
 * created 2005-Sep-09 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: v1729.c,v 1.4 2017/10/20 23:20:52 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/unixvme/vme.h"
#include "../../../lowlevel/devices.h"
#include "../vme_verify.h"

extern ems_u32* outptr;
extern unsigned int *memberlist;

RCS_REGISTER(cvsid, "procs/unixvme/caen")

/*
 * V1729 4-channel 12-bit 2GHz sampling ADC
 * 
 */

/*****************************************************************************/
plerrcode
proc_v1729reset(__attribute__((unused)) ems_u32* p)
{
    return plOK;
}

plerrcode
test_proc_v1729reset(__attribute__((unused)) ems_u32* p)
{
    return plOK;
}

char name_proc_v1729reset[] = "v1729reset";
int ver_proc_v1729reset = 1;
/*****************************************************************************/
plerrcode
proc_v1729calibrate_vernier(__attribute__((unused)) ems_u32* p)
{
    return plOK;
}

plerrcode
test_proc_v1729calibrate_vernier(__attribute__((unused)) ems_u32* p)
{
    return plOK;
}

char name_proc_v1729calibrate_vernier[] = "v1729calibrate_vernier";
int ver_proc_v1729calibrate_vernier = 1;
/*****************************************************************************/
plerrcode
proc_v1729calibrate_pedestals(__attribute__((unused)) ems_u32* p)
{
    return plOK;
}

plerrcode
test_proc_v1729calibrate_pedestals(__attribute__((unused)) ems_u32* p)
{
    return plOK;
}

char name_proc_v1729calibrate_pedestals[] = "v1729calibrate_pedestals";
int ver_proc_v1729calibrate_pedestals = 1;
/*****************************************************************************/
plerrcode
proc_v1729start(__attribute__((unused)) ems_u32* p)
{
    return plOK;
}

plerrcode
test_proc_v1729start(__attribute__((unused)) ems_u32* p)
{
    return plOK;
}

char name_proc_v1729start[] = "v1729start";
int ver_proc_v1729start = 1;
/*****************************************************************************/
plerrcode
proc_v1729trigger(__attribute__((unused)) ems_u32* p)
{
    return plOK;
}

plerrcode
test_proc_v1729trigger(__attribute__((unused)) ems_u32* p)
{
    return plOK;
}

char name_proc_v1729trigger[] = "v1729trigger";
int ver_proc_v1729trigger = 1;
/*****************************************************************************/
plerrcode
proc_v1729read(__attribute__((unused)) ems_u32* p)
{
    return plOK;
}

plerrcode
test_proc_v1729read(__attribute__((unused)) ems_u32* p)
{
    return plOK;
}

char name_proc_v1729read[] = "v1729read";
int ver_proc_v1729read = 1;
/*****************************************************************************/
/*****************************************************************************/
