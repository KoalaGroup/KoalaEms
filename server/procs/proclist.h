/*
 * procs/proclist.h
 * created 05.06.97
 * $ZEL: proclist.h,v 1.4 2017/10/09 21:21:39 wuestner Exp $
 */

#ifndef _proclist_h_
#define _proclist_h_

errcode test_proclist(ems_u32* p, unsigned int len, ssize_t* limit);
errcode scan_proclist(ems_u32* p);

#endif
