/******************************************************************************
*                                                                             *
* lowlevel/fastbus/sfi/sfi_fbmulti.c                                          *
*                                                                             *
* created: 27.04.97                                                           *
* last changed: 23.05.97                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: sfi_fbmulti.c,v 1.3 2011/04/06 20:30:23 wuestner Exp $";

#include <errno.h>
#include <stdio.h>
#include <sconf.h>
#include <rcs_ids.h>
#include "sfilib.h"
#include "../fastbus.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sfi")

/*****************************************************************************/

static const u_int32_t mist=0x0815;

/*****************************************************************************/

u_int32_t FRCM_(sfi_info* info, u_int32_t ba)
{
SEQ_W(info, seq_prim_csrm, ba);
SEQ_W(info, seq_rndm_r_dis, mist);
return sfi_read_fifoword(info);
}

/*****************************************************************************/

u_int32_t FRDM_(sfi_info* info, u_int32_t ba)
{
SEQ_W(info, seq_prim_dsrm, ba);
SEQ_W(info, seq_rndm_r_dis, mist);
return sfi_read_fifoword(info);
}

/*****************************************************************************/

void FWCM_(sfi_info* info, u_int32_t ba,u_int32_t data)
{
SEQ_W(info, seq_prim_csrm, ba);
SEQ_W(info, seq_rndm_w_dis, data);
if (sfi_wait_sequencer(info)<0) printf("FWCM_: Fehler\n");
}

/*****************************************************************************/

void FWDM_(sfi_info* info, u_int32_t ba, u_int32_t data)
{
SEQ_W(info, seq_prim_dsrm, ba);
SEQ_W(info, seq_rndm_w_dis, data);
if (sfi_wait_sequencer(info)<0) printf("FWDM_: Fehler\n");
}

/*****************************************************************************/
/*****************************************************************************/
