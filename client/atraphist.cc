/*
 * atraphist.cc
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
#include <debuglevel.hxx>

VERSION("Jun 18 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: atraphist.cc,v 1.3 2005/02/11 15:44:49 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_readargs* args;
STRING outfile;
int olit, vedport;
int dump, is_id, numevent;
Clusterreader* reader=0;
C_iopath writer;
STRING vedhost;
C_VED* ved;
C_instr_system *is0, *is;
int columns, numrows, *rows;
int* colstart;
int** hits;

struct sev_header
  {
  int IS_ID;
  int ndata;
  };
struct ev_header
  {
  int size;
  int evno;
  int trigno;
  int nsev;
  };

/*****************************************************************************/
int readargs()
{
args->addoption("outfile", 2, "-", "output file", "output");
args->addoption("olit", "oliterally", false,
    "use name of output file literally", "output file literally");
args->addoption("verbose", "verbose", false, "put out some comments", "verbose");
args->addoption("debug", "debug", false, "debug info", "debug");
args->addoption("dump", "dump", false, "dump data", "dump");
args->addoption("hostname", "h", "localhost",
    "host running commu", "hostname");
args->use_env("hostname", "EMSHOST");
args->addoption("port", "p", 4096,
    "port for connection with commu", "port");
args->use_env("port", "EMSPORT");
args->addoption("sockname", "s", "/var/tmp/emscomm",
    "socket for connection with commu", "socket");
args->use_env("sockname", "EMSSOCKET");
args->addoption("vedname", 0, "", "Name des VED", "vedname");
args->addoption("vedport", "vp", 9000, "dataoutport for VED", "vedport");
args->addoption("numevent", "num", 1000, "number of events", "numevents");
args->hints(C_readargs::required, "vedname", 0);

if (args->interpret(1)!=0) return(-1);

outfile=args->getstringopt("outfile");
olit=args->getboolopt("olit");
vedport=args->getintopt("vedport");
numevent=args->getintopt("numevent");
debuglevel::debug_level=args->getboolopt("debug");
debuglevel::verbose_level=debuglevel::debug_level||args->getboolopt("verbose");
dump=args->getboolopt("dump");
return 0;
}
/*****************************************************************************/
int init_arrays()
{
int i, j;

colstart=new int[columns];
if (!colstart) goto fehler;
numrows=0;
for (i=0; i<columns; i++)
  {
  colstart[i]=numrows;
  numrows+=rows[i];
  }
hits=new int*[numrows];
if (!hits) goto fehler;
for (i=0; i<numrows; i++)
  {
  hits[i]=new int[16];
  if (!hits[i]) goto fehler;
  }
return 0;
fehler:
cerr<<"cannot allocate memory."<<endl;
return -1;
}
/*****************************************************************************/
void clear_arrays()
{
for (int i=0; i<numrows; i++)
  {
  for (int j=0; j<16; j++) hits[i][j]=0;
  }
}
/*****************************************************************************/
void incr(int col, int row, int chan)
{
//cerr<<"incr("<<col<<", "<<row<<", "<<chan<<')'<<endl;
hits[colstart[col]+row][chan]++;
}
/*****************************************************************************/
int output(int dac_val)
{
cout<<dac_val;
for (int i=0; i<numrows; i++)
  {
  for (int j=0; j<16; j++)
    {
    cout<<' '<<hits[i][j];
    }
  }
cout<<endl;
return 0;
}
/*****************************************************************************/
int read_data(int dac_val)
{
if (debuglevel::verbose_level)
    cerr<<"do_convert_complete called for "<<numevent<<" events"<<endl;
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
    return -1;
    }

  if (event==(int*)-1)
    {
    cerr<<"got error from reader"<<endl;
    }
  else if (event==0)
    {
    //cerr<<"got no event from reader"<<endl;
    }
  else
    {
    if (debuglevel::debug_level)
        cerr<<"event "<<event[1]<<endl;
    if (++count<numevent)
      {
      if ((event[3]!=1) || (event[4]!=1))
        {
        cerr<<"event "<<event[1]<<": "<<event[3]<<" subevents; first ID: "
            <<event[4]<<endl;
        return -1;
        }
      if ((count%10)==0) cerr<<'.'<<flush;
      int num=event[6];
      unsigned char* data=(unsigned char*)(event+7);
      int col=0, row=0;
      for (int i=0; i<num; i++)
        {
        int d=data[i];
        int code=(d>>4)&0xf;
        int chan=d&0xf;
        switch (code)
          {
          case 0:
            if (i+1<num) {
              cerr<<"endekennung: i="<<i<<"; num="<<num<<endl;
              return -1;
            }
            break;
          case 4: incr(col, row, chan); break;
          case 5: row=0; col++; break;
          case 6: incr(col, row, chan); row++; break;
          case 7: row++; break;
          default:
            cerr<<"unknown code "<<code<<endl;
          }
        }
      }
    else
      {
      weiter=0;
      }
    delete[] event;
    }
  }
