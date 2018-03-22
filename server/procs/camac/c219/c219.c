/*
 * procs/camac/c219/c219.c
 * created 20.Sep.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: c219.c,v 1.13 2017/10/20 23:20:52 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../lowlevel/camac/camac.h"
#include "../../../objects/var/variables.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../procprops.h"

RCS_REGISTER(cvsid, "procs/camac/c219")

#define BITS 16
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module
 * p[2]: var1; size==BITS   ; not used if var1==0
 * p[3]: var2; size==2**BITS; not used if var2==0
 * p[4]: var3; size==1      ; not used if var3==0
 */
plerrcode proc_c219statist(ems_u32* p)
{
    ml_entry* m=ModulEnt(p[1]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), 0, 0);
    ems_u32 val;
    int i, res;

    res=dev->CAMACread(dev, &addr, &val);
    if (res) {
        printf("c219statist: CAMACread slot %d failed\n", CAMACslot_e(m));
        return plErr_System;
    }
    val&=((1<<BITS)-1);
    *outptr++=val;

    if (p[4]) {
        var_list[p[4]].var.val++;
    }
    if (p[3]) {
        var_list[p[3]].var.ptr[val]++;
    }
    if (p[2]) {
        for (i=0; val; i++, val>>=1)
            if (val&1) var_list[p[2]].var.ptr[i]++;
    }

    return(plOK);
}

plerrcode test_proc_c219statist(ems_u32* p)
{
    if (p[0]!=4) return plErr_ArgNum;
    if ((unsigned int)p[1]>25) return plErr_ArgRange;

    if (p[2]) {
        if ((unsigned int)p[2]>MAX_VAR) return plErr_IllVar;
        if (!var_list[p[2]].len) return plErr_NoVar;
        if (var_list[p[2]].len!=BITS) return plErr_IllVarSize;
    }

    if (p[3]) {
        if ((unsigned int)p[3]>MAX_VAR) return plErr_IllVar;
        if (!var_list[p[3]].len) return plErr_NoVar;
        if (var_list[p[3]].len!=(1<<BITS)) return plErr_IllVarSize;
    }

    if (p[4]) {
        if ((unsigned int)p[4]>MAX_VAR) return plErr_IllVar;
        if (!var_list[p[4]].len) return plErr_NoVar;
        if (var_list[p[4]].len!=1) return plErr_IllVarSize;
    }

    wirbrauchen=1;
    return plOK;
}

char name_proc_c219statist[]="C219statist";
int ver_proc_c219statist=1;

/*****************************************************************************/
/*****************************************************************************/
