/*
 * server/procs/dataout/dataoutlog.c
 * 
 * created: 24.Jul.2004 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: dataoutlog.c,v 1.4 2017/10/09 21:25:37 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../objects/domain/dom_dataout.h"
#include "../../objects/do/dataout.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/dataout")

/*****************************************************************************/
/*
 * p[0]: argnum
 * p[1]: do_ID
 * p[2...]: logfilename
 */

plerrcode proc_set_dataoutlog(ems_u32* p)
{
    char* s;

    xdrstrcdup(&s, p+2);
    printf("set_dataoutlog: name=%s\n", s);

    if (dataout[p[1]].bufftyp==-1) return plErr_ArgRange;
    if (dataout[p[1]].addrtyp!=Addr_File) return plErr_ArgRange;

    free(dataout[p[1]].logfilename);
    if (xdrstrclen(p+2)) {
        xdrstrcdup(&dataout[p[1]].logfilename, p+2);
    } else {
        dataout[p[1]].logfilename=0;
    }

    return plOK;
}

plerrcode test_proc_set_dataoutlog(ems_u32* p)
{
    if (p[0]<1) return plErr_ArgNum;
    if ((xdrstrlen(p+2))+1!=p[0]) {
        printf("set_dataoutlog: xdrstrlen(p+2)=%d p[0]+1=%d\n",
                xdrstrlen(p+2), p[0]+1);
        return plErr_ArgNum;
    }
    if (p[1]>=MAX_DATAOUT) return plErr_ArgRange;
    wirbrauchen=0;
    return plOK;
}

char name_proc_set_dataoutlog[]="set_dataoutlog";
int ver_proc_set_dataoutlog=1;

/*****************************************************************************/
/*
 * p[0]: argnum
 * p[1]: do_ID
 */

plerrcode proc_get_dataoutlog(ems_u32* p)
{
    if (dataout[p[1]].bufftyp==-1) return plErr_ArgRange;

    if (dataout[p[1]].logfilename) {
        outptr=outstring(outptr, dataout[p[1]].logfilename);
    } else {
        outptr=outstring(outptr, "");
    }

    return plOK;
}

plerrcode test_proc_get_dataoutlog(ems_u32* p)
{
    if (p[0]!=1) return plErr_ArgNum;
    if (p[1]>=MAX_DATAOUT) return plErr_ArgRange;
    wirbrauchen=-1;
    return plOK;
}

char name_proc_get_dataoutlog[]="get_dataoutlog";
int ver_proc_get_dataoutlog=1;

/*****************************************************************************/
/*****************************************************************************/
