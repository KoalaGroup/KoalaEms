/******************************************************************************
*                                                                             *
* tapedup.cc                                                                  *
*                                                                             *
* OSF1/ULTRIX                                                                 *
*                                                                             *
* created: 31.01.95                                                           *
* last changed: 15.09.95                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include "config.h"
#include "cxxcompat.hxx"
#include <readargs.hxx>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "versions.hxx"

VERSION("Aug 17 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: tdd.cc,v 1.5 2005/02/11 15:45:14 wuestner Exp $")
#define XVERSION

C_readargs* args;
STRING infile;
STRING outfile;
int recmax;

/*****************************************************************************/

int readargs()
{
args->addoption("recsize", "rs", 65536, "max. record size", "recsize");
args->addoption("infile", 0, "", "input file", "input");
args->addoption("outfile", 1, "", "output file", "output");

if (args->interpret(1)!=0) return(-1);

infile=args->getstringopt("infile");
outfile=args->getstringopt("outfile");
recmax=args->getintopt("recsize");

return(0);
}

/*****************************************************************************/

static const char* ss(char* s)
{
static char sss[9];
int i;
for (i=0; i<8; i++) sss[i]=s[i];
sss[8]=0;
return sss;
}

/*****************************************************************************/

int status(int p, const char* headline)
{
struct devget dev_get;

clog<<headline<<endl;
int res=ioctl(p, DEVIOCGET, &dev_get);
if (res<0)
  {
  clog<<"DEVIOCGET: "<<strerror(errno)<<endl;
  }
else
  {
  clog<<"  category     ="<<dev_get.category<<endl;
  clog<<"  bus          ="<<dev_get.bus<<endl;
  clog<<"  interface    ="<<ss(dev_get.interface)<<endl;
  clog<<"  device       ="<<ss(dev_get.device)<<endl;
  clog<<"  adpt_num     ="<<dev_get.adpt_num<<endl;
  clog<<"  nexus_num    ="<<dev_get.nexus_num<<endl;
  clog<<"  bus_num      ="<<dev_get.bus_num<<endl;
  clog<<"  ctlr_num     ="<<dev_get.ctlr_num<<endl;
  clog<<"  rctlr_num    ="<<dev_get.rctlr_num<<endl;
  clog<<"  slave_num    ="<<dev_get.slave_num<<endl;
  clog<<"  dev_name     ="<<ss(dev_get.dev_name)<<endl;
  clog<<"  unit_num     ="<<dev_get.unit_num<<endl;
  clog<<"  soft_count   ="<<dev_get.soft_count<<endl;
  clog<<"  hard_count   ="<<dev_get.hard_count<<endl;
  clog<<"  stat         ="<<dev_get.stat<<endl;
  clog<<"  category_stat="<<dev_get.category_stat<<endl;
  }
return 0;
}

/*****************************************************************************/

int write_mark(int p, int n)
{
struct mtop mt_com;
mt_com.mt_op=MTWEOF;
mt_com.mt_count=n;
int res=ioctl(p, MTIOCTOP, &mt_com);
if (res<0)
  return errno;
else
  return 0;
}

/*****************************************************************************/

int clear_error(int p)
{
struct mtop mt_com;
mt_com.mt_op=MTCSE;
mt_com.mt_count=0;
int res=ioctl(p, MTIOCTOP, &mt_com);
if (res<0)
  {
  clog << "clear exception: " << strerror(errno) << endl;
  return errno;
  }
else
  return 0;
}

/*****************************************************************************/

int copy()
{
int ip, op, res, ok, recidx, byteidx;
char* recbuf;
ip=-1; op=-1; res=0; recbuf=0;

recbuf=new char[recmax];
if (recbuf==0)
  {
  clog << "malloc buffer: " << strerror(errno) << endl;
  res=-1; goto raus;
  }
if ((ip=open((char*)infile.c_str(), O_RDONLY, 0))==-1)
  {
  clog << "open " << infile << " for reading: " << strerror(errno) << endl;
  res=-1; goto raus;
  }
if ((op=open((char*)outfile.c_str(), O_WRONLY, 0))==-1)
  {
  clog << "open " << outfile << " for writing: " << strerror(errno) << endl;
  res=-1; goto raus;
  }
status(ip, "input  tape:");
status(op, "output tape:");
recidx=0; byteidx=0;
do
  {
  recidx++;
  int rnum=read(ip, recbuf, recmax);
  if (rnum>0)
    {
    int wnum=write(op, recbuf, rnum);
    if (wnum==rnum)
      {
      byteidx+=wnum;
      ok=1;
      }
    else if (wnum<0)
      {
      clog<<"record "<<recidx<<" ("<<rnum<<" bytes): "<<strerror(errno)<<endl;
      ok=0; res=-1;
      }
    else
      {
      clog<<"record "<<recidx<<": wrote "<<wnum<<" of "<<rnum<<" bytes: "
          <<strerror(errno)<<endl;
      ok=0; res=-1;
      }
    }
  else if (rnum==0) // filemark
    {
    int r;
    if ((r=write_mark(op, 1))!=0)
      {
      clog<<"write filemark (record "<<recidx<<"): "<<strerror(r)<<endl;
      ok=0; res=-1;
      }
    else
      ok=0;
    }
  else // error
    {
    clog<<"read record "<<recidx<<": "<<strerror(errno)<<endl;
    status(ip, "before MTCSE:");
    clear_error(ip);
    status(ip, "after  MTCSE:");
    ok=1;
    }
  }
while(ok);

clog<<"wrote "<<recidx<<" records containing "<<byteidx<<" bytes"<<endl;
raus:
if (ip>=0) close(ip);
if (op>=0) close(op);
if (recbuf!=0) delete recbuf;
return res;
}

/*****************************************************************************/

main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

copy();

return(0);
}

/*****************************************************************************/
/*****************************************************************************/
