/******************************************************************************
*                                                                             *
* fbtsingle.c                                                                 *
*                                                                             *
* created before: 15.11.97                                                    *
* last changed: 15.11.97                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: fbtsingle.c,v 1.4 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../procprops.h"
#include "../../../lowlevel/fastbus/fastbus.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/fastbus/test_sfi")

/*****************************************************************************/
/*
FRCL
p[0] : No. of parameters (==4)
p[1] : primary address
p[2] : secondary address
p[3] : loops
p[4] : 0: with arb and pa 1: with pa 2: without disco
outptr[0] : SS-code
outptr[1] : val
outptr[2] : time/ms
*/

plerrcode proc_FRCL(int* p)
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
      SEQ_W(&sfi, (SFI_MS0|SFI_FUNC_PA), pa);
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
      SEQ_W(&sfi, (SFI_MS0|SFI_FUNC_PA_HM), pa);
      SEQ_W(&sfi, (SFI_MS1|SFI_FUNC_RNDM), sa);
      SEQ_W(&sfi, (SFI_RD| SFI_FUNC_RNDM_DIS), 0);
      res=sfi_read_fifoword(&sfi);
      }
    gettimeofday(&tv1, 0);
    SEQ_W(&sfi, (SFI_FUNC_DISCON_RM), 0);
    break;
  case 2:
    SEQ_W(&sfi, (SFI_MS0|SFI_FUNC_PA), pa);
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
  default: return plErr_ArgRange;
  }
*outptr++=sscode();
*outptr++=res;
*outptr++=(tv1.tv_sec-tv0.tv_sec)*1000+(tv1.tv_usec-tv0.tv_usec)/1000;
return(plOK);
}

plerrcode test_proc_FRCL(int* p)
{
if (p[0]!=4) return(plErr_ArgNum);
return(plOK);
}
#ifdef PROCPROPS
static procprop FRCL_prop={0, 3, 0, 0};

procprop* prop_proc_FRCL()
{
return(&FRCL_prop);
}
#endif
char name_proc_FRCL[]="FRCL";
int ver_proc_FRCL=3;

/*****************************************************************************/
/*****************************************************************************/
