/*
 * xferbuf.cc
 * 
 * created 26.07.95 PW
 * 05.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>

#include "xferbuf.hxx"
#include "errors.hxx"
#include "compat.h"
#include "versions.hxx"

VERSION("Jun 06 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: xferbuf.cc,v 2.11 2006/11/27 10:47:59 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_sender::C_sender()
:path(-1)
{}

/*****************************************************************************/

C_sender::C_sender(int path)
:path(path)
{}

/*****************************************************************************/

C_sender::~C_sender()
{}

/*****************************************************************************/

void C_sender::setpath(int path_)
{
path=path_;
}

/*****************************************************************************/

void C_sender::xsend(char *block, int size)
{
int res=0, idx=0;

while((idx<size) && (res!=-1))
  {
  res=write(path, block+idx, size-idx);
  if (res>=0)
    idx+=res;
  else if (errno==EINTR)
    res=0;
  }
if (res<0)
  throw new C_unix_error(errno, "C_sender::xsend");
}

/*****************************************************************************/

void C_sender::send(C_outbuf& ob)
{
int h, size;
ems_u32* buffer;
// init
size=ob.size();
buffer=ob.getbuffer();
try
  {
  // send length
  h=htonl(size);
  xsend(reinterpret_cast<char*>(&h), sizeof(int));
  // send block
  for (h=0; h<size; h++) buffer[h]=htonl(buffer[h]);
  xsend(reinterpret_cast<char*>(buffer), size*sizeof(int));
  delete[] buffer;
  }
catch(C_error*)
  {
  // zusaetzliche Behandlung?
  delete[] buffer;
  throw;
  }
catch (...)
  {
  cerr << "caught ... in C_sender::send in xferbuf.cc" << endl;
  exit(0);
  }
}

/*****************************************************************************/

C_receiver::C_receiver()
:path(-1)
{}

/*****************************************************************************/

C_receiver::C_receiver(int path)
:path(path)
{}

/*****************************************************************************/

C_receiver::~C_receiver()
{}

/*****************************************************************************/

void C_receiver::setpath(int path_)
{
path=path_;
}

/*****************************************************************************/

void C_receiver::xreceive(char *block, int size)
{
int res, idx=0;

while (idx<size)
  {
  res=read(path, block+idx, size-idx);
  if (res>0)
    idx+=res;
  else if (res<0)
    {
    if (errno!=EINTR)
      throw new C_unix_error(errno, "C_receiver::xreceive");
    }
  else /* res==0 */
      throw new C_unix_error(EPIPE, "C_receiver::xreceive");
  }
}

/*****************************************************************************/

C_inbuf* C_receiver::receive()
{
int size, h;
ems_u32* buffer;
C_inbuf* ib=0;

try
  {
  // receive length
  xreceive(reinterpret_cast<char*>(&size), sizeof(int));
  size=ntohl(size);
  // receive block
  ib=new C_inbuf(size);
  buffer=ib->wbuf();
  xreceive(reinterpret_cast<char*>(buffer), size*sizeof(int));
  for (h=0; h<size; h++) buffer[h]=ntohl(buffer[h]);
  }
catch (C_error*)
  {
  delete ib;
  throw;
  }
catch (...)
  {
  cerr << "caught ... in C_receiver::receive() in xferbuf.cc" << endl;
  exit(0);
  }
return(ib);
}

/*****************************************************************************/
/*****************************************************************************/
