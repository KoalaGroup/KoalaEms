/*
 * procs/fastbus/sfi/sfi_inout.c
 * 
 * created: 14.05.97
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sfi_inout.c,v 1.9 2017/10/09 21:25:37 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../procs.h"
#include "../../procprops.h"

extern ems_u32* outptr;

#define get_device(crate) \
    (struct fastbus_dev*)get_gendevice(modul_fastbus, (crate))

RCS_REGISTER(cvsid, "procs/fastbus/sfi")

/*****************************************************************************/
/*
SFIled
p[0] : No. of parameters (==1)
p[1] : crate
p[2] : int LED-bits
*/

plerrcode proc_SFIled(ems_u32* p)
{
    struct fastbus_dev* dev=get_device(p[1]);

    dev->LEVELOUT(dev, 0, p[2], (~p[2])&0xf);
    return plOK;
}

plerrcode test_proc_SFIled(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK)
        return pres;
    if (p[2]&~0xf)
        return plErr_ArgRange;
    wirbrauchen=0;
    return plOK;
}
#ifdef PROCPROPS
static procprop SFIled_prop={0, 0, 0, 0};

procprop* prop_proc_SFIled()
{
    return(&SFIled_prop);
}
#endif
char name_proc_SFIled[]="SFIled";
int ver_proc_SFIled=1;

/*****************************************************************************/
/*
SFIout
p[0] : No. of parameters (==2)
p[1] : crate
p[2] : int set IO-bits
p[3] : int clear IO-bits
*/

plerrcode proc_SFIout(ems_u32* p)
{
    struct fastbus_dev* dev=get_device(p[1]);

    dev->LEVELOUT(dev, 0, p[2], p[3]);
    return plOK;
}

plerrcode test_proc_SFIout(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK)
        return pres;
    if ((p[2]|p[3])&~0xffff)
        return plErr_ArgRange;
    wirbrauchen=0;
    return plOK;
}
#ifdef PROCPROPS
static procprop SFIout_prop={0, 0, 0, 0};

procprop* prop_proc_SFIout()
{
    return(&SFIout_prop);
}
#endif
char name_proc_SFIout[]="SFIout";
int ver_proc_SFIout=2;

/*****************************************************************************/
/*
SFIin
p[0] : No. of parameters (==0)
p[1] : crate
*/

plerrcode proc_SFIin(ems_u32* p)
{
    struct fastbus_dev* dev=get_device(p[1]);

    dev->LEVELIN(dev, 0, outptr);
    outptr++;
    return plOK;
}

plerrcode test_proc_SFIin(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK)
        return pres;
    return plOK;
}
#ifdef PROCPROPS
static procprop SFIin_prop={0, 1, 0, 0};

procprop* prop_proc_SFIin()
{
    wirbrauchen=1;
    return(&SFIin_prop);
}
#endif
char name_proc_SFIin[]="SFIin";
int ver_proc_SFIin=1;

/*****************************************************************************/
/*****************************************************************************/
