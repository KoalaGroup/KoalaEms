/******************************************************************************
*                                                                             *
* GetIntVar.c                                                                 *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 07.09.93                                                                    *
*                                                                             *
******************************************************************************/

#include <errorcodes.h>
#include "../../../objects/var/variables.h"

extern ems_u32* outptr;

/*****************************************************************************/
/*
GetIntVar(var)
  schreibt Inhalt der Variablen var (muss Laenge 1 haben) in den Ausgabepuffer
*/
plerrcode proc_GetIntVar(p)
int *p;
{
*outptr++=var_list[p[1]].var.val;
return(plOK);
}

plerrcode proc_GetVar(p)
int *p;
{
register int i;
i=var_list[p[1]].len;
if(i==1)*outptr++=var_list[p[1]].var.val;
else{
	register int *ptr;
	ptr=var_list[p[1]].var.ptr;
	while(i--)*outptr++= *ptr++;
}
return(plOK);
}

plerrcode test_proc_GetIntVar(p)
int *p;
{
if (p[0]!=1) return(plErr_ArgNum);
if (*++p>MAX_VAR) return(plErr_IllVar);
if (!var_list[*p].len) return(plErr_NoVar);
if (var_list[*p].len!=1) return(plErr_IllVarSize);
return(plOK);
}

plerrcode test_proc_GetVar(p)
int *p;
{
if (p[0]!=1) return(plErr_ArgNum);
if (*++p>MAX_VAR) return(plErr_IllVar);
if (!var_list[*p].len) return(plErr_NoVar);
return(plOK);
}

char name_proc_GetIntVar[]="GetIntVar";
char name_proc_GetVar[]="GetVar";
int ver_proc_GetIntVar=1;
int ver_proc_GetVar=1;

/*****************************************************************************/
/*****************************************************************************/

