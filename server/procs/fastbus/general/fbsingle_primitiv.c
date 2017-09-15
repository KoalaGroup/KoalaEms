/*
 * procs/fastbus/general/fbsingle_primitiv.c
 * created 22.May.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fbsingle_primitiv.c,v 1.6 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../procprops.h"
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../../lowlevel/devices.h"

extern ems_u32* outptr;
extern int wirbrauchen;

#define get_device(class, crate) \
    (struct fastbus_dev*)get_gendevice((class), (crate))

RCS_REGISTER(cvsid, "procs/fastbus/general")

/*****************************************************************************/
/*
frc
p[0] : No. of parameters (==3)
p[1] : crate
p[2] : primary address
p[3] : secondary address
outptr[0] : SS-code
outptr[1] : val
*/

plerrcode proc_frc(ems_u32* p)
{
        int res;
        struct fastbus_dev* dev=get_device(modul_fastbus, p[1]);

        res=dev->FRC(dev, p[2], p[3], outptr+1/*data read*/, outptr/*sscode*/);
        if (!res) {
            outptr+=2;
            return plOK;
        } else {
            *outptr++=res;
            return plErr_System;
        }
}

plerrcode test_proc_frc(ems_u32* p)
{
        plerrcode pres;

        if (p[0]!=3)
            return plErr_ArgNum;
        if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK)
            return pres;
        wirbrauchen=2;
        return plOK;
}
#ifdef PROCPROPS
static procprop frc_prop={0, 3, 0, 0};

procprop* prop_proc_frc()
{
        return(&frc_prop);
}
#endif
char name_proc_frc[]="frc";
int ver_proc_frc=1;

/*****************************************************************************/
/*
fwc
p[0] : No. of parameters (==4)
p[1] : crate
p[2] : primary address
p[3] : secondary address
p[4] : value
outptr[0] : SS-code
*/
plerrcode proc_fwc(ems_u32* p)
{
        int res;
        struct fastbus_dev* dev=get_device(modul_fastbus, p[1]);

        res=dev->FWC(dev, p[2], p[3], p[4], outptr++/*sscode*/);
        if (!res) {
            return plOK;
        } else {
            *(outptr-1)=res;
            return plErr_System;
        }
}

plerrcode test_proc_fwc(ems_u32* p)
{
        plerrcode pres;

        if (p[0]!=4)
            return plErr_ArgNum;
        if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK)
            return pres;
        wirbrauchen=1;
        return plOK;
}
#ifdef PROCPROPS
static procprop fwc_prop={0, 1, 0, 0};

procprop* prop_proc_fwc()
{
return(&fwc_prop);
}
#endif
char name_proc_fwc[]="fwc";
int ver_proc_fwc=1;

/*****************************************************************************/
/*
frd
p[0] : No. of parameters (==3)
p[1] : crate
p[2] : primary address
p[3] : secondary address
outptr[0] : SS-code
outptr[1] : val
*/

plerrcode proc_frd(ems_u32* p)
{
        int res;
        struct fastbus_dev* dev=get_device(modul_fastbus, p[1]);

        res=dev->FRD(dev, p[2], p[3], outptr+1/*data read*/, outptr/*sscode*/);
        if (!res) {
            outptr+=2;
            return plOK;
        } else {
            *outptr++=res;
            return plErr_System;
        }
}

plerrcode test_proc_frd(ems_u32* p)
{
        plerrcode pres;

        if (p[0]!=3)
            return plErr_ArgNum;
        if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK)
            return pres;
        wirbrauchen=2;
        return plOK;
}
#ifdef PROCPROPS
static procprop frd_prop={0, 3, 0, 0};

procprop* prop_proc_frd()
{
        return(&frd_prop);
}
#endif
char name_proc_frd[]="frd";
int ver_proc_frd=1;

/*****************************************************************************/
/*
fwc
p[0] : No. of parameters (==4)
p[1] : crate
p[2] : primary address
p[3] : secondary address
p[4] : value
outptr[0] : SS-code
*/
plerrcode proc_fwd(ems_u32* p)
{
        int res;
        struct fastbus_dev* dev=get_device(modul_fastbus, p[1]);

        res=dev->FWD(dev, p[2], p[3], p[4], outptr++/*sscode*/);
        if (!res) {
            return plOK;
        } else {
            *(outptr-1)=res;
            return plErr_System;
        }
}

plerrcode test_proc_fwd(ems_u32* p)
{
        plerrcode pres;

        if (p[0]!=4)
            return plErr_ArgNum;
        if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK)
            return pres;
        wirbrauchen=1;
        return plOK;
}
#ifdef PROCPROPS
static procprop fwd_prop={0, 1, 0, 0};

procprop* prop_proc_fwd()
{
return(&fwd_prop);
}
#endif
char name_proc_fwd[]="fwd";
int ver_proc_fwd=1;

/*****************************************************************************/
/*****************************************************************************/
