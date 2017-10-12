/*
 * lowlevel/hlral/hlral.c
 * static char *rcsid="$ZEL: hlral.c,v 1.4 2002/11/06 15:36:44 wuestner Exp $";
 */

#include <cdefs.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

#include <dev/pci/pcihotlinkvar.h>
#include "hlralreg.h"

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <swap.h>
#ifndef HAVE_CGETCAP
#include <getcap.h>
#endif

#include <dev/rawmem.h>
#include <sys/mman.h>
#include "../sync/pci_zel/sync_pci_zel.h"
#include <dev/pci/zelsync.h>

#include "hlral.h"

#if (HLRAL_VERSION==HLRAL_ATRAP)
#define MAX_RAL_RPC 16
#else
# if (HLRAL_VERSION==HLRAL_ANKE)
# define MAX_RAL_RPC 4
# else
# error HLRAL_VERSION must be defined
# endif
#endif

#define MAXHLRAL 10
static int nhlral;
static int hlhandle[MAXHLRAL];

static int rawmem;
static u_long rawmempbase[MAXHLRAL];
static char *rawmemvbase[MAXHLRAL];
static u_long rawmemlen;

/*****************************************************************************/

int
hlral_low_printuse(outfilepath)
	FILE *outfilepath;
{
	fprintf(outfilepath,
    "  [:pcihlp=<pcihlpath>[:rmemp=<rawmempath>]]\n");
	return (1);
}
/*****************************************************************************/
errcode
hlral_low_init(arg)
	char *arg;
{
	char *hlpath, *help;
	int i, res, zero = 0;
	char *rawmempath;
	u_long dmabufpaddr, dmabuflen;
	char *dmabuf;

	T(hlral_low_init)

	nhlral = 0;
	for (i = 0; i < MAXHLRAL; i++)
		hlhandle[i] = -1;

	if ((!arg) || (cgetstr(arg, "pcihlp", &hlpath) < 0)) {
		printf("kein PCI-Hotlink-Device gegeben\n");
		return (Err_ArgNum);
	}
	D(D_USER, printf("PCI-Hotlinks are %s\n", hlpath); )
	if (!strcmp(hlpath, "none")) {
		free(hlpath);
		printf("No PCI-Hotlink.\n");
		return (OK);
	}

	help = hlpath;
	res = 0;
	while (help) {
		char *comma;

		comma = strchr(help, ',');
		if (comma)
			*comma = '\0';

		hlhandle[nhlral] = open(help, O_RDWR, 0);
		if (hlhandle[nhlral] < 0) {
			printf("open PCI-Hotlink at %s: %s\n",
			       help, strerror(errno));
			res = Err_System;
			break;
		}

		if (fcntl(hlhandle[nhlral], F_SETFD, FD_CLOEXEC)<0)
		{
			printf("fcntl(HLRAL, FD_CLOEXEC): %s\n",
			       strerror(errno));
			res = Err_System;
			break;
		}

		res = ioctl(hlhandle[nhlral], HLLOOPBACK, &zero);
		if (res < 0) {
			printf("HLLOOPBACK failed\n");
			res = Err_System;
			break;
		}
		nhlral++;
		if (comma)
			help = comma + 1;
		else
			help = 0;
	}
	free(hlpath);
	if (res)
		goto errout;

	rawmem = -1;
	dmabufpaddr = 0;
	dmabuf = 0;

	if((!arg) || (cgetstr(arg, "rmemp", &rawmempath)) < 0) {
		printf("kein rawmem-Device gegeben\n");
		goto norawmem;
	}
	rawmem = open(rawmempath, O_RDWR, 0);
	if (rawmem < 0) {
		printf("open %s\n", rawmempath);
		return(Err_System);
	}
	if (fcntl(rawmem, F_SETFD, FD_CLOEXEC) <0 ) {
		printf("fcntl(rawmem, FD_CLOEXEC): %s\n", strerror(errno));
	}
	res = ioctl(rawmem, RAWMEM_GETPBASE, &dmabufpaddr);
	if (res < 0) {
		printf("ioctl getbase\n");
		return(Err_System);
	}
	res = ioctl(rawmem, RAWMEM_GETSIZE, &dmabuflen);
	if (res < 0) {
		printf("ioctl getsize\n");
		return(Err_System);
	}

	dmabuf = (char*)mmap(0, dmabuflen, PROT_READ|PROT_WRITE,
			     MAP_FILE | MAP_SHARED, rawmem, 0);
	if (dmabuf == (char*)-1) {
		printf("mmap %s\n", rawmempath);
		return(Err_System);
	}
	rawmemlen = (dmabuflen / nhlral) & (-4);

norawmem:
	sleep(3);

	for (i = 0; i < nhlral; i++) {

		if (rawmem >= 0) {
			struct hlmem rmem;

			rawmempbase[i] = dmabufpaddr + i * rawmemlen;
			rawmemvbase[i] = dmabuf + i * rawmemlen;
			rmem.type = HLMEMTYPE_PHYSICAL;
			rmem.base = rawmempbase[i];
			rmem.len = rawmemlen;

			res = ioctl(hlhandle[i], HLSETMEM, &rmem);
			if (res < 0) {
				printf("setmem: %d\n", errno);
				goto errout;
			}
		}

		res = hlral_reset(i);
		if (res < 0) {
			printf("hlral_reset failed\n");
			goto errout;
		}
	}

	return(OK);

errout:
	for (i = 0; i < MAXHLRAL; i++) {
		if (hlhandle[i] >= 0) {
			close(hlhandle[i]);
			hlhandle[i] = -1;
		}
	}
	nhlral = 0;
	return (res);
}
/*****************************************************************************/
int number_of_ralhotlinks()
{
return nhlral;
}
/*****************************************************************************/
errcode
hlral_low_done()
{
	int i;

	for (i = 0; i < MAXHLRAL; i++) {
		if (hlhandle[i] >= 0) {
			close(hlhandle[i]);
			hlhandle[i] = -1;
		}
	}
	nhlral = 0;

	if (rawmem != -1) {
		close(rawmem);
		rawmem = -1;
	}

	return(OK);
}
/*****************************************************************************/
static int
hlsend(int board, ems_u8 data)
{
        int res;

        res = write(hlhandle[board], &data, 1);
        if (res != 1)
                return (-1);
	return (0);
}
/*****************************************************************************/
/*
 * static int
 * hlsend2(int board, ems_u16 data)
 * {
 *         int res;
 * 
 *         res = write(hlhandle[board], &data, 2);
 *         if (res != 1)
 *                 return (-1);
 * 	return (0);
 * }
 */
