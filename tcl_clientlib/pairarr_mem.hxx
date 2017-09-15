/*
 * pairarr_mem.hxx
 * 
 * $ZEL: pairarr_mem.hxx,v 1.5 2004/11/18 12:35:57 wuestner Exp $
 * 
 * created 13.09.96 PW
 */

#ifndef _pairarr_mem_hxx_
#define _pairarr_mem_hxx_

#include "config.h"
#include "cxxcompat.hxx"
#include "pairarr.hxx"

/*****************************************************************************/

class C_pairarr_mem: public C_pairarr
  {
  public:
    C_pairarr_mem(const char* file);
    C_pairarr_mem(int initialsize, int increment, int maxsize, const char* file);
    virtual ~C_pairarr_mem();
  protected:
    STRING filename;
    vt_pair* arr;
    int arrlimit; // max. Groesse ist 2^arrlimit
    int arrmask;  // 2^arrlimit-1
    int arrsize;  // Gesamtgroesse des Arrays
    int idx_a;    // realer Index des ersten Elements
    int idx_e;    // realer Index des naechsten zu schreibenden Elements
    int numarrays; // Anzahl der Arrays, die auch wachsen
  public:
    virtual int checkidx(int);
    virtual int size();
    virtual int notempty() const {return arr!=0;}
    virtual int empty() const {return arr==0;}
    virtual void add(double, double);
    virtual void clear();
    virtual int limit() const;
    virtual void limit(int size);
    virtual void forcelimit();
    virtual const vt_pair* operator[](int);
    virtual int leftidx(double, int);
    virtual int rightidx(double, int);
    virtual double time(int);
    virtual double maxtime();
    virtual double val(int);
    virtual int getmaxxvals(double&, double&);
    virtual void flush() {}
    virtual void print(ostream&) const;
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
