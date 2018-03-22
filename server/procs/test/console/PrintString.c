/******************************************************************************
* $ZEL: PrintString.c,v 1.7 2017/10/09 21:25:38 wuestner Exp $
*                                                                             *
* PrintString.c                                                               *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 21.01.93                                                                    *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: PrintString.c,v 1.7 2017/10/09 21:25:38 wuestner Exp $";

#include <stdio.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../../procs.h"

RCS_REGISTER(cvsid, "procs/test/console")

/*****************************************************************************/

plerrcode proc_PrintString(ems_u32* p)
{
char buf[256];

extractstring(buf,++p);
printf("%s",buf);
return(plOK);
}

plerrcode test_proc_PrintString(ems_u32* p)
{
if (*p>63) return(plErr_NoMem);
wirbrauchen=0;
return(plOK);
}

char name_proc_PrintString[]="PrintString";
int ver_proc_PrintString=1;

/*****************************************************************************/
/*****************************************************************************/

