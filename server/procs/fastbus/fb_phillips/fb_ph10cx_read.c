/*
 * procs/fastbus/fb_phillips/fb_ph10Cx_init.c
 * created 04.Feb.2001 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fb_ph10cx_read.c,v 1.16 2011/04/06 20:30:31 wuestner Exp $";

#include <config.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../../objects/var/variables.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../objects/pi/readout.h"
#include "../../../trigger/trigger.h"
#include "../../procs.h"
#include "../../procprops.h"

#define MAX_WCs  18                   /* max number of words for single event */
#define MAX_WCm  1024                 /* max number of words for multi event */

extern ems_u32* outptr;
extern int *memberlist;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/fastbus/fb_phillips")

/******************************************************************************/

char name_proc_fb_ph10cx_read[]="fb_ph10Cx_read";
int ver_proc_fb_ph10cx_read=1;

/******************************************************************************/
/*
read of Phillips 10Cx
========================

parameters:
-----------
  p[ 0]:                 3 (number of parameters)
  p[ 1]:                 primary link; index in modullist
  p[ 2]:                 multi event mode
  p[ 3]:                 number of modules

*/
/******************************************************************************/
plerrcode proc_fb_ph10cx_read(ems_u32* p)
{
    int wc;                               /* word count from block read      */
    int mwc;                              /* max. word count                 */
    ems_u32 *ptr;
    int pa;
    ems_u32 ssr;
    ml_entry* module;
    struct fastbus_dev* dev;

    T(proc_fb_ph10Cx_read)

    module=modullist->entry+p[1];
    dev=module->address.fastbus.dev;
    pa=module->address.fastbus.pa;

    ptr=outptr++;
    mwc=p[3]*(p[2]?MAX_WCm:MAX_WCs)+1;

/*
Thomas S. ist der festen Ueberzeugung, dass diese dammichten 10C6 immer das
Wort beim Blocktransfer vergessen (welches?). Er hat vermutlich recht.
Also bitte nicht wundern, wenn die Daten seltsam aussehen.
*/

    wc=dev->FRDB(dev, pa, 2, mwc, outptr, &ssr, "proc_fb_ph10cx_read");
    if ((wc<0) || (ssr!=2 && ssr!=0)) {
        printf("ph10Cx_read: FRDB(pa=%d sa=%d count=%d): wc=%d ssr=%d\n",
            pa, 2, mwc, wc, ssr);
        if (wc<0) {
            *outptr++=-1;
            *outptr++=4;
            return plErr_HW;
        } else {
            send_unsol_warning(10, 3, trigger.eventcnt, pa, ssr);
            invalidate_event();
            return plOK;
        }
    }
    wc--;                                 /* last word is dummy              */
    *ptr=wc;
    outptr+=wc;

    return plOK;
}
/******************************************************************************/
plerrcode test_proc_fb_ph10cx_read(ems_u32* p)
{
    ml_entry* module;
    T(test_proc_fb_ph10Cx_read)

    if (p[0]!=3) return plErr_ArgNum;
/*
    printf("test_proc_fb_ph10cx_read: p1=%d, p2=%d, p3=%d\n", p[1], p[2], p[3]);
*/
    if ((unsigned int)p[1]>modullist->modnum) {
        *outptr++=p[1];
        *outptr++=1;
        return plErr_ArgRange;
    }

    module=modullist->entry+p[1];
    if ((module->modultype!=PH_ADC_10C2) && (module->modultype!=PH_TDC_10C6)) {
        *outptr++=module->modultype;
        *outptr++=2;
        return plErr_BadModTyp;
    }

    wirbrauchen=p[3]*p[2]?MAX_WCm:MAX_WCs+1;
#if PERFSPECT
    dev->FRDB_S_perfspect_needs(dev, &perfbedarf, perfnames);
#endif
    return plOK;
}

/******************************************************************************/
/******************************************************************************/
