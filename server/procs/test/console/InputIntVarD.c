/******************************************************************************
*                                                                             *
* InputIntVarD.c                                                              *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 07.09.93                                                                    *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: InputIntVarD.c,v 1.7 2017/10/09 21:25:38 wuestner Exp $";

#include <stdio.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../objects/var/variables.h"
#include "../../procs.h"

RCS_REGISTER(cvsid, "procs/test/console")

/*****************************************************************************/

plerrcode proc_InputIntVarD(ems_u32* p)
{
    if (scanf("%d", &(var_list[*++p].var.val))!=1) {
        printf("cannot parse input\n");
        return plErr_Other;
    } else {
        return plOK;
    }
}

plerrcode test_proc_InputIntVarD(ems_u32* p)
{
if (p[0]!=1) return(plErr_ArgNum);
if ((unsigned int)p[1]>MAX_VAR) return(plErr_IllVar);
if (var_list[p[1]].len==0) return(plErr_NoVar);
if (var_list[p[1]].len!=1) return(plErr_IllVarSize);
wirbrauchen=0;
return(plOK);
}

char name_proc_InputIntVarD[]="InputIntVarD";
int ver_proc_InputIntVarD=1;

/*****************************************************************************/
/*****************************************************************************/

