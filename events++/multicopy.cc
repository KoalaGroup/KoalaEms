/*
 * multicopy.cc
 * 
 * created: 01.08.1998 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <versions.hxx>

#include <readargs.hxx>
#include <errors.hxx>

VERSION("Aug 01 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: multicopy.cc,v 1.4 2005/02/11 15:45:09 wuestner Exp $")
#define XVERSION

/*****************************************************************************/
C_readargs* args;
int recsize;
int* ilit, numnames;
///////////////////////////////////////////////////////////////////////////////
class C_statist
  {
  public:
    C_statist();
  protected:
    int after_seconds, after_bytes;
    struct timeval letzt;
    int last_bytes;
  public:
    int bytes;
    void reset();
    void print(int);
    void setup(int, int);
  };

C_statist statist;
///////////////////////////////////////////////////////////////////////////////
C_statist::C_statist()
{
reset();
}
///////////////////////////////////////////////////////////////////////////////
void C_statist::reset()
{
bytes=0;
last_bytes=0;
}
///////////////////////////////////////////////////////////////////////////////
void C_statist::setup(int after_seconds_, int after_bytes_)
{
after_seconds=after_seconds_;
after_bytes=after_bytes_;
}
///////////////////////////////////////////////////////////////////////////////
void C_statist::print(int immer)
{
if (!immer && !after_seconds && !after_bytes) return;
struct timeval jetzt;
gettimeofday(&jetzt, 0);
float zeit=float(jetzt.tv_sec-letzt.tv_sec)+
    (float)(jetzt.tv_usec-letzt.tv_usec)/1000000.;
int ok=immer;
if (!ok && after_seconds) ok=zeit>=after_seconds;
if (!ok && after_bytes) ok=bytes>=last_bytes+after_bytes;
if (!ok) return;
unsigned int max=UINT_MAX;
cerr<<bytes<<" bytes transferred";
if (zeit>0)
  {
  cerr<<" "<<(bytes-last_bytes)/zeit/1024.<<" KBytes/s";
  }
cerr<<endl;
letzt=jetzt;
last_bytes=bytes;
}
/*****************************************************************************/
void printheader(ostream& os)
{
os<<"Copies input stream to zero or more output streams."<<endl<<endl;
}
/*****************************************************************************/
void printstreams(ostream& os)
{
os<<endl;
os<<"input and output may be: "<<endl;
os<<"  - for stdin/stdout ;"<<endl;
os<<"  <filename> for regular files or tape;"<<endl;
os<<"      (if filename contains a colon use -literally <x>"<<endl;
os<<"       x=0: input x=1 ... output)"<<endl;
os<<"  :<port#> or <host>:<port#> for tcp sockets;"<<endl;
os<<"  :<name> for unix sockets (passive);"<<endl;
os<<"  @:<name> for unix sockets (active);"<<endl;
}
/*****************************************************************************/
int readargs()
{
int i;
args->helpinset(3, printheader);
args->helpinset(5, printstreams);
args->addoption("infile", 0, "-", "input file", "input");
args->addoption("lit", "literally", 0,
    "use name of stream literally", "stream literally");
args->addoption("recsize", "recsize", 65536, "maximum record size of input (tape only)",
    "recsize");
args->addoption("verbose", "verbose", false, "put out some comments", "verbose");
args->addoption("debug", "debug", false, "debug info", "debug");
args->multiplicity("lit", 0);
args->hints(C_readargs::required, "infile", 0);

if (args->interpret(1)!=0) return(-1);

recsize=args->getintopt("recsize");
numnames=args->numnames();
ilit=new int[numnames];
int nilit=args->multiplicity("lit");
for (i=0; i<numnames; i++) ilit[i]=0;
for (i=0; i<nilit; i++)
  {
  int n=args->getintopt("lit", i);
  if ((n<0) || (n>=numnames))
    {
    cerr<<"-literally "<<n<<": value out of range."<<endl;
    return -1;
    }
  ilit[n]=1;
  }
debuglevel::debug_level=args->getboolopt("debug");
debuglevel::verbose_level=debuglevel::debug_level||args->getboolopt("verbose");

return(0);
}
/*****************************************************************************/
void copy(C_iopath& reader, C_iopath* writer, int numwriters)
{
char* buf=new char[recsize];
int res;
try
  {
  res=reader.read(buf, recsize);
  while (res>0)
    {
    statist.bytes+=res;
    for (int i=0; i<numwriters; i++)
      {
      if (writer[i].typ()!=C_iopath::iotype_none)
        {
        int wres=writer[i].write(buf, res, 0);
        if (wres<0)
          {
          cerr<<"write to "<<writer[i].pathname()<<": "<<strerror(errno)<<endl;
          writer[i].close();
          }
        }
      }
    statist.print(0);
    res=reader.read(buf, recsize);
    }
  }
catch (C_error* e)
  {
  cerr<<(*e)<<endl;
  delete e;
  }
statist.print(1);
}
/*****************************************************************************/
main(int argc, char* argv[])
{
int res=0, i;
statist.setup(60, 1024*1024*10);
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

C_iopath reader(args->getstringopt("infile"), C_iopath::iodir_input, ilit[0]);
if (reader.typ()==C_iopath::iotype_none) return 1;
C_iopath* writer=new C_iopath[numnames-1];
for (i=1; i<numnames; i++)
  {
  writer[i-1].init(args->getnames(i), C_iopath::iodir_output, ilit[i]);
  if (writer[i-1].typ()==C_iopath::iotype_none) return 1;
  }
copy(reader, writer, numnames-1);
delete[] writer;
delete args;
cerr<<"multicopy fertig"<<endl;
}
/*****************************************************************************/
/*****************************************************************************/
