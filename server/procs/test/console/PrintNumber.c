/*
 * procs/test/console/PrintNumber.c
 * created (before?) 21.01.93
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: PrintNumber.c,v 1.7 2011/04/06 20:30:34 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procs.h"

extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/test/console")

/*****************************************************************************/

plerrcode proc_PrintNumberD(ems_u32* p)
{
printf("%d\n",*++p);
return(plOK);
}

plerrcode proc_PrintNumberX(ems_u32* p)
{
printf("%x\n",*++p);
return(plOK);
}

plerrcode test_proc_PrintNumberD(ems_u32* p)
{
if (*p!=1) return(plErr_ArgNum);
wirbrauchen=0;
return(plOK);
}

plerrcode test_proc_PrintNumberX(ems_u32* p)
{
return(test_proc_PrintNumberD(p));
}

char name_proc_PrintNumberD[]="PrintNumberD";
int ver_proc_PrintNumberD=1;
char name_proc_PrintNumberX[]="PrintNumberX";
int ver_proc_PrintNumberX=1;

/*****************************************************************************/
/*****************************************************************************/

