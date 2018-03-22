/*
 * support/outbuf.cc
 * 
 * created 09.11.94 PW
 * 
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <errno.h>
#include <cstring>
#include "errors.hxx"
#include "outbuf.hxx"
#include <xdrstring.h>
#include "versions.hxx"

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: outbuf.cc,v 2.27 2016/05/02 15:22:22 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

C_outbuf::C_outbuf()
:C_buf(0, 100)
{}

/*****************************************************************************/

C_outbuf::C_outbuf(int size)
:C_buf(size, 100)
{}

/*****************************************************************************/

C_outbuf::C_outbuf(int size, int incr)
:C_buf(size, incr)
{}

/*****************************************************************************/

C_outbuf::~C_outbuf()
{}

/*****************************************************************************/

int C_outbuf::empty() const
{
return dsize==0;
}

/*****************************************************************************/
/*
idx>=(?)dsize
grow?
if (idx>dsize) dsize=idx;???
int& C_outbuf::operator * () const
{
if (idx>dsize)
  throw new C_program_error("C_outbuf::operator*: idx>dsize");
return(buffer[idx]);
}
*/
/*****************************************************************************/
C_outbuf& C_outbuf::operator<<(int v)
{
if (idx>=bsize) grow(idx);
buffer[idx++]=v;
if (idx>dsize) dsize=idx;
return(*this);
}
/*****************************************************************************/
C_outbuf& C_outbuf::operator<<(unsigned int v)
{
if (idx>=bsize) grow(idx);
buffer[idx++]=v;
if (idx>dsize) dsize=idx;
return(*this);
}
/*****************************************************************************/
C_outbuf& C_outbuf::operator<<(short int v)
{
    if (idx>=bsize)
        grow(idx);
    buffer[idx++]=v;
    if (idx>dsize)
        dsize=idx;
    return *this;
}
/*****************************************************************************/
C_outbuf& C_outbuf::operator<<(unsigned short int v)
{
    if (idx>=bsize)
        grow(idx);
    buffer[idx++]=v;
    if (idx>dsize)
        dsize=idx;
    return *this;
}
/*****************************************************************************/
/*****************************************************************************/
#if 0
C_outbuf& C_outbuf::operator<<(float v)
{
    if (idx+1>bsize)
        grow(idx+1-1);
    union {
        ems_u32 i;
        float f;
    } u;
    u.f=v;
    buffer[idx++]=u.i;
    if (idx>dsize)
        dsize=idx;
    return *this;
}
#endif
/*****************************************************************************/
#if 0
C_outbuf& C_outbuf::operator<<(double v)
{
    union {
        double d;
        int i[2];
    } u={v};

    if (idx+2>bsize)
        grow(idx+2-1);
    for (int i=0; i<2; i++)
        buffer[idx++]=u.i[i];
    if (idx>dsize)
        dsize=idx;
    return(*this);
}
#endif
/*****************************************************************************/

C_outbuf& C_outbuf::operator<<(struct timeval v)
{
if (idx+1>=bsize) grow(idx+1);
buffer[idx++]=v.tv_sec;
buffer[idx++]=v.tv_usec;
if (idx>dsize) dsize=idx;
return(*this);
}

/*****************************************************************************/

C_outbuf& C_outbuf::operator<<(char v)
{
if (idx>=bsize) grow(idx);
buffer[idx++]=v;
if (idx>dsize) dsize=idx;
return(*this);
}

/*****************************************************************************/
C_outbuf& C_outbuf::operator<<(const char *v)
{
    int num;
    num=v?strxdrlen(v):1;
    if (idx+num>bsize)
        grow(idx+num-1);
    if (v) {
        outstring(buffer+idx, v);
        idx+=num;
    } else {
        buffer[idx++]=0; // empty string
    }
    if (idx>dsize)
        dsize=idx;

    return *this;
}
/*****************************************************************************/

C_outbuf& C_outbuf::operator<<(const string& v)
{
int num;

num=strxdrlen(v.c_str());
if (idx+num>bsize) grow(idx+num-1);
outstring(buffer+idx, v.c_str());
idx+=num;
if (idx>dsize) dsize=idx;
return(*this);
}

