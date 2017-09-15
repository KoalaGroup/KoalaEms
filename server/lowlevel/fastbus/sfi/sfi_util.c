/*
 * lowlevel/fastbus/sfi/sfi_util.c
 * created: 06.11.97
 * 02.02.1999 PW: FBPULSE_ added
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sfi_util.c,v 1.4 2011/04/06 20:30:23 wuestner Exp $";

#include <errno.h>
#include <unistd.h>
#include <sconf.h>
#include <rcs_ids.h>
#include "sfilib.h"
#include "../fastbus.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sfi")

/*****************************************************************************/
void SFIout_seq(u_int32_t val)
{
#ifdef SFIMAPPED
SEQ_W(&sfi, seq_out, val);
#else
sfi_command comm[1];
comm[0].cmd=seq_out;      comm[0].data=val;
if (write(sfi->path, comm, sizeof(comm))!=sizeof(comm))
  {
  perror("SFIout_seq: write sfi");
  sfi->status.error=sfi_error_unknown;
  return -1;
  }
#endif
/*if (sfi_wait_sequencer(sfi)<0) printf("SFIout_seq: Fehler\n");*/
}
/*****************************************************************************/
void FBPULSE_(sfi_info* info, int outmask)
{
SEQ_W(info, seq_out, outmask);
SEQ_W(info, seq_out, outmask<<16);
}
/*****************************************************************************/
void FBPULSE_reset_(sfi_info* info, int outmask)
{
SEQ_W(info, seq_out, outmask<<16);
}
/*****************************************************************************/
/*****************************************************************************/
