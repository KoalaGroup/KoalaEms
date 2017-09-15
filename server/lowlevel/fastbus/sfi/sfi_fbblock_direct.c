/*
 * lowlevel/fastbus/sfi/sfi_fbblock_direct.c
 * 
 * created      20.01.1999
 * last changed 14.03.2000 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sfi_fbblock_direct.c,v 1.6 2011/04/06 20:30:23 wuestner Exp $";

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sconf.h>
#include <rcs_ids.h>
#include "sfilib.h"
#include <dev/pci/zelsync.h>
#include "../fastbus.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sfi")

/*****************************************************************************/

static const u_int32_t mist=0x0815;
extern int* fbbuf;
extern int fbbufsize;
/*****************************************************************************/
static int blockread_direct(sfi_info* sfi)
{
int flags, dma_status, num, ss, mask;

mask=0x7000000<<sfi->inbit;
/* wait for end of transfer */
if ((sfi->syncinf->base[SYNC_CSR]&mask)==0)
  {
  struct timeval tv0, tv1;
  gettimeofday(&tv0, 0);
  while ((sfi->syncinf->base[SYNC_CSR]&mask)==0)
    {
    gettimeofday(&tv1, 0);
    if (((tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec))>1000000)
      {
      printf("blockread: timeout\n");
      goto fehler;
      }
    }
  }
if ((SFI_R(sfi, sequencer_status)&0x8001)!=0x8001) goto fehler;
SEQ_W(sfi, seq_out, 0x100000<<sfi->outbit);
if ((flags=SFI_R(sfi, fifo_flags))&0x10) /* FIFO empty */
  {
  dma_status=SFI_R(sfi, read_seq2vme_fifo); /* dummy read */
  flags=SFI_R(sfi, fifo_flags);
  if (flags&0x10) /* FIFO empty */
    {
    printf("FIFO empty\n");
    goto fehler;
    }
  }
dma_status=SFI_R(sfi, read_seq2vme_fifo);
num=(dma_status&0xffffff) + 1;
ss=(dma_status>>24)&0x37; /* ss | FB timeout | VME timeout */
/* printf("dma_status=%08x\n", dma_status);*/
if (dma_status&0x08000000) num--; /* DMA stopped by limit counter */
if (ss&0x30) /* VME or FB timeout */
  {
  if (ss&0x10) printf("FB timeout\n");
  if (ss&0x20) printf("VME timeout\n");
  goto fehler;
  }
sfi->status.ss=ss;
sfi->status.error=ss?sfi_error_sequencer:sfi_error_ok;
/* NIM 2 zuruecknehmen */
/* SEQ_W(&sfi, seq_out, 0x2000000); */
return num; /* for compatibility witch CHI code *//*XXX*/

fehler:
sfi_sequencer_status(sfi, 1);
sfi_restart_sequencer(sfi);
sfi->status.error=sfi_error_sequencer;
sfi->status.ss=10;
sleep(2);
return 0;
}
/*****************************************************************************/
int FRDB_direct(sfi_info* sfi, u_int32_t pa, u_int32_t sa, u_int32_t* dest,
    u_int32_t count)
{
u_int32_t* destination;
int num;
# ifdef SFISWAP
int i, *help;
#endif
/* NIM 2 u. 3 setzen */
/* SEQ_W(sfi, seq_out, 0x600); */
#ifdef LOWLEVELBUFFER
destination=dest;
#else
destination=fbbuf;
{
int i;
for (i=0; i<count; i++) destination[i]=0xffffffff;
}
if (fbbufsize<count)
  {
  printf("FRDB_direct: count=%d but fbbufsize=%d\n", count, fbbufsize);
  return -1;
  }
#endif
SFI_W(sfi, sequencer_reset, 0); /* stop sequencer */
SEQ_W(sfi, seq_load_address, (u_int32_t)((caddr_t)destination
                 -sfi->backbase+sfi->dma_vmebase));
SEQ_W(sfi, seq_prim_dsr, pa);
SEQ_W(sfi, seq_secad_w, sa);
SEQ_W(sfi, seq_dma_r_clearwc, ((count-1) & 0xffffff) | 0xa<<24);
SEQ_W(sfi, seq_discon, 0);
SEQ_W(sfi, seq_store_statwc, 0);
/* ECL sfi.outbit setzen *//* und NIM 3 zuruecknehmen */
SEQ_W(sfi, seq_out, (0x10<<sfi->outbit)+0x4000000);
SFI_W(sfi, sequencer_enable, 0); /* start sequencer */

num=blockread_direct(sfi);
if (destination[num-2]==0xffffffff)
  {
  printf("FRDB_direct: missing word; num=%d; pa=%d; ss=%d\n",
      num,
      SFI_R(sfi, last_primadr),
      sfi->status.ss);
  }
#ifdef LOWLEVELBUFFER
# ifdef SFISWAP
  for (i=0; i<num; i++) {*dest=ntohl(*dest); dest++}
# endif
#else
# ifdef SFISWAP
  help=fbbuf;
  for (i=0; i<num; i++) *dest++=ntohl(*help++);
# else
  if (num) memcpy(dest, fbbuf, num*sizeof(u_int32_t));
# endif
#endif
return num;
}
/*****************************************************************************/
int FRCB_direct(sfi_info* sfi, u_int32_t pa, u_int32_t sa, u_int32_t* dest,
    u_int32_t count)
{
u_int32_t* destination;
int num;
# ifdef SFISWAP
int i, *help;
#endif
/* NIM 2 u. 3 setzen */
/*SEQ_W(sfi, seq_out, 0x600);*/
#ifdef LOWLEVELBUFFER
destination=dest;
#else
destination=fbbuf;
if (fbbufsize<count)
  {
  printf("FRCB_direct: count=%d but fbbufsize=%d\n", count, fbbufsize);
  return -1;
  }
#endif
SFI_W(sfi, sequencer_reset, 0); /* stop sequencer */
SEQ_W(sfi, seq_load_address, (u_int32_t)((caddr_t)destination
                 -sfi->backbase+sfi->dma_vmebase));
SEQ_W(sfi, seq_prim_csr, pa);
SEQ_W(sfi, seq_secad_w, sa);
SEQ_W(sfi, seq_dma_r_clearwc, ((count-1) & 0xffffff) | 0xa<<24);
SEQ_W(sfi, seq_discon, 0);
SEQ_W(sfi, seq_store_statwc, 0);
/* ECL sfi.outbit setzen *//* und NIM 3 zuruecknehmen */
SEQ_W(sfi, seq_out, (0x10<<sfi->outbit)+0x4000000);
SFI_W(sfi, sequencer_enable, 0); /* start sequencer */

num=blockread_direct(sfi);
#ifdef LOWLEVELBUFFER
# ifdef SFISWAP
  for (i=0; i<num; i++) {*dest=ntohl(*dest); dest++}
# endif
#else
# ifdef SFISWAP
  help=fbbuf;
  for (i=0; i<num; i++) *dest++=ntohl(*help++);
# else
  if (num) memcpy(dest, fbbuf, num*sizeof(u_int32_t));
# endif
#endif
return num;
}
/*****************************************************************************/
int FRDB_S_direct(sfi_info* sfi, u_int32_t pa, u_int32_t* dest,
    u_int32_t count)
{
u_int32_t* destination;
int num;
# ifdef SFISWAP
int i, *help;
#endif
/* NIM 2 u. 3 setzen */
/*SEQ_W(sfi, seq_out, 0x600);*/
#ifdef LOWLEVELBUFFER
destination=dest;
#else
destination=fbbuf;
if (fbbufsize<count)
  {
  printf("FRDB_S_direct: count=%d but fbbufsize=%d\n", count, fbbufsize);
  return -1;
  }
#endif
SFI_W(sfi, sequencer_reset, 0); /* stop sequencer */
SEQ_W(sfi, seq_load_address, (u_int32_t)((caddr_t)destination
                 -sfi->backbase+sfi->dma_vmebase));
SEQ_W(sfi, seq_prim_dsr, pa);
SEQ_W(sfi, seq_dma_r_clearwc, ((count-1) & 0xffffff) | 0xa<<24);
SEQ_W(sfi, seq_discon, 0);
SEQ_W(sfi, seq_store_statwc, 0);
/* ECL sfi.outbit setzen *//* und NIM 3 zuruecknehmen */
SEQ_W(sfi, seq_out, (0x10<<sfi->outbit)+0x4000000);
SFI_W(sfi, sequencer_enable, 0); /* start sequencer */

num=blockread_direct(sfi);
#ifdef LOWLEVELBUFFER
# ifdef SFISWAP
  for (i=0; i<num; i++) {*dest=ntohl(*dest); dest++}
# endif
#else
# ifdef SFISWAP
  help=fbbuf;
  for (i=0; i<num; i++) *dest++=ntohl(*help++);
# else
  if (num) memcpy(dest, fbbuf, num*sizeof(u_int32_t));
# endif
#endif
return num;
}
/*****************************************************************************/
/*****************************************************************************/