/*****************************************************************************/
static int
hlsend4(int board, ems_u32 data)
{
        int res;

        res = write(hlhandle[board], &data, 4);
        if (res != 1)
                return (-1);
      return (0);
}
/*****************************************************************************/
static void
selectcol(int board, int col)
{
	int i;

	hlsend(board, STP_FCLK);
	for (i = 0; i < col; i++) {
		hlsend(board, STP_FCLK | CTL_LD_DAC);
		hlsend(board, STP_FCLK);
	}
}
/*****************************************************************************/
int
hlral_reset(board)
	int board;
{
	int res;

	hlsend(board, CTL_RESET);
	hlsend(board, 0);

	res = ioctl(hlhandle[board], HLCLEARRCV);
	if (res)
		return (-1);
	return (0);
}
/*****************************************************************************/
int
hlral_countchips(int board, int col, int testregs)
{
	ems_u8 buf[HLRAL_MAX_ROWS * MAX_RAL_RPC + 4];
	int res, i, chips;
	int RAL_RPC;
        int pat_5, pat_a;
        if (testregs)
          {
          RAL_RPC=4;
          pat_5=0x05;
          pat_a=0x0a;
          }
        else
          {
#if (HLRAL_VERSION==HLRAL_ATRAP)
          RAL_RPC=16;
          pat_5=0x0d;
          pat_a=0x0e;
#else
# if (HLRAL_VERSION==HLRAL_ANKE)
          RAL_RPC=1;
          pat_5=0x05;
          pat_a=0x0a;
# endif
#endif
          }

	hlral_reset(board);
	selectcol(board, col);

	if (testregs)
	  hlsend(board, STP_FCLK | CTL_LD_TST);
	else
	  hlsend(board, STP_FCLK | CTL_LD_DAC);

	for (i = 0; i < HLRAL_MAX_ROWS * RAL_RPC; i++) {
		hlsend(board, SET_DREG | 0x5); /* DAC value */
		hlsend(board, CLOCK);
	}
	res = read(hlhandle[board], buf, sizeof(buf) & (-4));
	if (res != HLRAL_MAX_ROWS * RAL_RPC) {
		printf("hlral_countchips: sent %d, got %d\n",
		       HLRAL_MAX_ROWS * RAL_RPC, res);
		return (-1);
	}
	for (i = 0; i < HLRAL_MAX_ROWS * RAL_RPC; i++) {
		hlsend(board, SET_DREG | 0xa); /* DAC value */
		hlsend(board, CLOCK);
	}
	hlsend(board, STP_FCLK);
	res = read(hlhandle[board], buf, sizeof(buf) & (-4));
	if (res != HLRAL_MAX_ROWS * RAL_RPC) {
		printf("hlral_countchips: sent %d, got %d\n",
		       HLRAL_MAX_ROWS * RAL_RPC, res);
		return (-1);
	}
	for (i = 0; i < HLRAL_MAX_ROWS * RAL_RPC; i++) {
		if ((buf[i] & 0x0f) != pat_5)
			break;
	}
	if (i == 0) {
		printf("hlral_countchips: no chips in col %d\n", col);
		return (0);
	}
	if (i % RAL_RPC) {
		printf("hlral_countchips: %d registers???\n", i);
		return (-1);
	}
	chips = i / RAL_RPC;
	for (; i < HLRAL_MAX_ROWS * RAL_RPC; i++) {
		if ((buf[i] & 0x0f) != pat_a) {
			printf("hlral_countchips: got %x at %d\n",
			       buf[i], i);
			return (-1);
		}
	}
	printf("chips: %d\n", chips);
	return (chips);
}
/*****************************************************************************/
static plerrcode
hlral_sendline(int board, int col, int num, ems_u8* buf_a, int which)
{
ems_u8 buf_b[HLRAL_MAX_ROWS*16+4];
int i, j, res;

D(D_DUMP, {
  printf("hlral_sendline: col=%d num=%d which=%d:\n", col, num, which);
  if (which)
    for (i=0; i<num; i++) printf("%X", ~buf_a[i]&0x3);
  else
    for (i=0; i<num; i++) printf("%X", buf_a[i]&0xf);
  printf("\n");})
  for (j=0; j<2; j++) {
    for (i=0; i<num; i++) {
      
      hlsend(board, SET_DREG|buf_a[i]);
      if (which)
        hlsend4(board, CLOCK*0x01010101);
      else
        {
        hlsend(board, CLOCK);
        }
    }
    res=read(hlhandle[board], buf_b, num);
    if (res!=num) {
      printf("hlral_loadregs_2: sent %d, got %d\n", num, res);
      return plErr_HW;
    }
  }
  if (which) {
    for (i=0; i<num; i++) {
      if ((buf_b[i]&0x3)!=buf_a[i]) {
        printf("hlral_sendline: column %d; idx %d: expected %d, got %d\n",
            col, i, buf_a[i], buf_b[i]&0x3);
        printf("buf_a:\n");
        for (i=0; i<num; i++) printf("%X", buf_a[i]&0x3);
        printf("\n");
        printf("buf_b:\n");
        for (i=0; i<num; i++) printf("%X", buf_b[i]&0x3);
        printf("\n");
        return plErr_HW;
      }
    }
  } else {
    for (i=0; i<num; i++) {
      if ((buf_b[i]&0xf)!=buf_a[i]) {
        printf("hlral_sendline: column %d; idx %d: expected %d, got %d\n",
            col, i, buf_a[i], buf_b[i]&0xf);
        printf("buf_a:\n");
        for (i=0; i<num; i++) printf("%X", buf_a[i]&0xf);
        printf("\n");
        printf("buf_b:\n");
        for (i=0; i<num; i++) printf("%X", buf_b[i]&0xf);
        printf("\n");
        return plErr_HW;
      }
    }
  }
return plOK;
}
/*****************************************************************************/
plerrcode
hlral_buffermode(int board, int mode)
{
plerrcode pres;
ems_u8 buf_a[HLRAL_MAX_ROWS*16+4];
int i, j, col, chips, command;

#if (HLRAL_VERSION!=HLRAL_ATRAP)
printf("hlral_buffermode only works for ATRAP\n");
return plErr_Other;
#endif

command=STP_FCLK|CTL_LD_DAC;
col=0;
chips=hlral_countchips(board, col, 1);
if (chips<0) return plErr_HW;
while (chips) {
  hlral_reset(board);
  selectcol(board, col);
  hlsend(board, command);

  for (i=0; i<chips*16; i++) buf_a[i]=3;
  for (i=0; i<chips; i++) buf_a[i*16+8]=0;
  if (mode) {
    for (i=0; i<chips; i++) {
      for (j=10; j<16; j++) buf_a[i*16+j]=0;
    }
  } else {
    /* nothing */
  }
  if ((pres=hlral_sendline(board, col, chips*16, buf_a, 1))!=plOK)
      return pres;
  col++;
  chips=hlral_countchips(board, col, 1);
}
hlral_reset(board);
return plOK;
}
/*****************************************************************************/
plerrcode
hlral_loaddac_2(int board, int col, int row, int channel, int* data, int num)
{
ems_u8 buf_a[HLRAL_MAX_ROWS*16+4];
int i, j, k;
int chips;
plerrcode pres;

#if (HLRAL_VERSION!=HLRAL_ATRAP)
printf("hlral_loadregs_2 only works for ATRAP\n");
return plErr_Other;
#endif

chips=hlral_countchips(board, col, 1); /* only testregs possible */
if (chips<0) return plErr_HW;
hlral_reset(board);
if (!chips)
  {
  printf("column %d contains no chips(?).\n", col);
  return plOK;
  }
selectcol(board, col);
if (row>=chips)
  {
  printf("hlral_loadregs_2: row %d requested but only %d rows available\n",
      row, chips);
  return (plErr_ArgRange);
  }

hlsend(board, STP_FCLK|CTL_LD_DAC);

if (num==1)
  {
  for (i=0; i<chips*16; i++) buf_a[i]=3;
  if (channel>=0) /* only one channel */
    {
    if (channel<8)
      {
      buf_a[row*16+7-channel]&=~1;
      for (i=0; i<8; i++) buf_a[row*16+15-i]&=*data>>i|~1;
      }
    else
      {
      buf_a[row*16+7-(channel-8)]&=~2;
      for (i=0; i<8; i++) buf_a[row*16+15-i]&=*data>>i<<1|~2;
      }
    }
  else if (row>=0) /* all 16 channels of one row (board) */
    {
    for (i=0; i<8; i++) buf_a[row*16+i]=0;
    for (i=0; i<8; i++) buf_a[row*16+15-i]=(*data>>i&1)*3;
    }
  else /* all channels of all boards */
    {
    for (j=0; j<chips; j++)
      {
      for (i=0; i<8; i++) buf_a[j*16+i]=0;
      for (i=0; i<8; i++) buf_a[j*16+15-i]=(*data>>i&1)*3;
      }
    }
  if ((pres=hlral_sendline(board, col, chips*16, buf_a, 1))!=plOK) return pres;
  }
else
  {
  if (channel>=0) /*chan>=0*/
    {
    printf("hlral_loadregs_2: only one channel but %d data\n", num);
    return (plErr_ArgNum);
    }
  else if (row>=0) /*chan<0; row>=0; all 16 channels of one row (board) */
    {
    if (num!=16)
      {
      printf("hlral_loadregs_2: row=%d and channel=-1 but num=%d (not 16)\n",
          row, num);
      return (plErr_ArgNum);
      }
    for (j=0; j<8; j++)
      {
      for (i=0; i<chips*16; i++) buf_a[i]=3;
      buf_a[row*16+8+j]=0;
      for (i=0; i<8; i++)
        {
        buf_a[row*16+i]&=(data[j]>>i|~1)&(data[j+8]>>i<<1|~2);
        }
      if ((pres=hlral_sendline(board, col, chips*16, buf_a, 1))!=plOK) return pres;
      }
    }
  else /*chan<0; row<0; all channels of all boards */
    {
    if (num!=chips*16)
      {
      printf("hlral_loadregs_2: column=%d with %d chips but num=%d\n",
          col, chips, num);
      return (plErr_ArgNum);
      }
    for (j=0; j<8; j++)
      {
      for (i=0; i<chips*16; i++) buf_a[i]=3;
      for (k=0; k<chips; k++)
        {
        buf_a[k*16+8+j]=0;
        for (i=0; i<8; i++)
          {
          buf_a[k*16+i]&=(data[k*16+j]>>i|~1)&(data[k*16+j+8]>>i<<1|~2);
          }
        }
      if ((pres=hlral_sendline(board, col, chips*16, buf_a, 1))!=plOK) return pres;
      }
    }
  }
hlral_reset(board);
return plOK;
}
/*****************************************************************************/
static int datapathtest(int board, int col, int testregs, int chips, int val)
{
ems_u8 *buf;
int RAL_RPC, res=0, r, mask, i, fail;

if (testregs)
  {
  RAL_RPC=4;
  mask=0xf;
  }
else
  {
#if (HLRAL_VERSION==HLRAL_ATRAP)
  RAL_RPC=16;
  mask=0x3;
#else
# if (HLRAL_VERSION==HLRAL_ANKE)
  RAL_RPC=1;
  mask=0xf;
# endif
#endif
}
buf=(ems_u8*)malloc(chips*RAL_RPC+4);
if (!buf)
  {
  printf("datapathtest: cannot allocate %d bytes.\n", chips*RAL_RPC+4);
  return -1;
  }
for (i=0; i<chips*RAL_RPC; i++)
  {
  hlsend(board, SET_DREG | (val & mask));
  hlsend(board, CLOCK);
  }
r=read(hlhandle[board], buf, chips*RAL_RPC);
if (r!=chips*RAL_RPC)
  {
  printf("datapathtest: sent %d, got %d\n", chips*RAL_RPC, r);
  res=-1; goto raus;
  }
for (i=0; i<chips*RAL_RPC; i++)
  {
  hlsend(board, SET_DREG | (~val&mask));
  hlsend(board, CLOCK);
  }
r=read(hlhandle[board], buf, chips*RAL_RPC);
if (r!=chips*RAL_RPC)
  {
  printf("datapathtest: sent %d, got %d\n", chips*RAL_RPC, r);
  res=-1; goto raus;
  }
fail=0;
for (i=0; i<chips*RAL_RPC; i++)
  {
  if ((buf[i]^val)&mask) fail=1;
  }
if (fail)
  {
  printf("datapathtest a %s(0x%x):\n", testregs?"testregs":"dacregs", val);
  for (i=0; i<chips*RAL_RPC; i++) printf("%x ", buf[i]);
  printf("\n");
  res=-1; goto raus;
  }

for (i=0; i<chips*RAL_RPC; i++)
  {
  hlsend(board, SET_DREG|(i%2?val:~val)&mask);
  hlsend(board, CLOCK);
  }
r=read(hlhandle[board], buf, chips*RAL_RPC);
if (r!=chips*RAL_RPC)
  {
  printf("datapathtest: sent %d, got %d\n", chips*RAL_RPC, r);
  res=-1; goto raus;
  }
for (i=0; i<chips*RAL_RPC; i++)
  {
  hlsend(board, SET_DREG|~val&mask);
  hlsend(board, CLOCK);
  }
r=read(hlhandle[board], buf, chips*RAL_RPC);
if (r!=chips*RAL_RPC)
  {
  printf("datapathtest: sent %d, got %d\n", chips*RAL_RPC, r);
  res=-1; goto raus;
  }
fail=0;
for (i=0; i<chips*RAL_RPC; i++)
  {
  if ((buf[i]^(i%2?val:~val))&mask) fail=1;
  }
if (fail)
  {
  printf("datapathtest b %s(0x%x):\n", testregs?"testregs":"dacregs", val);
  for (i=0; i<chips*RAL_RPC; i++) printf("%x ", buf[i]);
  printf("\n");
  res=-1; goto raus;
  }

raus:
free(buf);
return res;
}
/*****************************************************************************/
int hlral_datapathtest(int board, int col, int testregs)
{
int chips, i;
chips=hlral_countchips(board, col, testregs);
if (chips<1)
  {
  int i;
  for (i=0; i<100; i++) printf("hlral_datapathtest: nix chips!\n");
  return -1;
  }
hlral_reset(board);
selectcol(board, col);

if (testregs)
  hlsend(board, STP_FCLK | CTL_LD_TST);
else
  hlsend(board, STP_FCLK | CTL_LD_DAC);

for (i=0; i<0xf; i++)
  {
  if (datapathtest(board, col, testregs, chips, i)<0) return -1;
  }

return 0;
}
/*****************************************************************************/
int
hlral_loadregs(board, col, which, data, len)
	int board, col, which;
	ems_u8 *data;
	int len;
{
	ems_u8 buf[HLRAL_MAX_ROWS * 4 + 4];
	int res, i;

	hlral_reset(board);
	selectcol(board, col);

	if (which)
		hlsend(board, STP_FCLK | CTL_LD_DAC);
	else
		hlsend(board, STP_FCLK | CTL_LD_TST);

	for (i = 0; i < len; i++) {
		hlsend(board, SET_DREG | *data++);
		hlsend(board, CLOCK);
	}

	hlsend(board, STP_FCLK);

	res = read(hlhandle[board], buf, sizeof(buf) & (-4));
	if (res != len) {
		printf("hlral_loadregs: sent %d, got %d\n",
		       len, res);
		return (-1);
	}

	return (0);
}
/*****************************************************************************/
#if 0
plerrcode
hlral_loadtregs(int col, int* data, int len)
{
ems_u8 buf[HLRAL_MAX_ROWS * 4 + 4];
int chips, i;
plerrcode pres;

chips=hlral_countchips(col, 1 /*testregs*/);

hlral_reset();
selectcol(col);

hlsend(STP_FCLK | CTL_LD_TST);
for (i=0; i<chips; i++)
  {
  for (j=0; j<4; j++)
    buf[4*i+j]=0;
    for (k=0; k<4; k++)
      {
      if (data[i]>>(4*j+k)&1) buf[4*i+j]|=1<<k;
      }
  }
return ((hlral_sendline(col, chips*4, buf, 0 /*!daqreqs*/))!=plOK)
}
#endif
/*****************************************************************************/

