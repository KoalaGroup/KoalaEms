/* $ZEL*/

#ifndef _VME_h_
#define _VME_h_
u_char *VMEinit();
u_char *SMPaddr();
u_char *A_Addr();
u_char *VME_Addr();
int    MAKDOU();
u_char *MAKBYT();
void SIEM_303SETCTR5();
void SIEM_303SETMLH5();
u_char *SIEM_303GETMLH5();
u_char *SIEM_303GETCOU5();
u_char *SIEM_303GETCRT5();

#include "directvme.h"
#endif

