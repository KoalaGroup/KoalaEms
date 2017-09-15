/******************************************************************************
*                                                                             *
* tapedup.cc                                                                  *
*                                                                             *
* OSF1/ULTRIX                                                                 *
*                                                                             *
* created: 31.01.95                                                           *
* last changed: 01.02.95                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include "config.h"
#include "cxxcompat.hxx"
#include <readargs.hxx>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "compat.h"
#include <versions.hxx>

VERSION("Aug 14 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: tapecmp.cc,v 1.5 2005/02/11 15:45:13 wuestner Exp $")
#define XVERSION

C_readargs* args;

STRING file1;
STRING file2;

/*****************************************************************************/

int readargs()
{
args->addoption("file1", 0, "", "first file", "file1");
args->addoption("file2", 1, "", "second file", "file2");

if (args->interpret(1)!=0) return -1;

file1=args->getstringopt("file1");
file2=args->getstringopt("file2");

return(0);
}

/*****************************************************************************/

int copy()
{
int p1, p2, res1, res2;
const int bigmax=240*1024;
const int smallmax=64*1024;
int max1=bigmax;
int max2=bigmax;
char buf1[bigmax];
char buf2[bigmax];
int recs;

p1=open(file1.c_str(), O_RDONLY, 0);
if (p1==-1)
  {
  clog << "open " << file1 << ": "<< strerror(errno) << endl;
  return(-1);
  }

p2=open(file2.c_str(), O_RDONLY, 0);
if (p2==-1)
  {
  clog << "open " << file2 << ": "<< strerror(errno) << endl;
  close(p1);
  return(-1);
  }

recs=0;

res1=read(p1, buf1, max1);
if ((res1==-1) && (errno==EIO))
  {
  clog << "read " << file1 << strerror(errno) << "; try blocksize 64K" << endl;
  max1=smallmax;
  res1=read(p1, buf1, max1);
  }

res2=read(p2, buf2, max2);
if ((res2==-1) && (errno==EIO))
  {
  clog << "read " << file2 << strerror(errno) << "; try blocksize 64K" << endl;
  max2=smallmax;
  res2=read(p2, buf2, max2);
  }

while ((res1!=-1) && (res2!=-1))
  {
  if (res1!=res2)
    cout << "record " << recs << "; size: " << res1 << " | " << res2 << endl;
  else
    {
    int i;
    for (i=0; (i<res1) && (buf1[i]==buf2[i]); i++);
    if (i<res1)
      cout << "record " << recs << "; size=" << res1 << " differ on position "
          << i << endl;
    }
  recs++;
  if ((recs%1000)==0) cout << recs << ": "<< endl;
  res1=read(p1, buf1, max1);
  res2=read(p2, buf2, max2);
  }
if (res1==-1) clog << "read " << file1 << strerror(errno) << endl;
if (res2==-1) clog << "read " << file2 << strerror(errno) << endl;

cout << recs << " records tested" << endl;

close(p1);
close(p2);

return(0);
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
