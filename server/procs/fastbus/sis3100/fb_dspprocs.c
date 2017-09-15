/*
 * procs/fastbus/sis3100/vme_dspprocs.c
 * created 2004-03-25 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fb_dspprocs.c,v 1.2 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <sys/types.h>

#include <modultypes.h>
#include <rcs_ids.h>
#include "fb_dspprocs.h"

RCS_REGISTER(cvsid, "procs/fastbus/sis3100")

struct fb_dspproc fb_dspprocs[]={
    {PH_ADC_10C2, 1},
    {LC_ADC_1881, -1},
    {LC_TDC_1877, -1},
    {LC_TDC_1875A, -1},
    {0, -1}
};
