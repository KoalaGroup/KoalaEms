/*
 * cl2sel.cc
 * 
 * created: 22.07.1997 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <readargs.hxx>
#include <swap_int.h>
#include <math.h>
#include <versions.hxx>

VERSION("Jul 14 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: cl_evselect.cc,v 1.6 2004/11/26 22:41:50 wuestner Exp $")
#define XVERSION

typedef enum {clusterty_events=0, clusterty_text,
    clusterty_no_more_data=0x10000000} clustertypes;

C_readargs* args;
STRING infile, outfile;
int recsize, vednum;
int inf, of, itape;
unsigned int sevid, ev_id;
int num_events=0;
unsigned int maxclsize, maxevsize;
int cl_idx;

// cluster[0]: size (number of following words)
// cluster[1]: endientest =0x12345678
// cluster[2]: cluster type (event=0; text=1; last cluster=0x10000000)
// cluster[3]: flags
// cluster[4]: ved_id
// cluster[5]: fragment id
// cluster[6]: number of events
// event  [0]: len
// event  [1]: evno
// event  [2]: trigno
// event  [3]: numofsubevs
// subev  [0]: isid
// subev  [1]: subevlen

/*****************************************************************************/

void printheader(ostream& os)
{
os<<"Selects one subevent using cluster format."<<endl<<endl;
}

/*****************************************************************************/

int readargs()
{
args->helpinset(3, printheader);
args->addoption("infile", 0, "-", "input file", "input");
args->addoption("outfile", 1, "-", "output file", "output");
args->addoption("recsize", "recsize", 65536, "maximum record size (tape only)",
    "recsize");
args->addoption("maxclsize", "maxclsize", 204800,
    "maximum cluster size (only test)", "maxsize");
args->addoption("maxevsize", "maxevsize", 204800,
    "maximum event size (only test)", "maxevsize");
args->addoption("vednum", "vednum", 1, "number of VEDs", "vednum");
args->addoption("sevid", "sevid", 1, "id of subevent", "sevid");

if (args->interpret(1)!=0) return(-1);

infile=args->getstringopt("infile");
outfile=args->getstringopt("outfile");
recsize=args->getintopt("recsize");
maxclsize=args->getintopt("maxclsize");
maxevsize=args->getintopt("maxevsize");
vednum=args->getintopt("vednum");
sevid=args->getintopt("sevid");
return(0);
}

/*****************************************************************************/

int do_open()
{
struct mtop mt_com;
mt_com.mt_op=MTNOP;
mt_com.mt_count=0;

if (infile=="-")
  inf=0;
else
  {
  inf=open(infile.c_str(), O_RDONLY, 0);
  if (inf<0)
    {
    clog<<"open "<<infile<<" for reading: "<<strerror(errno)<<endl;
    return -1;
    }
  }
itape=ioctl(inf, MTIOCTOP, &mt_com)==0;
clog<<"input file is "<< (itape?"":"not a ")<<"tape"<<endl;
of=open(outfile.c_str(), O_WRONLY|O_CREAT, 0666);
if (of<0)
  {
  clog<<"open "<<outfile<<" for writing: "<<strerror(errno)<<endl;
  return -1;
  }
return 0;
}

/*****************************************************************************/

int do_close()
{
if (inf>0) close(inf);
if (of>0) close(of);
return 0;
}

/*****************************************************************************/
// checks subeventid
// moves a subevent from srcptr to destptr
// increments srcptr and eventually destptr
// returns the amount of moved words (inclusive header)
int writesubevent(unsigned int *& srcptr, unsigned int *& destptr)
{
int len=srcptr[1]+2;
if (srcptr[0]!=sevid)
  {
  srcptr+=len;
  return 0;
  }
for (int i=0; i<len; i++) *destptr++=*srcptr++;
return len;
}

/*****************************************************************************/
// moves a event from srcptr to destptr
// increments srcptr and destptr
// returns the amount of moved words (inclusive header)
int writeevent(unsigned int *& srcptr, unsigned int *& destptr)
{
int i;
int old_size, new_size;
int old_num_sev, new_num_sev;
unsigned int *old_srcptr, *old_destptr;

old_srcptr=srcptr; old_destptr=destptr;
old_size=srcptr[0]; new_size=0;
old_num_sev=srcptr[3]; new_num_sev=0;
//if (hack) old_size-=4;
// copy header; length and number of subevents are invalid
for (i=0; i<4; i++) {*destptr++=*srcptr++;} 
new_size=3;
for (i=0; i<old_num_sev; i++)
  {
  int s=writesubevent(srcptr, destptr);
  if (s>0) {new_num_sev++; new_size+=s;}
  if (srcptr>old_srcptr+old_size+1)
    {
    cerr<<"wrong eventsize"<<endl;
    }
  }
//clog<<"event "<<old_destptr[1]<<" old_size="<<old_size<<"; new_size="<<new_size<<endl;
old_destptr[0]=new_size;
old_destptr[3]=new_num_sev;
num_events++;
return new_size+1;
}

