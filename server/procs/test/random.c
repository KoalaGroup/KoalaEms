/*
 * procs/test/random.c
 * 
 * created 04.11.94 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: random.c,v 1.16 2017/10/25 21:10:01 wuestner Exp $";

#include <unistd.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#ifdef OBJ_VAR
#include "../../objects/var/variables.h"
#endif
#include "../procprops.h"
#include "../procs.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/test")

/*****************************************************************************/

plerrcode proc_WriteVarSize(__attribute__((unused)) ems_u32* p)
{
T(proc_WriteVarSize)
#ifdef OBJ_VAR
{
ems_u32 ui;
ems_i32 i;

    var_read_int(p[1], &ui);
    i=ui;
    if (i>=0) {
#if 1
        for (; i>0; i--)
            *outptr++=i;
#else
        outptr+=i;
#endif
    }
}
#endif
    return plOK;
}

plerrcode test_proc_WriteVarSize(ems_u32* p)
{
T(test_proc_WriteVarSize)
if (p[0]!=1) return(plErr_ArgNum);
#ifndef OBJ_VAR
return(plErr_IllVar);
#else
{
unsigned int size;
plerrcode res;
if ((res=var_attrib(p[1], &size))!=plOK) return(res);
if (size!=1) return(plErr_IllVarSize);
wirbrauchen=var_list[p[1]].var.val;
return(plOK);
}
#endif
}
#ifdef PROCPROPS
static procprop WriteVarSize_prop={1, -1, "<max size>", 0};

procprop* prop_proc_WriteVarSize(void)
{
return(&WriteVarSize_prop);
}
#endif
char name_proc_WriteVarSize[]="WriteVarSize";
int ver_proc_WriteVarSize=1;

/*****************************************************************************/

plerrcode proc_WriteVarRSize(__attribute__((unused)) ems_u32* p)
{
T(proc_WriteVarRSize)
#ifdef OBJ_VAR
{
ems_u32 v, w;
int i;
var_read_int(p[1], &v);
w=v>0?random()%v:0;
*outptr++=w;
for (i=w; i>0; i--) *outptr++=i;
}
#endif
return(plOK);
}

plerrcode test_proc_WriteVarRSize(ems_u32* p)
{
T(test_proc_WriteVarRSize)
if (p[0]!=1) return(plErr_ArgNum);
#ifndef OBJ_VAR
return(plErr_IllVar);
#else
{
unsigned int size;
plerrcode res;
if ((res=var_attrib(p[1], &size))!=plOK) return(res);
if (size!=1) return(plErr_IllVarSize);
srandom(13);
wirbrauchen=var_list[p[1]].var.val;
return(plOK);
}
#endif
}
#ifdef PROCPROPS
static procprop WriteVarRSize_prop={1, -1, "<variable>", 0};

procprop* prop_proc_WriteVarRSize(void)
{
return(&WriteVarRSize_prop);
}
#endif
char name_proc_WriteVarRSize[]="WriteVarRSize";
int ver_proc_WriteVarRSize=1;

/*****************************************************************************/

plerrcode proc_WriteRandSize(ems_u32* p)
{
register int i;

T(proc_WriteRandSize)
D(D_USER, printf("WriteRandSize: %d\n", p[1]);)
if (p[1]) for (i=random()%p[1]; i>0; i--) *outptr++=i;
return(plOK);
}

plerrcode test_proc_WriteRandSize(ems_u32* p)
{
T(test_proc_WriteRandSize)
if (p[0]!=1) return(plErr_ArgNum);
if (p[1]<=0) return(plErr_ArgRange);
srandom(13);
wirbrauchen=p[1];
return(plOK);
}
#ifdef PROCPROPS
static procprop WriteRandSize_prop={1, -1, "<max size>", 0};

procprop* prop_proc_WriteRandSize(void)
{
return(&WriteRandSize_prop);
}
#endif
char name_proc_WriteRandSize[]="WriteRandSize";
int ver_proc_WriteRandSize=1;

/*****************************************************************************/

plerrcode proc_WriteRandom(ems_u32* p)
{
register int i;

T(proc_WriteRandom)
for (i=p[1]; i>0; i--) *outptr++=random();
return(plOK);
}

plerrcode test_proc_WriteRandom(ems_u32* p)
{
T(test_proc_WriteRandom)
if (p[0]!=1) return(plErr_ArgNum);
srandom(13);
wirbrauchen=p[1];
return(plOK);
}
#ifdef PROCPROPS
static procprop WriteRandom_prop={1, -1, "<max size>", 0};

procprop* prop_proc_WriteRandom(void)
{
return(&WriteRandom_prop);
}
#endif
char name_proc_WriteRandom[]="WriteRandom";
int ver_proc_WriteRandom=1;

/*****************************************************************************/

plerrcode proc_WriteVarSizeRandom(__attribute__((unused)) ems_u32* p)
{
T(proc_WriteVarSizeRandom)
#ifdef OBJ_VAR
{
ems_u32 i;
for (var_read_int(p[1], &i); i>0; i--) *outptr++=random();
}
#endif
return(plOK);
}

plerrcode test_proc_WriteVarSizeRandom(ems_u32* p)
{
T(test_proc_WriteVarSizeRandom)
if (p[0]!=1) return(plErr_ArgNum);
#ifndef OBJ_VAR
return(plErr_IllVar);
#else
{
unsigned int size;
plerrcode res;
if ((res=var_attrib(p[1], &size))!=plOK) return(res);
if (size!=1) return(plErr_IllVarSize);
srandom(13);
wirbrauchen=var_list[p[1]].var.val;
return(plOK);
}
#endif
}
#ifdef PROCPROPS
static procprop WriteVarSizeRandom_prop={1, -1, "<variable>", 0};

procprop* prop_proc_WriteVarSizeRandom(void)
{
return(&WriteVarSizeRandom_prop);
}
#endif
char name_proc_WriteVarSizeRandom[]="WriteVarSizeRandom";
int ver_proc_WriteVarSizeRandom=1;

/*****************************************************************************/
plerrcode proc_Stream(__attribute__((unused)) ems_u32* p)
{
#ifdef OBJ_VAR
{
plerrcode res;
ems_u32 *var;
int size;
if ((res=var_get_ptr(p[1], &var))!=plOK) return res;
size=var[0]?random()%var[0]:0;
*outptr++=size;
for (; size; size--) *outptr++=var[1]++;
}
#endif
return plOK;
}
plerrcode test_proc_Stream(ems_u32* p)
{
if (p[0]!=1) return(plErr_ArgNum);
#ifndef OBJ_VAR
return(plErr_IllVar);
#else
{
plerrcode res;
unsigned int size;
if ((res=var_attrib(p[1], &size))!=plOK) return(res);
if (size!=2) return(plErr_IllVarSize);
/* srandom(13); */
wirbrauchen=var_list[p[1]].var.ptr[0];
return plOK;
}
#endif
}
#ifdef PROCPROPS
static procprop Stream_prop={1, -1, "<variable[2]>", 0};
procprop* prop_proc_Stream(void)
{
return(&Stream_prop);
}
#endif
char name_proc_Stream[]="Stream";
int ver_proc_Stream=1; 
/*****************************************************************************/
/*****************************************************************************/
