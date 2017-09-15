/*
 * events++/make_file.cc
 *
 * created: ???
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include "make_file.hxx"
#include <versions.hxx>

VERSION("Feb 10 2004", __FILE__, __DATE__, __TIME__,
"$ZEL: make_file.cc,v 1.4 2004/11/26 14:40:19 wuestner Exp $")
#define XVERSION


void
make_file(fileinfo& info)
{
    char* base=basename(info.name);
    char filename[1024];
    int count=0, errnum=0;

    if (info.dirname && info.dirname[0])
        sprintf(filename, "%s/%s;%d", info.dirname, base, ++count);
    else
        sprintf(filename, "%s;%d", base, ++count);
    do {
        info.path=open(filename, O_WRONLY|O_CREAT|O_EXCL, 0644);
        if (info.path<0) {
            if ((errnum=errno)==EEXIST) {
                if (info.dirname && info.dirname[0])
                    sprintf(filename, "%s/%s;%d", info.dirname, base, ++count);
                else
                    sprintf(filename, "%s;%d", base, ++count);
            } else {
                cerr<<"cannot create file "<<filename<<": "<<strerror(errno)<<endl;
            }
        } else {
            info.pathname=strdup(filename);
        }
    }
    while ((info.path<0) && (errnum==EEXIST));
}

void fileinfo::clear()
{
    free(name);
    name=0;
    free(pathname);
    pathname=0;
    path=-1;
}
