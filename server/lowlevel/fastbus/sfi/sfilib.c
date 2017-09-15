/*
 * lowlevel/fastbus/sfi/sfilib.c
 * 
 * created      ?
 * last changed 14.03.2000 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sfilib.c,v 1.5 2011/04/06 20:30:23 wuestner Exp $";

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <rcs_ids.h>
#include <sconf.h>
#include "sfilib.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sfi")

/*****************************************************************************/

static const u_int32_t mist=0x0815;

/*****************************************************************************/

int sfi_open(char* path, sfi_info* info)
{
info->pathname=path;
if ((info->path=open(info->pathname, O_RDWR))<0)
  {
  printf("sfi_open: open(name=%s, prot=%d) failed\n", info->pathname, O_RDWR);
  return -1;
  }
if (fcntl(info->path, F_SETFD, FD_CLOEXEC)<0)
  {
  printf("sfilib.c: fcntl(info->path, FD_CLOEXEC): %s\n", strerror(errno));
  }

#ifdef SFIMAPPED
printf("sfi_open: try mmap(%d, 0x%x, %d, %d, %d, %d)\n", 0, 0x20000,
    PROT_READ|PROT_WRITE, 0, info->path, 0);
if ((info->base=mmap(0, 0x20000, PROT_READ|PROT_WRITE, 0, info->path, 0))
    ==(caddr_t)-1)
  {
  printf("sfi_open: mmap(addr=%p, len=0x%x, prot=%d, flags=%d, "
      "path=%d, offs=0x%x) failed\n", (char*)0, 0x20000, PROT_READ|PROT_WRITE,
      0, info->path, 0);
  return -1;
  }
#endif

/* sfi_sequencer_status(info, 1); */

return 0;
}

/*****************************************************************************/

int sfi_close(sfi_info* info)
{
int res=0;

#ifdef SFIMAPPED
if (munmap(info->base, 0x20000)<0) res=-1;
#endif
if (close(info->path)<0) res=-1;
info->path=-1;
return res;
}

/*****************************************************************************/

int sfi_backmap(sfi_info* info, u_int32_t* buf, int size)
{
struct vmebackmapping back;

if (((unsigned long)buf)&3) {errno=EINVAL; return -1;}
back.buf=(caddr_t)buf;
back.size=size*sizeof(u_int32_t);
printf("call ioctl(%d, %s, buf=%p, size=%d)\n", info->path, "MAPTOVME",
  back.buf, back.size);
/*
 * fflush(stdout);
 * sleep(2);
 */
if (ioctl(info->path, MAPTOVME, &back)<0)
  {
  info->backbase=0;
  return -1;
  }
else
  {
  printf("got vmebase=%d\n", back.dma_vmebase);
  info->backbase=(caddr_t)buf;
  info->dma_vmebase=(caddr_t)back.dma_vmebase;
  return 0;
  }
}

/*****************************************************************************/

int sfi_arblevel(sfi_info* info, int arblevel)
{
if (arblevel&~0xbf) {errno=EINVAL; return -1;}
SFI_W(info, fastbus_arbitration_level, arblevel);
return 0;
}

/*****************************************************************************/

int sfi_timeout(sfi_info* info, int shorttimeout, int longtimeout)
{
if (shorttimeout&~0xb) {errno=EINVAL; return -1;}
if (longtimeout&~0xf) {errno=EINVAL; return -1;}
SFI_W(info, fastbus_timeout, (longtimeout<<4)|shorttimeout);
return 0;
}

/*****************************************************************************/
/*
 * void sfi_listaddresses(sfi_info* info)
 * {
 * printf("base: %p\n", info->base);
 * printf("seq : %p\n", info->seq);
 * printf("\n");
 * printf("write_vme_out_signal: %p\n", &(SFI_W(info, write_vme_out_signal)));
 * printf("clear_both_lca1_test_register: %p\n", &(SFI_W(info, clear_both_lca1_test_register)));
 * printf("write_int_fb_ad: %p\n", &(SFI_W(info, write_int_fb_ad)));
 * }
 */
/*****************************************************************************/

void sfi_reset(sfi_info* info)
{
SFI_W(info, reset_register_group_lca2, mist);
SFI_W(info, sequencer_reset, mist);
SFI_W(info, sequencer_enable, mist);
}

/*****************************************************************************/

void sfi_restart_sequencer(sfi_info* info)
{
SFI_W(info, sequencer_reset, mist);
SFI_W(info, sequencer_enable, mist);
/* printf("restart sequencer\n"); */
}

