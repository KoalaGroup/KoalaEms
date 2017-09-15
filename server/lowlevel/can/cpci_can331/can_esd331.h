/*
 * lowlevel/can/cpci_can331/can_esd331.h
 * 
 * $ZEL: can_esd331.h,v 1.2 2006/08/04 19:09:55 wuestner Exp $
 */

#ifndef _can_esd331_h_
#define _can_esd331_h_

#include "../canbus.h"

errcode can_init_esd331(char* pathname, struct can_dev* dev);

#endif
