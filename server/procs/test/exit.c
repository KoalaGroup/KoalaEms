/******************************************************************************
*                                                                             *
* exit.c                                                                      *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 04.11.94                                                                    *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: exit.c,v 1.7 2011/04/06 20:30:34 wuestner Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "../procprops.h"
#include "../procs.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/test")

/*****************************************************************************/

plerrcode proc_Exit(ems_u32* p)
{
printf("Auf Wiedersehen !\n");
exit(2);
return(plOK);
}

plerrcode test_proc_Exit(ems_u32* p)
{
if ((p[0]!=1) || (p[1]!=1879)) return(plErr_NoSuchProc);
return(plOK);
}
#ifdef PROCPROPS
static procprop Exit_prop={0, 0, "1879", 0};

procprop* prop_proc_Exit(void)
{
return(&Exit_prop);
}
#endif
char name_proc_Exit[]="Exit";
int ver_proc_Exit=1;

/*****************************************************************************/
/*****************************************************************************/
