/*
 * proc_dataoutstatus.hxx
 * created: 12.06.97
 * 22.03.1999 PW: device eliminated
 */
#ifndef _proc_dataoutstatus_hxx_
#define _proc_dataoutstatus_hxx_

#include "config.h"
#include "cxxcompat.hxx"

#include <inbuf.hxx>

class C_confirmation;

/*****************************************************************************/

class C_dataoutstatus
  {
  public:
    C_dataoutstatus(const C_confirmation* conf);
    ~C_dataoutstatus();
  protected:
    C_inbuf buf;
  public:
    ostream& print(ostream&);
  };

ostream& operator <<(ostream&, const C_dataoutstatus&);

#endif

/*****************************************************************************/
/*****************************************************************************/
