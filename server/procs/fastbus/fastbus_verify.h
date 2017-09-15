/*
 * procs/fastbus/fastbus_verify.h
 * $ZEL: fastbus_verify.h,v 1.2 2002/11/06 15:37:26 wuestner Exp $
 * created 05.Oct.2002 PW
 *
 */

#ifndef _fastbus_verify_h_
#define _fastbus_verify_h_

#include <emsctypes.h>

plerrcode verify_fastbus_id(struct fastbus_dev* dev, ems_u32 addr, int modultype);
plerrcode verify_fastbus_module(ml_entry* module);
ems_u32 fastbus_module_id(struct fastbus_dev* dev, ems_u32 addr);

void scan_fastbus_crate(struct fastbus_dev* dev, ems_u32 addr, ems_u32 incr, int num);

plerrcode test_proc_fastbus(int* list, ems_u32* module_types);

#endif
