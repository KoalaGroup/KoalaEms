/******************************************************************************
*                                                                             *
* cycle.c                                                                     *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* created: 21.10.94                                                           *
* last changed: 04.11.94                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: cycle.c,v 1.3 2011/04/06 20:30:31 wuestner Exp $";

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
FCPD
p[0] : No. of parameters (==1)
p[1] : primary address
*/

plerrcode proc_FCPD(int* p)
{
FCPD_(p[1]);
return(plOK);
}

plerrcode test_proc_FCPD(int* p)
{
if (p[0]!=1) return(plErr_ArgNum);
return(plOK);
}

static procprop FCPD_prop={0, 0, 0, 0};

procprop* prop_proc_FCPD()
{
return(&FCPD_prop);
}

char name_proc_FCPD[]="FCPD";
int ver_proc_FCPD=1;

/*****************************************************************************/
/*
FCPC
p[0] : No. of parameters (==1)
p[1] : primary address
*/

plerrcode proc_FCPC(int* p)
{
FCPC_(p[1]);
return(plOK);
}

plerrcode test_proc_FCPC(int* p)
{
if (p[0]!=1) return(plErr_ArgNum);
return(plOK);
}

static procprop FCPC_prop={0, 0, 0, 0};

procprop* prop_proc_FCPC()
{
return(&FCPC_prop);
}

char name_proc_FCPC[]="FCPC";
int ver_proc_FCPC=1;

/*****************************************************************************/
/*
FCPD_2
p[0] : No. of parameters (==1)
p[1] : primary address
*/

plerrcode proc_FCPD_2(int* p)
{
FCPD(p[1]);
return(plOK);
}

plerrcode test_proc_FCPD_2(int* p)
{
if (p[0]!=1) return(plErr_ArgNum);
return(plOK);
}

static procprop FCPD_2_prop={0, 0, 0, 0};

procprop* prop_proc_FCPD_2()
{
return(&FCPD_2_prop);
}

char name_proc_FCPD_2[]="FCPD";
int ver_proc_FCPD_2=2;

/*****************************************************************************/
/*
FCPC_2
p[0] : No. of parameters (==1)
p[1] : primary address
*/

plerrcode proc_FCPC_2(int* p)
{
FCPC(p[1]);
return(plOK);
}

plerrcode test_proc_FCPC_2(int* p)
{
if (p[0]!=1) return(plErr_ArgNum);
return(plOK);
}

static procprop FCPC_2_prop={0, 0, 0, 0};

procprop* prop_proc_FCPC_2()
{
return(&FCPC_2_prop);
}

char name_proc_FCPC_2[]="FCPC";
int ver_proc_FCPC_2=2;

/*****************************************************************************/
/*
FCPDM
p[0] : No. of parameters (==1)
p[1] : primary address
*/

plerrcode proc_FCPDM(int* p)
{
FCPD(p[1]);
return(plOK);
}

plerrcode test_proc_FCPDM(int* p)
{
if (p[0]!=1) return(plErr_ArgNum);
return(plOK);
}

static procprop FCPDM_prop={0, 0, 0, 0};

procprop* prop_proc_FCPDM()
{
return(&FCPDM_prop);
}

char name_proc_FCPDM[]="FCPDM";
int ver_proc_FCPDM=1;

/*****************************************************************************/
/*
FCPCM
p[0] : No. of parameters (==1)
p[1] : primary address
*/

plerrcode proc_FCPCM(int* p)
{
FCPC(p[1]);
return(plOK);
}

plerrcode test_proc_FCPCM(int* p)
{
if (p[0]!=1) return(plErr_ArgNum);
return(plOK);
}

static procprop FCPCM_prop={0, 0, 0, 0};

procprop* prop_proc_FCPCM()
{
return(&FCPCM_prop);
}

char name_proc_FCPCM[]="FCPCM";
int ver_proc_FCPCM=1;

/*****************************************************************************/
/*
FCDISC
p[0] : No. of parameters (==0)
*/

