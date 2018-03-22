/*
 * procs/listprocs.h
 * created (before?) 11.11.94
 * $ZEL: listprocs.h,v 1.10 2017/10/20 23:20:52 wuestner Exp $
 */

#ifndef _listprocs_h_
#define _listprocs_h_
#include <errorcodes.h>
#include <sconf.h>
#include <sys/types.h>
#include <emsctypes.h>
#ifdef PROCPROPS
#include "procprops.h"
#endif

typedef struct
  {
  plerrcode (*proc)(ems_u32*);
  plerrcode (*test_proc)(ems_u32*);
  char* name_proc;
  int* ver_proc;
  } listproc;

#ifdef PROCPROPS
typedef struct
  {
  procprop* (*prop_proc)(void);
  } listprop;
#endif

extern listproc Proc[];
extern unsigned int NrOfProcs;
errcode getlistproclist(ems_u32* p, unsigned int num);
errcode getlistprocprop(ems_u32* p, unsigned int num);

#endif

/*****************************************************************************/
/*****************************************************************************/
