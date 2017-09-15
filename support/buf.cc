/*
 * buf.cc
 * 
 * created 28.01.1996 PW
 * 05.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#include "config.h"
#include <errno.h>
#include "buf.hxx"
#include "errors.hxx"
#include "versions.hxx"

VERSION("Jun 06 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: buf.cc,v 2.18 2004/11/26 14:45:29 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_buf::C_buf()
:buffer(0), bsize(0), ibsize(0), incr(100), dsize(0), idx(0)/*, marke(0)*/
{}

/*****************************************************************************/

C_buf::C_buf(const C_buf& b)
:ibsize(b.ibsize), incr(b.incr), dsize(b.dsize), idx(0)/*, marke(0)*/,
annobuf(0), annosize(0)
{
buffer=new ems_u32[ibsize];
if (buffer==0) throw new C_unix_error(errno, "C_buf::C_buf(C_buf&)");
bsize=ibsize;
for (int i=0; i<bsize; i++) buffer[i]=b.buffer[i];
}

/*****************************************************************************/

C_buf::C_buf(int size, int incr)
:ibsize(size), incr(incr), dsize(0), idx(0)/*, marke(0)*/,
annobuf(0), annosize(0)
{
if (size)
  {
  buffer=new ems_u32[ibsize];
  if (buffer==0) throw new C_unix_error(errno, "C_buf::C_buf");
  }
else
  buffer=0;
bsize=ibsize;
}

/*****************************************************************************/

C_buf::~C_buf()
{
delete[] buffer;
delete[] annobuf;
}

/*****************************************************************************/

void C_buf::grow(int i)
{
int len=bsize;
if (ibsize>len) len=ibsize;
if (incr==0) incr=100;
while (i>=len) len+=incr;
ems_u32 *help;
if ((help=new ems_u32[len])==0)
    throw new C_unix_error(errno, "C_buf::grow");
if (buffer!=0)
  for (int j=0; j<dsize; j++) help[j]=buffer[j];
else
  for (int j=0; j<dsize; j++) help[j]=0;
delete[] buffer;
buffer=help;
bsize=len;
}

/*****************************************************************************/

int C_buf::size(int size)
{
int s=dsize;
if (size>dsize)
    throw new C_program_error("C_buf::size: size can not grow");
dsize=size;
return s;
}

/*****************************************************************************/
int C_buf::annotate(unsigned int a)
{
    if (annosize<=idx) {
        int i;
        unsigned int* help=new unsigned int[bsize];
        if (!help) {
            throw new C_unix_error(errno, "C_buf::grow annobuf");
        }
        for (i=0; i<annosize; i++) help[i]=annobuf[i];
        for (i=annosize; i<bsize; i++) help[i]=0;
        annosize=bsize;
        delete[] annobuf;
        annobuf=help;
    }
    int r=annobuf[idx];
    annobuf[idx]=a;
    return r;
}
/*****************************************************************************/

C_buf& C_buf::operator ++(int)
{
idx++;
if (idx>dsize) dsize=idx;
return *this;
}

/*****************************************************************************/

C_buf& C_buf::operator +=(int i)
{
idx+=i;
if (idx>dsize) dsize=idx;
return *this;
}

/*****************************************************************************/

ems_u32& C_buf::operator [] (int i)
{
if (i<0)
  {
  cerr<<"C_buf::operator[](int idx="<<i<<")"<<endl;
  cerr<<(*this)<<endl;
    throw new C_program_error("C_buf::operator [](int): underflow");
  }
if (i>=bsize) grow(i);
return buffer[i];
}

/*****************************************************************************/

ems_u32& C_buf::operator [] (int i) const
{
if ((i<0) || (i>=bsize))
    throw new C_program_error("C_buf::operator[]: i>size");
return buffer[i];
}

/*****************************************************************************/

ems_u32* C_buf::getbuffer(void)
{
ems_u32* b=buffer;
buffer=0;
idx=0;
bsize=0;
dsize=0;
return b;
}

/*****************************************************************************/

int C_buf::index(int i)
{
if (i>bsize) grow(i);
int j=idx;
idx=i;
return j;
}

/*****************************************************************************/

void C_buf::clear()
{
if (bsize>ibsize+incr)
  {
  delete[] buffer;
  buffer=0;
  if (ibsize)
    {
    buffer=new ems_u32[ibsize];
    if (buffer==0) throw new C_unix_error(errno, "C_buf::clear");
    bsize=ibsize;
    }
  else
    bsize=0;
  }
dsize=0;
idx=0;
}

/*****************************************************************************/
// void C_buf::mark_it()
// {
// marke=idx;
// }
/*****************************************************************************/
int C_buf::write(C_iopath& path)
{
    if (path.write(&dsize, 4, 0)!=4)
        return -1;
    if (path.write(buffer, dsize*4, 0)!=dsize*4)
        return -1;
    return 0;
}
/*****************************************************************************/
int C_buf::read(C_iopath& path)
{
    clear();
    if (path.read(&dsize, 4)!=4)
        return -1;
    grow(dsize);
    if (path.read(buffer, dsize*4)!=dsize*4)
        return -1;
    return 0;
}
/*****************************************************************************/
// C_buf& mark(C_buf& buf)
// {
// buf.mark_it();
// return buf;
// }
/*****************************************************************************/

ostream& C_buf::print(ostream& os) const
{
int i;
os << "  idx=" << idx << "; size=" << dsize << endl << "    ";
for (i=0; i<idx; i++) os << buffer[i] << (i+1<idx?", ":"");
os << " | ";
for (i=idx; i<dsize; i++) os << buffer[i] << (i+1<dsize?", ":"");
os << endl;
return os;
}

/*****************************************************************************/

C_iopath& C_buf::iopath_out(C_iopath& p) const
{
    int res;
    res=p.write(buffer, dsize*4, 0);
    if (res!=dsize*4)
        throw new C_unix_error(errno, "C_buf::iopath_out");
    return p;
}

/*****************************************************************************/

ostream& operator <<(ostream& os, const C_buf& b)
{
return b.print(os);
}

/*****************************************************************************/

C_iopath& operator <<(C_iopath& p, const C_buf& b)
{
    return b.iopath_out(p);
}

/*****************************************************************************/
/*****************************************************************************/
