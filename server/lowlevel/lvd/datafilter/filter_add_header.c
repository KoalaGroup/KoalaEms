/*
 * lowlevel/lvd/datafilter/filter_add_header.c
 * created 2010-Feb-11 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: filter_add_header.c,v 1.2 2011/04/06 20:30:25 wuestner Exp $";

#include <sconf.h>
#include <stdlib.h>
#include <string.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../../objects/pi/readout.h"
#include "datafilter.h"

/*
 * all words in filter->givenargs are added to the output data just after
 * the LVDS header
 */

RCS_REGISTER(cvsid, "lowlevel/lvd/datafilter")

/*****************************************************************************/
plerrcode
filter_add_header_init(struct datafilter *filter)
{
    return plOK;
}
/*****************************************************************************/
plerrcode
filter_add_header_filter(struct datafilter *filter, ems_u32 *data, int *len)
{
    int i;
    if (*len+filter->num_givenargs>event_max) {
        complain("filter_add_header: too many data");
        return plErr_NoMem;
    }
    data+=3; /* leave original header untouched */
    memmove(data+filter->num_givenargs, data, 4*filter->num_givenargs);
    for (i=0; i<filter->num_givenargs; i++)
        data[i]=filter->givenargs[i];
    (*len)+=filter->num_givenargs;

    return plOK;
}
/*****************************************************************************/
/*****************************************************************************/
