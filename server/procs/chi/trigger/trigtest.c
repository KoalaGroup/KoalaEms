/******************************************************************************
*                                                                             *
* trigtest.c for chi                                                          *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 05.04.94                                                                    *
*                                                                             *
******************************************************************************/

#include <errorcodes.h>
#include "../../../trigger/chi/gsi/gsitrigger.h"
#include <ssm.h>

#ifdef NOVOLATILE
#define VOLATILE
#else
#define VOLATILE volatile
#endif

extern ems_u32* outptr;

/*****************************************************************************/

plerrcode proc_GSI_r_stat(p)
int *p;
{
VOLATILE int* addr;

addr=(int*)(GSI_BASE+0x100*p[1]+STATUS);
*outptr++=*addr;
return(plOK);
}

plerrcode test_proc_GSI_r_stat(p)
int *p;
{
if (p[0]!=1) return(plErr_ArgNum);
if ((p[1]<0) || (p[1]>3)) return(plErr_ArgRange);
permit(GSI_BASE+0x100*p[1], 16, ssm_RW);
return(plOK);
}

char name_proc_GSI_r_stat[]="GSI_r_stat";
int ver_proc_GSI_r_stat=1;

/*****************************************************************************/

plerrcode proc_GSI_w_stat(p)
int *p;
{
VOLATILE int* addr;

addr=(int*)(GSI_BASE+0x100*p[1]+STATUS);
*addr=p[2];
*outptr++=*addr;
return(plOK);
}

plerrcode test_proc_GSI_w_stat(p)
int *p;
{
if (p[0]!=2) return(plErr_ArgNum);
if ((p[1]<0) || (p[1]>3)) return(plErr_ArgRange);
permit(GSI_BASE+0x100*p[1], 16, ssm_RW);
return(plOK);
}

char name_proc_GSI_w_stat[]="GSI_w_stat";
int ver_proc_GSI_w_stat=1;

/*****************************************************************************/

plerrcode proc_GSI_r_cont(p)
int *p;
{
VOLATILE int* addr;

addr=(int*)(GSI_BASE+0x100*p[1]+CONTROL);
*outptr++=*addr;
return(plOK);
}

plerrcode test_proc_GSI_r_cont(p)
int *p;
{
if (p[0]!=1) return(plErr_ArgNum);
if ((p[1]<0) || (p[1]>3)) return(plErr_ArgRange);
permit(GSI_BASE+0x100*p[1], 16, ssm_RW);
return(plOK);
}

char name_proc_GSI_r_cont[]="GSI_r_cont";
int ver_proc_GSI_r_cont=1;

/*****************************************************************************/

plerrcode proc_GSI_w_cont(p)
int *p;
{
VOLATILE int* addr;

addr=(int*)(GSI_BASE+0x100*p[1]+CONTROL);
*addr=p[2];
*outptr++=*addr;
return(plOK);
}

plerrcode test_proc_GSI_w_cont(p)
int *p;
{
if (p[0]!=2) return(plErr_ArgNum);
if ((p[1]<0) || (p[1]>3)) return(plErr_ArgRange);
permit(GSI_BASE+0x100*p[1], 16, ssm_RW);
return(plOK);
}

char name_proc_GSI_w_cont[]="GSI_w_cont";
int ver_proc_GSI_w_cont=1;

/*****************************************************************************/

plerrcode proc_GSI_r_fca(p)
int *p;
{
VOLATILE int* addr;

addr=(int*)(GSI_BASE+0x100*p[1]+FCATIME);
*outptr++=*addr;
return(plOK);
}

plerrcode test_proc_GSI_r_fca(p)
int *p;
{
if (p[0]!=1) return(plErr_ArgNum);
if ((p[1]<0) || (p[1]>3)) return(plErr_ArgRange);
permit(GSI_BASE+0x100*p[1], 16, ssm_RW);
return(plOK);
}

char name_proc_GSI_r_fca[]="GSI_r_fca";
int ver_proc_GSI_r_fca=1;

/*****************************************************************************/

plerrcode proc_GSI_w_fca(p)
int *p;
{
VOLATILE int* addr;

addr=(int*)(GSI_BASE+0x100*p[1]+FCATIME);
*addr=p[2];
*outptr++=*addr;
return(plOK);
}

plerrcode test_proc_GSI_w_fca(p)
int *p;
{
if (p[0]!=2) return(plErr_ArgNum);
if ((p[1]<0) || (p[1]>3)) return(plErr_ArgRange);
permit(GSI_BASE+0x100*p[1], 16, ssm_RW);
return(plOK);
}

char name_proc_GSI_w_fca[]="GSI_w_fca";
int ver_proc_GSI_w_fca=1;

/*****************************************************************************/

plerrcode proc_GSI_r_ctime(p)
int *p;
{
VOLATILE int* addr;

addr=(int*)(GSI_BASE+0x100*p[1]+CTIME);
*outptr++=*addr;
return(plOK);
}

plerrcode test_proc_GSI_r_ctime(p)
int *p;
{
if (p[0]!=1) return(plErr_ArgNum);
if ((p[1]<0) || (p[1]>3)) return(plErr_ArgRange);
permit(GSI_BASE+0x100*p[1], 16, ssm_RW);
return(plOK);
}

char name_proc_GSI_r_ctime[]="GSI_r_ctime";
int ver_proc_GSI_r_ctime=1;

/*****************************************************************************/

plerrcode proc_GSI_w_ctime(p)
int *p;
{
VOLATILE int* addr;

addr=(int*)(GSI_BASE+0x100*p[1]+CTIME);
*addr=p[2];
*outptr++=*addr;
return(plOK);
}

plerrcode test_proc_GSI_w_ctime(p)
int *p;
{
if (p[0]!=2) return(plErr_ArgNum);
if ((p[1]<0) || (p[1]>3)) return(plErr_ArgRange);
permit(GSI_BASE+0x100*p[1], 16, ssm_RW);
return(plOK);
}

char name_proc_GSI_w_ctime[]="GSI_w_ctime";
int ver_proc_GSI_w_ctime=1;

/*****************************************************************************/
/*****************************************************************************/

