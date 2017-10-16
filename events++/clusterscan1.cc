/*
 * clusterscan1.cc
 * 
 * created: 2003-May-05 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <readargs.hxx>
#include <clusterformat.h>
#include <events.hxx>
#include "compat.h"
#include "swap_int.h"
#include "versions.hxx"

VERSION("2003-May-05", __FILE__, __DATE__, __TIME__,
"$ZEL: clusterscan1.cc,v 1.2 2005/02/11 15:44:55 wuestner Exp $")
#define XVERSION

C_readargs* args;
STRING infile;
int ilit;
C_iopath* inpath;
int maxrec, verbose, debug;

/*****************************************************************************/
static int readargs()
{
    args->addoption("verbose", "verbose", 0, "verbose");
    args->addoption("debug", "debug", false, "debug");
    args->addoption("maxrec", "maxrec", 65536, "maxrec", "max. recordsize");
    args->addoption("ilit", "iliterally", false,
        "use name of input file literally", "input file literally");
    args->addoption("infile", 0, "/dev/nrmt0h", "input file", "input");

    if (args->interpret(1)!=0) return -1;

    verbose=args->getintopt("verbose");
    debug=args->getboolopt("debug");
    infile=args->getstringopt("infile");
    maxrec=args->getintopt("maxrec");
    ilit=args->getboolopt("ilit");

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
void scan_file()
{
int head[3];
int res, complete;
int *c=0, c_size=0;

do { // foreach cluster
  res=inpath->read(head, 3*4);
  if (res!=3*4) {
    if (res==0)
      cout<<"no more data"<<endl;
    else if (res<0)
      cerr<<"read file: "<<strerror(errno)<<endl;
    else
      cerr<<"read file: short read("<<res<<" of "<<3*4<<" bytes)"<<endl;
    goto fehler;
  }
  int endien=head[1];
  switch (endien) { // endientest
    case 0x12345678: // host byteorder
      //if (debug) cout<<"host byteorder"<<endl;
      break;
    case 0x78563412: { // verkehrt
      //if (debug) cout<<"not host byteorder"<<endl;
      for (int i=0; i<3; i++) head[i]=swap_int(head[i]);
      }
      break;
    default:
      cerr<<"unknown endien: "<<hex<<c[1]<<dec<<endl;
      delete[] c;
      goto fehler;
  }
  int size=head[0]+1;
  if (c_size<size) {
    delete[] c;
    if ((c=new int[size])==0) {
      cerr<<"new int["<<size<<"]: "<<strerror(errno)<<endl;
      goto fehler;
    }
    c_size=size;
  }
  res=inpath->read(c, (size-3)*4);
  if (res!=(size-3)*4) {
    if (res<0)
      cerr<<"read file: "<<strerror(errno)<<endl;
    else
      cerr<<"read file: short read("<<res<<" of "<<(size-3)*4<<" bytes)"<<endl;
    goto fehler;
  }
  if (endien==0x78563412) {
    for (int i=0; i<size-3; i++) c[i]=swap_int(c[i]);
  }
  res=check_cluster(c, size-3, (clustertypes)head[2]);
  if (res!=0) goto fehler;
}
while (1);
fehler:
delete[] c;
counter.dump_all();
}
/*****************************************************************************/
void scan_tape()
{
    stringstack callers("scan_tape");
    int* c=new int[maxirec];
    int weiter, res;
    clustertypes typ;

    do {
    	cout<<"---- File "<<counter.file<<" ------------------------------"
            "------------------------------------"<<endl;

        do {
            res=check_record(c, typ);
        } while (res>0);

    	weiter=(res==0);
        counter.dump_all();
        counter.new_file(callers);
    } while (weiter);

    delete c;
}
/*****************************************************************************/
void scan_it()
{
    try {
        inpath=new C_iopath(infile, C_iopath::iodir_input, ilit);
        if (inpath->typ()==C_iopath::iotype_none) {
            cout<<"cannot open infile \""<<infile<<"\""<<endl;
            return;
        }
        if (verbose>0) cout<<"infile: "<<(*inpath)<<endl;

        switch (inpath->typ()) {
        case C_iopath::iotype_file:
            scan_file();
            break;
        case C_iopath::iotype_tape:
            scan_tape();
            break;
        case C_iopath::iotype_socket: // nobreak
        case C_iopath::iotype_fifo: // nobreak
        default: scan_file(); break;
        }
        delete inpath;
    }
    catch (C_error* e) {
        cout<<(*e)<<endl;
        delete e;
    }
}
/*****************************************************************************/
main(int argc, char* argv[])
{
    args=new C_readargs(argc, argv);
    if (readargs()!=0) return(0);

    scan_it();

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
