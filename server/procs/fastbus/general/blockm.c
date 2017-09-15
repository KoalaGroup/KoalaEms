/******************************************************************************
*                                                                             *
* blockm.c                                                                    *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* created: 25.10.94                                                           *
* last changed: 04.11.94                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: blockm.c,v 1.3 2011/04/06 20:30:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../lowlevel/fastbus/fastbus.h"
#include "../procprops.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/fastbus/general")

/*****************************************************************************/
/*
FRCBM
p[0] : No. of parameters (==3)
p[1] : primary address
p[2] : secondary address
p[3] : count
outptr[0] : SS-code
outptr[1] : count
outptr[2] : values
*/

plerrcode proc_FRCBM(int* p)
{
outptr[1]=FRCBM(p[1], p[2], outptr+2, p[3]);
outptr[0]=sscode();
outptr+=2+outptr[1];
return(plOK);
}

plerrcode test_proc_FRCBM(int* p)
{
if (p[0]!=3) return(plErr_ArgNum);
return(plOK);
}

static procprop FRCBM_prop={1, -1, 0, 0};

procprop* prop_proc_FRCBM()
{
return(&FRCBM_prop);
}

char name_proc_FRCBM[]="FRCBM";
int ver_proc_FRCBM=2;

/*****************************************************************************/
/*
FWCBM
p[0] : No. of parameters (>=3)
p[1] : primary address
p[2] : secondary address
p[3] : count
...  : values
outptr[0] : SS-code
outptr[1] : count
*/

plerrcode proc_FWCBM(int* p)
{
/*
memcpy(FB_BUFBEG, p+4, p[3]*4);
outptr[1]=FWCBM(p[1], p[2], (int*)FB_BUFBEG, p[3]);
*/
outptr[1]=FWCBM(p[1], p[2], CPY_TO_FBBUF(0, p+4, p[3]), p[3]);
outptr[0]=sscode();
outptr+=2;
return(plOK);
}

plerrcode test_proc_FWCBM(int* p)
{
if (p[0]<3) return(plErr_ArgNum);
if (p[0]!=(p[3]+3)) return(plErr_ArgNum);
return(plOK);
}

static procprop FWCBM_prop={0, 2, 0, 0};

procprop* prop_proc_FWCBM()
{
return(&FWCBM_prop);
}

char name_proc_FWCBM[]="FWCBM";
int ver_proc_FWCBM=2;

/*****************************************************************************/
/*
FRDBM
p[0] : No. of parameters (==3)
p[1] : primary address
p[2] : secondary address
p[3] : count
outptr[0] : SS-code
outptr[1] : count
outptr[2] : values
*/

plerrcode proc_FRDBM(int* p)
{
outptr[1]=FRDBM(p[1], p[2], outptr+2, p[3]);
outptr[0]=sscode();
outptr+=2+outptr[1];
return(plOK);
}

plerrcode test_proc_FRDBM(int* p)
{
if (p[0]!=3) return(plErr_ArgNum);
return(plOK);
}

static procprop FRDBM_prop={1, -1, 0, 0};

procprop* prop_proc_FRDBM()
{
return(&FRDBM_prop);
}

char name_proc_FRDBM[]="FRDBM";
int ver_proc_FRDBM=2;

/*****************************************************************************/
/*
FWDBM
p[0] : No. of parameters (>=3)
p[1] : primary address
p[2] : secondary address
p[3] : count
...  : values
outptr[0] : SS-code
outptr[1] : count
*/

plerrcode proc_FWDBM(int* p)
{
/*
memcpy(FB_BUFBEG, p+4, p[3]*4);
outptr[1]=FWDBM(p[1], p[2], (int*)FB_BUFBEG, p[3]);
*/
outptr[1]=FWDBM(p[1], p[2], CPY_TO_FBBUF(0, p+4, p[3]), p[3]);
outptr[0]=sscode();
outptr+=2;
return(plOK);
}

plerrcode test_proc_FWDBM(int* p)
{
if (p[0]<3) return(plErr_ArgNum);
if (p[0]!=(p[3]+3)) return(plErr_ArgNum);
return(plOK);
}

static procprop FWDBM_prop={0, 2, 0, 0};

procprop* prop_proc_FWDBM()
{
return(&FWDBM_prop);
}

char name_proc_FWDBM[]="FWDBM";
int ver_proc_FWDBM=2;

/*****************************************************************************/
/*****************************************************************************/
