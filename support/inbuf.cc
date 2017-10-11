/*
 * support/inbuf.cc
 * 
 * created 04.02.1995 PW
 * 
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "versions.hxx"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <xdrstring.h>
#include "xdrstrdup.hxx"
#include "errors.hxx"
#include "inbuf.hxx"
#include "versions.hxx"

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: inbuf.cc,v 2.28 2014/07/14 15:09:53 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

C_inbuf::C_inbuf(const int* ptr, int size)
:C_buf(size)
{
for (int i=0; i<size; i++) buffer[i]=*ptr++;
dsize=size;
}

/*****************************************************************************/

C_inbuf::C_inbuf(const unsigned int* ptr, int size)
:C_buf(size)
{
for (int i=0; i<size; i++) buffer[i]=*ptr++;
dsize=size;
}

/*****************************************************************************/

C_inbuf::C_inbuf(const int* ptr)
:C_buf(*ptr++)
{
for (int i=0; i<bsize; i++) buffer[i]=*ptr++;
dsize=bsize;
}

/*****************************************************************************/

C_inbuf::C_inbuf(const unsigned int* ptr)
:C_buf(*ptr++)
{
for (int i=0; i<bsize; i++) buffer[i]=*ptr++;
dsize=bsize;
}

/*****************************************************************************/
C_inbuf::C_inbuf(int size)
:C_buf(size)
{
cerr << "Warning: C_inbuf::C_inbuf: dsize will not be set." << endl;
}
/*****************************************************************************/
C_inbuf::C_inbuf()
:C_buf()
{}
/*****************************************************************************/

C_inbuf::C_inbuf(const string& name)
{
int path=open(name.c_str(), O_RDONLY, 0);
if (path==-1)
  {
  ostringstream ss;
  ss << "open file \"" << name << "\"";
  throw new C_unix_error(errno, ss);
  }
struct stat status;
if (fstat(path, &status)<0)
  {
  ostringstream ss;
  ss << "fstat \"" << name << "\"";
  close(path);
  throw new C_unix_error(errno, ss);
  }
dsize=status.st_size;
grow(dsize);
if (::read(path, buffer, dsize)!=dsize)
  {
  ostringstream ss;
  ss << "read \"" << name << "\"";
  close(path);
  throw new C_unix_error(errno, ss);
  }
close(path);
}

/*****************************************************************************/
C_inbuf::C_inbuf(const C_buf& buf)
:C_buf(buf)
{}
/*****************************************************************************/
C_inbuf::~C_inbuf()
{}
/*****************************************************************************/
int C_inbuf::empty(void) const
{
return idx>=dsize;
}
/*****************************************************************************/
// const unsigned int& C_inbuf::operator * () const
// {
//     if (idx>=dsize)
//         throw new C_program_error("C_inbuf::operator*: idx>dsize");
//     return(buffer[idx]);
// /*
// XXX
// cxx: Warning: /usr/users/wuestner/ems/support/inbuf.cc, line 157: returning
//           reference to local temporary
// */
// }
/*****************************************************************************/
int C_inbuf::index(int newidx)
{
    int i=idx;
    if (newidx>dsize) {
        cerr<<"C_inbuf::index: idx="<<idx
            <<" dsize="<<dsize<<" newidx="<<newidx<<endl;
        throw new C_program_error("C_inbuf::index: not enough words");
    }
    idx=newidx;
return i;
}
/*****************************************************************************/
unsigned int C_inbuf::getint(void)
{
if (idx>=dsize)
  throw new C_program_error("C_inbuf::getint: buffer exhausted");
return(buffer[idx++]);
}
/*****************************************************************************/
C_inbuf& C_inbuf::operator>>(int& val)
{
if (idx>=dsize)
  throw new C_program_error("C_inbuf::operator>>(int&): buffer exhausted");
val=buffer[idx++];
return *this;
}

/*****************************************************************************/

C_inbuf& C_inbuf::operator >>(unsigned int& val)
{
if (idx>=dsize)
  throw new C_program_error("C_inbuf::operator>>(u_int&): buffer exhausted");
val=buffer[idx++];
return *this;
}

