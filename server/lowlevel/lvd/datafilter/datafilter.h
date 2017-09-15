/*
 * lowlevel/lvd/datafilter/datafilter.h
 * created 2010-Feb-03 PW
 * $ZEL: datafilter.h,v 1.2 2010/04/20 10:04:46 wuestner Exp $
 */

#ifndef _datafilter_h_
#define _datafilter_h_

#include <sconf.h>
#include "../lvdbus.h"

int       lvd_list_datafilter(ems_u32 *dest);
plerrcode lvd_add_datafilter(struct lvd_dev* dev, const char *name,
            ems_u32 *args, int num, int front);
int       lvd_get_datafilter(struct lvd_dev* dev, ems_u32 *dest);
void      lvd_clear_datafilter(struct lvd_dev* dev);
void      lvd_free_datafilter(struct datafilter *filter);


plerrcode filter_sync_statist_init(struct datafilter *filter);
plerrcode filter_sync_statist_filter(struct datafilter*, ems_u32 *data, int *len);

plerrcode filter_qdc_sparse_init(struct datafilter *filter);
plerrcode filter_qdc_sparse_filter(struct datafilter*, ems_u32 *data, int *len);

plerrcode filter_add_header_init(struct datafilter *filter);
plerrcode filter_add_header_filter(struct datafilter*, ems_u32 *data, int *len);

#endif
