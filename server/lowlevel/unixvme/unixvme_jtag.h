/*
 * lowlevel/unixvme/unixvme_jtag.h
 * created 2005-Aug-03 PW
 * $ZEL: unixvme_jtag.h,v 1.3 2008/03/11 20:29:55 wuestner Exp $
 */

#ifndef _unixvme_jtag_h_
#define _unixvme_jtag_h_

#include <sconf.h>
#include <debug.h>

plerrcode unixvme_init_jtag_dev(struct generic_dev*, void*);

#endif
