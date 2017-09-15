/******************************************************************************
* $ZEL: InputNumberD.c,v 1.8 2011/04/06 20:30:34 wuestner Exp $
*                                                                             *
* InputNumberD.c                                                              *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 21.01.93                                                                    *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: InputNumberD.c,v 1.8 2011/04/06 20:30:34 wuestner Exp $";

#include <stdio.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procs.h"

extern ems_u32* outptr;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/test/console")

/*****************************************************************************/

plerrcode proc_InputNumberD(ems_u32* p)
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

