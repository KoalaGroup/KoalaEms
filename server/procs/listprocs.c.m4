/*
 * procs/listprocs.c.m4
 * 06.11.94      
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: listprocs.c.m4,v 1.13 2011/04/06 20:30:29 wuestner Exp $";

#include "listprocs.h"
#include "procs.h"
#include <sconf.h>
#include <debug.h>
#include <xdrstring.h>
#include "rcs_ids.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs")

/*****************************************************************************/

dnl define(`proc',`extern plerrcode proc_$1(int*);
dnl extern plerrcode test_proc_$1(int*);
dnl #ifdef PROCPROPS
dnl extern procprop* prop_proc_$1(void);
dnl #endif
dnl extern char name_proc_$1[];
dnl extern int ver_proc_$1;')
dnl include(procedures)

/*****************************************************************************/

define(`proc', `{proc_$1, test_proc_$1,
  name_proc_$1, &ver_proc_$1},')
listproc Proc[]=
{
include(procedures)
};

#ifdef PROCPROPS
define(`proc', `{prop_proc_$1},')
listprop Prop[]=
{
include(procedures)
};
#endif

int NrOfProcs=sizeof(Proc)/sizeof(listproc);

/*****************************************************************************/
/*
getlistproclist
p[0] : Capabtyp (==Capab_listproc)
*/
errcode getlistproclist(ems_u32* p, unsigned int num)
{
register int i;

T(getlistproclist)
D(D_REQ, printf("GetCapabilityList(Capab_listproc)\n");)
if (num!=1) return(Err_ArgNum);
*outptr++=NrOfProcs;
for (i=0; i<NrOfProcs; i++)
  {
  *outptr++=i;
  outptr=outstring(outptr, Proc[i].name_proc);
  *outptr++= *(Proc[i].ver_proc);
  }
return(OK);
}

/*****************************************************************************/

#ifdef PROCPROPS
static procprop dummyprop={
    -1, -1,
    "*", "keine Erklaerung"
};
#endif

/*
getlistprop
p[0] : level (0: nur Laengen, 1: Syntax, 2: Text, ...?)
p[1] : no. of procs
.... : procIDs
*/
errcode getlistprocprop(ems_u32* p, unsigned int num)
{
int i;

T(getlistprocprop)
for (i=0; i<p[1]; i++)
  {
  if ((unsigned int)p[i+2]>=NrOfProcs)
    {
    *outptr++=p[i+2];
    return(Err_ArgRange);
    }
  }
#ifdef PROCPROPS
*outptr++=p[0];
*outptr++=p[1];
for (i=0; i<p[1]; i++)
  {
  int proc_id;
  procprop* prop;

  proc_id=p[i+2];
  *outptr++=proc_id;
  prop=Prop[proc_id].prop_proc();
  if(!prop){ /* erzeuge Dummy */
      prop= &dummyprop;
  }
  *outptr++=prop->varlength;  /* (0: constant, 1: variable) */
  *outptr++=prop->maxlength;
  if (p[0]>0)
    {
    if (prop->syntax)
      outptr=outstring(outptr, prop->syntax);
    else
      outptr=outstring(outptr, "");
    }
  if (p[0]>1)
    {
    if (prop->text)
      outptr=outstring(outptr, prop->text);
    else
      outptr=outstring(outptr, "");
    }
  }
return(OK);
#else
return(Err_NotImpl);
#endif
}

/*****************************************************************************/
/*****************************************************************************/