/*****************************************************************************/

void sfi_stop_sequencer(sfi_info* info)
{
SFI_W(info, sequencer_disable, mist);
}

/*****************************************************************************/

void sfi_sequencer_status(sfi_info* info, int verbose)
{
int ss;
ss=SFI_R(info, sequencer_status);
printf("SFI status (*0x2020): 0x%08x\n", ss);
info->status.status=ss&0xffff;
info->status.error=sfi_error_ok;
info->status.fb1=SFI_R(info, fastbus_status_1);
printf("SFI fb1_status (*0x2024): 0x%08x\n", info->status.fb1);
info->status.fb2=SFI_R(info, fastbus_status_2);
printf("SFI fb2_status (*0x2028): 0x%08x\n", info->status.fb2);
if (info->status.status&0xc0)
  info->status.ss=(info->status.fb2>>4)&7;
else if (info->status.status&0x20)
  info->status.ss=(info->status.fb1>>4)&7;
info->status.pa=SFI_R(info, last_primadr);
printf("SFI pa (*0x1004): 0x%08x\n", info->status.pa);
info->status.cmd=SFI_R(info, last_sequencer_protocol);
printf("SFI cmd (*0x201c): 0x%08x\n", info->status.cmd);
info->status.seqaddr=SFI_R(info, next_sequencer_ram_address);
printf("SFI seqaddr (*0x2018): 0x%08x\n", info->status.seqaddr);
if (verbose) sfi_reporterror(&info->status);
}

/*****************************************************************************/

int sfi_wait_sequencer(sfi_info* info)
{
u_int32_t status;

/*while (((status=SFI_R(info, sequencer_status))&0x8000)==0);*/
status=SFI_R(info, sequencer_status);
if (status==0)
  {
  info->status.error=sfi_error_unknown;
  printf("sfi_wait_sequencer: sequencer never started\n");
  return -1;
  }
while ((status&0x8000)==0) status=SFI_R(info, sequencer_status);
if (status&0x00f0) printf("sequencer status=0x%08x\n", status);
info->status.status=status&0xffff;
if ((status&0x8001)!=0x8001)
  {
  info->status.error=sfi_error_sequencer;
  info->status.fb1=SFI_R(info, fastbus_status_1);
  info->status.fb2=SFI_R(info, fastbus_status_2);
  if (status&0xc0)
    info->status.ss=(info->status.fb2>>4)&7;
  else if (status&0x20)
    info->status.ss=(info->status.fb1>>4)&7;
  info->status.pa=SFI_R(info, last_primadr);
  info->status.cmd=SFI_R(info, last_sequencer_protocol);
  info->status.seqaddr=SFI_R(info, next_sequencer_ram_address);
  /* sfi_reporterror(&info->status); */
  sfi_restart_sequencer(info);
  return -1;
  }
info->status.error=sfi_error_ok;
return 0;
}

/*****************************************************************************/

int sfi_read_fifo(sfi_info* info, u_int32_t* dest, int max)
{
if (sfi_wait_sequencer(info)==0)
  {
  u_int32_t flags;
  int num=0;
  if ((flags=SFI_R(info, fifo_flags))&0x10)
    {
    volatile u_int32_t d=SFI_R(info, read_seq2vme_fifo);
    (void) d;
    flags=SFI_R(info, fifo_flags);
    }
  while (((flags&0x10)==0) && (num<max))
    {
    dest[num++]=SFI_R(info, read_seq2vme_fifo);
    flags=SFI_R(info, fifo_flags);
    }
  return num;
  }
else
  return -1;
}

/*****************************************************************************/

int sfi_read_fifoword(sfi_info* info)
{
u_int32_t flags;
int res;

if (sfi_wait_sequencer(info)<0) return 0;

if ((flags=SFI_R(info, fifo_flags))&0x10) /* FIFO empty */
  {
  u_int32_t d=SFI_R(info, read_seq2vme_fifo); /* dummy read */
  (void)d;
  flags=SFI_R(info, fifo_flags);
  if (flags&0x10) /* FIFO empty */
    {
    info->status.error=sfi_error_fifo;
    return 0;
    }
  }
res=SFI_R(info, read_seq2vme_fifo);
return res;
}

/*****************************************************************************/

int sscode_(sfi_info* info)
{
/*
 * printf("ss: error=0x%x, status=0x%x, ss=0x%x\n",
 *     info->status.error,
 *     info->status.status,
 *     info->status.ss);
 */
if (info->status.error==sfi_error_ok) return 0;
if (info->status.error!=sfi_error_sequencer) return 0x8;
if ((info->status.status&0x10) || (info->status.ss==0))
  return 0x8;
else
  return info->status.ss;
}

