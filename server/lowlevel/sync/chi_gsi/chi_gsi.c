/******************************************************************************
*                                                                             *
* gsi_init.c                                                                  *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* created: 11.10.94                                                           *
* last changed: 24.10.94                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include <errno.h>
#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <ssm.h>
#include "../bustrap/bustrap.h"
#include "../../trigger/chi/gsi/gsitrigger.h"

/*****************************************************************************/

static int spaces[4];

/*****************************************************************************/

errcode chi_gsi_low_init(char* arg)
{
int i, ok;

T(chi_gsi_low_init)

/* search for Triggermodule */

ok=0;
for (i=0; i<=3; i++) spaces[i]=0;
for (i=1; i<=3; i++)
  {
  if (check_access(GSI_BASE+0x100*i, 4, 1)==0)
    {
    D(D_MEM, printf("Triggermodule found at space %d\n", i);)
    permit(GSI_BASE+0x100*i, 16, ssm_RW);
    spaces[i]=1; ok=1;
    }
  }
if (!ok) printf("chi_gsi_low_init: Warning: no Triggermodule found.\n");
/*return(Err_HWTestFailed);*/
return(OK);
}

/*****************************************************************************/

errcode chi_gsi_low_done()
{
int i;

T(chi_gsi_low_done)
for (i=1; i<=3; i++)
  {
  if (spaces[i]) protect(GSI_BASE+0x100*i, 16);
  }
return(OK);
}

/*****************************************************************************/

int is_gsi_space(int space)
{
return(spaces[space]);
}

/*****************************************************************************/
/*****************************************************************************/
