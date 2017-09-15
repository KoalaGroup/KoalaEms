/*
 * mkpath.hxx
 */

/* $ZEL: mkpath.hxx,v 2.2 2003/09/30 16:33:28 wuestner Exp $ */

#ifndef _mkpath_hxx_
#define _mkpath_hxx_

#include <fcntl.h>
#include <sys/types.h>

int makedir(const char* path, mode_t mode);
int makefile(const char* path, int oflag, mode_t mode);

#endif
