/*
 * fbloop.c
 * 
 * created 18.01.1999
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fbloop.c,v 1.4 2011/04/06 20:30:32 wuestner Exp $";

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
FBLOOP
p[0] : No. of parameters (==4)
p[1] : primary address
p[2] : secondary address
p[3] : loops
p[4] : read   0: with arb and pa 1: with pa 2: without disco 3: miniloop
       write 10, 11, 12
outptr[0] : ss-code
outptr[1] : time/ms
*/

plerrcode proc_FBLOOP(int* p)
{
register int i, loops, pa, sa, res=0;
struct timeval tv0, tv1;

pa=p[1]; sa=p[2]; loops=p[3];
switch (p[4])
  {
  case 0:
    gettimeofday(&tv0, 0);
    for (i=0; i<loops; i++)
      {
      SEQ_W(&sfi, (        SFI_FUNC_PA), pa);
      SEQ_W(&sfi, (SFI_MS1|SFI_FUNC_RNDM), sa);
      SEQ_W(&sfi, (SFI_RD| SFI_FUNC_RNDM_DIS), 0);
      res=sfi_read_fifoword(&sfi);
      }
    gettimeofday(&tv1, 0);
    break;
  case 1:
    gettimeofday(&tv0, 0);
    for (i=0; i<loops; i++)
      {
      SEQ_W(&sfi, (        SFI_FUNC_PA_HM), pa);
      SEQ_W(&sfi, (SFI_MS1|SFI_FUNC_RNDM), sa);
      SEQ_W(&sfi, (SFI_RD| SFI_FUNC_RNDM_DIS), 0);
      res=sfi_read_fifoword(&sfi);
      }
    gettimeofday(&tv1, 0);
    SEQ_W(&sfi, (SFI_FUNC_DISCON_RM), 0);
    break;
  case 2:
    SEQ_W(&sfi, (          SFI_FUNC_PA), pa);
    gettimeofday(&tv0, 0);
    for (i=0; i<loops; i++)
      {
      SEQ_W(&sfi, (SFI_MS1|SFI_FUNC_RNDM), sa);
      SEQ_W(&sfi, (SFI_RD| SFI_FUNC_RNDM), 0);
      res=sfi_read_fifoword(&sfi);
      }
    gettimeofday(&tv1, 0);
    SEQ_W(&sfi, (SFI_FUNC_DISCON_RM), 0);
    break;
  case 3:
    SEQ_W(&sfi, (          SFI_FUNC_PA), pa);
    gettimeofday(&tv0, 0);
    for (i=0; i<loops; i++)
      {
      int j;
        SEQ_W(&sfi, (SFI_MS1|SFI_FUNC_RNDM), sa);
      for (j=0; j<100; j++)
        {
        SEQ_W(&sfi, (SFI_RD| SFI_FUNC_RNDM), 0);
        }
      res=sfi_read_fifo(&sfi, outptr+1, 100);
      }
    gettimeofday(&tv1, 0);
    *outptr++=res;
    if (res>0) outptr+=res;
    SEQ_W(&sfi, (SFI_FUNC_DISCON_RM), 0);
    break;
  case 4:
    SEQ_W(&sfi, (          SFI_FUNC_PA), pa);
    gettimeofday(&tv0, 0);
    for (i=0; i<loops; i++)
      {
      int j;
      for (j=0; j<100; j++)
        {
        *outptr=17;
        }
      }
    gettimeofday(&tv1, 0);
    SEQ_W(&sfi, (SFI_FUNC_DISCON_RM), 0);
    break;
  case 10:
    gettimeofday(&tv0, 0);
    for (i=0; i<loops; i++)
      {
      SEQ_W(&sfi, (        SFI_FUNC_PA), pa);
      SEQ_W(&sfi, (SFI_MS1|SFI_FUNC_RNDM), sa);
      SEQ_W(&sfi, (        SFI_FUNC_RNDM_DIS), 0);
      }
    gettimeofday(&tv1, 0);
    break;
  case 11:
    gettimeofday(&tv0, 0);
    for (i=0; i<loops; i++)
      {
      SEQ_W(&sfi, (        SFI_FUNC_PA_HM), pa);
      SEQ_W(&sfi, (SFI_MS1|SFI_FUNC_RNDM), sa);
      SEQ_W(&sfi, (        SFI_FUNC_RNDM_DIS), 0);
      }
    gettimeofday(&tv1, 0);
    SEQ_W(&sfi, (SFI_FUNC_DISCON_RM), 0);
    break;
  case 12:
    SEQ_W(&sfi, (          SFI_FUNC_PA), pa);
    gettimeofday(&tv0, 0);
    for (i=0; i<loops; i++)
      {
      SEQ_W(&sfi, (SFI_MS1|SFI_FUNC_RNDM), sa);
      SEQ_W(&sfi, (        SFI_FUNC_RNDM), 0);
      }
    gettimeofday(&tv1, 0);
    SEQ_W(&sfi, (SFI_FUNC_DISCON_RM), 0);
    break;
  default: return plErr_ArgRange;
  }
*outptr++=sscode();
*outptr++=(tv1.tv_sec-tv0.tv_sec)*1000+(tv1.tv_usec-tv0.tv_usec)/1000;
return(plOK);
}

plerrcode test_proc_FBLOOP(int* p)
{
if (p[0]!=4) return(plErr_ArgNum);
return(plOK);
}
#ifdef PROCPROPS
static procprop FBLOOP_prop={0, 3, 0, 0};

procprop* prop_proc_FBLOOP()
{
return(&FBLOOP_prop);
}
#endif
char name_proc_FBLOOP[]="FBLOOP";
int ver_proc_FBLOOP=1;

/*****************************************************************************/
/*****************************************************************************/
