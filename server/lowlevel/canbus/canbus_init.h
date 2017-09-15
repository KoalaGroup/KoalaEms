/*
 * lowlevel/canbus/canbus_init.h
 * created: 2007-Jul-02 PW
 * $ZEL: canbus_init.h,v 1.1 2007/08/10 14:05:27 wuestner Exp $
 */

#ifndef _canbus_init_h_
#define _canbus_init_h_

#include "can.h"

errcode can_init_peakpci(struct canbus_dev* dev);
errcode can_init_esd331(struct canbus_dev* dev);

#endif
