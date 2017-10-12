/*
 * lowlevel/can/peakpci/can_peakpci.h
 * 
 * $ZEL: can_peakpci.h,v 1.2 2006/08/04 19:09:57 wuestner Exp $
 */

#ifndef _can_peakpci_h_
#define _can_peakpci_h_

#include "../canbus.h"

errcode can_init_peakpci(char* pathname, struct can_dev* dev);

#endif
