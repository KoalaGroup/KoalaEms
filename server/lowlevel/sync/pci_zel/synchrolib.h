/*
 * lowlevel/sync/pci_zel/synchrolib.h
 * created 12.09.96 PW
 * 
 * $ZEL: synchrolib.h,v 1.5 2007/10/15 14:30:45 wuestner Exp $
 */

#ifndef _synchrolib_h_
#define _synchrolib_h_

#include "emsctypes.h"
#include <dev/pci/pcisyncvar.h>

struct syncmod_info
  {
  int valid;
  int count;
  char* pathname;
  int path;
  struct syncintctrl intctrl;
  volatile ems_u32* base;
  void* mapbase;
  size_t mapsize;
  };

/*****************************************************************************/

int map_syncmod(struct syncmod_info* info);
int syncmod_unmap(struct syncmod_info* info);
unsigned int readcsr(struct syncmod_info* info, int offs);
int syncmod_setsig(struct syncmod_info* info, int sig);
int syncmod_intctrl(struct syncmod_info* info, struct syncintctrl* ctrl);
int syncmod_map(struct syncmod_info* info);

#endif

/*****************************************************************************/
/*****************************************************************************/
