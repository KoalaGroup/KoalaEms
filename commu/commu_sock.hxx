/*
 * commu_sock.hxx
 * 
 * $ZEL: commu_sock.hxx,v 2.11 2014/07/14 15:12:19 wuestner Exp $
 * 
 * created before 30.07.93 PW
 */

#ifndef _commu_sock_hxx_
#define _commu_sock_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <sys/types.h>
#include <un.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/uio.h> /* wegen struct iovec */
#include <socket_prot.h> /* sys/socket.h, aber geschuetzt (wegen ultrix4.4) */
#include <debug.hxx>
#include <compat.h>

using namespace std;

/*****************************************************************************/
class C_socket;

ostream& operator << (ostream&, const C_socket&);

class C_socket
  {
  protected:
    int sock;
  public:
    C_socket():sock(-1) {}
    virtual ~C_socket();
    operator int() {return sock;}
    int path() {return sock;}
    virtual int far()=0;
    int listen(int num) {return ::listen(sock, num);}
    virtual int bind()=0;
    virtual C_socket* accept()=0;
    virtual void connect()=0;
    virtual int getstatus();
    virtual void close();
    virtual void connected()=0;
    virtual ostream& print(ostream&) const=0;
  };

/*****************************************************************************/

class C_unix_socket: public C_socket
  {
  protected:
    struct sockaddr_un addr;
    C_unix_socket(int);
    int owner;
  public:
    C_unix_socket();
    virtual ~C_unix_socket();
    virtual int far() {return(0);}
    virtual int bind();
    void setname(const string&);
    //void setname(const char*);
    virtual C_socket* accept();
    virtual void connect();
    virtual void connected(){}
    virtual ostream& print(ostream&) const;
  };

/*****************************************************************************/

class C_tcp_socket: public C_socket
  {
  protected:
    struct sockaddr_in addr;
    C_tcp_socket(int);
  public:
    C_tcp_socket();
    virtual ~C_tcp_socket();
    virtual int far() {return(1);};
    virtual int bind();
    //void setname(const char*, int);
    void setname(const string&, int);
    virtual C_socket* accept();
    virtual void connect();
    virtual void connected();
    virtual ostream& print(ostream&) const;
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
