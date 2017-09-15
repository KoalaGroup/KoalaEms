/*
 * lowlevel/fastbus/sfi/sfilib.h
 * 
 * created      ?
 * last changed 14.03.2000 PW
 * 
 * $ZEL: sfilib.h,v 1.3 2002/09/28 17:40:58 wuestner Exp $
 */

#ifndef _sfilib_h_
#define _sfilib_h_
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sconf.h>
#include <dev/vme/vmevar.h>
#include <dev/vme/sfivar.h>
#include <dev/vme/sfi_map.h>
#include "../../sync/pci_zel/synchrolib.h"

#ifdef SFISWAP
#define SFI2H(x) ntohl(x)
#define H2SFI(x) ntohl(x)
#else
#define SFI2H(x) (x)
#define H2SFI(x) (x)
#endif

#define SFI_R(info, x) SFI2H((((sfi_r)(info)->base)->x))
#define SFI_W(info, x, v) (((sfi_w)(info)->base)->x=H2SFI(v))
#define SEQ_W(info, x, v) SFI_W(info, seq[x], v)

typedef struct
  {
  char* pathname;
  int path;
#ifdef SFIMAPPED
  caddr_t base;
  caddr_t backbase;
  caddr_t dma_vmebase;
#endif
  int syncid;
  syncmod_info* syncinf;
  int inbit, outbit;
  struct sfi_status status;
  } sfi_info;

typedef struct
  {
  u_int32_t cmd;
  u_int32_t data;
  } sfi_command;

typedef struct
  {
  int size;
  sfi_command* sequence;
  } sfi_sequence;

int open_sfi(char*, sfi_info*);
int close_sfi(sfi_info*);
#ifdef SFIMAPPED
int sfi_backmap(sfi_info*, u_int32_t*, int);
#endif
int sfi_arblevel(sfi_info*, int);
int sfi_timeout(sfi_info*, int, int);
void sfi_listaddresses(sfi_info*);
int sfi_store_sequence(sfi_info*, sfi_command[], int, u_int32_t);
int sfi_execute_sequence(sfi_info*, sfi_command[], int);
int sfi_execute_stored_sequence(sfi_info*, u_int32_t);
void sfi_reset(sfi_info* info);
void sfi_restart_sequencer(sfi_info*);
void sfi_stop_sequencer(sfi_info*);
int sfi_read_fifo(sfi_info*, u_int32_t*, int);
int sfi_read_fifoword(sfi_info*);
int sfi_print_status(sfi_info*);
void sfi_reporterror(struct sfi_status*);
void sfi_sequencer_status(sfi_info*, int verbose);
int sfi_wait_sequencer(sfi_info* info);

int sscode_(sfi_info*);

int sfi_open(char* path, sfi_info* info);
int sfi_close(sfi_info* info);

#endif