/*****************************************************************************/
C_inbuf& C_inbuf::operator >>(ems_i64& val)
{
    if (idx+1>=dsize)
        throw new C_program_error("C_inbuf::operator>>(ems_i64&): buffer exhausted");

    val=buffer[idx++];
    val<<=32;
    val|=buffer[idx++];

    return *this;
}
/*****************************************************************************/
C_inbuf& C_inbuf::operator >>(ems_u64& val)
{
    if (idx+1>=dsize)
        throw new C_program_error("C_inbuf::operator>>(ems_u64&): buffer exhausted");

    val=buffer[idx++];
    val<<=32;
    val|=buffer[idx++];

    return *this;
}
/*****************************************************************************/

C_inbuf& C_inbuf::operator >>(short& val)
{
if (idx>=dsize)
  throw new C_program_error("C_inbuf::operator>>(short&): buffer exhausted");
val=buffer[idx++];
return *this;
}

/*****************************************************************************/

C_inbuf& C_inbuf::operator >>(unsigned short& val)
{
if (idx>=dsize)
  throw new C_program_error("C_inbuf::operator>>(u_short&): buffer exhausted");
val=buffer[idx++];
return *this;
}

/*****************************************************************************/
#if 0
C_inbuf& C_inbuf::operator>>(float& v)
{
    int size=sizeof(float)/sizeof(int);
    if (idx+size>dsize)
        throw new C_program_error("C_inbuf::operator>>(float&): buffer exhausted");
    v=*reinterpret_cast<float*>(buffer+idx);
    idx+=size;
    return *this;
}
#endif
/*****************************************************************************/
#if 0
C_inbuf& C_inbuf::operator>>(double& v)
{
    int size=sizeof(double)/sizeof(int);
    if (idx+size>dsize)
        throw new C_program_error("C_inbuf::operator>>(float&): buffer exhausted");
    union {
        ems_u32 i[2];
        double d;
    } u;
    u.i[0]=buffer[idx++];
    u.i[1]=buffer[idx++];
    v=u.d;
    return *this;
}
#endif
/*****************************************************************************/

C_inbuf& C_inbuf::operator >>(struct timeval& val)
{
if (idx+1>=dsize)
  throw new C_program_error("C_inbuf::operator>>(timeval&): buffer exhausted");
val.tv_sec=buffer[idx++];
val.tv_usec=buffer[idx++];
return *this;
}

/*****************************************************************************/

C_inbuf& C_inbuf::operator >>(string& str)
{
ems_u32* ptr=buffer+idx;
char* s;
if (idx>=dsize)
  throw new C_program_error("C_inbuf::operator>>(String&): no count");
idx+=xdrstrlen(ptr);
if (idx>dsize)
  throw new C_program_error("C_inbuf::operator>>(String&): not enough words");
s=xdrstrdup(ptr);
str=s;
delete[] s;
return *this;
}
/*****************************************************************************/
C_inbuf& C_inbuf::operator ++(int)
{
    idx++;
    if (idx>dsize)
        throw new C_program_error("C_inbuf::operator++: buffer exhausted");
    return *this;
}
/*****************************************************************************/
C_inbuf& C_inbuf::operator +=(int i)
{
    idx+=i;
    if (idx>dsize)
        throw new C_program_error("C_inbuf::operator+=: buffer exhausted");
    return *this;
}
/*****************************************************************************/
void C_inbuf::getopaque(void* arr, int maxsize)
{
if (idx>=dsize)
  throw new C_program_error("C_inbuf::getopaque: buffer exhausted");
int size=buffer[idx++];
if (size!=maxsize)
  {
  cerr<<"getopaque: size="<<size<<", maxsize="<<maxsize<<endl;
  }
int newidx=idx+(size+3)/4;
if (newidx>dsize)
  throw new C_program_error("C_inbuf::getopaque: buffer exhausted");

//bcopy((const char*)(buffer+idx), (char*)arr, size);
memcpy(arr, buffer+idx, size);
idx=newidx;
}
/*****************************************************************************/
/*****************************************************************************/
