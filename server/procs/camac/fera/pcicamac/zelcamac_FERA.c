/*
 * procs/camac/fera/pcicamac/zelcamac_FERA.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: zelcamac_FERA.c,v 1.14 2017/10/20 23:20:52 wuestner Exp $";

#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../../../lowlevel/camac/camac.h"
#include "../../../../objects/domain/dom_ml.h"
#include "../../../procs.h"
#include "../fera.h"

RCS_REGISTER(cvsid, "procs/camac/fera/pcicamac")

/*****************************************************************************/
/*
 * p[0]: # args (>=4)
 * p[1]: crate
 * p[2]: timeout
 * p[3]: reqdelay
 * p[4]: sgl
 * p[5]: data
 */
plerrcode proc_FERAsetupZEL(ems_u32* p)
{
	int vsn;
	unsigned int i;
        struct camac_dev* dev=get_camac_device(p[1]);
        ems_u32* data;
        struct FERA_procs* FERA_procs=dev->get_FERA_procs(dev);

        FERA_procs->FERAdisable(dev);

	/* Setup der ADC-Module */
	vsn = 1;
	data = &p[5];
	for(i = 0; i < memberlist[0]; i++)
		data = SetupFERAmodul(ModulEnt(i + 1), data, &vsn);

	if (FERA_procs->FERAenable(dev, p[2], p[3], p[4]))
	    return(plErr_System);

	return plOK;
}

plerrcode test_proc_FERAsetupZEL(ems_u32* p)
{
        struct camac_dev* dev;
	unsigned int i;
        plerrcode pres;
	ems_u32* data = p+5;

	if (p[0]<4)
            return plErr_ArgNum;

        if ((pres=_find_camac_odevice(p[1], &dev))!=plOK)
            return pres;
        if (dev->get_FERA_procs(dev)==0) {
            printf("FERAsetupZEL: device %s does not support FERA\n",
                    dev->pathname);
            return plErr_Other;
        }

	if (!memberlist)
		return(plErr_NoISModulList);
        /*
        empty modullist is not an error
	if (memberlist[0] < 1)
		return(plErr_BadISModList);
        */
	for (i = 1; i <= memberlist[0]; i++) {
		plerrcode res;

		res = test_SetupFERA(ModulEnt(i), &data);
		if (res) {
			*outptr++= i;
			return(res);
		}
	}
	if (data != &p[p[0] + 1])
		return(plErr_ArgNum);
	return plOK;
}

char name_proc_FERAsetupZEL[] = "FERAsetupZEL";

int ver_proc_FERAsetupZEL = 1;
/*****************************************************************************/
/*
 * p[0]: # args (==2)
 * p[1]: crate
 * p[2]: dmasize/KByte
 */
plerrcode proc_FERAdmasize(ems_u32* p)
{
        struct camac_dev* dev=get_camac_device(p[1]);
        struct FERA_procs* FERA_procs=dev->get_FERA_procs(dev);

	if (FERA_procs->FERAdmasize(dev, p[2]*1024)<0)
	    return plErr_System;

	return plOK;
}

plerrcode test_proc_FERAdmasize(ems_u32* p)
{
        struct camac_dev* dev;
        plerrcode pres;

	if (p[0]!=2)
            return plErr_ArgNum;

        if ((pres=_find_camac_odevice(p[1], &dev))!=plOK)
            return pres;
        if (dev->get_FERA_procs(dev)==0) {
            printf("FERAdmasize: device %s does not support FERA\n",
                    dev->pathname);
            return plErr_Other;
        }

	return plOK;
}

char name_proc_FERAdmasize[] = "FERAdmasize";

int ver_proc_FERAdmasize = 1;
/*****************************************************************************/
/*
 * p[0]: # args (==1)
 * p[1]: crate
 */
plerrcode proc_FERAreadoutZEL(ems_u32* p)
{
        struct camac_dev* dev=get_camac_device(p[1]);
        struct FERA_procs* FERA_procs=dev->get_FERA_procs(dev);

	if (FERA_procs->FERAreadout(dev, &outptr)) {
		FERA_procs->FERAstatus(dev, &outptr);
      		return plErr_HW;
	}
	return plOK;
}

plerrcode test_proc_FERAreadoutZEL(ems_u32* p)
{
        struct camac_dev* dev;
        plerrcode pres;

	if (p[0]!=1)
            return plErr_ArgNum;

        if ((pres=_find_camac_odevice(p[1], &dev))!=plOK)
            return pres;
        if (dev->get_FERA_procs(dev)==0) {
            printf("FERAreadoutZEL: device %s does not support FERA\n",
                    dev->pathname);
            return plErr_Other;
        }

	return plOK;
}

char name_proc_FERAreadoutZEL[] = "FERAreadoutZEL";
int ver_proc_FERAreadoutZEL = 1;
/*****************************************************************************/
/*
 * p[0]: # args (==1)
 * p[1]: crate
 */
plerrcode proc_FERAgateZEL(ems_u32* p)
{
        struct camac_dev* dev=get_camac_device(p[1]);
        struct FERA_procs* FERA_procs=dev->get_FERA_procs(dev);

	FERA_procs->FERAgate(dev);
	return plOK;
}

plerrcode test_proc_FERAgateZEL(ems_u32* p)
{
        struct camac_dev* dev;
        plerrcode pres;

	if (p[0]!=1)
            return plErr_ArgNum;

        if ((pres=_find_camac_odevice(p[1], &dev))!=plOK)
            return pres;
        if (dev->get_FERA_procs(dev)==0) {
            printf("FERAgateZEL: device %s does not support FERA\n",
                    dev->pathname);
            return plErr_Other;
        }

	return plOK;
}
char name_proc_FERAgateZEL[] = "FERAgateZEL";
int ver_proc_FERAgateZEL = 1;
/*****************************************************************************/
/*
 * p[0]: # args (==1)
 * p[1]: crate
 */
plerrcode proc_FERAstatusZEL(ems_u32* p)
{
        struct camac_dev* dev=get_camac_device(p[1]);
        struct FERA_procs* FERA_procs=dev->get_FERA_procs(dev);

	if (FERA_procs->FERAstatus(dev, &outptr))
		return(plErr_HW);
	return plOK;
}

plerrcode test_proc_FERAstatusZEL(ems_u32* p)
{
        struct camac_dev* dev;
        plerrcode pres;

	if (p[0]!=1)
            return plErr_ArgNum;

        if ((pres=_find_camac_odevice(p[1], &dev))!=plOK)
            return pres;
        if (dev->get_FERA_procs(dev)==0) {
            printf("FERAstatusZEL: device %s does not support FERA\n",
                    dev->pathname);
            return plErr_Other;
        }

	return plOK;
}
char name_proc_FERAstatusZEL[] = "FERAstatusZEL";
int ver_proc_FERAstatusZEL = 1;
/*****************************************************************************/
/*****************************************************************************/
