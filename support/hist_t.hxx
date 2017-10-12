/*
 * histogram_t.hxx
 * 
 * created 05.06.98 PW
 */

#ifndef _histogram_t_hxx_
#define _histogram_t_hxx_

#include "config.h"
//#include "cxxcompat.hxx"
#include "outbuf.hxx"
#include "inbuf.hxx"
#include "typinfo.hxx"

#ifndef HAVE_NAMESPACES
#define typen
#endif

template<class T>
class hist
  {
  public:
    histogram(char* name, int dims, .../* double min/max for each dimension */);
    histogram(char* name, double minx, double maxx);
    virtual ~histogram();
  protected:
    T *array_;
    virtual int restoredata(C_inbuf&);
    virtual C_outbuf& savedata(C_outbuf&) const;
  public:
    virtual const char* type_name() const {return typen::type_name((T)0);}
    virtual typen::type_ids type_id() const {return typen::type_id((T)0);}
    virtual int type_size() const {return sizeof(T);}
    virtual int type_float() const {return typen::is_float((T)0);}
            T& ref(double v0) {return array_[idx(v0)];}
            T& ref(int v0) {return array_[idx(v0)];}
    virtual void fill(double v0, double weight) {array_[idx(v0)]+=(T)weight;}
    virtual void fill(double v0, long weight) {array_[idx(v0)]+=(T)weight;}
    virtual void set(int bin0, double weight) {array_[idx(bin0)]=(T)weight;}
    virtual void set(int bin0, long weight) {array_[idx(bin0)]=(T)weight;}
    virtual long iget(int bin0) const {return (long)array_[idx(bin0)];}
    virtual double fget(int bin0) const {return (double)array_[idx(bin0)];}
    virtual long iget_lin(int i) const {return (long)array_[i];}
    virtual double fget_lin(int i) const {return (double)array_[i];}
    virtual void clear();
    virtual void operator=(const C_histogram&);
    virtual C_histogram* clone() const;
    virtual int diff(const C_histogram&) const;
    virtual ostream& print(ostream&) const;
    virtual ostream& dump(ostream&) const;
  };