/*****************************************************************************/
// modifies a cluster in buf
// returns an error code
int write_cluster(unsigned int* buf)
{
int i;
int old_size, new_size;
int num_ev;
unsigned int *srcptr, *destptr;

srcptr=buf+7; destptr=buf+7;
num_ev=buf[6];
old_size=buf[0]; new_size=6;

for (i=0; i<num_ev; i++) new_size+=writeevent(srcptr, destptr);
buf[0]=new_size;
//clog<<"cluster "<<cl_idx<<" old_size="<<old_size<<"; new_size="<<new_size<<endl;
if (write(of, buf, (new_size+1)*sizeof(int))!=(ssize_t)((new_size+1)*sizeof(int)))
  {
  clog<<"write: "<<strerror(errno)<<endl;
  return -1;
  }
return 0;
}

/*****************************************************************************/
// copies a cluster in buf unmodified
// returns an error code
int copy_cluster(unsigned int* buf)
{
int size;

size=buf[0]+1;
if (write(of, buf, size*sizeof(int))!=(ssize_t)(size*sizeof(int)))
  {
  clog<<"write: "<<strerror(errno)<<endl;
  return -1;
  }
return 0;
}

/*****************************************************************************/

int write_last_cluster()
{
static int cl[7]={6, 0x12345678, clusterty_no_more_data, 0, 0, 0, 0};

if (write(of, cl, 7*sizeof(int))!=7*sizeof(int))
  {
  clog<<"write: "<<strerror(errno)<<endl;
  return -1;
  }
return 0;
}

/*****************************************************************************/

int do_convert_tape()
{
clog<<infile<<" is tape; not jet supported"<<endl;
return -1;
}

/*****************************************************************************/
int do_convert_file()
{
    unsigned int *buf=0, bsize=0, kopf[2], size, type;
    int res, wenden;
    cl_idx=0;
    do {
        // header lesen
        if ((res=read(inf, (char*)kopf, 2*sizeof(int)))!=2*sizeof(int)) {
            if (res<0) {
                cerr << "read new cluster: res="<<res<<": "<<strerror(errno)
                    <<endl;
                goto fehler;
            }
            if (res==0) {
                write_last_cluster();
                goto raus;
            }
        }
        // endien testen
        switch (kopf[1]) {
        case 0x12345678:
            wenden=0;
            break;
        case 0x87654321:
            wenden=1;
            break;
        default:
            clog<<hex<<"wrong endian "<<kopf[1]
                <<"; must be 0x12345678 or 0x87654321"<<endl<<dec;
            goto fehler;
        }
        size=wenden?swap_int(kopf[0]):kopf[0];
        size++; // include first word
        // buffer allozieren
        if (size>maxclsize)
            cerr<<"size="<<size<<endl;
        if (bsize<size) {
            delete[] buf;
            buf=new unsigned int[size];
            bsize=size;
            if (buf==0) {
                clog<<"allocate buffer for cluster "<<cl_idx<<" ("<<size
                    <<" words): "<<strerror(errno)<<endl;
                goto fehler;
            }
            //clog<<"new buf: "<<size<<" words"<<endl;
        }
        // daten lesen
        int bytesize=(size-2)*sizeof(int);
        if ((res=read(inf, (char*)(buf+2), bytesize))!=bytesize) {
            cerr << "read cluster data: res="<<res<<": "<<strerror(errno)<<endl;
            goto fehler;
        }
        // endien korrigieren
        if (wenden) {
            for (unsigned int i=2; i<size; i++) buf[i]=swap_int(buf[i]);
        }
        buf[0]=size-1; buf[1]=0x12345678;
        switch (type=buf[2]/*cluster type*/) {
        case clusterty_events:
            res=write_cluster(buf);
            if (res<0) goto fehler;
            break;
        case clusterty_text:
        case clusterty_no_more_data:
        copy_cluster(buf);
            break;
        default:
            clog<<"unknown cluster type "<<type<<endl;
            goto fehler;
        }
        cl_idx++;
    } while (type!=clusterty_no_more_data);

raus:
    clog<<cl_idx<<" cluster converted ("<<num_events <<" events)"<<endl;
    return 0;
fehler:
    return -1;
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);
if (do_open()<0) {do_close(); return 1;}
if (itape)
  do_convert_tape();
else
  do_convert_file();
do_close();
return 0;
}

/*****************************************************************************/
/*****************************************************************************/
