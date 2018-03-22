/*
 * procs/listprocs.h.m4
 * created: 06.02.1999 PW
 */
/* $Id: procs.h.m4,v 1.7 2017/10/20 23:20:52 wuestner Exp $ */

#ifndef _procs_h_
#define _procs_h_
#include <sconf.h>
#include <sys/types.h>
#include <emsctypes.h>

/*****************************************************************************/

extern ems_u32* outptr;
extern unsigned int *memberlist;

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
extern ssize_t wirbrauchen; /* defined in proclist.c */

/*****************************************************************************/
/*****************************************************************************/
