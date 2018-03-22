/*
 * procs/general/vars/MakeVar.c
 * created 25.02.97 
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: MakeVar.c,v 1.5 2017/10/09 21:25:37 wuestner Exp $";

#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../objects/var/variables.h"
#include "../../procprops.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/general/vars")

/*****************************************************************************/
char name_proc_MakeVar[]="MakeVar";
int ver_proc_MakeVar=1;

/*****************************************************************************/
static procprop MakeVar_prop= {0,0,0,0};

procprop* prop_proc_MakeVar()
{
return(&MakeVar_prop);
}

/*****************************************************************************/

/*
MakeVar(var_list)
  in der Variablenliste angegebene Variablen werden mit angegebener Laenge
  erstellt, bereits existierende Variable derselben ID vernichtet
*/

plerrcode proc_MakeVar(int *p)
{
int nvar;
int vsize;
int ind, size;
plerrcode res;
int i;

nvar= (*p++)/2;

for (i=1;i<=nvar;i++) {
  ind= *p++;
  size= *p++;
  if ((res=var_attrib(ind,&vsize))==plOK)
    var_delete(ind);
  if ((res=var_create(ind,size))!=plOK) {
    *outptr++=ind;*outptr++=i;
    return(res);
   }
 }
return(plOK);
}

/*****************************************************************************/

plerrcode test_proc_MakeVar(int *p)
{
int nvar;
int i;

if (p[0]%2)  return(plErr_ArgNum);     /* check number of parameters         */
nvar= p[0]/2;
for (i=1;i<=nvar;i++) {
  if ((unsigned int)p[2*i-1]>MAX_VAR) {
    *outptr++=(2*i-1);*outptr++=p[2*i-1];
    return(plErr_IllVar);
   }
 }
wirbrauchen= 2;
return(plOK);
}

/*****************************************************************************/
/*****************************************************************************/
