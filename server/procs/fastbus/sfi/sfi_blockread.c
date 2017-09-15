/******************************************************************************
*                                                                             *
* sfi_block.c                                                                 *
*                                                                             *
* created: 12.11.97                                                           *
* last changed: 12.11.97                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

static char *rcsid="$ZEL: sfi_blockread.c,v 1.5 2005/02/11 17:57:23 wuestner Exp $";

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
static plerrcode blockread()
{
int status, flags, dma_status, num, ss, mask;

mask=0x7000000<<sfi.inbit;
/* wait for end of transfer */
if ((sfi.syncinf->base[SYNC_CSR]&mask)==0)
  {
  struct timeval tv0, tv1;
  gettimeofday(&tv0, 0);
  while ((sfi.syncinf->base[SYNC_CSR]&mask)==0)
    {
    gettimeofday(&tv1, 0);
    if (((tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec))>1000000)
      {
      printf("blockread: timeout\n");
      goto fehler;
      }
    }
  }
if ((SFI_R(&sfi, sequencer_status)&0x8001)!=0x8001) goto fehler;
SEQ_W(&sfi, seq_out)=0x100000<<sfi.outbit;


if ((flags=SFI_R(&sfi, fifo_flags))&0x10) /* FIFO empty */
  {
  dma_status=SFI_R(&sfi, read_seq2vme_fifo); /* dummy read */
  flags=SFI_R(&sfi, fifo_flags);
  if (flags&0x10) /* FIFO empty */
    {
    printf("FIFO empty\n");
    goto fehler;
    }
  }
dma_status=SFI_R(&sfi, read_seq2vme_fifo);
num=dma_status&0xffffff;
ss=(dma_status>>24)&0x3f;
if (ss&8) num--; /* stopped by limit counter */
if (ss&0x30)
  {
  if (ss&0x10) printf("FB timeout\n");
  if (ss&0x20) printf("VME timeout\n");
  goto fehler;
  }
*outptr++=ss&7;
*outptr++=num;
/* memcpy(outptr, lowbuf_extrabuffer(), num*sizeof(int)); */
outptr+=num;
/* NIM 2 zuruecknehmen */
SEQ_W(&sfi, seq_out)=0x2000000;
return plOK;

fehler:
sfi_sequencer_status(&sfi, 1);
sfi_restart_sequencer(&sfi);
SEQ_W(&sfi, seq_out)=0x100000<<sfi.outbit;
sleep(2); outptr+=20;
return plErr_HW;
}

/*****************************************************************************/
/*
 * p[0] : No. of parameters (==3)
 * p[1] : primary address
 * p[2] : secondary address
 * p[3] : max. count
 */
plerrcode proc_FRDB(ems_u32* p)
{
#error darf eigentlich nur FRDB(p[1], p[2], p[3]) aufrufen
{
int i;
for (i=0; i<50; i++) {outptr[i]=0xdeadbeef;}
}
/* NIM 2 u. 3 setzen */
SEQ_W(&sfi, seq_out)=0x600;
SFI_W(&sfi, sequencer_reset)=0; /* stop sequencer */
SEQ_W(&sfi, seq_load_address)=(u_int32_t)((caddr_t)(outptr+2)
                 -sfi.backbase+sfi.dma_vmebase);
SEQ_W(&sfi, seq_prim_dsr)=p[1];
SEQ_W(&sfi, seq_secad_w)=p[2];
SEQ_W(&sfi, seq_dma_r_clearwc)=(p[3] & 0xffffff) | 0xa<<24;
SEQ_W(&sfi, seq_discon)=0;
SEQ_W(&sfi, seq_store_statwc)=0;
/* ECL sfi.outbit setzen *//* und NIM 3 zuruecknehmen */
SEQ_W(&sfi, seq_out)=(0x10<<sfi.outbit)+0x4000000;
SFI_W(&sfi, sequencer_enable)=0; /* start sequencer */
return blockread();
}
/*****************************************************************************/
/*
 * p[0] : No. of parameters (==3)
 * p[1] : primary address
 * p[2] : max. count
 */