plerrcode proc_FCDISC(int* p)
{
FCDISC();
return(plOK);
}

plerrcode test_proc_FCDISC(int* p)
{
if (p[0]!=0) return(plErr_ArgNum);
return(plOK);
}

static procprop FCDISC_prop={0, 0, 0, 0};

procprop* prop_proc_FCDISC()
{
return(&FCDISC_prop);
}

char name_proc_FCDISC[]="FCDISC";
int ver_proc_FCDISC=1;

/*****************************************************************************/
/*
FCRW
p[0] : No. of parameters (==0)
*/

plerrcode proc_FCRW(int* p)
{
outptr[1]=FCRW();
*outptr=sscode();
outptr+=2;
return(plOK);
}

plerrcode test_proc_FCRW(int* p)
{
if (p[0]!=0) return(plErr_ArgNum);
return(plOK);
}

static procprop FCRW_prop={0, 2, 0, 0};

procprop* prop_proc_FCRW()
{
return(&FCRW_prop);
}

char name_proc_FCRW[]="FCRW";
int ver_proc_FCRW=1;

/*****************************************************************************/
/*
FCWW
p[0] : No. of parameters (==1)
p[1] : word
*/

plerrcode proc_FCWW(int* p)
{
FCWW(p[1]);
*outptr++=sscode();
return(plOK);
}

plerrcode test_proc_FCWW(int* p)
{
if (p[0]!=1) return(plErr_ArgNum);
return(plOK);
}

static procprop FCWW_prop={0, 1, 0, 0};

procprop* prop_proc_FCWW()
{
return(&FCWW_prop);
}

char name_proc_FCWW[]="FCWW";
int ver_proc_FCWW=1;

/*****************************************************************************/
/*
FCRSA
p[0] : No. of parameters (==0)
*/

plerrcode proc_FCRSA(int* p)
{
outptr[1]=FCRSA();
*outptr=sscode();
outptr+=2;
return(plOK);
}

plerrcode test_proc_FCRSA(int* p)
{
if (p[0]!=0) return(plErr_ArgNum);
return(plOK);
}

static procprop FCRSA_prop={0, 2, 0, 0};

procprop* prop_proc_FCRSA()
{
return(&FCRSA_prop);
}

char name_proc_FCRSA[]="FCRSA";
int ver_proc_FCRSA=1;

/*****************************************************************************/
/*
FCWSA
p[0] : No. of parameters (==1)
p[1] : secondary address
*/

plerrcode proc_FCWSA(int* p)
{
FCWSA(p[1]);
*outptr++=sscode();
return(plOK);
}

plerrcode test_proc_FCWSA(int* p)
{
if (p[0]!=1) return(plErr_ArgNum);
return(plOK);
}

static procprop FCWSA_prop={0, 1, 0, 0};

procprop* prop_proc_FCWSA()
{
return(&FCWSA_prop);
}

char name_proc_FCWSA[]="FCWSA";
int ver_proc_FCWSA=1;

/*****************************************************************************/
/*
FCRWss
p[0] : No. of parameters (==0)
*/

plerrcode proc_FCRWss(int* p)
{
*outptr++=FCRWss();
return(plOK);
}

plerrcode test_proc_FCRWss(int* p)
{
if (p[0]!=0) return(plErr_ArgNum);
return(plOK);
}

static procprop FCRWss_prop={0, 1, 0, 0};

procprop* prop_proc_FCRWss()
{
return(&FCRWss_prop);
}

char name_proc_FCRWss[]="FCRWss";
int ver_proc_FCRWss=1;

/*****************************************************************************/
/*
FCRWS
p[0] : No. of parameters (==1)
p[1] : secondary address
*/

plerrcode proc_FCRWS(int* p)
{
outptr[1]=FCRWS(p[1]);
*outptr=sscode();
outptr+=2;
return(plOK);
}

plerrcode test_proc_FCRWS(int* p)
{
if (p[0]!=1) return(plErr_ArgNum);
return(plOK);
}

