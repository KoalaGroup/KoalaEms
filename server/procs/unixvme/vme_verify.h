/*
 * procs/unixvme/vme_verify.h
 * $ZEL: vme_verify.h,v 1.7 2017/10/20 23:20:52 wuestner Exp $
 * created 07.Sep.2002 PW
 *
 */

#ifndef _vme_verify_h_
#define _vme_verify_h_
#include "../../lowlevel/unixvme/vme.h"
#include "../../objects/domain/dom_ml.h"

plerrcode verify_vme_id(struct vme_dev* dev, ems_u32 addr,
        ems_u32 modultype, int verbose);
plerrcode verify_vme_module(ml_entry* module, int verbose);
ems_u32 vme_module_id(struct vme_dev* dev, ems_u32 addr);

int scan_vme_crate(struct vme_dev* dev, ems_u32 addr, ems_u32 incr, int num,
        ems_u32* outptr);

plerrcode test_proc_vme(unsigned int* list, ems_u32* module_types);
plerrcode test_proc_vmemodule(ml_entry* module, ems_u32* module_types);

#endif
