/*
 * hitest.cc
 * 
 * created 05.06.98 PW
 */

#error HITEST.CC USED

#include "config.h"
#include "cxxcompat.hxx"
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "errors.hxx"
#include "versions.hxx"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "histogram_t.hxx"
#include "iopathes.hxx"

VERSION("Jun 11 1998", __FILE__, __DATE__, __TIME__)
static char* rcsid="$Id: hitest.cc,v 2.4 2003/09/11 12:05:34 wuestner Exp $";
#define XVERSION

/*****************************************************************************/

main()
{
try {
  histogram<float> hi1("qwert", 1, 0. ,10. , 11);
  for (int i=-2; i<39; i++) hi1.fill(i/3., 1.);
  {
  C_outbuf ob;
  ob<<hi1;
  C_iopath opath("hist.buf", C_iopath::iodir_output, 1);
  if (opath.typ()==C_iopath::iotype_none)
    {
    cerr<<"error open opath"<<endl;
    return 1;
    }
  if (ob.write(opath)<0)
    {
    cerr<<"error write"<<endl;
    }
  }

  histogram<double>* hi2;

  {
  C_iopath ipath("hist.buf", C_iopath::iodir_input, 1);
  if (ipath.typ()==C_iopath::iotype_none)
    {
    cerr<<"error open ipath"<<endl;
    return 1;
    }
  C_inbuf ib;
  if (ib.read(ipath)<0)
    {
    cerr<<"error read"<<endl;
    }

  hi2=(histogram<double>*)(C_histogram::restore(ib));
  }
  histogram<float>* hi3=(histogram<float>*)hi2;

  cerr<<"  call hi1.diff(*hi2)"<<endl;
  hi1.diff(*hi2);
  cerr<<"  call hi2->diff(hi1)"<<endl;
  hi2->diff(hi1);

  histogram<char>* hi4=(histogram<char>*)(hi2->clone());
  cerr<<"  call hi2->diff(*hi4)"<<endl;
  hi2->diff(*hi4);
  histogram<char>* hi5=new histogram<char>;
  cerr<<"CALL *hi5=*hi2"<<endl;
  *hi5=*hi2;
  cerr<<"*hi5=*hi2 CALLED"<<endl;
  cerr<<"  call hi2->diff(*hi5)"<<endl;
  hi2->diff(*hi5);
  hi2->dump(cerr);
  hi5->dump(cerr);
  delete hi2;
  }
catch (C_error* e)
  {
  cerr<<"Fehler: "<<(*e)<<endl;
  delete e;
  }
}
/*****************************************************************************/
/*****************************************************************************/
