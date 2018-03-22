/*
 * procs/general/eventparser/proc_eventparse.c
 * created: 2007-Feb-11 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: proc_eventparse.c,v 1.4 2017/10/09 21:25:37 wuestner Exp $";

#include <sconf.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procprops.h"
#include "../../procs.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/general/eventparser")

/*****************************************************************************/
/*
 * p[0]: argcount
 * p[1]: is_id
 * p[2]: name
 */
plerrcode
proc_set_parseproc(ems_u32* p)
{
    struct ml_entry* module=ModulEnt(p[1]);
    int plen=p[0]-1, i;
    p+=2;

    if (module->property_data) free(module->property_data);
    module->property_length=0;

    module->property_data=malloc(sizeof( unsigned int)*plen);
    if (!module->property_data) return plErr_NoMem;

    for (i=0; i<plen; i++) {
        module->property_data[i]=*p++;
    }
    module->property_length=plen;

    return plOK;
}

plerrcode test_proc_set_parseproc(ems_u32* p)
{
    if (p[0]<2)
        return plErr_ArgNum;
    if (xdrstrlen(p[2])>p[0]+1)
        return plErr_ArgNum;
    wirbrauchen=0;
    return plOK;
}
char name_proc_set_parseproc[]="set_parseproc";
int ver_proc_set_parseproc=1;
/*****************************************************************************/
/*
 * p[0]: argcount
 * p[1]: is_id (-1: all IS)
 */
plerrcode
proc_clear_parseproc(ems_u32* p)
{
    struct ml_entry* module=ModulEnt(p[1]);
    int plen=p[0]-1, i;
    p+=2;

    if (module->property_data) free(module->property_data);
    module->property_length=0;

    module->property_data=malloc(sizeof( unsigned int)*plen);
    if (!module->property_data) return plErr_NoMem;

    for (i=0; i<plen; i++) {
        module->property_data[i]=*p++;
    }
    module->property_length=plen;

    return plOK;
}

plerrcode test_proc_clear_parseproc(ems_u32* p)
{
    if (p[0]<2)
        return plErr_ArgNum;
    if (xdrstrlen(p[2])>p[0]+1)
        return plErr_ArgNum;
    wirbrauchen=0;
    return plOK;
}
char name_proc_clear_parseproc[]="clear_parseproc";
int ver_proc_clear_parseproc=1;
/*****************************************************************************/
/*****************************************************************************/
