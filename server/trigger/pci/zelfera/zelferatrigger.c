/*
 * trigger/pci/zelfera/zelferatrigger.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: zelferatrigger.c,v 1.14 2017/10/21 22:21:27 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <dev/pci/pcicamvar.h>
#include <rcs_ids.h>

#include "../../../main/scheduler.h"
#include "../../trigger.h"
#include "../../trigprocs.h"
#include "../../../lowlevel/devices.h"
#include "../../../lowlevel/camac/camac.h"

RCS_REGISTER(cvsid, "trigger/pci/zelfera")

struct private {
    struct camac_dev* dev;
    int p;
};

static plerrcode
done_trig_zelfera(struct triggerinfo* trinfo)
{
        struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;

#if 1
        printf("done_trig_zelfera\n");
#endif
        free(tinfo->private);
        tinfo->private=0;
	return plOK;
}

static int
get_trig_zelfera(struct triggerinfo* trinfo)
{
#if 1
        struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
        struct private* priv=(struct private*)tinfo->private;
	struct feraeventinfo i;
	int res;
#endif

#if 1
        printf("get_trig_zelfera\n");
#endif
#if 1
	i.flags = FERAWAIT;
	i.timeout = 1;
	res = ioctl(priv->p, FERAEVENT, &i);
	if (res < 0) {
		printf("get_trig_zelfera: %s\n", strerror(errno));
		return (0);
	}
	if (!i.waitstat) {
		printf("get_trig_zelfera: no event\n");
		return (0);
	}
#endif

	trinfo->count++;
	return 1;
}

static void
reset_trig_zelfera(__attribute__((unused)) struct triggerinfo* trinfo)
{
#if 1
    printf("reset_trig_zelfera\n");
#endif
}

plerrcode
init_trig_zelfera(ems_u32* p, struct triggerinfo* trinfo)
{
        struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
        struct private* priv;
        struct FERA_procs* fera_procs;
        plerrcode pres;

#if 1
        printf("init_trig_zelfera\n");
#endif

        if (p[0]!=1)
            return plErr_ArgNum;

        priv=calloc(1, sizeof(struct private));
        if (!priv) {
            printf("init_trig_zelfera: %s\n", strerror(errno));
            return plErr_NoMem;
        }
        priv->p=-1;
        tinfo->private=priv;

        if ((pres=find_camac_odevice(p[1], &priv->dev))!=plOK) {
            free(priv);
            return pres;
        }
        if (priv->dev->get_FERA_procs(priv->dev)==0) {
            printf("trig_zelfera: device %s does not support FERA\n",
                    priv->dev->pathname);
            free(priv);
            return plErr_Other;
        }
        if ((fera_procs=priv->dev->get_FERA_procs(priv->dev))==0) {
            printf("trig_zelfera: no FERA available\n");
            free(priv);
            return plErr_BadModTyp;
        }
	priv->p = fera_procs->FERA_select_fd(priv->dev);
	if (priv->p < 0) {
            free(priv);
		return plErr_HWTestFailed;
        }

	tinfo->proctype = tproc_select;
	tinfo->i.tp_select.path = priv->p;
	tinfo->i.tp_select.seltype = select_read;
        tinfo->get_trigger=get_trig_zelfera;
        tinfo->reset_trigger=reset_trig_zelfera;
        tinfo->done_trigger=done_trig_zelfera;

	trinfo->count = 0;
	return plOK;
}

char name_trig_zelfera[] = "zelfera";
int ver_trig_zelfera = 1;
