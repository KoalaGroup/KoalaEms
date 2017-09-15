/*
 * lowlevel/sync/pci_zel/sync_pci_zel.h
 * created 23.10.96 PW
 */
/*
 * $ZEL: sync_pci_zel.h,v 1.6 2007/10/15 14:28:14 wuestner Exp $
 */

#ifndef _syncmod_h_
#define _syncmod_h_
#include <stdio.h>
#include <errorcodes.h>
#include "synchrolib.h"

int sync_pci_zel_low_printuse(FILE* outfilepath);
errcode sync_pci_zel_low_init(char* arg);
errcode sync_pci_zel_low_done(void);
plerrcode syncmod_attach(char* path, int* id);
int syncmod_valid_id(int id);
plerrcode syncmod_detach(int id);
struct syncmod_info* syncmod_getinfo(int id);
void syncmod_init(int id, int master);
void syncmod_reset(int id);
void syncmod_clear(int id);
void syncmod_write(int id, int reg, unsigned int val);
unsigned int syncmod_read(int id, int reg);
int syncmod_status(int id);
int syncmod_ctime(int id);
int syncmod_evcount(int id);
int syncmod_tmc(int id);
int syncmod_tdt(int id);
int syncmod_reje(int id);

#endif

/*****************************************************************************/
/*****************************************************************************/
