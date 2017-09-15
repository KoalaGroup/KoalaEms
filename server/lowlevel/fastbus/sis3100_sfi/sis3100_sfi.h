/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi.h
 * created 23.May.2002 PW
 * $ZEL: sis3100_sfi.h,v 1.21 2007/04/19 22:17:29 wuestner Exp $
 */

#ifndef _sis3100_sfi_h_
#define _sis3100_sfi_h_

#include <sconf.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <debug.h>
#include <errorcodes.h>
#include <emsctypes.h>
#include "../fastbus.h"
#include "sis3100_sfifastbus.h"
#include "dev/pci/sis1100_var.h"
#include "dev/pci/sis3100_map.h"
#include "dev/vme/sfi_map.h"
#ifdef SIS3100_SFI_MMAPPED
#include "dev/pci/sis1100_map.h"
#endif

/*#define ofs(what, elem) ((off_t)&(((what*)0)->elem))*/
#define ofs(what, elem) ((size_t)&(((what*)0)->elem))

#ifdef DELAYED_READ
struct delayed_hunk {
    off_t src;
    ems_u8* dst;
    size_t size;
};
extern ems_u8* sis3100_sfi_delay_buffer;
extern size_t sis3100_sfi_delay_buffer_size;
void   sis3100_sfi_reset_delayed_read(struct generic_dev*);
int    sis3100_sfi_delay_read(struct fastbus_dev*, off_t, ems_u8*, size_t);
#endif

errcode sis3100_sfi_init_mapped(struct fastbus_dev*);
int     sis3100_sfi_reset(struct fastbus_dev*);
errcode sis3100_sfi_done(struct generic_dev*);
void    sis3100_sfi_reset_delayed_read(struct generic_dev*);
int     sis3100_sfi_read_delayed(struct generic_dev*);
int     sis3100_sfi_setarbitrationlevel(struct fastbus_dev*, unsigned int);
int     sis3100_sfi_getarbitrationlevel(struct fastbus_dev*, unsigned int*);
int     sis3100_sfi_FRC(struct fastbus_dev*, ems_u32 pa, ems_u32 sa,
                ems_u32* data, ems_u32* ss);
int     sis3100_sfi_FRCa(struct fastbus_dev*, ems_u32 pa, ems_u32 sa,
                ems_u32* data, ems_u32* ss);
int     sis3100_sfi_FRD(struct fastbus_dev*, ems_u32 pa, ems_u32 sa,
                ems_u32* data, ems_u32* ss);
int     sis3100_sfi_FRDa(struct fastbus_dev*, ems_u32 pa, ems_u32 sa,
                ems_u32* data, ems_u32* ss);
int     sis3100_sfi_multiFRC(struct fastbus_dev*, unsigned int num, ems_u32* pa,
                ems_u32 sa, ems_u32* data, ems_u32* ss);
int     sis3100_sfi_multiFRD(struct fastbus_dev*, unsigned int num, ems_u32* pa,
                ems_u32 sa, ems_u32* data, ems_u32* ss);
int     sis3100_sfi_FWC(struct fastbus_dev*, ems_u32, ems_u32, ems_u32, ems_u32*);
int     sis3100_sfi_FWCa(struct fastbus_dev*, ems_u32, ems_u32, ems_u32, ems_u32*);
int     sis3100_sfi_FWD(struct fastbus_dev*, ems_u32, ems_u32, ems_u32, ems_u32*);
int     sis3100_sfi_FWDa(struct fastbus_dev*, ems_u32, ems_u32, ems_u32, ems_u32*);
int     sis3100_sfi_FRCB(struct fastbus_dev*, ems_u32 pa, ems_u32 sa,
            unsigned int count, ems_u32* dest, ems_u32* ss, const char* caller);
int     sis3100_sfi_FRDB(struct fastbus_dev*, ems_u32 pa, ems_u32 sa,
            unsigned int count, ems_u32* dest, ems_u32* ss, const char* caller);
void    sis3100_sfi_FRDB_perfspect_needs(struct fastbus_dev*, int*, char**);
int     sis3100_sfi_FRDB_S(struct fastbus_dev*, ems_u32, ems_u32, ems_u32*, ems_u32*, const char* caller);
void    sis3100_sfi_FRDB_S_perfspect_needs(struct fastbus_dev*, int*, char**);
int     sis3100_sfi_FRCM(struct fastbus_dev*, ems_u32 ba, ems_u32* data,
            ems_u32* ss);
int     sis3100_sfi_FRDM(struct fastbus_dev*, ems_u32 ba, ems_u32* data,
            ems_u32* ss);
int     sis3100_sfi_FWCM(struct fastbus_dev*, ems_u32 ba, ems_u32 data,
            ems_u32* ss);
int     sis3100_sfi_FWDM(struct fastbus_dev*, ems_u32 ba, ems_u32 data,
            ems_u32* ss);
