/*
 * procs/fastbus/fastbus_verify.c
 * created 06.Sep.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fastbus_verify.c,v 1.4 2011/04/06 20:30:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/fastbus/fastbus.h"
#include "../../lowlevel/devices.h"
#include "fastbus_verify.h"

RCS_REGISTER(cvsid, "procs/fastbus")

plerrcode
verify_fastbus_id(struct fastbus_dev* dev, ems_u32 pa, int modultype)
{
/*
 * return:
 *     plErr_HWTestFailed: not verified but not fatal
 *     plErr_HW: fatal error (crate offline ...)
 *     plOK: verified
 */

    ems_u32 csr0, ss;
    int res;

    res=dev->FRC(dev, pa, 0, &csr0, &ss);
    if (res) {
        return plErr_HW;
    }
    if (ss) {
        return plErr_HWTestFailed;
    }
    if (((csr0>>16)&0xffff)==(modultype&0xffff)) {
        return plOK;
    }
    return plErr_HWTestFailed;
}

plerrcode
verify_fastbus_module(ml_entry* module)
{
    return verify_fastbus_id(module->address.fastbus.dev,
            module->address.fastbus.pa,
            module->modultype);
}

/*
 * checking whether all modulues of IS have the correct type
 */
plerrcode
test_proc_fastbus(int* list, ems_u32* module_types)
{
    int i, res;

    if (!list) return plErr_NoISModulList;
    for (i=list[0]; i>0; i--) {
        ml_entry* module=&modullist->entry[list[i]];
        int j;

        if (module->modulclass!=modul_fastbus) {
            printf("test_proc_fastbus[%d]: not fastbus\n", i);
            return plErr_BadModTyp;
        }
        res=-1;
        for (j=0; module_types[j] && res; j++) {
            if (module->modultype==module_types[j]) res=0;
        }
        if (res) {
            printf("test_proc_fastbus[%d]: type in modullist is %08x\n",
                    i, module->modultype);
            printf("looked for");
            for (j=0; module_types[j]; j++) {
                printf(" %08x", module_types[j]);
            }
            printf("\n");
            return plErr_BadModTyp;
        }
        return verify_fastbus_module(module);
    }
    return plOK;
}
