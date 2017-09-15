/*
 * proc_conf.cc
 * 
 * created: 19.08.94
 * 12.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <proc_conf.hxx>
#include <clientcomm.h>
#include <ved_errors.hxx>
#include <versions.hxx>

VERSION("Jun 12 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: proc_conf.cc,v 2.8 2004/11/26 14:44:26 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_confirmation::C_confirmation(ems_i32* confir, int size, Request request)
{
//cerr << "C_confirmation::C_confirmation(int* " << confir << "; size=" << size
//    << "; request=" << request << "); this=" << (void*)this << endl;
conf=new C_conf(confir, size, request);
}

/*****************************************************************************/

C_confirmation::C_confirmation(ems_i32* confir, int size, Request request,
void(*deleter)(ems_i32*))
{
//cerr << "C_confirmation::C_confirmation(int* " << confir << "; size=" << size
//    << "; request=" << request << "; deleter=" << (void*)deleter << "); this="
//    << (void*)this << endl;
conf=new C_conf(confir, size, request, deleter);
}

/*****************************************************************************/

C_confirmation::C_confirmation(ems_i32* confir, msgheader header)
{
//cerr << "C_confirmation::C_confirmation(int* " << confir << "; header); this="
//    << (void*)this << endl;
conf=new C_conf(confir, header);
}

/*****************************************************************************/

C_confirmation::C_confirmation(ems_i32* confir, msgheader header,
void(*deleter)(ems_i32*))
{
//cerr << "C_confirmation::C_confirmation(int* " << confir
//    << "; header; deleter=" << (void*)deleter << "); this=" << (void*)this
//    << endl;
conf=new C_conf(confir, header, deleter);
}

/*****************************************************************************/

C_confirmation::C_confirmation(const C_confirmation& confir)
{
//cerr << "C_confirmation::C_confirmation(const C_confirmation& "
//    << (void*)&confir << "); this=" << (void*)this << endl;
conf=confir.conf;
if (conf) (*conf)++;
}

/*****************************************************************************/

C_confirmation& C_confirmation::operator = (const C_confirmation& confir)
{
//cerr << "C_confirmation::operator=(" << (void*)&confir << "); this="
//    << (void*)this << endl;
if (this==&confir) return *this;
conf=confir.conf;
if (conf) (*conf)++;
return *this;
}

/*****************************************************************************/

C_confirmation::C_confirmation(void)
{
//cerr << "C_confirmation::C_confirmation(void); this=" << (void*)this << endl;
conf=0;
}

/*****************************************************************************/

C_confirmation::~C_confirmation()
{
//cerr << "C_confirmation::~C_confirmation; this=" << (void*)this << endl;
if (conf) (*conf)--;
}

/*****************************************************************************/

C_confirmation::C_conf::C_conf(ems_i32* conf, int size, Request request,
        void(*deleter)(ems_i32*))
:count(1), ved_(0), buffer(conf), header(0),
deleter(deleter), size(size), request(request)
{
//cerr << "C_conf::C_conf_a(); this=" << (void*)this << endl;
}

/*****************************************************************************/

C_confirmation::C_conf::C_conf(ems_i32* conf, msgheader head,
        void(*deleter)(ems_i32*))
:count(1), ved_(0), buffer(conf), deleter(deleter), size(head.size),
request(head.type.reqtype)
{
//cerr << "C_conf::C_conf_b(); this=" << (void*)this << endl;
header=new msgheader;
*header=head;
}

/*****************************************************************************/

C_confirmation::C_conf::~C_conf()
{
//cerr << "C_conf::~C_conf(); this=" << (void*)this << endl;
if (deleter)
  (*deleter)(buffer);
else
  delete[] buffer;
delete header;
}

/*****************************************************************************/

void C_confirmation::C_conf::operator --(int)
{
//cerr << "C_conf::--; this=" << (void*)this << "; new count=" << count-1 << endl;
if (--count==0)
  {
  //cerr << "  will delete" << endl;
  delete this;
  }
}

/*****************************************************************************/

void C_confirmation::C_conf::operator ++(int)
{
//cerr << "C_conf::++; this=" << (void*)this << "; new count=" << count+1 << endl;
count++;
}

/*****************************************************************************/

ostream& C_confirmation::print_error(ostream& os) const
{
print_header(os);
if (conf->ved_==0)
  {
  cerr << "WARNING: C_confirmation::print_error: ved_==0\n" << endl;
  }
C_ved_error* e=new C_ved_error(conf->ved_, this);
os << *e << endl;
delete e;
return os;
}

/*****************************************************************************/

ostream& C_confirmation::print_body(ostream& os) const
{
ems_i32 *buffer=conf->buffer;
int size=conf->size;
os << "Body:" << endl << hex << setiosflags(ios::showbase);
for (int i=0; i<size; i++)
  {
  os << (unsigned int)buffer[i];
  if (i+1<size) os << ' ';
  }
os << endl << dec << resetiosflags(ios::showbase);
return os;
}

/*****************************************************************************/

ostream& C_confirmation::print_header(ostream& os) const
{
msgheader* header=conf->header;
os << "Header:" << endl;
os << "  size   : " << header->size << endl;
os << "  client : " << header->client << endl;
os << "  ved    : " << header->ved << endl;
os << "  reqtype: " << header->type.reqtype << endl;
os << "  flags  : " << hex << setiosflags(ios::showbase)
    << (unsigned int)header->flags
    << dec << resetiosflags(ios::showbase) <<  endl;
os << "  xid    : " << header->transid << endl;
return os;
}

/*****************************************************************************/

ostream& C_confirmation::print(ostream& os) const
{
print_header(os);
print_body(os);
return os;
}

/*****************************************************************************/

ostream& operator <<(ostream& os, const C_confirmation& con)
{
return con.print(os);
}

/*****************************************************************************/
/*****************************************************************************/
