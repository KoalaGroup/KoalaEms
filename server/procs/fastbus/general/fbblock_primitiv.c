/*
 * procs/fastbus/general/fbblock_primitiv.c
 * 
 * created 22.May.2002 PW
 * 05.Nov.2002 PW: word counter fixed
 *
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fbblock_primitiv.c,v 1.11 2017/10/09 21:25:37 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../../lowlevel/devices.h"
#include "../../procs.h"
#include "../../procprops.h"
#if PERFSPECT
#include "../../../lowlevel/perfspect/perfspect.h"
#endif

extern ems_u32* outptr;
extern int* memberlist;

#define get_device(class, crate) \
    (struct fastbus_dev*)get_gendevice((class), (crate))

RCS_REGISTER(cvsid, "procs/fastbus/general")

/*****************************************************************************/
/*
frcb
p[0] : No. of parameters (==4)
p[1] : crate
p[2] : primary address
p[3] : secondary address
p[4] : count
outptr[0] : SS-code
outptr[1] : count
outptr[2] : values
*/

plerrcode proc_frcb(ems_u32* p)
{
        int res;
        struct fastbus_dev* dev=get_device(modul_fastbus, p[1]);

        res=dev->FRCB(dev, p[2], p[3], p[4], outptr+2, outptr/*sscode*/, "proc_frcb");
        if (res>=0) {
            if (*outptr) res--; /* FRDB not stopped by limit counter */
            *(outptr+1)=res;
            outptr+=2+res;
            return plOK;
        } else {
            *outptr++=res;
            return plErr_System;
        }
}

plerrcode test_proc_frcb(ems_u32* p)
{
        plerrcode pres;

        if (p[0]!=4)
            return plErr_ArgNum;
        if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK)
            return pres;
        wirbrauchen=2+p[4];
        return plOK;
}
#ifdef PROCPROPS
static procprop frcb_prop={1, -1, 0, 0};

procprop* prop_proc_frcb()
{
        return(&frcb_prop);
}
#endif
char name_proc_frcb[]="frcb";
int ver_proc_frcb=1;
/*****************************************************************************/
/*
frdb
p[0] : No. of parameters (==4)
p[1] : crate
p[2] : primary address
p[3] : secondary address
p[4] : count
outptr[0] : SS-code
outptr[1] : count
outptr[2] : values
*/

plerrcode proc_frdb(ems_u32* p)
{
        int res;
        struct fastbus_dev* dev=get_device(modul_fastbus, p[1]);

        res=dev->FRDB(dev, p[2], p[3], p[4], outptr+2, outptr/*sscode*/, "proc_frdb");
        if (res>=0) {
            if (*outptr) res--; /* FRDB not stopped by limit counter */
            *(outptr+1)=res;
            outptr+=2+res;
            return plOK;
        } else {
            *outptr++=res;
            return plErr_System;
        }
}

plerrcode test_proc_frdb(ems_u32* p)
{
        struct fastbus_dev* dev;
        plerrcode pres;

        if (p[0]!=4)
            return plErr_ArgNum;
        if ((pres=find_odevice(modul_fastbus, p[1], (struct generic_dev**)&dev))!=plOK)
            return pres;
        wirbrauchen=2+p[4];
#if PERFSPECT
        dev->FRDB_S_perfspect_needs(dev, &perfbedarf, perfnames);
#endif
        return plOK;
}
#ifdef PROCPROPS
static procprop frdb_prop={1, -1, 0, 0};

procprop* prop_proc_frdb()
{
        return(&frdb_prop);
}
#endif
char name_proc_frdb[]="frdb";
int ver_proc_frdb=1;
/*****************************************************************************/
/*
frdb_s
p[0] : No. of parameters (==4)
p[1] : crate
p[2] : primary address
p[3] : count
outptr[0] : SS-code
outptr[1] : count
outptr[2] : values
*/

plerrcode proc_frdb_s(ems_u32* p)
{
        int res;
        struct fastbus_dev* dev=get_device(modul_fastbus, p[1]);

        res=dev->FRDB_S(dev, p[2], p[3], outptr+2, outptr/*sscode*/, "proc_frdb_s");
        if (res>=0) {
            if (*outptr) res--; /* FRDB not stopped by limit counter */
            *(outptr+1)=res;
            outptr+=2+res;
            return plOK;
        } else {
            *outptr++=res;
            return plErr_System;
        }
}

plerrcode test_proc_frdb_s(ems_u32* p)
{
        plerrcode pres;
        if (p[0]!=3) return plErr_ArgNum;
        if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK)
            return pres;
        wirbrauchen=2+p[3];
        return plOK;
}
#ifdef PROCPROPS
static procprop frdb_s_prop={1, -1, 0, 0};

procprop* prop_proc_frdb_s()
{
        return(&frdb_s_prop);
}
#endif
char name_proc_frdb_s[]="frdb_s";
int ver_proc_frdb_s=1;

/*****************************************************************************/
/*****************************************************************************/
