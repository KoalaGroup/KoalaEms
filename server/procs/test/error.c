/******************************************************************************
*                                                                             *
* error.c                                                                     *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 04.11.94                                                                    *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: error.c,v 1.10 2017/10/20 23:20:52 wuestner Exp $";

#include <errorcodes.h>
#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "../procprops.h"
#include "../procs.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/test")

/*****************************************************************************/

plerrcode proc_Error(ems_u32* p)
{
unsigned int i;

D(D_USER, printf("proc_Error(");
        for (i=1; i<=p[0]; i++) printf("%d%s", p[i], i<p[0]?", ":")\n");)
for (i=2; i<=p[0]; i++)
  *outptr++=p[i];
return(p[1]);
}

plerrcode test_proc_Error(ems_u32* p)
{
if (p[0]<1) return(plErr_ArgNum);
if ((unsigned int)p[1]>=NrOfplErrs) return(plErr_ArgRange);
return(plOK);
}
#ifdef PROCPROPS
static procprop Error_prop={1, -1, "<errorcode> ...", 0};

procprop* prop_proc_Error(void)
{
return(&Error_prop);
}
#endif
char name_proc_Error[]="Error";
int ver_proc_Error=1;

/*****************************************************************************/

plerrcode proc_T_Error(__attribute__((unused)) ems_u32* p)
{
D(D_USER, printf("proc_T_Error called!\n");)
return(plErr_Other);
}

plerrcode test_proc_T_Error(ems_u32* p)
{
if (p[0]<1) return(plErr_ArgNum);
if ((unsigned int)p[1]>=NrOfplErrs) return(plErr_ArgRange);
return(p[1]);
}
#ifdef PROCPROPS
static procprop T_Error_prop={0, 0, "<errorcode>", 0};

procprop* prop_proc_T_Error(void)
{
return(&T_Error_prop);
}
#endif
char name_proc_T_Error[]="T_Error";
int ver_proc_T_Error=1;

/*****************************************************************************/
/*****************************************************************************/
