/*
 * lowlevel/lvd/datafilter/filter_qdc_sparse.c
 * created 2010-Feb-11 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: filter_qdc_sparse.c,v 1.2 2011/04/06 20:30:25 wuestner Exp $";

#include <sconf.h>
#include <stdlib.h>
#include <string.h>
#include <rcs_ids.h>
#include "datafilter.h"

/*
 * qdc_sparse deletes some unwanted data words fron the data stream
 * filter->givenargs:
 *     [0] version of procedure
 *     [1] mask of the affected module addresses
 */

RCS_REGISTER(cvsid, "lowlevel/lvd/datafilter")

/*****************************************************************************/
static plerrcode
filter_qdc_sparse_filter_0(struct datafilter *filter, ems_u32 *data, int *len)
{
    int seq[256];
    int l=*len;
    int n, i, addr, chan, code;
    ems_u32 d;

    memset(seq, 0, sizeof(seq));

    /*
     * data[0..2] are header words, left untouched
     */
    for (i=3, n=3; i<l; i++) {
        d=data[i];
        addr=(d>>28)&0xf0;

        if (!((1<<addr)&filter->givenargs[0])) {
            data[n++]=d;
            continue;
        }

        chan=(addr<<4) | ((d>>22)&0xf);
        code=(d>>16)&0x3f;
        if (!(code&0x20) /* raw data */
            || (code&0x30)==0x20) { /* Q */
            data[n++]=d;
            continue;
        }

        switch (code) {
        case 0x37:
            if (seq[chan]++==0) /* mean level */
                data[n++]=d;
            /*
             * all other words with 0x37 are deleted
             */
            break;
        case 0x32: /* minimum time */
        case 0x33: /* minimum level */
        case 0x34: /* maximum time */
            break;
        default:
            data[n++]=d;
        }
    }

    *len=n;

    return plOK;
}
/*****************************************************************************/
static plerrcode
filter_qdc_sparse_filter_1(struct datafilter *filter, ems_u32 *data, int *len)
{
    struct chaninfo {
        ems_u32 dat37[4];
        int seq37;
        int q_found;
    };
    struct chaninfo info[256];
    int l=*len;
    int n, i, addr, chan, code;
    ems_u32 d;

    memset(info, 0, sizeof(info));

    /*
     * data[0..2] are header words, left untouched
     */
    for (i=3, n=3; i<l; i++) {
        d=data[i];
        addr=(d>>28)&0xf0;

        if (!((1<<addr)&filter->givenargs[0])) { /* address not selected */
            data[n++]=d;
            continue;
        }

        chan=(addr<<4) | ((d>>22)&0xf);
        code=(d>>16)&0x3f;

        if (!(code&0x20)) { /* raw data */
            data[n++]=d;
            continue;
        }

        if ((code&0x30)==0x20) {       /* Q */
            if (info[chan].seq37==2) { /* fixed q */
                info[chan].dat37[2]=d;
                info[chan].seq37++;
            } else {                   /* pulse q*/
                info[chan].q_found=1;
                data[n++]=d;
            }
            continue;
        }

        switch (code) {
        case 0x37:
            if (info[chan].seq37<4) {
                info[chan].dat37[info[chan].seq37]=d;
            } else { /* data corrupt */
                if (info[chan].seq37==4) {
                    data[n++]=info[chan].dat37[0];
                    data[n++]=info[chan].dat37[1];
                    data[n++]=info[chan].dat37[2];
                    data[n++]=info[chan].dat37[3];
                }
                data[n++]=d;
            }
            info[chan].seq37++;
            break;
        case 0x32: /* minimum time */
        case 0x33: /* minimum level */
        case 0x34: /* maximum time */
            break;
        default:
            data[n++]=d;
        }
    }

    /* check whether we need the fixed q */
    for (chan=0; i<256; i++) {
        if (info[chan].seq37==4) {
            data[n++]=info[chan].dat37[0]; /* mean level */
            if (!info[chan].q_found)
                data[n++]=info[chan].dat37[2]; /* fixed q */
        }
    }

    *len=n;

    return plOK;
}
/*****************************************************************************/
static plerrcode
filter_qdc_sparse_filter_2(struct datafilter *filter, ems_u32 *data, int *len)
{
    return plOK;
}
/*****************************************************************************/
plerrcode (*f[])(struct datafilter*, ems_u32*, int*)={
    filter_qdc_sparse_filter_0,
    filter_qdc_sparse_filter_1,
    filter_qdc_sparse_filter_2,
};
/*****************************************************************************/
plerrcode
filter_qdc_sparse_init(struct datafilter *filter)
{
    if (filter->num_givenargs!=2)
        return plErr_ArgNum;
    if (filter->givenargs[0]>=
            sizeof(f)/sizeof(plerrcode(*)(struct datafilter*, ems_u32*, int*)))
        return plErr_ArgRange;

    return plOK;
}
/*****************************************************************************/
plerrcode
filter_qdc_sparse_filter(struct datafilter *filter, ems_u32 *data, int *len)
{
    return f[filter->givenargs[0]](filter, data, len);
}
/*****************************************************************************/
/*****************************************************************************/
