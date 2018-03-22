/*
 * lowlevel/oscompat/tsleep.c
 * created: long long ago
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: tsleep.c,v 1.5 2016/05/12 21:06:28 wuestner Exp $";

#include <errno.h>
#include <string.h>
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

int
microsleep(int us)
{
    struct timespec ts={0, 0};

    ts.tv_nsec=us*1000;
    if (nanosleep(&ts, 0)<0) {
        printf("nanosleep: %s\n", strerror(errno));
    }
    return 0;
}
