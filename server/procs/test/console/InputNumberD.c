/******************************************************************************
* $ZEL: InputNumberD.c,v 1.10 2017/10/20 23:20:52 wuestner Exp $
*                                                                             *
* InputNumberD.c                                                              *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 21.01.93                                                                    *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: InputNumberD.c,v 1.10 2017/10/20 23:20:52 wuestner Exp $";

#include <stdio.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procs.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/test/console")

/*****************************************************************************/

plerrcode proc_InputNumberD(__attribute__((unused)) ems_u32* p)
{
    int num;

    if (scanf("%d", &num)!=1) {
        printf("cannot parse input\n");
        return plErr_Other;
    } else {
        *outptr++=num;
        return plOK;
    }
}

plerrcode test_proc_InputNumberD(ems_u32* p)
{
if(*p!=0) return (plErr_ArgNum);
wirbrauchen=1;
return(plOK);
}

char name_proc_InputNumberD[]="InputNumberD";
int ver_proc_InputNumberD=1;

/*****************************************************************************/
/*****************************************************************************/

