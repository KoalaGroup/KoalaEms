/*
 * lowlevel/unixvme/unixvme.h
 * created 23.Jan.2001 PW
 * $ZEL: unixvme.h,v 1.3 2002/06/20 14:41:04 wuestner Exp $
 */

#ifndef _unixvme_h_
#define _unixvme_h_
#include "../../objects/domain/dom_ml.h"

int unixvme_low_printuse(FILE* outfilepath);
errcode unixvme_low_init(char* arg);
errcode unixvme_low_done(void);
errcode valid_vme_module(ml_entry*);

#endif
