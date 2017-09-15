/*
 * procs/general/vars/GetVar.c
 * created before 07.09.93
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: GetVar.c,v 1.7 2011/04/06 20:30:32 wuestner Exp $";

#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../objects/var/variables.h"
#include "../../procs.h"
#include "../../procprops.h"

extern ems_u32* outptr;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/general/vars")

/*****************************************************************************/
/*
GetIntVar(var)
  schreibt Inhalt der Variablen var (muss Laenge 1 haben) in den Ausgabepuffer
*/
plerrcode proc_GetIntVar(ems_u32* p)
{
    *outptr++=var_list[p[1]].var.val;
    return plOK;
}

plerrcode proc_GetVar(ems_u32* p)
{
    register int i;

    i=var_list[p[1]].len;
    if (i==1) {
        *outptr++=var_list[p[1]].var.val;
    } else {
        register ems_u32 *ptr;
        ptr=var_list[p[1]].var.ptr;
        while (i--)
            *outptr++= *ptr++;
    }
    return plOK;
}

plerrcode test_proc_GetIntVar(ems_u32* p)
{
if (p[0]!=1) return(plErr_ArgNum);
if (*++p>MAX_VAR) return(plErr_IllVar);
if (!var_list[*p].len) return(plErr_NoVar);
if (var_list[*p].len!=1) return(plErr_IllVarSize);
wirbrauchen=1;
return(plOK);
}

plerrcode test_proc_GetVar(ems_u32* p)
{
if (p[0]!=1) return(plErr_ArgNum);
if (*++p>MAX_VAR) return(plErr_IllVar);
if (!var_list[*p].len) return(plErr_NoVar);
wirbrauchen=var_list[*p].len;
return(plOK);
}
#ifdef PROCPROPS
static procprop GetIntVar_prop={0, 1, "int var", ""};

static procprop GetVar_prop={1, -1, "int var", ""};

procprop* prop_proc_GetIntVar()
{
return(&GetIntVar_prop);
}

procprop* prop_proc_GetVar()
{
return(&GetVar_prop);
}
#endif
char name_proc_GetIntVar[]="GetIntVar";
char name_proc_GetVar[]="GetVar";
int ver_proc_GetIntVar=1;
int ver_proc_GetVar=1;

/*****************************************************************************/
/*****************************************************************************/
