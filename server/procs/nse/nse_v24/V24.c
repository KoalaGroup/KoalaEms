/*****************************************************************************/
/*                                                                           */
/* Erstellungsdatum: 12.10.'93                                               */
/*                                                                           */
/* letzte Aenderung: 15.10.                                                  */
/*                                                                           */
/* Autor: Twardowski, Joerg                                                  */
/*                                                                           */
/* Bemerkung: Die Prozeduren wurden mit einem Terminal an der V24-Schnitt-   */
/*            stelle getestet. Soll ein String gelesen werden wird als       */
/*            Endezeichen ein Wagenruecklauf erwartet. Das Schreiben ist     */
/*            soweit unproblematisch.                                        */
/*            Es sollte jedoch bei der Inbetriebname noch die maximale       */
/*            Buffergroesse ueberprueft werden.                              */
/*                                                                           */
/*            Bei Debugoption muss noch die Modulnummer und Channelnummer    */
/*            mit ausgegeben werden. Auch muss die Modulnummer an den Klient */
/*            zurueckgegeben werden.                                         */
/*****************************************************************************/

          
#include <stdio.h>
#include <strings.h>
#include <modes.h>
#include <errno.h>
#include <xdrstring.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <sconf.h>
#include <debug.h>
#include <types.h>
#include "../../../LOWLEVEL/VME/vme.h"
#include "../../../OBJECTS/VAR/variables.h"

#define T1 "/t1"
#define T2 "/t2"
#define T3 "/t3"
#define TIME(s) ( s | (1<<31) )

extern ems_u32* outptr;
extern int* memberlist;

/*****************************************************************************/
/*                                                                           */
/* Procedure V24_write(channel,buffer)                                       */
/*                                                                           */
/*           channel: channelnummer                                          */
/*           buffer : Buffer                                                 */
/*                                                                           */
/* Confirmation: Fehlercode (EMS), ErrorVariable, errno (System),            */
/*               Responsstring                                               */
/*                                                                           */
/*****************************************************************************/

plerrcode proc_V24_write(p)
int *p;
{
int fd;
int channel;
char *buffer;
int bufsiz; 
char *fname=(char *)malloc(5*sizeof(char));
int i;
int error=0,errorhlp;


channel= memberlist[*(++p)];
buffer=(char *)malloc( (*(++p)+1)*sizeof(char) ); 

extractstring(buffer,p);
D(D_REQ,printf("extract:---%s---\n",buffer);)

switch (channel) {
   case 1 : strcpy(fname,T1);
            break;
   case 2 : strcpy(fname,T2);
            break;
   case 3 : strcpy(fname,T3);
            break;
   default: return(-1);
}

if ((fd = open(fname,0x22)) < 0) {
  D(D_REQ,printf("cannot open file - %d\n",errno);)
  error= (-1);
}

if (error != (-1) ){
   bufsiz=strlen(buffer);D(D_REQ,printf("strlen(buffer)=%d\n",bufsiz);)
   errorhlp=write(fd,buffer,bufsiz);
   if (errorhlp==(-1) ) error=(-1);
}

*outptr++ = error;
if ( error==(-1) ) *outptr++ = errno;
else *outptr++ = 0;

free(buffer);
close(fd);

return(plOK);       
}

plerrcode test_proc_V24_write(p)
int *p;
{   
   if ( p[0] < 2 )                  return(plErr_ArgNum);
   if ( p[0] >=23 )                 return(plErr_ArgNum);
   if ( !memberlist )               return(plErr_NoISModulList);
   if ( !((memberlist[p[1]]<=3) &&
          (memberlist[p[1]]>=1)) )      return(plErr_BadHWAddr);
   if ( get_mod_type(memberlist[p[1]]) != CHANNEL) {
      *outptr++=memberlist[p[1]];
                                    return(plErr_BadModTyp);
   }
   return(plOK);
}
char name_proc_V24_write[]="V24_write";
int ver_proc_V24_write=1;

/*****************************************************************************/
/*                                                                           */
/* Procedure V24_read(channel)                                               */
/*                                                                           */
/*           channel: channelnummer                                          */
/*                                                                           */
/* Confirmation: Fehlercode (EMS), ErrorVariable, errno (System),            */
/*               Responsstring                                               */
/*                                                                           */
/*****************************************************************************/

