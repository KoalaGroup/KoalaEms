/*
 * lowlevel/hlral/hlral.h
 * $ZEL: hlral.h,v 1.7 2008/07/10 13:16:00 wuestner Exp $
 */

#ifndef _hlral_h_
#define _hlral_h_

int hlral_low_printuse(FILE *);
errcode hlral_low_init(char *);
errcode hlral_low_done(void);

#endif
