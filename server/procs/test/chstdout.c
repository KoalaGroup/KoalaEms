/*
 * procs/test/chstdout.c
 * created before 25.10.96
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: chstdout.c,v 1.11 2011/04/06 20:30:34 wuestner Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#ifdef OSK
#include <modes.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include <errorcodes.h>
#include <sconf.h>
#include <debug.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../procprops.h"
#include "../procs.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

extern ems_u32* outptr;
extern int wirbrauchen;

static int oldout= -1, olderr= -1, dummy;

RCS_REGISTER(cvsid, "procs/test")

/*****************************************************************************/
static plerrcode StdOutFile(char* fname)
{
    int path;
    T(proc_StdOutFile)

    D(D_USER, printf("creat %s\n", fname);)
#ifdef OSK
    path=creat(fname, 3);
#else
    path=open(fname, O_WRONLY|O_CREAT|O_APPEND, 0644);
#endif

    if (path==-1) {
        printf("cannot open \"%s\" as new stdout; errno=%d.\n", fname, errno);
        *outptr++=errno;
        return plErr_System;
    }
    printf("redirect output to %s\n", fname);
    oldout=dup(1);
    olderr=dup(2);
    close(1);
    close(2);
    dummy=dup(path);
    dummy=dup(path);
    close(path);
    printf("output redirected to %s\n", fname);
    return plOK;
}
/*****************************************************************************/
static plerrcode StdOutNorm(void)
{
    T(proc_StdOutNorm)

    if (oldout==-1) {
        printf("output is already directed to stdout.\n");
        return plErr_Other;
    } else {
        printf("redirect output to stdout.\n");
        close(1);
        close(2);
        dummy=dup(oldout);
        dummy=dup(olderr);
        close(oldout);
        oldout= -1;
        close(olderr);
        olderr= -1;
        printf("output is redirected to stdout.\n");
    }
    return plOK;
}
/*****************************************************************************/

static plerrcode StdOut_test(ems_u32* p)
{
T(StdOut_test)
if (p[0]>0)
  {
  if ((xdrstrlen(p+1))!=p[0]) return(plErr_ArgNum);
  wirbrauchen=1;
  }
else
  wirbrauchen=0;
return(plOK);
}

/*****************************************************************************/
/*
StdOut
p[1] : filename
*/

plerrcode proc_StdOut(ems_u32* p)
{
char *fname;
plerrcode res;

T(proc_StdOut)

if (p[0]==0)
  {
  res=StdOutNorm();
  }
else
  {
  fname=(char*)malloc(xdrstrclen(p+1)+1);
  extractstring(fname, p+1);
  res=StdOutFile(fname);
  free(fname);
  }
return(res);
}

plerrcode test_proc_StdOut(ems_u32* p)
{
return(StdOut_test(p));
}
#ifdef PROCPROPS
static procprop StdOut_prop={1, 1, "[pathname]", 0};

procprop* prop_proc_StdOut(void)
{
return(&StdOut_prop);
}
#endif
char name_proc_StdOut[]="StdOut";
int ver_proc_StdOut=2;

/*****************************************************************************/
/*****************************************************************************/
