/******************************************************************************
*                                                                             *
* sfi_blockwrite.c                                                            *
*                                                                             *
* created: 12.11.97                                                           *
* last changed: 13.11.97                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

static char *rcsid="$ZEL: sfi_blockwrite.c,v 1.5 2005/02/11 17:57:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <stdio.h>
#include <errorcodes.h>
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../../lowlevel/fastbus/sfi/sfilib.h"
#include "../../procprops.h"
#include <dev/pci/zelsync.h>

extern ems_u32* outptr;

#error sollte in lowlevel sein

/*****************************************************************************/
static plerrcode blockwrite(int num, int* p)
{
int status, flags, n, ss, weiter=1;
plerrcode res=plOK;

for (n=0; (n<num) && weiter; n++)
  {
  SEQ_W(&sfi, (seq_rndm_w|SFI_MS0))=p[n];
  if ((n&0xff)==0xff)
    {
    while (SFI_R(&sfi, fifo_flags)&4)
      {
      if ((SFI_R(&sfi, sequencer_status)&1)==0) weiter=0;
      }
    }
  }
if (weiter) SEQ_W(&sfi, seq_cleanup_dis)=0;

while (((status=SFI_R(&sfi, sequencer_status))&0x4001)==0x4001);

if (((status=SFI_R(&sfi, sequencer_status))&0x8001)!=0x8001)
  {
  printf("status=%04x\n", status&0xffff);
  switch (status&0xf0)
    {
    case 0x20: /* primary address error */
      {
      int ss;
      sfi_sequencer_status(&sfi, 1);
      ss=(sfi.status.fb1>>4)&7; /* ss code */
      *outptr++=ss?ss:8;
      *outptr++=0;
      }
      break;
    case 0x40: /* data cycle error */
      {
      int ss=(SFI_R(&sfi, fastbus_status_2)>>4)&7;
      *outptr++=ss?ss:8;
      *outptr++=-1;
      }
      break;
    default:/* unknown error */
      sfi_sequencer_status(&sfi, 1);
      res=plErr_Other;
      break;
    }
  sfi_restart_sequencer(&sfi);
  }
else
  {
  *outptr++=0;
  *outptr++=num;
  }
return res;
}

/*****************************************************************************/
/*
 * p[0] : No. of parameters (>=3)
 * p[1] : primary address
 * p[2] : secondary address
 * p[3] : count
 * p[4]... : data
 */
plerrcode proc_FWDB(ems_u32* p)
{
int i;

SEQ_W(&sfi, seq_prim_dsr)=p[1];
SEQ_W(&sfi, seq_secad_w)=p[2];
return blockwrite(p[3], p+4);
}
/*****************************************************************************/
/*
 * p[0] : No. of parameters (>=3)
 * p[1] : primary address
 * p[2] : secondary address
 * p[3] : count
 * p[4]... : data
 */
plerrcode proc_FWCB(ems_u32* p)
{
SEQ_W(&sfi, seq_prim_csr)=p[1];
SEQ_W(&sfi, seq_secad_w)=p[2];
return blockwrite(p[3], p+4);
}
/*---------------------------------------------------------------------------*/
plerrcode test_proc_FWDB(ems_u32* p)
{
if ((p[0]<3) || (p[0]!=p[3]+3)) return plErr_ArgNum;
return plOK;
}
plerrcode test_proc_FWCB(ems_u32* p)
{
if ((p[0]<3) || (p[0]!=p[3]+3)) return plErr_ArgNum;
return plOK;
}

static procprop FWDB_prop={1, -1, "pa, sa, count, data ...", ""};
static procprop FWCB_prop={1, -1, "pa, sa, count, data ...", ""};

procprop* prop_proc_FWDB()
{
return(&FWDB_prop);
}
procprop* prop_proc_FWCB()
{
return(&FWCB_prop);
}
char name_proc_FWDB[]="FWDB";
int ver_proc_FWDB=1;
char name_proc_FWCB[]="FWCB";
int ver_proc_FWCB=1;

/*****************************************************************************/
/*****************************************************************************/
