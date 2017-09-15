/*
 * lowlevel/fastbus/sfi/sfi.c
 * 
 * created      25.04.1997
 * last changed 14.03.2000 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sfi.c,v 1.4 2011/04/06 20:30:23 wuestner Exp $";

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include "../fastbus.h"
#include "fastbus_sfi.h"
#include "sfilib.h"
#ifndef HAVE_CGETCAP
#include <getcap.h>
#endif
#include <rcs_ids.h>
#include "../../lowbuf/lowbuf.h"
#include "../../sync/pci_zel/sync_pci_zel.h"
#define INBIT 0   /* syncmod aux0 */
#define OUTBIT 1  /* sfi ecl_out1 */

sfi_info sfi;
static int arbitrationlevel;
#ifndef LOWLEVELBUFFER
int* fbbuf_=0;
#endif
int* fbbuf=0;
int fbbufsize;

RCS_REGISTER(cvsid, "lowlevel/fastbus/sfi")

/*****************************************************************************/
int fastbus_sfi_low_printuse(FILE* outfilepath)
{
fprintf(outfilepath,
"  [:fbpath=<fastbuspath>][:fbnohesitate]\n"
"  [:auxpath=<pci_sync>][:auxin#<bit>][:sfiout#<bit>]\n"
"    defaults: auxin#%d sfiout#%d\n", INBIT, OUTBIT);
return 1;
}
/*****************************************************************************/
errcode fastbus_sfi_low_init(char* arg)
{
char *fbpath;
char *auxpath;
long argval;
int status;

T(fastbus_sfi_low_init)
sfi.syncid=-1; sfi.syncinf=0;

if ((!arg) || (cgetstr(arg, "fbpath", &fbpath)<0))
  {
  printf("kein FASTBUS-Device gegeben\n");
  return(Err_ArgNum);
  }
D(D_USER, printf("FASTBUS ist %s\n", fbpath); )

if (strcmp(fbpath, "none")==0)
  {
  free(fbpath);
  printf("No FASTBUS.\n");
  sfi.pathname=0;
  sfi.path=-1;
#ifdef SFIMAPPED
  sfi.base=0;
  sfi.backbase=0;
#endif
  return OK;
  }

if (sfi_open(fbpath, &sfi)<0)
  {
  printf("sfi_open(%s): %s\n", fbpath, strerror(errno));
  free(fbpath);
  return Err_System;
  }
free(fbpath);

status=SFI_R(&sfi, sequencer_status)&0xffff;
printf("SFI status: 0x%08x\n", status);
if (status!=0xa001)
  {
  printf("init: status= %x\n", status);
  sfi_sequencer_status(&sfi, 1);
  sfi_reset(&sfi);
  printf("after reset SFI:\n");
  sfi_sequencer_status(&sfi, 1);
  }

#ifdef LOWLEVELBUFFER
fbbuf=lowbuf_buffer();
fbbufsize=lowbuf_buffersize();
#else
fbbufsize=0x800; /* 8 KByte in Worten */
fbbuf_=malloc(0x4000); /* malloc 16 KByte */
if (!fbbuf_)
  {
  printf("sfi_backmap: malloc(0x4000): %s\n", strerror(errno));
  return Err_System;
  }
fbbuf=(int*)((long)(fbbuf_+(0x1000-1))&~0xfff); /* align to 4 KByte entity */
printf("fbbuf=%p\n", fbbuf);
#endif
printf("call sfi_backmap(..., buffer=%p, size=%d)\n", fbbuf, fbbufsize);
if (sfi_backmap(&sfi, fbbuf, fbbufsize)==-1)
  {
  printf("sfi_backmap: %s\n", strerror(errno));
  sfi_close(&sfi);
  return Err_System;
  }

if (arg && (cgetcap(arg, "fbnohesitate", ':')==0))
  {
  struct sfiattrib att;
  bzero(&att, sizeof(struct sfiattrib));
  att.hesitate=1;
  att.flags=SFI_ATTRIB_HESITATE;
  if (ioctl(sfi.path, SFI_SET_ATTRIB, &att)<0)
    {
    printf("set hesitate: %s\n", strerror(errno));
    }
  }

setarbitrationlevel(0x81);
if ((arg) && (cgetstr(arg, "auxpath", &auxpath)>0))
  {
  plerrcode pres;
  struct pcisyncaux syncaux;
  /* printf("use %s for polling\n", auxpath);*/
  if ((pres=syncmod_attach(auxpath, &sfi.syncid))!=plOK)
    {
    printf("attach syncmodule in fastbus_sfi_low_init: %s\n", strerror(errno));
    free(auxpath);
    return Err_System;
    }
  free(auxpath);
  sfi.syncinf=syncmod_getinfo(sfi.syncid);
  if (cgetnum(arg, "auxin", &argval)==0)
    sfi.inbit=argval;
  else
    sfi.inbit=INBIT;
  if ((sfi.inbit<0) || (sfi.inbit>2))
    {
    printf("sfi_low_init: bad auxin (%d)\n", sfi.inbit);
    return Err_ArgRange;
    }
  if (cgetnum(arg, "sfiout", &argval)==0)
    sfi.outbit=argval;
  else
    sfi.outbit=OUTBIT;
  if ((sfi.outbit<1) || (sfi.outbit>7))
    {
    printf("sfi_low_init: bad sfiout (%d)\n", sfi.outbit);
    return Err_ArgRange;
    }
  /* outbit 1..4: ecl_out; 5..7: nim_out */
  if (sfi.outbit<5)
    {
    sfi.outbit=4-sfi.outbit;
    }
  else /* 5..7 */
    {
    sfi.outbit=sfi.outbit-1;
    }
  syncaux.idx=sfi.inbit;
  printf("sfi.outbit=0x%0x, syncaux.inbit=0x%0x\n", sfi.outbit, sfi.inbit);
  if (ioctl(sfi.syncinf->path, PCISYNC_GETAUX, &syncaux)<0)
    {
    printf("PCISYNC_GETAUX: %s\n", strerror(errno));
    return Err_System;
    }
  if (ioctl(sfi.path, SFI_SETAUX, &syncaux)<0)
    {
    printf("SFI_SETAUX: %s\n", strerror(errno));
    return Err_System;
    }
  }
else
  {
  printf("no auxpath given, blockread will not work.\n");
  }
return OK;
}
/*****************************************************************************/
errcode fastbus_sfi_low_done()
{
if (sfi.syncid>=0) syncmod_detach(sfi.syncid);
sfi.syncid=-1; sfi.syncinf=0;
if (sfi.path>=0)
  {
  if (sfi_close(&sfi)<0) printf("sfi_close: %s\n", strerror(errno));
  sfi.path=-1;
  }
#ifndef LOWLEVELBUFFER
if (fbbuf_)
  {
  free (fbbuf_);
  fbbuf_=0;
  }
#endif
fbbuf=0;
return OK;
}
/*****************************************************************************/
int setarbitrationlevel(int arblevel)
{
sfi_arblevel(&sfi, arblevel);
arbitrationlevel=arblevel;
return arblevel;
}
/*****************************************************************************/
int fb_modul_id(int pa)
{
int res;

res=FRC(pa, 0);
if (sscode()==0)
  return res>>16;
else
  return 0;
}
/*****************************************************************************/
/*****************************************************************************/
