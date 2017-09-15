/*
 * $ZEL: caenet.h,v 1.1 2007/03/20 20:40:31 wuestner Exp $
 * ems/server/lowlevel/caenet/caenet.h
 * 
 * created: 2007-Mar-20 PW
 */

#ifndef _caenet_h_
#define _caenet_h_

#include <stdio.h>

int caenet_low_printuse(FILE* outfilepath);
errcode caenet_low_init(char*);
errcode caenet_low_done(void);

#endif

/*****************************************************************************/
