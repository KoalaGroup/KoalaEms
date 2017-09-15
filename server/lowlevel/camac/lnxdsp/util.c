/*
 * $ZEL: util.c,v 1.5 2005/12/04 21:59:13 wuestner Exp $
 */

#include <sconf.h>
#include <debug.h>

#include "camac_lnxdsp.h"
#include "util.h"
#include "pc004.h"
#include "../../common/getcap.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <errorcodes.h>

struct PC004_DATA_T pc004data;

static int camachandle = -1;



camadr_t CAMACaddr(int N, int A, int F)
{
  T(CAMACaddr)
  return ( (N<<16) | (A<<8) | (F) );
}

int CAMACread(camadr_t adr)
{
  int status;
  T(CAMACread)
  pc004data.N = (char)((adr & 0x00ff0000) >> 16);
  pc004data.A = (char)((adr & 0x0000ff00) >>  8);
  pc004data.F = (char)((adr & 0x000000ff) >>  0);
  status = ioctl(camachandle, PC004_CAMI, &pc004data);
  return (pc004data.DATA & 0x80ffffff);
/*   return (pc004data.DATA); */
}

int CAMACreadX(camadr_t adr)
{
int status;
  T(CAMACreadX)

  pc004data.N = (char)((adr & 0x00ff0000) >> 16);
  pc004data.A = (char)((adr & 0x0000ff00) >>  8);
  pc004data.F = (char)((adr & 0x000000ff) >>  0);
  status = ioctl(camachandle, PC004_CAMI, &pc004data);
  return pc004data.DATA;
}



void CAMACwrite(camadr_t adr, int val)
{
int status;
  T(CAMACwrite)

  pc004data.N = (char)((adr & 0x00ff0000) >> 16);
  pc004data.A = (char)((adr & 0x0000ff00) >>  8);
  pc004data.F = (char)((adr & 0x000000ff) >>  0);
  pc004data.DATA = val;
  status = ioctl(camachandle, PC004_CAMO, &pc004data);
}

void CAMACcntl(camadr_t addr)
{
int status;
  T(CAMACcntl)

  pc004data.N = (char)((addr & 0x00ff0000) >> 16);
  pc004data.A = (char)((addr & 0x0000ff00) >>  8);
  pc004data.F = (char)((addr & 0x000000ff) >>  0);
  status = ioctl(camachandle, PC004_CAMO, &pc004data);
}

int CAMACstatus()
{
int status;
  T(CAMACstatus)

  status = ioctl(camachandle, PC004_GETQX, &pc004data);
  return pc004data.DATA;
}

void CCCC()
{
int status;
  T(CCCC)

  status = ioctl(camachandle, PC004_CAMCLC, &pc004data);
}


void CCCZ()
{
int status;
  T(CCCZ)

  status = ioctl(camachandle, PC004_CAMCLZ, &pc004data);
}

void CCCI(int flag)
{
int status;
  T(CCCI)
  if( flag ) status = ioctl(camachandle, PC004_CAMCLI1, &pc004data);
  else status = ioctl(camachandle, PC004_CAMCLI0, &pc004data);
}

int CAMAClams()
{
int status;
  T(CAMAClams)

  status = ioctl(camachandle, PC004_CAML, &pc004data);
  if(pc004data.DATA) return(1 << (pc004data.DATA -1) );
  else return 0;
}

camadr_t CAMACincFunc(camadr_t adr){
int N, A, F;
  T(CAMACincFunc)

  N = (adr & 0x00ff0000) >> 16;
  A = (adr & 0x0000ff00) >>  8;
  F = (adr & 0x000000ff) >>  0;
  A += 1;
  if(A >= 16){
    A = 0;
    N += 1;
  }
  return (  (N << 16) | (A << 8) | (F << 0) );
}

static  errcode done_lnxdsp(struct generic_dev* gdev)
{
  T(camac_lnxdsp_low_done)
    if(camachandle!=-1){
        close(camachandle);
        camachandle= -1;
    }
    printf("LOW_DONE: sucessfull!\n");
    return(OK);
}

errcode camac_init_lnxdsp(char *args)
	{
	int port = 0;
	long argval;
	char device[256];
	T(camac_lnxdsp_low_init)
	if (!args)
		{
		port = 0;
		}
	else
		{
		if (cgetnum(args, "camid", &argval)==0)
			{
			port = (int) argval;
			if (port < 0 || port > 3)
				{
				printf("LOW_INIT: PORTid = %d\n",port);
				return Err_ArgRange;
				}
			}
		else
			{
			printf("LOW_INIT: wrong argument: %s\n",args);
			return Err_ArgRange;
			}
		}
	snprintf (device, 256, "/dev/pcdsp%1d", port);
	if ((camachandle = open (device, O_RDONLY)) < 0)
		{
		printf("LOW_INIT: Error in opening PC004\n");
		return Err_System;
		}
	printf("LOW_INIT: sucessfull!\n");
	return OK;
}

int camac_lnxdsp_low_printuse(FILE *outfilepath)
	{
	T(camac_lnxdsp_low_printuse)
	fprintf(outfilepath,"  [:camid#PORTid]\n");
	return 1;
	}

int *CAMACblread(int *buffer, camadr_t adr, int num)
{
int i;
  T(CAMACblread)

  for(i=0; i<num; i++){
    *buffer++ = CAMACread(adr);
  }
  return (buffer);
}

int *CAMACblreadAddrScan(int *buffer, camadr_t adr, int num)
{
  T(CAMACblreadAddrScan)
  return (buffer);
}

int CAMACgotQ(int a)
{
  T(CAMACgotQ)
  if( a & 0x80000000) return (1); else return 0;
}



int *CAMACblreadQstop(int *buffer, camadr_t adr, int space)
{
  T(CAMACblreadQstop)

  while(CAMACgotQ(*buffer++ = CAMACread(adr)) && (--space));
  if(!space){
    space=1000; /* Modul leeren */
    while(CAMACgotQ(CAMACread(adr))&&(--space));
    *buffer++=0;
  }
  return(buffer);
}


int *CAMACblwrite(camadr_t adr, int *buffer,  int num)
{
int i;
  T(CAMACblwrite)

  for(i=0; i<num; i++){
    CAMACwrite(adr, *buffer++);
  }
  return (buffer);
}

int *CAMACblread16(int *dest, camadr_t addr, int num)
{
  T(CAMACblread16)
  /* return (CAMACblread16(dest, addr, num) ); */
  return 0;
}
