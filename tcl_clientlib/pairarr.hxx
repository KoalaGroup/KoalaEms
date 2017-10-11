/*
 * pairarr.hxx
 * 
 * $ZEL: pairarr.hxx,v 1.6 2014/07/14 15:13:26 wuestner Exp $
 * 
 * created 08.09.96
 */

#ifndef _pairarr_hxx_
#define _pairarr_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>

using namespace std;

/*****************************************************************************/

struct vt_pair {
    double time;
    double val;
};

class C_pairarr {
  public:
    C_pairarr();
    virtual ~C_pairarr();
  protected:
    int valid_;
    string errormessage;
  public:
    int invalid() const {return !valid_;}
    const string& errormsg() const {return errormessage;}
    virtual int checkidx(int) =0;
    virtual int size() =0;
    virtual int notempty() const =0;
    virtual int empty() const =0;
    virtual void add(double, double) =0;
    virtual void clear() =0;
    virtual int limit() const =0;
    virtual void limit(int size) =0;
    virtual void forcelimit() =0;
    virtual const vt_pair* operator[](int) =0;
    virtual int leftidx(double, int) =0;
    virtual int rightidx(double, int) =0;
    virtual double time(int) =0;
    virtual double maxtime() =0;
    virtual double val(int) =0;
    virtual int getmaxxvals(double&, double&) =0;
    virtual void flush() =0;
    virtual void print(ostream&) const =0;
};

#endif

/*****************************************************************************/
/*****************************************************************************/
