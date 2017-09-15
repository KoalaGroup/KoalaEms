/*
 * procs/general/modules/dump_mlist.c
 * created: 2011-05-01 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: dump_mlist.c,v 1.1 2011/08/13 20:37:28 wuestner Exp $";

#include <sconf.h>
#include <rcs_ids.h>
#include "../../../objects/domain/dom_ml.h"
#include <errorcodes.h>
#include "../../procs.h"

extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/general/modules")

/*****************************************************************************/
/*
 * p[0]: # args 0|1
 * [p[1]: idx (0: old verrsion, 1: new version (default))]
 */
plerrcode proc_dump_modlist(ems_u32* p)
{
    return dump_modulelist();
}

plerrcode test_proc_dump_modlist(ems_u32* p)
{
    if (p[0]!=0 && p[0]!=1)
        return plErr_ArgNum;
    wirbrauchen=0;
    return plOK;
}

char name_proc_dump_modlist[]="dump_modullist";
int ver_proc_dump_modlist=1;
/*****************************************************************************/
/*
 * p[0]: # args ==0
 */
plerrcode proc_dump_memblist(ems_u32* p)
{
    return dump_memberlist();
}

plerrcode test_proc_dump_memblist(ems_u32* p)
{
    if (p[0])
        return plErr_ArgNum;
    wirbrauchen=0;
    return plOK;
}

char name_proc_dump_memblist[]="dump_memberlist";
int ver_proc_dump_memblist=1;
/*****************************************************************************/
/*****************************************************************************/
