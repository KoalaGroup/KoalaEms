/*
 * procs/general/vars/SetIntVar.c
 * created before 28.09.93
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: SetIntVar.c,v 1.9 2017/10/09 21:25:37 wuestner Exp $";

#include <sconf.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../procprops.h"
#include "../../../objects/var/variables.h"
#ifdef READOUT_CC
#include "../../../objects/pi/readout.h"
#endif

RCS_REGISTER(cvsid, "procs/general/vars")

/*****************************************************************************/
/*
SetIntVar(var,value)
  setzt die Variable var (muss Laenge 1 haben) auf den Wert value
*/
plerrcode proc_SetIntVar(ems_u32* p)
{
var_list[*(p+1)].var.val= *(p+2);
return(plOK);
}

plerrcode test_proc_SetIntVar(ems_u32* p)
{
if (*p!=2) return(plErr_ArgNum);
if (p[1]>MAX_VAR) return(plErr_IllVar);
if (!var_list[p[1]].len) return(plErr_NoVar);
if (var_list[p[1]].len!=1) return(plErr_IllVarSize);
wirbrauchen=0;
return(plOK);
}
#ifdef PROCPROPS
static procprop all_prop={0, 0, "int var, int val", "setzt Integer-Variable"};

procprop* prop_proc_SetIntVar()
{
return(&all_prop);
}
#endif
char name_proc_SetIntVar[]="SetIntVar";
int ver_proc_SetIntVar=1;

/*****************************************************************************/
/*
 * evc2var
 * writes eventcount%x into var
 * p[0]: argcount;
 * p[1]: integer variable
 * [p[2]: x]
 */
plerrcode proc_evc2var(ems_u32* p)
{
#ifndef READOUT_CC
    return plErr_NoSuchProc;
#else
    ems_u32 evc=global_evc.ev_count;
    if (p[0]>1)
        evc%=p[2];
    var_list[p[1]].var.val=evc;
    return plOK;
#endif
}

plerrcode test_proc_evc2var(ems_u32* p)
{
#ifndef READOUT_CC
    return plErr_NoSuchProc;
#else
    if (p[0]!=1 && p[0]!=2)
        return plErr_ArgNum;
    if (p[1]>MAX_VAR)
        return plErr_IllVar;
    if (!var_list[p[1]].len)
        return plErr_NoVar;
    if (var_list[p[1]].len!=1)
        return plErr_IllVarSize;
    if (p[0]>1 && !p[2])
        return plErr_ArgRange;

    wirbrauchen=0;
    return plOK;
#endif
}

char name_proc_evc2var[]="evc2var";
int ver_proc_evc2var=1;
/*****************************************************************************/
/*****************************************************************************/
