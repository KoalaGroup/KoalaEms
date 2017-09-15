/*
 * LVDpoll Trigger
 * created: 2004-12-21 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: LVDpoll.c,v 1.7 2011/04/06 20:30:36 wuestner Exp $";

#include <stdio.h>

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <sys/time.h>
#include <rcs_ids.h>
#include "../../../objects/var/variables.h"
#include "../../../main/scheduler.h"
#include "../../trigprocs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/devices.h"
#include "../../../lowlevel/lvd/lvdbus.h"
#include "../../../commu/commu.h"
#include "../../../objects/pi/readout.h"

static int trigger;
extern ems_u32 eventcnt;
extern ems_u32* outptr;
struct lvd_dev* dev;

RCS_REGISTER(cvsid, "trigger/pci/lvd")

/*
 * p[0]: argcount==2
 * p[1]: trigger
 * p[2]: device
*/
plerrcode init_trig_LVDpoll(ems_u32* p, struct trigprocinfo* info)
{
    if (p[0]!=2)
        return plErr_ArgNum;
    if (p[1]>=MAX_TRIGGER)
        return plErr_ArgRange;

    dev=(struct lvd_dev*)find_device(modul_lvd, p[2]);
    if (!dev)
        return plErr_ArgRange;

    trigger=p[1];
    eventcnt=0;	
  
    info->proctype=tproc_poll;

    if (dev->trigger_init(dev)<0) {
        printf("init lvd trigger failed\n");
        return plErr_Other;
    }
    return plOK;
}
/*****************************************************************************/
plerrcode done_trig_LVDpoll()
{
    dev->trigger_done(dev);
    dev=0;
    return plOK;
}
/*****************************************************************************/
int get_trig_LVDpoll()
{
    int res;

    res=dev->trigger_get(dev);
    if (res<0) {
        send_unsol_var(Unsol_RuntimeError, 4, rtErr_Trig, eventcnt, 5, 0);
        fatal_readout_error();
    }
    if (!res)
        return 0;

    eventcnt++;
    return trigger;
}
/*****************************************************************************/
void reset_trig_LVDpoll()
{
    dev->trigger_reset(dev);
}
/*****************************************************************************/

char name_trig_LVDpoll[] = "LVDpoll";
int ver_trig_LVDpoll     = 1;
int lam_trig_LVDpoll     = 0;

/*****************************************************************************/
/*****************************************************************************/
