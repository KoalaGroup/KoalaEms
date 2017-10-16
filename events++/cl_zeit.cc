/******************************************************************************
*                                                                             *
* clusterscan.cc                                                              *
*                                                                             *
* created: 24.04.97                                                           *
* last changed: 24.04.97                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include "config.h"
#include "cxxcompat.hxx"
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <readargs.hxx>
#include <inbuf.hxx>
#include "gnuthrow.hxx"
#include <errors.hxx>
#include <swap_int.h>
#include "versions.hxx"

VERSION("Jun 05 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: cl_zeit.cc,v 1.5 2005/02/11 15:44:54 wuestner Exp $")
#define XVERSION


typedef enum {clusterty_events=0, clusterty_text,
    clusterty_no_more_data=0x10000000} clustertypes;

C_readargs* args;
STRING infile;
const int maxcluster=1048576/4;
int zeit[10], tmc, tdt, ldt;

/*****************************************************************************/

int readargs()
{
args->addoption("infile", 0, "/dev/nrmt0h", "input file", "input");
args->hints(C_readargs::required, "infile", 0);
if (args->interpret(1)!=0) return(-1);

infile=args->getstringopt("infile");
return(0);
}

/*****************************************************************************/
/*****************************************************************************/

void do_scan()
{
int p, res, wenden, size, cl_idx, last_evid;
int *c, i, n;
for (i=0; i<10; i++) zeit[i]=0;
tmc=tdt=ldt=n=0;

if ((p=open((char*)infile, O_RDONLY, 0))<0)
  {
  cout << "open " << infile << ": " << strerror(errno) << endl;
  return;
  }

if ((c=new int[maxcluster])==0)
  {
  cout << "new int[" << maxcluster << "]: " << strerror(errno) << endl;
  goto fehler;
  }
cl_idx=0;
last_evid=0;
do
  {
  if ((res=read(p, (char*)c, 2*sizeof(int)))!=2*sizeof(int))
    {
    cout << "read new cluster: res="<<res<<": "<<strerror(errno)<<endl;
    goto fehler;
    }
  if (c[1]!=0x12345678)
    {
    if (c[1]!=0x87654321)
      {
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
  if (size>=maxcluster)
    {
    cout << "  too big"<<endl;
    goto fehler;
    }
  if ((res=read(p, (char*)(c+2), (size-1)*sizeof(int)))!=(size-1)*sizeof(int))
    {
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
    switch (type)
      {
      case clusterty_events:
        {
        int flags, ved_id, fragment_id, num_events;
        buf >>flags>> ved_id>> fragment_id>> num_events;
        for (int ev_idx=0; ev_idx<num_events; ev_idx++)
          {
          int ev_size, ev_id, trig_id, sev_num;
          int start_id=buf.index();
          buf >>ev_size>> ev_id>> trig_id>> sev_num;
          int next_idx=start_id+ev_size+1;
          if (ev_id!=last_evid+1)
            {
            cout << "event-id="<<ev_id<<"; expected "<<last_evid+1<<endl;
            }
          last_evid=ev_id;
          for (int sev_idx=0; sev_idx<sev_num; sev_idx++)
            {
            int sev_id, sev_size;
            int start_id=buf.index();
            buf >> sev_id>> sev_size;
            int next_idx=start_id+sev_size+2;
            printf("%8u ", (unsigned int)ev_id);
            for (int data_idx=0; data_idx<sev_size; data_idx++)
              {
              int val=buf.getint();
              switch (data_idx)
                {
                case 0:
                case 1:
                case 7:
                case 8:
                case 9:
                case 14:
                case 28:
                case 29: break;
                case 10:
                case 11:
                case 12:
                case 13: printf("%d ", val); break;
                default: printf("%8u ", (unsigned int)val); break;
                }
//               if (n>0)
//                 {
//                 if (data_idx==3)
//                   tmc+=val;
//                 else if (data_idx==4)
//                   tdt+=val;
//                 else if (data_idx==5)
//                   ldt+=val;
//                 else if (data_idx>9)
//                   zeit[data_idx-9]+=val;
//                 }
              }
//             n++;
//             if ((n==2) || (n==101))
//               {
//               fprintf(stderr, "tmc:%d, tdt=%d, ldt=%d", tmc, tdt, ldt);
//               for (i=0; i<10; i++) fprintf(stderr, " [%d]=%d", i, zeit[i]);
//               fprintf(stderr, "\n");
//               }
            printf("\n");
            if (buf.index()!=next_idx)
              {
              cout << "cluster "<<cl_idx<<"; event "<<ev_idx<<"; sev="<<sev_idx
                  <<": new idx="<<buf.index()<<"; expected idx="<<next_idx<<endl;
              }
            }
          if (buf.index()!=next_idx)
            {
            cout << "cluster "<<cl_idx<<"; event "<<ev_idx
                <<": new idx="<<buf.index()<<"; expected idx="<<next_idx<<endl;
            }
          }
        }
        break;
      case clusterty_text:
        break;
      case clusterty_no_more_data:
        break;
      default:
        break;
      }
    }
  catch(C_error* e)
    {
    cout << *e<<endl;
    delete e;
    }
  }
while (1);

fehler:
cout << cl_idx << " read" << endl;
close(p);
}

/*****************************************************************************/

main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

do_scan();

return(0);
}

/*****************************************************************************/
/*****************************************************************************/
