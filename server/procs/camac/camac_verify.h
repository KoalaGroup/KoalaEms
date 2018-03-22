/*
 * procs/unixcamac/camac_verify.h
 * $ZEL: camac_verify.h,v 1.2 2017/10/20 23:20:52 wuestner Exp $
 * created 07.Sep.2002 PW
 *
 */

#ifndef _camac_verify_h_
#define _camac_verify_h_

plerrcode test_proc_camac(unsigned int* list, ems_u32* module_types);
plerrcode test_proc_camac1(unsigned int* list, unsigned int idx,
        ems_u32* module_types);

#endif
