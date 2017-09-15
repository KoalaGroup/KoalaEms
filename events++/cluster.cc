/*
 * events++/cluster.cc
 * 
 * created: 12.03.1998 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <time.h>
#include "cluster.hxx"
#include <versions.hxx>

VERSION("Apr 20 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: cluster.cc,v 1.6 2004/11/26 14:40:11 wuestner Exp $")
#define XVERSION

///////////////////////////////////////////////////////////////////////////////
ostream& C_cluster::print(ostream& os)
{
switch (typ())
  {
  case clusterty_events: os<<"events"; break;
  case clusterty_ved_info: os<<"ved_info"; break;
  case clusterty_text: os<<"text"; break;
  case clusterty_no_more_data: os<<"no_more_data"; break;
  default: os<<"unknown type "<<ClTYPE(d); break;
  }
os<<"; size="<<ClLEN(d)<<"; optsize="<<ClOPTSIZE(d);
if (ClOPTSIZE(d))
  {
  int* opts=get_opts();
  opts++; /* int num=(*opts++)-1; not used here */
  int flags=*opts++;
  if (flags&ClOPTFL_TIME)
    {
    char s[1024];
    time_t sec=*opts++;
    opts++; /* skip usec */
    strftime(s, 1024, "%Y-%m-%d %H:%M:%S", localtime(&sec));
    cerr<<" opt time: "<<s;
    }
  if (flags&ClOPTFL_CHECK)
    {
    cerr<<endl<<"do not know anything about optionflag 'check'"<<endl;
    }
  }
switch (typ())
  {
  case clusterty_events:
    os<<" vedid="<<get_ved_id();
    os<<" evnum="<<get_num_events();
    if (get_num_events()) os<<" first event="<<ClEVID1(d);
    break;
  case clusterty_ved_info:
    {
    os<<" vednum="<<get_vednum();
    os<<" isnum="<<get_isnum();
    os<<"(";
    for (int i=0; i<get_isnum(); i++)
      {
      os<<get_is_ids()[i];
      if (i+1<get_isnum()) os<<", ";
      }
    os<<")";
    }
    break;
  case clusterty_text:
    break;
  case clusterty_file:
    break;
  case clusterty_no_more_data:
    break;
  }
return os;
}
///////////////////////////////////////////////////////////////////////////////
ostream& C_cluster::dump(ostream& os, int max) const
{
char ff=os.fill('0');
os<<hex;
for (int i=0; (i<max) && (i<size_);)
  {
  for (int j=0; (j<4) && (i<max) && (i<size_); i++, j++)
    os<<setw(8)<<(unsigned int)d[i]<<" ";
  os<<endl;
  }
os<<dec;
os.fill(ff);
return os;
}
///////////////////////////////////////////////////////////////////////////////
ostream& C_cluster::xdump(ostream& os, int max) const
{
char ff=os.fill('0');
os<<hex;
for (int i=0; (i<max) && (i<size_); i++)
  {
  os<<setw(8)<<(void*)(d+i)<<": "<<d[i]<<endl;
  }
os<<dec;
os.fill(ff);
return os;
}
///////////////////////////////////////////////////////////////////////////////
void C_cluster::initialise()
{
if (initialised) return;
switch (typ())
  {
  case clusterty_ved_info:
    if ((d[4+ClOPTSIZE(d)]&0x80000000)==0)
      {
      cerr<<"Warning: old style of ved-info"<<endl;
      ved_num=d[4+ClOPTSIZE(d)];
      is_num=d[5+ClOPTSIZE(d)];
      ved_ids=0;
      is_ids=new int[is_num];
      for (int i=0; i<is_num; i++) is_ids[i]=d[6+i+ClOPTSIZE(d)];
      }
    else
      {
      int *p, i, j, k;
      p=d+5+ClOPTSIZE(d);
      ved_num=*p++;
      is_num=0;
      ved_ids=new int[ved_num];
      for (i=0; i<ved_num; i++)
        {
        p++;        // ved_id
        int n=*p++; // isnum
        is_num+=n;  // is_ids
        p+=n;
        }
      is_ids=new int[is_num];
      p=d+6+ClOPTSIZE(d);
      for (i=0, k=0; i<ved_num; i++)
        {
        ved_ids[i]=*p++;
        int n=*p++;
        for (j=0; j<n; j++) is_ids[k++]=*p++;
        }
      }
    break;
  case clusterty_events:
  case clusterty_text:
  case clusterty_file:
  case clusterty_no_more_data:
    {} // nothing to do
  }
initialised=1;
}
///////////////////////////////////////////////////////////////////////////////
void C_cluster::destroy()
{
delete[] ved_ids;
delete[] is_ids;
}
///////////////////////////////////////////////////////////////////////////////
int  C_cluster::get_vednum()
{
initialise();
return ved_num;
}
///////////////////////////////////////////////////////////////////////////////
int* C_cluster::get_ved_ids()
{
initialise();
return ved_ids;
}
///////////////////////////////////////////////////////////////////////////////
int  C_cluster::get_isnum()
{
initialise();
return is_num;
}
///////////////////////////////////////////////////////////////////////////////
int* C_cluster::get_is_ids()
{
initialise();
return is_ids;
}
///////////////////////////////////////////////////////////////////////////////
ostream& operator <<(ostream& os, C_cluster& rhs)
{
return rhs.print(os);
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
