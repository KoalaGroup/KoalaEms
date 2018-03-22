/*
 * procs/wincc/wincc.c
 * 09.02.1999
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: wincc_rw.c,v 1.5 2017/10/09 21:25:38 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../procs.h"
#include <xdrstring.h>
#include <xdrfloat.h>
#include "../../lowlevel/wincc/wincc.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/wincc")

/*****************************************************************************/
/*
 * p[0]= strxdrlen(hostname)+1
 * p[1]: hostname
 * p[x]: port
 * outptr[0]: path_id
 */
plerrcode proc_wcc_open(int* p)
{
plerrcode res;
char* hostname;
int *pp;
pp=xdrstrcdup(&hostname, p+1);
if ((res=wincc_open(outptr, hostname, *pp))==plOK) outptr++;
free(hostname);
return res;
}

plerrcode test_proc_wcc_open(int* p)
{
if (p[0]<2) return plErr_ArgNum;
if (p[0]!=xdrstrlen(p+1)+1) return plErr_ArgNum;
return plOK;
}

char name_proc_wcc_open[]="WCC_open";
int ver_proc_wcc_open=1;
/*****************************************************************************/
/*
 * p[0]: length (==1)
 * p[1]: path_id
 */
plerrcode proc_wcc_close(int* p)
{
return wincc_close(p[1]);
}

plerrcode test_proc_wcc_close(int* p)
{
if (p[0]!=1) return plErr_ArgNum;
return plOK;
}

char name_proc_wcc_close[]="WCC_close";
int ver_proc_wcc_close=1;
/*****************************************************************************/
static const int* extract_wcc_type(wincc_types* typ, const int* ptr, int rest,
    plerrcode* res)
{
char* styp;

*res=plOK;
if (*ptr==0)
  {
  ptr++;
  *typ=*ptr++;
  return ptr;
  }

if ((unsigned int)*ptr>1024)
  {
  printf("proc_wcc_r: typestring too large (%d bytes)\n", *ptr);
  *res=plErr_ArgRange;
  return (int*)0;
  }
ptr=xdrstrcdup(&styp, ptr);
if (!strcasecmp(styp, "bit"))
  *typ=WCC_TBIT;
else if (!strcasecmp(styp, "byte"))
  *typ=WCC_TBYTE;
else if (!strcasecmp(styp, "char"))
  *typ=WCC_TCHAR;
else if (!strcasecmp(styp, "double"))
  *typ=WCC_TDOUBLE;
else if (!strcasecmp(styp, "float"))
  *typ=WCC_TFLOAT;
else if (!strcasecmp(styp, "word"))
  *typ=WCC_TWORD;
else if (!strcasecmp(styp, "dword"))
  *typ=WCC_TDWORD;
else if (!strcasecmp(styp, "raw"))
  *typ=WCC_TRAW;
else if (!strcasecmp(styp, "sbyte"))
  *typ=WCC_TSBYTE;
else if (!strcasecmp(styp, "sword"))
  *typ=WCC_TSWORD;
else
  {
  printf("proc_wcc_r: type >%s< is not known\n", styp);
  *res=plErr_ArgRange;
  }
free(styp);
/*
 * if (res==plOK)
 *   printf(">%s< converted to type %d\n", styp, *typ);
 * else
 *   printf(">%s< not converted\n", styp);
 */
return ptr;
}
/*****************************************************************************/
static void clear_varspec(wincc_var_spec* spec, int num)
{
int i;
for (i=0; i<num; i++)
  {
  free(spec[i].name);
  switch (spec[i].value.typ)
    {
    case WCC_TCHAR: free(spec[i].value.val.str); break;
    case WCC_TRAW: free(spec[i].value.val.opaque.vals); break;
    }
  }
}
/*****************************************************************************/
/*
 * p[0]: length
 * p[1]: path
 * p[2]: name
 * p[n...]: type as string or 0
 * p[n+1] type as int if p[n]==0
 * outptr[0]: length
 * outptr[1]: result code; 0: success, -1: timeout, -2: pad path
 * only if result code==0:
 *   outptr[2]: type
 *   outptr[3...]: value
 */
