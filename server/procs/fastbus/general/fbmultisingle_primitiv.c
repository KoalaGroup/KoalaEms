/*
 * procs/fastbus/general/fbmultisingle_primitiv.c
 * created 10.Jul.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fbmultisingle_primitiv.c,v 1.7 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../procprops.h"
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../../lowlevel/devices.h"

extern ems_u32* outptr;
extern int wirbrauchen;

#define get_device(class, crate) \
    (struct fastbus_dev*)get_gendevice((class), (crate))

RCS_REGISTER(cvsid, "procs/fastbus/general")

/*****************************************************************************/
/*
frc_multi
p[0] : No. of parameters
p[1] : crate
p[2] : secondary address
p[3...] : primary addresses

outptr[0] : No. of words
outptr[1...] : words read
*/

plerrcode proc_frc_multi_1(ems_u32* p)
{
        ems_u32 *help;
        int i;
        struct fastbus_dev* dev=get_device(modul_fastbus, p[1]);

        *outptr=p[0]-2;
        help=outptr++;
        for (i=3; i<=p[0]; i++) {
            ems_u32 ssr;
            int res;
            res=dev->FRC(dev, p[i], p[2], outptr, &ssr);
            outptr++;
            if (res||ssr) {
                *help=i-3;
                *outptr++=res;
                *outptr++=res?errno:ssr;
                return plErr_HW;
            }
        }
        return plOK;
}

plerrcode test_proc_frc_multi_1(ems_u32* p)
{
        plerrcode pres;
        if (p[0]<2) return plErr_ArgNum;
        if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK)
            return pres;
        wirbrauchen=p[0]+1;
        return plOK;
}

char name_proc_frc_multi_1[]="frc_multi";
int ver_proc_frc_multi_1=1;

/*****************************************************************************/
/*
frc_multi
p[0] : No. of parameters
p[1] : crate
p[2] : secondary address
p[3...] : primary addresses

outptr[0] : No. of words
outptr[1...] : words read
*/

plerrcode proc_frc_multi_2(ems_u32* p)
{
        ems_u32 ssr;
        int res;
        struct fastbus_dev* dev=get_device(modul_fastbus, p[1]);

        *outptr++=p[0]-2;

        res=dev->multiFRC(dev, p[0]-2, p+3, p[2], outptr, &ssr);
        if (res||ssr) {
                *outptr++=res;
                *outptr++=res?errno:ssr;
                return plErr_HW;
        }
        outptr+=p[0]-2;
        return plOK;
}

plerrcode test_proc_frc_multi_2(ems_u32* p)
{
        plerrcode pres;
        if (p[0]<2) return plErr_ArgNum;
        if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK)
            return pres;
        wirbrauchen=p[0]+1;
        return plOK;
}

char name_proc_frc_multi_2[]="frc_multi";
int ver_proc_frc_multi_2=2;

/*****************************************************************************/
/*****************************************************************************/