plerrcode proc_V24_read(p)
int *p;
{

int channel;
char *buf=(char *)malloc(sizeof(char));
char *buffer=(char *)malloc(81*sizeof(char));
int bufsiz;
int *xdrstr=(int *)malloc(81*sizeof(int));
int fd;
char *fname=(char *)malloc(5*sizeof(char));
int i=0,j;
int error=0;
int hlp;

channel=memberlist[p[1]];

switch (channel) {
   case 1 : strcpy(fname,T1);
            break;
   case 2 : strcpy(fname,T2);
            break;
   case 3 : strcpy(fname,T3);
            break;
   default: return(-1);
}

if ((fd = open(fname,0x41)) < 0) {
  D(D_REQ,printf("cannot open file - %d\n",errno);)
  error=(-1);
}

if( error!=(-1) ){
   while ( (_gs_rdy(fd)>0) && hlp ) { 
      hlp=read(fd,buf,1);
      buffer[i++]=*buf;
      D(D_REQ,printf("V24_read in SCHLEIFE:---%c---%x---\n",*buf,*buf);)
   }
}

buffer[i]='\0'; 
D(D_REQ,printf("buffer= ---%s--- i=%d\n",&buffer[0],i);)
close(fd);

outstring(xdrstr,buffer);
bufsiz=strlwlen(&xdrstr[0]);

*outptr++ = error;/* ???? */
if (error==(-1) ) *outptr++ = errno;
else *outptr++ =0;
for (j=0;j<bufsiz;j++) *outptr++ = xdrstr[j];

free(buf);
free(buffer);
free(xdrstr);
free(fname);

return(plOK);       
}

plerrcode test_proc_V24_read(p)
int *p;
{
   if ( p[0]!=1 )                   return(plErr_ArgNum);
   if ( !memberlist )               return(plErr_NoISModulList);  
   if ( !((memberlist[p[1]]<=3) &&
          (memberlist[p[1]]>=1)) )      return(plErr_BadHWAddr);
   if ( get_mod_type(memberlist[p[1]]) != CHANNEL) {
      *outptr++=memberlist[p[1]];
                                    return(plErr_BadModTyp);
   }

   return(plOK);
}
char name_proc_V24_read[]="V24_read";
int ver_proc_V24_read=1;




/*****************************************************************************/
/*                                                                           */
/* Procedure V24_WriteRead(channel,sleep,buffer)                             */
/*                                                                           */
/*           channel: channelnummer                                          */
/*           sleep  : sleep/256 Zeitraum bis zum Zuruecklesen                */
/*           buffer : COMMANDstring                                          */
/*                                                                           */
/* Confirmation: Fehlercode (EMS), ErrorVariable, errno (System),            */
/*               Responsstring                                               */
/*                                                                           */
/*****************************************************************************/

plerrcode proc_V24_WriteRead(p)
int *p;
{

int channel;
int fd; /* Filedescriptor */
char *fname=(char *)malloc(5*sizeof(char));
int error=0,errorhlp;
char *buf=(char *)malloc(sizeof(char));
char *buffer=(char *)malloc(81*sizeof(char));
char *bufferout=(char *)malloc(81*sizeof(char));
int bufsiz;
int *xdrstr=(int *)malloc(81*sizeof(int));
int s;
int i=0,j;
int help,hlp;

/* --- Channelnummer ---------------------------------------------------- */
channel= memberlist[*(++p)];

/* --- Sleep bis zum Zuruecklesen (1/256 sec) --------------------------- */
s= *(++p);

/* --- Characterpointer fuer den Befehlsstring -------------------------- */
buffer=(char *)malloc( (*(++p)+1)*sizeof(char) ); 
buffer[0]='\0';

extractstring(buffer,p);
{int t=strlen(buffer); for(j=0;j<t;j++)
 D(D_REQ,printf("Buf: ---%c---%d---\n",buffer[j],buffer[j]);)}

D(D_REQ,printf("extract:---%s---\n",buffer);)

/* --- Channel waehlen -------------------------------------------------- */
switch (channel) {
   case 1 : strcpy(fname,T1);
            break;
   case 2 : strcpy(fname,T2);
            break;
   case 3 : strcpy(fname,T3);
            break;
}

/* --- Open fuer write ------------------------------------------------- */
if ((fd = open(fname,0x22)) < 0) {
  error=(-1);
  D(D_REQ,printf("cannot open file - %d\n",errno);)
}

/* --- write ----------------------------------------------------------- */
if ( error!=(-1) ){
   bufsiz=strlen(buffer);
   errorhlp=write(fd,buffer,bufsiz);
   if ( errorhlp==(-1) ) {
      error=(-1); 
      D(D_REQ,printf("Fehler bei write\n");)fflush(stdout);
   }
   close(fd);
}

/* --- sleep s/256 sec, da manche Geraete nicht schnell genug antworten */
tsleep( TIME(s) );

/* --- Open fuer read ------------------------------------------------- */
if ( error !=(-1) ){
   if ((fd = open(fname,0x41)) < 0) {
     error=(-1);
     D(D_REQ,printf("cannot open file - %d\n",errno);)
   }
}

if (error != (-1) ){
   while ( (_gs_rdy(fd)>0)&& hlp &&(i<=80) ) {
      hlp=read(fd,buf,1);
      bufferout[i++]=*buf;
   }
}
bufferout[i]='\0';
D(D_REQ,printf("bufferout= ---%s--- i=%d\n",bufferout,i);)

close(fd);

*outptr++ = error;
if ( error==(-1) ) *outptr++ = errno;
else *outptr++ = 0;

outstring(xdrstr,bufferout);
bufsiz=strlwlen(&xdrstr[0]);
for (j=0;j<bufsiz;j++) *outptr++ = xdrstr[j];

free(fname);
free(xdrstr);
free(buffer);
free(buf);
free(bufferout);

return(plOK);       
}

