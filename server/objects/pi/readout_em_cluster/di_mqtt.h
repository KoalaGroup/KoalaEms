/*
 * objects/pi/readout_em_cluster/di_mqtt.h
 * created 2014-09-05 PW
 * 
 * $ZEL: di_mqtt.h,v 1.1 2014/09/10 15:35:54 wuestner Exp $
 */

#ifndef _di_mqtt_h_
#define _di_mqtt_h_

#include <sconf.h>
#include <errorcodes.h>
#include <emsctypes.h>

errcode di_mqtt_init(int, int, ems_u32*);
void init_mqtt(void);
void cleanup_mqtt(void);

#endif

/*****************************************************************************/
/*****************************************************************************/
