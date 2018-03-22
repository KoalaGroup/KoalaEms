/*
 * procs/fastbus/general/fbbroadcast.c
 * created before: 11.08.93
 * 22.May.2002 PW rewritten for multiple crates
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fbbroadcast.c,v 1.8 2017/10/09 21:25:37 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../procprops.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/fastbus/fastbus.h"

extern ems_u32* outptr;
extern int* memberlist;

RCS_REGISTER(cvsid, "procs/fastbus/general")

/*****************************************************************************/
/*
FRCM
p[0] : No. of parameters (==2)
p[1] : (dummy-)module; selects crate
p[2] : broadcast address
outptr[0] : SS-code
outptr[1] : val
*/

plerrcode proc_FRCM(ems_u32* p)
{
        int res;
        ml_entry* module=ModulEnt(p[1]);
        struct fastbus_dev* dev=module->address.fastbus.dev;

        res=dev->FRCM(dev, p[2], outptr+1, outptr);
        if (!res) {
            outptr+=2;
            return plOK;
        } else {
            *outptr++=res;
            return plErr_System;
        }
}

plerrcode test_proc_FRCM(ems_u32* p)
{
        if (p[0]!=2) return plErr_ArgNum;
        if (!valid_module(p[1], modul_fastbus)) return plErr_ArgRange;
        wirbrauchen=2;
        return plOK;
}
#ifdef PROCPROPS
static procprop FRCM_prop={0, 2, 0, 0};

procprop* prop_proc_FRCM()
{
return(&FRCM_prop);
}
#endif
char name_proc_FRCM[]="FRCM";
int ver_proc_FRCM=3;

/*****************************************************************************/
/*
FRDM
p[0] : No. of parameters (==2)
p[1] : (dummy-)module; selects crate
p[2] : broadcast address
outptr[0] : SS-code
outptr[1] : val
*/

plerrcode proc_FRDM(ems_u32* p)
{
        int res;
        ml_entry* module=ModulEnt(p[1]);
        struct fastbus_dev* dev=module->address.fastbus.dev;

        res=dev->FRDM(dev, p[2], outptr+1, outptr);
        if (!res) {
            outptr+=2;
            return plOK;
        } else {
            *outptr++=res;
            return plErr_System;
        }
}

plerrcode test_proc_FRDM(ems_u32* p)
{
        if (p[0]!=2) return plErr_ArgNum;
        if (!valid_module(p[1], modul_fastbus)) return plErr_ArgRange;
        wirbrauchen=2;
        return plOK;
}
#ifdef PROCPROPS
static procprop FRDM_prop={0, 2, 0, 0};

procprop* prop_proc_FRDM()
{
return(&FRDM_prop);
}
#endif
char name_proc_FRDM[]="FRDM";
int ver_proc_FRDM=3;
/*****************************************************************************/
/*
FWCM
p[0] : No. of parameters (==3)
p[1] : (dummy-)module; selects crate
p[2] : broadcast-address
p[3] : value
outptr[0] : SS-code
*/
plerrcode proc_FWCM(ems_u32* p)
{
        int res;
        ml_entry* module=ModulEnt(p[1]);
        struct fastbus_dev* dev=module->address.fastbus.dev;

        res=dev->FWCM(dev, p[2], p[3], outptr++);
        if (!res) {
            return plOK;
        } else {
            *(outptr-1)=res;
            return plErr_System;
        }
}

plerrcode test_proc_FWCM(ems_u32* p)
{
        if (p[0]!=3) return plErr_ArgNum;
        if (!valid_module(p[1], modul_fastbus)) return plErr_ArgRange;
        wirbrauchen=1;
        return plOK;
}

char name_proc_FWCM[]="FWCM";
int ver_proc_FWCM=3;

/*****************************************************************************/
/*
FWDM
p[0] : No. of parameters (==3)
p[1] : (dummy-)module; selects crate
p[2] : broadcast-address
p[3] : value
outptr[0] : SS-code
*/
plerrcode proc_FWDM(ems_u32* p)
{
        int res;
        ml_entry* module=ModulEnt(p[1]);
        struct fastbus_dev* dev=module->address.fastbus.dev;

        res=dev->FWDM(dev, p[2], p[3], outptr++);
        if (!res) {
            return plOK;
        } else {
            *(outptr-1)=res;
            return plErr_System;
        }
}

plerrcode test_proc_FWDM(ems_u32* p)
{
        if (p[0]!=3) return plErr_ArgNum;
        if (!valid_module(p[1], modul_fastbus)) return plErr_ArgRange;
        wirbrauchen=1;
        return plOK;
}

char name_proc_FWDM[]="FWDM";
int ver_proc_FWDM=3;
//*****************************************************************************/
/*****************************************************************************/
