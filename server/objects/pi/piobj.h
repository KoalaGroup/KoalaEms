#ifndef _piobj_h_
#define _piobj_h_

#include "../objectcommon.h"

typedef struct{
  objectcommon common;
  errcode (*create)(ems_u32*, unsigned int);
  errcode (*delete)(ems_u32*, unsigned int);
  errcode (*start)(ems_u32*, unsigned int);
  errcode (*reset)(ems_u32*, unsigned int);
  errcode (*stop)(ems_u32*, unsigned int);
  errcode (*resume)(ems_u32*, unsigned int);
  errcode (*getattr)(ems_u32*, unsigned int);
  errcode (*getparam)(ems_u32*, unsigned int);
}piobj;

extern piobj pi_obj;

#endif
