/*
 * trigger/general/nietrigger.c
 * created before 24.03.93
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: nietrigger.c,v 1.10 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <emsctypes.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../main/scheduler.h"
#include "../trigger.h"
#include "../trigprocs.h"

RCS_REGISTER(cvsid, "trigger/general")

/*****************************************************************************/
static plerrcode
done_trig_nie(__attribute__((unused)) struct triggerinfo* trinfo)
{
    return plOK;
}

static int get_trig_nie(__attribute__((unused)) struct triggerinfo* trinfo)
{
    return 0;
}

static void reset_trig_nie(__attribute__((unused)) struct triggerinfo* trinfo)
{
}

plerrcode init_trig_nie(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    if (p[0]!=0)
        return plErr_ArgNum;
    trinfo->count=0;

    tinfo->get_trigger=get_trig_nie;
    tinfo->reset_trigger=reset_trig_nie;
    tinfo->done_trigger=done_trig_nie;

    tinfo->proctype=tproc_poll;
    return plOK;
}

char name_trig_nie[]="Nie";
int ver_trig_nie=1;

/*****************************************************************************/
/*****************************************************************************/
