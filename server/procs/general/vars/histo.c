/******************************************************************************
*                                                                             *
* histo.c                                                                     *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* created before 03.03.94                                                     *
* last changed 17.01.95                                                       *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: histo.c,v 1.6 2011/04/06 20:30:32 wuestner Exp $";

#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../objects/var/variables.h"
#include "../../procs.h"
#include "../../procprops.h"

extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/general/vars")

plerrcode proc_IncHistoVar(ems_u32* p)
{
var_list[p[1]].var.ptr[(var_list[p[2]].var.val&p[3])>>p[4]]++;
return(plOK);
}

plerrcode test_proc_IncHistoVar(ems_u32* p)
{
if (*p!=4) return(plErr_ArgNum);
if(((unsigned)p[1]>MAX_VAR)||((unsigned)p[2]>MAX_VAR))return(plErr_IllVar);
if((!var_list[p[1]].len)||(!var_list[p[2]].len))return(plErr_NoVar);
if (var_list[p[1]].len<((p[3]>>p[4])+1))return(plErr_IllVarSize);
if (var_list[p[2]].len!=1) return(plErr_IllVarSize);
wirbrauchen=0;
return(plOK);
}
#ifdef PROCPROPS
static procprop IncHistoVar_prop={0, 0, "", ""};

procprop* prop_proc_IncHistoVar()
{
return(&IncHistoVar_prop);
}
#endif
char name_proc_IncHistoVar[]="IncHistoVar";
int ver_proc_IncHistoVar=1;

/*****************************************************************************/
/*****************************************************************************/
