/*
 * procs/lvd/lvd_verify.h
 * $ZEL: lvd_verify.h,v 1.2 2006/05/22 15:36:29 wuestner Exp $
 * created 2005-Feb-25 PW
 *
 */

#ifndef _lvd_verify_h_
#define _lvd_verify_h_

plerrcode verify_lvd_id(struct lvd_dev* dev, ems_u32 addr, ems_u32 modultype);
plerrcode verify_lvd_ids(struct lvd_dev* dev, ems_u32 addr, ems_u32* modultype);
plerrcode verify_lvd_module(ml_entry* module, int verbose);
ems_u32 lvd_module_id(struct lvd_dev* dev, ems_u32 addr);

void scan_lvd_crate(struct lvd_dev* dev, ems_u32 addr, ems_u32 incr, int num);

plerrcode test_proc_lvd(int* list, ems_u32* module_types);

#endif
