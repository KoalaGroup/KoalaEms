/*
 * procs/general/raw/ReadRaw.c
 * 
 * created 04.11.94 or before
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: ReadRaw.c,v 1.7 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <emsctypes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../procprops.h"

#ifdef NOVOLATILE
#define VOLATILE
#else
#define VOLATILE volatile
#endif

extern ems_u32* outptr;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/general/raw")

/*****************************************************************************/
/*
ReadRaw(addr)
liest ein Langwort an der physikalischen Adresse addr
und schreibt das Resultat in den Ausgabepuffer
*/

plerrcode
proc_ReadRaw(ems_u32* p)
{
    T(proc_ReadRaw)
    *outptr++=*(VOLATILE ems_u32*)p[1];
    return plOK;
}

plerrcode
test_proc_ReadRaw(ems_u32* p)
{
    T(test_proc_ReadRaw)
    if (p[1]!=1)
        return plErr_ArgNum;
    wirbrauchen=1;
    return plOK;
}

#ifdef PROCPROPS
static procprop ReadRaw_prop={0, 1, "addr", 0};
procprop* prop_proc_ReadRaw()
{
    return &ReadRaw_prop;
}
#endif

char name_proc_ReadRaw[]="ReadRaw";
int ver_proc_ReadRaw=1;

/*****************************************************************************/
/*
ReadByteRaw(addr)
liest ein Byte an der physikalischen Adresse addr
und schreibt das Resultat in den Ausgabepuffer
*/

plerrcode
proc_ReadByteRaw(ems_u32* p)
{
    T(proc_ReadByteRaw)
    *outptr++=*(VOLATILE ems_u8*)p[1];
    return plOK;
}

plerrcode
test_proc_ReadByteRaw(ems_u32* p)
{
    T(test_proc_ReadByteRaw)
    if (p[1]!=1)
        return plErr_ArgNum;
    wirbrauchen=1;
    return plOK;
}

#ifdef PROCPROPS
static procprop ReadByteRaw_prop={0, 1, "addr", 0};
procprop* prop_proc_ReadByteRaw()
{
    return &ReadByteRaw_prop;
}
#endif

char name_proc_ReadByteRaw[]="ReadByteRaw";
int ver_proc_ReadByteRaw=1;

/*****************************************************************************/
/*****************************************************************************/
