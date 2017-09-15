/*
 * clusterreader.cc
 * 
 * created: 12.03.1998 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
//#define _SOCKADDR_LEN
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <limits.h>
#include <sys/time.h>
#include <utime.h>
#include <sys/types.h>
#include <string.h>
#include "swap_int.h"
#include <errors.hxx>
#include <sockets.hxx>
#include "clusterreader.hxx"
#include "memory.hxx"
#include "inbuf.hxx"
#include <versions.hxx>
#include <debuglevel.hxx>
#include <xdrstring.h>

VERSION("May 10 2001", __FILE__, __DATE__, __TIME__,
"$ZEL: clusterreader.cc,v 1.19 2010/09/04 21:16:54 wuestner Exp $")
#define XVERSION

///////////////////////////////////////////////////////////////////////////////
Clusterreader::Clusterreader(const STRING& pathname, int literally)
:iopath_(pathname, C_iopath::iodir_input, literally), maxmem_(0), veds(0),
  statist_(this), save_header_(0)
{
switch (iopath_.typ())
  {
  case C_iopath::iotype_none: input_status_=input_error; return;
  case C_iopath::iotype_file:
  case C_iopath::iotype_socket:
  case C_iopath::iotype_fifo: reader=&Clusterreader::streamreader; break;
  case C_iopath::iotype_tape: reader=&Clusterreader::tapereader; break;
  }
input_status_=input_ok;
reset();
}
///////////////////////////////////////////////////////////////////////////////
Clusterreader::Clusterreader(const char* hostname, int port)
:iopath_(hostname, port, C_iopath::iodir_input), maxmem_(0), veds(0),
  statist_(this), save_header_(0)
{
if (iopath_.typ()!=C_iopath::iotype_socket)
  {
  cerr<<"Clusterreader::Clusterreader(hostname="<<hostname
      <<", port="<<port<<"):"<<endl;
  cerr<<"  iopath_.typ()!=iotype_socket"<<endl;
  return;
  }
reader=&Clusterreader::streamreader;
input_status_=input_ok;
reset();
}
///////////////////////////////////////////////////////////////////////////////
Clusterreader::Clusterreader(int path)
:iopath_(path, C_iopath::iodir_input), maxmem_(0), veds(0), statist_(this),
    save_header_(0)
{
switch (iopath_.typ())
  {
  case C_iopath::iotype_none: input_status_=input_error; return;
  case C_iopath::iotype_file:
  case C_iopath::iotype_socket:
  case C_iopath::iotype_fifo: reader=&Clusterreader::streamreader; break;
  case C_iopath::iotype_tape: reader=&Clusterreader::tapereader; break;
  }
input_status_=input_ok;
reset();
}
///////////////////////////////////////////////////////////////////////////////
Clusterreader::~Clusterreader()
{
delete[] veds;
}
///////////////////////////////////////////////////////////////////////////////
void Clusterreader::reset()
{
delete[] veds; veds=0; numveds=0;
num_cached_clusters=0;
next_evnum=1;
statist_.reset();
}
///////////////////////////////////////////////////////////////////////////////
void Clusterreader::restart()
{
iopath_.reopen();
if (iopath_.typ()==C_iopath::iotype_none)
  {
  input_status_=input_error;
  return;
  }
input_status_=input_ok;
reset();
}
///////////////////////////////////////////////////////////////////////////////
void Clusterreader::got_events(CLUSTERcontainer* cluster)
{
if (debuglevel::debug_level)
  {
  cerr<<(*cluster)<<endl;
  cerr<<endl;
  }
int vedidx=ved_info.vedid2vedidx(cluster->get_ved_id());
if (vedidx<0) // ved not found
  {
  OSTRINGSTREAM st;
  st<<"Panik in C_clusterreader::read_cluster: idx of ved "
    <<cluster->get_ved_id()<<" not found."<<endl;
  cluster->dump(st, 100);
  st<<ends;
  throw new C_program_error(st);
  }
veds[vedidx].add_cluster(cluster);
}
///////////////////////////////////////////////////////////////////////////////
void Clusterreader::got_ved_info(CLUSTERcontainer* cluster)
{
int i;
if (debuglevel::verbose_level)
  {
  cerr<<(*cluster)<<endl;
  if (debuglevel::debug_level)
    {
    cluster->dump(cerr, 80);
    cerr<<endl;
    }
  }
if (veds!=0)
  {
  cerr<<"Got VEDinfo Cluster; delete all old data."<<endl;
  delete[] veds;
  ved_info.reset();
  }
ved_info.vednum=cluster->get_vednum();
ved_info.isnum=cluster->get_isnum();
delete[] ved_info.islist;
ved_info.islist=new int[ved_info.isnum];
delete[] ved_info.indices;
ved_info.indices=new int[ved_info.vednum];
for (i=0; i<ved_info.vednum; i++) ved_info.indices[i]=-1;
int* ids=cluster->get_is_ids();
for (i=0; i<ved_info.isnum; i++) ved_info.islist[i]=ids[i];
numveds=ved_info.vednum;
veds=new VEDslot[numveds];
for (i=0; i<numveds; i++) veds[i].init(this);
}
///////////////////////////////////////////////////////////////////////////////
void Clusterreader::got_text(CLUSTERcontainer* cluster)
{
    if (debuglevel::verbose_level) {
        cerr<<(*cluster)<<endl;
        if (debuglevel::debug_level) {
            cluster->dump(cerr, 80);
            cerr<<endl;
        }
    }
    if (!save_header_)
        return;

    C_inbuf ib(cluster->get_data(), cluster->datasize());

    int flags, fragment_id, count;
    STRING str;

    ib>>flags>>fragment_id>>count;
    if (!count) {
        cerr<<"got_text: empty cluster"<<endl;
       return;
    }
    int index=ib.index();
    ib>>str;
    if (str.substr(0, 5)!="Key: ") {
        cerr<<"got_text: no key in first line: "<<str<<endl;
        return;
    }
    STRING name=str.substr(5, STRING::npos);
    if (name=="configuration") {
        STRING str2;
        ib>>str2;
        if (str2.substr(0, 5)!="name ") {
            cerr<<"got_text: no name in second line: "<<str2<<endl;
            return;
        }
        name+="_";
        name+=str2.substr(5, STRING::npos);
    }
    f_info.name=strdup((char*)name.c_str());
    make_file(f_info);
    if (f_info.path<0) {
        f_info.clear();
        return;
    }
    ib.index(index);

    while (count--) {
        ib>>str;
        if (write(f_info.path, str.c_str(), str.length())<0) {
            cerr<<"write error: "<<strerror(errno)<<endl;
            break;
        }
        if (write(f_info.path, "\n", 1)<0) {
            cerr<<"write error: "<<strerror(errno)<<endl;
            break;
        }
    }
    close(f_info.path);
    //cerr<<"saved text as "<<f_info.pathname<<endl;
    f_info.clear();
}
///////////////////////////////////////////////////////////////////////////////
void Clusterreader::got_file(CLUSTERcontainer* cluster)
{
    if (!save_header_)
        return;

    C_inbuf ib(cluster->get_data(), cluster->datasize());

    int flags, fragment_id;
    int ctime, mtime, perm, size;
    int rsize;
    struct utimbuf ubuf;
    STRING name;

    ib>>flags>>fragment_id>>name;
    ib>>ctime>>mtime>>perm>>size;
    ubuf.actime=mtime;
    ubuf.modtime=mtime;
    rsize=ib[ib.index()];

    if (fragment_id>0) {
        if ((fragment_id>0) && (name!=f_info.name)) {
            cerr<<"got_file: cannot complete file "<<name<<endl;
            cerr<<"  (previous information lost)"<<endl;
            f_info.clear();
            return;
    }
    if (fragment_id!=f_info.sequence+1) {
        cerr<<"got_file: cannot complete file "<<name<<endl;
        cerr<<"  (sequence out of order ("<<f_info.sequence<<" --> "
            <<fragment_id<<"))"<<endl;
        f_info.clear();
        return;
        }
    } else {
        f_info.name=strdup(name.c_str());
        make_file(f_info);
        if (f_info.path<0) {f_info.clear(); return;}
        f_info.filesize=size;
        f_info.size=0;
    }
    f_info.sequence=fragment_id;

    char* data;
    xdrstrcdup(&data, ib.buf()+ib.index());

    if (write(f_info.path, data, rsize)<0) {
        cerr<<"write error: "<<strerror(errno)<<endl;
    }
    f_info.size+=rsize;

    if (!(flags&clusterfl_fragfollows)) {
        close(f_info.path);
        //cerr<<"saved file "<<f_info.name<<" as "<<f_info.pathname<<endl;
        //cerr<<"  size="<<f_info.size<<", filesize="<<f_info.filesize<<endl;
        utime(f_info.pathname, &ubuf);
        f_info.clear();
    }
}
///////////////////////////////////////////////////////////////////////////////
void Clusterreader::got_no_more_data(CLUSTERcontainer* cluster)
{
if (debuglevel::verbose_level)
  {
  cerr<<(*cluster)<<endl;
  if (debuglevel::debug_level)
    {
    cluster->dump(cerr, 80);
    cerr<<endl;
    }
  }
input_status_=input_exhausted;
iopath_.close();
}
///////////////////////////////////////////////////////////////////////////////
void Clusterreader::purge_cluster(void)
{
int n=0;
unsigned int last_of_bottom=veds[0].get_last_of_bottom();
for (int i=1; i<numveds; i++)
  {
  unsigned int l=veds[i].get_last_of_bottom();
  if (l<last_of_bottom)
    {
    last_of_bottom=l;
    n=i;
    }
  }
veds[n].purge_cluster();
}
///////////////////////////////////////////////////////////////////////////////
int* Clusterreader::get_event(unsigned int ev)
{
int i;
int size=4;
int sevs=0;
int trigger=-1;

for (i=0; i<numveds; i++)
  if (veds[i].eventinfo(ev, size, sevs, trigger)<0)
    {
    cerr<<"event "<<next_evnum<<" lost."<<endl;
    veds[i].purge_events(next_evnum);
    next_evnum=ev+1;
    return (int*)0;
    }
int* event=new int[size];
event[0]=size-1;
//event[1]=next_evnum;
event[1]=0x12345678;
event[2]=trigger;
event[3]=sevs;
//printf("the size of int is %d\n",sizeof(int));
			for(int k=0;k<10;k++)
			{
		//	printf("event[ %d ] is 0x%08x \n",k, event[k]);	
			}
int idx=4;
for (i=0; i<numveds; i++) idx+=veds[i].get_subevent(next_evnum, event+idx);
statist_.events_extracted++;
statist_.bytes_extracted+=size*4;
next_evnum=ev+1;
return event;
}
///////////////////////////////////////////////////////////////////////////////
int* Clusterreader::get_next_event()
{
    int i;
    if (numveds==0) {
        read_cluster();
        return (int*)0;
    }
    for (i=0; i<numveds; i++) {
        int ev=veds[i].test_event(next_evnum);
        if (ev<0) { // event ist noch nicht da, kommt aber spaeter
            read_cluster();
            return (int*)0;
        } else if (ev>0) { // event ist nicht mehr da, ev ist firstevent
            cerr<<"events lost: jump from "<<next_evnum-1<<" to "<<ev<<endl;
            next_evnum=ev;
            return (int*)0;
        }
        // ev==0: event ist verfuegbar
    }
    return get_event(next_evnum);
}
///////////////////////////////////////////////////////////////////////////////
int* Clusterreader::get_best_event(int lowdelay)
{
int i;
if (numveds==0)
  {
//printf("In get_bes_event(): numveds ==0!\n");
  // Zahl der VEDs=0 oder noch nicht bekannt
  return (int*)0;
  }

// Nummer des juengsten kompletten Events bestimmen; ==> min_last
unsigned int min_last=veds[0].last_seen_event();
for (i=1; i<numveds; i++)
  {
//printf("In get_bes_event(): i=1;i<numveds!\n");
  unsigned int l=veds[i].last_seen_event();
  if (l<min_last) min_last=l;
  }
// alle Cluster loeschen, die das juengste Event nicht enthalten
if (lowdelay) {for (i=1; i<numveds; i++) veds[i].purge_events(min_last);
//printf("In get_bes_event(): if(lowdelay)!\n");
}

// Nummer des aeltesten kompletten Events bestimmen; ==> bottomevent
unsigned int bottomevent=veds[0].bottomevent();
//printf("In get_bes_event(): bottomevent = veds[0].bottomevent()!\n");
for (i=1; i<numveds; i++)
  {
  unsigned int l=veds[i].bottomevent();
//printf("In get_bes_event(): i=1,i<numveds!\n");
  if (l==0)
    {
    return (int*)0;
    }
  if (l>bottomevent) bottomevent=l;
  }

// next_evnum korrigieren
if (bottomevent>next_evnum) next_evnum=bottomevent;
//printf("In get_bes_event(): if (bottomevent>next_evnum)!\n");
//}

if (next_evnum>min_last)
  {
  // keine brauchbaren Events mehr vorhanden
  return (int*)0;
  }

// Event extrahieren
//printf("In get_bes_event(): before event = get_event(next evnum)!\n");
int* event=get_event(next_evnum);
//printf("In get_bes_event(): after event = get_event(next evnum)!\n");
if (event==0)
  {
//printf("In get_bes_event(): event = 0!\n");
  // sollte nie auftreten, nur Sicherheitsueberpruefung
  cerr<<"get_best_event: next_evnum="<<next_evnum<<", min_last="<<min_last
      <<", bottomevent="<<bottomevent<<endl;
  min_last=veds[0].last_seen_event();
  for (i=1; i<numveds; i++)
    {
    unsigned int l=veds[i].last_seen_event();
    if (l<min_last) min_last=l;
    }
  for (i=1; i<numveds; i++) veds[i].purge_events(min_last);
  bottomevent=veds[0].bottomevent();
  for (i=1; i<numveds; i++)
    {
    unsigned int l=veds[i].bottomevent();
    if (l==0)
      {
      read_cluster();
      return (int*)0;
      }
    if (l>bottomevent) bottomevent=l;
    }
  cerr<<"min_last="<<min_last<<", bottomevent="<<bottomevent<<endl;
  }
//printf("In get_bes_event(): return event\n");
return event;
}
///////////////////////////////////////////////////////////////////////////////
void Clusterreader::read_cluster()
{
while ((maxmem_!=0) && (memory::used_mem>maxmem_))
  {
//printf("In Clusterreader::read_cluster(),  while((maxmem!=0)\n");
  purge_cluster();
  }
//cerr<<"vor read cluster"<<endl;
CLUSTERcontainer* cluster=0;
//printf("In Clusterreader::read_cluster(),  cluster = 0;\n");
// try
//   {
  cluster=(this->*reader)();
//printf("In Clusterreader::read_cluster(),  cluster=(this->*reader)();\n");
//   }
// catch (C_status_error* e)
//   {
//   cerr<<(*e)<<endl;
//   delete e;
//   cerr<<"da fehlt was!!!"<<endl;
//   }
//cerr<<"nach read cluster"<<endl;
if (cluster==0) return;
statist_.print(0);
//printf("In Clusterreader::read_cluster(),  stattist_.print(0)\n");
switch (cluster->typ())
  {
  case clusterty_events: got_events(cluster); printf("the cluster type is events\n");break;
  case clusterty_ved_info: got_ved_info(cluster); printf("the cluster type is get_ved_info\n");break;
  case clusterty_text: got_text(cluster); printf("the cluster type is got_text\n");break;
  case clusterty_file: got_file(cluster); printf("the cluster type is got_file\n");break;
  case clusterty_no_more_data: got_no_more_data(cluster); printf("the cluster type is got_no_more_data\n");break;
  default:
    cerr<<"Warning!: got clustertype "<<cluster->typ()<<endl;
    break;
  }
}
///////////////////////////////////////////////////////////////////////////////
int Clusterreader::save_header(int save)
{
int t=save_header_;
save_header_=save;
return t;
}
///////////////////////////////////////////////////////////////////////////////
CLUSTERcontainer* Clusterreader::tapereader(void)
{
int size, da;
int* d;
try
  {
  d=new int[recsize_];
  da=iopath_.read(d, recsize_);
  }
catch (C_status_error* e)
  {
  switch (e->code())
    {
    case C_status_error::err_filemark:
      cerr<<"tapereader: filemark reached"<<endl;
      break;
    default: cerr<<"tapereader: "<<(*e)<<endl;
    }
  delete e;
  return (CLUSTERcontainer*)0;
  }
if (da<0)
  {
  delete[] d;
  throw new C_unix_error(errno, "read cluster");
  }
// else if (da==0)
//   {
//   delete[] d;
//   cerr<<"read cluster: filemark reached"<<endl;
//   return (CLUSTERcontainer*)0;
//   }
else
  {
  if (da&3)
    {
    delete[] d;
    OSTRINGSTREAM st;
    st<<"read cluster: recordsize="<<da<<ends;
    throw new C_program_error(st);
    }
  int wenden;
  if (d[1]==0x12345678) // host ordering
    wenden=0;
  else if (d[1]==0x78563412) // verkehrt
    wenden=1;
  else // unbekannt
    {
    delete[] d;
    OSTRINGSTREAM st;
    st<<"read cluster: byteorder="<<hex<<d[1]<<dec<<ends;
    throw new C_program_error(st);
    }
  size=da>>2;
  if (wenden) for (int i=0; i<size; i++) d[i]=swap_int(d[i]);
  if (size!=d[0]+1)
    {
    delete[] d;
    OSTRINGSTREAM st;
    st<<"read cluster: recordsize="<<size<<"; clustersize="<<d[0]+1<<ends;
    throw new C_program_error(st);
    }
  }
CLUSTERcontainer* cluster=new CLUSTERcontainer(d, size, num_cached_clusters);
return cluster;
}
///////////////////////////////////////////////////////////////////////////////
CLUSTERcontainer* Clusterreader::streamreader(void)
{
if (debuglevel::trace_level) cerr<<"streamreader called"<<endl;
int *d, kopf[2], size, wenden;
// header lesen
try
  {
  iopath_.read(kopf, 2*4);
  }
// catch(C_status_error* e)
//   {
//   switch (e->code())
//     {
//     case C_status_error::err_end_of_file:
//       cerr<<"end of file reached."<<endl;
//       break;
//     default:
//       cerr<<(*e)<<endl;
//     }
//   throw;
//   }
catch(C_error*)
  {
  iopath_.close();
  throw;
  }
// endian testen
if (kopf[1]==0x12345678) // host ordering
  {
  wenden=0;
  size=kopf[0]+1;
	//		for(int i=0;i<4;i++)
	//		{
	//		printf("buf->head[ %d ] is 0x%08x \n",i, kopf[i]);	
	//		}
  }
else if (kopf[1]==0x78563412) // verkehrt
  {
  wenden=1;
  size=swap_int(kopf[0])+1;
  }
else // unbekannt
  {
  OSTRINGSTREAM st;
  st<<"read cluster: byteorder="<<hex<<kopf[1]<<dec<<ends;
  throw new C_program_error(st);
  }
d=new int[size];
//printf("the d= new int[size] is %d, and size is %d \n",d,size);
if (d==0)
  {
  OSTRINGSTREAM st;
  st<<"read cluster:  alloc "<<size<<" words for new cluster"<<ends;
  throw new C_unix_error(errno, st);
  }
d[0]=kopf[0];
d[1]=kopf[1];
try
  {
  iopath_.read(d+2, (size-2)*4);
  }
catch(C_error*)
  {
  delete[] d;
  iopath_.close();
  throw;
  }
if (wenden) for (int i=0; i<size; i++) d[i]=swap_int(d[i]);
		//	for(int i=0;i<10;i++)
		//	{
		//	printf("cluster[ %d ] is 0x%08x \n",i, d[i]);	
		//	}
CLUSTERcontainer* cluster=new CLUSTERcontainer(d, size, num_cached_clusters);
return cluster;
}
///////////////////////////////////////////////////////////////////////////////
VEDslot::VEDslot(void)
:bottom(0), top(0), bottomevent_(0), topevent_(0), nextevent_(0), 
    last_seen_event_(0)
{}
///////////////////////////////////////////////////////////////////////////////
VEDslot::~VEDslot(void)
{
while (bottom)
  {
  CLUSTERcontainer* help=bottom->next;
  delete bottom;
  bottom=help;
  }
}
///////////////////////////////////////////////////////////////////////////////
void VEDslot::init(Clusterreader* r)
{
reader=r;
}
///////////////////////////////////////////////////////////////////////////////
void VEDslot::add_cluster(CLUSTERcontainer* cluster)
{
int numevents=cluster->get_num_events();
reader->statist_.clusters_read++;
//reader->statist_.bytes_read+=cluster->size()*4;
reader->statist_.events_read+=numevents;
if (numevents==0)
  {
  cerr<<"Warning: got empty eventcluster."<<endl;
  delete cluster;
  return;
  }
cluster->next_data=cluster->get_events();
unsigned int firstevent=cluster->next_data[1];
unsigned int lastevent=firstevent+numevents-1;
last_seen_event_=lastevent;
if (lastevent<reader->next_evnum)
  {
  delete cluster;
  return;
  }
if (topevent_ && (firstevent!=topevent_+1))
  {
  cerr<<"Warning: strange evnum for VED "<<cluster->get_ved_id()<<":"<<endl;
  cerr<<"  VED    : firstevent="<<bottomevent_<<"; lastevent="<<topevent_<<endl;
  cerr<<"  Cluster: firstevent="<<firstevent<<"; lastevent="<<lastevent<<endl;
  if (firstevent<=topevent_)
    {
    delete cluster;
    return;
    }
  }
if (bottomevent_==0) bottomevent_=firstevent;
topevent_=lastevent;
cluster->firstevent=firstevent;
cluster->lastevent=lastevent;
if (bottom)
  top->next=cluster;
else
  bottom=cluster;
top=cluster;
if (!nextevent_) nextevent_=firstevent;
}
///////////////////////////////////////////////////////////////////////////////
unsigned int VEDslot::get_last_of_bottom() const
{
if (bottom)
  return bottom->lastevent;
else
  return UINT_MAX;
}
///////////////////////////////////////////////////////////////////////////////
void VEDslot::purge_cluster()
{
if (bottom==0)
  {
  cerr<<"VEDslot::purge_cluster(): no cluster"<<endl;
  return;
  }
CLUSTERcontainer* help=bottom->next;
delete bottom;
bottom=help;
if (bottom)
  bottomevent_=bottom->firstevent;
else
  {
  bottomevent_=0;
  topevent_=0;
  top=0;
  }
if (!bottomevent_ || (nextevent_<bottomevent_)) nextevent_=bottomevent_;
}
///////////////////////////////////////////////////////////////////////////////
void VEDslot::purge_events(unsigned int ev)
{
while (bottom && bottom->lastevent<ev) purge_cluster();
}
///////////////////////////////////////////////////////////////////////////////
int VEDslot::test_event(unsigned int ev) const
{
    int event;
    if (ev<bottomevent_)
        event=bottomevent_;
    else if (ev>topevent_)
        event=-1;
    else
        event=0;
    return event;
}
///////////////////////////////////////////////////////////////////////////////
int VEDslot::eventinfo(unsigned int ev, int& size, int& sevs, int& trigger)
{
    purge_events(ev);
    if (!bottom)
        return -1;
    if (bottomevent_>ev)
        return -1;
    while ((bottom->next_data<bottom->get_top()) &&
            ((unsigned int)bottom->next_data[1]<ev)) {
        bottom->next_data+=bottom->next_data[0]+1;
    }
    if (bottom->next_data>=bottom->get_top()) {
        cerr<<"cluster exhausted"<<endl;
        purge_cluster();
        return -1;
    }

    if ((unsigned int)bottom->next_data[1]!=ev)
        return -1;

    size+=bottom->next_data[0]-3;
    sevs+=bottom->next_data[3];
    trigger=bottom->next_data[2];
    return 0;
}
///////////////////////////////////////////////////////////////////////////////
int VEDslot::get_subevent(unsigned int ev, int* event)
{
// vor get_subevent muss eventinfo aufgerufen werden
if ((unsigned int)bottom->next_data[1]!=ev)
  {
  throw new C_program_error("mismatch in VEDslot::get_subevent");
  }
bcopy((char*)(bottom->next_data+4), (char*)event, (bottom->next_data[0]-3)*4);
return bottom->next_data[0]-3;
}
///////////////////////////////////////////////////////////////////////////////
int Clusterreader::vedinfo::vedid2vedidx(int id)
{
if (vednum==0) throw new C_program_error("vedinfo: vednum==0");
int i;
for (i=0; i<vednum; i++) if (indices[i]==id) return i;
for (i=0; i<vednum; i++) if (indices[i]==-1) {indices[i]=id; return i;}
return -1;
}
///////////////////////////////////////////////////////////////////////////////
C_eventstatist::C_eventstatist(Clusterreader* r)
:reader(r)
{
reset();
}
///////////////////////////////////////////////////////////////////////////////
void C_eventstatist::reset()
{
clusters_read=0;
bytes_read=0;
events_read=0;
events_extracted=0;
bytes_extracted=0;
last_clusters_read=0;
last_events_extracted=0;
gettimeofday(&letzt, 0);
}
///////////////////////////////////////////////////////////////////////////////
void C_eventstatist::setup(int after_seconds_, int after_clusters_)
{
after_seconds=after_seconds_;
after_clusters=after_clusters_;
}
///////////////////////////////////////////////////////////////////////////////
void C_eventstatist::print(int immer)
{
if (!immer && !after_seconds && !after_clusters) return;
if (!reader->numveds) return;
struct timeval jetzt;
gettimeofday(&jetzt, 0);
float zeit=float(jetzt.tv_sec-letzt.tv_sec)+
    (float)(jetzt.tv_usec-letzt.tv_usec)/1000000.;
int ok=immer;
if (!ok && after_seconds) ok=zeit>=after_seconds;
if (!ok && after_clusters) ok=clusters_read>=last_clusters_read+after_clusters;
if (!ok) return;
cerr<<endl;
unsigned int max=UINT_MAX;
for (int i=0; i<reader->numveds; i++)
  {
  unsigned int ev=reader->veds[i].last_seen_event();
  if (ev<max) max=ev;
  }
cerr<<events_extracted<<" events of "<<max<<" extracted";
if (max>0)
  {
  cerr<<" ("<<(float)events_extracted/(float)max*100.<<"%)";
  }
if (zeit>0)
  {
  cerr<<" "<<(events_extracted-last_events_extracted)/zeit<<" events/s";
  cerr<<" "<<(bytes_extracted-last_bytes_extracted)/zeit/1024.<<" KBytes/s";
  }
cerr<<endl;
cerr<<clusters_read<<" clusters read, "
    <<reader->num_cached_clusters<<" clusters cached, "
    <<memory::used_mem/1048576.<<" MBytes of memory used"<<endl;
letzt=jetzt;
last_clusters_read=clusters_read;
last_events_extracted=events_extracted;
last_bytes_extracted=bytes_extracted;  
//last_bytes_read=bytes_read;  
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
