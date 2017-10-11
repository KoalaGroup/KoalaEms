/*
 * trigger/vme/VMEpoll.c
 * created: 20.10.99 AM
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: VMEpoll.c,v 1.11 2015/04/21 16:44:35 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../objects/var/variables.h"
#include "../../main/scheduler.h"
#include "../trigger.h"
#include "../trigprocs.h"
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/unixvme/vme.h"
#if defined(__unix__) || defined(Lynx) || defined(__Lynx__)
#include <sys/time.h>
#endif

struct private {
    int trigger;
    int offset;
    int mask;
    int mode;
    int invert;
    struct vme_dev* dev;
    ml_entry* module;
};

extern ems_u32* outptr;
extern int *memberlist;

RCS_REGISTER(cvsid, "trigger/vme")

/*****************************************************************************/
static plerrcode done_trig_VMEpoll(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    free(tinfo->private);
    tinfo->private=0;
    return plOK;
}
/*****************************************************************************/
static int get_trig_VMEpoll(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    struct vme_dev* dev=priv->dev;
    unsigned int temp = 0;
    unsigned short temp16 = 0;
    switch(priv->mode) {
    case 2:
        dev->read_a32d16(dev, priv->module->address.vme.addr+priv->offset,
                &temp16);
        temp=temp16;
        break;
    case 4:
        dev->read_a32d32(dev, priv->module->address.vme.addr+priv->offset,
                &temp);
        break;
    }

#if 0
    printf("VMEtrigger: temp = 0x%04hx  mask = 0x%04hx invert = 0x%04hx\n",
            temp, mask, invert);  
#endif

    if ((temp & priv->mask)==priv->invert) {
        trinfo->eventcnt++;
        return priv->trigger;
    }

    return 0;
}
/*****************************************************************************/
static void reset_trig_VMEpoll(struct triggerinfo* trinfo)
{}
/*****************************************************************************/
/*
 * p[0]: argcount==5
 * p[1]: trigger
 * p[2]: module
 * p[3]: offset
 * p[4]: mode
 * p[5]: mask
 * p[6]: invert
*/
plerrcode init_trig_VMEpoll(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv;

    if (p[0]!=6)
        return plErr_ArgNum;
    if ((p[4]!=2) && (p[4]!=4))
        return plErr_ArgRange;
    if (p[1]>=MAX_TRIGGER) {
        *outptr++=MAX_TRIGGER-1;
        return plErr_ArgRange;
    }
    if (!valid_module(p[2], modul_vme))
        return plErr_ArgRange;

    priv=calloc(1, sizeof(struct private));
    if (!priv) {
        printf("init_trig_VMEpoll: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    tinfo->private=priv;

    priv->trigger=p[1];
    priv->offset=p[3];
    priv->mode=p[4];
    priv->mask=p[5];
    priv->invert=p[6];
  
    priv->module=ModulEnt(p[2]);
    priv->dev=priv->module->address.vme.dev;

    trinfo->eventcnt=0;	
  
    tinfo->get_trigger=get_trig_VMEpoll;
    tinfo->reset_trigger=reset_trig_VMEpoll;
    tinfo->done_trigger=done_trig_VMEpoll;

    tinfo->proctype=tproc_poll;

    return plOK;
}
/*****************************************************************************/

char name_trig_VMEpoll[] = "VMEpoll";
int ver_trig_VMEpoll     = 1;

/*****************************************************************************/
/*****************************************************************************/
