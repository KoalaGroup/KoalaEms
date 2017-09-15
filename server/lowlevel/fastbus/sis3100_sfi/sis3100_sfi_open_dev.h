/*
 * $ZEL: sis3100_sfi_open_dev.h,v 1.3 2005/12/04 21:59:20 wuestner Exp $
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_open_dev.h
 * created 2003
 */

#ifndef _sis3100_sfi_open_dev_h_
#define _sis3100_sfi_open_dev_h_

#include <dev/pci/sis1100_var.h>

/*extern const char* sis1100names[];*/
int sis3100_sfi_open_dev(const char*, enum sis1100_subdev);

#endif
