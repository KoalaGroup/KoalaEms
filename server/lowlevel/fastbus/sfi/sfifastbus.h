/******************************************************************************
*                                                                             *
* lowlevel/sfi/sfifastbus.h                                                   *
*                                                                             *
* created: 25.04.97                                                           *
* last changed: 27.04.97                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#ifndef _sfifastbus_h_
#define _sfifastbus_h_

#include <sconf.h>
#include "string.h"
#include "sfilib.h"
#include "../../lowbuf/lowbuf.h"

extern sfi_info sfi;
/*
#define CPY_TO_FBBUF(dest, source, num) (memcpy(lowbuf_extrabuffer()+(dest), \
  (source), ((num)*4)) ,lowbuf_extrabuffer()+(dest))
#define CPY_FROM_FBBUF(dest, source, num)
*/
#define FRC(pa, sa) \
  FRC_(&sfi, (pa), (sa))
u_int32_t FRC_(sfi_info* info, u_int32_t pa, u_int32_t sa);
#define FRCa(pa, sa) \
  FRCa_(&sfi, (pa), (sa))
u_int32_t FRCa_(sfi_info* info, u_int32_t pa, u_int32_t sa);

#define FRCM(ba) \
  FRCM_(&sfi, (ba))
u_int32_t FRCM_(sfi_info* info, u_int32_t ba);

#define FRD(pa, sa) \
  FRD_(&sfi, (pa), (sa)) 
u_int32_t FRD_(sfi_info* info, u_int32_t pa, u_int32_t sa);
#define FRDa(pa, sa) \
  FRDa_(&sfi, (pa), (sa)) 
u_int32_t FRDa_(sfi_info* info, u_int32_t pa, u_int32_t sa);
#define FRDM(ba) \
  FRDM_(&sfi, (ba)) 
u_int32_t FRDM_(sfi_info* info, u_int32_t ba);

#define FWC(pa, sa, data) \
  FWC_(&sfi, (pa), (sa), (data))
void FWC_(sfi_info* info, u_int32_t pa, u_int32_t sa, u_int32_t data);
#define FWCa(pa, sa, data) \
  FWCa_(&sfi, (pa), (sa), (data))
void FWCa_(sfi_info* info, u_int32_t pa, u_int32_t sa, u_int32_t data);
#define FWCM(ba, data) \
  FWCM_(&sfi, (ba), (data))
void FWCM_(sfi_info* info, u_int32_t ba,u_int32_t data);

#define FWD(pa, sa, data) \
  FWD_(&sfi, (pa), (sa), (data))
void FWD_(sfi_info* info, u_int32_t pa, u_int32_t sa, u_int32_t data);
#define FWDa(pa, sa, data) \
  FWDa_(&sfi, (pa), (sa), (data))
void FWDa_(sfi_info* info, u_int32_t pa, u_int32_t sa, u_int32_t data);
#define FWDM(ba, data) \
  FWDM_(&sfi, (ba), (data))
void FWDM_(sfi_info* info, u_int32_t ba, u_int32_t data);

#ifdef SFI_DRIVER_BLOCKREAD
#define FRCB(pa, sa, dest, count) \
  FRCB_driver(&sfi, (pa), (sa), (dest), (count))
#define FRDB(pa, sa, dest, count) \
  FRDB_driver(&sfi, (pa), (sa), (dest), (count))
#define FRDB_S(pa, dest, count) \
  FRDB_S_driver(&sfi, (pa), (dest), (count))
#else
#define FRCB(pa, sa, dest, count) \
  FRCB_direct(&sfi, (pa), (sa), (dest), (count))
#define FRDB(pa, sa, dest, count) \
  FRDB_direct(&sfi, (pa), (sa), (dest), (count))
#define FRDB_S(pa, dest, count) \
  FRDB_S_direct(&sfi, (pa), (dest), (count))
#endif
int FRDB_driver(sfi_info* info, u_int32_t pa, u_int32_t sa, u_int32_t* dest,
    u_int32_t count);
int FRCB_direct(sfi_info* sfi, u_int32_t pa, u_int32_t sa, u_int32_t* dest,
    u_int32_t count);
int FRCB_driver(sfi_info* info, u_int32_t pa, u_int32_t sa, u_int32_t* dest,
    u_int32_t count);
int FRDB_direct(sfi_info* sfi, u_int32_t pa, u_int32_t sa, u_int32_t* dest,
    u_int32_t count);
int FRDB_S_direct(sfi_info* sfi, u_int32_t pa, u_int32_t* dest,
    u_int32_t count);
int FRDB_S_driver(sfi_info* info, u_int32_t pa, u_int32_t* dest,
  u_int32_t count);

#define FWCB(pa, sa, source, count) \
  FWCB_(&sfi, (pa), (sa), (source), (count))
int FWCB_(sfi_info* sfi, u_int32_t pa, u_int32_t sa, u_int32_t* src,
    u_int32_t count);
#define FWDB(pa, sa, source, count) \
  FWDB_(&sfi, (pa), (sa), (source), (count))
int FWDB_(sfi_info* sfi, u_int32_t pa, u_int32_t sa, u_int32_t* src,
    u_int32_t count);

#define sscode() sscode_(&sfi)
#define FCDISC() FCDISC_(&sfi)
void FCDISC_(sfi_info* info);
#define FCWW(dat) FCWW_(&sfi, (dat))
void FCWW_(sfi_info* info, int dat);
#define FCWWss(dat) FCWWss_(&sfi, (dat))
int FCWWss_(sfi_info* info, int dat);
#define FCRWS(sa) FCRWS_(&sfi, (sa))
int FCRWS_(sfi_info* info, int sa);
#define FCWWS(sa, dat) FCWWS_(&sfi, (sa), (dat))
int FCWWS_(sfi_info* info, int sa, int dat);
#define FCRW() FCRW_(&sfi)
int FCRW_(sfi_info* info);
#define FCPC(pa) FCPC_(&sfi, (pa))
void FCPC_(sfi_info* info, int pa);
#define FCPD(pa) FCPD_(&sfi, (pa))
void FCPD_(sfi_info* info, int pa);
#define FCWSAss(sa) FCWSAss_(&sfi, (sa))
int FCWSAss_(sfi_info* info, int sa);

#define FBPULSE() FBPULSE_(&sfi, 0x100) /* NIM Output 1 */
#define FBPULSE_reset() FBPULSE_reset_(&sfi, 0x100) /* NIM Output 1 */
void FBPULSE_(sfi_info* info, int outmask);
void FBPULSE_reset_(sfi_info* info, int outmask);

int setarbitrationlevel(int arblevel);
int fb_modul_id(int pa);
void SFIout_seq(u_int32_t val);
int ist_aehnlich(int type, int mtype);

#endif
/*****************************************************************************/
/*****************************************************************************/
