/*
 * common/xprintf.h
 * $ZEL: xprintf.h,v 2.1 2013/01/17 22:38:02 wuestner Exp $
 *
 * created 2013-01-17 PW
 */

#ifndef _xprintf_h_
#define _xprintf_h_

void xprintf_init(void **xpp);
void xprintf_done(void **xpp);
const char* xprintf_get(void *vxp);
/*int xprintf_print(FILE *f, void *vxp);*/
int xprintf(void *vxp, const char *format, ...);

#endif
