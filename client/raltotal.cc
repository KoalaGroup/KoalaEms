/*
 * raltotal.cc
 * 07.Apr.2003 PW: cxxcompat.hxx used
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <versions.hxx>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <readargs.hxx>
#include <errors.hxx>
#include <ved_addr.hxx>
#include <proc_is.hxx>
#include <proc_communicator.hxx>
#include "clusterreader.hxx"
#include "memory.hxx"
#include "swap_int.h"
#include <debuglevel.hxx>

VERSION("Oct 05 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: raltotal.cc,v 1.7 2005/02/11 15:45:10 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_readargs* args;
int use_infile;
STRING infile;
STRING vedhost;
C_VED* ved;
C_instr_system *is0, *is;
Clusterreader* reader=0;
int isid;
int ilit, vedport;
bool *values, *ovalues;
int numvals;
int columns, *rows, maxrows;
int pixh, pixv;
int pixhspace, pixvspace;
int cardhspace, cardvspace;
Display* display;
Window window=0;
Window* cratewin=0;
Window** cardwin=0;
GC gc0, gc1, gcl;
int simul;

/*****************************************************************************/

int readargs()
{
args->addoption("infile", "if", "", "input file", "input");
args->addoption("ilit", "iliterally", false,
    "use name of input file literally", "input file literally");
args->addoption("columns", "columns", 0, "columns", "columns");
args->addoption("rows", "rows", 0, "rows", "rows");
args->addoption("isid", "isid", 1, "isid", "isid");
args->addoption("display", "display", ":0", "display", "display");
args->addoption("hostname", "h", "localhost",
    "host running commu", "hostname");
args->use_env("hostname", "EMSHOST");
args->addoption("port", "p", 4096,
    "port for connection with commu", "port");
args->use_env("port", "EMSPORT");
args->addoption("sockname", "s", "/var/tmp/emscomm",
    "socket for connection with commu", "socket");
args->use_env("sockname", "EMSSOCKET");
args->addoption("vedname", "ved", "", "Name des VED", "vedname");
args->addoption("vedport", "vp", 9000, "dataoutport for VED", "vedport");
args->addoption("verbose", "verbose", false, "verbose", "verbose");
args->addoption("simul", "simul", false, "use simulation mode of server", "simul");

args->addoption("backcolor", "bc", "yellow", "background color", "background");
args->addoption("forecolor", "fc", "blue", "foreground color", "foreground");
//args->addoption("usedcolor", "uc", "GreenYellow", "color for used cells", "usedcolor");
args->addoption("usedcolor", "uc", "white", "color for used cells", "usedcolor");

args->addoption("geometry", "geometry", "", "window geometry", "geometry");

args->addoption("pixheight", "pixheight", 3, "pixel height", "pixheight");
args->addoption("pixwidth", "pixwidth", 3, "pixel width", "pixwidth");
args->addoption("pixhspace", "pixhspace", 1, "horizontal pixel space", "pixhspace");
args->addoption("pixvspace", "pixvspace", 1, "vertical pixel space", "pixvspace");
args->addoption("rowhspace", "rowhspace", 3, "horizontal row space", "rowhspace");
args->addoption("rowvspace", "rowvspace", 3, "vertical row space", "rowvspace");

args->hints(C_readargs::demands, "infile", "columns", "rows", (char*)0);

if (args->interpret(1)!=0) return(-1);

infile=args->getstringopt("infile");
ilit=args->getboolopt("ilit");
use_infile=!args->isdefault("infile");
isid=args->getintopt("isid");
vedport=args->getintopt("vedport");
debuglevel::verbose_level=debuglevel::debug_level||args->getboolopt("verbose");
simul=args->getboolopt("simul");
if (args->isdefault("infile") && args->isdefault("vedname"))
  {
  cerr<<"either infile or vedname must be given"<<endl;
  return -1;
  }
return(0);
}

