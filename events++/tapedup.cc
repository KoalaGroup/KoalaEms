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
#include "compat.h"
#include <versions.hxx>

VERSION("Aug 03 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: tapedup.cc,v 1.6 2005/02/11 15:45:13 wuestner Exp $")
#define XVERSION

C_readargs* args;
int recsize;
int dontask;
STRING infile;
STRING outfile;

int newfile, wrote;
int ip, op;
char* buf;

long bytes_o=0;
long bytes_i=0;
long bytes_o_tape=0;
long bytes_i_tape=0;
long bytes_o_file=0;
long bytes_i_file=0;

long recs_o=0;
long recs_i=0;
long recs_o_tape=0;
long recs_i_tape=0;
long recs_o_file=0;
long recs_i_file=0;

long files_o=0;
long files_i=0;
long files_o_tape=0;
long files_i_tape=0;
long tapes_o=0;
long tapes_i=0;

/*****************************************************************************/

int readargs()
{
args->addoption("recsize", "rs", 65535, "max. record size", "recsize");
args->addoption("dontask", "da", false, "keine dummen fragen", "dontask");
args->addoption("infile", 0, "", "input file", "input");
args->addoption("outfile", 1, "", "output file", "output");

if (args->interpret(1)!=0) return(-1);

infile=args->getstringopt("infile");
outfile=args->getstringopt("outfile");
recsize=args->getintopt("recsize");
dontask=args->getboolopt("dontask");

return(0);
}


/*****************************************************************************/

void add_ro_stat(int b)
{
recs_o++;
recs_o_file++;
recs_o_tape++;
bytes_o+=b;
bytes_o_file+=b;
bytes_o_tape+=b;
if (((recs_o_file%1000)==0) && (bytes_o_file>(32768*recs_o_file)))
    clog << "record " << recs_o_file << endl;
}

void add_ri_stat(int b)
{
recs_i++;
recs_i_file++;
recs_i_tape++;
bytes_i+=b;
bytes_i_file+=b;
bytes_i_tape+=b;
}

void add_fo_stat()
{
clog << "wrote " << recs_o_file << (recs_o_file==1?" record, ":" records, ")
    << bytes_o_file << " bytes to this file" << endl;
recs_o_file=0;
bytes_o_file=0;
files_o++;
files_o_tape++;
}

void add_fi_stat()
{
recs_i_file=0;
bytes_i_file=0;
files_i++;
files_i_tape++;
}

void add_to_stat()
{
clog << "wrote " << files_o_tape << (files_o_tape==1?" file, ":" files, ")
    << recs_o_tape << (recs_o_tape==1?" record, ":" records, ")
    << bytes_o_tape << " bytes to this tape" << endl;
bytes_o_tape=0;
recs_o_tape=0;
files_o_tape=0;
tapes_o++;
}

void add_ti_stat()
{
clog << "read " << files_i_tape << (files_i_tape==1?" file, ":" files, ")
    << recs_i_tape << (recs_i_tape==1?" record, ":" records, ")
    << bytes_i_tape << " bytes from this tape" << endl;
bytes_i_tape=0;
recs_i_tape=0;
files_i_tape=0;
tapes_i++;
}

void print_final_stats()
{
clog << "read " << tapes_i << (tapes_i==1?" tape, ":" tapes, ")
    << files_i << (files_i==1?" file, ":" files, ")
    << recs_i << (recs_i==1?" record, ":" records, ")
    << bytes_i << " bytes" << endl;
clog << "wrote " << tapes_o << (tapes_o==1?" tape, ":" tapes, ")
    << files_o << (files_o==1?" file, ":" files, ")
    << recs_o << (recs_o==1?" record, ":" records, ")
    << bytes_o << " bytes" << endl;
}

/*****************************************************************************/
//
// res== 0: filemark
// res==-1: tape zuende oder richtiger Fehler
// default: gelesene bytes
//

int read_buf()
{
int res;

res=read(ip, buf, recsize);
if (res==0) clog << "filemark" << endl;
if (res==-1) clog << "read: " << strerror(errno) << endl;
return res;
}

/*****************************************************************************/
//
// res== 0: filemark
// res==-1: tape zuende oder richtiger Fehler
// default: gelesene bytes
//

int write_buf(int n)
{
int res;

res=write(op, buf, n);
if (res<1)
  clog << "write: " << strerror(errno) << endl;
else
  wrote=1;
return res;
}

