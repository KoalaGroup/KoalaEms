/* $ZEL: quick.c,v 1.5 2005/02/11 17:58:29 wuestner Exp $

#include <errorcodes.h>
#include <types.h>
#include "../../LOWLEVEL/VME/vme.h"
#include "../../OBJECTS/VAR/variables.h"

extern ems_u32* outptr;

/*****************************************************************/
plerrcode proc_VME_read(p)
int *p;
{
    *outptr++= *SMPaddr(p[1]);
    return(plOK);
}
/*****************************************************************/
plerrcode proc_VME_write(p)
int *p;
{
    *SMPaddr(p[1])=p[2];
    return(plOK);
}
/*****************************************************************/
plerrcode proc_VMEbl_read(p)                                    
int *p;                                                          
{                                                                 
    int i=p[2];
    u_char *addr;

    addr=SMPaddr(p[1]);
        
    while(i--) *outptr++=(int) *addr++;                                   
    return(plOK);                                                 
}
/*****************************************************************/
plerrcode proc_VMEbl_write(p)                                    
int *p;                                                          
{       
    int i=p[0];                                                          
    u_char *addr;

    addr=SMPaddr(p[1]);
    i--;p++;
        
    while(i--) *addr++=*++p;                                   
    return(plOK);                                                 
}
/*****************************************************************/
plerrcode proc_VMEhelp(p)
int *p;
{
printf("                      NOHELP\n");
return(0);
}
/*****************************************************************/
plerrcode test_proc_VME_read(p)
int *p;
{
if (*p!=1) return(plErr_ArgNum);
return(plOK);
}

plerrcode test_proc_VME_write(p)
int *p;
{
if (*p!=2) return(plErr_ArgNum);
return(plOK);
}

plerrcode test_proc_VMEbl_read(p)
int *p;
{
if (*p!=2) return(plErr_ArgNum);
return(plOK);
}

plerrcode test_proc_VMEbl_write(p)
int *p;
{
return(plOK);
}

plerrcode test_proc_VMEhelp(p)
int *p;
{
return(plOK);
}

char name_proc_VME_read[]="VMEread";
char name_proc_VME_write[]="VMEwrite";
char name_proc_VMEbl_read[]="VMEblread";             
char name_proc_VMEbl_write[]="VMEblwrite";
char name_proc_VMEhelp[]="VMEhelp";

int ver_proc_VME_read=1;
int ver_proc_VME_write=1;
int ver_proc_VMEbl_write=1;
int ver_proc_VMEbl_read=1;                           
int ver_proc_VMEhelp=1;
