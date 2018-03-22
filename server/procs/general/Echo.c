/*
 * procs/general/Echo.c
 * 04.11.94
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: Echo.c,v 1.9 2017/10/09 21:25:37 wuestner Exp $";

#include <errorcodes.h>
#include <rcs_ids.h>
#include "../procprops.h"
#include "../procs.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/general")

/*****************************************************************************/
/*
Echo(...) schreibt alle Argumente in den Ausgabepuffer
*/
plerrcode proc_Echo(ems_u32* p)
{
register int i= *p++;
while (i--) *outptr++= *p++;
return(plOK);
}

plerrcode test_proc_Echo(ems_u32* p)
{
wirbrauchen=p[0];
return(plOK);
}
#ifdef PROCPROPS
static procprop Echo_prop={1, -1, "{data}", 0};

procprop* prop_proc_Echo(void)
{
return(&Echo_prop);
}
#endif
char name_proc_Echo[]="Echo";
int ver_proc_Echo=1;

/*****************************************************************************/

plerrcode proc_MakeData(ems_u32* p)
{
    ems_u32 n=p[1];
    if (n)
        outptr+=n;
    return plOK;
}

plerrcode test_proc_MakeData(ems_u32* p)
{
    if(p[0]!=1)
        return plErr_ArgNum;
    wirbrauchen=p[1];
    return plOK;
}
#ifdef PROCPROPS
static procprop MakeData_prop={1, -1, "num", 0};

procprop* prop_proc_MakeData(void)
{
    return &MakeData_prop;
}
#endif
char name_proc_MakeData[]="MakeData";
int ver_proc_MakeData=1;
/*****************************************************************************/
/*****************************************************************************/
