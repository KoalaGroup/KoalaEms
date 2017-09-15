/*
 * fbtblock.c
 * created 29.01.1999
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fbtblock.c,v 1.5 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../procprops.h"
#include "../../../lowlevel/fastbus/fastbus.h"
#include <dev/pci/zelsync.h>

extern ems_u32* outptr;
static struct timeval tvg;

RCS_REGISTER(cvsid, "procs/fastbus/test_sfi")

/*****************************************************************************/
static int blockreadt_direct(sfi_info* sfi, int max)
{
int *status, flags, num, ss, mask, words, i;

mask=0x7000000<<sfi->inbit;
/* wait for end of transfer */
if ((sfi->syncinf->base[SYNC_CSR]&mask)==0)
  {
  struct timeval tv0, tv1;
  gettimeofday(&tv0, 0);
  while ((sfi->syncinf->base[SYNC_CSR]&mask)==0)
    {
    gettimeofday(&tv1, 0);
    if (((tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec))>10000000)
      {
      printf("blockread: timeout\n");
      goto fehler;
      }
    /*sleep(0);*/
    }
  }
gettimeofday(&tvg, 0);

if ((SFI_R(sfi, sequencer_status)&0x8001)!=0x8001) goto fehler;
SEQ_W(sfi, seq_out, 0x100000<<sfi->outbit);

if ((flags=SFI_R(sfi, fifo_flags))&0x10)
  {
  volatile ems_u32 d=SFI_R(sfi, read_seq2vme_fifo);
  (void) d;
  flags=SFI_R(sfi, fifo_flags);
  }
status=(int*)malloc(max*sizeof(int)*2);
words=0;
while (((flags&0x10)==0) && (words<max*2))
  {
  status[words++]=SFI_R(sfi, read_seq2vme_fifo);
  flags=SFI_R(sfi, fifo_flags);
  }
if (max!=words) printf("read %d statuswords\n", words);
for (i=0; i<words; i++)
  {
  num=status[i]&0xffffff;
  ss=(status[i]>>24)&0x37;
  if (ss!=0) printf("status[%d]=%d; num=%d\n", i, ss, num);
  }
free(status);
return 0;

fehler:
sfi_sequencer_status(sfi, 1);
sfi_restart_sequencer(sfi);
sfi->status.error=sfi_error_sequencer;
sfi->status.ss=10;
sleep(2);
return -1;
}
/*****************************************************************************/
/*
SFItFRDBload
p[0] : No. of parameters (==4)
p[1] : Sequencer RAM address
p[2] : primary address
p[3] : secondary address
p[4] : count
p[5] : (boolean) set ECL-outbit
*/

plerrcode proc_SFItFRDBload(int* p)
{
struct sfilist list;
struct sficommand comm[20];
int res=0, i;
int pa=p[2], sa=p[3], count=p[4], out=p[5];
int addr=p[1];

list.list=comm;
i=0;
comm[i].cmd=seq_prim_dsr;      comm[i++].data=pa;
comm[i].cmd=seq_secad_w;       comm[i++].data=sa;
comm[i].cmd=seq_dma_r_clearwc; comm[i++].data=(count & 0xffffff) | 0xa<<24;
comm[i].cmd=seq_discon;        comm[i++].data=0;
comm[i].cmd=seq_store_statwc;  comm[i++].data=0;
if (out) {comm[i].cmd=seq_out;  comm[i++].data=(0x10<<sfi.outbit);}
comm[i].cmd=seq_disable_ram;   comm[i++].data=0;

list.size=i;
list.addr=addr;
if (ioctl(sfi.path, SFI_LOAD_LIST, &list)<0)
  {
  res=errno;
  printf("sfi_loadlist_FRDB: %s\n", strerror(errno));
  }
if (res)
  {
  *outptr++=res;
  return plErr_Other;
  }
else
  return plOK;
}

plerrcode test_proc_SFItFRDBload(int* p)
{
if (p[0]!=5) return(plErr_ArgNum);
return plOK;
}
#ifdef PROCPROPS
static procprop SFItFRDBload_prop={1, 1, "seq. addr., pa, sa, count", ""};

procprop* prop_proc_SFItFRDBload(void)
{
return(&SFItFRDBload_prop);
}
#endif
char name_proc_SFItFRDBload[]="SFItFRDBload";
int ver_proc_SFItFRDBload=1;

/*****************************************************************************/
/*
SFItexec
p[0] : No. of parameters (==2)
p[1] : address
p[2] : loops
*/

plerrcode proc_SFItexec(int* p)
{
int res, i, loops=p[2];
int addr=p[1];
struct timeval tv0, tv1, tv2;

gettimeofday(&tv0, 0);
for (i=1; i<loops; i++)
  {
  SEQ_W(&sfi, seq_load_address, (ems_u32)((caddr_t)(outptr+2)
                   -sfi.backbase+sfi.dma_vmebase));
  SEQ_W(&sfi, seq_enable_ram+(addr>>2), 0);
  }
SEQ_W(&sfi, seq_load_address, (ems_u32)((caddr_t)(outptr+2)
                 -sfi.backbase+sfi.dma_vmebase));
SEQ_W(&sfi, seq_enable_ram+((addr+0x100)>>2), 0);

gettimeofday(&tv1, 0);
res=blockreadt_direct(&sfi, loops);
gettimeofday(&tv2, 0);
*outptr++=((tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec))/loops;
*outptr++=((tvg.tv_sec-tv0.tv_sec)*1000000+(tvg.tv_usec-tv0.tv_usec))/loops;
*outptr++=((tv2.tv_sec-tv0.tv_sec)*1000000+(tv2.tv_usec-tv0.tv_usec))/loops;

outptr[1]=res;
outptr[0]=sscode();
outptr+=2+res;
return plOK;
}

plerrcode test_proc_SFItexec(int* p)
{
if (p[0]!=2) return(plErr_ArgNum);
return plOK;
}
#ifdef PROCPROPS
static procprop SFItexec_prop={1, -1, "seq. addr., count", ""};

procprop* prop_proc_SFItexec()
{
return(&SFItexec_prop);
}
#endif
char name_proc_SFItexec[]="SFItexec";
int ver_proc_SFItexec=1;

/*****************************************************************************/
/*****************************************************************************/
