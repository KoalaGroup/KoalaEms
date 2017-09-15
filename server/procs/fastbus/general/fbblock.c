/*
 * procs/fastbus/general/fbblock.c
 * 
 * created before: 11.08.93
 * 20.01.1999 PW: changed for sfi
 * 04.Jun.2002 PW multi crate support                                            *
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fbblock.c,v 1.12 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../procs.h"
#include "../../procprops.h"
#ifdef OBJ_VAR
#include "../../../objects/var/variables.h"
#endif
#include "../../../objects/domain/dom_ml.h"
#if PERFSPECT
#include "../../../lowlevel/perfspect/perfspect.h"
#endif

extern ems_u32* outptr;
extern int *memberlist;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/fastbus/general")

/*****************************************************************************/
/*
FRCB
p[0] : No. of parameters (==3)
p[1] : module
p[2] : secondary address
p[3] : count
outptr[0] : SS-code
outptr[1] : count
outptr[2] : values
*/

plerrcode proc_FRCB(ems_u32* p)
{
/* WARNING: may be incompatible to CHI (see ../pre/fbblock.c.pre_sfi) */
    ml_entry* module=ModulEnt(p[1]);
    struct fastbus_dev* dev=module->address.fastbus.dev;
    ems_u32 pa=module->address.fastbus.pa;

    outptr[1]=dev->FRCB(dev, pa, p[2], p[3], outptr+2, outptr+0, "proc_FRCB");
    if (outptr[0]) outptr[1]--; /* remove dummy data word in case of error */
    outptr+=2+outptr[1];
    return plOK;
}

plerrcode test_proc_FRCB(ems_u32* p)
{
if (p[0]!=3) return(plErr_ArgNum);
        if (!valid_module(p[1], modul_fastbus, 0)) return plErr_ArgRange;
return(plOK);
}
#ifdef PROCPROPS
static procprop FRCB_prop={1, -1, 0, 0};

procprop* prop_proc_FRCB()
{
return(&FRCB_prop);
}
#endif
char name_proc_FRCB[]="FRCB";
int ver_proc_FRCB=2;

/*****************************************************************************/
/*
FRDB
p[0] : No. of parameters (==3)
p[1] : module
p[2] : secondary address
p[3] : count
outptr[0] : SS-code
outptr[1] : count
outptr[2] : values
*/

plerrcode proc_FRDB(ems_u32* p)
{
/* WARNING: may be incompatible to CHI (see ../pre/fbblock.c.pre_sfi) */
    ml_entry* module=ModulEnt(p[1]);
    struct fastbus_dev* dev=module->address.fastbus.dev;
    ems_u32 pa=module->address.fastbus.pa;

    outptr[1]=dev->FRDB(dev, pa, p[2], p[3], outptr+2, outptr+0, "proc_FRDB");
    if (outptr[0]) outptr[1]--; /* remove dummy data word in case of error */
    outptr+=2+outptr[1];
    return plOK;
}

plerrcode test_proc_FRDB(ems_u32* p)
{
    if (p[0]!=3) return(plErr_ArgNum);
    if (!valid_module(p[1], modul_fastbus, 0)) return plErr_ArgRange;

    wirbrauchen=2+p[3];
#if PERFSPECT
    dev->FRDB_perfspect_needs(dev, &perfbedarf, perfnames);
#endif

    return(plOK);
}
#ifdef PROCPROPS
static procprop FRDB_prop={1, -1, 0, 0};

procprop* prop_proc_FRDB()
{
return(&FRDB_prop);
}
#endif
char name_proc_FRDB[]="FRDB";
int ver_proc_FRDB=2;

/*****************************************************************************/
/*
FRDB_S
p[0] : No. of parameters (==2)
p[1] : module
p[2] : count
outptr[0] : SS-code
outptr[1] : count
outptr[2] : values
*/

plerrcode proc_FRDB_S(ems_u32* p)
{
/* WARNING: may be incompatible to CHI (see ../pre/fbblock.c.pre_sfi) */
    ml_entry* module=ModulEnt(p[1]);
    struct fastbus_dev* dev=module->address.fastbus.dev;
    ems_u32 pa=module->address.fastbus.pa;

    outptr[1]=dev->FRDB_S(dev, pa, p[2], outptr+2, outptr+0, "proc_FRDB_S");
    if (outptr[0]) outptr[1]--; /* remove dummy data word in case of error */
    outptr+=2+outptr[1];
    return(plOK);
}

plerrcode test_proc_FRDB_S(ems_u32* p)
{
if (p[0]!=2) return(plErr_ArgNum);
        if (!valid_module(p[1], modul_fastbus, 0)) return plErr_ArgRange;
wirbrauchen=2+p[2];
return(plOK);
}
#ifdef PROCPROPS
static procprop FRDB_S_prop={1, -1, 0, 0};

procprop* prop_proc_FRDB_S()
{
return(&FRDB_S_prop);
}
#endif
char name_proc_FRDB_S[]="FRDB_S";
int ver_proc_FRDB_S=2;

/*****************************************************************************/
/*
FRDBv
p[0] : No. of parameters (==3)
p[1] : module
p[2] : secondary address
p[3] : Variable for count
outptr[0] : SS-code
outptr[1] : count
outptr[2] : values
*/

