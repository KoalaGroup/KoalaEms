static const char* cvsid __attribute__((unused))=
    "$ZEL: tsleep.c,v 1.4 2011/04/06 20:30:27 wuestner Exp $";

#include <rcs_ids.h>

RCS_REGISTER(cvsid, "lowlevel/oscompat")

#ifndef __linux
#define _GNU_SOURCE
#endif
#include <unistd.h>
#include "oscompat.h"

#if 0
void tsleep(int hunds)
{
	usleep(10000 * hunds);
}
#else
#include <time.h>
void tsleep(int hunds)
{
    struct timespec rqtp={hunds/100, (hunds%100)*10000000};
    struct timespec rmtp;

    nanosleep(&rqtp, &rmtp);
}
#endif
