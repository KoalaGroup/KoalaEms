/*
 * objects/ved/ved.c
 * created before 07.11.94
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: ved.c,v 1.21 2011/04/06 20:30:29 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <objecttypes.h>
#include <errorcodes.h>
#include <requesttypes.h>
#include <unsoltypes.h>
#include <xdrstring.h>
#include <rcs_ids.h>

#include "ved.h"
#include "vedobj.h"

#include "../objectcommon.h"
#include "../domain/domain.h"
#include "../pi/pi.h"
#include "../do/dataout.h"
#include "../is/is.h"
#ifdef OBJ_VAR
#include "../var/variables.h"
#endif
#include "../../procs/listprocs.h"
#include "../../trigger/trigger.h"

#define ver_VED 4
extern ems_u32* outptr;

/* in server/conf/sconf.c */
extern char configuration[], buildname[], builddate[];

struct ved_globals ved_globals;

RCS_REGISTER(cvsid, "objects/ved")

/*****************************************************************************/

errcode ved_init()
{
    errcode res;

    T(ved_init)
    ved_globals.ved_id=0;
    ved_globals.runnr=-1;
#ifdef USE_SQL
    ved_globals.sqldb.valid=0;
    ved_globals.sqldb.do_id=-1;
    ved_globals.sqldb.host=0;
    ved_globals.sqldb.user=0;
    ved_globals.sqldb.passwd=0;
    ved_globals.sqldb.db=0;
    ved_globals.sqldb.port=0;
#endif

#ifdef OBJ_DOMAIN
    if ((res=domain_init())) return res;
#endif

#ifdef OBJ_IS
    if ((res=is_init())) return res;
#endif

#ifdef OBJ_VAR
    if ((res=var_init())) return res;
#endif

#ifdef OBJ_DO
    if ((res=do_init())) return res;
#endif

#ifdef OBJ_PI
    if ((res=pi_init())) return res;
#endif

    return OK;
}

/*****************************************************************************/

errcode ved_done()
{
    errcode rr, res=OK;

    T(ved_done)

#ifdef OBJ_PI
    if ((rr=pi_done())) res=rr;
#endif

#ifdef OBJ_DO
    if ((rr=do_done())) res=rr;
#endif

#ifdef OBJ_VAR
    if ((rr=var_done())) res=rr;
#endif

#ifdef OBJ_IS
    if ((rr=is_done())) res=rr;
#endif

#ifdef OBJ_DOMAIN
    if ((rr=domain_done())) res=rr;
#endif


#ifdef USE_SQL
    free(ved_globals.sqldb.host);
    free(ved_globals.sqldb.user);
    free(ved_globals.sqldb.passwd);
    free(ved_globals.sqldb.db);
    ved_globals.sqldb.valid=0;
    ved_globals.sqldb.do_id=-1;
    ved_globals.sqldb.host=0;
    ved_globals.sqldb.user=0;
    ved_globals.sqldb.passwd=0;
    ved_globals.sqldb.db=0;
    ved_globals.sqldb.port=0;
#endif

    return res;
}

/*****************************************************************************/

errcode Initiate(ems_u32* p, int num)
{
T(Initiate)
if (num!=1) return(Err_ArgNum);
D(D_REQ, printf("Initiate(%d)\n", p[0]);)
/* ved_globals.ved_id=p[0]; */ /* changing is fatal for readout */
*outptr++=ver_VED;
*outptr++=ver_requesttab;
*outptr++=ver_unsolmsgtab;
return(OK);
}

/*****************************************************************************/

errcode Conclude(ems_u32* p, int num)
{/* Conclude */
T(Conclude)
D(D_REQ, printf("Conclude\n");)
if (num!=0) return(Err_ArgNum);
ved_globals.ved_id=0;
return(OK);
}/* Conclude */

/*****************************************************************************/

errcode GetVEDStatus(ems_u32* p, int num)
{
T(GetVEDStatus)
D(D_REQ, printf("GetVEDStatus\n");)
if (num!=0) return(Err_ArgNum);
*outptr++=ved_globals.ved_id;
return(OK);
}/* GetVEDStatus */

/*****************************************************************************/

