/*
 * procs/unixvme/mesytec/mesytec_common.h
 * created 2011-09-22 PeWue
 *
 * "$ZEL: mesytec_common.h,v 1.1 2015/10/24 14:50:30 wuestner Exp $"
 */

#ifndef _mesytec_common_h_
#define _mesytec_common_h_

int is_mesytecvme(ml_entry* module, ems_u32 *modtypes);
int nr_mxdc_modules(ems_u32 *modtypes);
plerrcode for_each_mxdc_member(ems_u32*, ems_u32*, plerrcode(*)(ems_u32*));
plerrcode for_each_mxdc_module(ems_u32 *, plerrcode(*)(ml_entry*));
//static int nr_mxdc_modules(ems_u32 *);
int mxdc_member_idx(void);
plerrcode mxdc32_init_private(ml_entry*, ems_u32, ems_u32, ems_u32);


#endif