plerrcode test_proc_V24_WriteRead(p)
int *p;
{
   if ( p[0] < 3 )                  return(plErr_ArgNum);
   if ( p[0] >=23 )                 return(plErr_ArgNum);
   if ( !memberlist )               return(plErr_NoISModulList);
   if ( !((memberlist[p[1]]<=3) &&
          (memberlist[p[1]]>=1)) )      return(plErr_BadHWAddr);
   if ( get_mod_type(memberlist[p[1]]) != CHANNEL) {
      *outptr++=memberlist[p[1]];
                                    return(plErr_BadModTyp);
   }


   return(plOK);
}
char name_proc_V24_WriteRead[]="V24_WriteRead";
int ver_proc_V24_WriteRead=1;




/*****************************************************************************/
/*                                                                           */
/* Procedure V24_initialize(channel)                                         */
/*                                                                           */
/*           channel: channelnummer                                          */
/*           buffer : COMMANDstring                                          */
/*                                                                           */
/* Confirmation: Fehlercode (EMS), ErrorVariable, errno (System),            */
/*               Responsstring                                               */
/*                                                                           */
/*****************************************************************************/

plerrcode proc_V24_initialize(p)
int *p;
{

int channel;   /* Nummer des Channels */
char *buffer=(char *)malloc(81*sizeof(char));
int *xdrstr=(int *)malloc(81*sizeof(int));
char *c;
char *chelp;

char *b1,*b2,*b3,*b4,*b5;
int error=0;

b1=(char *)malloc(60*sizeof(char));
b2=(char *)malloc(60*sizeof(char));
b3=(char *)malloc(60*sizeof(char));
b4=(char *)malloc(60*sizeof(char));
b5=(char *)malloc(60*sizeof(char));

strcpy(b1,"deiniz /t");
strcpy(b2,"xmode /t");
strcpy(b3,"xmode /t");
strcpy(b4,"iniz /t");
strcpy(b5,"xmode /t");

/* --- Channelnummer ---------------------------------------------------- */
channel= memberlist[*(++p)];

/* --- Channel waehlen -------------------------------------------------- */
switch (channel) {
   case 1 : strcpy(c,"1");
            break;
   case 2 : strcpy(c,"2");
            break;
   case 3 : strcpy(c,"3");
            break;
}
strcat(b1,c);
strcat(b2,c);
strcat(b3,c);
strcat(b4,c);
strcat(b5,c);

/* --- CommandString uebernehmen ---------------------------------------- */
extractstring(buffer,++p);
D(D_REQ,printf("extract: ---%s---\n",buffer);)

strcat(b2," ");
while( strncmp(buffer," ",1) ) {strncat(b2,buffer,1);buffer++;}
buffer++;strncat(b2," ",1);
while( strncmp(buffer," ",1) ) {strncat(b2,buffer,1);buffer++;} 

strcat(b3," lf noecho nopause");
strcat(b3,buffer);

D(D_REQ,printf("---%s---\n",b1);)
D(D_REQ,printf("---%s---\n",&b2[0]);)
D(D_REQ,printf("---%s---\n",b3);)
D(D_REQ,printf("---%s---\n",b4);)
D(D_REQ,printf("---%s---\n",b5);)

system(b1);
system(&b2[0]);
system(b3);
system(b4);
system(b5);

return(plOK);       
}

plerrcode test_proc_V24_initialize(p)
int *p;
{
   if ( p[0] < 3 )                  return(plErr_ArgNum);
   if ( !memberlist )               return(plErr_NoISModulList);
   if ( !((memberlist[p[1]]<=3) &&
          (memberlist[p[1]]>=1)) )      return(plErr_BadHWAddr);
   if ( get_mod_type(memberlist[p[1]]) != CHANNEL) {
      *outptr++=memberlist[p[1]];
                                    return(plErr_BadModTyp);
   }


   return(plOK);
}
char name_proc_V24_initialize[]="V24_initialize";
int ver_proc_V24_initialize=1;


