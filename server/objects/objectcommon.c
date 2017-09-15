/*
 * server/objects/objectcommon.c
 * created 16.01.96 MD
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: objectcommon.c,v 1.9 2011/04/06 20:30:29 wuestner Exp $";

#include <string.h>
#include <sconf.h>
#include <debug.h>
#include <objecttypes.h>
#include <errorcodes.h>
#include <rcs_ids.h>

#include "objectcommon.h"

#include "ved/vedobj.h"
#ifdef OBJ_VAR
#include "var/varobj.h"
#endif
#ifdef OBJ_PI
#include "pi/piobj.h"
#endif
#ifdef OBJ_DOMAIN
#include "domain/domobj.h"
#endif
#ifdef OBJ_IS
#include "is/isobj.h"
#endif
#ifdef OBJ_DO
#include "do/doobj.h"
#endif

extern objectcommon objobj; /* forward */

RCS_REGISTER(cvsid, "objects")

/*****************************************************************************/

objectcommon *lookup_object(ems_u32* id, unsigned int idlen, unsigned int* remlen)
{
  if(idlen>0){
    objectcommon *obj;
    switch(id[0]){
      case Object_null:
        obj=&objobj;
        break;
      case Object_ved:
	obj= &ved_obj;
	break;
#ifdef OBJ_DOMAIN
      case Object_domain:
	obj=(objectcommon*)&dom_obj;
	break;
#endif
#ifdef OBJ_IS
      case Object_is:
      obj=(objectcommon*)&is_obj;
      break;
#endif
#ifdef OBJ_VAR
      case Object_var:
	obj= &var_obj;
	break;
#endif
#ifdef OBJ_PI
      case Object_pi:
	obj=(objectcommon*)&pi_obj;
	break;
#endif
#ifdef OBJ_DO
      case Object_do:
      obj=(objectcommon*)&do_obj;
      break;
#endif
      default:
	return(0);
    }
    return(obj->lookup(id+1, idlen-1, remlen));
  }else
    {
    *remlen=0;
    return(&objobj);
    }
}

/*****************************************************************************/

static ems_u32 *dir_object(ems_u32* ptr)
{
*ptr++=Object_ved;
#ifdef OBJ_DOMAIN
  *ptr++=Object_domain;
#endif
#ifdef OBJ_IS
  *ptr++=Object_is;
#endif
#ifdef OBJ_VAR
  *ptr++=Object_var;
#endif
#ifdef OBJ_PI
  *ptr++=Object_pi;
#endif
#ifdef OBJ_DO
  *ptr++=Object_do;
#endif
  return(ptr);
}

/*****************************************************************************/

objectcommon objobj={
  0, 0,
  lookup_object,
  dir_object,
  0
};

/*****************************************************************************/

errcode notimpl(ems_u32* idx, unsigned int idxlen){
  return(Err_NotImpl);
}

/*****************************************************************************/
#if 0
static char *currentid;

int checkperm(objectcommon* obj, ems_u32* idx, unsigned int idxlen, int op)
{
  if(obj->getprot){
    char *owner;
    int *perms;
    int rights;

    obj->getprot(idx, idxlen, &owner, &perms);
    if(!perms)return(1);
    rights=perms[perms[0]];
    if(owner && currentid && strcmp(owner,currentid))
      rights=perms[1];
    if((op&rights)!=op){
      D(D_REQ,printf(
	"operation %x for %s denied (object owner %s, perm %x)\n",
	       op,currentid?currentid:"nobody",
	       owner?owner:"nobody",rights);)
      return(0);
    }
  }
  return(1);
}
#endif
/*****************************************************************************/
#if 0
void setid(char* id)
{
currentid=id;
}
#endif
/*****************************************************************************/
/*****************************************************************************/
