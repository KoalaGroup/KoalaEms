/*
 * procs/unixvme/sis3100/vme_dspprocs.c
 * created 25.Jun.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: vme_dspprocs.c,v 1.7 2011/04/06 20:30:35 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <sys/types.h>

#include <modultypes.h>
#include "vme_dspprocs.h"
#include <rcs_ids.h>

RCS_REGISTER(cvsid, "procs/unixvme/sis3100")

struct vme_dspproc vme_dspprocs[]={
    {CAEN_V259, -1},
    {CAEN_V259E, -1},
    {CAEN_V262, -1},
    {CAEN_V512, -1},
    {CAEN_V550, 1},   /* 27.Jun.2003 */
    /*{CAEN_V550, 5},*/   /* 06.Aug.2003 */
    {CAEN_V551B, -1},
    {CAEN_V560, -1},
    {CAEN_V673, -1},
    {CAEN_V693, -1},
    {CAEN_V729A, -1},
    {CAEN_V767, -1},
    {CAEN_V775, 4},   /* 29.Jul.2003 */
    {CAEN_V785, -1},
    {CAEN_V792, -1},
    {CAEN_V820, -1},
    {CAEN_V830, -1},
    {CAEN_V1290, 0}, /* dummy procedure, module ignored */
    {CAEN_VN1488, -1},
    {SIS_3300, -1},
    {SIS_3301, -1},
    {SIS_3400, -1},
    {SIS_3600, -1},
    {SIS_3601, -1},
    {SIS_3700, -1},
    {SIS_3800, 3},    /* 29.Jul.2003 */
    {SIS_3801, -1},
    {SIS_3802, -1},
    {SIS_3803, -1},
    {SIS_3804, -1},
    {SIS_3805, -1},
    {SIS_3806, -1},
    {SIS_3807, -1},
    {SIS_3808, -1},
    {SIS_3809, -1},
    {SIS_3810, -1},
    {SIS_3811, -1},
    {VPG517, 0}, /* dummy procedure, module ignored */
    {VMIC8X16BITDAC, -1},
    {0, -1}
};
