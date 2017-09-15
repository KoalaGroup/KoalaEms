#ifndef _domevobj_h_
#define _domevobj_h_

#include "../objectcommon.h"

typedef struct{
  objectcommon common;
  errcode (*download)(ems_u32*, unsigned int);
  errcode (*upload)(void);
  errcode (*delete)(ems_u32*, unsigned int);
}domevobj;

extern domevobj dom_ev_obj;

#endif
