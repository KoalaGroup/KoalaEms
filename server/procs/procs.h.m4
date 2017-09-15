/*
 * procs/listprocs.h.m4
 * created: 06.02.1999 PW
 */
/* $Id: procs.h.m4,v 1.5 2006/02/23 18:37:55 wuestner Exp $ */

#ifndef _procs_h_
#define _procs_h_
#include <sconf.h>
#include <sys/types.h>
#include <emsctypes.h>

/*****************************************************************************/

define(`proc',`plerrcode proc_$1(ems_u32*);
plerrcode test_proc_$1(ems_u32*);
#ifdef PROCPROPS
procprop* prop_proc_$1(void);
#endif
extern char name_proc_$1[];
extern int ver_proc_$1;')
include(procedures)

#endif

/* defined in procs/proclist.c */
extern int perfbedarf;
extern char* perfnames[];

/*****************************************************************************/
/*****************************************************************************/
