/*
 * procs/test/xprintf.c
 * created 2013-01-17 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: xprintf.c,v 1.1 2013/01/17 22:43:45 wuestner Exp $";

#include <stdlib.h>
#include <stdio.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <xprintf.h>
#include <rcs_ids.h>
#include "../procs.h"

extern ems_u32* outptr;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/test")

/*****************************************************************************/
/*
 * p[0]: argcount==1+
 * p[1]: flag: 0 --> stdout, 1 --> outptr
 * p[2..]: 1st string
 * p[...]: 2nd string
 * ...
 */
plerrcode
proc_xprint(ems_u32* p)
{
    const char *xs;
    char *ss;
    void *xp=0;
    ems_u32 *pp;
    int flag, num=0;

    flag=p[1];
    pp=p+2;

    if (flag)
        xprintf_init(&xp);

    while (pp-p<=p[0]) {
        pp=xdrstrcdup(&ss, pp);
        xprintf(xp, "%s\n", ss);
        free(ss);
        num++;
    }
    xprintf(xp, "%d strings found\n", num);

    if (xp) {
        xs=xprintf_get(xp);
        if (flag&2)
            printf("%s", xs);
        outptr=outstring(outptr, xs);
        xprintf_done(&xp);
    }

    return plOK;
}

plerrcode
test_proc_xprint(ems_u32* p)
{
    if (p[0]<1)
        return plErr_ArgNum;
    /* here we should check all the strings, but it's a test procedure ... */

    wirbrauchen=-1;
    return plOK;
}

char name_proc_xprint[]="xprint";
int ver_proc_xprint=1;

/*****************************************************************************/
/*****************************************************************************/
