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
//#include <anylist.hxx>

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
    C_io_type(io_direction, bool use_lists, C_inbuf&);
    C_io_type(io_direction, const C_outbuf&);
    C_io_type(io_direction, int size, int prior);
    C_io_type(io_direction, int num, const int* vals);
    //C_io_type(io_direction, const C_anylist *args);
    virtual ~C_io_type();
    static C_io_type* create(io_direction, C_inbuf&);
  protected:
    io_direction direction;
    int argnum;
    int* args;
    bool forcelists_;
    bool uselists() const;
    bool forcelists(bool);
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
    C_io_type_ring(io_direction, bool use_lists, C_inbuf&);
    C_io_type_ring(io_direction direction, const C_outbuf &ob)
        :C_io_type(direction, ob) {};
    C_io_type_ring(io_direction, int, int);
    C_io_type_ring(io_direction, int, const int*);
    //C_io_type_ring(io_direction direction, const C_anylist *args)
    //    :C_io_type(direction, args) {};
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
    C_io_type_stream(io_direction, bool use_lists, C_inbuf&);
    C_io_type_stream(io_direction direction, const C_outbuf &ob)
        :C_io_type(direction, ob) {};
    C_io_type_stream(io_direction, int, int);
    C_io_type_stream(io_direction, int, const int*);
    //C_io_type_stream(io_direction direction, const C_anylist *args)
    //    :C_io_type(direction, args) {};
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
    C_io_type_cluster(io_direction, bool use_lists, C_inbuf&);
    C_io_type_cluster(io_direction direction, const C_outbuf &ob)
        :C_io_type(direction, ob) {};
    C_io_type_cluster(io_direction, int, int);
    C_io_type_cluster(io_direction, int, const int*);
    //C_io_type_cluster(io_direction direction, const C_anylist *args)
    //    :C_io_type(direction, args) {};
  protected:
    virtual C_outbuf& out(C_outbuf&) const;
    virtual ostream& print(ostream&) const;
  public:
    InOutTyp iotype() const {return InOut_Cluster;}
  };

class C_io_type_opaque: public C_io_type
  {
  public:
    C_io_type_opaque(io_direction);
    C_io_type_opaque(io_direction, bool use_lists, C_inbuf&);
    C_io_type_opaque(io_direction direction, const C_outbuf &ob)
        :C_io_type(direction, ob) {};
    C_io_type_opaque(io_direction, int, int);
    C_io_type_opaque(io_direction, int, const int*);
    //C_io_type_opaque(io_direction direction, const C_anylist *args)
    //    :C_io_type(direction, args) {};
  protected:
    virtual C_outbuf& out(C_outbuf&) const;
    virtual ostream& print(ostream&) const;
  public:
    InOutTyp iotype() const {return InOut_Opaque;}
  };

class C_io_type_mqtt: public C_io_type
  {
  public:
    C_io_type_mqtt(io_direction);
    C_io_type_mqtt(io_direction direction, bool use_lists, C_inbuf& ib)
        :C_io_type(direction, use_lists, ib) {};
    C_io_type_mqtt(io_direction direction, const C_outbuf &ob)
        :C_io_type(direction, ob) {};
    C_io_type_mqtt(io_direction direction, int size, int prior)
        :C_io_type(direction, size, prior) {};
    C_io_type_mqtt(io_direction direction, int num, const int *args)
        :C_io_type(direction, num, args) {};
    //C_io_type_mqtt(io_direction direction, const C_anylist *args)
    //    :C_io_type(direction, args) {};
  protected:
    virtual C_outbuf& out(C_outbuf&) const;
    virtual ostream& print(ostream&) const;
  public:
    InOutTyp iotype() const {return InOut_MQTT;}
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
