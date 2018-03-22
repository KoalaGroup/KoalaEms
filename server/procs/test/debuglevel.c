/******************************************************************************
*                                                                             *
* debuglevel.c                                                                *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 04.11.94                                                                    *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: debuglevel.c,v 1.8 2017/10/20 23:20:52 wuestner Exp $";

#include <errorcodes.h>
#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "../procprops.h"
#include "../procs.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/test")

/*****************************************************************************/
/*
GetDebugMask
*/
plerrcode proc_GetDebugMask(__attribute__((unused)) ems_u32* p)
{
#ifdef DEBUG
*outptr++=debug;
#endif
return(plOK);
}

plerrcode test_proc_GetDebugMask(ems_u32* p)
{
#ifdef DEBUG
if (p[0]==0)
  return(plOK);
else
  return(plErr_ArgNum);
#else
return(plErr_NoSuchProc);
#endif
}
#ifdef PROCPROPS
static procprop GetDebugMask_prop={0, 1, "void", 0};

procprop* prop_proc_GetDebugMask(void)
{
return(&GetDebugMask_prop);
}
#endif
char name_proc_GetDebugMask[]="GetDebugMask";
int ver_proc_GetDebugMask=1;

/*****************************************************************************/
/*
SetDebugMask
*/
plerrcode proc_SetDebugMask(ems_u32* p)
{
#ifdef DEBUG
*outptr++=debug;
debug=p[1];
#endif
return(plOK);
}

plerrcode test_proc_SetDebugMask(ems_u32* p)
{
#ifdef DEBUG
if (p[0]==1)
  return(plOK);
else
  return(plErr_ArgNum);
#else
return(plErr_NoSuchProc);
#endif
}
#ifdef PROCPROPS
static procprop SetDebugMask_prop={0, 1, "<mask>", 0};

procprop* prop_proc_SetDebugMask(void)
{
return(&SetDebugMask_prop);
}
#endif
char name_proc_SetDebugMask[]="SetDebugMask";
int ver_proc_SetDebugMask=1;

/*****************************************************************************/
/*
GetVerboseFlag
*/
plerrcode proc_GetVerboseFlag(__attribute__((unused)) ems_u32* p)
{
#ifdef DEBUG
*outptr++=verbose;
#endif
return(plOK);
}

plerrcode test_proc_GetVerboseFlag(ems_u32* p)
{
#ifdef DEBUG
if (p[0]==0)
  return(plOK);
else
  return(plErr_ArgNum);
#else
return(plErr_NoSuchProc);
#endif
}
#ifdef PROCPROPS
static procprop GetVerboseFlag_prop={0, 1, "void", 0};

procprop* prop_proc_GetVerboseFlag(void)
{
return(&GetVerboseFlag_prop);
}
#endif
char name_proc_GetVerboseFlag[]="GetVerboseFlag";
int ver_proc_GetVerboseFlag=1;

/*****************************************************************************/
/*
SetVerboseFlag
*/
plerrcode proc_SetVerboseFlag(ems_u32* p)
{
#ifdef DEBUG
*outptr++=verbose;
verbose=p[1];
#endif
return(plOK);
}

plerrcode test_proc_SetVerboseFlag(ems_u32* p)
{
#ifdef DEBUG
if (p[0]==1)
  return(plOK);
else
  return(plErr_ArgNum);
#else
return(plErr_NoSuchProc);
#endif
}
#ifdef PROCPROPS
static procprop SetVerboseFlag_prop={0, 1, "<0|1>", 0};

procprop* prop_proc_SetVerboseFlag(void)
{
return(&SetVerboseFlag_prop);
}
#endif
char name_proc_SetVerboseFlag[]="SetVerboseFlag";
int ver_proc_SetVerboseFlag=1;

/*****************************************************************************/
/*****************************************************************************/
