/******************************************************************************
*                                                                             *
* chi_init.c                                                                  *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* created: 05.10.94                                                           *
* last changed: 22.10.94                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include <errno.h>
#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include "globals.h"
#include "fbacc.h"
#include "chi_map.h"
#include <ssm.h>
#include "../bustrap/bustrap.h"

extern void fbtrap_hand(short* pc);
extern int fbtrap();

/*****************************************************************************/

errcode chi_neu_low_init(char* arg)
{
int arblevel, normarb;
int val, res;

T(chi_neu_low_init)

arblevel=1;
normarb=0;

if (arg)
  {
  val=0;
  cgetnum(arg, "chi_arblevel", &val);
  if (val) arblevel=val;
  if (cgetcap(arg, "chi_normarb", ':')) normarb=1;
  }
D(D_LOW, printf("chi_neu_low_init: arblevel=%d; normarb=%d\n",
    arblevel, normarb);)

setarbitrationlevel(arblevel);

res=permit(M_BASE, 0x100000, ssm_RW) &&
    permit(P_BASE, 0x10000, ssm_RW) &&
    permit(F_BASE, 0x10000, ssm_RW) &&
    permit(REG_BASE, 0xa, ssm_RW) &&
    permit(DUART_BASE, sizeof(DUART_struct), ssm_RW);

if (res!=1)
  {
  printf("chi_neu_low_init: permit failed.\n");
  return(Err_System);
  }

change_bustrap(fbtrap);
add_bustrapfunc(fbtrap_hand);

FBCLEAR();
if (!normarb)
  {
  if (FCARB()!=0)
    printf("Warning: chi_neu_low_init: cannot become FB-Master.\n");
  keep_mastership(1);
  }
init_dma();
return(OK);
}

/*****************************************************************************/

errcode chi_neu_low_done()
{
T(chi_neu_low_done)

keep_mastership(0);
FCREL();
FBCLEAR();
protect(M_BASE, 0x100000);
protect(P_BASE, 0x10000);
protect(F_BASE, 0x10000);
protect(REG_BASE, 0xa);
protect(DUART_BASE, sizeof(DUART_struct));

return(OK);
}

/*****************************************************************************/
/*****************************************************************************/
