/*
 * lowlevel/camac/pcicamac/lam.c
 * 21.Jan.2001 PW: multicrate support
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lam.c,v 1.5 2011/04/06 20:30:22 wuestner Exp $: lam.c,v 1.4 2002/10/06 21:57:14 wuestner Exp ";

#include <errorcodes.h>
#include <rcs_ids.h>

#include "../camac.h"
#include "../../get_lam.h"

RCS_REGISTER(cvsid, "lowlevel/camac/pcicamac")

int zel_get_lam(struct camac_dev* dev)
{
        struct camac_pci_info* info=&dev->info.pcicamac;
	int lamstat = dev->CAMAClams(dev) & info->lammask;
	return(lamstat);
}

static int get_lam_mask(int idx)
{
	return(1<<(idx-1));
}

void zel_enable_lam(struct camac_dev* dev, int idx, int argnum, int* arg)
{
	dev->info.pcicamac.lammask |= get_lam_mask(idx);
}

void zel_disable_lam(struct camac_dev* dev, int idx)
{
	dev->info.pcicamac.lammask &= ~get_lam_mask(idx);
}