/*****************************************************************************/
void incr(int col, int row, int chan, int slice)
{
//cerr<<"incr("<<col<<", "<<row<<", "<<chan<<", "<<slice<<')'<<endl;
values[((col*maxrows+row)*16+chan)*64+slice]=true;
}
/*****************************************************************************/
void display_it()
{
int i, j, k, l, idx;
//cerr<<"display_it()"<<endl;

idx=0;
for (i=0; i<columns; i++)
  {
  for (j=0; j<maxrows; j++)
    {
    Window win=cardwin[i][j];
    for (k=0; k<16; k++)
      {
      int x=cardhspace+(pixhspace+pixh)*k;
      for (l=0; l<64; l++)
        {
        if (values[idx]!=ovalues[idx])
          {
          int y=cardvspace+(pixvspace+pixv)*(63-l);
          bool v=values[idx];
          if (v)
            {
            XFillRectangle(display, win, gc1, x, y, pixh, pixv);
            }
          else
            {
            XFillRectangle(display, win, gcl, x, y, pixh, pixv);
            }
          ovalues[idx]=v;
          }
        idx++;
        }
      }
    }
  }
//XSync(display, 0);
}
/*****************************************************************************/
int init_display()
{
XEvent event;
Colormap colormap;
XColor screen_def_return, exact_def_return;
Status result;

int i, j, k, l;

int cardwinh, cardwinv, cratewinh, cratewinv, winh, winv;

cerr<<"init_display"<<endl;
pixh=args->getintopt("pixheight");
pixv=args->getintopt("pixwidth");
pixhspace=args->getintopt("pixhspace");
pixvspace=args->getintopt("pixvspace");
cardhspace=args->getintopt("rowhspace");
cardvspace=args->getintopt("rowvspace");

cardwinh=16*pixh+15*pixhspace+2*cardhspace;
cardwinv=64*pixv+63*pixvspace+2*cardvspace;
cratewinh=cardwinh*maxrows;
cratewinv=cardwinv;
winh=cratewinh;
winv=cratewinv*columns;

XSizeHints sizehints;
int screen;
unsigned long whitepixel, blackpixel;
unsigned long backpixel, forepixel, usedpixel;

if (args->isdefault("display"))
  display=XOpenDisplay("");
else
  display=XOpenDisplay(args->getstringopt("display"));
if (display==0)
  {
  cerr<<"cannot open Display"<<endl;
  return -1;
  }

//XSynchronize(display, 1);
screen=DefaultScreen(display);
colormap=DefaultColormap(display, screen);
whitepixel=WhitePixel(display, screen);
blackpixel=BlackPixel(display, screen);

if (XAllocNamedColor(display, colormap, args->getstringopt("backcolor"),
    &screen_def_return, &exact_def_return))
  backpixel=screen_def_return.pixel;
else
  {
  cerr<<"cannot allocate background color; will use white"<<endl;
  backpixel=whitepixel;
  }
if (XAllocNamedColor(display, colormap, args->getstringopt("forecolor"),
    &screen_def_return, &exact_def_return))
  forepixel=screen_def_return.pixel;
else
  {
  cerr<<"cannot allocate foreground color; will use black"<<endl;
  forepixel=blackpixel;
  }
if (XAllocNamedColor(display, colormap, args->getstringopt("usedcolor"),
    &screen_def_return, &exact_def_return))
  usedpixel=screen_def_return.pixel;
else
  {
  cerr<<"cannot allocate used-color; will use background"<<endl;
  usedpixel=backpixel;
  }

sizehints.x=100; sizehints.y=100;
sizehints.width=winh; sizehints.height=winv;
sizehints.flags=PPosition|PSize;
//cerr<<"x="<<sizehints.x<<", y="<<sizehints.y<<", w="<<sizehints.width<<", h="<<sizehints.height<<endl;
window=XCreateSimpleWindow(display, DefaultRootWindow(display),
  sizehints.x, sizehints.y, sizehints.width, sizehints.height,
  1, whitepixel, blackpixel);
XSetStandardProperties(display, window,
    "ral total readout", "ral total readout",
    None, (char**)0, 0, &sizehints);

cratewin=new Window[columns];
cardwin=new Window*[columns];
for (i=0; i<columns; i++)
  {
  cratewin[i]=XCreateSimpleWindow(display, window,
    0, i*cratewinv, cratewinh, cratewinv,
    1, blackpixel, whitepixel);
  XMapWindow(display, cratewin[i]);
  cardwin[i]=new Window[maxrows];
  for (j=0; j<maxrows; j++)
    {
    cardwin[i][j]=XCreateSimpleWindow(display, cratewin[i],
      j*cardwinh, 0, cardwinh, cardwinv,
      1, blackpixel, backpixel);
    XMapWindow(display, cardwin[i][j]);
    }
  }

gc0=XCreateGC(display, window, 0, 0);
XSetForeground(display, gc0, backpixel);
gc1=XCreateGC(display, window, 0, 0);
XSetForeground(display, gc1, forepixel);
gcl=XCreateGC(display, window, 0, 0);
XSetForeground(display, gcl, usedpixel);

XSelectInput(display, window, KeyPressMask | ExposureMask);
XMapRaised(display, window);

// for (i=0; i<columns; i++)
//   {
//   for (j=0; j<maxrows; j++)
//     {
//     Window win=cardwin[i][j];
//     for (k=0; k<16; k++)
//       {
//       int x=cardhspace+(pixhspace+pixh)*k;
//       for (l=0; l<64; l++)
//         {
//         int y=cardvspace+(pixvspace+pixv)*l;
//         XFillRectangle(display, win, gcl, x, y, pixh, pixv);
//         }
//       }
//     }
//   }
return 0;
}
/*****************************************************************************/
int init_arrays()
{
int i, j, k, l;

numvals=columns*maxrows*16*64;

values=new bool[numvals];
ovalues=new bool[numvals];
if (!values || !ovalues)
  {
  cerr<<"error: alloc "<<numvals<<"*bool"<<endl;
  return -1;
  }
bzero((char*)ovalues, numvals*sizeof(bool));
return 0;
}
/*****************************************************************************/
void clear_it()
{
bzero((char*)values, numvals*sizeof(bool));
}
/*****************************************************************************/
void copy_it()
{
bcopy((char*)values, (char*)ovalues, numvals*sizeof(bool));
}
/*****************************************************************************/
void scan_subevent(int* subevent, int size)
{
int num=subevent[0];
unsigned char* data=(unsigned char*)(subevent+1);
int col=0, row=0, chan, slice=0;
int count=0;

clear_it();

for (int i=0; i<num; i++)
  {
  int d=data[i];
  int code=(d>>4)&0x7;
  int chan=d&0xf;
  switch (code)
    {
    case 0:
      if (i+1<num) {
        cerr<<"endekennung: i="<<i<<"; num="<<num<<endl;
        return;
      }
      break;
    case 4: incr(col, row, chan, slice); count++; break;
    case 5:
      if (chan)
        {row=0; col=0; slice++;}
      else
        {row=0; col++;}
      break;
    case 6: incr(col, row, chan, slice); count++; row++; break;
    case 7: row++; break;
    default:
      cerr<<"unknown code "<<code<<endl;
    }
  }
display_it();
copy_it();
}
/*****************************************************************************/
void scan_event(int* event, int isid)
{
int evsize=event[0];
int subevents=event[3];
int *p=event+4;
for (int idx=0; idx<subevents; idx++)
  {
  int IS_ID=*p++;
  int size=*p++;
  if (IS_ID==isid)
    {
    scan_subevent(p, size);
    break;
    }
  else
    p+=size;
  }
}
/*****************************************************************************/
int handle_X()
{
XEvent event;
XNextEvent(display, &event);
switch (event.type)
  {
  case ConfigureNotify:
    //cerr<<"width="<<((XConfigureEvent*)&event)->width<<endl;
    //cerr<<"height="<<((XConfigureEvent*)&event)->height<<endl;
    break;
  case Expose:
  case GraphicsExpose:
    //cerr<<"Expose"<<endl;
    break;
  case MappingNotify:
    //cerr<<"MappingNotify"<<endl;
    XRefreshKeyboardMapping((XMappingEvent*)&event);
    break;
  case KeyPress:
    //cerr<<"KeyPress: "<<XLookupKeysym((XKeyEvent*)&event, 0)<<endl;
    if (XLookupKeysym((XKeyEvent*)&event, 0)=='q') return 1;
    break;
  case ClientMessage:
    //cerr<<"ClientMessage"<<endl;
    break;
  }
}
/*****************************************************************************/
void read_data()
{
int count=0, weiter=1;
int socketused;
int xsocket, datasocket;
int nfds;
fd_set readfds;
struct timeval tv;

socketused=reader->iopath().typ()==C_iopath::iotype_socket;
if (socketused)
  {
  datasocket=reader->iopath().path();
  xsocket=ConnectionNumber(display);
  if (datasocket>xsocket) nfds=datasocket+1; else nfds=xsocket+1;
  }

do
  {
  int* event=0;
  try
    {
    if (socketused)
      {
      int res;
      FD_ZERO(&readfds);
      FD_SET(xsocket, &readfds);
      FD_SET(datasocket, &readfds);
      res=select(nfds, &readfds, 0, 0, 0);
      if ((res<0) && errno!=EINTR)
        {
        cerr<<"select: "<<strerror(errno)<<endl;
        return;
        }
      else if (res>0)
        {
        if (FD_ISSET(xsocket, &readfds))
          weiter=!handle_X();
        else if (FD_ISSET(datasocket, &readfds))
          {
          XSync(display, 0);
          event=reader->get_next_event();
          }
        else
          cerr<<"select: komischer Pfad"<<endl;
        }
      }
    else
      {
      if (XPending(display))
        weiter=!handle_X();
      else
        event=reader->get_next_event();
      }
    }
  catch (C_error* e)
    {
    cerr<<(*e)<<endl;
    delete e;
    cerr<<count<<" events processed so far."<<endl;
    return;
    }

  if (event!=0)
    {
    scan_event(event, isid);
    count++;
    delete[] event;
    }
  }
while (weiter);
}
/*****************************************************************************/
int measure()
{
  cerr<<"starting measurement"<<endl;
  if (simul) {
    try {
      is->execution_mode(delayed);
      is->clear_command();
      is->command("HLRALstartreadoutDummy");
      is->add_param(2, 0, 1);
      cerr<<"execute HLRALstartreadoutDummy 0 1 ..."<<flush;
      delete is->execute();
      cerr<<"OK."<<endl;
    } catch (C_error* e) {
      cerr<<"Error."<<endl;
      cerr<<(*e)<<endl;
      delete e;
      return -1;
    }
    try {
      ved->StartReadout();
    } catch (C_error* e) {
      cerr<<(*e)<<endl;
      delete e;
      return -1;
    }
  } else {
    try {
      is->execution_mode(delayed);
      is->clear_command();
      is->command("HLRALstartreadout");
      is->add_param(2, 0, 1);
      cerr<<"execute HLRALstartreadout 0 1 ..."<<flush;
      delete is->execute();
      cerr<<"OK."<<endl;
    } catch (C_error* e) {
      cerr<<"Error."<<endl;
      cerr<<(*e)<<endl;
      delete e;
      return -1;
    }
    try {
      ved->StartReadout();
    } catch (C_error* e) {
      cerr<<(*e)<<endl;
      delete e;
      return -1;
    }
  }
  read_data();

  try {
    ved->ResetReadout();
    int afterevents=0;
    cerr<<"deleting already queued events..."<<flush;
    while (reader->input_status()!=Clusterreader::input_exhausted)
      {
      int* event;
      event=reader->get_next_event();
      if (event)
        {
        afterevents++;
        delete[] event;
        }
      }
    cerr<<"   "<<afterevents<<" events deleted."<<endl;
    ved->DeleteDataout(0);
  } catch (C_error* e) {
    cerr<<(*e)<<endl;
    delete e;
    return -1;
  }
  return 0;
}
/*****************************************************************************/
int ved_info()
{
try
  {
  C_VED* commu=new C_VED("commu");
  C_instr_system* is=commu->is0();
  if (is==0)
    {
    cerr<<"VED "<<commu->name()<<" has no procedures."<<endl;
    return -1;
    }
  C_confirmation* conf=is->command("VEDaddress", args->getstringopt("vedname"));
  C_inbuf inbuf(conf->buffer(), conf->size());
  delete conf;
  inbuf++; // fehlercode ==0
  if (inbuf.getint()==0)
    {
    cout << "VED "<<args->getstringopt("vedname")<<" does not exist." << endl;
    return -1;
    }
  C_VED_addr* addr=extract_ved_addr(inbuf);
  cout<<addr<<endl;
  switch (addr->type())
    {
    case C_VED_addr::inet:
      vedhost=((C_VED_addr_inet*)addr)->hostname();
      break;
    default:
      cerr<<"can not (yet) use VED with address "<<addr<<endl;
      delete addr;
      return -1;
    }
  delete addr;
  }
catch(C_error* e)
  {
  cerr << endl << (*e) << endl;
  delete e;
  return(-1);
  }
cerr<<"hostname= >"<<vedhost<<"<"<<endl;
return(0);
}
/*****************************************************************************/
int init_commu()
{
try
  {
  if (!args->isdefault("hostname") || !args->isdefault("port"))
    {
    if (debuglevel::verbose_level)
        cerr<<"use host "<<args->getstringopt("hostname")<<", port "
        << args->getintopt("port")<<" for communication: "<<flush;
    communication.init(args->getstringopt("hostname"),
        args->getintopt("port"));
    }
  else //if (!args->isdefault("sockname"))
    {
    if (debuglevel::verbose_level)
        cerr<<"use socket "<<args->getstringopt("sockname")
        <<" for communication: "<<flush;
    communication.init(args->getstringopt("sockname"));
    }
  if (debuglevel::verbose_level) cerr<<"ok."<<endl;
  }
catch(C_error* e)
  {
  cerr<<(*e)<<endl;
  delete e;
  return -1;
  }
return 0;
}
/*****************************************************************************/
int open_ved()
{
try
  {
  if (debuglevel::verbose_level)
      cerr<<"open "<<args->getstringopt("vedname")<<": "<<flush;
  ved=new C_VED(args->getstringopt("vedname"));
  if (debuglevel::verbose_level) cout<<"ok."<<endl;
  is0=ved->is0();
  if (is0==0)
    {
    cerr<<"VED "<<ved->name()<<" has no procedures."<<endl;
    return -1;
    }
  is=new C_instr_system(ved, 1, C_instr_system::open, isid);
  isid=is->is_id();
  }
catch(C_error* e)
  {
  cerr<<(*e)<<endl;
  delete e;
  return -1;
  }
return 0;
}
/*****************************************************************************/
int init_ved()
{
// determine current status
  try {
    InvocStatus status;
    status=ved->GetReadoutStatus(0);
    switch (status) {
      case Invoc_error:
      case Invoc_alldone:
      case Invoc_stopped:
      case Invoc_active:
        cerr<<"Readout is running; please stop it first."<<endl;
        return -1;
        break;
      case Invoc_notactive: break;
      default:
        cerr<<"unknown ReadoutStatus "<<(int)status<<endl;
        return -1;
    };
  } catch (C_error* e) {
    cerr<<(*e)<<endl;
    delete e;
    return -1;
  }
// determine RAL confuguration
  if (simul)
    {
    try {
      C_confirmation* conf;
      int i;
      is0->execution_mode(immediate);
      is0->clear_command();
      delete is0->command("HLRALconfigDummy", 3, 0, 11, 13);
      conf=is0->command("HLRALconfigDummy", 1, 0);
      columns=conf->size()-1;
      rows=new int[columns];
      for (i=0; i<columns; i++) rows[i]=conf->buffer(i+1);
      delete conf;
      cerr<<"RAL configuration:";
      for (i=0; i<columns; i++) cerr<<' '<<rows[i];
      cerr<<endl;
    } catch (C_error* e) {
      cerr<<(*e)<<endl;
      delete e;
      return -1;
    }
    }
  else
    {
    try {
      C_confirmation* conf;
      int i;
      is0->execution_mode(immediate);
      is0->clear_command();
      conf=is0->command("HLRALconfig", 2, 0, 1);
      columns=conf->size()-1;
      rows=new int[columns];
      for (i=0; i<columns; i++) rows[i]=conf->buffer(i+1);
      delete conf;
      cerr<<"RAL configuration:";
      for (i=0; i<columns; i++) cerr<<' '<<rows[i];
      cerr<<endl;
    } catch (C_error* e) {
      cerr<<(*e)<<endl;
      delete e;
      return -1;
    }
    }
maxrows=rows[0];
for (int i=1; i<columns; i++)
    {if (rows[i]>columns) maxrows=rows[i];}
// download trigger
  try {
    ved->DeleteTrigger();
  } catch (C_error* e) {
    //cerr<<(*e)<<endl;
    delete e;
  }
  if (simul) {
    try {
      ved->clear_triglist();
      ved->trig_set_proc("Immer");
      ved->trig_add_param(1);
      ved->DownloadTrigger();
    } catch (C_error* e) {
      cerr<<(*e)<<endl;
      delete e;
      return -1;
    }
  } else {
    try {
      ved->clear_triglist();
      ved->trig_set_proc("zel");
      ved->trig_add_param("/tmp/sync0");
      ved->trig_add_param(3, 1, 10, 2);
      ved->DownloadTrigger();
    } catch (C_error* e) {
      cerr<<(*e)<<endl;
      delete e;
      return -1;
    }
  }
// create dataout
  try {
    ved->DeleteDataout(0);
  } catch (C_error* e) {
    //cerr<<(*e)<<endl;
    delete e;
  }
  try {
    ved->DownloadDataout(0, InOut_Cluster, 1, 0, Addr_Socket, "0.0.0.0", 9000);
  } catch (C_error* e) {
    cerr<<(*e)<<endl;
    delete e;
    return -1;
  }
// load empty memberlist
  try {
    is->DeleteISModullist();
  } catch (C_error* e) {
    //cerr<<(*e)<<endl;
    delete e;
  }
  try {
    is->DownloadISModullist(0, 0);
  } catch (C_error* e) {
    cerr<<(*e)<<endl;
    delete e;
    return -1;
  }
// load readoutlist
  try {
    is->DeleteReadoutlist(1);
  } catch (C_error* e) {
    //cerr<<(*e)<<endl;
    delete e;
  }
  if (simul) {
    try {
      cerr<<"download readoutlist... "<<flush;
      is->ro_add_proc("HLRALreadoutDummy");
      is->ro_add_param(0);
      is->DownloadReadoutlist(1, 1, 1);
      cerr<<" OK."<<endl;
    } catch (C_error* e) {
      cerr<<" Error."<<endl;
      cerr<<(*e)<<endl;
      delete e;
      return -1;
    }
  } else {
    try {
      cerr<<"download readoutlist... "<<flush;
      is->ro_add_proc("HLRALreadout");
      is->ro_add_param(0);
      is->DownloadReadoutlist(1, 15,
          1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
      cerr<<" OK."<<endl;
    } catch (C_error* e) {
      cerr<<" Error."<<endl;
      cerr<<(*e)<<endl;
      delete e;
      return -1;
    }
  }
// some initprocedures
  if (simul) {
    try {
      is0->execution_mode(delayed);
      is0->clear_command();
      is0->command("ClusterEvMax");
      is0->add_param(1);
      //is0->command("MaxevCount");
      //is0->add_param(numevent);
      //is0->command("SetDebugMask");
      //is0->add_param(4);
      cerr<<"execute ClusterEvMax ... "<<flush;
      delete is0->execute();
      cerr<<" OK."<<endl;
    } catch (C_error* e) {
      cerr<<" Error."<<endl;
      cerr<<(*e)<<endl;
      delete e;
      return -1;
    }
  } else {
    try {
      is0->execution_mode(delayed);
      is0->clear_command();
      is0->command("InitPCITrigger");
      is0->add_param("/tmp/sync0");
      is0->add_param(1);
      is0->command("ClusterEvMax");
      is0->add_param(1);
      //is0->command("MaxevCount");
      //is0->add_param(numevent);
      //is0->command("SetDebugMask");
      //is0->add_param(4);
      cerr<<"execute InitPCITrigger /tmp/sync0 1 ... "<<flush;
      delete is0->execute();
      cerr<<" OK."<<endl;
//   // hlral buffermode
//       is0->clear_command();
//       is0->command("HLRALbuffermode");
//       is0->add_param(2, 0, 1);
//       cerr<<"execute HLRALbuffermode 0 1 ... "<<flush;
//       delete is0->execute();
//       cerr<<" OK."<<endl;
    } catch (C_error* e) {
      cerr<<" Error."<<endl;
      cerr<<(*e)<<endl;
      delete e;
      return -1;
    }
  }
return 0;
}
/*****************************************************************************/
int init_clusterreader()
{
if (use_infile)
  {
  reader=new Clusterreader(infile, ilit);
  }
else
  {
  reader=new Clusterreader(vedhost.c_str(), vedport);
  }

if (reader->input_status()!=Clusterreader::input_ok)
  {
  cerr<<"reader->input_status()="<<(int)reader->input_status()<<endl;
  delete reader;
  return -1;
  }
reader->statist().setup(10, 0);
if (debuglevel::verbose_level)
  {
  cerr<<reader->iopath()<<endl;
  }
return 0;
}
/*****************************************************************************/
void close_ved()
{
delete ved;
communication.done();
}
/*****************************************************************************/
void do_work()
{
if (use_infile)
  {
  columns=args->getintopt("columns");
  int rownum=args->getintopt("rows");
  rows=new int[columns];
  for (int i=0; i<columns; i++) rows[i]=rownum;
  maxrows=rownum;
  }
else
  {
  if (init_commu()) return;
  if (ved_info()) return;
  if (open_ved()) return;
  if (init_ved()) return;
  }
if (init_arrays()) return;
if (init_clusterreader()) return;
if (init_display()) return;
if (use_infile)
  read_data();
else
  {
  measure();
  close_ved();
  }
}
/*****************************************************************************/

main(int argc, char* argv[])
{
int res=0;
args=0;

try
  {
  args=new C_readargs(argc, argv);
  if (readargs()!=0) return(0);

  do_work();
  }
catch (C_error* e)
  {
  cerr<<(*e)<<endl;
  delete e;
  res=2;
  }
delete reader;
delete args;
return res;
}
/*****************************************************************************/
/*****************************************************************************/
