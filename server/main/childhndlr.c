/******************************************************************************
*                                                                             *
* childhndlr.h                                                                *
*                                                                             *
* created 08.10.97                                                            *
* last changed 08.10.97                                                       *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: childhndlr.c,v 1.5 2011/04/06 20:30:28 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
//#define _XOPEN_SOURCE_EXTENDED
#include <sys/wait.h>
//#undef _XOPEN_SOURCE_EXTENDED
#include <rcs_ids.h>
#include "childhndlr.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

struct childcallback {
    pid_t pid;
    childproc proc;
    union callbackdata data;
};

static struct childcallback* callbacks=0;
static int callbacksize=0, callbacknum=0;

RCS_REGISTER(cvsid, "main")

/*****************************************************************************/
void
child_dispatcher(int sig)
{
    pid_t pid;
    int status, idx;
    int flags=WNOHANG|WUNTRACED;
#ifdef __osf__
    flags|=WCONTINUED;
#endif

    while ((pid=wait3(&status, flags, 0))>0) {
        if (WIFEXITED(status)) {
            int exitstatus=WEXITSTATUS(status);
            printf("child %d terminated; exitcode %d\n", pid, exitstatus);
        }
        if (WIFSIGNALED(status)) {
            int signal=WTERMSIG(status);
            printf("child %d terminated; signal %d\n", pid, signal);
        }
        if (WIFSTOPPED(status)) {
            int signal=WSTOPSIG(status);
            printf("child %d suspended; signal %d\n", pid, signal);
        }
#ifdef __osf__
        if (WIFCONTINUED(status)) {
            printf("child %d reactivated\n", pid);
        }
#endif
        for (idx=0; (idx<callbacknum) && (callbacks[idx].pid!=pid); idx++);
        if (idx<callbacknum) {
            callbacks[idx].proc(pid, status, callbacks[idx].data);
        } else {
            printf("child_dispatcher: pid %d not found\n", pid);
        }
    }
}
/*****************************************************************************/
int
install_childhandler(childproc proc, pid_t pid, union callbackdata data,
        const char* caller)
{
    int idx;
    printf("install_childhandler(proc=%p, pid=%d, ...) called from %s\n",
            proc, pid, caller);
    if (pid==0)
        return -1;
    for (idx=0; (idx<callbacknum) && (callbacks[idx].pid!=pid); idx++);
    if (idx<callbacknum) {
        if (proc) {
            printf("childhandler: proc for pid %d changed\n", pid);
            callbacks[idx].proc=proc;
            callbacks[idx].data=data;
        } else {
            int i;
            printf("childhandler: proc for pid %d deleted\n", pid);
            for (i=idx; i<callbacknum-1; i++)
                callbacks[i]=callbacks[i+1];
            callbacknum--;
        }
    } else if (proc!=0) {
        if (callbacknum==callbacksize) {
            struct childcallback* help;
            callbacksize=callbacksize?callbacksize*2:10;
            help=(struct childcallback*)malloc(
                        callbacksize*sizeof(struct childcallback));
            for (idx=0; idx<callbacknum; idx++)
                help[idx]=callbacks[idx];
            for (; idx<callbacksize; idx++)
                help[idx].pid=0;
            free(callbacks);
            callbacks=help;
        }
        callbacks[callbacknum].pid=pid;
        callbacks[callbacknum].proc=proc;
        callbacks[callbacknum].data=data;
        callbacknum++;
        printf("childhandler: proc for pid %d set\n", pid);
    } else {
        printf("install_childhandler: pid %d not found\n", pid);
        return -1;
    }
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
