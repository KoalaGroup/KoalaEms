/*
 * procs/general/modules/ModuleParams.c
 * created: 10.Aug.2003
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: ModuleParams.c,v 1.8 2017/10/20 23:20:52 wuestner Exp $";

#include <sconf.h>
#include <stdlib.h>
#include <rcs_ids.h>
#include "../../../objects/is/is.h"
#include "../../../objects/domain/dom_ml.h"
#include <errorcodes.h>
#include "../../procprops.h"
#include "../../procs.h"

#ifndef DOM_ML
#error DOM_ML must be defined
#endif

extern ems_u32* outptr;
extern unsigned int* memberlist;

RCS_REGISTER(cvsid, "procs/general/modules")

/*****************************************************************************/
static int
module_exists(unsigned idx)
{
    if (memberlist) {
        if (idx==0 || idx>memberlist[0]) return 0;
        idx=memberlist[idx];
    }
    if (idx>=modullist->modnum) return 0;

    return 1;
}
/*****************************************************************************/
char name_proc_SetModuleParams[]="SetModuleParams";
int ver_proc_SetModuleParams=1;
/*
 * p[0]: argcount
 * p[1]: module idx (either in modullist or in memberlist)
 * p[2..]: params to be stored
 */
plerrcode proc_SetModuleParams(ems_u32* p)
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

plerrcode test_proc_SetModuleParams(ems_u32* p)
{
    if (p[0]<1) return plErr_ArgNum;
    if (!module_exists(p[1])) return plErr_ArgRange;
    wirbrauchen=0;
    return plOK;
}
/*****************************************************************************/
char name_proc_GetModuleParams[]="GetModuleParams";
int ver_proc_GetModuleParams=1;
/*
 * p[0]: argcount==1
 * p[1]: module idx (either in modullist or in memberlist)
 */
plerrcode proc_GetModuleParams(ems_u32* p)
{
    struct ml_entry* module=ModulEnt(p[1]);
    int i;

    *outptr++=module->property_length;
    for (i=0; i<module->property_length; i++) {
        *outptr++=module->property_data[i];
    }

    return plOK;
}

plerrcode test_proc_GetModuleParams(ems_u32* p)
{
    if (p[0]!=1) return plErr_ArgNum;
    if (!module_exists(p[1])) return plErr_ArgRange;
    wirbrauchen=ModulEnt(p[1])->property_length;
    return plOK;
}
/*****************************************************************************/
/*****************************************************************************/
