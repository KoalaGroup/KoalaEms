/******************************************************************************
*                                                                             *
* fbacc.h                                                                     *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* created: 12.10.94                                                           *
* last changed: 25.10.94                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#ifndef _fbacc_h_
#define _fbacc_h_

#include "fbaccmacro.h"

void FBCLEAR();
void FBPULSE();
int  get_ga_chi();
int  setarbitrationlevel(int arblevel);
int  FCARB();
void FCREL();
int  keep_mastership(int keep);
int  auto_disconnect(int automat);
int  set_pip(int pip);
int  ssenable(int ssreg);
int  fb_modul_id(int pa);

int  FRC(int pa, int sa);
int  FRD(int pa, int sa);
void FWC(int pa, int sa, int data);
void FWD(int pa, int sa, int data);
int  FRCM(int ba);
int  FRDM(int ba);
void FWCM(int ba, int data);
void FWDM(int ba, int data);
int  FRCa(int pa, int sa);
int  FRDa(int pa, int sa);
void FWCa(int pa, int sa, int data);
void FWDa(int pa, int sa, int data);
int  FRCMa(int ba);
int  FRDMa(int ba);
void FWCMa(int ba, int data);
void FWDMa(int ba, int data);

int  FRCB(int pa, int sa, int* dest, int count);
int  FRDB(int pa, int sa, int* dest, int count);
int  FWCB(int pa, int sa, int* source, int count);
int  FWDB(int pa, int sa, int* source, int count);
int  FRCBM(int pa, int sa, int* dest, int count);
int  FRDBM(int pa, int sa, int* dest, int count);
int  FWCBM(int pa, int sa, int* source, int count);
int  FWDBM(int pa, int sa, int* source, int count);
int  FRDB_S(int pa, int* dest, int count);

void FCPD(int pa);
void FCPC(int pa);
int  FCRWss();
int  FCRWS(int sa);
int  FCWWss(int dat);
int  FCWWS(int sa, int dat);
int  FCWSAss(int sa);
int  FCPWSAss(int pa, int sa);
int  FCRB(int* dest, int count);
int  FCWB(int* source, int count);

#endif

/*****************************************************************************/
/*****************************************************************************/
