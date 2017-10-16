/******************************************************************************
*                                                                             *
* statist.hxx                                                                 *
*                                                                             *
* OSF1/ULTRIX                                                                 *
*                                                                             *
* created: 19.08.96                                                           *
* last changed: 20.08.96                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#ifndef _statist_hxx_
#define _statist_hxx_

#include "config.h"
#include "cxxcompat.hxx"

/*****************************************************************************/

class statist
  {
  public:
    statist();
    //virtual ~statist();
  protected:
    int num;
    double sum;
    double qsum;
    int mdist, nextmark;
    void mark();
  public:
    int getnum() const {return num;}
    virtual void print(ostream&)=0;
    int markdist(int);
  };

class statist_int: public statist
  {
  public:
    statist_int() {}
    statist_int(const statist_int&);
    // virtual ~statist_int();
  protected:
    int min, max;
    int nmin, nmax;
  public:
    void add(int);
    void add(statist_int&);
    virtual void print(ostream&);
  };

class statist_float: public statist
  {
  public:
    statist_float() {}
    statist_float(const statist_float&);
    // virtual ~statist_float();
  protected:
    double min, max;
  public:
    void add(double);
    void add(statist_float&);
    virtual void print(ostream&);
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
