/*
 * histogram.cc
 * 
 * created 03.06.1998 PW
 * 05.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#include "config.h"
//#include "cxxcompat.hxx"
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "errors.hxx"
#include "versions.hxx"

#ifdef HAVE_TYPEINFO
#include <typeinfo>
#endif
#include <stdarg.h>
#include <math.h>
#include "compat.h"
#include "histogram_t.hxx"

VERSION("Jun 11 1998", __FILE__, __DATE__, __TIME__)
static char* rcsid="$Id: histogram.cc,v 2.5 2004/11/26 14:45:34 wuestner Exp $";
#define XVERSION

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
int C_histogram::sequence=0;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
C_histogram::C_histogram(STRING name)
:name_(name), dimension_(0), minimum_(0), maximum_(0), fact_(0), bins_(0),
    arrsize_(0), axes_(0)
{
sequence_=sequence++;
//cerr<<"C_histogram::ctor, seq="<<sequence_<<", this="<<(void*)this<<endl;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
C_histogram::~C_histogram()
{
clear();
//cerr<<"C_histogram::dtor, seq="<<sequence_<<", this="<<(void*)this<<endl;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
void C_histogram::clear()
{
delete[] minimum_; minimum_=0;
delete[] maximum_; maximum_=0;
delete[] fact_; fact_=0;
delete[] bins_; bins_=0;
delete[] axes_; axes_=0;
dimension_=0;
arrsize_=0;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
C_outbuf& C_histogram::save(C_outbuf& b) const
{
int i;
b<<name_;
b<<type_size();
b<<type_id();
b<<type_name();
b<<dimension_;
for (i=0; i<dimension_; i++)
  {
  b<<(double)minimum_[i];
  b<<(double)maximum_[i];
  b<<bins_[i];
  b<<axes_[i];
  }
savedata(b);
return b;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
C_histogram* C_histogram::restore(C_inbuf& b)
{
STRING name, tname;
int tsize, t;
typen::type_ids tid;

b>>name;
b>>tsize;
b>>t;
tid=(typen::type_ids)t;
b>>tname;
C_histogram* x;

switch (tid)
  {
  case typen::  i8_type: x=new histogram<char>()          ; break;
  case typen:: ui8_type: x=new histogram<unsigned char>() ; break;
  case typen:: i16_type: x=new histogram<short>()         ; break;
  case typen::ui16_type: x=new histogram<unsigned short>(); break;
  case typen:: i32_type: x=new histogram<int>()           ; break;
  case typen::ui32_type: x=new histogram<unsigned int>()  ; break;
  case typen:: i64_type: x=new histogram<long>()          ; break;
  case typen::ui64_type: x=new histogram<unsigned long>() ; break;
  case typen:: f32_type: x=new histogram<float>()         ; break;
  case typen:: f64_type: x=new histogram<double>()        ; break;
  default:
    cerr<<"C_histogram<T>::restore: typ "<<tid<<" ("<<tname<<") not implemented"<<endl; 
    x=0;
  }
if (x==0) return 0;
x->name_=name;
b>>x->dimension_;
x->minimum_=new double[x->dimension_];
x->maximum_=new double[x->dimension_];
x->fact_=new double[x->dimension_];
x->bins_=new int[x->dimension_];
x->axes_=new STRING[x->dimension_];
x->restoredata(b);
return x;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
ostream& C_histogram::print(ostream& os) const
{
os<<"C_histogram >"<<name_<<"< dim="<<dimension_<<" type="<<type_name()<<endl;
os<<"  min, max, bins:"<<endl;
for (int d=0; d<dimension_; d++)
  {
  os<<"  ["<<d<<"]: "<<minimum_[d]<<" -- "<<maximum_[d]<<", "<<bins_[d]<<endl;
  }
os<<"C_histogram::this ="<<(void*)this<<", seq="<<sequence_<<endl;
return os;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
double C_histogram::minimum(int i) const
{
if ((i<0) || (i>=dimension_)) throw 0;
return minimum_[i];
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
double C_histogram::maximum(int i) const
{
if ((i<0) || (i>=dimension_)) throw 0;
return maximum_[i];
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
int C_histogram::bins(int i) const
{
if ((i<0) || (i>=dimension_)) throw 0;
return bins_[i];
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
const STRING& C_histogram::axes(int i) const
{
if ((i<0) || (i>=dimension_)) throw 0;
return axes_[i];
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
int C_histogram::idx(double v0) const
{
if (dimension_!=1)
  {
  ostrstream os;
  os<<"C_histogram::idx(double "<<v0<<"): dim="<<dimension_;
  throw new C_program_error(os);
  }
int x0;
if (v0<minimum_[0])
  x0=0;
else if (v0>maximum_[0])
  x0=bins_[0]+1;
else
  x0=(int)((v0-minimum_[0])*fact_[0])+1;
return x0;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
int C_histogram::idx(int b0) const
{
if (dimension_!=1)
  {
  ostrstream os;
  os<<"C_histogram::idx(int "<<b0<<"): dim="<<dimension_;
  throw new C_program_error(os);
  }
int x0;
if (b0<0)
  x0=0;
else if (b0>=bins_[0])
  x0=bins_[0]+1;
else
  x0=b0+1;
return x0;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
// T& C_histogram<T>::ref(double v0, double v1)
// {
// if (dimension_!=2) throw(0);
// int x0, x1;
// if (v0<minimum_[0])
//   x0=0;
// else if (v0>maximum_[0])
//   x0=bins_[0]+1;
// else
//   x0=(int)((v0-minimum_[0])*fact_[0])+1;
// if (v1<minimum_[1])
//   x1=0;
// else if (v1>maximum_[1])
//   x1=bins_[1]+1;
// else
//   x1=(int)((v1-minimum_[1])*fact_[1])+1;
// int x=x0+(bins_[0]+2)*x1;
// return array_[x];
// }
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
// T& C_histogram<T>::ref(int b0, int b1)
// {
// if (dimension_!=2) throw(0);
// int x0, x1;
// if (b0<0)
//   x0=0;
// else if (b0>=bins_[0])
//   x0=bins_[0]+1;
// else
//   x0=b0+1;
// if (b1<0)
//   x1=0;
// else if (b1>=bins_[1])
//   x1=bins_[1]+1;
// else
//   x1=b1+1;
// int x=x0+(bins_[0]+2)*x1;
// return array_[x];
// }
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
C_outbuf& operator <<(C_outbuf& b, const C_histogram& h)
{
return h.save(b);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
ostream& operator <<(ostream& os, const C_histogram& h)
{
return h.print(os);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