errcode Identify(ems_u32* p, int num)
{
ems_u32 *help;
int i;

T(Identify)
D(D_REQ, printf("Identify\n");)

if (num!=1) return(Err_ArgNum);
if (p[0]>3)
    return Err_ArgRange;
*outptr++=ver_VED;
*outptr++=ver_requesttab;
*outptr++=ver_unsolmsgtab;
help=outptr++; i=0;
if (p[0]>0)
  {
  outptr=outstring(outptr, buildname); i++;
  outptr=outstring(outptr, builddate); i++;
  }
if (p[0]>1) {
    outptr=outstring(outptr, configuration);
    i++;
}
if (p[0]>2) {
    const struct rcs_id *ids;
    char *s;
    int num, n, j;

    num=rcs_get(&ids);
    /* count the characters */
    n=2*num+1; /* '\n' for each string and the final '\0' */
    for (j=0; j<num; j++) {
        n+=strlen(ids[j].path);
        n+=strlen(ids[j].id);
    }
    /* create one large string */
    s=malloc(n);
    if (!s) {
        printf("Identify(>2): malloc(%d): %s\n", n, strerror(errno));
        return Err_NoMem;
    }
    s[0]='\0';
    for (j=0; j<num; j++) {
        strcat(s, ids[j].path);
        strcat(s, "\n");
        strcat(s, ids[j].id);
        strcat(s, "\n");
    }
    /* output the string */
    outptr=outstring(outptr, s);
    i++;
}

*help=i;
return(OK);
}

/*****************************************************************************/
/*
GetNameList
p[0] : object
...
*/
errcode GetNameList(ems_u32* p, unsigned int num)
{
    T(GetNameList)
    D(D_REQ, int i; printf("GetNameList(");
        for (i=0; i<num; i++) printf("%d%s", p[i], i+1<num?", ":"");
        printf(")\n");
    )

    if (num==0) { /* Baum aller Objekte gefordert */
        return(Err_ArgNum);
    } else {
        objectcommon *obj;
        unsigned int rem;
        ems_u32* help;

/*
 *   printf("GetNameList: call lookup_object(p={");
 *   for (i=0; i<num; i++) printf("%d%s", p[i], i+1<num?", ":"");
 *   printf("}, num=%d, ...)\n", num);
 */
        obj=lookup_object(p, num, &rem);
        if (!obj) {
            printf("GetNameList: got (objectcommon*)0\n");
            return(Err_IllObject);
        }
        if (rem>0) {
            printf("getnamelist-lookup: rem=%d\n",rem);
            *outptr++=rem;
            return Err_ArgNum;
        }

        help=outptr++;
        outptr=obj->dir(outptr);
        *help=outptr-help-1;
        return OK;
    }
    return Err_Program;
}
/*****************************************************************************/
/*
GetCapabilityList
p[0] : Capabtyp
*/
errcode GetCapabilityList(ems_u32* p, unsigned int num)
{
    errcode res;

    T(GetCapabilityList)
    D(D_REQ, printf("GetCapabilityList\n");)

    if (num!=1)
        return Err_ArgNum;

    switch (*p) {
#ifdef PROCS
    case Capab_listproc:
        res=getlistproclist(p, num);
        break;
#endif /* PROCS */
#ifdef TRIGGER
    case Capab_trigproc:
D(D_REQ, printf("GetCapabilityList Trigger\n");)
        res=gettrigproclist(p, num);
        break;
#endif /* TRIGGER */
    default:
        D(D_REQ, printf("GetCapabilityList default\n");)
        res=Err_IllCapTyp;
    }
    return res;
}
/*****************************************************************************/
/*
GetProcProperties
p[0] : level (0: nur Laengen, 1: Syntax, 2: Text, ...?)
p[1] : no. of procs
.... : procIDs
*/
errcode GetProcProperties(ems_u32* p, int num)
{
T(GetProcProperties)
D(D_REQ, printf("GetProcProperties\n");)
if ((num<2) || (num-2!=p[1])) return(Err_ArgNum);
#if defined (PROCS) && defined (PROCPROPS)
return(getlistprocprop(p, num));
#else
return(Err_NotImpl);
#endif
}

/*****************************************************************************/

static objectcommon *lookup_ved(ems_u32* id, unsigned int idlen,
    unsigned int* remlen)
{
*remlen=0;
return(&ved_obj);
}

/*****************************************************************************/

static ems_u32 *dir_ved(ems_u32* ptr)
{
*ptr++=0;
return(ptr);
}

/*****************************************************************************/

objectcommon ved_obj={
  0,
  0,
  lookup_ved,
  dir_ved,
  0
};

/*****************************************************************************/
/*****************************************************************************/
