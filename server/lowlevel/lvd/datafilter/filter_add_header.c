/*
 * lowlevel/lvd/datafilter/filter_add_header.c
 * created 2010-Feb-11 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: filter_add_header.c,v 1.4 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <stdlib.h>
#include <string.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../../objects/pi/readout.h"
#include "../lvd_map.h"
#include "datafilter.h"

/*
 * all words in filter->givenargs are added to the output data just after
 * the LVDS header
 */

RCS_REGISTER(cvsid, "lowlevel/lvd/datafilter")

/*****************************************************************************/
plerrcode
filter_add_header_init(__attribute__((unused)) struct datafilter *filter)
{
    return plOK;
}
/*****************************************************************************/
/*
 * Filter can modify data and data size.
 * After adding data *num has to be increased and *maxnum decreased.
 * After removing data *num has to be decreased and *maxnum
 * increased.
 * The event header has to be corrected too.
 * It is not allowed to remove the event header.
 * Of course *num must not go below 3 (event header) and *maxnum
 * must not become negative.
 * The length of the modified event must fit into LVD_FIFO_MASK (18 bit)
 */
plerrcode
filter_add_header_filter(struct datafilter *filter, ems_u32 *data,
        int *num, int *maxnum)
{
    int i;

    /* we want to insert filter->num_givenargs words */
    if (*num+filter->num_givenargs>*maxnum) {
        complain("filter_add_header: data do no fit into buffer");
        return plErr_NoMem;
    }
    if ((*num+filter->num_givenargs)*4>=LVD_FIFO_SIZE) {
        complain("filter_add_header: data do no fit into LVDS event");
        return plErr_Overflow;
    }

    data+=3; /* leave original header untouched */
    memmove(data+3+filter->num_givenargs, data+3, (*num-3)*4);
    for (i=0; i<filter->num_givenargs; i++)
        data[3+i]=filter->givenargs[i];

    *num+=filter->num_givenargs;
    *maxnum-=filter->num_givenargs;
    data[0]=(data[0]&!LVD_FIFO_MASK)|*num*4;

    return plOK;
}
/*****************************************************************************/
/*****************************************************************************/
