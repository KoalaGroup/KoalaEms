/******************************************************************************
*                                                                             *
* events.hxx                                                                  *
*                                                                             *
* OSF1/ULTRIX                                                                 *
*                                                                             *
* created 24.01.95                                                            *
* last changed 04.11.96                                                       *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#ifndef _events_hxx_
#define _events_hxx_

#include "config.h"
#include "cxxcompat.hxx"
#include <stdio.h>
#include <errors.hxx>
#include <nocopy.hxx>


//#define MAXEVENT (0x3fff) // 64 kByte -1 Word
//#define MAXEVENT (0xefff) // 240 kByte -1 Word

/*****************************************************************************/

class C_eventp;

// C_subeventp haelt nur Pointer auf Daten, Daten werden von C_eventp
// oder C_event verwaltet

class C_subeventp
  {
  public:
    C_subeventp();
    virtual ~C_subeventp();
  protected:
    int valid;
    int* subevent;
    int size_;
    int trace;
  public:
    int size() const {return(size_);}
    int dsize() const {return(size_-2);}
    int id() const {return subevent[0];}
    void id(int);
    virtual void store(const int*);
    int operator [](int i) const {return(subevent[i]);}
    int isvalid() const {return valid;}
    virtual void clear();
    int settrace(int);
  };

// C_subevent verwaltet Daten selbst

class C_subevent: public C_subeventp
  {
  public:
    C_subevent();
    virtual ~C_subevent();
  protected:
  public:
    virtual void store(const int*);
    virtual void clear();
  };

/*****************************************************************************/

class C_event_manipi
  {
  private:
    typedef C_eventp& (*event_manipi) (C_eventp&, int);
    event_manipi f;
    int arg;
  public:
    C_event_manipi(event_manipi, int);
    friend C_eventp& operator <<(C_eventp&, const C_event_manipi&);
    friend C_eventp& operator >>(C_eventp&, const C_event_manipi&);
  };

class C_event_manipir
  {
  private:
    typedef C_eventp& (*event_manipir) (C_eventp&, int&);
    event_manipir f;
    int& arg;
  public:
    C_event_manipir(event_manipir, int&);
    friend C_eventp& operator <<(C_eventp&, const C_event_manipir&);
    friend C_eventp& operator >>(C_eventp&, const C_event_manipir&);
  };

C_event_manipi setcopy(int);
C_event_manipi setignore(int);
C_eventp& clearcpyign(C_eventp&);

/*****************************************************************************/

// C_eventp healt nur Pointer auf Daten, Daten werden von C_evinout verwaltet

class C_eventp
  {
  public:
    C_eventp();
    C_eventp(C_eventp&);
    virtual ~C_eventp();
  protected:
    int valid_;
    int* event;
    int size_;
    int idx;
    int num_ignore;
    int* to_ignore;
    int num_copy;
    int* to_copy;
    int nextsubevent();
    int lnr;
    int trace;
  public:
    void operator=(C_eventp&);
    virtual C_eventp& operator<<(const C_subeventp&); // illegal
//    C_event& operator<<(event_manip);
    virtual C_eventp& operator>>(C_subeventp&);
//    C_event& operator>>(event_manip);
    operator int() const;
    int flush();
    int levnr() const {return(lnr);}
    virtual int evnr() const;
    virtual int evnr(int);
    virtual int trignr() const;
    virtual int trignr(int);
    int sevnr() const;
    virtual void clear() {valid_=0;}
    void setcopy(int);
    void setignore(int);
    void clearcpyign();
    void reset();
    virtual void store(const int*);
    int size() const {return(size_);}
    int operator [](int i) const {return(event[i]);}
    int* operator + (int i) const {return(event+i);}
    const int* buffer() const {return(event);}
    int empty() const;
    virtual void read_usr(FILE*);
    virtual void write_usr(FILE*);
    int valid() const {return valid_;}
    int settrace(int);
    int getsubevent(C_subeventp& sev, int id);
  };

// C_event verwaltet Daten selbst

class C_event: public C_eventp
  {
  public:
    C_event();
    C_event(C_event&);
    virtual ~C_event();
  public:
    void operator=(C_event&);
    virtual C_eventp& operator<<(const C_subeventp&);
    virtual void store(const int*);
    virtual int evnr() const;
    virtual int evnr(int);
    virtual int trignr() const;
    virtual int trignr(int);
    virtual void read_usr(FILE*);
  };

/*****************************************************************************/

// base class

