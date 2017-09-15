/*
 * procs/test/console/InputString.c
 * created 21.01.93
 * 13.Jul.2002 fgets() instead of gets()
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: InputString.c,v 1.8 2011/04/06 20:30:34 wuestner Exp $";

#include <stdio.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../../procs.h"

extern ems_u32* outptr;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/test/console")

/*****************************************************************************/

plerrcode proc_InputString(ems_u32* p)
{
    char buf[256];

    if (!fgets(buf, 256, stdin)) {
        printf("InputString: cannot parse input\n");
        return  plErr_Other;
    } else {
        outptr=outstring(outptr, buf);
        return plOK;
    }
}

plerrcode test_proc_InputString(ems_u32* p)
{
if (*p!=0) return(plErr_ArgNum);
wirbrauchen=65;
return(plOK);
}

char name_proc_InputString[]="InputString";
int ver_proc_InputString=1;

/*****************************************************************************/
/*****************************************************************************/

