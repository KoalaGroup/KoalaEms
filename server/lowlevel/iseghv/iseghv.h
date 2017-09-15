/*
 * $ZEL: iseghv.h,v 1.1 2006/04/20 13:48:53 peterka Exp $
 * ems/server/lowlevel/iseghv/iseghv.h
 * 
 * (re)created: 09.1.2006 pk
 */
 
#ifndef	_iseghv_low_h_
#define	_iseghv_low_h_

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <emsctypes.h>
#include "../devices.h"
#include "libpcan.h"

int iseghv_low_printuse(FILE* outfilepath);
plerrcode iseghv_low_write(ems_u32 id, ems_u32 *buff, ems_u32 len);
plerrcode iseghv_low_read(ems_u32 *id, ems_u32 *buff, ems_u32 *len);
errcode iseghv_low_init(char*);		/*	(void)	*/
errcode iseghv_low_done(void);	

/*errcode peakpci_modules(HANDLE hnd);
errcode peakpci_printf_canframe(HANDLE hnd);
void peakpci_printf_canbuffer(TPCANRdMsg	tmsg);	*/

#endif
/*****************************************************************************/
