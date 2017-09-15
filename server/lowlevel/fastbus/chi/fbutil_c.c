/******************************************************************************
*                                                                             *
* fbutil_c.c                                                                  *
*                                                                             *
* created: 12.10.94                                                           *
* last changed: 24.10.94                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include <sconf.h>
#include <debug.h>
#include <setjmp.h>
#include <modultypes.h>
#include "globals.h"
#include "fbacc.h"
#include "chi_map.h"
#include "../../objects/domain/test_modul_id.h"

extern int arbitrationlevel;

/*****************************************************************************/

int get_ga_chi()
{
char garegval;

garegval=*(char*)(F_BASE+GAREG);
garegval&=0x1f;
return(garegval);
}

/*****************************************************************************/

int setarbitrationlevel(int arblevel)
{
int level;

level=arbitrationlevel;
if (arblevel>0)
  {
  arbitrationlevel=arblevel;
  P_BASE_A =(char*)P_BASE+0x40*arbitrationlevel;
  CGEOptr  =(int*)(P_BASE_A+CGEO);
  DGEOptr  =(int*)(P_BASE_A+DGEO);
  DBREGptr =(int*)(P_BASE_A+DBREG);
  CBREGptr =(int*)(P_BASE_A+CBREG);
  SECADptr =(int*)(P_BASE_A+SECAD);
  RNDMptr  =(int*)(P_BASE_A+RNDM);
  DISCONptr=P_BASE_A+DISCON;
  SSREGptr =(char*)F_BASE+SSREG;
  }
return(level);
}

/*****************************************************************************/
/* become busmaster */
int FCARB()
{
volatile char dummy;
volatile char *exarb, *pollreg0;

exarb=(char*)F_BASE+A_PROT_NORM+arbitrationlevel*0x40+EXARB;
dummy=*exarb;
pollreg0=(char*)REG_BASE+POLLREG0;
if (*pollreg0 & MASTER)
  return(0);
else
  return(-1);
}

/*****************************************************************************/
/* release busmastership */
void FCREL()
{
volatile char dummy;
volatile char *resgk;

resgk=(char*)F_BASE+RESGK;
dummy=*resgk;
}

/*****************************************************************************/

int keep_mastership(int keep)
{
volatile char* fbcr0;
int res;

fbcr0=(char*)F_BASE+FBCR0;
res=((*fbcr0&FGK)!=0);
if (keep>=0)
  {
  if (keep)
    *fbcr0|=FGK;
  else
    *fbcr0&=~FGK;
  }
return(res);
}

/*****************************************************************************/

int auto_disconnect(int automat)
{
volatile char* fbcr0;
int res;

fbcr0=(char*)F_BASE+FBCR0;
res=((*fbcr0&FAS)!=0);
if (automat>=0)
  {
  if (automat)
    *fbcr0|=FAS;
  else
    *fbcr0&=~FAS;
  }
return(res);
}

/*****************************************************************************/

int set_pip(int pip)
{
volatile char* fbcr0;
char reg, res;

fbcr0=(char*)F_BASE+FBCR0;
reg=*fbcr0;
res=reg&7;
if (pip>=0)
  {
  reg&=~7;
  reg|=pip&7;
  *fbcr0=reg;
  }
return(res);
}

/*****************************************************************************/

int ssenable(int ssreg)
{
volatile unsigned short* ssenable;
unsigned short ss;

ssenable=(unsigned short*)((char*)F_BASE+SSENABLE);
ss=*ssenable;
if (ssreg>=0) *ssenable=ssreg;
return(ss);
}

/*****************************************************************************/

void init_dma()
{
/* set dma mode */
*(volatile int*)((char*)F_BASE+WRCR)=0x01010101;
*(volatile unsigned char*)(REG_BASE+IMASKREG)|=IM6;
}

/*****************************************************************************/

int fb_modul_id(int pa)
{
jmp_buf buf;
int res;

if (pa==get_ga_chi()) return(0x68a0);

if (setjmp(buf)==0)
  {
  register_jmp(buf);
  res=FRC(pa, 0);
  unregister_jmp();
  return(res>>16);
  }
else
  {
  unregister_jmp();
  FBCLEAR();
  return(0);
  }
}

/*****************************************************************************/

int ist_aehnlich(int type, int mtype)
{
int res;

if ((type&0xffff)==mtype)
  res=0;
else
  {
  res=-1;

  switch (type)
    {
/*    case LC_TDC_1876:
      if (mtype==0x6854) res=0;
      break;*/
    default:
      res=-1;
      break;
    }
  }
return(res);
}

/*****************************************************************************/

int test_modul_id(int addr, int type)
{
int mtype;

mtype=fb_modul_id(addr);
if (mtype==0)
  {
  D(D_USER, printf("test_modul_id(%d, 0x%04x): slot is empty.\n",
      addr, type);)
  return(-1);
  }

if (ist_aehnlich(type, mtype)==-1)
  {
  D(D_USER, printf("test_modul_id(%d, 0x%04x): found 0x%04x.\n",
      addr, type, mtype);)
  return(-1);
  }
else
  return(0);
}

/*****************************************************************************/
/*****************************************************************************/
