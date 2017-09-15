/*
 * proc_iotype.cc
 *
 * created: 08.06.97
 * 05.May.2001 PW: bugfixes in C_io_type::C_io_type and C_io_type::create
 */

#include <errno.h>
#include <stdlib.h>
#include "proc_iotype.hxx"
#include "proc_conf.hxx"
#include <errors.hxx>
#include "versions.hxx"

VERSION("May 05 2001", __FILE__, __DATE__, __TIME__,
"$ZEL: proc_iotype.cc,v 2.8 2004/11/26 14:44:28 wuestner Exp $")
#define XVERSION

/*****************************************************************************/
/*****************************************************************************/
C_io_type::C_io_type(io_direction direction, C_inbuf& ib)
:direction(direction), forcelists_(0)
{
    if ((ems_i32)ib[ib.index()]==-1) {
        forcelists_=1;
        ib++; // -1
        ib >> argnum;
    } else {
        forcelists_=0;
        argnum=(direction==io_out)?2:0;
    }
    args=new int[argnum];
    for (int i=0; i<argnum; i++)
        ib >> args[i];
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

int C_io_type::uselists(void) const
{
return forcelists_ || ((direction==io_out) && (argnum!=2))
    || ((direction==io_in) && (argnum!=0));
}

/*****************************************************************************/

int C_io_type::forcelists(int force)
{
int f=forcelists_;
forcelists_=force;
return f;
}

/*****************************************************************************/

C_io_type* C_io_type::create(io_direction direction, C_inbuf& ib)
{
InOutTyp iotyp;

iotyp=(InOutTyp)ib.getint();
switch (iotyp)
  {
  case InOut_Ringbuffer:
    return new C_io_type_ring(direction, ib);
  case InOut_Stream:
    return new C_io_type_stream(direction, ib);
  case InOut_Cluster:
    return new C_io_type_cluster(direction, ib);
  default:
    {
    OSTRINGSTREAM s;
    s << "C_io_type::create(): InOutTyp " << iotyp << " is not known";
    throw new C_program_error(s);
    }
  }
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

C_io_type_ring::C_io_type_ring(io_direction direction, C_inbuf& ib)
:C_io_type(direction, ib)
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

C_io_type_stream::C_io_type_stream(io_direction direction, C_inbuf& ib)
:C_io_type(direction, ib)
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

C_io_type_cluster::C_io_type_cluster(io_direction direction, C_inbuf& ib)
:C_io_type(direction, ib)
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
/*****************************************************************************/
