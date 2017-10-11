/*
 * proclib/proc_communicator.hxx
 * 
 * created: 11.06.97 PW
 * 
 * $ZEL: proc_communicator.hxx,v 2.8 2014/07/14 15:11:53 wuestner Exp $
 */

#ifndef _proc_communicator_hxx_
#define _proc_communicator_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <sys/time.h>
#include <nocopy.hxx>

class C_confirmation;

using namespace std;

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
    string host;
    string socket;
    int port;
    int path_; //nur fuer callback verwendet
    struct timeval deftimeout_;
    static void ccallback(int, int, int, void*);
    void ccallbackm(int, int, int);
  public:
    void init();
    void init(const string& socket);
    void init(const string& host, int port);
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
