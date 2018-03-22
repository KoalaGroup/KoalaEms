/*
 * procs/general/vars/modvar.c
 * created 2011-10-17 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: modvar.c,v 1.3 2017/10/20 23:20:52 wuestner Exp $";

#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../procprops.h"
#include "../../../objects/var/variables.h"

RCS_REGISTER(cvsid, "procs/general/vars")

/*****************************************************************************/
/*
 * var_bitselect
 * p[0]: argcount==3;
 * p[1]: integer variable
 * p[2]: right shift (negative: left shift)
 * p[3]: mask
 */

plerrcode proc_var_bitselect(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ems_u32 v=var_list[p[1]].var.val;
    if (ip[2]>0)
        v>>=ip[2];
    else if (ip[2]<0)
        v<<=-ip[2];
    v&=p[3];
    var_list[p[1]].var.val=v;
    return plOK;
}

plerrcode test_proc_var_bitselect(ems_u32* p)
{
    if (p[0]!=3)
        return plErr_ArgNum;
    if (p[1]>MAX_VAR)
        return plErr_IllVar;
    if (!var_list[p[1]].len)
        return plErr_NoVar;
    if (var_list[p[1]].len!=1)
        return plErr_IllVarSize;
    wirbrauchen=0;
    return plOK;
}

char name_proc_var_bitselect[]="var_bitselect";
int ver_proc_var_bitselect=1;

/*****************************************************************************/
/*****************************************************************************/
