/******************************************************************************
*                                                                             *
* fbustrap_c.c                                                                *
*                                                                             *
* created: 18.10.94                                                           *
* last changed: 26.10.94                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include "fbacc.h"
#include "../bustrap/bustrap.h"

extern int fb_arg_d0, fb_arg_d1, fb_arg_s4, fb_arg_s8, fb_arg_s12, fb_arg_s16,
    fb_arg_s20;
extern void FRC_p(), FRD_p(), FWC_p(), FWD_p();
extern void FRCa_p(), FRDa_p(), FWCa_p(), FWDa_p();
extern void FRCM_p(), FRDM_p(), FWCM_p(), FWDM_p();
extern void FRCMa_p(), FRDMa_p(), FWCMa_p(), FWDMa_p();
extern void FCPC_p(), FCPD_p();
extern void FRCB_p(), FWCB_p(),FRDB_p(), FWDB_p();
extern void FRCBM_p(), FWCBM_p(),FRDBM_p(), FWDBM_p();
extern void FRDB_S_p();

typedef enum {non, paddr, rsingle, wsingle, rblock, wblock, rblock_s,
    rmulti, wmulti} fbfunctypes;

typedef struct
  {
  trappc proc;
  fbfunctypes functype;
  char* name;
  } fbfuncent;

fbfuncent fbfunctab[]=
  {
  {FRC_p, rsingle, "FRC"},
  {FRD_p, rsingle, "FRD"},
  {FWC_p, wsingle, "FWC"},
  {FWD_p, wsingle, "FWD"},
  {FRCa_p, rsingle, "FRCa"},
  {FRDa_p, rsingle, "FRDa"},
  {FWCa_p, wsingle, "FWCa"},
  {FWDa_p, wsingle, "FWDa"},
  {FRCM_p, rmulti, "FRCM"},
  {FRDM_p, rmulti, "FRDM"},
  {FWCM_p, wmulti, "FWCM"},
  {FWDM_p, wmulti, "FWDM"},
  {FRCMa_p, rmulti, "FRCMa"},
  {FRDMa_p, rmulti, "FRDMa"},
  {FWCMa_p, wmulti, "FWCMa"},
  {FWDMa_p, wmulti, "FWDMa"},
  {FCPC_p, paddr,  "FCPC"},
  {FCPD_p, paddr,  "FCPD"},
  {FRCB_p, rblock, "FRCB"},
  {FWCB_p, wblock, "FWCB"},
  {FRDB_p, rblock, "FRDB"},
  {FWDB_p, wblock, "FWDB"},
  {FRCBM_p, rblock, "FRCBM"},
  {FWCBM_p, wblock, "FWCBM"},
  {FRDBM_p, rblock, "FRDBM"},
  {FWDBM_p, wblock, "FWDBM"},
  {FRDB_S_p, rblock_s, "FRDB_S"},
  {0, non, 0},
  };

/*****************************************************************************/

void fbtrap_hand(trappc pc)
{
int i=0;

while ((fbfunctab[i].proc!=pc) && (fbfunctab[i].proc!=0)) i++;
if (fbfunctab[i].proc!=0)
  {
  printf("Fehler in Fastbusprozedur %s", fbfunctab[i].name);
  switch (fbfunctab[i].functype)
    {
    case paddr:
      printf("(%d)", fb_arg_d0);
      break;
    case rsingle:
      printf("(%d, 0x%x)", fb_arg_d0, fb_arg_d1);
      break;
    case wsingle:
      printf("(%d, 0x%x, 0x%08x)", fb_arg_d0, fb_arg_d1, fb_arg_s4);
      break;
    case rmulti:
      printf("(%d)", fb_arg_d0);
      break;
    case wmulti:
      printf("(%d, 0x%08x)", fb_arg_d0, fb_arg_d1);
      break;
    case rblock:
      printf("(%d, 0x%x, 0x%08x, %d)", fb_arg_d0, fb_arg_d1, fb_arg_s16,
          fb_arg_s20);
      break;
    case wblock:
      printf("(%d, 0x%x, 0x%08x, %d, ...)", fb_arg_d0, fb_arg_d1, fb_arg_s16,
          fb_arg_s20);
      break;
    case rblock_s:
      printf("(%d, 0x%08x, %d)", fb_arg_d0, fb_arg_d1, fb_arg_s16);
      break;
    }
  printf("\n");
  }
}

/*****************************************************************************/
/*****************************************************************************/
