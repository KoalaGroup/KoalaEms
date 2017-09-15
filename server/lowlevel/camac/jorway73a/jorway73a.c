#include <sconf.h>
#include <errorcodes.h>
#include <debug.h>

#include <getcap.h>
#include "ieee_fun_types.h"
#include "jorway73a.h"

static int camachandle = -1;

int CAMACread(camadr_t ext_add)
	{
	int cmd, data;
	T(CAMACread)
	cmd = ext_add | SJY_D24_MODE | (SJY_STOP_ON_WORD << 16);
	sjy_read(BRANCH, cmd, &data, sizeof(data));
	sjy_get_qx(BRANCH);
	data = (data & 0x00ffffff) | (ieee_last_Q << 31) | (ieee_last_X << 30);
	return data;
	}

void CAMACcntl(camadr_t ext_add)
	{
	int cmd;
	T(CAMACcntl)
	cmd = ext_add | SJY_D24_MODE;
	sjy_nondata(BRANCH, cmd);
	sjy_get_qx(BRANCH);
	}

void CAMACwrite(camadr_t ext_add, int data)
	{
	int cmd;
	T(CAMACwrite)
	cmd = ext_add | SJY_D24_MODE | (SJY_STOP_ON_WORD << 16);
	sjy_write(BRANCH, cmd, &data, sizeof(data));
	sjy_get_qx(BRANCH);
	}

int CAMACstatus()
	{
	T(CAMACstatus)
	return (ieee_last_Q << 31) | (ieee_last_X << 30);
	}

camadr_t CAMACincFunc(camadr_t ext_add)
	{
	int a, f, n;
	T(CAMACincFunc)
	f = (ext_add & 0xff000000) >> 24;
	n = (ext_add & 0x00ff0000) >> 16;
	a = (ext_add & 0x0000ff00) >>  8;
	a++;
	if (a>=16)
		{
		a = 0;
		n++;
		}
	return SJY_CAMAC_EXT(f, n, a, 0);
	}

void CCCC()
	{
	int cmd;
	T(CCCC)
	cmd = SJY_CAMAC_EXT(26, 28, 9, 0);
	sjy_nondata(BRANCH, cmd);
	sjy_get_qx(BRANCH);
	}

void CCCZ()
	{
	int cmd;
	T(CCCZ)
	cmd = SJY_CAMAC_EXT(26, 28, 8, 0);
	sjy_nondata(BRANCH, cmd);
	sjy_get_qx(BRANCH);
	}

void CCCI(int flag)
	{
	int cmd;
	T(CCCI)
	if (flag==0)
		cmd = SJY_CAMAC_EXT(24, 30, 9, 0);
	else
		cmd = SJY_CAMAC_EXT(26, 30, 9, 0);
	sjy_nondata(BRANCH, cmd);
	sjy_get_qx(BRANCH);
	}

int CAMAClams()
	{
	int cmd, lam;
	T(CAMAClams)
	cmd = SJY_CAMAC_EXT(0, 30, 0, 0);
	sjy_read(BRANCH, cmd, &lam, sizeof(lam));
	sjy_get_qx(BRANCH);
	return lam;
	}

#define CDCHN_OPEN 1
#define CDCHN_CLOSE 0

errcode camac_jorway73a_low_init(char *args)
	{
	int branch = 0;
	long argval;
	T(camac_jorway73a_low_init)
	if (!args)
		{
		printf("LOW_INIT: no argument!\n");
		return Err_ArgNum;
		}
	else
		{
		if (cgetnum(args, "camid", &argval)==0)
			{
			branch = (int) argval;
			if (branch < 0 || branch > 7)
				{
				printf("LOW_INIT: SCSIid = %d\n",branch);
				return Err_ArgRange;
				}
			}
		else
			{
			printf("LOW_INIT: wrong argument: %s\n",args);
			return Err_ArgRange;
			}
		}
	printf("LOW_INIT: if it blocks here,\n");
	printf("          - remove semaphore\n");
	printf("          - delete /tmp/sjykeyfile\n");
	if (cdchn(branch, CDCHN_OPEN, 0) != CAM_S_SUCCESS)
		{
		printf("LOW_INIT: Error in CDCHN_OPEN\n");
		return Err_System;
		}
	camachandle = sjy_branch[branch].fd;
	BRANCH = branch;
	printf("LOW_INIT: sucessfull!\n");
	return OK;
	}
	
errcode camac_jorway73a_low_done()
	{
	T(camac_jorway73a_low_done)
	if (camachandle != -1)
		{
		if (cdchn(BRANCH, CDCHN_CLOSE, 0) != CAM_S_SUCCESS)
			{
			printf("LOW_DONE: Error in CDCHN_CLOSE\n");
			return Err_System;
			}
		}
	camachandle = -1;
	printf("LOW_DONE: sucessfull!\n");
	return OK;
	}

int camac_jorway73a_low_printuse(FILE* outfilepath)
	{
	T(camac_jorway73a_low_printuse)
	fprintf(outfilepath,"  [:camid#SCSI-id]\n");
	return 1;
	}

int *CAMACblread(int *buffer, camadr_t ext_adr, int num)
	{
	int i;
	T(CAMACblread)
	for (i=0; i<num; i++)
		{
		*buffer++ = CAMACread(ext_adr);
		}
	return buffer;
	}

int *CAMACblreadQstop(int *buffer, camadr_t ext_adr, int num)
	{
	int cmd;
	T(CAMACblreadQstop)
	cmd = ext_adr | SJY_D24_MODE | (SJY_STOP_ON_WORD << 16);
	buffer += sjy_read(BRANCH, cmd, buffer, num*sizeof(int)) / sizeof(int);
	sjy_get_qx(BRANCH);
	return buffer;
	}

int *CAMACblreadAddrScan(int *buffer, camadr_t ext_adr, int num)
	{
	int cmd;
	T(CAMACblreadAddrScan)
	cmd = ext_adr | SJY_D24_MODE | (SJY_ADD_SCAN << 16);
	buffer += sjy_read(BRANCH, cmd, buffer, num*sizeof(int)) / sizeof(int);
	sjy_get_qx(BRANCH);
	return buffer;
	}

int *CAMACblwrite(camadr_t ext_adr, int *buffer, int num)
	{
	int i;
	T(CAMACblwrite)
	for (i=0; i<num; i++)
		{
		CAMACwrite(ext_adr, *buffer++);
		}
	return buffer;
	}

int *CAMACblread16(int *buffer, camadr_t ext_adr, int num)
	{
	T(CAMACblread16)
	}
