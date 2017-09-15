/*
 * sockets.hxx
 * 
 * created 25.03.95 PW
 * 16.03.1998 PW: operator<<(ostream&,sockaddr&) added
 * 16.03.1998 PW: adapted for <string>
 * 25.03.1998 PW: sys/types.h included (for NetBSD)
 * 05.03.1999 PW: sock::close added
 * 07.Apr.2003 PW: use cxxcompat.hxx
 * 07.Apr.2003 PW: accept renamed to sock_accept
 */

#ifndef _sockets_hxx_
#define _sockets_hxx_

#include "config.h"
#include "cxxcompat.hxx"
#include <sys/types.h>
#include <sys/socket.h>

/*****************************************************************************/

class sock
  {
  public:
    sock(STRING);
    sock(int, STRING);
    virtual ~sock();
  protected:
    STRING name_;
    int path_;
  public:
    int path() const {return(path_);}
    virtual void create()=0;
    void listen();
    virtual void ndelay()=0;
    const STRING& name() const {return(name_);}
    virtual sock* sock_accept(int)=0;
    void close();
    operator int() {return path_;}
  };

class tcp_socket: public sock
  {
  public:
    tcp_socket(STRING);
    tcp_socket(int, STRING);
    virtual ~tcp_socket();
    virtual void create();
    void bind(short port);
    void connect(const char* host, short port);
    virtual void ndelay();
    virtual sock* sock_accept(int=0);
  };

class unix_socket: public sock
  {
  public:
    unix_socket(STRING);
    unix_socket(int, STRING);
    virtual ~unix_socket();
  protected:
    STRING fname;
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
