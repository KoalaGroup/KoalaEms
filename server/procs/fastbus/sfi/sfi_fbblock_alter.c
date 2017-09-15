/*
 * procs/fastbus/sfi/sfi_fbblock_alter.c
 * 
 * created: 20.01.1999 (PW)
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sfi_fbblock_alter.c,v 1.6 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../procs.h"
#include "../../procprops.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/fastbus/sfi")

/*****************************************************************************/
/*
FRDB_alter
p[0] : No. of parameters (==3)
p[1] : primary address
p[2] : secondary address
p[3] : count
outptr[0] : SS-code
outptr[1] : count
outptr[2] : values
*/

plerrcode proc_FRDB_alter(ems_u32* p)
{
/* call the OTHER method */
#ifdef SFI_DRIVER_BLOCKREAD
outptr[1]=FRDB_direct(&sfi, p[1], p[2], outptr+2, p[3]);
#else
outptr[1]=FRDB_driver(&sfi, p[1], p[2], outptr+2, p[3]);
#endif
outptr[0]=sscode();
outptr+=2+outptr[1];
return(plOK);
}

plerrcode test_proc_FRDB_alter(ems_u32* p)
{
if (p[0]!=3) return(plErr_ArgNum);
return(plOK);
}
#ifdef PROCPROPS
static procprop FRDB_alter_prop={1, -1, 0, 0};

procprop* prop_proc_FRDB_alter()
{
return(&FRDB_alter_prop);
}
#endif
char name_proc_FRDB_alter[]="FRDB_alter";
int ver_proc_FRDB_alter=1;

/*****************************************************************************/
/*****************************************************************************/