plerrcode proc_wcc_r(int* p)
{
const int *ihelp;
int *ohelp, rest;
plerrcode pres=plOK;
wincc_var_spec var_spec;
wincc_errors wres;

/*
 * {
 * int i;
 * printf("proc_wcc_r:\n");
 * for (i=0; i<=p[0]; i++)
 *   {
 *   printf("[%03d] %010d\n", i, p[i]);
 *   }
 * }
 */
ohelp=outptr++;
ihelp=p+2;

rest=p+p[0]-ihelp+1;
if ((rest<1) || (rest<xdrstrlen(ihelp)))
  {
  printf("proc_wcc_r: confused by wrong arguments.\n");
  return plErr_ArgNum;
  }
ihelp=xdrstrcdup(&var_spec.name, ihelp);
rest=p+p[0]-ihelp+1;
ihelp=extract_wcc_type(&var_spec.value.typ, ihelp, rest, &pres);
if (pres!=plOK)
  {
  free(var_spec.name);
  return pres;
  }

var_spec.operation=WCC_RD;
var_spec.error=WCC_V_OK;
bzero(&var_spec.value, sizeof(wincc_value));

wres=wincc_var(p[1], 1/*number of vars*/, &var_spec, 0/* not async*/);

*outptr++=wres;

switch (wres)
  {
  case WCC_USER_ERR:
    pres=plErr_Other;
    break;
  case WCC_NO_PATH:
    pres=plErr_ArgRange;
    break;
  case WCC_SYSTEM:
    pres=plErr_System;
    break;
  case WCC_OK:
    {
    *outptr++=var_spec.error;
    if (var_spec.error==WCC_V_OK)
    switch (var_spec.value.typ)
      {
      case WCC_TBIT:
      case WCC_TBYTE:
      case WCC_TSBYTE:
      case WCC_TWORD:
      case WCC_TSWORD:
      case WCC_TDWORD:
        *outptr++=var_spec.value.val.i;
        break;
      case WCC_TCHAR:
        outptr=outstring(outptr, var_spec.value.val.str);
        break;
      case WCC_TDOUBLE:
        outptr=outdouble(outptr, var_spec.value.val.d);
        break;
      case WCC_TFLOAT:
        outptr=outfloat(outptr, var_spec.value.val.f);
        break;
      case WCC_TRAW:
        {
        outptr=outnstring(outptr, var_spec.value.val.opaque.vals,
            var_spec.value.val.opaque.size);
        }
        break;
      }
    
    }
  }
*ohelp=outptr-ohelp-1;
clear_varspec(&var_spec, 1);
return pres;
}

plerrcode test_proc_wcc_r(int* p)
{
if (p[0]<1) return plErr_ArgNum;
return plOK;
}

char name_proc_wcc_r[]="WCC_R";
int ver_proc_wcc_r=1;
/*****************************************************************************/
/*
 * p[0]: length
 * p[1]: path
 * p[2]: name
 * p[n...]: type as string or 0
 * p[n+1] type as int if p[n]==0
 * p[...]: value(s)
 * outptr[0]: length
 * outptr[1]: result code; 0: success, -1: timeout, -2: pad path
 * only if result code==0:
 *   outptr[2]: type
 *   outptr[3...]: value
 */
