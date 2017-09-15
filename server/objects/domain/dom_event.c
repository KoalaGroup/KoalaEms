/*
 * dom_event.c
 * created: 1993-Jan-10
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: dom_event.c,v 1.10 2011/04/06 20:30:29 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <objecttypes.h>
#include <rcs_ids.h>
#include "dom_event.h"
#include "domevobj.h"
#include <../pi/readout.h>
#include "../../dataout/dataout.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "objects/domain")

/*****************************************************************************/
errcode
dom_event_init()
{
    T(dom_event_init)
    return OK;
}
/*****************************************************************************/
errcode
dom_event_done()
{
    T(dom_event_done)
    return OK;
}
/*****************************************************************************/
static objectcommon*
lookup_dom_ev(ems_u32* id, unsigned int idlen, unsigned int* remlen)
{
    T(lookup_dom_ev)
    if (idlen>0) {
        if ((id[0]!=0)||(readout_active==Invoc_notactive))
            return 0;
        *remlen=idlen;
        return (objectcommon*)&dom_ev_obj;
    } else {
        *remlen=0;
        return (objectcommon*)&dom_ev_obj;
    }
}
/*****************************************************************************/
static ems_u32*
dir_dom_ev(ems_u32* ptr)
{
    T(dir_dom_ev)
    if(readout_active)
        *ptr++=0;
    return ptr;
}
/*****************************************************************************/
domevobj dom_ev_obj={
    {
        0,0,
        /*(lookupfunc)*/lookup_dom_ev,
        dir_dom_ev
    },
    notimpl,  /* download */
    get_last_databuf,
    notimpl  /* delete */
};
/*****************************************************************************/
/*****************************************************************************/
