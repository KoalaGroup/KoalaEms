/*
 * procs/test/printevent.c
 * created: 04.11.94
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: printevent.c,v 1.10 2011/04/06 20:30:34 wuestner Exp $";

#include <stdio.h>
#include <errorcodes.h>
#include <sconf.h>
#include <rcs_ids.h>
#include "../procprops.h"
#include "../procs.h"
#include "../../trigger/trigger.h"

RCS_REGISTER(cvsid, "procs/test")

/*****************************************************************************/

plerrcode proc_PrintEvent(ems_u32* p)
{
#ifdef READOUT_CC
printf("event %d\n", trigger.eventcnt);
#endif
return(plOK);
}

plerrcode test_proc_PrintEvent(ems_u32* p)
{
#ifndef READOUT_CC
return(plErr_NoSuchProc);
#endif
if (*p!=0) return(plErr_ArgNum);
return(plOK);
}
#ifdef PROCPROPS
static procprop PrintEvent_prop={0, 0, "void", 0};

procprop* prop_proc_PrintEvent(void)
{
return(&PrintEvent_prop);
}
#endif
char name_proc_PrintEvent[]="PrintEvent";
int ver_proc_PrintEvent=1;

/*****************************************************************************/
/*****************************************************************************/
