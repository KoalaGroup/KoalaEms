/*
 * buf.hxx
 * 
 * created 28.01.96 PW
 * 05.06.1998 PW: adapted for STD_STRICT_ANSI
 * 06.09.2000 PW: member "initial size" added
 */

#ifndef _buf_hxx_
#define _buf_hxx_

#include "config.h"
//#include "cxxcompat.hxx"
#include "iopathes.hxx"
#include "nocopy.hxx"

/*****************************************************************************/

class C_buf: public nocopy
  {
  friend C_buf& mark(C_buf&);
  public:
    C_buf();
    C_buf(int size, int incr=100);
    C_buf(const C_buf&);
    virtual ~C_buf();
  protected:
    ems_u32* buffer;
    int bsize;  // actual size of buffer
    int ibsize; // initial size of buffer
    int incr;   // increment for buffer
    int dsize;  // size of contained data
    int idx;    // index for get and put
    unsigned int* annobuf; // buffer for annotations
    int annosize;          // size of buffer for annotations
    //int marke;
    void grow(int);
    //void mark_it();
  public:
    void clear();
    int write(C_iopath&);
    int read(C_iopath&);
    const ems_u32* buf() const {return buffer;}
    //const unsigned int* ubuf() const {return (const unsigned int*)buffer;}
    //const ems_u32* idx_buf() const {return buffer+idx;}
    //const unsigned int* idx_ubuf() const {return (const unsigned int*)buffer+idx;}
    int index() const {return idx;}
    int index(int);
    ems_u32* getbuffer();                         // gibt Verwaltung ab
    int size() const {return dsize;}
    int size(int);
    int annotate(unsigned int);
    int annotation(int i) const {return annosize<=i?0:annobuf[i];}
    virtual int empty() const=0;
    ostream& print(ostream&) const;
    C_iopath& iopath_out(C_iopath& p) const;
    ems_u32& operator [] (int);
    ems_u32& operator [] (int) const;
    C_buf& operator ++();
    C_buf& operator ++(int);
    C_buf& operator --();
    C_buf& operator --(int);
    C_buf& operator +=(int);
    C_buf& operator -=(int);
  };

//C_buf& mark(C_buf&);
ostream& operator <<(ostream&, const C_buf&);
C_iopath& operator <<(C_iopath&, const C_buf&);

#endif

/*****************************************************************************/
/*****************************************************************************/
