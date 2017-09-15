/******************************************************************************
*                                                                             *
* modulnames.c                                                                *
*                                                                             *
* ULTRIX, OSF1                                                                *
*                                                                             *
* created: 10.02.94                                                           *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: modulnames.c,v 2.3 2011/04/07 14:07:08 wuestner Exp $";

#include <stdio.h>
#include "modulnames.h"
#include <modultypeinfo.h>
#include "rcs_ids.h"

extern modultypeinfo modultypes[];
extern int anzahl_der_bekannten_module;

RCS_REGISTER(cvsid, "common")

/*****************************************************************************/

char *modulname(int id, int no_known)
{
int i;

i=0;
while ((i<anzahl_der_bekannten_module) && (modultypes[i].id!=id)) i++;
if (modultypes[i].id==id) return(modultypes[i].name);
if (no_known) return(modultypes[0].name);
return("");
}

/*****************************************************************************/

char *modulname_e(int id, int no_known)
{
int i;

i=0;
while ((i<anzahl_der_bekannten_module) && ((modultypes[i].id&0xffff)!=id)) i++;
if ((modultypes[i].id&0xffff)==id) return(modultypes[i].name);
if (no_known) return(modultypes[0].name);
return("");
}

/*****************************************************************************/
/*****************************************************************************/
