/*
 * procs/hlral/dummy/hlraldummyread.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: hlraldummyread.c,v 1.7 2011/04/06 20:30:33 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <emsctypes.h>
#include "hlraldummyread.h"
#include <rcs_ids.h>
#include "../trigger.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/hlral/dummy")

/*****************************************************************************/
void HLRAL_Dummy_total(int num_crates, int* numcards)
{
    char *out, *outstart;
    int slice;
    ems_u32* help=outptr++;
    int idx=0;

    out=(char*)outptr;
    outstart=out;
    for (slice=0; slice<64; slice++) {
        int crate;
        for (crate=0; crate<num_crates; crate++) {
            int card;
            for (card=0; card<numcards[crate]; card++) {
                int chan;
                for (chan=0; chan<16; chan++) {
                    int x=eventcnt%1024+1;
                    if (idx%x==0) *out++=0x40+chan;
                    /*if (idx==eventcnt) *out++=0x40+chan;*/
                    idx++;
                }
                *out++=0x70; /* next card/chip/row */
            }
            *out++=0x50;   /* next crate/column */
        }
        *out++=0x51;     /* next slice */
    }
    *out++=0;          /* ende */
    *help=out-outstart;
    outptr=(ems_u32*)(((unsigned long)out+3)&~3);
}
/*****************************************************************************/
/*****************************************************************************/