ems_u32 *
hlral_testreadout(board, data)
	int board;
	ems_u32 *data;
{
	int res;
	int *p;
	int got;

	p = data + 1;
	got = 0;

	hlral_reset(board);

	if (rawmem >= 0) {
		res = ioctl(hlhandle[board], HLSTARTDMA, 0);
		if (res < 0)
			printf("hlral_testreadout: startdma: %s\n", strerror(errno));
	}

	hlsend(board, 0 | SET_DREG); /* no external strobe */
	hlsend(board, CTL_LD_TST);
	hlsend(board, STROBE);
	hlsend(board, 0);
	hlsend(board, ROUT_DIRECT);

	if (rawmem >= 0) {
		struct hlstopdma sd;

		sd.wait = 1;
		res = ioctl(hlhandle[board], HLSTOPDMA, &sd);
		if (res < 0)
			printf("hlral_readout: stopdma: %s\n", strerror(errno));

		if (sd.len % 4) {
			printf("hlral_readout: DMA %ld\n", sd.len);
			return (0);
		}

		bcopy(rawmemvbase[board], p, sd.len);
		p += sd.len / 4;
		got = sd.len;
		if (sd.alldata)
			goto gotall;
	}

	res = read(hlhandle[board], p, 1000000); /* XXX */
	if (res < 0)
		return (0);
	got += res;
gotall:
	if (got <= 0) /* must have something */
		return (0);
	*data = got; /* counter as header */

	return (data + (got + 3) / 4 + 1);
}
/*****************************************************************************/
static syncmod_info* info=0;

