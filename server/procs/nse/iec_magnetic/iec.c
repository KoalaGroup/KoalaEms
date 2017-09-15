/****************************************************************************/
/*                                                                          */
/*                    09.05.'94                                             */
/*                                                                          */
/* Autor : Twardo (verifiziert fuer EMS)                                    */
/*                                                                          */
/****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <modes.h>
#include <errno.h>
#include <strings.h>
#include <signal.h>
#include "tms.h"
#include "iec.h"
#include <errorcodes.h>
#include <modultypes.h>
#include <sconf.h>
#include <debug.h>
#include <types.h>
#include <xdrstring.h>
#include "../../../LOWLEVEL/VME/vme.h"
#include "../../../OBJECTS/VAR/variables.h"

#define     PATHNAME        "/iecc"

extern ems_u32* outptr;
extern int* memberlist;

int     signal = 0;
char *combuf;

/****************************************************************************/
/* Procedure IEC_transmit                                                   */
/*                                                                          */
/* Confirmation:                                                            */
/*                                                                          */
/****************************************************************************/

plerrcode proc_IEC_transmit(p)

int *p;
{
int i,errcode,iecpath;
char end_of_msg,*c_hlp;

combuf=(char *)malloc(256*sizeof(char));

/* --- gebe Modulnummer zurueck -------------------------------------------- */
*outptr++= p[1];

D(D_REQ,printf("IEC Modul 0x%x Adresse 0x%x\n",p[1],memberlist[p[1]]);)

/* --- End of Msg ---------------------------------------------------------- */
end_of_msg=(char)p[2];

/* --- Commandstring ------------------------------------------------------- */
extractstring(combuf,&p[3]);

D(D_REQ,printf("EndOfMsg 0x%x Command ***%s***\n",end_of_msg,combuf);)fflush(stdout);

c_hlp=rindex(combuf,'\0');
*c_hlp=end_of_msg;
*(++c_hlp)=0;

if ((iecpath = open(PATHNAME, 3)) == -1)  
{ printf("ERRNO %d \n",errno);
  *outptr++ =CON_IEC+NO_TRI; *outptr++ =errno; free(combuf); return(plErr_Other); }

if ( isetstt(iecpath,SS_SSRq,SIGNAL,0) == -1 )
   D(D_REQ,printf("ERROR isetstt SS_SSRq, errno %d\n",errno);)

i = strlen(combuf);
if ( errcode=transmit(memberlist[p[1]],iecpath,combuf,i))
{  *outptr++ =TRA_IEC+errcode;*outptr++ =errno;close(iecpath);
   return(plErr_Other);}

close(iecpath);
free(combuf);
return(plOK);
}

/****************************************************************************/
/* Procedure IEC_receive                                                    */
/*                                                                          */
/* Confirmation:                                                            */
/*                                                                          */
/****************************************************************************/

plerrcode proc_IEC_receive(p)

int *p;
{
int i,errcode,iecpath,bufsiz;
int *xdrstr=(int *)malloc(BUFFERSIZE*sizeof(int));

combuf=(char *)malloc(256*sizeof(char));

/* --- gebe Modulnummer zurueck -------------------------------------------- */
*outptr++= p[1];

D(D_REQ,printf("# IEC Modul 0x%x Adresse 0x%x\n",p[1],memberlist[p[1]]);)

if ((iecpath = open(PATHNAME, 3)) == -1)  
{ printf("OpenIEC_ERROR ERRNO: %d \n",errno);
  *outptr++ =CON_IEC+NO_TRI; *outptr++ =errno; free(combuf); return(plErr_Other);}

if ( errcode=receive(memberlist[p[1]],iecpath,combuf,BUFFERSIZE) )
{  *outptr++ =REC_IEC+errcode;*outptr++ =errno;
   close(iecpath);free(combuf);return(plErr_Other);}
D(D_REQ,printf("receive: ---%s---\n",combuf);)

outstring(xdrstr,combuf);
bufsiz=strlwlen(xdrstr);printf("bufsiz %d (dez)\n",bufsiz);
for (i=0;i<bufsiz;i++) {*outptr++ = xdrstr[i]; printf(":%x\n",xdrstr[i]);}

close(iecpath);
free(combuf);
return(plOK);
}
/************************************************************************/
plerrcode test_proc_IEC_transmit(p)
int *p;
{
   if ( p[0] < 3 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != IEC ){
      *outptr++=memberlist[p[1]];
                                   return(plErr_BadModTyp);
   if ((p[1] >= 31) || (p[1] < 0)) return(plErr_BadHWAddr);
   }
   return(plOK);
}
char name_proc_IEC_transmit[]="IEC_transmit";
int ver_proc_IEC_transmit=1; 
/************************************************************************/
plerrcode test_proc_IEC_receive(p)
int *p;
{
   if ( p[0] != 1 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != IEC ){
      *outptr++=memberlist[p[1]];
                                   return(plErr_BadModTyp);
   }
   if ((p[1] >= 31) || (p[1] < 0)) return(plErr_BadHWAddr);
   return(plOK);
}
char name_proc_IEC_receive[]="IEC_receive";
int ver_proc_IEC_receive=1;
 
/************************************************************************/
  


