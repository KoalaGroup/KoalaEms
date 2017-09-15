/*
 * procs/unixcamac/camac_verify.h
 * $ZEL: camac_verify.h,v 1.1 2004/06/18 23:22:23 wuestner Exp $
 * created 07.Sep.2002 PW
 *
 */

#ifndef _camac_verify_h_
#define _camac_verify_h_

plerrcode test_proc_camac(int* list, ems_u32* module_types);
plerrcode test_proc_camac1(int* list, int idx, ems_u32* module_types);

#endif
