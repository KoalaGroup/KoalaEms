/*
 * dataout/cluster/do_tape.c
 * created      25.04.97
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: do_tape.c,v 1.4 2011/04/06 20:30:22 wuestner Exp $";

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <unistd.h>
#include <rcs_ids.h>
#include "do_tape.h"

RCS_REGISTER(cvsid, "dataout/cluster")

/*****************************************************************************/

static struct mtop mt_com;

static int action(int p)
{
return ioctl(p, MTIOCTOP, &mt_com)?errno:0;
}

/*****************************************************************************/
#if 0
int low_tape_write_mark(int p)
{
mt_com.mt_op=MTWEOF;
mt_com.mt_count=1;
return action(p);
}

int low_tape_fsf(int p, int num)
{
mt_com.mt_op=MTFSF;
mt_com.mt_count=num;
return action(p);
}

int low_tape_bsf(int p, int num)
{
mt_com.mt_op=MTBSF;
mt_com.mt_count=num;
return action(p);
}

int low_tape_fsr(int p, int num)
{
mt_com.mt_op=MTFSR;
mt_com.mt_count=num;
return action(p);
}

int low_tape_bsr(int p, int num)
{
mt_com.mt_op=MTBSR;
mt_com.mt_count=num;
return action(p);
}

int low_tape_rewind(int p)
{
mt_com.mt_op=MTREW;
mt_com.mt_count=0;
return action(p);
}

int low_tape_seod(int p)
{
#ifdef __osf__
mt_com.mt_op=MTSEOD;
#else
mt_com.mt_op=MTEOM; /* really end of media? */
#endif
mt_com.mt_count=0;
return action(p);
}

int low_tape_offline(int p)
{
mt_com.mt_op=MTOFFL;
mt_com.mt_count=0;
return action(p);
}

#ifdef __osf__
int low_tape_online(int p)
{
mt_com.mt_op=MTONLINE;
mt_com.mt_count=0;
return action(p);
}
#endif

#ifdef __osf__
int low_tape_load(int p)
{
mt_com.mt_op=MTLOAD;
mt_com.mt_count=0;
return action(p);
}
#endif

#ifdef __osf__
int low_tape_unload(int p)
{
mt_com.mt_op=MTUNLOAD;
mt_com.mt_count=0;
return action(p);
}
#endif

int low_tape_retension(int p)
{
mt_com.mt_op=MTRETEN;
mt_com.mt_count=0;
return action(p);
}

#ifdef __osf__
int low_tape_clear_error(int p)
{
mt_com.mt_op=MTCSE;
mt_com.mt_count=0;
return action(p);
}
#endif
#endif /* 0 */

int low_tape_nop(int p)
{
mt_com.mt_op=MTNOP;
mt_com.mt_count=0;
return action(p);
}

#ifdef __osf__
static const char* ss(char* s)
{
static char sss[9];
int i;
for (i=0; i<8; i++) sss[i]=s[i];
sss[8]=0;
return sss;
}
#endif

int do_tape_get(int p)
{
int res;
struct mtget mt_get;
#ifdef __osf__
struct devget dev_get;
#endif

res=ioctl(p, MTIOCGET, &mt_get);
if (res<0)
  {
  printf("MTIOCGET: %s\n", strerror(errno));
  }
else
  {
  printf("device type           =%d\n", mt_get.mt_type);
  printf("drive status register =%d\n", mt_get.mt_dsreg);
  printf("error register        =%d\n", mt_get.mt_erreg);
  printf("residual count        =%d\n", mt_get.mt_resid);
  }
#ifdef __osf__
res=ioctl(p, DEVIOCGET, &dev_get);
if (res<0)
  {
  printf("DEVIOCGET: %s\n", strerror(errno));
  }
else
  {
  printf("category     =%d\n", dev_get.category);
  printf("bus          =%d\n", dev_get.bus);
  printf("interface    =%s\n", ss(dev_get.interface));
  printf("device       =%s\n", ss(dev_get.device));
  printf("adpt_num     =%d\n", dev_get.adpt_num);
  printf("nexus_num    =%d\n", dev_get.nexus_num);
  printf("bus_num      =%d\n", dev_get.bus_num);
  printf("ctlr_num     =%d\n", dev_get.ctlr_num);
  printf("rctlr_num    =%d\n", dev_get.rctlr_num);
  printf("slave_num    =%d\n", dev_get.slave_num);
  printf("dev_name     =%s\n", ss(dev_get.dev_name));
  printf("unit_num     =%d\n", dev_get.unit_num);
  printf("soft_count   =%d\n", dev_get.soft_count);
  printf("hard_count   =%d\n", dev_get.hard_count);
  printf("stat         =0x%lX\n", dev_get.stat);
  printf("category_stat=0x%lX\n", dev_get.category_stat);
  }
#endif
return 0;
}

/*****************************************************************************/
/*****************************************************************************/