static procprop FCRWS_prop={0, 2, 0, 0};

procprop* prop_proc_FCRWS()
{
return(&FCRWS_prop);
}

char name_proc_FCRWS[]="FCRWS";
int ver_proc_FCRWS=1;

/*****************************************************************************/
/*
FCWWss
p[0] : No. of parameters (==1)
p[1] : word
*/

plerrcode proc_FCWWss(int* p)
{
*outptr++=FCWWss(p[1]);
return(plOK);
}

plerrcode test_proc_FCWWss(int* p)
{
if (p[0]!=1) return(plErr_ArgNum);
return(plOK);
}

static procprop FCWWss_prop={0, 1, 0, 0};

procprop* prop_proc_FCWWss()
{
return(&FCWWss_prop);
}

char name_proc_FCWWss[]="FCWWss";
int ver_proc_FCWWss=1;

/*****************************************************************************/
/*
FCWWS
p[0] : No. of parameters (==2)
p[1] : secondary address
p[2] : word
*/

plerrcode proc_FCWWS(int* p)
{
FCWWS(p[1], p[2]);
*outptr++=sscode();
return(plOK);
}

plerrcode test_proc_FCWWS(int* p)
{
if (p[0]!=2) return(plErr_ArgNum);
return(plOK);
}

static procprop FCWWS_prop={0, 1, 0, 0};

procprop* prop_proc_FCWWS()
{
return(&FCWWS_prop);
}

char name_proc_FCWWS[]="FCWWS";
int ver_proc_FCWWS=1;

/*****************************************************************************/
/*
FCWSAss
p[0] : No. of parameters (==1)
p[1] : secondary address
*/

plerrcode proc_FCWSAss(int* p)
{
*outptr++=FCWSAss(p[1]);
return(plOK);
}

plerrcode test_proc_FCWSAss(int* p)
{
if (p[0]!=1) return(plErr_ArgNum);
return(plOK);
}

static procprop FCWSAss_prop={0, 1, 0, 0};

procprop* prop_proc_FCWSAss()
{
return(&FCWSAss_prop);
}

char name_proc_FCWSAss[]="FCWSAss";
int ver_proc_FCWSAss=1;

/*****************************************************************************/
/*
FCRB
p[0] : No. of parameters (==1)
p[1] : count
outptr[0] : SS-code
outptr[1] : count
outptr[2] : values
*/

plerrcode proc_FCRB(int* p)
{
outptr[1]=FCRB(outptr+2, p[1]);
outptr[0]=sscode();
outptr+=2+outptr[1];
return(plOK);
}

plerrcode test_proc_FCRB(int* p)
{
if (p[0]!=1) return(plErr_ArgNum);
return(plOK);
}

static procprop FCRB_prop={1, -1, 0, 0};

procprop* prop_proc_FCRB()
{
return(&FCRB_prop);
}

char name_proc_FCRB[]="FCRB";
int ver_proc_FCRB=2;

/*****************************************************************************/
/*
FCWB
p[0] : No. of parameters (>=1)
p[1] : count
...  : values
outptr[0] : SS-code
outptr[1] : count
*/

plerrcode proc_FCWB(int* p)
{
/*
memcpy((int*)FB_BUFBEG, p+2, p[1]*4);
outptr[1]=FCWB((int*)FB_BUFBEG, p[1]);
*/
outptr[1]=FCWB(CPY_TO_FBBUF(0, p+2, p[1]), p[1]);
outptr[0]=sscode();
outptr+=2;
return(plOK);
}

plerrcode test_proc_FCWB(int* p)
{
if (p[0]<1) return(plErr_ArgNum);
if (p[0]!=(p[1]+1)) return(plErr_ArgNum);
return(plOK);
}

static procprop FCWB_prop={1, -1, 0, 0};

procprop* prop_proc_FCWB()
{
return(&FCWB_prop);
}

char name_proc_FCWB[]="FCWB";
int ver_proc_FCWB=2;

/*****************************************************************************/
/*****************************************************************************/
