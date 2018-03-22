/*
 * procs/test/console/PrintIntVar.c
 * 07.09.93
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: PrintIntVar.c,v 1.7 2017/10/09 21:25:38 wuestner Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../../../objects/var/variables.h"
#include "../../procs.h"

RCS_REGISTER(cvsid, "procs/test/console")

/*****************************************************************************/

plerrcode proc_PrintIntVarD(ems_u32* p)
{
    if (p[0]>1) {
        char *s;
        xdrstrcdup(&s, p+2);
        printf("%s %d\n", s, var_list[p[1]].var.val);
        free(s);
    } else {
        printf("%d\n", var_list[p[1]].var.val);
    }
    return plOK;
}

plerrcode proc_PrintIntVarX(ems_u32* p)
{
    if (p[0]>1) {
        char *s;
        xdrstrcdup(&s, p+2);
        printf("%s %08x\n", s, var_list[p[1]].var.val);
        free(s);
    } else {
        printf("%08x\n", var_list[p[1]].var.val);
    }
    return plOK;
}

plerrcode test_proc_PrintIntVarD(ems_u32* p)
{
    if (p[0]<1)
        return plErr_ArgNum;
    if (p[1]>=MAX_VAR)
        return plErr_IllVar;
    if (!var_list[p[1]].len)
        return plErr_NoVar;
    if (var_list[p[1]].len!=1)
        return plErr_IllVarSize;
    if (p[0]>1) {
        if (xdrstrlen(p+2)!=p[0]-1)
            return plErr_ArgNum;
    }
    wirbrauchen=0;
    return plOK;
}

plerrcode test_proc_PrintIntVarX(ems_u32* p)
{
    return test_proc_PrintIntVarD(p);
}

char name_proc_PrintIntVarD[]="PrintIntVarD";
int ver_proc_PrintIntVarD=1;
char name_proc_PrintIntVarX[]="PrintIntVarX";
int ver_proc_PrintIntVarX=1;

/*****************************************************************************/
/*****************************************************************************/

