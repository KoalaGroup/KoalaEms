/*
 * main/requests.h
 * $ZEL: requests.h,v 1.6 2009/08/09 21:43:47 wuestner Exp $
 * created at or before 14.01.93
 */

#ifndef _requests_h_
#define _requests_h_

#include <sys/types.h>
#include <errorcodes.h>
#include <emsctypes.h>

typedef errcode (*RequestHandler)(const ems_u32 *msg, unsigned int size);
extern RequestHandler DoRequest[];
extern const char* RequestName[];

#endif

/*****************************************************************************/
/*****************************************************************************/
