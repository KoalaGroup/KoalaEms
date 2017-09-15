/*
 * server/procs/dataout/dataoutloginfo.c
 * 
 * created: 02.Aug.2004 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: dataoutloginfo.c,v 1.3 2011/04/06 20:30:31 wuestner Exp $";

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
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/dataout")

/*****************************************************************************/
/*
 * p[0]: argnum
 * p[1]: do_ID
 * p[2...]: text (xdrstring)
 */

plerrcode proc_set_dataoutloginfo(ems_u32* p)
{
    char* s;

    xdrstrcdup(&s, p+2);
    printf("set_dataoutloginfo: name=%s\n", s);

    if (dataout[p[1]].bufftyp==-1) return plErr_ArgRange;

    free(dataout[p[1]].loginfo);
    if (xdrstrclen(p+2)) {
        xdrstrcdup(&dataout[p[1]].loginfo, p+2);
    } else {
        dataout[p[1]].loginfo=0;
    }

    return plOK;
}

plerrcode test_proc_set_dataoutloginfo(ems_u32* p)
{
    if (p[0]<1) return plErr_ArgNum;
    if ((xdrstrlen(p+2))+1!=p[0]) {
        printf("set_dataoutloginfo: xdrstrlen(p+2)=%d p[0]+1=%d\n",
                xdrstrlen(p+2), p[0]+1);
        return plErr_ArgNum;
    }
    if (p[1]>=MAX_DATAOUT) return plErr_ArgRange;
    wirbrauchen=0;
    return plOK;
}

char name_proc_set_dataoutloginfo[]="set_dataoutloginfo";
int ver_proc_set_dataoutloginfo=1;

/*****************************************************************************/
/*
 * p[0]: argnum
 * p[1]: do_ID
 */

plerrcode proc_get_dataoutloginfo(ems_u32* p)
{
    if (dataout[p[1]].bufftyp==-1) return plErr_ArgRange;

    if (dataout[p[1]].loginfo) {
        outptr=outstring(outptr, dataout[p[1]].loginfo);
    } else {
        outptr=outstring(outptr, "");
    }

    return plOK;
}

plerrcode test_proc_get_dataoutloginfo(ems_u32* p)
{
    if (p[0]!=1) return plErr_ArgNum;
    if (p[1]>=MAX_DATAOUT) return plErr_ArgRange;
    wirbrauchen=-1;
    return plOK;
}

char name_proc_get_dataoutloginfo[]="get_dataoutloginfo";
int ver_proc_get_dataoutloginfo=1;

/*****************************************************************************/
/*****************************************************************************/
