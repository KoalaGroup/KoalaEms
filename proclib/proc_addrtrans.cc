/******************************************************************************
*                                                                             *
* proc_addr.cc                                                                *
*                                                                             *
* OSF1/ULTRIX                                                                 *
*                                                                             *
* created: 09.09.94                                                           *
* last changed: 18.09.95                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include <stdlib.h>
#include <proc_addrtrans.hxx>
#include <ved_errors.hxx>
#include <versions.hxx>

VERSION("Jun 12 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: proc_addrtrans.cc,v 2.5 2004/11/26 14:44:25 wuestner Exp $")
#define XVERSION

/*
Domain datain:
==============

bus | path            | space            | offset
------------------------------------------------------------------------------
    |                 |                  |
VIC | /vic/<slave-id> | CAMAC: 1; CHI: 0 | <dataout-addr>-slaveoffs+masteroffs
    |                 |                  |
VME | /vme/<x>        | meist 0          | ""
    |                 |                  |

x: obere 8 bit der A32-Adresse

masteroffset:
=============

falls Adresstyp = addr_raw:

master   | local | VME        | VIC
------------------------------------------
         |       |            |
FIC 8232 | 0     | 0x80000000 | 0x40000000
         |       |            |
FIC 8234 | 0     | 0xc0000000 | 0x80000000
         |       |            |
E6       | 0     | 0          | 0xf0000000
         |       |            |

sonst: masteroffs=0



slaveoffet:
===========

master   | local | VME        | VIC
------------------------------------------
         |       |            |
FIC 8232 | 0     | 0x20000000 | /
         |       |            |
FIC 8234 | 0     | 0x20000000 | /
         |       |            |
E6       | 0     | 0          | /
         |       |            |
VCC 2117 | 0     | /          | 0x00800000 fuer 2MB-controller
         |       |            | 0x00880000 fuer 512kB-controller
         |       |            |
CHI      | 0     | /          | 0x20000000
*/

/*****************************************************************************/

C_add_trans::C_add_trans(cpu master, cpu slave)
{
ok=0;
masterdriver=0;
dest=master;
source=slave;
}

/*****************************************************************************/

C_add_trans::C_add_trans(cpu slave)
{
ok=0;
masterdriver=1;
source=slave;
}

/*****************************************************************************/

void C_add_trans::setoffs(int offset)
{
ok=1;
offs=offset;
}

/*****************************************************************************/

C_add_trans::operator int()
{
if (!ok)
  {
  OSTRINGSTREAM s;
  s << "Object C_add_trans has to be initialized with C_VED::UploadDataoutAddr";
  throw new C_ved_error(0, s);
  }

int res=offs;
int vic=0;

switch (source)
  {
  case VCC2117_512: res-=0x00880000; vic=1; break;
  case VCC2117    : res-=0x00800000; vic=1; break;
  case E6         : break;
  case E7         : break;
  case FIC8232    : res-=0x20000000; break;
  case FIC8234    : res-=0x20000000; break;
  case STR330     : vic=1; break;
  case FVSBI      : break;
  case STR340     : break;
  case VCC2118    : break;
  }

if (!masterdriver)
  {
  switch (dest)
    {
    case VCC2117_512:
    case VCC2117    :
    case VCC2118    :
    case STR330     :
    case STR340     :
    case FVSBI      :
      {
      OSTRINGSTREAM s;
      s << "Module can not work as master";
      throw new C_ved_error(0, s);
      }
    case E6         :
      if (vic)
        res+=0xf0000000;
      break;
    case E7         :
      {
      OSTRINGSTREAM s;
      s << "E7 not yet implemented";
      throw new C_ved_error(0, s);
      }
    case FIC8232    :
      if (vic)
        res+=0x40000000;
      else
        res+=0x80000000;
      break;
    case FIC8234    :
      if (vic)
        res+=0x80000000;
      else
        res+=0xc0000000;
      break;
    }
  }
return(res);
}

/*****************************************************************************/
/*****************************************************************************/
