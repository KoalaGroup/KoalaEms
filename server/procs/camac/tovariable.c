/*
 * procs/camac/tovariable.c
 * created before 07.09.93
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: tovariable.c,v 1.17 2012/09/10 22:42:47 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../lowlevel/camac/camac.h"
#include "../../objects/var/variables.h"
#include "../procs.h"
#include "../procprops.h"
#include "../../objects/domain/dom_ml.h"

extern ems_u32* outptr;
extern int wirbrauchen, *memberlist;

RCS_REGISTER(cvsid, "procs/camac")

/*****************************************************************************/
/*
 * p[0]: 4
 * p[1]: var
 * p[2]: module
 * p[3]: A
 * p[4]: F
 */
plerrcode proc_nAFreadIntVar(ems_u32* p)
{
    ml_entry* m=ModulEnt(p[2]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), p[3], p[4]);
    int res;

    res=dev->CAMACread(dev, &addr, &var_list[p[1]].var.val);
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_nAFreadIntVar(ems_u32* p)
{
if (*p!=4) return(plErr_ArgNum);
if (*++p>MAX_VAR) return(plErr_IllVar);
if (!var_list[*p].len) return(plErr_NoVar);
if (var_list[*p].len!=1) return(plErr_IllVarSize);
if (!valid_module(*++p, modul_camac, 1)) return plErr_ArgRange;
wirbrauchen=0;
return(plOK);
}
#ifdef PROCPROPS
static procprop nAFreadIntVar_prop={0, 0, "var, n, A, F", 0};

procprop* prop_proc_nAFreadIntVar()
{
return(&nAFreadIntVar_prop);
}
#endif
char name_proc_nAFreadIntVar[]="nAFreadIntVar";
int ver_proc_nAFreadIntVar=1;

/*****************************************************************************/
/*
 * p[0]: 4
 * p[1]: module
 * p[2]: A
 * p[3]: F
 * p[4]: var
 */
plerrcode proc_nAFwriteIntVar(ems_u32* p)
{
    ml_entry* m=ModulEnt(p[1]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), p[2], p[3]);
    int res;

    res=dev->CAMACwrite(dev, &addr, var_list[p[4]].var.val);
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_nAFwriteIntVar(ems_u32* p)
{
if (*p!=4) return(plErr_ArgNum);
if (!valid_module(p[1], modul_camac, 1)) return plErr_ArgRange;
p+=4;
if (*p>MAX_VAR) return(plErr_IllVar);
if (!var_list[*p].len) return(plErr_NoVar);
if (var_list[*p].len!=1) return(plErr_IllVarSize);
wirbrauchen=0;
return(plOK);
}
#ifdef PROCPROPS
static procprop nAFwriteIntVar_prop={0, 0, "n, A, F, var", 0};

procprop* prop_proc_nAFwriteIntVar()
{
return(&nAFwriteIntVar_prop);
}
#endif
char name_proc_nAFwriteIntVar[]="nAFwriteIntVar";
int ver_proc_nAFwriteIntVar=1;

/*****************************************************************************/
/*
 * p[0]: 5
 * p[1]: var
 * p[2]: module
 * p[3]: A
 * p[4]: F
 * p[5]: num
 */
plerrcode proc_nAFblreadVar(ems_u32* p)
{
    ems_u32 *var=var_list[p[1]].len==1?
        &var_list[p[1]].var.val:var_list[p[1]].var.ptr;
    int i;
    ml_entry* m=ModulEnt(p[2]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), p[3], p[4]);
    int res;

    res=dev->CAMACblread(dev, &addr, p[5], var);
    /* remove X/Q/inhibit */
    for (i=0; i<p[5]; i++)
        var[i]&=0xffffff;
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_nAFblreadVar(ems_u32* p)
{
    if (p[0]!=5)
        return plErr_ArgNum;
    if (p[1]>MAX_VAR)
        return plErr_IllVar;
    if (!var_list[p[1]].len)
        return plErr_NoVar;
    if (var_list[p[1]].len<p[5])
        return plErr_IllVarSize;
    if (!valid_module(p[2], modul_camac, 1))
        return plErr_ArgRange;
    wirbrauchen=0;
    return plOK;
}
#ifdef PROCPROPS
static procprop nAFblreadVar_prop={0, 0, "var, n, A, F, num", 0};

procprop* prop_proc_nAFblreadVar()
{
return(&nAFblreadVar_prop);
}
#endif
char name_proc_nAFblreadVar[]="nAFblreadVar";
int ver_proc_nAFblreadVar=1;

/*****************************************************************************/
/*
 * p[0]: 5
 * p[1]: var
 * p[2]: module
 * p[3]: A
 * p[4]: F
 * p[5]: num
 */
plerrcode proc_nAFblwriteVar(ems_u32* p)
{
    ems_u32 *var=var_list[p[1]].var.ptr;
    ml_entry* m=ModulEnt(p[2]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), p[3], p[4]);
    int res;

    res=dev->CAMACblwrite(dev, &addr, p[5], var);
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_nAFblwriteVar(ems_u32* p)
{
if (*p!=5) return(plErr_ArgNum);
if (*++p>MAX_VAR) return(plErr_IllVar);
if (!var_list[*p].len) return(plErr_NoVar);
if ((var_list[*p].len!=p[4])||(p[4]<2)) return(plErr_IllVarSize);
if (!valid_module(*++p, modul_camac, 1)) return plErr_ArgRange;
wirbrauchen=0;
return(plOK);
}
#ifdef PROCPROPS
static procprop nAFblwriteVar_prop={0, 0, "var, n, A, F, num", 0};

procprop* prop_proc_nAFblwriteVar()
{
return(&nAFblreadVar_prop);
}
#endif
char name_proc_nAFblwriteVar[]="nAFblwriteVar";
int ver_proc_nAFblwriteVar=1;

/*****************************************************************************/
/*
 * p[0]: 5
 * p[1]: var
 * p[2]: module
 * p[3]: A
 * p[4]: F
 * p[5]: num
 */
plerrcode proc_nAFblreadAddVar(ems_u32* p)
{
    ems_u32 *var=var_list[p[1]].len==1?
        &var_list[p[1]].var.val:var_list[p[1]].var.ptr;
    ems_u32 *tmp=outptr;
    ml_entry* m=ModulEnt(p[2]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), p[3], p[4]);
    int i=p[5], res;

    res=dev->CAMACblread(dev, &addr, p[5], tmp);
    while (i--)
        (*var++)+=dev->CAMACval(*tmp++);
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_nAFblreadAddVar(ems_u32* p)
{
    plerrcode res;
    res=test_proc_nAFblreadVar(p);
    wirbrauchen=p[5];
    return res;
}
#ifdef PROCPROPS
static procprop nAFblreadAddVar_prop={0, 0, "var, n, A, F, num", 0};

procprop* prop_proc_nAFblreadAddVar()
{
    return &nAFblreadAddVar_prop;
}
#endif
char name_proc_nAFblreadAddVar[]="nAFblreadAddVar";
int ver_proc_nAFblreadAddVar=1;

/*****************************************************************************/
/*****************************************************************************/
