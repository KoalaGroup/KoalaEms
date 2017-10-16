/*
 * clusterscan.cc
 * 
 * created: 24.04.97 PW
 */

#include "config.h"
#include <cstring>
#include "cxxcompat.hxx"
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <readargs.hxx>
#include <inbuf.hxx>
#include <errors.hxx>
#include "versions.hxx"

VERSION("Jul 14 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: clusterscan.cc,v 1.6 2014/07/14 16:18:17 wuestner Exp $")
#define XVERSION

typedef enum {clusterty_events=0, clusterty_text,
    clusterty_no_more_data=0x10000000} clustertypes;

C_readargs* args;
// level: 0 nur test
// level: 1 cluster
// level: 2 events
// level: 3 subevents
// level: 4 daten
int level;
int debug;
STRING infile, outfile;
int is_tape, put_out;
int maxrec;
FILE* of=0;
//const int maxcluster=65536/4;
const int maxcluster=1048576/4;

#define swap_int(a) ( ((a) << 24) | \
		      (((a) << 8) & 0x00ff0000) | \
		      (((a) >> 8) & 0x0000ff00) | \
	((unsigned int)(a) >>24) )

/*****************************************************************************/

int readargs()
{
args->addoption("level", "level", 0, "verb. level");
//args->addoption("debug", "debug", false, "debug");
//args->addoption("is_tape", "tape", false, "input from tape");
args->addoption("maxrec", "maxrec", 65536, "maxrec", "max. recordsize");
args->addoption("infile", 0, "/dev/nrmt0h", "input file", "input");
args->addoption("outfile", 1, "", "output file", "output");

if (args->interpret(1)!=0) return(-1);

level=args->getintopt("level");
//debug=args->getboolopt("debug");
//is_tape=args->getboolopt("is_tape");
maxrec=args->getintopt("maxrec");
infile=args->getstringopt("infile");
if (args->isdefault("outfile"))
  {
  outfile="";
  put_out=0;
  }
else
  {
  outfile=args->getstringopt("outfile");
  put_out=1;
  }
return(0);
}

/*****************************************************************************/

void print(OSTRINGSTREAM& os)
{
    os << ends;
    STRING s=os.str();
    cout << s << endl;
}

/*****************************************************************************/

