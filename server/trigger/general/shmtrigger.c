/*
 * trigger/general/shmtrigger.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: shmtrigger.c,v 1.8 2011/04/06 20:30:36 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errorcodes.h>
#include <objecttypes.h>
#include "emsctypes.h"
#include <rcs_ids.h>
#include "../../main/scheduler.h"
#include "../trigger.h"
#include "../trigprocs.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

struct private {
    int trigger;
    int id;
    int* addr;
};

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "trigger/general")

/*****************************************************************************/
static plerrcode
done_trig_shm(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;

    shmdt(priv->addr);
    free(tinfo->private);
    tinfo->private=0;
    return plOK;
}
/*****************************************************************************/
static int
get_trig_shm(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;

    if (*priv->addr>trinfo->eventcnt) {
        trinfo->eventcnt++;
        return priv->trigger;
    } else {
        return 0;
    }
}
/*****************************************************************************/
static void
reset_trig_shm(struct triggerinfo* trinfo)
{}
/*****************************************************************************/
plerrcode
init_trig_shm(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv;

    if (p[0]!=1)
        return plErr_ArgNum;
    if (p[1]>=MAX_TRIGGER) {
        *outptr++=MAX_TRIGGER-1;
        return plErr_ArgRange;
    }

    priv=calloc(1, sizeof(struct private));
    if (!priv) {
        printf("init_trig_immer: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    tinfo->private=priv;

    priv->trigger=p[1];

    priv->id=shmget(815, sizeof(int), IPC_CREAT|0666);
    if (priv->id==-1) {
        printf("shmtrigger: shmget: %s\n", strerror(errno));
        free(priv);
        return plErr_System;
    }
    priv->addr=(int*)shmat(priv->id, (void*)0, 0666);
    if (priv->addr==(int*)-1) {
        printf("shmtrigger: shmat: %s\n", strerror(errno));
        free(priv);
        return plErr_System;
    }
    *priv->addr=0;
    trinfo->eventcnt=0;

    tinfo->get_trigger=get_trig_shm;
    tinfo->reset_trigger=reset_trig_shm;
    tinfo->done_trigger=done_trig_shm;

    tinfo->proctype=tproc_poll;

    return plOK;
}
/*****************************************************************************/

char name_trig_shm[]="shm";
int ver_trig_shm=1;

/*****************************************************************************/
/*****************************************************************************/
