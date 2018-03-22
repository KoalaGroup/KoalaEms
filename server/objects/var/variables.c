/*
 * objects/var/variables.c
 * created 22.09.93
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: variables.c,v 1.18 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rcs_ids.h>
#include "variables.h"
#include <errorcodes.h>
#include "varobj.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

RCS_REGISTER(cvsid, "objects/var")

/*****************************************************************************/

extern ems_u32* outptr;
struct Var var_list[MAX_VAR];

/*****************************************************************************/

errcode var_init(void)
{
int i;

T(var_init)
for (i=0; i<MAX_VAR; i++) var_list[i].len=0;
return(OK);
}

/*****************************************************************************/

errcode var_done(void)
{
int i;

T(var_done)
for (i=0; i<MAX_VAR; i++)
  {
  if (var_list[i].len>1) free((void*)var_list[i].var.ptr);
  var_list[i].len=0;
  }
return(OK);
}

/*****************************************************************************/
/*
CreateVariable
p[0] : ID
p[1] : size of var (in terms of integers)
*/
errcode CreateVariable(ems_u32* p, unsigned int num)
{
ems_u32 *i;
unsigned int idx, size;

T(CreateVariable)
D(D_REQ, printf("CreateVariable\n");)

if (num!=2) return(Err_ArgNum);

D(D_REQ, printf("  index: %d\n", p[0]);)
D(D_REQ, printf("  size: %d\n", p[1]);)
idx=p[0];
size=p[1];

if (idx>=MAX_VAR) return(Err_IllVar);

if (size<1) return(Err_ArgRange);
if (var_list[idx].len) return(Err_VarDef);
if (size==1) {var_list[idx].var.val=0; var_list[idx].len=1; return(OK);}
if (!(i=(ems_u32*)calloc(size, sizeof(ems_u32)))) return(Err_NoMem);
var_list[idx].len=size;
var_list[idx].var.ptr=i;
while (size--) *(i++)=0;
return(OK);
}

/*****************************************************************************/
/*
DeleteVariable
p[0] : ID
*/
errcode DeleteVariable(ems_u32* p, unsigned int num)
{
register unsigned int idx;

T(DeleteVariable)
D(D_REQ, printf("DeleteVariable\n");)
if (num!=1) return(Err_ArgNum);
D(D_REQ, printf("  index: %d\n", p[0]);)
idx=p[0];
if (idx>=MAX_VAR) return(Err_IllVar);
if (!var_list[idx].len) return(Err_NoVar);
if (var_list[idx].len>1) free((void*)var_list[idx].var.ptr);
var_list[idx].len=0;
return(OK);
}

/*****************************************************************************/
/*
ReadVariable
p[0] : ID
*/
errcode ReadVariable(ems_u32* p, unsigned int num)
{
ems_u32 *i;
unsigned int idx, size;

T(ReadVariable)
D(D_REQ, printf("ReadVariable\n");)
if (num!=1) return(Err_ArgNum);
D(D_REQ, printf("  index: %d\n", p[0]);)
idx=p[0];
if (idx>=MAX_VAR) return(Err_IllVar);
if (!(size=var_list[idx].len)) return(Err_NoVar);
if ((*outptr++=size)==1) {*outptr++=var_list[idx].var.val; return(OK);}
i=var_list[idx].var.ptr;
while (size--) *outptr++= *i++;
return(OK);
}

/*****************************************************************************/
/*
WriteVariable
p[0] : ID
p[1] : number of integers
p[2] : data
...
*/
errcode WriteVariable(ems_u32* p, unsigned int num)
{
ems_u32 *i;
unsigned int idx;
unsigned int size;

T(WriteVariable)
D(D_REQ, printf("WriteVariable");)
if ((num<2) || (num!=p[1]+2)) return(Err_ArgNum);
D(D_REQ, printf("  index: %d\n", p[0]);)
D(D_REQ, printf("  Anzahl: %d\n", p[1]);)
D(D_REQ, unsigned int j; for (j=0; j<p[1]; j++) printf("  %d\n", p[j+2]);)
idx=p[0];
size=p[1];
if (idx>=MAX_VAR) return(Err_IllVar);
if (var_list[idx].len==0) return(Err_NoVar);
if (var_list[idx].len!=size) return(Err_IllVarSize);
if (size==1) {var_list[idx].var.val=p[2]; return(OK);}
i=var_list[idx].var.ptr;
p+=2;
while (size--) *i++= *p++;
return(OK);
}

