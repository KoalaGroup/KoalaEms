/*
 * common/get_mount_dev.h
 * created 2011-Apr-11 PW
 * $ZEL: get_mount_dev.h,v 2.1 2011/04/12 16:18:28 wuestner Exp $
 */

#ifndef _get_mount_dev_h_
#define _get_mount_dev_h_

#include <stdio.h>
#include <cdefs.h>

__BEGIN_DECLS
int get_mount_dev(const char *name, dev_t *dev, FILE *log);
__END_DECLS

#endif
