/*
 * objects/domain/dom_trigger.h
 * created before 02.02.93
 * $ZEL: dom_trigger.h,v 1.6 2007/10/15 15:23:40 wuestner Exp $
 */

#ifndef _dom_trigger_h_
#define _dom_trigger_h_

#include <errorcodes.h>
#include <emsctypes.h>

typedef struct
  {
  unsigned int proc;
  ems_u32 param[1];
  } *triginfo;

extern triginfo trigdata;

errcode dom_trigger_init(void);
errcode dom_trigger_done(void);

#endif

/*****************************************************************************/
/*****************************************************************************/

