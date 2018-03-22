/*
 * procs/pci/trigger/force_dt.c
 * 
 * created: 13.Jul.2001 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: force_dt.c,v 1.10 2017/10/21 21:59:18 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <errorcodes.h>
#include <sys/types.h>
#include <rcs_ids.h>
#include "../../../lowlevel/sync/pci_zel/sync_pci_zel.h"
#include "../../procs.h"
#ifdef OBJ_VAR
#include "../../../objects/var/variables.h"
#endif

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/sync/zelsync")

/*****************************************************************************/
/*
 * [0]: num (==2)
 * [1]: id
 * [2]: delay (/100ns)
 */
plerrcode proc_force_dt(ems_u32* p)
{
volatile unsigned int counter;
while ((counter=syncmod_read(p[1], 4/*SYNC_TMC*/))<p[2]);
*outptr++=counter;
return plOK;
}

plerrcode test_proc_force_dt(ems_u32* p)
{
if (p[0]!=2) return plErr_ArgNum;
if (syncmod_valid_id(p[1])<0) return plErr_ArgRange;
if ((unsigned int)p[2]>0x7fffff/*23 bit*/) return plErr_ArgRange;
wirbrauchen=1;
return plOK;
}

char name_proc_force_dt[]="ForceDT";
int ver_proc_force_dt=1;
/*****************************************************************************/
/*
 * [0]: num (==2)
 * [1]: id
 * [2]: variable containing delay (/100ns)
 */
plerrcode proc_force_dt_var(ems_u32* p)
{
#ifdef OBJ_VAR
volatile unsigned int counter;
while ((counter=syncmod_read(p[1], 4/*SYNC_TMC*/))<p[2]);
*outptr++=counter;
#endif
return plOK;
}

plerrcode test_proc_force_dt_var(ems_u32* p)
{
if (p[0]!=2) return plErr_ArgNum;
if (syncmod_valid_id(p[1])<0) return plErr_ArgRange;
#ifndef OBJ_VAR
return plErr_IllVar;
#else
{
unsigned int size;
plerrcode res;
if ((res=var_attrib(p[2], &size))!=plOK) return res;
if (size!=1) return plErr_IllVarSize;
}
#endif
wirbrauchen=1;
return plOK;
}

char name_proc_force_dt_var[]="ForceDTvar";
int ver_proc_force_dt_var=1;
/*****************************************************************************/
/*****************************************************************************/