plerrcode proc_FRDBv(ems_u32* p)
{
#ifdef OBJ_VAR
    unsigned int count;
    ml_entry* module=ModulEnt(p[1]);
    struct fastbus_dev* dev=module->address.fastbus.dev;
    ems_u32 pa=module->address.fastbus.pa;

    var_read_int(p[3], &count);
    outptr[1]=dev->FRDB(dev, pa, p[2], count, outptr+2, outptr+0, "proc_FRDBv");
    if (outptr[0]) outptr[1]--; /* remove dummy data word in case of error */
    outptr+=2+outptr[1];
#endif
    return(plOK);
}

plerrcode test_proc_FRDBv(ems_u32* p)
{
ems_u32 size;
int res;
if (p[0]!=3) return(plErr_ArgNum);
#ifndef OBJ_VAR
return(plErr_IllVar);
#else
if ((res=var_attrib(p[3], &size))!=plOK) return(res);
if (size!=1) return(plErr_IllVarSize);
        if (!valid_module(p[1], modul_fastbus, 0)) return plErr_ArgRange;
wirbrauchen=-1; /* content of variable(p[3] may change */
return(plOK);
#endif
}
#ifdef PROCPROPS
static procprop FRDBv_prop={1, -1, 0, 0};

procprop* prop_proc_FRDBv()
{
return(&FRDBv_prop);
}
#endif
char name_proc_FRDBv[]="FRDBv";
int ver_proc_FRDBv=1;

/*****************************************************************************/
#ifdef HAVE_FB_BLOCKWRITE
/*
FWCB
p[0] : No. of parameters (>=3)
p[1] : module
p[2] : secondary address
p[3] : count
...  : values
outptr[0] : SS-code
outptr[1] : count
*/

plerrcode proc_FWCB(ems_u32* p)
{
        ml_entry* module=ModulEnt(p[1]);
        struct fastbus_dev* dev=module->address.fastbus.dev;
        int pa=module->address.fastbus.pa;

/*memcpy(FB_BUFBEG, p+4, p[3]*4);*/
#ifdef CHIFASTBUS
#error CPY_TO_FBBUF entfernt
/* outptr[1]=FWCB(p[1], p[2], CPY_TO_FBBUF(0, p+4, p[3]), p[3]); */
#endif
outptr[1]=dev->FWCB(dev, pa, p[2], p[3], p+4, outptr+0);
outptr+=2;
return(plOK);
}

plerrcode test_proc_FWCB(ems_u32* p)
{
if (p[0]<3) return(plErr_ArgNum);
if (p[0]!=(p[3]+3)) return(plErr_ArgNum);
        if (!valid_module(p[1], modul_fastbus, 0)) return plErr_ArgRange;
wirbrauchen=2;
return(plOK);
}
#ifdef PROCPROPS
static procprop FWCB_prop={0, 2, 0, 0};

procprop* prop_proc_FWCB()
{
return(&FWCB_prop);
}
#endif
char name_proc_FWCB[]="FWCB";
int ver_proc_FWCB=2;

/*****************************************************************************/
/*
FWDB
p[0] : No. of parameters (>=3)
p[1] : module
p[2] : secondary address
p[3] : count
...  : values
outptr[0] : SS-code
outptr[1] : count
*/

plerrcode proc_FWDB(ems_u32* p)
{
        ml_entry* module=ModulEnt(p[1]);
        struct fastbus_dev* dev=module->address.fastbus.dev;
        int pa=module->address.fastbus.pa;
#ifdef CHIFASTBUS
#error CPY_TO_FBBUF fehlt?
#endif
outptr[1]=dev->FWDB(dev, pa, p[2], p[3], p+4, outptr+0);
outptr+=2;
return(plOK);
}

plerrcode test_proc_FWDB(ems_u32* p)
{
if (p[0]<3) return(plErr_ArgNum);
if (p[0]!=(p[3]+3)) return(plErr_ArgNum);
        if (!valid_module(p[1], modul_fastbus, 0)) return plErr_ArgRange;
wirbrauchen=2;
return(plOK);
}
#ifdef PROCPROPS
static procprop FWDB_prop={0, 2, 0, 0};

procprop* prop_proc_FWDB()
{
return(&FWDB_prop);
}
#endif
char name_proc_FWDB[]="FWDB";
int ver_proc_FWDB=2;

#endif /* HAVE_FB_BLOCKWRITE*/
/*****************************************************************************/
/*
simulated_FRDB
p[0] : No. of parameters (==3)
p[1] : module
p[2] : secondary address
p[3] : count
outptr[0] : SS-code
outptr[1] : count
outptr[2] : values
*/

plerrcode proc_simulated_FRDB(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct fastbus_dev* dev=module->address.fastbus.dev;
    ems_u32 pa=module->address.fastbus.pa, val, ss, *help;
    int res, num=0;

    help=outptr;
    outptr+=2;
    do {
        res=dev->FRD(dev, pa, p[2], &val, &ss);
        if (res)
            return plErr_HW;
        if (!ss) {
            *outptr++=val;
            num++;
        }
    } while (ss==0 && num<=p[3]);
    help[0]=ss;
    help[1]=num;
    return plOK;
}

plerrcode test_proc_simulated_FRDB(ems_u32* p)
{
    if (p[0]!=3)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_fastbus, 0))
        return plErr_ArgRange;

    wirbrauchen=p[3]+2;
    return plOK;
}

char name_proc_simulated_FRDB[]="simulated_FRDB";
int ver_proc_simulated_FRDB=1;
/*****************************************************************************/
/*****************************************************************************/
