/*
 * procs/general/modules/IdentIS.c
 * created before 04.11.94
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: IdentIS.c,v 1.6 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
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
extern int* memberlist;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/general/modules")

/*****************************************************************************/
/* IdentIS()
  schreibt Array mit Modulindizes und -typen des aktuellen
  Instrumentierungssystemes in den Ausgabepuffer
*/
plerrcode proc_IdentIS(ems_u32* p)
{
    if (memberlist) {
        int i, anz;
        for (i=1, anz=memberlist[0]; anz; anz--, i++) {
            *outptr++=memberlist[i];
            *outptr++=modullist->entry[memberlist[i]].modultype;
        }
    } else {
        *outptr++=0;
    }

    return plOK;
}

plerrcode test_proc_IdentIS(ems_u32* p)
{
    if (*p)
        return(plErr_ArgNum);
    wirbrauchen=(memberlist?2*memberlist[0]:0)+1;
    return plOK;
}
#ifdef PROCPROPS
static procprop IdentIS_prop={1, -1, "void", 0};

procprop* prop_proc_IdentIS(void)
{
    return(&IdentIS_prop);
}
#endif
char name_proc_IdentIS[]="IdentIS";
int ver_proc_IdentIS=1;

/*****************************************************************************/
/*****************************************************************************/
