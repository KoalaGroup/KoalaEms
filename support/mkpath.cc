/*
 * mkpath.cc
 * 
 * created ??? PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include "mkpath.hxx"

#include "versions.hxx"

VERSION("Mar 27 2003", __FILE__, __DATE__, __TIME__,
"$ZEL: mkpath.cc,v 2.5 2010/02/02 23:48:25 wuestner Exp $")
#define XVERSION

/*****************************************************************************/
static int
check_and_make_dir(char* path, mode_t mode)
{
    struct stat status;

    if (stat(path, &status)==0) {
        if (!S_ISDIR(status.st_mode)) {
            fprintf(stderr, "makedir: %s exists but is not a directory\n", path);
            return -1;
        }
    } else {
        if (mkdir(path, mode)<0) {
            fprintf(stderr, "mkdir(%s): %s\n", path, strerror(errno));
            return -1;
        }
    }
    return 0;
}
/*****************************************************************************/
int
makedir(const char* path, mode_t mode)
{
    const char *p0, *p1, *e;

    e=path+strlen(path);
    p0=path;
    do {
        p1=strchr(p0, '/');
        if (p1!=p0) {
            char start[MAXPATHLEN];
            if (p1) {
                strncpy(start, path, p1-path);
                start[p1-path]=0;
            } else {
                strcpy(start, path);
            }
            if (check_and_make_dir(start, mode)<0)
                return -1;
        }
        p0=p1+1;
    } while (p1 && (p0<e));
    return 0;
}
/*****************************************************************************/
int
makefile(const char* path, int oflag, mode_t mode)
{
    const char* p;
    int f;

    p=strrchr(path, '/');
    if (p) {
        if (p!=path) { /* p==path: path=="/" (it must exist) */
            char dir[MAXPATHLEN];
            strncpy(dir, path, p-path);
            dir[p-path]=0;
            if (makedir(dir, 0755)<0)
                return -1;
        }
    }
    if ((f=open(path, oflag, mode))<0) {
        fprintf(stderr, "open(%s): %s\n", path, strerror(errno));
        return -1;
    }
    return f;
}
/*****************************************************************************/
/*****************************************************************************/
