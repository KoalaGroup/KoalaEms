#ifndef _domobj_h_
#define _domobj_h_

#include "../objectcommon.h"

typedef struct{
  objectcommon common;
  errcode (*download)(ems_u32*, unsigned int);
  errcode (*upload)(ems_u32*, unsigned int);
  errcode (*delete)(ems_u32*, unsigned int);
}domobj;

extern domobj dom_obj;

#endif
