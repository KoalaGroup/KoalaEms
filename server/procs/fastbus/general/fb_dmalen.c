/*
 * procs/fastbus/general/fb_dmalen.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fb_dmalen.c,v 1.3 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
#include <stdlib.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../procs.h"

extern ems_u32* outptr;
extern int wirbrauchen;

#define get_fbdevice(crate) \
    (struct fastbus_dev*)get_gendevice(modul_fastbus, (crate))

RCS_REGISTER(cvsid, "procs/fastbus/general")

/*****************************************************************************/
/*
p[0] : No. of parameters (==3)
p[1] : crate
p[2] : subsystem    (0: vme 1: mem)
p[3] : read length  (-1: no change 0: no dma 1: always dma >1: dma if ...)
p[4] : write length
outptr[0] : old read length
outptr[1] : old write length
*/

plerrcode proc_fb_dmalen(ems_u32* p)
{
    struct fastbus_dev* dev=get_fbdevice(p[1]);
    int res, rlen=p[3], wlen=p[4];

    res=dev->dmalen(dev, p[2], &rlen, &wlen);
    if (res==0) {
        *outptr++=rlen;
        *outptr++=wlen;
        return plOK;
    } else {
        return plErr_System;
    }
}

plerrcode test_proc_fb_dmalen(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=2;
    return plOK;
}

char name_proc_fb_dmalen[]="fb_dmalen";
int ver_proc_fb_dmalen=1;
/*****************************************************************************/
/*****************************************************************************/
