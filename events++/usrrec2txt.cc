/******************************************************************************
*                                                                             *
* usrrec2txt.cc                                                               *
*                                                                             *
* OSF1/ULTRIX                                                                 *
*                                                                             *
* created: 30.01.95                                                           *
* last changed: 10.04.95                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include "config.h"
#include "cxxcompat.hxx"
#include <stdio.h>
#include <readargs.hxx>
#include <errno.h>
#include <sys/ioctl.h>
#include <inbuf.hxx>
#include "compat.h"
#include <versions.hxx>

VERSION("Aug 14 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: usrrec2txt.cc,v 1.6 2014/07/14 16:18:17 wuestner Exp $")
#define XVERSION

C_readargs* args;
STRING infile;
STRING outfile;

/*****************************************************************************/

int readargs()
{
args->addoption("infile", 0, "-", "input file (- - for stdin)", "input");
args->addoption("outfile", 1, "-", "output file", "output");

if (args->interpret(1)!=0) return -1;

infile=args->getstringopt("infile");
outfile=args->getstringopt("outfile");

if ((infile!="-") && (infile==outfile))
  {
  cerr << "input file and output file must be different" << endl;
  return(-1);
  }

return(0);
}

/*****************************************************************************/

int write_main(C_inbuf& in, ofstream& fo)
{
#if 0
int val, val1, val2;
char s[81];
int i;
#endif
/*
fi.read((char*)&val, 4);   // rec_subtype
val1=val&0xffff;           // record number
val2=(val>>16)&0xffff;     // number of records
clog << "record " << val1 << " of " << val2 << endl;
fi.read((char*)&val, 4);   // is_subtype == 0
fi.read((char*)&val, 4);   // run number
clog << "run    " << val << endl;

fi.read(s, 28);
s[28]=0;
clog << "datum: " << s << endl;

fi.read(s, 80);
s[80]=0;
clog << "label: " << s << endl;

fi.read(s, 80);
s[80]=0;
clog << "user label: " << s << endl;
*/
return(0);
}

/*****************************************************************************/

int write_watz_xdr(C_inbuf& in, ofstream& fo)
{
return(0);
}

/*****************************************************************************/

int convert()
{
#if 0
struct
  {
  int size;
  int ev_nr;
  int trig_nr;
  int rec_size;
  int rec_type;
  } header;

int res;
#endif

/*
ifstream fi(infile);
if (!fi.good())
  {
  clog << "open file " << infile << "for reading: ???" << endl;
  return(-1);
  }
*/

C_inbuf inbuf(infile);

ofstream fo(outfile.c_str());
if (!fo.good())
  {
  clog << "open file " << outfile << "for writing: ???" << endl;
  return(-1);
  }

//fi.read((char*)&header, 5*4);
/*
inbuf >> header.size >> header.ev_nr >> header.trig_nr >> header.rec_size
    >>  header.rec_type;

fo << header.rec_type << endl;

switch (header.rec_type)
  {
  case 1:
    clog << "main header record" << endl;
    write_main(inbuf, fo);
    break;
  case 2:
    clog << "hardware configuration file" << endl;
    write_watz_xdr(fi, fo);
    break;
  case 3:
    clog << "module type file" << endl;
    write_watz_xdr(fi, fo);
    break;
  case 4:
    clog << "synchronisation system save file" << endl;
    write_watz_xdr(fi, fo);
    break;
  case 5:
    clog << "instrumentation system save file" << endl;
    write_watz_xdr(fi, fo);
    break;
  case 6:
    clog << "user file" << endl;
    write_watz_xdr(fi, fo);
    break;
  default:
    clog << "no known type" << endl;
    break;
  }
*/
return(0);
}

/*****************************************************************************/
int
main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

convert();

return(0);
}

/*****************************************************************************/
/*****************************************************************************/
