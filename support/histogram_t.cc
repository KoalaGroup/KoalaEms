/*
 * histogram_t.cc
 * 
 * created 03.06.1998 PW
 * 05.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#include "config.h"
//#include "cxxcompat.hxx"
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "versions.hxx"

#include <stdarg.h>
#include <math.h>
#include "compat.h"
#include "histogram_t.hxx"
#include "typinfo.hxx"

#ifndef XVERSION
VERSION("Jun 11 1998", __FILE__, __DATE__, __TIME__)
static char* rcsid="$Id: histogram_t.cc,v 2.4 2003/09/11 12:05:33 wuestner Exp $";
#define XVERSION
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
// usage: C_histogram(name, dim, min1, max1, bins1, min2, max2, bins2...)
template <class T>
histogram<T>::histogram(char* name, int dim, ...)
:C_histogram(name)
{
//cerr<<"histogram<T>::histogram<"<<typen::type_name(T(0))<<">(...), seq="
//    <<seq()<<endl;
int d, i;
dimension_=dim;
minimum_=new double[dimension_];
maximum_=new double[dimension_];
fact_=new double[dimension_];
bins_=new int[dimension_];
axes_=new STRING[dimension_];
array_=0;
if (minimum_==0||maximum_==0||fact_==0||bins_==0||axes_==0)
  {
  cerr<<"alloc short fields for C_histogram >"<<name<<"<: "<<strerror(errno)<<endl;
  clear();
  throw(0);
  }
va_list argp;
va_start(argp, dim);

arrsize_=1;
for (d=0; d<dimension_; d++)
  {
  minimum_[d]=va_arg(argp, double);
  maximum_[d]=va_arg(argp, double);
  bins_[d]=va_arg(argp, int);
  arrsize_*=(bins_[d]+2);
  fact_[d]=bins_[d]/(maximum_[d]-minimum_[d]);
  }
va_end(argp);

array_=new T[arrsize_];
if (array_==0)
  {
  cerr<<"alloc array for C_histogram >"<<name<<"< ("<<arrsize_*sizeof(T)
      <<" bytes): "<<strerror(errno)<<endl;
  }
for (i=0; i<arrsize_; i++) array_[i]=0;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
template <class T>
histogram<T>::histogram()
:C_histogram(""), array_(0)
{
//cerr<<"histogram<T>::histogram<"<<typen::type_name(T(0))<<">(), seq="
//    <<seq()<<endl;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
template <class T>
void histogram<T>::clear()
{
delete[] array_; array_=0;
C_histogram::clear();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
template <class T>
C_outbuf& histogram<T>::savedata(C_outbuf& b) const
{
b.putopaque(array_, arrsize_*sizeof(T));
return b;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
template <class T>
int histogram<T>::restoredata(C_inbuf& b)
{
arrsize_=1;
for (int i=0; i<dimension_; i++)
  {
  b>>minimum_[i];
  b>>maximum_[i];
  b>>bins_[i];
  b>>axes_[i];
  arrsize_*=(bins_[i]+2);
  fact_[i]=bins_[i]/(maximum_[i]-minimum_[i]);
  }
array_=new T[arrsize_];

b.getopaque(array_, arrsize_*sizeof(T));
return b;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
template <class T>
void histogram<T>::operator=(const C_histogram& x)
{
clear();
name_=x.name();
dimension_=x.dimension();
minimum_=new double[dimension_];
maximum_=new double[dimension_];
fact_=new double[dimension_];
bins_=new int[dimension_];
axes_=new STRING[dimension_];
arrsize_=1;
for (int i=0; i<dimension_; i++)
  {
  minimum_[i]=x.minimum(i);
  maximum_[i]=x.maximum(i);
  bins_[i]=x.bins(i);
  axes_[i]=x.axes(i);
  arrsize_*=(bins_[i]+2);
  fact_[i]=bins_[i]/(maximum_[i]-minimum_[i]);
  }
array_=new T[arrsize_];
if (x.type_float())
  for (int j=0; j<arrsize_; j++) array_[j]=(T)x.fget_lin(j);
else
  for (int j=0; j<arrsize_; j++) array_[j]=(T)x.iget_lin(j);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
template <class T>
C_histogram* histogram<T>::clone() const
{
histogram<T>* x=new histogram<T>;
x->name(name_);
x->dimension_=dimension_;
x->minimum_=new double[x->dimension_];
x->maximum_=new double[x->dimension_];
x->fact_=new double[x->dimension_];
x->bins_=new int[x->dimension_];
x->axes_=new STRING[x->dimension_];
x->arrsize_=arrsize_;
x->array_=new T[arrsize_];
for (int i=0; i<dimension_; i++)
  {
  x->minimum_[i]=minimum_[i];
  x->maximum_[i]=maximum_[i];
  x->bins_[i]=bins_[i];
  x->axes_[i]=axes_[i];
  x->fact_[i]=fact_[i];
  }
for (int j=0; j<arrsize_; j++) x->array_[j]=array_[j];
return x;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
template <class T>
ostream& histogram<T>::dump(ostream& os) const
{
os<<(*this);
os<<"fact: ";
for (int i=0; i<dimension_; i++) os<<"  "<<fact_[i];
os<<endl;
os<<"arrsize="<<arrsize_<<endl;
if ((typen::type_id((T)0)==typen::i8_type) || (typen::type_id((T)0)==typen::ui8_type))
  for (int j=0; j<arrsize_; j++) os<<" ["<<j<<"]="<<(int)(array_[j]);
else
  for (int j=0; j<arrsize_; j++) os<<" ["<<j<<"]="<<array_[j];
os<<endl;
return os;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
template <class T>
ostream& histogram<T>::print(ostream& os) const
{
C_histogram::print(os);
os<<"histogram<T>::this="<<(void*)this<<"; arr="<<(void*)array_<<endl;
return os;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
template <class T>
histogram<T>::~histogram()
{
//cerr<<"C_histogram<T>::~C_histogram()"<<" name="<<name_<<", "<<ich("this")<<endl;
clear();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
template <class T>
int histogram<T>::diff(const C_histogram& h) const
{
//cerr<<"<<<diff>>>"<<endl<<"  self:"<<endl<<(*this)<<"  other:"<<endl<<h<<endl;
cerr<<"<<<diff>>>"<<endl;

int err=0;
if (name_!=h.name())
  {
  cerr<<"### diff: name: "<<name_<<" <--> "<<h.name()<<endl;
  err=-1;
  }
if (dimension_!=h.dimension())
  {
  cerr<<"### diff: dimension: "<<dimension_<<" <--> "<<h.dimension()<<endl;
  return -1;
  }
for (int i=0; i<dimension_; i++)
  {
  if (minimum_[i]!=h.minimum(i))
    {
    cerr<<"### diff: minimum["<<i<<"]: "<<minimum_[i]<<" <--> "<<h.minimum(i)<<endl;
    err=-1;
    }
  if (maximum_[i]!=h.maximum(i))
    {
    cerr<<"### diff: maximum["<<i<<"]: "<<maximum_[i]<<" <--> "<<h.maximum(i)<<endl;
    err=-1;
    }
  if (bins_[i]!=h.bins(i))
    {
    cerr<<"### diff: bins["<<i<<"]: "<<bins_[i]<<" <--> "<<h.bins(i)<<endl;
    err=-1;
    }
  }
if (type_id()!=h.type_id())
  {
  cerr<<"### diff: type: "<<type_name()<<" <--> "<<h.type_name()<<endl;
  return -1;
  }
histogram<T>* hh=(histogram<T>*)&h;
for (int j=0; j<arrsize_; j++)
  {
  if (array_[j]!=hh->array_[j])
    {
    cerr<<"### diff: array["<<j<<"]: "<<array_[j]<<" <--> "<<hh->array_[j]<<endl;
    return -1;
    }
  }
if (err)
  cerr<<"### diff not OK ###"<<endl;
else
  cerr<<"### diff OK ###"<<endl;
return err;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
template <class T>
ostream& operator <<(ostream& os, const histogram<T>& h)
{
return h.print(os);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
#if 0
#if defined (__STD_STRICT_ANSI) || defined (__GNUC__)
//#if defined (__GNUC__)
template class histogram<char>;
template class histogram<unsigned char>;
template class histogram<short>;
template class histogram<unsigned short>;
template class histogram<int>;
template class histogram<unsigned int>;
template class histogram<long>;
template class histogram<unsigned long>;
template class histogram<float>;
template class histogram<double>;
//template class C_outbuf& operator<< <float>(C_outbuf&, const histogram<float>&);
//template class C_inbuf& operator>> <float>(C_inbuf&, C_histogram<float>&);
#else
//#pragma define_template C_histogram<int>
//#pragma define_template C_histogram<long>
//#pragma define_template C_histogram<float>
//#pragma define_template C_histogram<double>
//#pragma define_template operator>> <float>
#endif
#endif
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
