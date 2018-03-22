/*
 * procs/perfspect/perfspect.c
 *
 * created 02.Jul.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: procperfspect.c,v 1.6 2017/10/09 21:25:38 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../procprops.h"
#include "../../lowlevel/perfspect/perfspect.h"

extern ems_u32* outptr;

#define get_device(crate) \
    (struct perf_dev*)get_gendevice(modul_perf, (crate))

RCS_REGISTER(cvsid, "procs/perfspect")

/*****************************************************************************/
char name_proc_perfspect_get[]="perfspect_get";
int ver_proc_perfspect_get=1;
/*---------------------------------------------------------------------------*/
/*
 * p[0]: number of arguments (==4)
 * p[1]: trigger id
 * p[2]: is idx
 * p[3]: procedure
 * p[4]: spectrum (-1: start -2: end; >=0: extra)
 * 
 * output:
 * size of header (4 or 5)
 *   bins
 *   scale        (only if headersize>4)
 *   hits
 *   overflows
 *   leading zeros
 * size of spectrum
 *   data
 */
plerrcode proc_perfspect_get(ems_u32* p)
{
    plerrcode res;

    res=perfspect_get_spect(p[1], p[2], p[3], (int)p[4], &outptr);
    return res;
}

plerrcode test_proc_perfspect_get(ems_u32* p)
{
    if (p[0]!=4) return plErr_ArgNum;
    wirbrauchen=-1;
    return plOK;
}
/*****************************************************************************/
char name_proc_perfspect_readout[]="perfspect_readout";
int ver_proc_perfspect_readout=1;
/*---------------------------------------------------------------------------*/
/*
 * p[0]: number of arguments (==0)
 * 
 */
plerrcode proc_perfspect_readout(ems_u32* p)
{
    plerrcode res;

    res=perfspect_readout_stack(outptr);
    outptr+=res;

    return plOK;
}

plerrcode test_proc_perfspect_readout(ems_u32* p)
{
    if (p[0]!=0) return plErr_ArgNum;
    wirbrauchen=-1;
    return plOK;
}
/*****************************************************************************/
char name_proc_perfspect_info[]="perfspect_info";
int ver_proc_perfspect_info=1;
/*---------------------------------------------------------------------------*/
/*
 * p[0]: number of arguments
if given:
 * nix                 --> list of used triggers
 * p[1]: trigger id    --> list of ISs using this trigger
 * p[2]: is idx        --> list of procedure IDs
 * p[3]: procedure-idx --> number of extraspects
 * p[4]: spectrum      --> name of spectrum
 */
plerrcode proc_perfspect_info(ems_u32* p)
{
    plerrcode res;

    res=perfspect_get_info(p, &outptr);
    return res;
}

plerrcode test_proc_perfspect_info(ems_u32* p)
{
    if (p[0]>4) return plErr_ArgNum;
    wirbrauchen=-1;
    return plOK;
}
/*****************************************************************************/
char name_proc_perfspect_infotree[]="perfspect_infotree";
int ver_proc_perfspect_infotree=1;
/*---------------------------------------------------------------------------*/
/*
 * p[0]: number of arguments (==0)
 * 
 * output:
 * number of triggers
 * for each trigger:
 *     trigger id
 *     number of is
 *     for each is:
 *         is idx
 *         number of procedures
 *         for each procedure:
 *             procidx (index in readoutlist)
 *             procid (from capability-list)
 *             number of extra-spects
 */

plerrcode proc_perfspect_infotree(ems_u32* p)
{
    plerrcode res;

    res=perfspect_infotree(&outptr);
    return res;
}

plerrcode test_proc_perfspect_infotree(ems_u32* p)
{
    if (p[0]!=0) return plErr_ArgNum;
    wirbrauchen=-1;
    return plOK;
}
/*****************************************************************************/
char name_proc_perfspect_clear[]="perfspect_clear";
int ver_proc_perfspect_clear=1;
/*---------------------------------------------------------------------------*/
/*
 * p[0]: number of arguments (==0)
 */
plerrcode proc_perfspect_clear(ems_u32* p)
{
    perfspect_set_setup(-1, -1);
    return plOK;
}

plerrcode test_proc_perfspect_clear(ems_u32* p)
{
    if (p[0]) return plErr_ArgNum;
    wirbrauchen=0;
    return plOK;
}
/*****************************************************************************/
char name_proc_perfspect_setup[]="perfspect_setup";
int ver_proc_perfspect_setup=1;
/*---------------------------------------------------------------------------*/
/*
 * p[0]: number of arguments (0||2)
 * [p[1]: bins
 *  p[2]]: scale (zweierpotenzen)
 */
plerrcode proc_perfspect_setup(ems_u32* p)
{
    int size, scale;

    perfspect_get_setup(&size, &scale);
    *outptr++=size;
    *outptr++=scale;
    if (p[0]>0) {
        return perfspect_set_setup((int)p[1], (int)p[2]);
    } else {
        return plOK;
    }
}

plerrcode test_proc_perfspect_setup(ems_u32* p)
{
    if ((p[0]!=0) && (p[0]!=2)) return plErr_ArgNum;
    wirbrauchen=2;
    return plOK;
}
/*****************************************************************************/
char name_proc_perfspect_unit[]="perfspect_unit";
int ver_proc_perfspect_unit=1;
/*---------------------------------------------------------------------------*/
/*
 * p[0]: number of arguments (==0)
 */
plerrcode proc_perfspect_unit(ems_u32* p)
{
    struct perf_dev* dev=perfspect_get_dev();
    if (dev==0)
        *outptr++=0;
    else
        *outptr++=dev->perf_unit(dev);
    return plOK;
}

plerrcode test_proc_perfspect_unit(ems_u32* p)
{
    if (p[0])
        return plErr_ArgNum;
    wirbrauchen=1;
    return plOK;
}
/*****************************************************************************/
char name_proc_perfspect_setdev[]="perfspect_setdev";
int ver_proc_perfspect_setdev=1;
/*---------------------------------------------------------------------------*/
/*
 * p[0]: number of arguments (==1)
 * p[1]: index of perfspect device (-1: disable)
 */
plerrcode proc_perfspect_setdev(ems_u32* p)
{
    return perfspect_set_dev((ems_i32)p[1]);
}

plerrcode test_proc_perfspect_setdev(ems_u32* p)
{
    if (p[0]!=1)
        return plErr_ArgNum;
    wirbrauchen=0;
    return plOK;
}
/*****************************************************************************/
/*****************************************************************************/
