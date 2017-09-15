/*
 * lowlevel/mmaped_data/mmapped_data.h
 * 
 * created: 2011-08-12 PW
 * 
 * $ZEL: mmapped_data.h,v 1.1 2011/08/13 19:27:47 wuestner Exp $
 */
#ifndef _mmapped_data_h_
#define _mmapped_data_h_

#include <errorcodes.h>
#include <emsctypes.h>

errcode mmapped_data_low_init(char*);
errcode mmapped_data_low_done(void);
int mmapped_data_low_printuse(FILE*);

#endif
