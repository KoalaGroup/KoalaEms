/*
 * procs/general/raw/WriteRaw.c
 * 
 * created 04.11.94 or before
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: WriteRaw.c,v 1.8 2017/10/09 21:25:37 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../objects/var/variables.h"
#include <emsctypes.h>
#include "../../procs.h"
#include "../../procprops.h"

#ifdef NOVOLATILE
#define VOLATILE
#else
#define VOLATILE volatile
#endif

RCS_REGISTER(cvsid, "procs/general/raw")

/*****************************************************************************/

/*
WriteRaw(addr, value)
schreibt value auf die physikalische Adresse addr
*/

plerrcode
proc_WriteRaw(ems_u32* p)
{
    T(proc_WriteRaw)
    *(VOLATILE ems_u32*)p[1]=p[2];
    return plOK;
}

plerrcode
test_proc_WriteRaw(ems_u32* p)
{
    T(test_proc_WriteRaw)
    if (p[1]!=2)
        return plErr_ArgNum;
    wirbrauchen=0;
    return plOK;
}

#ifdef PROCPROPS
static procprop WriteRaw_prop={0, 0, "<addr> <val>", 0};
procprop* prop_proc_WriteRaw()
{
    return &WriteRaw_prop;
}
#endif

char name_proc_WriteRaw[]="WriteRaw";
int ver_proc_WriteRaw=1;

/*****************************************************************************/

plerrcode
proc_WriteByteRaw(ems_u32* p)
{
    T(proc_WriteByteRaw)
    *(VOLATILE ems_u8*)p[1]= (ems_u8)p[2];
    return plOK;
}

plerrcode
test_proc_WriteByteRaw(ems_u32* p)
{
    T(test_proc_WriteByteRaw)
    if (*p!=2)
        return plErr_ArgNum;
    if (p[2]>255)
        return plErr_ArgRange;
    wirbrauchen=0;
    return plOK;
}

#ifdef PROCPROPS
static procprop WriteByteRaw_prop={0, 0, "<addr> <val>", 0};
procprop* prop_proc_WriteByteRaw()
{
    return &WriteByteRaw_prop;
}
#endif

char name_proc_WriteByteRaw[]="WriteByteRaw";
int ver_proc_WriteByteRaw=1;

/*****************************************************************************/

plerrcode
proc_WriteIntVarByteRaw(ems_u32* p)
{
    T(proc_WriteIntVarByteRaw)
    *(VOLATILE ems_u8*)p[1]= (ems_u8)var_list[p[2]].var.val;
    return plOK;
}

plerrcode test_proc_WriteIntVarByteRaw(ems_u32* p)
{
    T(test_proc_WriteIntVarByteRaw)
    if (*p!=2)
        return plErr_ArgNum;
    if (p[2]>MAX_VAR)
        return plErr_IllVar;
    if (!var_list[p[2]].len)
        return plErr_NoVar;
    if (var_list[p[2]].len!=1)
        return plErr_IllVarSize;
    wirbrauchen=0;
    return plOK;
}

#ifdef PROCPROPS
static procprop WriteIntVarByteRaw_prop={0, 0, "<addr> <variable>", 0};
procprop* prop_proc_WriteIntVarByteRaw()
{
    return &WriteIntVarByteRaw_prop;
}
#endif

char name_proc_WriteIntVarByteRaw[]="WriteIntVarByteRaw";
int ver_proc_WriteIntVarByteRaw=1;

/*****************************************************************************/
/*****************************************************************************/
