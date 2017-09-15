/*
 * proc_communicator.hxx
 * 
 * created: 11.06.97 PW
 * 25.03.1998 PW: adapded for <string>
 * 12.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#ifndef _proc_communicator_hxx_
#define _proc_communicator_hxx_

#include "config.h"
#include "cxxcompat.hxx"
#include <sys/time.h>
#include <nocopy.hxx>

class C_confirmation;

/*****************************************************************************/

class C_communicator : public nocopy
  {
  public:
    C_communicator();
    ~C_communicator();
    typedef void (*pbackproc)(void*,int, int, int);
    typedef void (*mbackproc)(void*);
    pbackproc pcallback;
    void* pbackdata;
    mbackproc mcallback;
    void* mbackdata;
  private:
    static int counter;
    typedef enum {none, local, tcp, decnet} typ;
    typ contyp;
    STRING host;
    STRING socket;
    int port;
    int path_; //nur fuer callback verwendet
    struct timeval deftimeout_;
    static void ccallback(int, int, int, void*);
    void ccallbackm(int, int, int);
  public:
    void init();
    void init(const STRING& socket);
    void init(const STRING& host, int port);
    void done();
    int valid() const;
    int path() const;
    void name(ostream&) const;
    pbackproc installpcallback(pbackproc, void* data);
    mbackproc installmcallback(mbackproc, void* data);
    C_confirmation* GetVConf(int xid, int ved, struct timeval *timeout=0);
    C_confirmation* GetVConf(int ved, struct timeval *timeout=0);
    C_confirmation* GetConf(int xid, struct timeval *timeout=0);
    C_confirmation* GetConf(struct timeval *timeout=0);
    int deftimeout(int);
    int deftimeout() const {return deftimeout_.tv_sec;}
    const struct timeval* getdeftimeout() const {return &deftimeout_;}
  };

ostream& operator <<(ostream&, const C_communicator&);

extern C_communicator communication;

#endif

/*****************************************************************************/
/*****************************************************************************/
