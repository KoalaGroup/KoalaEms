/*
 * xferbuf.hxx
 * 
 * created 26.07.95 PW
 */

#ifndef _xferbuf_hxx_
#define _xferbuf_hxx_

#include "outbuf.hxx"
#include "inbuf.hxx"

/*****************************************************************************/

class C_sender
  {
  public:
    C_sender();
    C_sender(int path);
    virtual ~C_sender();
  protected:
    int path;
    void xsend(char *block, int size);
  public:
    void setpath(int);
    void send(C_outbuf&);
  };

class C_receiver
  {
  public:
    C_receiver();
    C_receiver(int path);
    virtual ~C_receiver();
  protected:
    int path;
    void xreceive(char *block, int size);
  public:
    void setpath(int);
    C_inbuf* receive();
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
