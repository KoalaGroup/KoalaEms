/*
 * proc_iotype.cc
 *
 * created: 08.06.97
 * 
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <errno.h>
#include <stdlib.h>
#include "proc_iotype.hxx"
#include "proc_conf.hxx"
#include <errors.hxx>
#include "versions.hxx"

VERSION("2014-09-07", __FILE__, __DATE__, __TIME__,
"$ZEL: proc_iotype.cc,v 2.12 2014/09/12 13:07:33 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/
/*****************************************************************************/
C_io_type::C_io_type(io_direction direction, bool use_lists, C_inbuf& ib)
:direction(direction), forcelists_(use_lists)
{
    if (forcelists_) {
        ib >> argnum;
    } else {
        argnum=(direction==io_out)?2:0;
    }

    args=new int[argnum];
    for (int i=0; i<argnum; i++)
        ib >> args[i];
}
/*****************************************************************************/
C_io_type::C_io_type(io_direction direction, const C_outbuf& ob)
:direction(direction), forcelists_(true)
{
    argnum=ob.size();
    args=new int[argnum];

    for (int i=0; i<argnum; i++)
        args[i]=ob[i];
}
/*****************************************************************************/

C_io_type::C_io_type(io_direction direction, int size, int prior)
:direction(direction), argnum(2), forcelists_(0)
{
args=new int[2];
args[0]=size;
args[1]=prior;
}

/*****************************************************************************/

C_io_type::C_io_type(io_direction direction, int num, const int* vals)
:direction(direction), argnum(num), forcelists_(1)
{
args=new int[argnum];
for (int i=0; i<argnum; i++) args[i]=vals[i];
}

/*****************************************************************************/

C_io_type::~C_io_type(void)
{
delete[] args;
}

/*****************************************************************************/

void C_io_type::pruefen() const
{
cerr << "C_io_type:" << endl;
cerr << "  direction=" << (int)direction << "; argnum=" << argnum << endl;
cerr << "  args:";
for (int i=0; i<argnum; i++) cerr << ' ' << args[i]; cerr << endl;
cerr << "  forcelists_=" << forcelists_ << "; uselists()=" << uselists() << endl; 
}

/*****************************************************************************/

bool C_io_type::uselists(void) const
{
return forcelists_ || ((direction==io_out) && (argnum!=2))
    || ((direction==io_in) && (argnum!=0));
}

/*****************************************************************************/

bool C_io_type::forcelists(bool force)
{
bool f=forcelists_;
forcelists_=force;
return f;
}

/*****************************************************************************/

C_io_type* C_io_type::create(io_direction direction, C_inbuf& ib)
{
    InOutTyp iotyp;
    ems_i32 iotype_;
    bool use_lists=false;

    // If the first word is -1 then the new protocol with lists is in use.
    // In this case is:
    //   first word: -1
    //   second word: InOutTyp
    //   third word: number of arguments after InOutTyp
    //
    // If the first word is not -1 then the old protocol is in use.
    // In this case is:
    //   first word: InOutTyp
    // 
    // after that the address follows
    // 
    // the particular C_io_type_* constructors do not consume words from
    // ib.

    iotype_=(ems_i32)ib.getint();
    if (iotype_==-1) {
        use_lists=true;
        // use the next word
        iotype_=(ems_i32)ib.getint();
    }
    iotyp=static_cast<InOutTyp>(iotype_);

    switch (iotyp) {
    case InOut_Ringbuffer:
        return new C_io_type_ring(direction, use_lists, ib);
    case InOut_Stream:
        return new C_io_type_stream(direction, use_lists, ib);
    case InOut_Cluster:
        return new C_io_type_cluster(direction, use_lists, ib);
    case InOut_Opaque:
        return new C_io_type_opaque(direction, use_lists, ib);
    case InOut_MQTT:
        return new C_io_type_mqtt(direction, use_lists, ib);
    default:
        {
            ostringstream s;
            s<<"C_io_type::create(): InOutTyp "<<iotyp
                <<" is not known or not supported";
#if 0
            s<<endl<<"inbuf: "<<ib<<endl;
#endif
            throw new C_program_error(s);
        }
    }

cerr<<"C_io_type::create:"<<endl<<ib<<endl;
}

/*****************************************************************************/

void C_io_type::outargs(C_outbuf& b) const
{
if (uselists()) b << -1 << argnum;
for (int i=0; i<argnum; i++) b << args[i];
}

/*****************************************************************************/

ostream& operator <<(ostream& os, const C_io_type& typ)
{
return typ.print(os);
}

/*****************************************************************************/

C_outbuf& operator <<(C_outbuf& ob, const C_io_type& typ)
{
return typ.out(ob);
}

/*****************************************************************************/

C_io_type_ring::C_io_type_ring(io_direction direction, bool lists, C_inbuf& ib)
:C_io_type(direction, lists, ib)
{}

/*****************************************************************************/

