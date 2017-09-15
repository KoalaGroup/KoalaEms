/*
 * daemon.cc
 * 
 * created before 13.04.94
 */

#include <cstdio>
#include <iostream>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <config.h>
#ifdef HAVE_PATHS_H
#include <paths.h>
#else
#include <lpaths.h>
#endif

#include "versions.hxx"

VERSION("2009-Feb-25", __FILE__, __DATE__, __TIME__,
"$ZEL: daemon.cc,v 2.8 2009/08/21 21:50:51 wuestner Exp $")
#define XVERSION

/*****************************************************************************/
int cdaemon(int nochdir, int noclose)
{
    int cpid;

    if ((cpid = fork()) == -1)
        return -1;
    if (cpid)
        exit(0);

    setsid();
    if (!nochdir) {
        if (chdir("/"))
            std::cerr<<"chdir to / failed"<<std::endl;
    }
    if (!noclose) {
        int devnull;
        int devcons;
        devcons = open(_PATH_CONSOLE, O_WRONLY, 0);
        devnull = open(_PATH_DEVNULL, O_RDWR, 0);
        if (devcons != -1) {
            dup2(devcons, STDOUT_FILENO);
            dup2(devcons, STDERR_FILENO);
            if (devcons>2)
                close(devcons);
        }
        if (devnull != -1) {
            dup2(devnull, STDIN_FILENO);
            if (devnull>2)
                close(devcons);
        }
    }
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
