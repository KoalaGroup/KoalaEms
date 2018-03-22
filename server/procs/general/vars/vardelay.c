/*
 * procs/general/vars/vardelay.c
 * created: 2016-06-29 PeWue
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: vardelay.c,v 1.3 2017/10/20 23:20:52 wuestner Exp $";

#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../objects/var/variables.h"
#include "../../procs.h"
#include "../../procprops.h"

RCS_REGISTER(cvsid, "procs/general/vars")

/*****************************************************************************/
/*
 * p[0]: # of arguments (==1)
 * p[1]: idx of variable
 * [p[2]: initial value of variable]
 *       variable will be created if it does not exist
 *       if it does not exist and p[2] is not given it is initialised with 0
 * vardelay will suspend execution by [variable] ms
 * [variable}==0 means: no delay
 */
plerrcode proc_vardelay(ems_u32* p)
{
    plerrcode pres;
    struct timeval to;
    ems_u32 val;

    pres=var_read_int(p[1], &val);
    if (pres!=plOK)
        return pres;

    to.tv_sec=val/1000;
    to.tv_usec=(val%1000)*1000;

    select(0, 0, 0, 0, &to); /* may be interrupted */

    return plOK;
}

plerrcode test_proc_vardelay(ems_u32* p)
{
    plerrcode pres;
    unsigned int size;
    int created=0;

    if (p[0]<1 && p[0]>2)
        return plErr_ArgNum;
    pres=var_attrib(p[1], &size);
    /* p[1]>=MAX_VAR */
    if (pres==plErr_IllVar)
        return pres;
    /* var exists but size is !=1 */
    if (pres==plOK && size!=1)
        return plErr_IllVarSize;

    /* at this point var either exists with size 1 or we will create it */
    if (pres==plErr_NoVar) {
        if ((pres=var_create(p[1], 1))!=plOK)
            return pres;
        created=1;
    }
    /* here var exists but may be uninitialised */
    if (created || p[0]>1) {
        pres=var_write_int(p[1], p[0]>1?p[2]:0);
        if (pres!=plOK)
            return pres;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_vardelay[]="vardelay";
int ver_proc_vardelay=1;
/*****************************************************************************/
/*****************************************************************************/