class C_evput: public nocopy
  {
  public:
    C_evput(const char* name, int maxevent) :name_(name), maxevent_(maxevent),
        trace(0) {}
    C_evput(const STRING& name, int maxevent) :name_(name), maxevent_(maxevent),
        trace(0) {}
  protected:
    STRING name_;
    int maxevent_;
    int trace;
  public:
    STRING name() const {return name_;}
    int settrace(int);
  };

class C_evinput: public C_evput
  {
  public:
    C_evinput(const char* name, int maxevent);
    C_evinput(const STRING& name, int maxevent);
    C_evinput(C_evinput&);
    virtual ~C_evinput();
  protected:
    int status;
    int* eventbuf;
    typedef enum{unknown, yes, no} wendmod;
    wendmod lastwend;
    int maxexpect;
  public:
    operator int() const {return(status);}
    virtual C_evinput& operator>>(C_eventp&)=0;
    virtual int fatal() const=0;
    virtual int is_tape() const {return(0);}
    virtual void reset();
    void setmaxexpect(int);
  };

ostream& operator <<(ostream&, const C_evinput&);

// unbuffered input (via pathdescriptor)

class C_evpinput: public C_evinput
  {
  public:
    C_evpinput(const STRING&, int maxevent);
    C_evpinput(const char*, int maxevent);
    C_evpinput(int path, int maxevent);
    virtual ~C_evpinput();
  protected:
    int path;
    void xrecv(int*, int);
  public:
    virtual C_evinput& operator>>(C_eventp&);
    virtual int fatal() const;
  };

// input from tape

class C_evtinput: public C_evinput
  {
  public:
    C_evtinput(const STRING&, int maxevent);
    C_evtinput(const char*, int maxevent);
    C_evtinput(int path, int maxevent);
    virtual ~C_evtinput();
  protected:
    int path;
    int blocksize, idx, maxidx, last_nr;
    int filemark_, fatal_;
    int readrecord(int*);
  public:
    virtual C_evinput& operator>>(C_eventp&);
    virtual int fatal() const;
    int filemark() const {return(filemark_);}
    int windfile(int);
    int windrecord(int);
    int rewind();
    virtual int is_tape() const {return(1);}
  };

// buffered input (via filepointer)

class C_evfinput: public C_evinput
  {
  public:
    C_evfinput(const STRING&, int maxevent);
    C_evfinput(const char*, int maxevent);
    C_evfinput(FILE*, int maxevent);
    C_evfinput(int path, int maxevent);
    virtual ~C_evfinput();
  protected:
    FILE* file;
    void xrecv(int*, int);
  public:
    virtual C_evinput& operator>>(C_eventp&);
    virtual int fatal() const;
  };

/*****************************************************************************/

// base class

class C_evoutput: public C_evput
  {
  public:
    C_evoutput(const STRING&, int maxevent);
    C_evoutput(const char*, int maxevent);
    virtual ~C_evoutput();
  protected:
    int swap_;
    int* buffer;
  public:
    typedef C_evoutput& (*evoutput_manip) (C_evoutput&);
    C_evoutput& operator <<(evoutput_manip);
    virtual C_evoutput& operator <<(const C_eventp&)=0;
    operator void*();
    virtual void flush() {}
    int swap() const {return(swap_);}
    virtual int swap(int);
  };

C_evoutput& flush(C_evoutput&);

// unbuffered output (via pathdescriptor)

class C_evpoutput: public C_evoutput
  {
  public:
    C_evpoutput(const STRING&, int maxevent);
    C_evpoutput(const char*, int maxevent);
    C_evpoutput(int, int maxevent);
    virtual ~C_evpoutput();
  protected:
    int path;
    int xsend(const int*, int);
  public:
    virtual C_evoutput& operator<<(const C_eventp&);
  };

// output to tape

class C_evtoutput: public C_evoutput
  {
  public:
    C_evtoutput(const STRING&, int maxevent);
    C_evtoutput(const char*, int maxevent);
    C_evtoutput(int path, int maxevent);
    virtual ~C_evtoutput();
  protected:
    int path;
    int idx;
    int writerecord(const int*, int);
  public:
    virtual C_evoutput& operator<<(const C_eventp&);
    virtual void flush();
    virtual int swap(int);
  };

// buffered output (via filepointer)

class C_evfoutput: public C_evoutput
  {
  public:
    C_evfoutput(const STRING&, int maxevent);
    C_evfoutput(const char*, int maxevent);
    C_evfoutput(FILE* file, int maxevent);
    C_evfoutput(int path, int maxevent);
    virtual ~C_evfoutput();
  protected:
    FILE* file;
    int xsend(const int*, int);
  public:
    virtual C_evoutput& operator<<(const C_eventp&);
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
