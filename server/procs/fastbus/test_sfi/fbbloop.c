/*
 * fbbloop.c
 * 
 * created 18.01.1999
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fbbloop.c,v 1.4 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../procprops.h"
#include "../../../lowlevel/fastbus/fastbus.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/fastbus/test_sfi")

/*****************************************************************************/
/*
FBBLOOP
p[0] : No. of parameters (==4)
p[1] : primary address
p[2] : secondary address
p[3] : count
p[4] : loops
outptr[0] : time/(us/loop)
*/

plerrcode proc_FBBLOOP(int* p)
{
register int i, loops, pa, sa, res, count;
struct timeval tv0, tv1;

pa=p[1]; sa=p[2]; count=p[3]; loops=p[4];

gettimeofday(&tv0, 0);
for (i=0; i<loops; i++)
  {
  res=FRDB(pa, sa, outptr, count);
  if (res!=count)
    {
    *outptr++=-1;
    *outptr++=res;
    *outptr++=sscode();
    return plOK;
    }
  }
gettimeofday(&tv1, 0);
*outptr++=((tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec))/loops;
*outptr++=sscode();
return plOK;
}

plerrcode test_proc_FBBLOOP(int* p)
{
if (p[0]!=4) return(plErr_ArgNum);
return(plOK);
}
#ifdef PROCPROPS
static procprop FBBLOOP_prop={0, 3, 0, 0};

procprop* prop_proc_FBBLOOP()
{
return(&FBBLOOP_prop);
}
#endif
char name_proc_FBBLOOP[]="FBBLOOP";
int ver_proc_FBBLOOP=1;
/*****************************************************************************/
/*****************************************************************************/
