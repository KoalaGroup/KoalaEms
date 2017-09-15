/*
 * server/procs/general/request.c
 * created: 2009-08-05 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: request.c,v 1.2 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../main/requests.h"

extern ems_u32* outptr;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/general")

/*****************************************************************************/
/*
 * p[0]: argcount
 * p[1..n]: name of the EMS request
 * p[n+1..]: arguments
 * 
 * returns:
 * outptr[0]: errorcode from EMS request
 * outptr[1]: number of result words
 * outptr[2...]: original result of EMS request
 */
plerrcode proc_request(ems_u32* p)
{
    char *name;
    ems_u32 *help;
    int idx, slen;
    errcode res;

    slen=xdrstrlen(p+1);
    xdrstrcdup(&name, p+1);
    idx=0;
    while (RequestName[idx] && strcmp(name, RequestName[idx]))
        idx++;
    free(name);
    if (!RequestName[idx])
        return plErr_NotImpl;

    help=outptr;
    outptr+=2;
    res=DoRequest[idx](p+1+slen, p[0]-slen);
    help[0]=(ems_u32)res;
    help[1]=outptr-help-2;

    return plOK;
}

plerrcode test_proc_request(ems_u32* p)
{
    if (p[0]<1)
        return plErr_ArgNum;
    if (p[0]<1+xdrstrlen(p+1))
        return plErr_ArgNum;
    wirbrauchen=-1;
    return plOK;
}

char name_proc_request[]="request";
int ver_proc_request=1;

/*****************************************************************************/
/*****************************************************************************/
