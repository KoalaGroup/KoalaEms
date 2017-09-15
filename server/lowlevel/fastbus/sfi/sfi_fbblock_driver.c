/*
 * lowlevel/fastbus/sfi/sfi_fbblock_driver.c
 * 
 * created: 27.04.97
 * last changed: 02.06.97
 * 21.01.1999 PW: FWDB_ and FWCB_ removed (now in sfi_fbblockwrite.c)
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sfi_fbblock_driver.c,v 1.4 2011/04/06 20:30:23 wuestner Exp $";

#include <errno.h>
#include <stdio.h>
#include <sconf.h>
#include <rcs_ids.h>
#include "sfilib.h"
#include "../fastbus.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sfi")

/*****************************************************************************/

static const u_int32_t mist=0x0815;

/*****************************************************************************/
int FRDB_driver(sfi_info* info, u_int32_t pa, u_int32_t sa, u_int32_t* dest,
    u_int32_t count)
{
int res;
struct sfi_status err;
struct sfiblockread r;

r.pa=pa;
r.sa=sa;
r.dest=(caddr_t)dest-info->backbase;
printf("FRDB_: r.dest=%d\n", r.dest);
r.len=count;
r.err=&err;
err.error=17;
err.ss=17;
err.num=17;
err.pa=17;
err.cmd=17;
err.status=17;
err.fb1=17;
err.fb2=17;
err.seqaddr=17;
err.dmastatus=17;
/*
printf("FRDB_: dest=%p, count=%d\n", r.dest, r.len);
for (i=0; i<10; i++) dest[i]=10-i;
*/
res=ioctl(info->path, SFI_BLREAD, &r);
if (res<0)
  {
  perror("ioctl(SFI_BLREAD)");
  printf("dest=%p, count=%d\n", dest, count);
  printf("error    =%d\n", err.error);
  printf("ss       =%d\n", err.ss);
  printf("num      =%d\n", err.num);
  printf("pa       =%d\n", err.pa);
  printf("cmd      =%04x\n", err.cmd);
  printf("status   =%04x\n", err.status);
  printf("fb1      =%04x\n", err.fb1);
  printf("fb2      =%04x\n", err.fb2);
  printf("seqaddr  =%04x\n", err.seqaddr);
  printf("dmastatus=%08x\n", err.dmastatus);
  sfi_reporterror(&err);
  }
/*
for (i=0; i<30; i++) printf("%d: %X\n", i, dest[i]);
*/
printf("err.error=%d\n", err.error);
printf("err.ss=%d\n", err.ss);
printf("err.num=%d\n", err.num);
printf("err.pa=%d\n", err.pa);
printf("err.cmd=%d\n", err.cmd);
printf("err.status=%d\n", err.status);
printf("err.fb1=%d\n", err.fb1);
printf("err.fb2=%d\n", err.fb2);
printf("err.seqaddr=%d\n", err.seqaddr);
printf("err.dmastatus=%d\n", err.dmastatus);
return err.num;
}
/*****************************************************************************/
/* wie FRDB, ohne secondary address */
int FRDB_S_driver(sfi_info* info, u_int32_t pa, u_int32_t* dest, u_int32_t count)
{
int res;
struct sfi_status err;
struct sfiblockread r;
r.pa=pa;
r.sa=(u_int32_t)-1;
r.dest=(u_int32_t)dest; /* must be inside backmapped aera */
r.len=count;
r.err=&err;

res=ioctl(info->path, SFI_BLREAD, &r);
if (res<0)
  {
  perror("ioctl(SFI_BLREAD)");
  printf("error    =%d\n", err.error);
  printf("ss       =%d\n", err.ss);
  printf("num      =%d\n", err.num);
  printf("pa       =%d\n", err.pa);
  printf("cmd      =%04x\n", err.cmd);
  printf("status   =%04x\n", err.status);
  printf("fb1      =%04x\n", err.fb1);
  printf("fb2      =%04x\n", err.fb2);
  printf("seqaddr  =%04x\n", err.seqaddr);
  printf("dmastatus=%08x\n", err.dmastatus);
  }
return err.num;
}
/*****************************************************************************/
int FRCB_driver(sfi_info* info, u_int32_t pa, u_int32_t sa, u_int32_t* dest,
    u_int32_t count)
{
printf("lowlevel/fastbus/sfi/sfi_fbblock.c::FRCB_ not yet implemented\n");
return 0;
#if 0
int res;
struct sfi_status err;
struct sfiblockread r;
r.pa=pa;
r.sa=sa;
r.dest=dest; /* must be inside backmapped aera */
r.len=count;
r.err=&err;

res=ioctl(info->path, SFI_BLREAD, &r);
if (res<0)
  {
  perror("ioctl(SFI_BLREAD)");
  printf("error    =%d\n", err.error);
  printf("ss       =%d\n", err.ss);
  printf("num      =%d\n", err.num);
  printf("pa       =%d\n", err.pa);
  printf("cmd      =%04x\n", err.cmd);
  printf("status   =%04x\n", err.status);
  printf("fb1      =%04x\n", err.fb1);
  printf("fb2      =%04x\n", err.fb2);
  printf("seqaddr  =%04x\n", err.seqaddr);
  printf("dmastatus=%08x\n", err.dmastatus);
  }
return err.num;
#endif
}
/*****************************************************************************/
/*****************************************************************************/
