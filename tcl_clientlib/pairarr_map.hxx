/*
 * pairarr_map.hxx
 *
 * $ZEL: pairarr_map.hxx,v 1.3 2004/11/18 12:35:56 wuestner Exp $
 *
 * created 14.09.96 PW
 */

#ifndef _pairarr_map_hxx_
#define _pairarr_map_hxx_
#include <pairarr.hxx>

/*****************************************************************************/

class C_pairarr_map: public C_pairarr {
  public:
    C_pairarr_map(const char* file);
    C_pairarr_map(int initialsize, int increment, int maxsize, const char* file);
    virtual ~C_pairarr_map();
  protected:
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
    virtual int notempty() const;
    virtual int empty() const;
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
    virtual void flush();
    virtual void print(ostream&) const;
};

#endif

/*****************************************************************************/
/*****************************************************************************/
