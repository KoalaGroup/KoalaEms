/*
 * raldecoder.cc
 * 07.Apr.2003 PW: cxxcompat.hxx used
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <versions.hxx>
#include <readargs.hxx>
#include <errors.hxx>
#include <ved_addr.hxx>
#include <proc_is.hxx>
#include <proc_communicator.hxx>
#include "clusterreader.hxx"
#include "memory.hxx"
#include "swap_int.h"
#include <debuglevel.hxx>

VERSION("Oct 06 2000", __FILE__, __DATE__, __TIME__,
"$ZEL: raldecoder.cc,v 1.4 2005/02/11 15:45:10 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_readargs* args;
bool use_infile, use_outfile;
STRING infile, ofile;
STRING vedhost;
C_VED* ved=0;
C_instr_system *is0, *is;
Clusterreader* reader=0;
C_iopath* writer=0;
int isid;
int evnum, maxev;
bool ilit, olit;
int vedport;
int columns, numrows, *rows;
C_outbuf outbuf(16, 16);

/*****************************************************************************/

int readargs()
{
args->addoption("infile", "if", "", "input file", "input");
args->addoption("ofile", "of", "", "output file", "output");
args->addoption("ilit", "iliterally", false,
    "use name of input file literally", "input file literally");
args->addoption("olit", "oliterally", false,
    "use name of output file literally", "output file literally");
args->addoption("isid", "isid", 1, "isid", "isid");
args->addoption("evnum", "evnum", 0, "number of events to be copied", "evnum");
args->addoption("maxev", "maxev", 0, "id of last event to be copied", "maxev");
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
args->addoption("debug", "debug", false, "debug", "debug");

if (args->interpret(1)!=0) return(-1);

infile=args->getstringopt("infile");
ofile=args->getstringopt("ofile");
ilit=args->getboolopt("ilit");
olit=args->getboolopt("olit");
use_infile=(bool)!args->isdefault("infile");
use_outfile=(bool)!args->isdefault("ofile");
isid=args->getintopt("isid");
vedport=args->getintopt("vedport");
evnum=args->getintopt("evnum");
maxev=args->getintopt("maxev");
debuglevel::debug_level=args->getboolopt("debug");
debuglevel::verbose_level=debuglevel::debug_level||args->getboolopt("verbose");
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
if (use_outfile)
  {
  unsigned int code;
  code=((slice&0xff)<<24)|((col&0xff)<<16)|((row&0xff)<<8)|(chan&0xff);
  outbuf<<code;
  }
else
  cerr<<"incr("<<col<<", "<<row<<", "<<chan<<", "<<slice<<')'<<endl;
}
/*****************************************************************************/
int scan_subevent(int* subevent, int size)
{
int num=subevent[0];
unsigned char* data=(unsigned char*)(subevent+1);
int col=0, row=0, chan, slice=0;
int count=0;

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
      }
      return count;
      break;
    case 4: incr(col, row, chan, slice); count++; break;
    case 5:
      if (chan)
        {row=0; col=0; slice++;} // Zeitscheibe beim Testreadout
      else
        {row=0; col++;}
      break;
    case 6: incr(col, row, chan, slice); row++; break;
    case 7: row++; break;
    default:
      cerr<<"unknown code "<<code<<endl;
    }
  }
cerr<<"no end marker!"<<endl;
return count;
}
/*****************************************************************************/
void scan_event(int* event, int isid)
{
int evsize=event[0];
int subevents=event[3];
int *p=event+4;
int sevcount=0;
int count;

for (int idx=0; idx<subevents; idx++)
  {
  int IS_ID=*p++;
  int size=*p++;
  if (IS_ID==isid)
    {
    count=scan_subevent(p, size);
    sevcount++;
    break;
    }
  else
    p+=size;
  }
// if ((sevcount!=1) || !count)
//   {
//   cerr<<"MIST Event!"<<endl<<hex;
//   for (int i=0; i<=evsize; i++) cerr<<event[i]<<" ";
//   cerr<<dec<<endl;
//   }
}
/*****************************************************************************/
void read_data()
{
if (debuglevel::trace_level) cerr<<"do_convert_complete called"<<endl;
int count=0, weiter=1;
do
  {
  int* event;
  try
    {
    event=reader->get_next_event();
    }
  catch (C_error* e)
    {
    cerr<<(*e)<<endl;
    delete e;
    cerr<<count<<" events processed so far."<<endl;
    return;
    }

  if (event==0)
    {
    //cerr<<"got no event from reader"<<endl;
    }
  else
    {
    //cerr<<"got event "<<event[1]<<endl;
    if (event[1]%1000==0) cerr<<"got event "<<event[1]<<endl;
    if (debuglevel::trace_level) cerr<<"event "<<event[1]<<endl;
    if ((maxev==0) || (event[1]<=maxev))
      {
      if (use_outfile) outbuf.clear();
      scan_event(event, isid);
      count++;
      if (use_outfile) outbuf.write(*writer);
      }
    else
      weiter=0;
    delete[] event;
    if ((evnum!=0) && (count>=evnum)) weiter=0;
    }
  }
while (weiter);
cerr<<count<<" events processed"<<endl;
}
/*****************************************************************************/
int measure()
{
  cerr<<"starting measurement"<<endl;
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
  read_data();
  // never reached
  try {
    ved->ResetReadout();
    do
      {
      int* event;
      event=reader->get_next_event();
      if (event)
        {
        cerr<<"got after-event?"<<endl;
        delete[] event;
        }
      }
    while (reader->input_status()!=Clusterreader::input_exhausted);
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
// download trigger
  try {
    ved->DeleteTrigger();
  } catch (C_error* e) {
    cerr<<(*e)<<endl;
    delete e;
  }
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
// create dataout
    try {
    ved->DeleteDataout(0);
  } catch (C_error* e) {
    cerr<<(*e)<<endl;
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
    cerr<<(*e)<<endl;
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
    cerr<<(*e)<<endl;
    delete e;
  }
  try {
    cerr<<"download readoutlist... "<<flush;
    is->ro_add_proc("HLRALreadout");
    is->ro_add_param(0);
    is->DownloadReadoutlist(1, 1, 1);
    cerr<<" OK."<<endl;
  } catch (C_error* e) {
    cerr<<" Error."<<endl;
    cerr<<(*e)<<endl;
    delete e;
    return -1;
  }
// some initprocedures
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
// // hlral buffermode
//     is0->clear_command();
//     is0->command("HLRALbuffermode");
//     is0->add_param(2, 0, 1);
//     cerr<<"execute HLRALbuffermode 0 1 ... "<<flush;
//     delete is0->execute();
//     cerr<<" OK."<<endl;
  } catch (C_error* e) {
    cerr<<" Error."<<endl;
    cerr<<(*e)<<endl;
    delete e;
    return -1;
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
  reader=0;
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
int init_writer()
{
writer=new C_iopath(ofile, C_iopath::iodir_output, olit);
if (writer->typ()==C_iopath::iotype_none) return 1;
if (debuglevel::verbose_level)
  {
  cerr<<(*writer)<<endl;
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
if (!use_infile)
  {
  if (init_commu()) return;
  if (ved_info()) return;
  if (open_ved()) return;
  if (init_ved()) return;
  }
if (init_clusterreader()) return;
if (use_outfile)
  {
  if (init_writer()) return;
  }
if (use_infile)
  read_data();
else
  measure();

if (!use_infile) close_ved();
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