/*****************************************************************************/

void sfi_reporterror(struct sfi_status* stat)
{
printf("sequencer status:\n");
printf("  status : %04x\n", stat->status);
printf("  fb1    :  %03x\n", stat->fb1&0x7ff);
printf("  fb2    : %04x\n", stat->fb2&0xffff);
printf("  ss     :    %d\n", stat->ss);
printf("  pa     :   %2d\n", stat->pa);
printf("  cmd    : %04x\n", stat->cmd&0xfffc);
printf("  seqaddr: %04x\n", stat->seqaddr&0xffff);
switch (stat->status&0xf0)
  {
  case 0x80:
  case 0x40:
    {
    i32 fbs;
    if ((stat->status&0xf0)==0x80)
      printf("Blocktransfer Error\n");
    else
      printf("DataCycle Error\n");
    fbs=stat->fb2;
    if (fbs&0x8000) printf("DMA finished\n");
    if (fbs&0x4000) printf("DMA busy\n");
    if (fbs&0x2000) printf("VME Timeout\n");
    if (fbs&0x1000) printf("Fastbus Timeout\n");
    if (fbs&0x800)  printf("DMA stoped by Limit\n");
    if (fbs&0x700)  printf("last DMA SS=%d\n", (fbs>>4)&7);
    if (fbs&0x80)   printf("SS!=0\n");
    if (fbs&0x70)   printf("SS=%d\n", (fbs>>4)&7);
    if (fbs&0x8)    printf("DK Timeout\n");
    if (fbs&0x4)    printf("DS Timeout\n");
    if (fbs&0x2)    printf("DK is set\n");
    if (fbs&0x1)    printf("no ASAK Lock\n");
    }
    break;
  case 0x20:
    {
    i32 fbs;
    printf("Primary Address or Arbitration Error\n");
    fbs=stat->fb1;
    if (fbs&0x400) printf("Farside Timeout\n");
    if (fbs&0x200) printf("AK Timeout\n");
    if (fbs&0x80)  printf("SS!=0\n");
    if (fbs&0x70)  printf("SS=%d\n", (fbs>>4)&7);
    if (fbs&0x8)   printf("AS Timeout\n");
    if (fbs&0x4)   printf("other Master active\n");
    if (fbs&0x2)   printf("Arbitration Timeout\n");
    if (fbs&0x1)   printf("ASAK Lock is set\n");
    }
    break;
  case 0x10: printf("invalid Command\n"); break;
  }

switch (stat->cmd&0xc)
  {
  case 0: break;
  case 4:
    printf("FB command: ");
    if (stat->cmd&1000) printf("EG|");
    if (stat->cmd&800) printf("RD|"); else printf("WR|");
    if (stat->cmd&700) printf("MS=%d|", (stat->cmd>>8)&7);
    switch ((stat->cmd>>4)&0xf)
      {
      case  0: printf("pa cycle"); break;
      case  1: printf("pa cycle and hold mastership"); break;
      case  2: printf("disconnect"); break;
      case  3: printf("disconnect and release mastership"); break;
      case  4: printf("data cycle"); break;
      case  5: printf("data cycle and disconnect"); break;
      case  9: printf("load VME address"); break;
      case 10:
        printf("load limit counter, clear word counter, start blocktransfer");
        break;
      case 11: printf("load limit counter, start blocktransfer"); break;
      case 13: printf("store address pointer to FiFo"); break;
      case 14: printf("store status and word counter to FiFo"); break;
      case 15: printf("store word counter to FiFo"); break;
      default: printf("unknown command");
      }
    printf(" pa=%d", stat->pa);
    printf("\n");
    break;
  case 8:
    printf("control command: ");
    if (stat->cmd&0x7f00) printf("address %x; ", stat->cmd&0x7f00);
    switch ((stat->cmd>>4)&0xf)
      {
      case 0: printf("set sequencer out signal register"); break;
      case 1: printf("disable sequencer"); break;
      case 2: printf("start RAM sequencer list"); break;
      case 3: printf("stop RAM sequencer list"); break;
      case 6: printf("set VME IRQ"); break;
      default: printf("unknown command");
      }
    printf("\n");
    break;
  default:
    printf("wrong command\n");
  }
}

/*****************************************************************************/
/*****************************************************************************/
