/*
 * server/procs/dataout/dataoutlinkdir.c
 * 
 * created: 2006-Nov-18 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: dataoutlinkdir.c,v 1.7 2017/10/09 21:25:37 wuestner Exp $";

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
 * p[2...]: dataoutlinkdir
 * pp[0]: 1: absolute path 0: relative path
 */

plerrcode proc_set_dataoutlinkdir(ems_u32* p)
{
    ems_u32 *pp;

    pp=p+2+xdrstrlen(p+2);

    if (dataout[p[1]].bufftyp==-1)
        return plErr_ArgRange;
    if (dataout[p[1]].addrtyp!=Addr_File)
        return plErr_ArgRange;

    free(dataout[p[1]].runlinkdir);
    if (xdrstrclen(p+2)) {
        pp=xdrstrcdup(&dataout[p[1]].runlinkdir, p+2);
    } else {
        dataout[p[1]].runlinkdir=0;
    }
    if (p+p[0]<pp)
        dataout[p[1]].runlinkabsolute=0;
    else
        dataout[p[1]].runlinkabsolute=pp[0];

    return plOK;
}

plerrcode test_proc_set_dataoutlinkdir(ems_u32* p)
{
    if (p[0]<2)
        return plErr_ArgNum;
    if ((xdrstrlen(p+2))+1>p[0]) {
        printf("set_dataoutlinkdir: xdrstrlen(p+2)=%d p[0]+2=%d\n",
                xdrstrlen(p+2), p[0]+2);
        return plErr_ArgNum;
    }
    if (p[1]>=MAX_DATAOUT)
        return plErr_ArgRange;
    wirbrauchen=0;
    return plOK;
}

char name_proc_set_dataoutlinkdir[]="set_dataoutlinkdir";
int ver_proc_set_dataoutlinkdir=1;

/*****************************************************************************/
/*
 * p[0]: argnum
 * p[1]: do_ID
 */

plerrcode proc_get_dataoutlinkdir(ems_u32* p)
{
    if (dataout[p[1]].bufftyp==-1)
        return plErr_ArgRange;

    *outptr++=dataout[p[1]].runlinkabsolute;
    if (dataout[p[1]].runlinkdir) {
        outptr=outstring(outptr, dataout[p[1]].runlinkdir);
    } else {
        outptr=outstring(outptr, "");
    }

    return plOK;
}

plerrcode test_proc_get_dataoutlinkdir(ems_u32* p)
{
    if (p[0]!=1)
        return plErr_ArgNum;
    if (p[1]>=MAX_DATAOUT)
        return plErr_ArgRange;
    wirbrauchen=-1;
    return plOK;
}

char name_proc_get_dataoutlinkdir[]="get_dataoutlinkdir";
int ver_proc_get_dataoutlinkdir=1;

/*****************************************************************************/
/*****************************************************************************/
