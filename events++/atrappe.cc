/*
 * atrappe.cc
 * 07.Apr.2003 PW: cxxcompat.hxx used
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <versions.hxx>

#include <readargs.hxx>
#include <errors.hxx>

VERSION("May 11 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: atrappe.cc,v 1.3 2005/02/11 15:44:49 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_readargs* args;
STRING infile, outfile;
int ilit, olit;
int debug, verbose, dump, is;
C_iopath reader;
C_iopath writer;
struct sev_header
  {
  int IS_ID;
  int ndata;
  };
struct ev_header
  {
  int size;
  int evno;
  int trigno;
  int nsev;
  };
long chans[32];

/*****************************************************************************/
int readargs()
{
args->addoption("infile", 0, "-", "input file", "input");
args->addoption("outfile", 1, "-", "output file", "output");
args->addoption("ilit", "iliterally", false,
    "use name of input file literally", "input file literally");
args->addoption("olit", "oliterally", false,
    "use name of output file literally", "output file literally");
args->addoption("is", "is", 1, "ID of Instrumentation System", "IS ID");
args->addoption("verbose", "verbose", false, "put out some comments", "verbose");
args->addoption("debug", "debug", false, "debug info", "debug");
args->addoption("dump", "dump", false, "dump data", "dump");
if (args->interpret(1)!=0) return(-1);

infile=args->getstringopt("infile");
outfile=args->getstringopt("outfile");
ilit=args->getboolopt("ilit");
olit=args->getboolopt("olit");
debug=args->getboolopt("debug");
verbose=args->getboolopt("verbose");
dump=args->getboolopt("dump");
is=args->getintopt("is");
return 0;
}
/*****************************************************************************/
int do_data(unsigned char* data, int size)
{
int i, col, row;
int num=*(int*)data;
data+=4;
static int ccount=0, ecount=0;

if (dump)
  {
  cerr<<setw(4)<<num<<' ';
  cerr<<hex<<setfill('0');
  for (i=0; i<num; i++)
    {
    cerr<<hex<<setw(2)<<(int)data[i]<<' ';
    }
  cerr<<dec<<setfill(' ')<<endl;
  }
col=0; row=0;
for (i=0; i<num; i++)
  {
  int d=data[i];
  int code=(d>>4)&0xf;
  int chan=d&0xf;
  switch (code)
    {
    case 0: goto raus;
    case 4: chans[chan+16*col]++; ccount++; break;
    case 5: row=0; col++; break;
    case 6: chans[chan+16*col]++; ccount++; row++; break;
    case 7: row++; break;
    default:
      cerr<<"unknown code "<<code<<endl;
    }
  }
raus:
ecount++;
for (i=0; i<16; i++) cerr<<chans[i]<<' ';
cerr<<"   "<<(float)ccount/(float)ecount<<endl;
return 0;
}
/*****************************************************************************/
int do_read()
{
ev_header evheader;
sev_header sevheader;
int res, sev_idx, found_is;
static unsigned char* data=0;
static int datasize;

// read eventheader
res=reader.read(&evheader, sizeof(ev_header));
if (res!=sizeof(ev_header))
  {
  cerr<<"read ev_header: res="<<res<<" errno="<<strerror(errno)<<endl;
  return -1;
  }
found_is=0;
for (sev_idx=0; sev_idx<evheader.nsev; sev_idx++)
  {
  res=reader.read(&sevheader, sizeof(sev_header));
  if (res!=sizeof(sev_header))
    {
    cerr<<"read sev_header: res="<<res<<" errno="<<strerror(errno)<<endl;
    return -1;
    }
  if (!data || datasize<sevheader.ndata)
    {
    delete[] data;
    datasize=sevheader.ndata;
    data=new unsigned char[datasize];
    if (data==0)
      {
      cerr<<"allocate "<<datasize<<" bytes for subevent: "<<strerror(errno)<<endl;
      return -1;
      }
    }
  res=reader.read(data, sevheader.ndata*4);
  if (res!=sevheader.ndata*4)
    {
    cerr<<"read sev: res="<<res<<" errno="<<strerror(errno)<<endl;
    return -1;
    }
  if (sevheader.IS_ID==is)
    {
    found_is++;
    do_data(data, sevheader.ndata*4);
    }
  }
if (!found_is) cerr<<"event "<<evheader.evno<<": no IS "<<is<<" found"<<endl;
return 1;
}
/*****************************************************************************/
main(int argc, char* argv[])
{
int res;
try
  {
  args=new C_readargs(argc, argv);
  if (readargs()!=0) return(0);
  reader.init(infile, C_iopath::iodir_input, ilit);
  if (reader.typ()==C_iopath::iotype_none)
    {
    return 1;
    }
  writer.init(outfile, C_iopath::iodir_output, olit);
  if (writer.typ()==C_iopath::iotype_none)
    {
    reader.close();
    return 1;
    }
  if (verbose)
    {
    cerr<<reader<<endl;
    cerr<<writer<<endl;
    }
  if (writer.typ()==C_iopath::iotype_none||reader.typ()==C_iopath::iotype_none)
    {
    reader.close();
    writer.close();
    return 1;
    }
  for (int i=0; i<32; i++) chans[i]=0;
  while (do_read()>0);
  }
catch (C_error* e)
  {
  cerr<<(*e)<<endl;
  delete e;
  res=2;
  }
delete args;
return res;
}
/*****************************************************************************/
/*****************************************************************************/
