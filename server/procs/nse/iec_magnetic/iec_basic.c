#include	<stdio.h>
#include	<modes.h>
#include	<errno.h>
#include	<strings.h>
#include	<signal.h>
#include        <debug.h>
#include	"tms.h"
#include	"iec.h"



#define     PATHNAME        "/iecc"
#define     SIGNAL          7
int addr=12;
/*--------------------------------------------------------------*/
void gain()
{
int i;
char *out=(char *)malloc(2048*sizeof(char));
char    combuf[256];
int     iecpath;

int hlp;
char cpoint[]={20};

    if ((iecpath = open("/iecc", 3)) == -1)  {
printf("Can't open %s\n",PATHNAME);
        exit(0);
    }

hlp=isetstt(iecpath,SS_SSRq,SIGNAL,0);
printf("ERR nach isetstt %d\n",hlp);

strcpy(combuf,"GAIN155");
printf("COMBUF ---%s---\n",combuf);
hlp=strlen(combuf); combuf[hlp]=10; combuf[hlp+1]=0;
hlp=strlen(combuf);
for(i=0;i<hlp;i++) printf("-%d- -%c- \n",combuf[i],combuf[i]);

transmit(iecpath,combuf,hlp);
printf("In GAIN nach transive: PATH %d \n",iecpath);fflush(stdout);
sleep(2);fflush(stdout);
receive(iecpath,out,2048);
printf("receive: ---%s---\n",out);

    close(iecpath);
}
/*----------------------------------------------------------------*/
transmit(addr,path,data,number)
register int    addr;
register int    path;
register char   *data;
register int    number;
{
        if (isetstt(path, SS_LisAd, addr, 0) == -1)  {
            D(D_REQ,printf("Error in SetStt call  Error code %x\n", errno);)
            return(-1);
        }
        if (write(path, data, number) == -1)  {
            D(D_REQ,printf("Error in Write Routine    Error code %x\n", errno);)
            return(-1);
        }
        if (isetstt(path, SS_LisAd, 31, 0) == -1)  {
            D(D_REQ,printf("Error in SetStt call  Error code %x\n", errno);)
            return(-1);
        }
        return(0);
}
/*----------------------------------------------------------------*/
receive(addr,path,data,number)
register int    addr;
register int    path;
register char   *data;
register int    number;
{
register int    count;

if ( isetstt(path,SS_SSRq,SIGNAL,0) == -1 )
   D(D_REQ,printf("ERROR isetstt SS_SSRq, errno %d\n",errno);)

if (isetstt(path, SS_TlkAd, addr, 0) == -1)  {
  D(D_REQ,printf("Error in SetStt call  Error code %x\n", errno);)
return(-1);
}
if ((count = read(path, data, number)) == -1)  {
   D(D_REQ,printf("ERROR in READ path %d data ---%s--- number %d\n",path, data, number);)
   D(D_REQ,printf("Error in Read Routine    Error code %x\n", errno);)
return(-1);
}
data[count] = 0;   
if (isetstt(path, SS_TlkAd, 31, 0) == -1)  {
   D(D_REQ,printf("Error in SetStt call  Error code %x\n", errno);)
return(-1);
}
return(0);
}
/*----------------------------------------------------------------*/
isetstt(path, code, par1, par2)

int     path, code;
int par1;   /* must be d4  */
char    *par2;  /* must be a2  */
{
#asm
 movem.l d2-d4,-(sp)
 move.l  a2,-(sp)
 move.l 32(sp),d2
 move.l 36(sp),a2
 os9     I$SetStt
 bcs.b error
 move.l (sp)+,a2
 movem.l (sp)+,d2-d4
 bra out

error
 and.l #$ffff,d1
 move.l d1,errno(a6)
 moveq #-1,d0
 move.l (sp)+,a2
 movem.l (sp)+,d2-d4
out
#endasm
}
/*----------------------------------------------------------------*/





