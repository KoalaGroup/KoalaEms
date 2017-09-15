/******************************************************************************
* $ZEL: real.c,v 1.6 2011/04/06 20:30:35 wuestner Exp $                                                                    *
*                                                                             *
* real.c                                                                      *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 26.04.93                                                                    *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: real.c,v 1.6 2011/04/06 20:30:35 wuestner Exp $";

#include <errorcodes.h>
#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/test/real")

/*****************************************************************************/

plerrcode proc_PrintReal(p)
int *p;
{
float x;

D(D_USER, printf("PrintReal\n");)
x=*(float*)&p[1];
printf("%e\n", x);
return(plOK);
}

plerrcode test_proc_PrintReal(p)
int *p;
{
if (p[0]!=1) return(plErr_ArgNum);
return(plOK);
}

char name_proc_PrintReal[]="PrintReal";
int ver_proc_PrintReal=1;

/*****************************************************************************/

plerrcode proc_PrintDouble(p)
int *p;
{
double x;

D(D_USER, printf("PrintDouble\n");)
x=*(double*)&p[1];
printf("%E\n", x);
return(plOK);
}

plerrcode test_proc_PrintDouble(p)
int *p;
{
D(D_USER, printf("test_proc_PrintDouble(%d)\n", *p);)
if (p[0]!=2) return(plErr_ArgNum);
return(plOK);
}

char name_proc_PrintDouble[]="PrintDouble";
int ver_proc_PrintDouble=1;

/*****************************************************************************/

plerrcode proc_ReadReal(p)
int *p;
{
float x;

D(D_USER, printf("ReadReal\n");)
if (scanf("%e", &x)==1)
  {
  *(float*)outptr=x;
  outptr++;
  return(plOK);
  }
else
  {
  perror("scanf");
  return(plErr_Other);
  }
}

plerrcode test_proc_ReadReal(p)
int *p;
{
if (p[0]!=0) return(plErr_ArgNum);
return(plOK);
}

char name_proc_ReadReal[]="ReadReal";
int ver_proc_ReadReal=1;

/*****************************************************************************/

plerrcode proc_ReadDouble(p)
int *p;
{
double x;

D(D_USER, printf("ReadDouble\n");)
if (scanf("%E", &x)==1)
  {
  *(double*)outptr=x;
  outptr+=2;
  return(plOK);
  }
else
  {
  perror("scanf");
  return(plErr_Other);
  }
}

plerrcode test_proc_ReadDouble(p)
int *p;
{
if (p[0]!=0) return(plErr_ArgNum);
return(plOK);
}

char name_proc_ReadDouble[]="ReadDouble";
int ver_proc_ReadDouble=1;

/*****************************************************************************/
/*****************************************************************************/

