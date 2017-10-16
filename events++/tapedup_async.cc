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
#include <compat.h>
#include <iopathes.hxx>
#include <versions.hxx>

VERSION("Aug 17 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: tapedup_async.cc,v 1.4 2005/02/11 15:45:14 wuestner Exp $")
#define XVERSION

C_readargs* args;
int recsize;
STRING infile;
STRING outfile;

long wrote_bytes;
long wrote_records;
long wrote_marks;

/*****************************************************************************/
int readargs()
{
args->addoption("recsize", "rs", 65535, "max. record size", "recsize");
args->addoption("infile", 0, "", "input file", "input");
args->addoption("outfile", 1, "", "output file", "output");

if (args->interpret(1)!=0) return(-1);

infile=args->getstringopt("infile");
outfile=args->getstringopt("outfile");
recsize=args->getintopt("recsize");
return(0);
}
/*****************************************************************************/
int write_mark(C_iopath* op, int n)
{
struct mtop mt_com;
mt_com.mt_op=MTWEOF;
mt_com.mt_count=n;
int res=ioctl(op->path(), MTIOCTOP, &mt_com);
if (res<0) cerr << "write filemark: " << strerror(errno) << endl;
return res;
}
/*****************************************************************************/
#ifdef USE_AIO
int copy()
{
void* buf;
int res;

C_iopath* ip=new C_iopath(infile, C_iopath::iodir_input, 1);
if (ip->typ()==C_iopath::iotype_none)
  {
  cerr<<"cannot open inpath"<<endl;
  return(-1);
  }
C_iopath* op=new C_iopath(outfile, C_iopath::iodir_output, 1);
if (op->typ()==C_iopath::iotype_none)
  {
  cerr<<"cannot open outpath"<<endl;
  delete ip; return(-1);
  }

ip->aio_in(10);
ip->start_aio_read(recsize);

op->aio_out(10);

wrote_bytes=0;
wrote_records=0;
wrote_marks=0;
int print_diff=10485760;
int next_print=print_diff;

int weiter=1;
do
  {
  int res=ip->aio_read(buf);
  if (res>0)
    {
    op->write(buf, res, 1);
    wrote_bytes+=res;
    wrote_records++;
    if (wrote_bytes>=next_print)
      {
      cerr<<wrote_bytes<<" bytes written"<<endl;
      next_print+=print_diff;
      }
    }
  else if (res==0)
    {
    cerr<<"vor flush"<<endl;
    op->flush_buffer();
    write_mark(op, 1);
    wrote_marks++;
    cerr<<"nach flush"<<endl;
    }
  else /* res<0 */
    {
    cerr<<"readerror"<<endl;
    weiter=0;
    }
  }
while (weiter);

cerr<<wrote_bytes<<" bytes; "<<wrote_records<<" records; "
    <<wrote_marks<<" files"<<endl;
delete ip;
delete op;
return 0;
}
#else
int copy() {return -1;}
#endif
/*****************************************************************************/
int
main(int argc, char* argv[])
{
#ifdef USE_AIO
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);
copy();
return(0);
#else
cerr<<argv[0]<<" was compiled with asynchronous IO disabled. It can not work."
    <<endl;
return 1;
#endif
}

/*****************************************************************************/
/*****************************************************************************/
