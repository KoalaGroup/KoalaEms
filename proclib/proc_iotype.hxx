/*
 * proc_iotype.hxx
 * 
 * created: 08.06.97
 */

#ifndef _proc_iotype_hxx_
#define _proc_iotype_hxx_

#include "config.h"
#include <objecttypes.h>
#include <outbuf.hxx>
#include <inbuf.hxx>

class C_data_io;

/*****************************************************************************/

typedef enum {io_in, io_out} io_direction;

class C_io_type
  {
  friend class C_data_io;
  friend ostream& operator <<(ostream&, const C_io_type&);
  friend C_outbuf& operator <<(C_outbuf&, const C_io_type&);
  public:
    C_io_type(io_direction);
    C_io_type(io_direction, C_inbuf&);
    C_io_type(io_direction, int, int);
    C_io_type(io_direction, int, const int*);
    virtual ~C_io_type();
    static C_io_type* create(io_direction, C_inbuf&);
  protected:
    io_direction direction;
    int argnum;
    int* args;
    int forcelists_;
    int uselists() const;
    int forcelists(int);
    void outargs(C_outbuf&) const;
    virtual C_outbuf& out(C_outbuf&) const=0;
    virtual ostream& print(ostream&) const=0;
  public:
    virtual InOutTyp iotype() const=0;
    void pruefen() const;
 };

ostream& operator <<(ostream&, const C_io_type&);
C_outbuf& operator <<(C_outbuf&, const C_io_type&);

class C_io_type_ring: public C_io_type
  {
  public:
    C_io_type_ring(io_direction);
    C_io_type_ring(io_direction, C_inbuf&);
    C_io_type_ring(io_direction, int, int);
    C_io_type_ring(io_direction, int, const int*);
  protected:
    virtual C_outbuf& out(C_outbuf&) const;
    virtual ostream& print(ostream&) const;
  public:
    InOutTyp iotype() const {return InOut_Ringbuffer;}
  };

class C_io_type_stream: public C_io_type
  {
  public:
    C_io_type_stream(io_direction);
    C_io_type_stream(io_direction, C_inbuf&);
    C_io_type_stream(io_direction, int, int);
    C_io_type_stream(io_direction, int, const int*);
  protected:
    virtual C_outbuf& out(C_outbuf&) const;
    virtual ostream& print(ostream&) const;
  public:
    InOutTyp iotype() const {return InOut_Stream;}
  };

class C_io_type_cluster: public C_io_type
  {
  public:
    C_io_type_cluster(io_direction);
    C_io_type_cluster(io_direction, C_inbuf&);
    C_io_type_cluster(io_direction, int, int);
    C_io_type_cluster(io_direction, int, const int*);
  protected:
    virtual C_outbuf& out(C_outbuf&) const;
    virtual ostream& print(ostream&) const;
  public:
    InOutTyp iotype() const {return InOut_Cluster;}
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
