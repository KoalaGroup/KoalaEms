/*
 * lowlevel/lvd/lvd_initfuncs.h
 * created 2007-Sep-04 PW
 * $ZEL: lvd_initfuncs.h,v 1.4 2009/12/03 00:04:53 wuestner Exp $
 */

#ifndef _lvd_initfuncs_h_
#define _lvd_initfuncs_h_

#include <sconf.h>
#include <errorcodes.h>

struct lvd_acard;

int lvd_qdc_acard_init(struct lvd_dev* dev, struct lvd_acard* acard);
int lvd_f1_acard_init(struct lvd_dev* dev, struct lvd_acard* acard);
int lvd_gpx_acard_init(struct lvd_dev* dev, struct lvd_acard* acard);
int lvd_vertex_acard_init(struct lvd_dev* dev, struct lvd_acard* acard);
int lvd_msync_acard_init(struct lvd_dev* dev, struct lvd_acard* acard);
int lvd_osync_acard_init(struct lvd_dev* dev, struct lvd_acard* acard);
int lvd_trig_acard_init(struct lvd_dev* dev, struct lvd_acard* acard);

#endif
