/*
 * lowlevel/hlral/hlral.h
 * $ZEL: hlral.h,v 1.4 2004/02/11 13:14:16 wuestner Exp $
 */

#include <emsctypes.h>

int hlral_low_printuse __P((FILE *));
errcode hlral_low_init __P((char *));
errcode hlral_low_done __P((void));

int hlral_reset __P((int));
int hlral_countchips __P((int, int, int));
int hlral_datapathtest __P((int, int, int));
int hlral_loadregs __P((int, int, int, ems_u8 *, int));
plerrcode hlral_buffermode __P((int, int));
plerrcode hlral_loaddac_2 __P((int, int, int, int, int*, int));
ems_u32 *hlral_testreadout __P((int, ems_u32 *));
ems_u32 *hlral_readout __P((int, ems_u32 *));
void hlral_startreadout __P((int, int));
int number_of_ralhotlinks __P((void));

#define HLRAL_ANKE 1111
#define HLRAL_ATRAP 2222
