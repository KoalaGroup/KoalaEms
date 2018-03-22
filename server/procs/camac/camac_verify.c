/*
 * procs/unixcamac/camac_verify.c
 * created 06.Sep.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: camac_verify.c,v 1.3 2017/10/20 23:20:52 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/devices.h"
#include "camac_verify.h"

RCS_REGISTER(cvsid, "procs/camac")

/*
 * checking whether one modulue of the memberlist has the correct type
 */
plerrcode test_proc_camac1(unsigned int* list, unsigned int idx,
        ems_u32* module_types)
{
    int res;
    ml_entry* module;
    int j;

    if (!list) return plErr_NoISModulList;
    if (list[0]<idx) return plErr_IllModAddr;

    module=&modullist->entry[list[idx]];

    if (module->modulclass!=modul_camac) {
        printf("test_proc_camac[%d]: not camac\n", idx);
        return plErr_BadModTyp;
    }
    res=-1;
    for (j=0; module_types[j] && res; j++) {
        if (module->modultype==module_types[j]) res=0;
    }
    if (res) {
        printf("test_proc_camac[%d]: type in modullist is %08x\n",
                idx, module->modultype);
        printf("looked for");
        for (j=0; module_types[j]; j++) {
            printf(" %08x", module_types[j]);
        }
        printf("\n");
        return plErr_BadModTyp;
    }
    return plOK;
}

/*
 * checking whether all modulues of IS have the correct type
 */
plerrcode test_proc_camac(unsigned int* list, ems_u32* module_types)
{
    int i, res;

    if (!list) return plErr_NoISModulList;
    for (i=list[0]; i>0; i--) {
        ml_entry* module=&modullist->entry[list[i]];
        int j;

        if (module->modulclass!=modul_camac) {
            printf("test_proc_camac[%d]: not camac\n", i);
            return plErr_BadModTyp;
        }
        res=-1;
        for (j=0; module_types[j] && res; j++) {
            if (module->modultype==module_types[j]) res=0;
        }
        if (res) {
            printf("test_proc_camac[%d]: type in modullist is %08x\n",
                    i, module->modultype);
            printf("looked for");
            for (j=0; module_types[j]; j++) {
                printf(" %08x", module_types[j]);
            }
            printf("\n");
            return plErr_BadModTyp;
        }
    }
    return plOK;
}
