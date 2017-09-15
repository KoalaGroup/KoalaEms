/*
 * hist2hbook.cc
 * 
 * created: 04.06.1998 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <errno.h>
#include <readargs.hxx>
#include "versions.hxx"

VERSION("Mar 27 2003", __FILE__, __DATE__, __TIME__,
"$ZEL: hist2hbook.cc,v 1.3 2004/11/26 22:20:41 wuestner Exp $")
#define XVERSION

C_readargs* args;
String hfile;
String* ifiles;
int *ids, fmulti;

/*****************************************************************************/
int readargs()
{
args->addoption("hfile", "f", "", "hbook file", "hbook_file");
args->addoption("id", "id", 1, "histogram ids", "ids");
args->hints(C_readargs::required, "hfile", 0);
args->multiplicity("id", 0);
if (args->interpret(1)!=0) return(-1);

hfile=args->getstringopt("hfile");

int imulti;
imulti=args->multiplicity("id");
fmulti=args->numnames();
if (fmulti<imulti) imulti=fmulti;
ids=new int[fmulti];
for (i=0; i<imulti; i++)
  {
  id[i]=args->getintopt("id", i);
  }
for ( ;i<fmulti; i++)
  {
  id[i]=id[i-1]+1;
  }
ifiles=new String[fmulti];
ifiles[i]=args->getnames(i);
return(0);
}
/*****************************************************************************/
void convert(String& fname, int id)
{
C_iopath ipath(fname, C_iopath::iodir_input, 1);
if (ipath.typ()==C_iopath::iotype_none)
  {
  cerr<<"error open "<<fname<<" for reading"<<endl;
  return;
  }
C_inbuf ib;
if (ib.read(ipath)<0)
  {
  cerr<<"error reading "<<fname<<endl;
  return;
  }
ib>>(*hi1a);
}
/*****************************************************************************/
main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

initHbook();

for (int i=0; i<fmulti; i++)
  {
  convert(ifiles[i], ids[i]);
  }
Hrput(0, (char*)hfile, "N");
return 0;
}
/*****************************************************************************/
/*****************************************************************************/
