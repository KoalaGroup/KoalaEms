/******************************************************************************
*                                                                             *
* lowlevel/fastbus/sfi/sfi_fbutil.c                                           *
*                                                                             *
* created: 15.05.97                                                           *
* last changed: 15.05.97                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: fb_util.c,v 1.3 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../objects/domain/test_modul_id.h"
#include "fastbus.h"

RCS_REGISTER(cvsid, "")

/*****************************************************************************/

int ist_aehnlich(int type, int mtype)
{
int res;

if ((type&0xffff)==mtype)
  res=0;
else
  {
  res=-1;

  switch (type)
    {
/*    case LC_TDC_1876:
      if (mtype==0x6854) res=0;
      break;*/
    default:
      res=-1;
      break;
    }
  }
return(res);
}

/*****************************************************************************/

int test_modul_id(int addr, int type)
{
int mtype;

mtype=fb_modul_id(addr);
if (mtype==0)
  {
  D(D_USER, printf("test_modul_id(%d, 0x%04x): slot is empty.\n",
      addr, type);)
  return(-1);
  }

if (ist_aehnlich(type, mtype)==-1)
  {
  D(D_USER, printf("test_modul_id(%d, 0x%04x): found 0x%04x.\n",
      addr, type, mtype);)
  return(-1);
  }
else
  return(0);
}

/*****************************************************************************/
/*****************************************************************************/
