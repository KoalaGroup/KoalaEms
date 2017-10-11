/*
 * proclib/proc_dataio.hxx
 * 
 * created: 10.06.97 PW
 * 
 * $ZEL: proc_dataio.hxx,v 2.6 2014/07/14 15:11:53 wuestner Exp $
 */

#ifndef _proc_dataio_hxx_
#define _proc_dataio_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <outbuf.hxx>
#include "proc_iotype.hxx"
#include "proc_ioaddr.hxx"

class C_confirmation;

using namespace std;

/*****************************************************************************/

class C_data_io
  {
  friend ostream& operator <<(ostream&, const C_data_io&);
  friend C_outbuf& operator <<(C_outbuf&, const C_data_io&);
  public:
    C_data_io(C_io_type*, C_io_addr*);
    ~C_data_io();
    static C_data_io* create(io_direction, const C_confirmation*);
  protected:
    C_io_type* typ;
    C_io_addr* addr;
    int* args;
    int numargs;
    ostream& print(ostream&) const;
    C_outbuf& put(C_outbuf&) const;
  public:
    int forcelists(int);
    int uselists() const;
    InOutTyp iotype() const {return typ->iotype();}
    IOAddr addrtype() const {return addr->addrtype();}
    void set_args(int*, int);
    void pruefen() const;
  };

ostream& operator <<(ostream&, const C_data_io&);
C_outbuf& operator <<(C_outbuf&, const C_data_io&);

#endif

/*****************************************************************************/
/*****************************************************************************/
