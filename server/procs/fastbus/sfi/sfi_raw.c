/*
 * procs/fastbus/sfi/sfi_raw.c
 * 
 * created: 09.11.97
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sfi_raw.c,v 1.7 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <stdio.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../procs.h"
#include "../../procprops.h"

extern ems_u32* outptr;
extern int wirbrauchen;

#define get_device(crate) \
    (struct fastbus_dev*)get_gendevice(modul_fastbus, (crate))

RCS_REGISTER(cvsid, "procs/fastbus/sfi")

/*****************************************************************************/
/*
SFIreset
p[0] : No. of parameters (==1)
p[1] : crate
*/

plerrcode proc_SFIreset(ems_u32* p)
{
    struct fastbus_dev* dev=get_device(p[1]);
    dev->reset(dev);
    return plOK;
}

plerrcode test_proc_SFIreset(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}
#ifdef PROCPROPS
static procprop SFIreset_prop={0, 0, 0, 0};

procprop* prop_proc_SFIreset()
{
return(&SFIreset_prop);
}
#endif
char name_proc_SFIreset[]="SFIreset";
int ver_proc_SFIreset=1;

/*****************************************************************************/
/*
SFIreset
p[0] : No. of parameters (==1)
p[1] : crate
*/

plerrcode proc_SFIseqrest(ems_u32* p)
{
    struct fastbus_dev* dev=get_device(p[1]);
    dev->restart_sequencer(dev);
    return plOK;
}

plerrcode test_proc_SFIseqrest(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}
#ifdef PROCPROPS
static procprop SFIseqrest_prop={0, 0, 0, 0};

procprop* prop_proc_SFIseqrest()
{
return(&SFIseqrest_prop);
}
#endif
char name_proc_SFIseqrest[]="SFIseqreset";
int ver_proc_SFIseqrest=1;

/*****************************************************************************/
/*
SFIreset
p[0] : No. of parameters (==1)
p[1] : crate
*/

plerrcode proc_SFIstatus(ems_u32* p)
{
    struct fastbus_dev* dev=get_device(p[1]);
    int res;

    res=dev->status(dev, outptr, 7);
    if (res<0) {
        *outptr++=res;
        return plErr_Program;
    }
    outptr+=res;
/*
 *  the values for SFI/NGF are:
 *  status;
 *  fb1;
 *  fb2;
 *  //ss;
 *  //pa;
 *  //cmd;
 *  //seqaddr;
 */
return plOK;
}

plerrcode test_proc_SFIstatus(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1) return(plErr_ArgNum);
    if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=8;
    return plOK;
}
#ifdef PROCPROPS
static procprop SFIstatus_prop={0, 0, 0, 0};

procprop* prop_proc_SFIstatus()
{
return(&SFIstatus_prop);
}
#endif
char name_proc_SFIstatus[]="SFIstatus";
int ver_proc_SFIstatus=1;

/*****************************************************************************/
#if 0
plerrcode proc_SFIW(ems_u32* p)
{
*(volatile int*)(sfi.base+p[1])=p[2];
return plOK;
}

plerrcode test_proc_SFIW(ems_u32* p)
{
if (p[0]!=2) return(plErr_ArgNum);
return plOK;
}
#ifdef PROCPROPS
static procprop SFIW_prop={0, 0, 0, 0};

procprop* prop_proc_SFIW()
{
return(&SFIW_prop);
}
#endif
char name_proc_SFIW[]="SFIW";
int ver_proc_SFIW=1;

/*****************************************************************************/

plerrcode proc_SFIR(ems_u32* p)
{
*outptr++=*(volatile int*)(sfi.base+p[1]);
return plOK;
}

plerrcode test_proc_SFIR(ems_u32* p)
{
if (p[0]!=1) return(plErr_ArgNum);
return plOK;
}
#ifdef PROCPROPS
static procprop SFIR_prop={0, 0, 0, 0};

procprop* prop_proc_SFIR()
{
return(&SFIR_prop);
}
#endif
char name_proc_SFIR[]="SFIR";
int ver_proc_SFIR=1;
#endif
/*****************************************************************************/
/*****************************************************************************/
