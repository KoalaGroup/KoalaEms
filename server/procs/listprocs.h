/******************************************************************************
*                                                                             *
* listprocs.h                                                                 *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 11.11.94                                                                    *
*                                                                             *
******************************************************************************/

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
extern int NrOfProcs;
errcode getlistproclist(ems_u32* p, unsigned int num);
errcode getlistprocprop(ems_u32* p, unsigned int num);

#endif

/*****************************************************************************/
/*****************************************************************************/