/*****************************************************************************/
/*
GetVariableAttributes
p[0] : ID
*/
errcode GetVariableAttributes(ems_u32* p, unsigned int num)
{
T(GetVariableAttributes)
D(D_REQ, printf("GetVariableAttributes\n");)
if (num!=1) return(Err_ArgNum);
D(D_REQ, printf("  index: %d\n", p[0]);)
if (*p>=MAX_VAR) return(Err_IllVar);
if (var_list[*p].len==0) return(Err_NoVar);
*outptr++=var_list[*p].len;
return(OK);
}

/*****************************************************************************/

plerrcode var_create(unsigned int index, unsigned int size)
{
ems_u32 *i;

T(var_create)
if (index>=MAX_VAR) return(plErr_IllVar);
if (size<1) return(plErr_ArgRange);
if (var_list[index].len) return(plErr_VarDef);
if (size==1) {var_list[index].var.val=0; var_list[index].len=1; return(OK);}
if (!(i=(ems_u32*)malloc(size*sizeof(ems_u32)))) return(plErr_NoMem);
var_list[index].len=size;
var_list[index].var.ptr=i;
while (size--) *(i++)=0;
return(OK);
}

/*****************************************************************************/

plerrcode var_delete(unsigned int index)
{

T(var_delete)
if (index>=MAX_VAR) return(plErr_IllVar);
if (!var_list[index].len) return(plErr_NoVar);
if (var_list[index].len>1) free((void*)var_list[index].var.ptr);
var_list[index].len=0;
return(OK);
}

/*****************************************************************************/
plerrcode var_clear(unsigned int index)
{
    struct Var *var;

    T(var_clear)
    if (index>=MAX_VAR)
        return plErr_IllVar;
    if (!var_list[index].len)
        return plErr_NoVar;
    var=var_list+index;
    if (var->len>1)
        memset(var->var.ptr, 0, var->len*sizeof(ems_u32));
    else
        var->var.val=0;
    return plOK;
}
/*****************************************************************************/

plerrcode var_read_int(unsigned int index, ems_u32* val)
{
    T(var_read_int)
    if (index>=MAX_VAR) return plErr_IllVar;
    if (!var_list[index].len) return plErr_NoVar;
    if (var_list[index].len!=1) return plErr_IllVarSize;
    *val=var_list[index].var.val;
    return plOK;
}

/*****************************************************************************/

plerrcode var_write_int(unsigned int index, ems_u32 val)
{
register int size;

T(var_write_int)
if (index>=MAX_VAR) return(plErr_IllVar);
if ((size=var_list[index].len)==0) return(plErr_NoVar);
if (size!=1) return(plErr_IllVarSize);
var_list[index].var.val=val;
return plOK;
}

/*****************************************************************************/

plerrcode var_get_ptr(unsigned int index, ems_u32** ptr)
{
register int size;

T(var_get_ptr)
if (index>=MAX_VAR) return plErr_IllVar;
if ((size=var_list[index].len)==0)
  return plErr_NoVar;
else if (size==1)
  *ptr=&var_list[index].var.val;
else
  *ptr=var_list[index].var.ptr;
return plOK;
}

/*****************************************************************************/

plerrcode var_attrib(unsigned int index, unsigned int* size)
{
T(var_attrib)
if (index>=MAX_VAR) return(plErr_IllVar);
if (var_list[index].len==0) return(plErr_NoVar);
*size=var_list[index].len;
return(plOK);
}

/*****************************************************************************/

objectcommon* lookup_var(ems_u32* id, unsigned int idlen, unsigned int* remlen)
{
/*
 * int i;
 * printf("lookup_var({");
 * for (i=0; i<idlen; i++)
 *   printf("%d%s", id[i], i+1<idlen?", ":"");
 * printf("}, %d, %d)\n", idlen, remlen);
 */

if (idlen>0)
  {
  if (id[0]<MAX_VAR)
    {
    *remlen=idlen;
    return(&var_obj);
    }
  else
    return(0);
  }
else
  {
  *remlen=0;
  return(&var_obj);
  }
}

/*****************************************************************************/

ems_u32* dir_var(ems_u32* ptr)
{
int idx;

for (idx=0; idx<MAX_VAR; idx++)
  if (var_list[idx].len) *ptr++=idx;
return(ptr);
}

/*****************************************************************************/

objectcommon var_obj={
  0,
  0,
  lookup_var,
  dir_var,
  0
};

/*****************************************************************************/
/*****************************************************************************/