static int ralmode[MAXHLRAL];

ems_u32 *
hlral_readout(board, data)
	int board;
	ems_u32 *data;
{
	int res;
	int *p;
	int got;
	p = data + 1;
	got = 0;
        info->base[SYNC_SSCR]=4<<4;
	if (rawmem >= 0) {
		struct hlstopdma sd;
		sd.wait = 1;
        info->base[SYNC_SSCR]=1<<4;
		res = ioctl(hlhandle[board], HLSTOPDMA, &sd);
        info->base[SYNC_SSCR]=1<<20;
		if (res < 0)
			printf("hlral_readout: stopdma: %s\n", strerror(errno));

		if (sd.len % 4) {
			printf("hlral_readout: DMA %ld\n", sd.len);
			return (0);
		}

		bcopy(rawmemvbase[board], p, sd.len);
		p += sd.len / 4;
		got = sd.len;
		if (sd.alldata)
			goto gotall;
	}

	res = read(hlhandle[board], p, 1000000); /* XXX */
	if (res < 0) {
		printf("hlral_readout: read %d: %s\n", res, strerror(errno));
		return (0);
	}
	got += res;
gotall:
	if (got <= 0) { /* must have something */
		printf("hlral_readout: got %d\n", got);
		return (0);
	}
	*data = got; /* counter as header */

	if (rawmem >= 0) {
		res = ioctl(hlhandle[board], HLSTARTDMA, 0);
		if (res < 0)
			printf("hlral_readout: startdma: %s\n", strerror(errno));
	}

	if (ralmode[board])
		hlsend(board, ROUT_ACT_TEST);
	else
		hlsend(board, ROUT_ACT_NORM);
        info->base[SYNC_SSCR]=4<<20;
	return (data + (got + 3) / 4 + 1);
}
/*****************************************************************************/
void
hlral_startreadout(board, mode)
	int board, mode;
{
	int res;

	hlral_reset(board);

	if (rawmem >= 0) {
		res = ioctl(hlhandle[board], HLSTARTDMA, 0);
		if (res < 0)
			printf("hlral_startreadout: startdma: %s\n", strerror(errno));
	}

	hlsend(board, 0 | SET_DREG); /* no external strobe */

	if (mode & 2)
		hlsend(board, CTL_LD_TST); /* use test registers */
	mode &= 1;

	if (mode)
		hlsend(board, ROUT_ACT_TEST); /* total readout */
	else
		hlsend(board, ROUT_ACT_NORM);
	ralmode[board] = mode;
if (info==0)
  {
  int res, id;
  res=syncmod_attach("/tmp/sync0", &id);
  info=syncmod_getinfo(id);
  printf("syncmod: res=%d, id=%d, info=%p\n", res, id, info);
  }
}
/*****************************************************************************/
/*****************************************************************************/
