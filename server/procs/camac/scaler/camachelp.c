static const char* cvsid __attribute__((unused))=
    "$ZEL: camachelp.c,v 1.2 2011/04/06 20:30:30 wuestner Exp $";

/* einziger Zweck: Erzeugen von Call-Overhead,
so, dass der Compiler nicht optimieren kann! */

#include <rcs_ids.h>
#include "../../../lowlevel/camac/camac.h"

RCS_REGISTER(cvsid, "procs/camac/scaler")

int privCAMACread(addr)
int *addr;
{
return(CAMACread(addr));
}
