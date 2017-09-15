/*
 * proc_dataio.hxx
 * 
 * created: 10.06.97 PW
 * 12.06.1998 PW: adapted for STD_STRICT_ANSI
 * 04.May.2001 PW: additional arguments added for dataout
 */

#ifndef _proc_dataio_hxx_
#define _proc_dataio_hxx_

#include "config.h"
#include "cxxcompat.hxx"

#include <outbuf.hxx>
#include "proc_iotype.hxx"
#include "proc_ioaddr.hxx"

class C_confirmation;

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
