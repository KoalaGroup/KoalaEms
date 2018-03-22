/*
 * procs/fastbus/general/fbsingle.c
 * created before: 11.08.94
 * 22.May.2002 PW rewritten for multiple crates
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fbsingle.c,v 1.8 2017/10/09 21:25:37 wuestner Exp $";

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
FRC
p[0] : No. of parameters (==2)
p[1] : module
p[2] : secondary address
outptr[0] : SS-code
outptr[1] : val
*/

plerrcode proc_FRC(ems_u32* p)
{
        int res;
        ml_entry* module=ModulEnt(p[1]);
        struct fastbus_dev* dev=module->address.fastbus.dev;

        res=dev->FRC(dev, module->address.fastbus.pa, p[2], outptr+1, outptr);
        if (!res) {
            outptr+=2;
            return plOK;
        } else {
            *outptr++=res;
            return plErr_System;
        }
}

plerrcode test_proc_FRC(ems_u32* p)
{
        if (p[0]!=2) return plErr_ArgNum;
        if (!valid_module(p[1], modul_fastbus)) return plErr_ArgRange;
        wirbrauchen=2;
        return plOK;
}
#ifdef PROCPROPS
static procprop FRC_prop={0, 2, 0, 0};

procprop* prop_proc_FRC()
{
return(&FRC_prop);
}
#endif
char name_proc_FRC[]="FRC";
int ver_proc_FRC=3;

/*****************************************************************************/
/*
FWC
p[0] : No. of parameters (==3)
p[1] : module
p[2] : secondary address
p[3] : value
outptr[0] : SS-code
*/
plerrcode proc_FWC(ems_u32* p)
{
        int res;
        ml_entry* module=ModulEnt(p[1]);
        struct fastbus_dev* dev=module->address.fastbus.dev;

        res=dev->FWC(dev, module->address.fastbus.pa, p[2], p[3], outptr++);
        if (!res) {
            return plOK;
        } else {
            *(outptr-1)=res;
            return plErr_System;
        }
}

plerrcode test_proc_FWC(ems_u32* p)
{
        if (p[0]!=3) return plErr_ArgNum;
        if (!valid_module(p[1], modul_fastbus)) return plErr_ArgRange;
        wirbrauchen=1;
        return plOK;
}
#ifdef PROCPROPS
static procprop FWC_prop={0, 1, 0, 0};

procprop* prop_proc_FWC()
{
return(&FWC_prop);
}
#endif
char name_proc_FWC[]="FWC";
int ver_proc_FWC=3;

/*****************************************************************************/
/*
FRD
p[0] : No. of parameters (==2)
p[1] : module
p[2] : secondary address
outptr[0] : SS-code
outptr[1] : val
*/

plerrcode proc_FRD(ems_u32* p)
{
        int res;
        ml_entry* module=ModulEnt(p[1]);
        struct fastbus_dev* dev=module->address.fastbus.dev;

        res=dev->FRD(dev, module->address.fastbus.pa, p[2], outptr+1, outptr);
        if (!res) {
            outptr+=2;
            return plOK;
        } else {
            *outptr++=res;
            return plErr_System;
        }
}

plerrcode test_proc_FRD(ems_u32* p)
{
        if (p[0]!=2) return plErr_ArgNum;
        if (!valid_module(p[1], modul_fastbus)) return plErr_ArgRange;
        wirbrauchen=2;
        return plOK;
}
#ifdef PROCPROPS
static procprop FRD_prop={0, 2, 0, 0};

procprop* prop_proc_FRD()
{
return(&FRD_prop);
}
#endif
char name_proc_FRD[]="FRD";
int ver_proc_FRD=3;

/*****************************************************************************/
/*
FWD
p[0] : No. of parameters (==3)
p[1] : module
p[2] : secondary address
p[3] : value
outptr[0] : SS-code
*/
plerrcode proc_FWD(ems_u32* p)
{
        int res;
        ml_entry* module=ModulEnt(p[1]);
        struct fastbus_dev* dev=module->address.fastbus.dev;

        res=dev->FWD(dev, module->address.fastbus.pa, p[2], p[3], outptr++);
        if (!res) {
            return plOK;
        } else {
            *(outptr-1)=res;
            return plErr_System;
        }
}

plerrcode test_proc_FWD(ems_u32* p)
{
        if (p[0]!=3) return plErr_ArgNum;
        if (!valid_module(p[1], modul_fastbus)) return plErr_ArgRange;
        wirbrauchen=1;
        return plOK;
}
#ifdef PROCPROPS
static procprop FWD_prop={0, 1, 0, 0};

procprop* prop_proc_FWD()
{
return(&FWD_prop);
}
#endif
char name_proc_FWD[]="FWD";
int ver_proc_FWD=3;

/*****************************************************************************/
/*****************************************************************************/
