/*
 * procs/fastbus/general/frdb_multi.c
 * 
 * created 07.Nov.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: frdb_multi.c,v 1.7 2017/10/09 21:25:37 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/fastbus/fastbus.h"

extern ems_u32* outptr;
extern int *memberlist;

RCS_REGISTER(cvsid, "procs/fastbus/general")

/*****************************************************************************/
/*
 * p[0] : No. of parameters (>2)
 * p[1] : secondary address (-1 if not used)
 * p[2] : max. count (pro module)
 * p[3...] : modulindices
 */
plerrcode proc_FRDB_multi_init(ems_u32* p)
{
    ems_u32 *pa;
    int i, res;
    struct fastbus_dev* dev;

    dev=ModulEnt(p[3])->address.fastbus.dev;
    pa=malloc((p[0]-2)*sizeof(ems_u32));
    for (i=3; i<=p[0]; i++) {
        pa[i-3]=ModulEnt(p[i])->address.fastbus.pa;
    }
    res=dev->FRDB_multi_init(dev, 0, p[0]-2, pa, p[1], p[2]);
    free(pa);

    if (res) {
        printf("proc_FRDB_multi_init: multi_init failed\n");
    }
    return res?plErr_System:plOK;
}
/*---------------------------------------------------------------------------*/
plerrcode test_proc_FRDB_multi_init(ems_u32* p)
{
    struct fastbus_dev* dev;
    int i;

    if (p[0]<3) return plErr_ArgNum;
    for (i=3; i<=p[0]; i++) {
        ml_entry* module=ModulEnt(p[i]);
        if (module->modulclass!=modul_fastbus) {
            *outptr++=1; *outptr++=i;
            return plErr_BadModTyp;
        }
    }
    dev=ModulEnt(p[3])->address.fastbus.dev;
    for (i=4; i<p[0]; i++) {
        ml_entry* module=ModulEnt(p[i]);
        if (module->address.fastbus.dev!=dev) {
            *outptr++=2; *outptr++=i;
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_FRDB_multi_init[]="FRDB_multi_init";
int ver_proc_FRDB_multi_init=1;
/*****************************************************************************/
/*
 * p[0] : No. of parameters (==1)
 * p[1] : one arbitrary modul of the FASTBUS crate (to define 'dev')
 * p[2] : max. count; only needed for 'wirbrauchen'
 */
plerrcode proc_FRDB_multi_read(ems_u32* p)
{
    struct fastbus_dev* dev;
    int res, count;

    dev=modullist->entry[memberlist[1]].address.fastbus.dev;
    res=dev->FRDB_multi_read(dev, 0, outptr+1, &count);
    if (res<0) {
        printf("proc_FRDB_multi_read failed; res=%d\n", res);
        return plErr_HW;
    }
    
    *outptr++=count;
    outptr+=count;
    return plOK;
}
/*---------------------------------------------------------------------------*/
plerrcode test_proc_FRDB_multi_read(ems_u32* p)
{
    if (p[0]!=2) return plErr_ArgNum;
    if (!valid_module(p[1], modul_fastbus)) return plErr_BadModTyp;
    wirbrauchen=p[2];
    return plOK;
}

char name_proc_FRDB_multi_read[]="FRDB_multi_read";
int ver_proc_FRDB_multi_read=1;
/*****************************************************************************/
/*****************************************************************************/
