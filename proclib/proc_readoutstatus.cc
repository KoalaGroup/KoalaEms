/******************************************************************************
*                                                                             *
* proc_dataio.cc                                                              *
*                                                                             *
* created: 10.06.97                                                           *
* last changed: 28.10.97                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include <errno.h>
#include <stdlib.h>
#include <proc_readoutstatus.hxx>
#include <proc_dataio.hxx>
#include <proc_conf.hxx>
#include <errors.hxx>
#include <versions.hxx>

VERSION("Oct 28 1997", __FILE__, __DATE__, __TIME__,
"$ZEL: proc_readoutstatus.cc,v 2.6 2004/11/26 14:44:33 wuestner Exp $")
#define XVERSION

/*****************************************************************************/
C_readoutstatus::C_readoutstatus(C_inbuf& ib, int useaux)
:time_valid_(0), numaux_(0), aux_key_(0), aux_size_(0), aux_(0)
{
ib++; // errorcode
//cerr<<ib<<endl;
status_=(InvocStatus)ib.getint();
ib >> eventcount_;
if (ib)
  {
  int timecode=ib.getint();
  switch (timecode)
    {
    case 1: /* unix-format */
      ib>>timestamp_;
      //ib>>timestamp_.tv_sec>>timestamp_.tv_usec;
      break;
    case 2: /* OS/9-format */
      int t, s, h;
      ib>>t>>s>>h;
      timestamp_.tv_sec=s+86400*t/*-2440587*/;
      timestamp_.tv_usec=h*10000;
      break;
    default:
      {
      OSTRINGSTREAM ss;
      ss << "C_readoutstatus: unknown timecode "<<timecode;
      throw new C_program_error(ss);
      }
    }
  time_valid_=1;
  }
if (useaux)
  {
  // !! throw fuehrt hier zum memoryleak.
  if (ib)
    {
    ib>>numaux_;
    aux_key_=new int[numaux_];
    if (aux_key_==0) throw new C_unix_error(errno, "malloc");
    aux_size_=new int[numaux_];
    if (aux_size_==0) throw new C_unix_error(errno, "malloc");
    aux_=new int*[numaux_];
    if (aux_==0) throw new C_unix_error(errno, "malloc");
    for (int i=0; i<numaux_; i++)
      {
      ib>>aux_key_[i]>>aux_size_[i];
      if (aux_size_[i])
        {
        aux_[i]=new int[aux_size_[i]];
        if (aux_[i]==0) throw new C_unix_error(errno, "malloc");
        for (int j=0; j<aux_size_[i]; j++) aux_[i][j]=ib.getint();
        }
      else
        aux_[i]=0;
      }
    }
  }
}
/*****************************************************************************/
C_readoutstatus::C_readoutstatus(InvocStatus stat, int count,
    struct timeval& zeit)
:status_(stat), eventcount_(count), time_valid_(1), timestamp_(zeit),
numaux_(0), aux_key_(0), aux_size_(0), aux_(0)
{}
/*****************************************************************************/
C_readoutstatus::~C_readoutstatus()
{
    if (aux_) for (int i=0; i<numaux_; i++) delete[] aux_[i];
    delete[] aux_key_;
    delete[] aux_size_;
    delete[] aux_;
}
/*****************************************************************************/
void C_readoutstatus::settime(const struct timeval& tv)
{
    time_valid_=1;
    timestamp_.tv_sec=tv.tv_sec;
    timestamp_.tv_usec=tv.tv_usec;
}
/*****************************************************************************/
double C_readoutstatus::evrate(const C_readoutstatus& s) const
{
    int diff;
    double tdiff;

    if (!time_valid_ || !s.time_valid()) {
        cerr << "C_readoutstatus::evrate: schlechte Zeiten" << endl;
        return(0.0);
    }

    diff=eventcount_-s.eventcount_;
    tdiff=gettime()-s.gettime();
    return (double)diff/tdiff;
}
/*****************************************************************************/
double C_readoutstatus::gettime() const
{
    return (double)timestamp_.tv_sec+(double)timestamp_.tv_usec/1000000.;
}
/*****************************************************************************/
ostream& C_readoutstatus::print(ostream& os) const
{
    os<<"status="<<status_<<"; eventcount="<<eventcount_<<endl;
    if (time_valid_)
        os<<"sec="<<timestamp_.tv_sec<<"; usec="<<timestamp_.tv_usec<<endl;
    else
        os<<"no timestamp"<<endl;
    os<<"numaux="<<numaux_<<endl;
    for (int i=0; i<numaux_; i++) {
        os<<"key="<<aux_key_[i]<<"; size="<<aux_size_[i];
        for (int j=0; j<aux_size_[i]; j++) os<<" "<<aux_[i][j];
        os<<endl;
    }
    return os;
}
/*****************************************************************************/
ostream& operator <<(ostream& os, const C_readoutstatus& b)
{
    return b.print(os);
}
/*****************************************************************************/
/*****************************************************************************/
