/******************************************************************************
*                                                                             *
* directcamac.c                                                               *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 09.11.94                                                                    *
*                                                                             *
******************************************************************************/

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>

#include "directcamac.h"
#include "../../oscompat/oscompat.h"

#ifndef OSK
char *camacbase;
#endif
static mmapresc cammem;

errcode camac_vcc2117_low_init()
{
int *res;
mmapresc zmem;
volatile int *zadr;

T(camac_low_init)

res=map_memory(CAMAC_START,CAMAC_LEN/4,&cammem);
if(!res)return(Err_System);
#ifndef OSK
camacbase=(char*)res;
#endif

zadr=map_memory(Z2_PB,1,&zmem);
if(!zadr)return(Err_System);
*zadr|=JUELICH_BIT;   /* deaktivieren */
unmap_memory(&zmem);

CCCC();
CCCZ();

return(OK);
}

/*****************************************************************************/

errcode camac_vcc2117_low_done()
{
T(camac_low_done)

unmap_memory(&cammem);

return(OK);
}

/*****************************************************************************/
/*****************************************************************************/
