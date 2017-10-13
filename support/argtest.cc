/******************************************************************************
*                                                                             *
* argtest.cc                                                                  *
*                                                                             *
* OSF1/ULTRIX                                                                 *
*                                                                             *
* created: 26.10.94                                                           *
* last changed: 29.10.94                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include "config.h"
//#include "cxxcompat.hxx"

#include "readargs.hxx"
#include "versions.hxx"

VERSION("Jun 05 1998", __FILE__, __DATE__, __TIME__)
static char *rcsid="$Id: argtest.cc,v 2.5 2003/09/11 12:05:24 wuestner Exp $";
#define XVERSION

/*****************************************************************************/

main(int argc, char* argv[])
{
C_readargs args(argc, argv);

args.addoption("bool1",   "b1", false, "boolean 1");
args.addoption("bool2",   "b2", true, "boolean 2");
args.addoption("int1",    "i1qwert", 0, "integer 1", "zahl 1");
args.addoption("char1",   "c1", 'a', "zeichen 1", "char1");
args.multiplicity("char1", 3);
args.addoption("char2",   "c2", 'b', "zeichen 2", "char2");
args.multiplicity("char2", 0);
args.addoption("string1", "s1", "string_a", "erster Text");
args.addoption("hilfe",   "h", false, "dieser Hilfetext");
args.addoption("file1",   0, "", "Textfile", "File 1");
args.addoption("ss3",   3, 13, "txt3");
args.addoption("ss1",   1, "", "txt1");
args.addoption("ss2",   2, "", "txt2", "n2");

args.use_env("bool1", "BOOL");
args.use_env("int1", "INT");


if (args.interpret(1)!=0) return(0);

if (args.getboolopt("hilfe")) args.printhelp();

cout << "program: " << args.progname() << endl;
if (!args.isdefault("bool1"))
    cout << "bool1: " << args.getboolopt("bool1") << endl;

if (!args.isdefault("int1"))
    cout << "int1: " << args.getintopt("int1") << endl;

if (!args.isdefault("char1"))
    cout << "char1: " << args.getcharopt("char1") << endl;

if (!args.isdefault("string1"))
    cout << "string1: " << args.getstringopt("string1") << endl;

if (!args.isdefault("file1"))
    cout << "file1: " << args.getstringopt("file1") << endl;

int i, n;

n=args.numnames();
cout << "extrastrings: " << n << endl;
for (i=0; i<n; i++)
  cout << "[" << i << "]: " << args.getnames(i) << endl;
return(0);
}

/*****************************************************************************/
/*****************************************************************************/