void do_scan()
{
    int p, wenden, size, cl_idx=0, last_evid=0;
    ssize_t res;
    int* c;

    if ((p=open(infile.c_str(), O_RDONLY, 0))<0) {
        cout << "open " << infile << ": " << strerror(errno) << endl;
        return;
    }
    if (put_out) {
        of=fopen(outfile.c_str(), "w");
        if (of==0) {
            cout << "open " << outfile << ": " << strerror(errno) << endl;
            return;
        }
    }
    if ((c=new int[maxcluster])==0) {
        cout << "new int[" << maxcluster << "]: " << strerror(errno) << endl;
        goto fehler;
    }

do
  {
  OSTRINGSTREAM os1;
  os1 << "CLUSTER "<<cl_idx;
  if ((res=read(p, (char*)c, 2*sizeof(int)))!=2*sizeof(int))
    {
    print(os1);
    cout << "read new cluster: res="<<res<<": "<<strerror(errno)<<endl;
    goto fehler;
    }
  if (c[1]!=0x12345678)
    {
    if (c[1]!=0x78563412)
      {
      print(os1);
      cout << "wrong header in cluster "<<cl_idx<<endl<<hex;
      cout << "  [0]: "<<c[0]<<endl;
      cout << "  [1]: "<<c[1]<<endl<<dec;
      goto fehler;
      }
    wenden=1;
    size=swap_int(c[0]);
    }
  else
    {
    wenden=0;
    size=c[0];
    }
  os1 << "; size="<<size;
  if (size>=maxcluster)
    {
    print(os1);
    cout << "  too big"<<endl;
    goto fehler;
    }
  if ((res=read(p, (char*)(c+2), (size-1)*sizeof(int)))!=(ssize_t)((size-1)*sizeof(int)))
    {
    print(os1);
    cout << "read cluster: res="<<res<<": "<<strerror(errno)<<endl;
    goto fehler;
    }
  if (wenden)
    {
    for (int i=0; i<size+1; i++) c[i]=swap_int(c[i]);
    }
  try
    {
    C_inbuf buf(c, size+1);
    buf.index(2);
    int type;
    buf >> type;
    os1 << "; type="<<type;
    switch (type)
      {
      case clusterty_events:
        {
        int flags, ved_id, fragment_id, num_events;
        buf >>flags>> ved_id>> fragment_id>> num_events;
        os1 << endl << "  flags="<<flags<<"; ved_id="<<ved_id
            <<"; fragment="<<fragment_id<<"; num_events="<<num_events;
        if (level>=1) print(os1);
        for (int ev_idx=0; ev_idx<num_events; ev_idx++)
          {
          OSTRINGSTREAM os2;
          int ev_size, ev_id, trig_id, sev_num;
          int start_id=buf.index();
          buf >>ev_size>> ev_id>> trig_id>> sev_num;
          int next_idx=start_id+ev_size+1;
          os2 << "ev "<<ev_idx<<": ev_size="<<ev_size<<"; ev_id="<<ev_id
              <<"; trig_id="<<trig_id <<"; sev_num="<<sev_num;
          if (level>=2) print(os2);
          if (ev_id!=last_evid+1)
            {
            if (level<1) print(os1);
            if (level<2) print(os2);
            cout << "event-id="<<ev_id<<"; expected "<<last_evid+1<<endl;
            }
          last_evid=ev_id;
          for (int sev_idx=0; sev_idx<sev_num; sev_idx++)
            {
            OSTRINGSTREAM os3;
            int sev_id, sev_size;
            int start_id=buf.index();
            buf >> sev_id>> sev_size;
            int next_idx=start_id+sev_size+2;
            os3 <<"sev_id="<<sev_id<<"; sev_size="<<sev_size;
            if (level>=3) print(os3);
            if (put_out || (level>=4))
              {
              if (put_out) fprintf(of, "%8u ", (unsigned int)ev_id);
              for (int data_idx=0; data_idx<sev_size; data_idx++)
                {
                int val=buf.getint();
                if (level>=4) cout << val << " ";
                if (put_out) fprintf(of, "%8u ", (unsigned int)val);
                }
              if (level>=4) cout << endl;
              if (put_out) fprintf(of, "\n");
              }
            else
              {
              buf+=sev_size;
              }
            if (buf.index()!=next_idx)
              {
              cout << "cluster "<<cl_idx<<"; event "<<ev_idx<<"; sev="<<sev_idx
                  <<": new idx="<<buf.index()<<"; expected idx="<<next_idx<<endl;
              }
            }
          if (buf.index()!=next_idx)
            {
            if (level<1) print(os1);
            if (level<2) print(os2);
            cout << "cluster "<<cl_idx<<"; event "<<ev_idx
                <<": new idx="<<buf.index()<<"; expected idx="<<next_idx<<endl;
            }
          }
        }
        break;
      case clusterty_text:
        os1 << "text (not decoded)"<<endl;
        if (level>=1) print(os1);
        break;
      case clusterty_no_more_data:
        os1 << "last cluster"<<endl;
        if (level>=1) print(os1);
        break;
      default:
        os1 << "unknown type "<<type<<endl;
        print(os1);
      }
    }
  catch(C_error* e)
    {
    cout << *e<<endl;
    delete e;
    }
  if (level>0)
    cl_idx++;
  else
    {
    if (cl_idx++%10==9) cout << "cluster " << cl_idx << endl;
    }
  }
while (1);

fehler:
cout << cl_idx << " read" << endl;
close(p);
if (put_out) fclose(of);
}

/*****************************************************************************/
int
main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

do_scan();

return(0);
}

/*****************************************************************************/
/*****************************************************************************/
