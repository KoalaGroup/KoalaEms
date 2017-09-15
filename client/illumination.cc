/*
 * illumination.cc
 * created 02.Jun.1998 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <readargs.hxx>
#include <ved_errors.hxx>
#include <errors.hxx>
#include <proc_communicator.hxx>
#include <proc_is.hxx>
#include <errorcodes.h>
#include <stdio.h>
#include <errno.h>
#include "client_defaults.hxx"
#include "bong.hxx"

#include <versions.hxx>

VERSION("Mar 27 2003", __FILE__, __DATE__, __TIME__,
"$ZEL: illumination.cc,v 1.4 2007/04/18 19:57:52 wuestner Exp $")

C_VED* ved;
C_instr_system* is;
C_proclist* proclist;
C_readargs* args;
int verbose, debug, slot, code;
const char* vedname;
//extern "C" void usleep(unsigned int);

/*****************************************************************************/
int readargs()
{
int res;
args->addoption("verbose", "v", false, "verbose");
args->addoption("debug", "d", false, "debug");
args->addoption("hostname", "h", "localhost",
    "host running commu", "hostname");
args->use_env("hostname", "EMSHOST");
args->addoption("port", "p", DEFISOCK,
    "port for connection with commu", "port");
args->use_env("port", "EMSPORT");
args->addoption("sockname", "s", DEFUSOCK,
    "socket for connection with commu", "socket");
args->use_env("sockname", "EMSSOCKET");
args->addoption("vedname", 0, "", "Name des VED", "vedname");
args->addoption("slot", "slot", 1, "slotnumber of display", "slot");
args->addoption("code", 1, 0, "code", "code");
args->hints(C_readargs::required, "vedname", 0);
args->hints(C_readargs::exclusive, "hostname", "sockname", 0);
args->hints(C_readargs::exclusive, "port", "sockname", 0);

if ((res=args->interpret(1))!=0) return res;

debug=args->getboolopt("debug");
verbose=debug || args->getboolopt("verbose");
vedname=args->getstringopt("vedname");
if (debug) cout<<"vedname =\""<<vedname<<"\""<<endl;
slot=args->getintopt("slot");
code=args->getintopt("code");
return 0;
}
/*****************************************************************************/
int open_ved()
{
try
  {
  if (debug) cout<<"try communication.init"<<endl;
  if (!args->isdefault("hostname") || !args->isdefault("port"))
    {
    if (debug) cout << "use host \"" << args->getstringopt("hostname")
        << "\", port " << args->getintopt("port") << endl;
    communication.init(args->getstringopt("hostname"),
        args->getintopt("port"));
    }
  else
    {
    if (debug) cout << "use socket \""
        << args->getstringopt("sockname") << "\"" << endl;
    communication.init(args->getstringopt("sockname"));
    }
  if (debug) cout<<"try open ved"<<endl;
  }
catch (C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  return(-1);
  }
try
  {
  ved=new C_VED(vedname);
  is=ved->is0();
  }
catch (C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  communication.done();
  return(-1);
  }
if (verbose)
  cout<<"ved \""<<ved->name()<<"\" open via "<<communication<<endl;
return(0);
}
/*****************************************************************************/
void sleep_a_bit(int u)
{
usleep(u*100);
}
/*****************************************************************************/
// typedef enum {up, down} bang_dir;
// class bang
//   {
//   public:
//     bang(int pos, int speed);
//   protected:
//     static int map;
//     int pos, speed, timer;
//     bang_dir dir;
//   public:
//     void check();
//     void move();
//     operator int () const {return map;}
//   };
// int bang::map;
// 
// bang::bang(int pos, int speed)
// :pos(pos), speed(speed+1)
// {
// map|=1<<pos;
// if (pos==0)
//   dir=up;
// else
//   dir=down;
// timer=speed;
// }
// void bang::check()
// {
// if (dir==up)
//   {
//   if ((pos==23) || (map&(1<<(pos+1)))) dir=down;
//   }
// else
//   {
//   if ((pos==0) || (map&(1<<(pos-1)))) dir=up;
//   }
// }
// void bang::move()
// {
// if (--timer) return;
// map&=~(1<<pos);
// int npos=dir==up?pos+1:pos-1;
// if ((map&(1<<npos))==0)
//   {
//   pos=npos;
//   }
// map|=1<<pos;
// timer=speed;
// }
/*****************************************************************************/
void illu_count(int slot, int code)
{
int i;
int proc_id=is->procnum("nAFwrite");
for (i=0; i<0x1000000; i++)
  {
  is->command(proc_id, 4, 0, 0, 16, i);
  }
}
/*****************************************************************************/
void illu_up_down(int slot, int code)
{
int pattern=1;
int proc_id=is->procnum("nAFwrite");

while (1)
  {
  while (pattern!=0x800000)
    {
    is->command(proc_id, 4, 0, 0, 16, pattern);
    pattern<<=1;
    sleep_a_bit(100);
    }
  while (pattern!=0x000001)
    {
    is->command(proc_id, 4, 0, 0, 16, pattern);
    pattern>>=1;
    sleep_a_bit(100);
    }
  }
}
/*****************************************************************************/
struct bang_descr
  {
  float y; float v;
  };