/*****************************************************************************/

C_outbuf& C_outbuf::operator<<(ostringstream& v)
{
string st=v.str();
const char* s=st.c_str();
int num=strxdrlen(s);
if (idx+num>bsize) grow(idx+num-1);
outstring(buffer+idx, s);
idx+=num;
if (idx>dsize) dsize=idx;
return(*this);
}

/*****************************************************************************/

C_outbuf& C_outbuf::operator<<(outbuf_manip f)
{
return f(*this);
}

/*****************************************************************************/

// C_outbuf& mark(C_outbuf& buf)
// {
// buf.mark_it();
// return buf;
// }

/*****************************************************************************/

void C_outbuf::do_space(int& x)
{
x=index();
(*this)++;
}

C_outbuf_1<int&> space_(int& x)
{
return C_outbuf_1<int&>(&C_outbuf::do_space, x);
}

/*****************************************************************************/
void C_outbuf::do_space_for_counter(int x)
{
if ((x<0)||(x>=10))
    throw new C_program_error("C_outbuf::do_space_for_counter: wrong index");
marks[x]=index();
(*this)++;
}

C_outbuf_1<int> space_for_counter(int x)
{
return C_outbuf_1<int>(&C_outbuf::do_space_for_counter, x);
}
/*****************************************************************************/
void C_outbuf::do_set_counter(int x)
{
if ((x<0)||(x>=10))
    throw new C_program_error("C_outbuf::do_set_counter: wrong index");
if ((marks[x]<0) || (marks[x]>=size()))
    throw new C_program_error("C_outbuf::do_set_counter: index invalid");
buffer[marks[x]]=index()-marks[x]-1;
}

C_outbuf_1<int> set_counter(int x)
{
return C_outbuf_1<int>(&C_outbuf::do_set_counter, x);
}
/*****************************************************************************/

void C_outbuf::putval(int idx, int val)
{
(*this)[idx]=val;
}

C_outbuf_2<int, int> put(int idx, int val)
{
return C_outbuf_2<int, int>(&C_outbuf::putval, idx, val);
}

/*****************************************************************************/

void C_outbuf::putchararr(const char* s, int size)
{
int newidx=idx+(size+3)/4+1;
grow(newidx);
outnstring(buffer+idx, s, size);
idx=newidx;
if (idx>dsize) dsize=idx;
}

C_outbuf_2<const char*, int> put(const char* s, int size)
{
return C_outbuf_2<const char*, int>(&C_outbuf::putchararr, s, size);
}

/*****************************************************************************/

void C_outbuf::putintarr(const int* arr, int size)
{
int newidx=idx+size;
grow(newidx);
for (int i=0; i<size; i++) buffer[idx+i]=arr[i];
idx=newidx;
if (idx>dsize) dsize=idx;
}

C_outbuf_2<const int*, int> put(const int* arr, int size)
{
return C_outbuf_2<const int*, int>(&C_outbuf::putintarr, arr, size);
}

/*****************************************************************************/

void C_outbuf::putintarr(const unsigned int* arr, int size)
{
int newidx=idx+size;
grow(newidx);
for (int i=0; i<size; i++) buffer[idx+i]=arr[i];
idx=newidx;
if (idx>dsize) dsize=idx;
}

C_outbuf_2<const unsigned int*, int> put(const unsigned int* arr, int size)
{
return C_outbuf_2<const unsigned int*, int>(&C_outbuf::putintarr, arr, size);
}

/*****************************************************************************/
// void C_outbuf::putopaque(const void* arr, int size)
// {
// int newidx=idx+(size+3)/4+1;
// grow(newidx);
// buffer[idx++]=size;
// //bcopy((const char*)arr, (char*)(buffer+idx), size);
// memcpy((char*)(buffer+idx), (const char*)arr, size);
// idx=newidx;
// if (idx>dsize) dsize=idx;
// }
/*****************************************************************************/
/*****************************************************************************/
