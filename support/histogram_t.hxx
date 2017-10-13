/*
 * histogram_t.hxx
 * 
 * created 05.06.98 PW
 */

#ifndef _histogram_t_hxx_
#define _histogram_t_hxx_

#include "config.h"
////#include "cxxcompat.hxx"
#include "outbuf.hxx"
#include "inbuf.hxx"
#include "typinfo.hxx"

#ifndef HAVE_NAMESPACES
#define typen
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/

class C_histogram
  {
  public:
    C_histogram(STRING name);
    virtual ~C_histogram();
  private:
    static int sequence; 
    int sequence_; 
  protected:
    C_histogram(const C_histogram&); // copy construct forbidden
    virtual int restoredata(C_inbuf&)=0;
    virtual C_outbuf& savedata(C_outbuf&) const=0;
    int seq() const {return sequence_;} 
    
    int idx(double) const;
    int idx(int) const;
    STRING name_;
    int dimension_;
    double *minimum_, *maximum_;
    double *fact_;
    int *bins_, arrsize_;
    STRING *axes_;
  public:
            const STRING& name() const {return name_;}
            void name(const STRING& name) {name_=name;} 
    virtual const char* type_name() const=0;
    virtual typen::type_ids type_id() const=0;
    virtual int type_size() const=0;
    virtual int type_float() const=0;
            int dimension() const {return dimension_;}
            double minimum(int) const;
            double maximum(int) const;
            int bins(int) const;
            const STRING& axes(int) const;
    virtual void fill(double v0, double weight)=0;
    virtual void fill(double v0, long weight)=0;
    virtual void set(int bin0, double weight)=0;
    virtual void set(int bin0, long weight)=0;
    virtual long iget(int) const=0;
    virtual double fget(int) const=0;
    virtual long iget_lin(int) const=0;
    virtual double fget_lin(int) const=0;
    virtual void clear();
    virtual C_outbuf& save(C_outbuf&) const;
    static  C_histogram* restore(C_inbuf&);
    virtual C_histogram* clone() const=0;
    virtual void operator=(const C_histogram&)=0;
    virtual int diff(const C_histogram&) const=0;
    virtual ostream& print(ostream&) const;
    virtual ostream& dump(ostream&) const=0;
  };

ostream& operator <<(ostream&, const C_histogram&);
C_outbuf& operator <<(C_outbuf&, const C_histogram&);

template<class T>
class histogram: public C_histogram
  {
  public:
    histogram();
    histogram(char* name, int, ...);
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

template<class T>
ostream& operator <<(ostream&, const histogram<T>&);

#endif
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
