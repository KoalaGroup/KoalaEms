/*
 * lowlevel/jtag/jtag_int.h
 * created 2005-Aug-03 PW
 * $ZEL: jtag_int.h,v 1.4 2008/03/11 20:26:08 wuestner Exp $
 */

#ifndef _jtag_int_h_
#define _jtag_int_h_

#include <sconf.h>
#include <debug.h>
#include <sys/types.h>
#include <emsctypes.h>
#include <errorcodes.h>
#include "jtag_lowlevel.h"

enum jtag_states {
    jtag_TLR, /* test-logic-reset */
    jtag_RTI, /* run-test/idle */
    jtag_SDS, /* select-dr-scan */
    jtag_CD,  /* capture-dr */
    jtag_SD,  /* shift-dr */
    jtag_E1D, /* exit1-dr */
    jtag_PD,  /* pause-dr */
    jtag_E2D, /* exit2-dr */
    jtag_UD,  /* update-dr */
    jtag_SIS, /* select-ir-scan */
    jtag_CI,  /* capture-ir */
    jtag_SI,  /* shift-ir */
    jtag_E1I, /* exit1-ir */
    jtag_PI,  /* pause-ir */
    jtag_E2I, /* exit-ir */
    jtag_UI,  /* update-ir */
};

struct jtag_chain;
struct jtag_chip;

struct jtag_chipdata {
    ems_u32 vendor_id;
    ems_u32 part_id;
    const char* name;
    int ir_len;	          /* length of instruction register */
    int mem_size;
    int write_size;
    int erasetest;
    int (*read)(struct jtag_chip* chip, ems_u8* data, ems_u32* usercode);
    int (*write)(struct jtag_chip* chip, ems_u8* data, ems_u32 usercode);
};

struct jtag_chip {
    struct jtag_chain* chain;
    u_int32_t id;
    const struct jtag_chipdata* chipdata;
    int version;
    int pre_c_bits;
    int after_c_bits;
    int pre_d_bits;
    int after_d_bits;
    ems_u32 ir;
};

struct jtag_chain {
    struct jtag_dev jtag_dev;
    int num_chips;
    struct jtag_chip* chips;
    enum jtag_states jstate;
    int state;
#ifdef JTAG_DEBUG
    int oldjstate;
    int jstate_count;
    int loopcount;
    ems_u32 ichain;
    ems_u32 ochain;
    char dchain[64];
#endif
};

#ifdef JTAG_DEBUG
#define jtag_loopcount(chain, c) (chain)->loopcount=(c)
#else
#define jtag_loopcount(chain, c) do {} while (0)
#endif

#ifdef JTAG_DEBUG
extern int jtag_traced; /* defined in jtag_proc.c */
#endif

int  jtag_force_TLR(struct jtag_chain* chain);
int  jtag_action(struct jtag_chain* chain, int tms, int tdi, int* tdo);
int  jtag_read_ids(struct jtag_chain*, ems_u32** IDs, int* IDnum);
int  jtag_irlen(struct jtag_chain*);
int  jtag_datalen(struct jtag_chain*);
int  jtag_instruction(struct jtag_chip*, ems_u32 icode, ems_u32* ret);
int  jtag_data32(struct jtag_chip*, ems_u32 din, u_int32_t* dout, int len);
int  jtag_data(struct jtag_chain*, ems_u32* d);
int  jtag_rd_data(struct jtag_chip*, ems_u32 *buf, int len);
int  jtag_wr_data(struct jtag_chip*, void *buf, int len, int end);
void jtag_sleep(struct jtag_chain*, int us);

plerrcode jtag_read_data(const char* name, void* data, int size);

#endif