plerrcode proc_FRDB_S(ems_u32* p)
{
#error darf eigentlich nur FRDB(p[1], p[2]) aufrufen
/* NIM 2 u. 3 setzen */
SEQ_W(&sfi, seq_out)=0x600;
SFI_W(&sfi, sequencer_reset)=0; /* stop sequencer */
SEQ_W(&sfi, seq_load_address)=(u_int32_t)((caddr_t)(outptr+2)
                 -sfi.backbase+sfi.dma_vmebase);
SEQ_W(&sfi, seq_prim_dsr)=p[1];
SEQ_W(&sfi, seq_dma_r_clearwc)=(p[3] & 0xffffff) | 0xa<<24;
SEQ_W(&sfi, seq_discon)=0;
SEQ_W(&sfi, seq_store_statwc)=0;
/* ECL sfi.outbit setzen *//* und NIM 3 zuruecknehmen */
SEQ_W(&sfi, seq_out)=(0x10<<sfi.outbit)+0x4000000;
SFI_W(&sfi, sequencer_enable)=0; /* start sequencer */
return blockread();
}
/*****************************************************************************/
/*
 * p[0] : No. of parameters (==3)
 * p[1] : primary address
 * p[2] : secondary address
 * p[3] : max. count
 */
plerrcode proc_FRCB(ems_u32* p)
{
#error darf eigentlich nur FRCB(p[1], p[2], p[3]) aufrufen
/* NIM 2 u. 3 setzen */
SEQ_W(&sfi, seq_out)=0x600;
SFI_W(&sfi, sequencer_reset)=0; /* stop sequencer */
SEQ_W(&sfi, seq_load_address)=(u_int32_t)((caddr_t)(outptr+2)
                 -sfi.backbase+sfi.dma_vmebase);
SEQ_W(&sfi, seq_prim_csr)=p[1];
SEQ_W(&sfi, seq_secad_w)=p[2];
SEQ_W(&sfi, seq_dma_r_clearwc)=(p[3] & 0xffffff) | 0xa<<24;
SEQ_W(&sfi, seq_discon)=0;
SEQ_W(&sfi, seq_store_statwc)=0;
/* ECL sfi.outbit setzen *//* und NIM 3 zuruecknehmen */
SEQ_W(&sfi, seq_out)=(0x10<<sfi.outbit)+0x4000000;
SFI_W(&sfi, sequencer_enable)=0; /* start sequencer */
return blockread();
}
/*---------------------------------------------------------------------------*/
plerrcode test_proc_FRDB(ems_u32* p)
{
if (p[0]!=3) return plErr_ArgNum;
return plOK;
}
plerrcode test_proc_FRDB_S(ems_u32* p)
{
if (p[0]!=2) return plErr_ArgNum;
return plOK;
}
plerrcode test_proc_FRCB(ems_u32* p)
{
if (p[0]!=3) return plErr_ArgNum;
return plOK;
}

static procprop FRDB_prop={1, -1, "seq. addr., pa, sa, count", ""};
static procprop FRDB_S_prop={1, -1, "seq. addr., pa, count", ""};
static procprop FRCB_prop={1, -1, "seq. addr., pa, sa, count", ""};

procprop* prop_proc_FRDB()
{
return(&FRDB_prop);
}
procprop* prop_proc_FRDB_S()
{
return(&FRDB_S_prop);
}
procprop* prop_proc_FRCB()
{
return(&FRCB_prop);
}
char name_proc_FRDB[]="FRDB";
int ver_proc_FRDB=1;
char name_proc_FRDB_S[]="FRDB_S";
int ver_proc_FRDB_S=1;
char name_proc_FRCB[]="FRCB";
int ver_proc_FRCB=1;

/*****************************************************************************/
/*****************************************************************************/
