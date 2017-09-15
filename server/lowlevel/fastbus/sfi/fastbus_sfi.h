#ifndef _fastbus_sfi_h_
#define _fastbus_sfi_h_

#include <stdio.h>
#include <errorcodes.h>

errcode fastbus_sfi_low_init(char* arg);
errcode fastbus_sfi_low_done(void);
int fastbus_sfi_low_printuse(FILE* outfilepath);

#endif
