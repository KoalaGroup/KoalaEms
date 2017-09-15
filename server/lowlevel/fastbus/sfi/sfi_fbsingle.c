/*
 * lowlevel/fastbus/sfi/sfi_fbsingle.c
 * 
 * created: 27.04.97
 * last changed: 23.05.97
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sfi_fbsingle.c,v 1.5 2011/04/06 20:30:23 wuestner Exp $";

#include <errno.h>
#include <sconf.h>
#include <rcs_ids.h>
#include "sfilib.h"
#include "../fastbus.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sfi")

/*****************************************************************************/

static const u_int32_t mist=0x0815;

/*****************************************************************************/
#ifdef SFIMAPPED
u_int32_t FRC_(sfi_info* info, u_int32_t pa, u_int32_t sa)
{
u_int32_t res;
SEQ_W(info, seq_prim_csr, pa);
SEQ_W(info, seq_secad_w, sa);
SEQ_W(info, seq_rndm_r_dis, mist);
res=sfi_read_fifoword(info);
return res;
}
#else
u_int32_t FRC_(sfi_info* info, u_int32_t pa, u_int32_t sa)
{
sfi_command comm[3];
comm[0].cmd=seq_prim_csr;      comm[0].data=pa;
comm[1].cmd=seq_secad_w;       comm[1].data=sa;
comm[2].cmd=seq_rndm_r_dis;    comm[2].data=0;
if (write(info->path, comm, sizeof(comm))!=sizeof(comm))
  {
  perror("FRC_: write sfi");
  info->status.error=sfi_error_unknown;
  return -1;
  }
return sfi_read_fifoword(info);
}
#endif
/*****************************************************************************/
#ifdef SFIMAPPED
u_int32_t FRCa_(sfi_info* info, u_int32_t pa, u_int32_t sa)
{
SEQ_W(info, seq_prim_csr, pa);
SEQ_W(info, seq_secad_w, sa);
SEQ_W(info, seq_rndm_r, mist);
return sfi_read_fifoword(info);
}
#else
#error FRCa_ not implemented
#endif
/*****************************************************************************/
#ifdef SFIMAPPED
u_int32_t FRD_(sfi_info* info, u_int32_t pa, u_int32_t sa)
{
SEQ_W(info, seq_prim_dsr, pa);
SEQ_W(info, seq_secad_w, sa);
SEQ_W(info, seq_rndm_r_dis, mist);
return sfi_read_fifoword(info);
}
#else
u_int32_t FRD_(sfi_info* info, u_int32_t pa, u_int32_t sa)
{
sfi_command comm[3];
comm[0].cmd=seq_prim_dsr;      comm[0].data=pa;
comm[1].cmd=seq_secad_w;       comm[1].data=sa;
comm[2].cmd=seq_rndm_r_dis;    comm[2].data=0;
if (write(info->path, comm, sizeof(comm))!=sizeof(comm))
  {
  perror("FRD_: write sfi");
  info->status.error=sfi_error_unknown;
  return -1;
  }
return sfi_read_fifoword(info);
}
#endif
/*****************************************************************************/
#ifdef SFIMAPPED
u_int32_t FRDa_(sfi_info* info, u_int32_t pa, u_int32_t sa)
{
SEQ_W(info, seq_prim_dsr, pa);
SEQ_W(info, seq_secad_w, sa);
SEQ_W(info, seq_rndm_r, mist);
return sfi_read_fifoword(info);
}
#else
#error FRDa_ not implemented
#endif
/*****************************************************************************/
#ifdef SFIMAPPED
void FWC_(sfi_info* info, u_int32_t pa, u_int32_t sa, u_int32_t data)
{
SEQ_W(info, seq_prim_csr, pa);
SEQ_W(info, seq_secad_w, sa);
SEQ_W(info, seq_rndm_w_dis, data);
/* if (sfi_wait_sequencer(info)<0) printf("FWC_: Fehler\n"); */
sfi_wait_sequencer(info);
}
#else
void FWC_(sfi_info* info, u_int32_t pa, u_int32_t sa, u_int32_t data)
{
sfi_command comm[3];
comm[0].cmd=seq_prim_csr;      comm[0].data=pa;
comm[1].cmd=seq_secad_w;       comm[1].data=sa;
comm[2].cmd=seq_rndm_w_dis;    comm[2].data=data;
if (write(info->path, comm, sizeof(comm))!=sizeof(comm))
  {
  perror("FWC_: write sfi");
  info->status.error=sfi_error_unknown;
  return -1;
  }
sfi_wait_sequencer(info);
}
#endif
/*****************************************************************************/
#ifdef SFIMAPPED
void FWCa_(sfi_info* info, u_int32_t pa, u_int32_t sa, u_int32_t data)
{
SEQ_W(info, seq_prim_csr, pa);
SEQ_W(info, seq_secad_w, sa);
SEQ_W(info, seq_rndm_w, data);
/* if (sfi_wait_sequencer(info)<0) printf("FWC_: Fehler\n"); */
sfi_wait_sequencer(info);
}
#else
#error FWCa_ not implemented
#endif
/*****************************************************************************/
#ifdef SFIMAPPED
void FWD_(sfi_info* info, u_int32_t pa, u_int32_t sa, u_int32_t data)
{
SEQ_W(info, seq_prim_dsr, pa);
SEQ_W(info, seq_secad_w, sa);
SEQ_W(info, seq_rndm_w_dis, data);
sfi_wait_sequencer(info);
}
#else
void FWD_(sfi_info* info, u_int32_t pa, u_int32_t sa, u_int32_t data)
{
sfi_command comm[3];
comm[0].cmd=seq_prim_dsr;      comm[0].data=pa;
comm[1].cmd=seq_secad_w;       comm[1].data=sa;
comm[2].cmd=seq_rndm_w_dis;    comm[2].data=data;
if (write(info->path, comm, sizeof(comm))!=sizeof(comm))
  {
  perror("FWD_: write sfi");
  info->status.error=sfi_error_unknown;
  return -1;
  }
sfi_wait_sequencer(info);
}
#endif
/*****************************************************************************/
#ifdef SFIMAPPED
void FWDa_(sfi_info* info, u_int32_t pa, u_int32_t sa, u_int32_t data)
{
SEQ_W(info, seq_prim_dsr, pa);
SEQ_W(info, seq_secad_w, sa);
SEQ_W(info, seq_rndm_w, data);
sfi_wait_sequencer(info);
}
#else
#error FWDa_ not implemented
#endif
/*****************************************************************************/
#ifdef SFIMAPPED
void FCDISC_(sfi_info* info)
{
SEQ_W(info, (SFI_FUNC_DISCON), 0);
}
#else
#error FCDISC_ not implemented
#endif
/*****************************************************************************/
#ifdef SFIMAPPED
int FCWWss_(sfi_info* info, int dat)                    /*write word; res= ss*/
{
SEQ_W(info, seq_rndm_w, dat);
sfi_wait_sequencer(info);
return sscode();
}
#else
#error FCWWss_ not implemented
#endif
/*****************************************************************************/
#ifdef SFIMAPPED
int FCRWS_(sfi_info* info, int sa)                /*read word on sec. address*/
{
SEQ_W(info, seq_secad_w, sa);
SEQ_W(info, seq_rndm_r, mist);
return sfi_read_fifoword(info);
}
#else
#error FCRWS_( not implemented
#endif
/*****************************************************************************/
#ifdef SFIMAPPED
int FCWWS_(sfi_info* info, int sa, int dat) /*write word on sec. address; res= ss*/
{
SEQ_W(info, seq_secad_w, sa);
SEQ_W(info, seq_rndm_w, dat);
sfi_wait_sequencer(info);
return sscode();
}
#else
#error FCWWS_( not implemented
#endif
/*****************************************************************************/
#ifdef SFIMAPPED
int FCRW_(sfi_info* info)                                         /*read word*/
{
SEQ_W(info, seq_rndm_r, mist);
return sfi_read_fifoword(info);
}
#else
#error FCRW_ not implemented
#endif
/*****************************************************************************/
#ifdef SFIMAPPED
void FCWW_(sfi_info* info, int dat)                              /*write word*/
{
SEQ_W(info, seq_rndm_w, dat);
sfi_wait_sequencer(info);
}
#else
#error FCWW_ not implemented
#endif
/*****************************************************************************/
#ifdef SFIMAPPED
void FCPC_(sfi_info* info, int pa)           /*primary address cycle; control*/
{
SEQ_W(info, seq_prim_csr, pa);
sfi_wait_sequencer(info);
}
#else
#error FCPC_ not implemented
#endif
/*****************************************************************************/
#ifdef SFIMAPPED
void FCPD_(sfi_info* info, int pa)              /*primary address cycle; data*/
{
SEQ_W(info, seq_prim_dsr, pa);
sfi_wait_sequencer(info);
}
#else
#error FCPC_ not implemented
#endif
/*****************************************************************************/
#ifdef SFIMAPPED
int FCWSAss_(sfi_info* info, int sa)       /*write secondary address; res= ss*/
{
SEQ_W(info, seq_secad_w, sa);
sfi_wait_sequencer(info);
return sscode();
}
#else
#error FCWSAss_ not implemented
#endif
/*****************************************************************************/
/*****************************************************************************/
