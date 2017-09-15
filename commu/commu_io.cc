/*
 * commu_io.cc
 * 
 * created 29.07.94 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include <commu_log.hxx>
#include <swap.h>
#include <commu_io.hxx>
#include <fcntl.h>
#include "versions.hxx"

VERSION("2009-Feb-25", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_io.cc,v 2.13 2009/08/21 21:50:51 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

extern C_log elog, nlog, dlog;

/*****************************************************************************/

C_io::C_io()
:size(0), idx(0), status(iost_idle), error(0), message(0)
{
TR(C_io::C_io)
}

/*****************************************************************************/

C_io::~C_io()
{
TR(C_io::~C_io)
}

/*****************************************************************************/

const char* C_io::statstr()
{
static const char *str[]=
  {
  "iost_idle", "iost_ready", "iost_header", "iost_body", "iost_error",
  "iost_broken"
  };
if ((unsigned int)status>iost_error)
  return("fehler");
else
  return(str[status]);
}

/*****************************************************************************/

int C_io::read_it(int sock, int size, char* dest)
{
int res;

TR(C_io::read_it)
res=read(sock, &dest[idx], size-idx);
if (res==0)
  {
  DL(dlog << level(D_COMM) << "read_it: 0 bytes read." << flush;)
  error=EPIPE;
  return(-2);
  }
else if (res==-1)
  {
  if (errno==EINTR)
    res=0;
  else if (errno==EWOULDBLOCK)
    res=0;
  else
    {
    elog << "read_it(" << sock << "): " << strerror(errno) << flush;
    elog << "size=" << size << flush;
    error=errno;
    return(-1);
    }
  }
idx+=res;
return(0);
}

/*****************************************************************************/

int C_io::write_it(int sock, int size, char* source)
{
int res;

TR(C_io::write_it)
res=write(sock, &source[idx], size-idx);
if (res==0)
  {
  DL(dlog << level(D_COMM) << "write_it: 0 bytes written" << flush;)
  error=EPIPE;
  return(-2);
  }
else if (res==-1)
  {
  if (errno==EINTR)
    res=0;
  else if (errno==EWOULDBLOCK)
    {
    elog<<"C_io::write_it: EWOULDBLOCK"<<flush;
    res=0;
    }
  else
    {
    elog << "write_it(" << sock << "): errno=" << strerror(errno) << flush;
    elog<<"size="<<size<<flush;
    error=errno;
    return(-1);
    }
  }
idx+=res;
return(0);
}

/*****************************************************************************/

void C_io::read_(C_socket& socket)
{
int res;
int nochmal=1;

while (nochmal)
{
nochmal=0;
TR(C_io::read_)
switch (status)
  {
  case iost_idle:
    idx=0;
    status=iost_header;
    message=new C_message;
  case iost_header:
    {
    msgheader *header=&(message->header);
    if ((res=read_it(socket, sizeof(msgheader), (char*)header))<0)
      {
      delete message;
      if (res==-2)
        status=iost_broken;
      else
        status=iost_error;
      return;
      }
    if (idx==sizeof(msgheader))
      {
      if (socket.far()) swap_falls_noetig((ems_u32*)header, headersize);
      if (header->size==0)
        {
        status=iost_ready;
        }
      else
        {
        size=header->size*4;
        idx=0;
        status=iost_body;
        message->body=new ems_u32[header->size];
        nochmal=1;
        }
      }
    break;
    }
//  nach headerlesen gleich weitermachen?
  case iost_body:
    {
    ems_u32* body=message->body;
    if ((res=read_it(socket, size, (char*)body))<0)
      {
      delete message;
      if (res==-2)
        status=iost_broken;
      else
        status=iost_error;
      return;
      }
    if (idx==size)
      {
      if (socket.far()) swap_falls_noetig(body, message->header.size);
      status=iost_ready;
      }
    break;
    }
  default:
    elog << "C_io::read_: status=" << statstr() << flush;
    status=iost_error;
    break;
  }
}
return;
}

/*****************************************************************************/

void C_io::write_(C_socket& socket, C_messagelist& messagelist)
{
int res;
int nochmal=1;

while (nochmal)
{
nochmal=0;
TR(C_io::write_)
switch (status)
  {
  case iost_idle:
    {
    if ((message=messagelist.extract_first())==0)
      {
      elog << "C_io::write_: nichts zu schreiben" << flush;
      return;
      }
    idx=0;

    size=message->header.size*4;
    if (socket.far())
      {
      if (size>0) swap_falls_noetig(message->body, message->header.size);
      swap_falls_noetig((ems_u32*)&message->header, headersize);
      }
    status=iost_header;
    }
    // no break !
  case iost_header:
    {
    msgheader *header;
    header=&message->header;
    if ((res=write_it(socket, sizeof(msgheader), (char*)header))<0)
      {
      delete message;
      if (res==-2)
        status=iost_broken;
      else
        status=iost_error;
      return;
      }
    if (idx==sizeof(msgheader))
      {
      if (size==0)
        {
        delete message;
        status=iost_idle;
        }
      else
        {
        idx=0;
        status=iost_body;
        nochmal=1;
        }
      }
    break;
    }
  case iost_body:
    {
    ems_u32 *body;
    body=message->body;
    if ((res=write_it(socket, size, (char*)body))<0)
      {
      delete message;
      if (res==-2)
        status=iost_broken;
      else
        status=iost_error;
      return;
      }
    if (idx==size)
      {
      delete message;
      status=iost_idle;
      }
    break;
  default:
    elog << "C_io::write_: status=" << statstr() << flush;
    status=iost_error;
    break;
    }
  }
}
return;
}/* C_io::write_(C_socket& socket, C_messagelist& messagelist) */

/*****************************************************************************/
/*****************************************************************************/
