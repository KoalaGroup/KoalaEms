#include <errorcodes.h>

extern ems_u32* outptr;

plerrcode proc_ReadVIC(p)
int *p;
{
  int help;
  if(vicread(p[1],p[2],p[3],&help))return(plErr_HW);
  *outptr++=help;
  return(OK);
}

plerrcode proc_WriteVIC(p)
int *p;
{
  if(vicwrite(p[1],p[2],p[3],p[4]))return(plErr_HW);
}

plerrcode proc_blReadVIC(p)
int *p;
{
  int len;
  len=p[4];
  if(vicblread(p[1],p[2],p[3],outptr,&len)||(len!=p[4]))
    return(plErr_HW);
  outptr+=len;
  return(OK);
}

plerrcode proc_blWriteVIC(p)
int *p;
{
  if(vicblwrite(p[1],p[2],p[3],&p[4],p[0]-3))return(plErr_HW);
}

plerrcode proc_readCSR(p)
int *p;
{
  int help;
  if(csrread(p[1],p[2],&help))return(plErr_HW);
  *outptr++=help;
  return(OK);
}

plerrcode proc_writeCSR(p)
int *p;
{
  if(csrwrite(p[1],p[2],p[3]))return(plErr_HW);
  return(OK);
}

plerrcode test_proc_ReadVIC()
{
  return(plOK);
}
plerrcode test_proc_WriteVIC()
{
  return(plOK);
}
plerrcode test_proc_blReadVIC()
{
  return(plOK);
}
plerrcode test_proc_blWriteVIC()
{
  return(plOK);
}
plerrcode test_proc_readCSR()
{
  return(plOK);
}
plerrcode test_proc_writeCSR()
{
  return(plOK);
}

char name_proc_ReadVIC[]="ReadVIC";
char name_proc_WriteVIC[]="WriteVIC";
char name_proc_blReadVIC[]="blReadVIC";
char name_proc_blWriteVIC[]="blWriteVIC";
char name_proc_readCSR[]="readCSR";
char name_proc_writeCSR[]="writeCSR";

int ver_proc_ReadVIC=1;
int ver_proc_WriteVIC=1;
int ver_proc_blReadVIC=1;
int ver_proc_blWriteVIC=1;
int ver_proc_readCSR=1;
int ver_proc_writeCSR=1;
