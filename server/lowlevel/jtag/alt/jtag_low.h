/*
 * lowlevel/jtag/jtag_low.h
 * created 12.Aug.2005 PW
 * $ZEL: jtag_low.h,v 1.1 2006/09/15 17:54:01 wuestner Exp $
 */

#ifndef _jtag_low_h_
#define _jtag_low_h_

#include <sconf.h>
#include <stdio.h>
#include <errorcodes.h>
#include <emsctypes.h>

#define JTAG_TRACE

struct generic_dev;
struct jtag_chain;
struct jtag_chip;

plerrcode jtag_trace(struct generic_dev* dev, ems_u32 on, ems_u32* old);
void jtag_free_chain(struct jtag_chain* chain);
void jtag_dump_chain(struct jtag_chain* chain);
int jtag_irlen(struct jtag_chain* chain);
int jtag_datalen(struct jtag_chain* chain);
/*int jtag_id(struct jtag_chip* chip, ems_u32* id);*/

int jtag_instruction(struct jtag_chip* chip, ems_u32 icode, ems_u32 *ret,
    int do_sleep);

int jtag_read_XC18V00(struct jtag_chip* chip, ems_u8* data);
int jtag_write_XC18V00(struct jtag_chip* chip, ems_u8* data);

#endif
