/*
 * support/outbuf.hxx
 * 
 * created 09.11.94 PW
 * 
 * $ZEL: outbuf.hxx,v 2.19 2014/07/14 15:09:53 wuestner Exp $
 */

#ifndef _outbuf_hxx_
#define _outbuf_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <sys/time.h>

#include "buf.hxx"

using namespace std;

/*****************************************************************************/

class C_outbuf;

template<class A>
class C_outbuf_1
  {
  private:
    void (C_outbuf::*f) (A);
    A a;
  public:
    C_outbuf_1(void (C_outbuf::*ff)(A), A aa)
      :f(ff), a(aa) {}
    friend C_outbuf& operator<<(C_outbuf& ob, const C_outbuf_1<A>& m)
      {(ob.*(m.f))(m.a); return ob;}
  };

template<class A, class B> class C_outbuf_2
  {
  private:
    void (C_outbuf::*f)(A, B);
    A a;
    B b;
  public:
    C_outbuf_2(void (C_outbuf::*ff)(A, B), A aa, B bb)
        :f(ff), a(aa), b(bb) {}
    friend C_outbuf& operator<<(C_outbuf& ob, const C_outbuf_2<A, B>& m)
        {(ob.*(m.f))(m.a, m.b); return ob;}
  };

/*****************************************************************************/

class C_outbuf: public C_buf
  {
  //friend C_outbuf& mark(C_outbuf&);
  public:
    C_outbuf();
    C_outbuf(int);
    C_outbuf(int, int);
    //C_outbuf(C_buf&);
    C_outbuf(const C_buf&);
    C_outbuf operator=(C_outbuf&);
    C_outbuf operator=(const C_outbuf&);
    virtual ~C_outbuf();
  private:
    typedef C_outbuf& (*outbuf_manip) (C_outbuf&);
  protected:
    int marks[10];
  public:
    virtual int empty() const;
    int& operator * () const;
    C_outbuf& operator<<(int);
    C_outbuf& operator<<(unsigned int);
    C_outbuf& operator<<(short int);
    C_outbuf& operator<<(short unsigned int);
    //C_outbuf& operator<<(float);
    //C_outbuf& operator<<(double);
    C_outbuf& operator<<(char);
    C_outbuf& operator<<(const char*);
    C_outbuf& operator<<(const string&);
    C_outbuf& operator<<(stringstream&);
    C_outbuf& operator<<(ostringstream&);
    C_outbuf& operator<<(struct timeval);
    C_outbuf& operator<<(outbuf_manip);
    void do_space(int&);
    void do_space_for_counter(int);
    void do_set_counter(int);
    void putval(int idx, int val);
    void putintarr(const unsigned int* arr, int size);
    void putintarr(const int* arr, int size);
    void putchararr(const char* arr, int size);
    void putopaque(const void* arr, int size);
  };

/*****************************************************************************/

//C_outbuf& mark(C_outbuf&);

C_outbuf_1<int&> space_(int&);
C_outbuf_1<int> space_for_counter(int=0);
C_outbuf_1<int> set_counter(int=0);
C_outbuf_2<int, int> put(int, int);
C_outbuf_2<const int*, int> put(const int*, int);
C_outbuf_2<const unsigned int*, int> put(const unsigned int*, int);
C_outbuf_2<const char*, int> put(const char*, int);

#endif

/*****************************************************************************/
/*****************************************************************************/