plerrcode proc_wcc_w(int* p)
{
const int *ihelp;
int *ohelp, rest;
plerrcode pres=plOK;
wincc_var_spec var_spec;
wincc_errors wres;

{
int i;
printf("###############################################################\n");
printf("proc_wcc_w:\n");
for (i=0; i<=p[0]; i++)
  {
  printf("[%03d] %08x %10d\n", i, p[i], p[i]);
  }
printf("\n");
}
ohelp=outptr++;
ihelp=p+2;

rest=p+p[0]-ihelp+1;
if ((rest<1) || (rest<xdrstrlen(ihelp)))
  {
  printf("proc_wcc_w: confused by wrong arguments (a).\n");
  clear_varspec(&var_spec, 1);
  return plErr_ArgNum;
  }
ihelp=xdrstrcdup(&var_spec.name, ihelp);
rest=p+p[0]-ihelp+1;
ihelp=extract_wcc_type(&var_spec.value.typ, ihelp, rest, &pres);
if (pres!=plOK)
  {
  free(var_spec.name);
  return pres;
  }
if (pres!=plOK) return pres;

var_spec.operation=WCC_WR;
var_spec.error=WCC_V_OK;
rest=p+p[0]-ihelp+1;
if (rest<1)
  {
  printf("proc_wcc_w: confused by wrong arguments (b).\n");
  clear_varspec(&var_spec, 1);
  return plErr_ArgNum;
  }
switch (var_spec.value.typ)
  {
  case WCC_TBIT:   /* nobreak */
  case WCC_TBYTE:  /* nobreak */
  case WCC_TWORD:  /* nobreak */
  case WCC_TDWORD: /* nobreak */
  case WCC_TSBYTE: /* nobreak */
  case WCC_TSWORD:
    var_spec.value.val.i=*ihelp++;
    printf("  value: %d\n", var_spec.value.val.i);
  break;
  case WCC_TFLOAT:
    ihelp=extractfloat(&var_spec.value.val.f, ihelp);
    printf("  value: %f\n", var_spec.value.val.f);
  break;
  case WCC_TCHAR:
    if (rest<xdrstrlen(ihelp))
      {
      printf("proc_wcc_w: confused by wrong arguments (c).\n");
      clear_varspec(&var_spec, 1);
      return plErr_ArgNum;
      }
    ihelp=xdrstrcdup(&var_spec.value.val.str, ihelp);
    printf("  value: >%s<\n", var_spec.value.val.str);
  break;
  case WCC_TDOUBLE:
    if (rest<2)
      {
      printf("proc_wcc_rw: confused by wrong arguments (d).\n");
      clear_varspec(&var_spec, 1);
      return plErr_ArgNum;
      }
    ihelp=extractdouble(&var_spec.value.val.d, ihelp);
    printf("  value: %f\n", var_spec.value.val.d);
  break;
  case WCC_TRAW:
    if (rest<xdrstrlen(ihelp))
      {
      printf("proc_wcc_w: confused by wrong arguments (e).\n");
      clear_varspec(&var_spec, 1);
      return plErr_ArgNum;
      }
    var_spec.value.val.opaque.size=*ihelp;
    ihelp=xdrstrcdup(&var_spec.value.val.opaque.vals, ihelp);
  break;
  }

wres=wincc_var(p[1], 1/*number of vars*/, &var_spec, 0/* not async*/);

*outptr++=wres;

switch (wres)
  {
  case WCC_USER_ERR:
    pres=plErr_Other;
    break;
  case WCC_NO_PATH:
    pres=plErr_ArgRange;
    break;
  case WCC_SYSTEM:
    pres=plErr_System;
    break;
  case WCC_OK:
    {
    *outptr++=var_spec.error;
    if (var_spec.error==WCC_V_OK)
    switch (var_spec.value.typ)
      {
      case WCC_TBIT:
      case WCC_TBYTE:
      case WCC_TSBYTE:
      case WCC_TWORD:
      case WCC_TSWORD:
      case WCC_TDWORD:
        *outptr++=var_spec.value.val.i;
        break;
      case WCC_TCHAR:
        outptr=outstring(outptr, var_spec.value.val.str);
        break;
      case WCC_TDOUBLE:
        outptr=outdouble(outptr, var_spec.value.val.d);
        break;
      case WCC_TFLOAT:
        outptr=outfloat(outptr, var_spec.value.val.f);
        break;
      case WCC_TRAW:
        {
        outptr=outnstring(outptr, var_spec.value.val.opaque.vals,
            var_spec.value.val.opaque.size);
        }
        break;
      }
    
    }
  }
*ohelp=outptr-ohelp-1;
clear_varspec(&var_spec, 1);
return pres;
}

plerrcode test_proc_wcc_w(int* p)
{
return plOK;
}

char name_proc_wcc_w[]="WCC_W";
int ver_proc_wcc_w=1;
/*****************************************************************************/
/*
 * p[0]: length
 * p[1]: path
- beliebig oft wiederholt:
 * p[2]: name
 * p[3]: 0: read, 1: write
 * p[n...]: type as string or 0
 * p[n+1] type as int if p[n]==0
 * if p[3]: p[...]: value(s)]
-
 * outptr[0]: length
 * outptr[1]: result code; 0: success, -1: timeout, -2: pad path
 * only if result code==0:
 *   outptr[2]: type
 *   outptr[3...]: value
 */