/*****************************************************************************/

int write_mark(int n)
{
wrote=0;
struct mtop mt_com;
mt_com.mt_op=MTWEOF;
mt_com.mt_count=n;
int res=ioctl(op, MTIOCTOP, &mt_com);
if (res<0) clog << "write filemark: " << strerror(errno) << endl;
return res;
}

/*****************************************************************************/

int tape_bsf(int p)
{
struct mtop mt_com;
mt_com.mt_op=MTBSF;
mt_com.mt_count=1;
int res=ioctl(p, MTIOCTOP, &mt_com);
if (res<0) clog << "windfile: " << strerror(errno) << endl;
return res;
}

/*****************************************************************************/

int tape_eject(int p)
{
struct mtop mt_com;
//mt_com.mt_op=MTREW;
mt_com.mt_op=MTOFFL;
mt_com.mt_count=1;
int res=ioctl(p, MTIOCTOP, &mt_com);
if (res<0) clog << "rewind: " << strerror(errno) << endl;
return res;
}

/*****************************************************************************/
#ifdef __osf__
int clear_error(int p)
{
struct mtop mt_com;
mt_com.mt_op=MTCSE;
mt_com.mt_count=0;
int res=ioctl(p, MTIOCTOP, &mt_com);
if (res<0) clog << "clear exception: " << strerror(errno) << endl;
return res;
}
#endif
/*****************************************************************************/

int copy()
{
int need_intape=0;
int need_outtape=0;

buf=new char[recsize];
if (buf==0)
  {
  clog << "malloc buffer: " << strerror(errno) << endl;
  return(-1);
  }
if ((ip=open(infile.c_str(), O_RDONLY, 0))==-1)
  {
  clog << "open " << infile << " for reading: " << strerror(errno) << endl;
  return(-1);
  }
if ((op=open(outfile.c_str(), O_WRONLY, 0))==-1)
  {
  clog << "open " << outfile << " for writing: " << strerror(errno) << endl;
  close(ip); return(-1);
  }
newfile=1; wrote=0;

int weiter=1;
do
  {
  int res=read_buf();
  if (res<1) // fehler oder filemark
    {
    if (wrote)
      {
      if (write_mark(1)==-1) // fehler
        {
        need_outtape=1;
        // auf Tapemark wird verzichtet
        }
      add_fi_stat();
      add_fo_stat();
      }
    if (res==-1) need_intape=1;
    }
  else // gewoehnliche Daten
    {
    add_ri_stat(res);
    if (write_buf(res)==-1)
      {
      clog << "versuche letzten File zu loeschen" << endl;
      if (tape_bsf(op)==0) write_mark(2);
      need_outtape=1;
      clog << "versuche letzten File nochmal zu lesen" << endl;
      tape_bsf(ip);
      }
    else
      add_ro_stat(res);
    }

  if (need_intape)
    {
    need_intape=0;
    add_ti_stat();
    clog << "brauche neues input tape" << endl;
    tape_eject(ip);
    close(ip);
    cout << "continue? [y]/n: ";
    int c;
    if (((c=cin.get())=='n') || (c=='N'))
      {
      weiter=0;
      write_mark(1);
      }
    else
      {
      do
        {
        if ((ip=open(infile.c_str(), O_RDONLY, 0))==-1)
          {
          clog << "open " << infile << " for reading: " << strerror(errno)
            << endl;
          cout << "continue? [y]/n: ";
          if (((c=cin.get())=='n') || (c=='N')) weiter=0;
          }
        }
      while ((ip<0) && weiter);
      }
    }
  if (need_outtape)
    {
    need_outtape=0;
    add_to_stat();
    if (weiter)
      {
      clog << "brauche neues output tape" << endl;
      tape_eject(op);
      close(op);
      cout << "continue? [y]/n: ";
      int c;
      if (((c=cin.get())=='n') || (c=='N'))
        weiter=0;
      else
        {
        do
          {
          if ((op=open(outfile.c_str(), O_WRONLY, 0))==-1)
            {
            clog << "open " << outfile << " for writing: " << strerror(errno)
              << endl;
            cout << "continue? [y]/n: ";
            if (((c=cin.get())=='n') || (c=='N')) weiter=0;
            }
          }
        while ((op<0) && weiter);
        }
      }
    }
  }
while (weiter);

print_final_stats();
delete[] buf;
close(ip);
close(op);
return 0;
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
