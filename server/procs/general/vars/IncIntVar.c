/*
 * procs/general/vars/IncIntVar.c
 * created before 28.09.93
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: IncIntVar.c,v 1.8 2017/10/09 21:25:37 wuestner Exp $";

#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../procprops.h"
#include "../../../objects/var/variables.h"

RCS_REGISTER(cvsid, "procs/general/vars")

/*****************************************************************************/
/*
IncIntVar(var)
  inkrementiert die Variable var (muss Laenge 1 haben)
*/
plerrcode proc_IncIntVar(ems_u32* p)
{
var_list[*++p].var.val++;
return(plOK);
}

plerrcode test_proc_IncIntVar(ems_u32* p)
{
if (*p!=1) return(plErr_ArgNum);
if (*++p>MAX_VAR) return(plErr_IllVar);
if (!var_list[*p].len) return(plErr_NoVar);
if (var_list[*p].len!=1) return(plErr_IllVarSize);
wirbrauchen=0;
return(plOK);
}
#ifdef PROCPROPS
static procprop IncIntVar_prop={0, 0, "int var",
    "inkrementiert Integer-Variable"};

procprop* prop_proc_IncIntVar()
{
return(&IncIntVar_prop);
}
#endif
char name_proc_IncIntVar[]="IncIntVar";
int ver_proc_IncIntVar=1;

/*****************************************************************************/
/*****************************************************************************/
