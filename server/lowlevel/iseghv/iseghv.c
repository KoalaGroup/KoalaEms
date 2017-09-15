/*
 * ems/server/lowlevel/iseghv/iseghv.c
 * created 9.1.2006 pk
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: iseghv.c,v 1.2 2011/04/06 20:30:24 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifndef HAVE_CGETCAP
#include <getcap.h>
#endif
#include <rcs_ids.h>
#include "../devices.h"
#include "../can/peakpci/peakpci.h"
#include "iseghv.h"

RCS_REGISTER(cvsid, "lowlevel/iseghv")

/*****************************************************************************/
int iseghv_low_printuse(FILE* outfilepath){
	fprintf(outfilepath,"  ??? [:can=<canpath>;<cantype>[;rawmempart=1/x][,...]]\n");
	return 1;
}

/*****************************************************************************/
plerrcode iseghv_low_write(ems_u32 id, ems_u32 *buff, ems_u32 len)
{
    TPCANMsg	msg;
    int			i;
	plerrcode	plerr;
	
    if (len < 2 || len > 9) return plErr_ArgRange;
    msg.ID=id;
    msg.LEN=len;
    for (i=0; i<len; i++) msg.DATA[i]=buff[i];
    plerr = peakpci_write(&msg);
    return plOK;
}
/****************************************************************************/
/*	blocking read															*/
plerrcode iseghv_low_read(ems_u32 *id, ems_u32 *buff, ems_u32 *len){
    TPCANMsg	msg;
    int			i;
	plerrcode	plerr;
	
	plerr = peakpci_read(&msg);
	*id = msg.ID;
	*len = msg.LEN;
    /*if (*len < 2 || *len > 9) return plErr_ArgRange;*/
    for (i=0; i<*len; i++) buff[i]=msg.DATA[i];
    return plOK;
}
/*****************************************************************************/
errcode iseghv_low_done(void)
{
    return OK;
}
/*****************************************************************************/
errcode iseghv_low_init(char* param)
{
    return OK;
}
/*****************************************************************************/