plerrcode proc_wcc_rw(int* p)
{
const int *ihelp;
int *ohelp, vnum, rest;
plerrcode pres=plOK;
wincc_var_spec var_spec[64];
wincc_errors wres;

{
int i;
printf("proc_wcc_rw:\n");
for (i=0; i<=p[0]; i++)
  {
  printf("[%03d] %010d\n", i, p[i]);
  }
}

ohelp=outptr++;
ihelp=p+2;
vnum=0;

while (ihelp-p<=p[0])
  {
  if (vnum>=64)
    {
    printf("proc_wcc_rw: only 64 variables allowed.\n");
    clear_varspec(var_spec, vnum);
    return plErr_ArgNum;
    }
  rest=p+p[0]-ihelp+1;
  if ((rest<1) || (rest<xdrstrlen(ihelp)))
    {
    printf("proc_wcc_rw: confused by wrong arguments (a).\n");
    clear_varspec(var_spec, vnum);
    return plErr_ArgNum;
    }
  ihelp=xdrstrcdup(&var_spec[vnum].name, ihelp);
  rest=p+p[0]-ihelp+1;
  if (rest<1)
    {
    printf("proc_wcc_rw: confused by wrong arguments (b).\n");
    clear_varspec(var_spec, vnum);
    return plErr_ArgNum;
    }
  var_spec[vnum].operation=*ihelp++?WCC_WR:WCC_RD;
  rest=p+p[0]-ihelp+1;
  ihelp=extract_wcc_type(&var_spec[vnum].value.typ, ihelp, rest, &pres);
  if (pres!=plOK)
    {
    free(var_spec[vnum].name);
    clear_varspec(var_spec, vnum);
    return pres;
    }

  var_spec[vnum].error=WCC_V_OK;
  rest=p+p[0]-ihelp+1;
  if (rest<1)
    {
    printf("proc_wcc_rw: confused by wrong arguments (c).\n");
    clear_varspec(var_spec, vnum);
    return plErr_ArgNum;
    }
  switch (var_spec[vnum].value.typ)
    {
    case WCC_TBIT:   /* nobreak */
    case WCC_TBYTE:  /* nobreak */
    case WCC_TWORD:  /* nobreak */
    case WCC_TDWORD: /* nobreak */
    case WCC_TSBYTE: /* nobreak */
    case WCC_TSWORD:
      var_spec[vnum].value.val.i=*ihelp++;
      printf("  value: %d\n", var_spec[vnum].value.val.i);
    break;
    case WCC_TFLOAT:
      ihelp=extractfloat(&var_spec[vnum].value.val.f, ihelp);
      printf("  value: %f\n", var_spec[vnum].value.val.f);
    break;
    case WCC_TCHAR:
      if (rest<xdrstrlen(ihelp))
        {
        printf("proc_wcc_rw: confused by wrong arguments (d).\n");
        clear_varspec(var_spec, vnum);
        return plErr_ArgNum;
        }
      ihelp=xdrstrcdup(&var_spec[vnum].value.val.str, ihelp);
      printf("  value: >%s<\n", var_spec[vnum].value.val.str);
    break;
    case WCC_TDOUBLE:
      if (rest<2)
        {
        printf("proc_wcc_rw: confused by wrong arguments (e).\n");
        clear_varspec(var_spec, vnum);
        return plErr_ArgNum;
        }
      ihelp=extractdouble(&var_spec[vnum].value.val.d, ihelp);
      printf("  value: %f\n", var_spec[vnum].value.val.d);
    break;
    case WCC_TRAW:
      if (rest<xdrstrlen(ihelp))
        {
        printf("proc_wcc_rw: confused by wrong arguments (f).\n");
        clear_varspec(var_spec, vnum);
        return plErr_ArgNum;
        }
      var_spec[vnum].value.val.opaque.size=*ihelp;
      ihelp=xdrstrcdup(&var_spec[vnum].value.val.opaque.vals, ihelp);
    break;
    }
  vnum++;
  }

wres=wincc_var(p[1], vnum, var_spec, 0/* not async*/);

*outptr++=wres;
*outptr++=vnum;

switch (wres)
  {
  case WCC_USER_ERR:
    pres=plErr_Other;
    break;
  case WCC_NO_PATH:
    pres=plErr_ArgRange;
    break;
  case WCC_SYSTEM:
    pres=plErr_System;
    break;
  case WCC_OK:
    {
    int i;
    for (i=0; i<vnum; i++)
      {
      *outptr++=var_spec[i].error;
      if (var_spec[i].error==WCC_V_OK)
      switch (var_spec[i].value.typ)
        {
        case WCC_TBIT:
        case WCC_TBYTE:
        case WCC_TSBYTE:
        case WCC_TWORD:
        case WCC_TSWORD:
        case WCC_TDWORD:
          *outptr++=var_spec[i].value.val.i;
          break;
        case WCC_TCHAR:
          outptr=outstring(outptr, var_spec[i].value.val.str);
          break;
        case WCC_TDOUBLE:
          outptr=outdouble(outptr, var_spec[i].value.val.d);
          break;
        case WCC_TFLOAT:
          outptr=outfloat(outptr, var_spec[i].value.val.f);
          break;
        case WCC_TRAW:
          {
          outptr=outnstring(outptr, var_spec[i].value.val.opaque.vals,
              var_spec[i].value.val.opaque.size);
          }
          break;
        } /* switch (var_spec.typ) */
      } /* for */
    }
    break; /* WCC_OK */
  }
*ohelp=outptr-ohelp-1;
clear_varspec(var_spec, vnum);
return pres;
}

plerrcode test_proc_wcc_rw(int* p)
{
return plOK;
}

char name_proc_wcc_rw[]="WCC_RW";
int ver_proc_wcc_rw=1;
/*****************************************************************************/
/*****************************************************************************/
