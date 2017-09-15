/*
 * lowlevel/jtag/jtag_proc.h
 * created 2006-Jul_05 PW
 * $ZEL: jtag_proc.h,v 1.3 2008/03/11 20:26:08 wuestner Exp $
 */

#ifndef _jtag_proc_h_
#define _jtag_proc_h_

#include <sconf.h>
#include <emsctypes.h>
#include "../devices.h"

/*
 * this is the only information which is exported to ems procedures
 */

plerrcode jtag_trace(struct generic_dev* dev, ems_u32 on, ems_u32* old);
plerrcode jtag_loopcal(struct generic_dev* dev, int ci, int addr);
plerrcode jtag_list(struct generic_dev* dev, int ci, int addr);
plerrcode jtag_check(struct generic_dev* dev, int ci, int addr);
plerrcode jtag_read(struct generic_dev* dev, int ci, int addr,
    unsigned int chip_idx, const char* name, ems_u32* usercode);
plerrcode jtag_usercode(struct generic_dev* dev, int ci, int addr,
    unsigned int chip_idx, ems_u32* usercode);
plerrcode jtag_write(struct generic_dev* dev, int ci, int addr,
    unsigned int chip_idx, const char* name, int use_usercode,
    ems_u32 usercode);
plerrcode jtag_verify(struct generic_dev* dev, int ci, int addr,
    unsigned int chip_idx, const char* name);

#endif
