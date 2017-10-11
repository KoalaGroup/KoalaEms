/*
 * proc_dataoutstatus.hxx
 * created: 12.06.97
 *
 * $ZEL: proc_dataoutstatus.hxx,v 2.7 2014/07/14 15:11:53 wuestner Exp $
 */

#ifndef _proc_dataoutstatus_hxx_
#define _proc_dataoutstatus_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <inbuf.hxx>

class C_confirmation;

using namespace std;

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
