/*
 * lowlevel/fastbus/sfi/sfi_fbblockwrite.c
 * 
 * created: 21.01.1999
 */

static const char* cvsid __attribute__((unused))=
    "$ZEL: sfi_fbblockwrite.c,v 1.4 2011/04/06 20:30:23 wuestner Exp $";

#include <errno.h>
#include <stdio.h>
#include <sconf.h>
#include <rcs_ids.h>
#include "sfilib.h"
#include <dev/pci/zelsync.h>
#include "../fastbus.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sfi")

/*****************************************************************************/

static const u_int32_t mist=0x0815;

/*****************************************************************************/
int FWDB_(sfi_info* sfi, u_int32_t pa, u_int32_t sa, u_int32_t* source,
    u_int32_t count)
{
int i;
printf("FWDB_(pa=%d; sa=%d, count=%d\n)", pa, sa, count);
SEQ_W(sfi, seq_prim_dsr, pa);
SEQ_W(sfi, seq_secad_w, sa);
for (i=0; i<count; i++)
  {
  SEQ_W(sfi, SFI_FUNC_RNDM/*|SFI_MS0*/, source[i]); /* write data to sequencer */
  printf("wrote %d\n", source[i]);
  if ((i&0xff)==0xff)                           /* each 255. check half full */
    {
    /*sfi_wait_fifo_half_empty(info);*/

    }
  SEQ_W(sfi, seq_discon, mist);
  }
sfi_wait_sequencer(sfi);
return 0;
}
/*****************************************************************************/
int FWCB_(sfi_info* sfi, u_int32_t pa, u_int32_t sa, u_int32_t* source,
    u_int32_t count)
{
int i;
SEQ_W(sfi, seq_prim_csr, pa);
SEQ_W(sfi, seq_secad_w, sa);
for (i=0; i<count; i++)
  {
  SEQ_W(sfi, SFI_FUNC_RNDM|SFI_MS0, source[i]); /* write data to sequencer */
  if ((i&0xff)==0xff)                           /* each 255. check half full */
    {
    /*sfi_wait_fifo_half_empty(info);*/

    }
  SEQ_W(sfi, seq_cleanup_dis, mist);
  }
sfi_wait_sequencer(sfi);
return 0;
}
/*****************************************************************************/
/*****************************************************************************/