bang_descr descr[]={
  {23.1, 0.}
  };

void illu_bong(int slot, int code)
{
int proc_id=is->procnum("nAFwrite");
float nextcrash;
bang *crashbang;
C_confirmation* conf;
timeval timeout;
FILE* f;

bang* bangs=0;
bang* hbang=0;

bang::gg(-0.0);
f=fopen("bang.dat", "r");
if (f==0)
  {
  cerr<<"cannot open bang.dat"<<endl;
  for (unsigned int i=0; i<sizeof(descr)/sizeof(bang_descr); i++)
    {
    cerr<<"adding bang("<<descr[i].y<<", "<<descr[i].v<<")"<<endl;
    hbang=new bang(hbang, descr[i].y, descr[i].v, 1., 1.);
    if (!bangs) bangs=hbang;
    }
  }
else
  {
  char s[1024];
  while (fgets(s, 1024, f))
    {
    switch (s[0])
      {
      case '#': break;
      case 'g':
        {
        float g;
        int res=sscanf(s+1, "%f", &g);
        cerr<<"set g to "<<g<<endl;
        if (res>0) bang::gg(g);
        }
        break;
      default:
        {
        float y, v0=0, mt=1, mst=1;
        int res=sscanf(s, "%f %f %f %f", &y, &v0, &mt, &mst);
        if (res>0)  
          {
          cerr<<"add "<<y<<"; "<<v0<<"; "<<mt<<"; "<<mst<<endl;
          hbang=new bang(hbang, y, v0, mt, mst);
          if (!bangs) bangs=hbang;
          }
        }
        break;
      }
    }
  fclose(f);
  }
ved->confmode(asynchron);
timeout.tv_sec=timeout.tv_usec=0;
nextcrash=0.; crashbang=0;
while (1)
  {
  bang::clearmap();
  hbang=bangs;
  hbang->move(nextcrash, &crashbang);
  hbang=hbang->next;
  while (hbang)
    {
    hbang->move(nextcrash, &crashbang);
    hbang=hbang->next;
    }
  hbang=bangs;
  nextcrash=hbang->check(); crashbang=hbang;
  hbang=hbang->next;
  while (hbang)
    {
    float t=hbang->check();
    if (t<nextcrash) {nextcrash=t; crashbang=hbang;}
    hbang=hbang->next;
    }
  is->command(proc_id, 4, 0, 0, 16, bang::getmap());
  while ((conf=communication.GetConf(&timeout))!=0) delete conf;
  sleep_a_bit(static_cast<int>(nextcrash));
  }
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
try
  {
  args=new C_readargs(argc, argv);
  if (readargs()!=0) return(1);

  proclist=0;
  if (open_ved()<0) return 2;
  switch (code)
    {
    case 1: illu_up_down(slot, code); break;
    case 2: illu_bong(slot, code); break;
    default: illu_count(slot, code);
    }
  delete ved;
  communication.done();
  }
catch (C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  }
}
/*****************************************************************************/
/*****************************************************************************/