while (weiter);
cerr<<endl;
return 0;
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
  is=new C_instr_system(ved, 1, C_instr_system::open, 1);
  is_id=is->is_id();
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
    conf=is0->command("HLRALconfig");
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
    is->ro_add_proc("HLRALreadout");
    is->DownloadReadoutlist(1, 1, 1);
  } catch (C_error* e) {
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
    is0->command("MaxevCount");
    is0->add_param(numevent);
    is0->command("SetDebugMask");
    is0->add_param(4);
    delete is0->execute();
// hlral buffermode
    is0->clear_command();
    is0->command("HLRALbuffermode");
    is0->add_param(1);
    delete is0->execute();
  } catch (C_error* e) {
    cerr<<(*e)<<endl;
    delete e;
    return -1;
  }
return 0;
}
/*****************************************************************************/
int init_clusterreader()
{
reader=new Clusterreader((const char*)vedhost, vedport);

if (reader->input_status()!=Clusterreader::input_ok)
  {
  cerr<<"reader->input_status()="<<(int)reader->input_status()<<endl;
  delete reader;
  return -1;
  }
reader->statist().setup(0, 0);
if (debuglevel::verbose_level)
  {
  cerr<<reader->iopath()<<endl;
  }
return 0;
}
/*****************************************************************************/
int reinit_clusterreader()
{
reader->restart();
if (reader->input_status()!=Clusterreader::input_ok)
  return -1;
else
  return 0;
}
/*****************************************************************************/
void close_ved()
{
delete ved;
communication.done();
}
/*****************************************************************************/
int measure(int val)
{
  cerr<<"start measurement for threshold="<<val<<endl;
  try {
    is->execution_mode(delayed);
    is->clear_command();
    is->command("HLRALloaddac");
    is->add_param(4, 0, 0, -1, val);
    is->command("HLRALloaddac");
    is->add_param(4, 1, 0, -1, val);
    delete is->execute();
    is->clear_command();
    is->command("HLRALstartreadout");
    is->add_param(0);
    delete is->execute();
  } catch (C_error* e) {
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
  read_data(val);
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
void write_to_file()
{}
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
void do_work()
{
int first=1;
if (init_commu()) return;
if (ved_info()) return;
if (open_ved()) return;
if (init_ved()) return;
if (init_arrays()) return;
for (int i=255; i>=0; i--)
  {
  clear_arrays();
  if (first) {
    first=0;
    if (init_clusterreader()) return;
  } else {
    if (reinit_clusterreader()) return;
  }
  if (measure(i)) return;
  if (output(i)) return;
  }
write_to_file();
close_ved();
}
/*****************************************************************************/
main(int argc, char* argv[])
{
int res;
try
  {
  args=new C_readargs(argc, argv);
  if (readargs()!=0) return(0);

  writer.init(outfile, C_iopath::iodir_output, olit);
  if (writer.typ()==C_iopath::iotype_none)
    {
    delete reader;
    return 1;
    }
  if (debuglevel::verbose_level)
    {
    cerr<<writer<<endl;
    }
  if (writer.typ()==C_iopath::iotype_none)
    {
    delete reader;
    writer.close();
    return 1;
    }
  do_work();
  }
catch (C_error* e)
  {
  cerr<<(*e)<<endl;
  delete e;
  res=2;
  }
delete reader;
writer.close();
delete args;
return res;
}
/*****************************************************************************/
/*****************************************************************************/
