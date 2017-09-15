/*
 * commu_message.cc
 * 
 * created 25.07.94 PW
 * 31.08.1998 PW: C_message::C_message(const msgheader& header, int *body)
 */

#include "config.h"
#include <string.h>
#include "debug.hxx"
#include "commu_message.hxx"
#include <versions.hxx>

VERSION("Aug 31 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_message.cc,v 2.15 2004/11/26 15:14:24 wuestner Exp $")
#define XVERSION

/*****************************************************************************/
C_message::C_message(const msgheader* header, ems_u32 *body)
:header(*header), body(body)
{
}
/*****************************************************************************/
C_message::C_message(const msgheader& header, ems_u32 *body)
:header(header), body(body)
{
}
/*****************************************************************************/
C_message::C_message(C_message& message)
:header(message.header)
{
    unsigned int i;
    body=new ems_u32[header.size];
    for (i=0; i<header.size; i++) body[i]=message.body[i];
}
/*****************************************************************************/
C_message::C_message()
:body(0)
{
//bzero((char*)&header, sizeof(msgheader));
memset((char*)&header, 0, sizeof(msgheader));
}
/*****************************************************************************/
C_message::~C_message()
{
delete[] body;
}
/*****************************************************************************/
ostream& C_message::print(ostream& os) const
{
os<<header.transid;
return os;
}
/*****************************************************************************/
ostream& operator <<(ostream& os, const C_message& rhs)
{
rhs.print(os);
return os;
}
/*****************************************************************************/
C_messagelist::C_messagelist(int size, int min, int max)
:C_list<C_message>(size, "messagelist"),
    lowwatermark(min), highwatermark(max), stopped(0)
{
TR(C_messagelist::C_messagelist)
}
/*****************************************************************************/
int C_messagelist::add(C_message* message)
{
TR(C_messagelist::add)
if (stopped)
  {
  if (firstfree>lowwatermark)
    return(-1);
  else
    stopped=0;
  }
else
  {
  if (firstfree==highwatermark)
    {
    stopped=1;
    return(-1);
    }
  }
growlist();
list[firstfree++]=message;
return(0);
}
/*****************************************************************************/
/*****************************************************************************/