C_io_type_ring::C_io_type_ring(io_direction direction, int size, int prior)
:C_io_type(direction, size, prior)
{}

/*****************************************************************************/

C_io_type_ring::C_io_type_ring(io_direction direction, int num,
    const int* args)
:C_io_type(direction, num, args)
{}

/*****************************************************************************/

C_outbuf& C_io_type_ring::out(C_outbuf& b) const
{
b << InOut_Ringbuffer;
outargs(b);
return b;
}

/*****************************************************************************/

ostream& C_io_type_ring::print(ostream& os) const
{
if (uselists()) os<<"{";
os << "ringbuffer";
for (int i=0; i<argnum; i++) os<<' '<<args[i];
if (uselists()) os << '}';
return os;
}

/*****************************************************************************/

C_io_type_stream::C_io_type_stream(io_direction direction, bool lists,
        C_inbuf& ib)
:C_io_type(direction, lists, ib)
{}

/*****************************************************************************/

C_io_type_stream::C_io_type_stream(io_direction direction, int size, int prior)
:C_io_type(direction, size, prior)
{}

/*****************************************************************************/

C_io_type_stream::C_io_type_stream(io_direction direction, int num,
    const int* args)
:C_io_type(direction, num, args)
{}


/*****************************************************************************/

C_outbuf& C_io_type_stream::out(C_outbuf& b) const
{
b << InOut_Stream;
outargs(b);
return b;
}

/*****************************************************************************/

ostream& C_io_type_stream::print(ostream& os) const
{
if (uselists()) os<<"{";
os << "stream";
for (int i=0; i<argnum; i++) os<<' '<<args[i];
if (uselists()) os << '}';
return os;
}

/*****************************************************************************/

C_io_type_cluster::C_io_type_cluster(io_direction direction, bool lists,
        C_inbuf& ib)
:C_io_type(direction, lists, ib)
{}

/*****************************************************************************/

C_io_type_cluster::C_io_type_cluster(io_direction direction, int size,
    int prior)
:C_io_type(direction, size, prior)
{}

/*****************************************************************************/

C_io_type_cluster::C_io_type_cluster(io_direction direction, int num,
    const int* args)
:C_io_type(direction, num, args)
{}

/*****************************************************************************/

C_outbuf& C_io_type_cluster::out(C_outbuf& b) const
{
b << InOut_Cluster;
outargs(b);
return b;
}

/*****************************************************************************/

ostream& C_io_type_cluster::print(ostream& os) const
{
if (uselists()) os<<"{";
os<<"cluster";
for (int i=0; i<argnum; i++) os<<' '<<args[i];
if (uselists()) os << '}';
return os;
}

/*****************************************************************************/
C_io_type_opaque::C_io_type_opaque(io_direction direction, bool lists,
        C_inbuf& ib)
:C_io_type(direction, lists, ib)
{}
/*****************************************************************************/
C_io_type_opaque::C_io_type_opaque(io_direction direction, int size,
    int prior)
:C_io_type(direction, size, prior)
{}
/*****************************************************************************/
C_io_type_opaque::C_io_type_opaque(io_direction direction, int num,
    const int* args)
:C_io_type(direction, num, args)
{}
/*****************************************************************************/
C_outbuf&
C_io_type_opaque::out(C_outbuf& b) const
{
    b << InOut_Opaque;
    outargs(b);
    return b;
}
/*****************************************************************************/
ostream&
C_io_type_opaque::print(ostream& os) const
{
    if (uselists())
        os<<"{";
    os<<"opaque";
    for (int i=0; i<argnum; i++)
        os<<' '<<args[i];
    if (uselists())
        os << '}';
    return os;
}

/*****************************************************************************/
//C_io_type_mqtt::C_io_type_mqtt(io_direction direction, bool lists, C_inbuf& ib)
//:C_io_type(direction, lists, ib)
//{}
/*****************************************************************************/
//C_io_type_mqtt::C_io_type_mqtt(io_direction direction, int size, int prior)
//:C_io_type(direction, size, prior)
//{}
/*****************************************************************************/
//C_io_type_mqtt::C_io_type_mqtt(io_direction direction, int num,
//    const int* args)
//:C_io_type(direction, num, args)
//{}
/*****************************************************************************/
//C_io_type_mqtt::C_io_type_mqtt(io_direction direction, const C_anylist *args)
//:C_io_type(direction, args)
//{}
/*****************************************************************************/
C_outbuf&
C_io_type_mqtt::out(C_outbuf& b) const
{
    b << InOut_MQTT;
    outargs(b);
    return b;
}
/*****************************************************************************/
ostream&
C_io_type_mqtt::print(ostream& os) const
{
    if (uselists())
        os<<"{";
    os<<"mqtt";
    for (int i=0; i<argnum; i++)
        os<<' '<<args[i];
    if (uselists())
        os << '}';
    return os;
}
/*****************************************************************************/
/*****************************************************************************/
