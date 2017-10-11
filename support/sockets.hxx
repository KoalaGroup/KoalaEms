/*
 * support/sockets.hxx
 * 
 * created 25.03.95 PW
 * 
 * $ZEL: sockets.hxx,v 2.12 2014/07/14 15:09:53 wuestner Exp $
 */

#ifndef _sockets_hxx_
#define _sockets_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>

using namespace std;

/*****************************************************************************/

class sock
  {
  public:
    sock(string);
    sock(int, string);
    virtual ~sock();
  protected:
    string name_;
    int path_;
  public:
    int path() const {return(path_);}
    virtual void create()=0;
    void listen();
    virtual void ndelay()=0;
    const string& name() const {return(name_);}
    virtual sock* sock_accept(int)=0;
    void close();
    operator int() {return path_;}
  };

class tcp_socket: public sock
  {
  public:
    tcp_socket(string);
    tcp_socket(int, string);
    virtual ~tcp_socket();
    virtual void create();
    void bind(unsigned short port);
    void connect(const char* host, unsigned short port);
    virtual void ndelay();
    virtual sock* sock_accept(int=0);
  };

class unix_socket: public sock
  {
  public:
    unix_socket(string);
    unix_socket(int, string);
    virtual ~unix_socket();
  protected:
    string fname;
  public:
    virtual void create();
    void bind(const char* name);
    void connect(const char* name);
    virtual void ndelay();
    virtual sock* sock_accept(int=0);
  };

ostream& operator <<(ostream&, const struct sockaddr&);

#endif

/*****************************************************************************/
/*****************************************************************************/
