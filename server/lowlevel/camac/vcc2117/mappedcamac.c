/******************************************************************************
*                                                                             *
* mappedcamac.c                                                               *
*                                                                             *
* created before 17.01.95                                                     *
* last changed 24.01.95                                                       *
*                                                                             *
******************************************************************************/

#include <sconf.h>
#include <errorcodes.h>

#include "../vicvsb/vic.h"

static int *CAMACbase;
static int campath;

/*****************************************************************************/

/*
int *CAMACaddr(n,a,f)
unsigned int n,a,f;
{
    return((int*)(CAMACbase+(n<<6)+(a<<2)+(f<<11)));
}
*/

/*****************************************************************************/

errcode camac_low_init(arg)
char *arg;
{
int res;
struct vic_opts x;

path=CAMDEFPATH;
if (arg) cgetstr(arg, "campath", &path);
if ((campath=open(path, 3))==-1) return(Err_System);
_gs_opt(campath,&x);
if (x.NormalTyp!=1)
  {
  close(campath);
  campath=-1;
  return(Err_System);
  }

if ((res=_getstat(campath, ss_CSR,0))==-1)
  {
  close(campath);
  campath=-1;
  return(Err_System);
  }

_setstat(campath, ss_CSR, 0, res|6);
if ((res=_getstat(campath, ss_getbase))==-1)
  {
  close(campath);
  campath=-1;
  return(Err_System);
  }
CAMACbase=(int*)(res+0xc00000);

return(OK);
}
/*****************************************************************************/

errcode camac_low_done()
{
if (campath!=-1) close(campath);
return(OK);
}

/*****************************************************************************/
/*****************************************************************************/
