/*
 * procs/hlral/dummy/hlraldummy.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: hlraldummy.c,v 1.5 2011/04/06 20:30:33 wuestner Exp $";

#include <sys/types.h>
#include <cdefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "hlraldummyread.h"

#define board p[1]

extern ems_u32* outptr;
static int num_crates=0;
static int numcards[64];
static int readoutmode=0;

void HLRAL_Dummy_total(int, int*);

RCS_REGISTER(cvsid, "procs/hlral/dummy")

/*****************************************************************************/
/*
 * p[0]: length (==1)
 * p[1]: board
 */
plerrcode proc_HLRALconfigDummy(ems_u32* p)
{
    int i;

    if (p[0]==1) {
        for (i=0; i<num_crates; i++) *outptr++=numcards[i];
    } else  {
        num_crates=p[0]-1;
        for (i=0; i<num_crates; i++) numcards[i]=(p+2)[i];
    }
    return (plOK);
}

plerrcode test_proc_HLRALconfigDummy(ems_u32* p)
{
    if ((p[0]<1) || (p[0]>65)) return (plErr_ArgNum);
    return (plOK);
}

char name_proc_HLRALconfigDummy[] = "HLRALconfigDummy";

int ver_proc_HLRALconfigDummy = 1;
/*****************************************************************************/
/*
 * p[0]: length (==2)
 * p[1]: board
 * p[2]: mode 0: norm 1: test
 */
plerrcode proc_HLRALstartreadoutDummy(ems_u32* p)
{
    readoutmode=p[2];
    return (plOK);
}

plerrcode test_proc_HLRALstartreadoutDummy(ems_u32* p)
{
    if (p[0] != 2) return (plErr_ArgNum);
    return (plOK);
}

char name_proc_HLRALstartreadoutDummy[] = "HLRALstartreadoutDummy";

int ver_proc_HLRALstartreadoutDummy = 1;
/*****************************************************************************/
plerrcode proc_HLRALreadoutDummy(ems_u32* p)
{
    if (readoutmode) {
        HLRAL_Dummy_total(num_crates, numcards);
    } else {
    }
    return (plOK);
}

plerrcode test_proc_HLRALreadoutDummy(ems_u32* p)
{
    if (p[0] != 1) return (plErr_ArgNum);
    return (plOK);
}

char name_proc_HLRALreadoutDummy[] = "HLRALreadoutDummy";

int ver_proc_HLRALreadoutDummy = 1;
/*****************************************************************************/
/*****************************************************************************/