int     sis3100_sfi_FCPC(struct fastbus_dev*, ems_u32 pa, ems_u32* ss);
int     sis3100_sfi_FCPD(struct fastbus_dev*, ems_u32 pa, ems_u32* ss);
int     sis3100_sfi_FCDISC(struct fastbus_dev* dev);
int     sis3100_sfi_FCRW(struct fastbus_dev*, ems_u32* data, ems_u32* ss);
int     sis3100_sfi_FCRWS(struct fastbus_dev*, ems_u32 sa, ems_u32* data,
            ems_u32* ss);
int     sis3100_sfi_FCWSA(struct fastbus_dev*, ems_u32 sa, ems_u32* ss);
int     sis3100_sfi_FCWWS(struct fastbus_dev*, ems_u32 sa, ems_u32 data,
            ems_u32* ss);
int     sis3100_sfi_FCWW(struct fastbus_dev*, ems_u32 data, ems_u32*);
int     sis3100_sfi_LEVELIN(struct fastbus_dev*, int opt, ems_u32* dest);
int     sis3100_sfi_LEVELOUT(struct fastbus_dev*, int opt,
            ems_u32 set, ems_u32 res);
int     sis3100_sfi_PULSEOUT(struct fastbus_dev*, int, ems_u32 mask);
int     sis3100_sfi_FRDB_multi_init(struct fastbus_dev*, int addr,
            unsigned int num_pa, ems_u32* pa, ems_u32 sa, unsigned int max);
int     sis3100_sfi_FRDB_multi_read(struct fastbus_dev*, int addr,
            ems_u32* dest, int* count);
int     sis3100_sfi_load_list(struct fastbus_dev*, int, int, struct sficommand*);
#ifdef XXX
struct dsp_procs* sis3100_sfi_get_dsp_procs(struct fastbus_dev*);
struct mem_procs* sis3100_sfi_get_mem_procs(struct fastbus_dev*);
#endif
int     sis3100_restart_sequencer(struct fastbus_dev* dev);
int     sis3100_sfi_status(struct fastbus_dev* dev, ems_u32* status, int max);

int     sis3100_sfi_read(struct fastbus_dev*, int, volatile ems_u32*);
int     sis3100_sfi_write(struct fastbus_dev*, int, volatile ems_u32);
int     sis3100_vme_read(struct fastbus_dev*, int, volatile ems_u32*);
int     sis3100_vme_write(struct fastbus_dev*, int, volatile ems_u32);
int     sis3100_sfi_multi_write(struct fastbus_dev*, int, ems_u32*);

int     sis3100_sfi_read_fifoword(struct fastbus_dev*, ems_u32* data, ems_u32*);
int     sis3100_sfi_read_fifo_num(struct fastbus_dev*, ems_u32* dest, int, ems_u32*);
int     sis3100_sfi_read_fifo(struct fastbus_dev*, ems_u32* dest, int, ems_u32*);
int     sis3100_sfi_wait_sequencer(struct fastbus_dev*, ems_u32*);
void    sis3100_sfi_restart_sequencer(struct fastbus_dev*);
int     sis3100_sfi_enable_delayed_read(struct generic_dev*, int);

#ifdef SIS3100_SFI_MMAPPED

#define SFI_MAPSIZE 0x20000
#define SFI_R(dev, x, d)  (*d=((sfi_r)(dev->info.sis3100_sfi.vme_base))->x,0)
#define SFI_R_ioctl(dev, x, d) sis3100_sfi_read(dev, ofs(struct sfi_r, x), (d))
#define SFI_W(dev, x, v)  (((sfi_w)(dev->info.sis3100_sfi.vme_base))->x=v,0)
#define SEQ_W(dev, x, v)  SFI_W(dev, seq[x], v)
#define CTRL_R(dev, x)    (((struct sis1100_reg*)(dev->info.sis3100_sfi.ctrl_base))->x)
#define CTRL_W(dev, x, v) (((struct sis1100_reg*)(dev->info.sis3100_sfi.ctrl_base))->x=v)
#define VME_R(dev, x, d)  (*d=((struct sis3100_reg*)(dev->info.sis3100_sfi.ctrl_base))->x,0)
#define VME_W(dev, x, v)  (((struct sis3100_reg*)(dev->info.sis3100_sfi.ctrl_base))->x=v)
void sis3100_check_balance(struct fastbus_dev* dev, const char* caller);
#define SIS3100_CHECK_BALANCE(dev, caller) do{}while(0)
/*#define SIS3100_CHECK_BALANCE(dev, caller) sis3100_check_balance(dev, caller)*/

#else  /* SIS3100_SFI_MMAPPED */

#define SFI_R(dev, x, d) sis3100_sfi_read(dev, ofs(struct sfi_r, x), (d))
#define SFI_R_ioctl(dev, x, d) SFI_R(dev, x, d)
#define SFI_W(dev, x, v) sis3100_sfi_write(dev, ofs(struct sfi_w, x), (v))
#define SEQ_W(dev, x, v) SFI_W(dev, seq[x], v)
#define VME_R(dev, x, d) sis3100_vme_read(dev, ofs(struct sis3100_reg, x), (d))
#define VME_W(dev, x, v) sis3100_vme_write(dev, ofs(struct sis3100_reg, x), (v))
#define SIS3100_CHECK_BALANCE(dev, caller) do{}while(0)

#endif /* SIS3100_SFI_MMAPPED */

#endif
