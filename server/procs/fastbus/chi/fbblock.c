/******************************************************************************
*                                                                             *
* fbblock.c                                                                   *
*                                                                             *
* created before: 11.08.93                                                    *
* last changed: 05.11.97                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: fbblock.c,v 1.4 2011/04/06 20:30:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procprops.h"
#include "../../../lowlevel/fastbus/fastbus.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/fastbus")

/*****************************************************************************/
/*
FRCB
p[0] : No. of parameters (==3)
p[1] : primary address
p[2] : secondary address
p[3] : count
outptr[0] : SS-code
outptr[1] : count
outptr[2] : values
*/

plerrcode proc_FRCB(int* p)
{
outptr[1]=FRCB(p[1], p[2], outptr+2, p[3]);
outptr[0]=sscode();
outptr+=2+outptr[1];
return(plOK);
}

plerrcode test_proc_FRCB(int* p)
{
if (p[0]!=3) return(plErr_ArgNum);
return(plOK);
}

static procprop FRCB_prop={1, -1, 0, 0};

procprop* prop_proc_FRCB()
{
return(&FRCB_prop);
}

char name_proc_FRCB[]="FRCB";
int ver_proc_FRCB=2;

/*****************************************************************************/
/*
FRDB
p[0] : No. of parameters (==3)
p[1] : primary address
p[2] : secondary address
p[3] : count
outptr[0] : SS-code
outptr[1] : count
outptr[2] : values
*/

plerrcode proc_FRDB(int* p)
{
outptr[1]=FRDB(p[1], p[2], outptr+2, p[3]);
outptr[0]=sscode();
outptr+=2+outptr[1];
return(plOK);
}

plerrcode test_proc_FRDB(int* p)
{
if (p[0]!=3) return(plErr_ArgNum);
return(plOK);
}

static procprop FRDB_prop={1, -1, 0, 0};

procprop* prop_proc_FRDB()
{
return(&FRDB_prop);
}

char name_proc_FRDB[]="FRDB";
int ver_proc_FRDB=2;

/*****************************************************************************/
/*
FRDB_S
p[0] : No. of parameters (==2)
p[1] : primary address
p[2] : count
outptr[0] : SS-code
outptr[1] : count
outptr[2] : values
*/

plerrcode proc_FRDB_S(int* p)
{
outptr[1]=FRDB_S(p[1], outptr+2, p[2]);
outptr[0]=sscode();
outptr+=2+outptr[1];
return(plOK);
}

plerrcode test_proc_FRDB_S(int* p)
{
if (p[0]!=2) return(plErr_ArgNum);
return(plOK);
}

static procprop FRDB_S_prop={1, -1, 0, 0};

procprop* prop_proc_FRDB_S()
{
return(&FRDB_S_prop);
}

char name_proc_FRDB_S[]="FRDB_S";
int ver_proc_FRDB_S=2;

/*****************************************************************************/
/*
FRDBv
p[0] : No. of parameters (==3)
p[1] : primary address
p[2] : secondary address
p[3] : Variable for count
outptr[0] : SS-code
outptr[1] : count
outptr[2] : values
*/

plerrcode proc_FRDBv(int* p)
{
outptr[1]=FRDB(p[1], p[2], outptr+2, var_read_int(p[3]));
outptr[0]=sscode();
outptr+=2+outptr[1];
return(plOK);
}

plerrcode test_proc_FRDBv(int* p)
{
int res, size;
if (p[0]!=3) return(plErr_ArgNum);
#ifndef OBJ_VAR
return(plErr_IllVar);
#else
if ((res=var_attrib(p[3], &size))!=plOK) return(res);
if (size!=1) return(plErr_IllVarSize);
return(plOK);
#endif
}

static procprop FRDBv_prop={1, -1, 0, 0};

procprop* prop_proc_FRDBv()
{
return(&FRDBv_prop);
}

char name_proc_FRDBv[]="FRDBv";
int ver_proc_FRDBv=1;

/*****************************************************************************/
/*
FWCB
p[0] : No. of parameters (>=3)
p[1] : primary address
p[2] : secondary address
p[3] : count
...  : values
outptr[0] : SS-code
outptr[1] : count
*/

plerrcode proc_FWCB(int* p)
{
/*memcpy(FB_BUFBEG, p+4, p[3]*4);*/
outptr[1]=FWCB(p[1], p[2], CPY_TO_FBBUF(0, p+4, p[3]), p[3]);
outptr[0]=sscode();
outptr+=2;
return(plOK);
}

plerrcode test_proc_FWCB(int* p)
{
if (p[0]<3) return(plErr_ArgNum);
if (p[0]!=(p[3]+3)) return(plErr_ArgNum);
return(plOK);
}

static procprop FWCB_prop={0, 2, 0, 0};

procprop* prop_proc_FWCB()
{
return(&FWCB_prop);
}

char name_proc_FWCB[]="FWCB";
int ver_proc_FWCB=2;

/*****************************************************************************/
/*
FWDB
p[0] : No. of parameters (>=3)
p[1] : primary address
p[2] : secondary address
p[3] : count
...  : values
outptr[0] : SS-code
outptr[1] : count
*/

plerrcode proc_FWDB(int* p)
{
outptr[1]=FWDB(p[1], p[2], p+4, p[3]);
outptr[0]=sscode();
outptr+=2;
return(plOK);
}

plerrcode test_proc_FWDB(int* p)
{
if (p[0]<3) return(plErr_ArgNum);
if (p[0]!=(p[3]+3)) return(plErr_ArgNum);
return(plOK);
}

static procprop FWDB_prop={0, 2, 0, 0};

procprop* prop_proc_FWDB()
{
return(&FWDB_prop);
}

char name_proc_FWDB[]="FWDB";
int ver_proc_FWDB=2;

/*****************************************************************************/
/*****************************************************************************/
