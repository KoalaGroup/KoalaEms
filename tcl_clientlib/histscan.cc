/*
 * histscan.cc
 * 
 * created: 11.12.96 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <readargs.hxx>
#include "compat.h"
#include "tcl_cxx.hxx"
#include <versions.hxx>

VERSION("Nov 16 2004", __FILE__, __DATE__, __TIME__,
"$ZEL: histscan.cc,v 1.7 2004/11/26 23:03:51 wuestner Exp $")
#define XVERSION

C_readargs* args;
int verbose;
int decode;
int onlytime;
String infile;
String outfile;
int ifile;
ofstream o;

struct vt_pair
  {
  double time;
  double val;
  };

/*****************************************************************************/

int readargs()
{
args->addoption("verbose", "verbose", false, "verbose");
args->addoption("decode", "decode", false, "decode");
args->addoption("onlytime", "time", false, "print only first and last time");
args->addoption("infile", 0, "", "input file", "input");
args->addoption("outfile", 1, "", "output file", "output");
args->hints(C_readargs::required, "infile", 0);

if (args->interpret(1)!=0) return(-1);

verbose=args->getboolopt("verbose");
decode=args->getboolopt("decode");
onlytime=args->getboolopt("onlytime");
infile=args->getstringopt("infile");
outfile=args->getstringopt("outfile");
if (decode && args->isdefault("outfile"))
  {
  cerr << "decode requires outfile" << endl;
  return -1;
  }
return(0);
}

/*****************************************************************************/

int openinfile()
{
ifile=open((char*)(const char*)infile, O_RDWR|O_CREAT, 0640);
if (ifile<0)
  {
  cerr<<"open "<<infile<<": "<<strerror(errno)<<endl;
  return -1;
  }
else
  return 0;
}

/*****************************************************************************/

int openoutfile()
{
o.open(outfile);
if (!o.good())
  {
  cerr << "can't open " << outfile << " for writing: ???" << endl;
  return -1;
  }
return 0;
}

/*****************************************************************************/

void decode_file()
{
vt_pair pair;
int ok=1;
do
  {
  int res=read(ifile, (char*)&pair, sizeof(vt_pair));
  if (res!=sizeof(vt_pair))
    {
    cerr << "read: "<<strerror(errno)<<endl;
    ok=0;
    }
  else
    {
    o<<setprecision(20)<<pair.time<<' '<<setprecision(20)<<pair.val<<endl;
    }
  }
while (ok);
}

/*****************************************************************************/

void test_file()
{
vt_pair pair1, pair2;
int res;

res=read(ifile, (char*)&pair1, sizeof(vt_pair));
if (res!=sizeof(vt_pair))
  {
  cerr << "read: "<<strerror(errno)<<endl;
  return;
  }
if (lseek(ifile, -sizeof(vt_pair), SEEK_END)<0)
  {
  cerr << "lseek: " << strerror(errno) << endl;
  return;
  }
res=read(ifile, (char*)&pair2, sizeof(vt_pair));
if (res!=sizeof(vt_pair))
  {
  cerr << "read: "<<strerror(errno)<<endl;
  return;
  }
char t[1024];
struct tm *tm;
time_t sec;
if (onlytime)
  {
  sec=(time_t)pair1.time;
  tm=localtime(&sec);
  strftime(t, 1024, "%a %H.%M:%S", tm);
  cout << t << " --> ";
  sec=(time_t)pair2.time;
  tm=localtime(&sec);
  strftime(t, 1024, "%a %H.%M:%S", tm);
  cout << t << endl;
  }
else
  {
  sec=(time_t)pair1.time;
  tm=localtime(&sec);
  strftime(t, 1024, "%a %H.%M:%S", tm);
  cout<<setprecision(20)<<pair1.time<<' '<<setprecision(20)<<pair1.val
      <<" ("<<t<<')'<<endl;
  sec=(time_t)pair2.time;
  tm=localtime(&sec);
  strftime(t, 1024, "%a %H.%M:%S", tm);
  cout<<setprecision(20)<<pair2.time<<' '<<setprecision(20)<<pair2.val
      <<" ("<<t<<')'<<endl;
  }
}

/*****************************************************************************/

main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

if (openinfile()!=0) return 0;

if (decode)
  {
  if (openoutfile()!=0) return 0;
  decode_file();
  }
else
  test_file();
  
close(ifile);
return(0);
}

/*****************************************************************************/
/*****************************************************************************/
