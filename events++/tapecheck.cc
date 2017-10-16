/*
 * tapecheck.cc
 * 
 * created: 15.Aug.2001 PW
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

VERSION("Aug 15 2001", __FILE__, __DATE__, __TIME__,
"$ZEL: tapecheck.cc,v 1.5 2005/02/11 15:45:12 wuestner Exp $")
#define XVERSION

C_readargs* args;
STRING infile;
int maxrec, maxirec;
int ilit;
C_iopath* inpath;

/*****************************************************************************/
class stringstack {
     public:
        stringstack();
        stringstack(char*);
        ostream& print(ostream&) const;
        void push(char*);
    protected:
        int numstr;
        int listsize;
        char** list;
};
ostream& operator <<(ostream&, const stringstack&);

class tstamp: public timeval {
    public:
	tstamp() {tv_sec=0; tv_usec=0;}
	void clear() {tv_sec=0; tv_usec=0;}
	ostream& print(ostream&) const;
};
ostream& operator <<(ostream&, const tstamp&);

class eventinfo {
    public:
    	eventinfo();
    	eventinfo(char* name);
    	~eventinfo();
	void clear(stringstack& caller);
        char* name;
	int id;
	int num_cluster;
	int num_events;
	int last_event;
	tstamp stamp;
};

class Counter {
    public:
        Counter();
	void clear(stringstack& caller);
    	void new_file(stringstack& caller);
	int file;
	eventinfo all_veds;
	int num_ved_info;
	tstamp stamp_ved_info;
	int num_text;
	tstamp stamp_text;
	int num_no_more_data;
	tstamp stamp_no_more_data;
	int num_veds;
	eventinfo* veds;
	int no_VED_Info_before_event_reported;
	int no_Text_before_event_reported;
	int event_after_no_more_data_reported;
	
	void got_events(tstamp stamp, int ved, int first_event,
	    	int num_events);
	void got_ved_info(tstamp stamp, int num_veds, int* veds);
	void got_text(tstamp stamp);
	void got_no_more_data(tstamp stamp);
	void dump_all();
};
Counter counter;
/*****************************************************************************/
void dump_cluster(int* c, int size)
{
    cout<<hex;
    for (int i=0; (i<16)&&(i<size); i++) {
        cout<<setw(8)<<c[i]<<" ";
    }
    cout<<endl<<dec;
}
/*****************************************************************************/
stringstack::stringstack()
:numstr(0), listsize(10)
{
    list=new char*[listsize];
}
/*****************************************************************************/
stringstack::stringstack(char* str)
:numstr(1), listsize(10)
{
    list=new char*[listsize];
    list[0]=strdup(str);
}
/*****************************************************************************/
void stringstack::push(char* str)
{
    if (listsize==numstr) {
        char** help=new char*[listsize+10];
        for (int i=0; i<numstr; i++) help[i]=list[i];
        delete[] list;
        list=help;
        listsize+=10;
    }
    list[numstr]=strdup(str);
    numstr++;
}
/*****************************************************************************/
ostream& operator<<(ostream& os, const stringstack& st)
{
    return st.print(os);
}
/*****************************************************************************/
ostream& stringstack::print(ostream& os) const
{
    for (int i=0; i<numstr; i++) {
        os<<list[i];
        if (numstr+1<i) os<<"->";
    }
    return os;
}
/*****************************************************************************/
eventinfo::eventinfo()
:name(0)
{
    //cout<<"eventinfo::eventinfo()"<<endl;
    stringstack callers("eventinfo::eventinfo");
    clear(callers);
}
/*****************************************************************************/
eventinfo::eventinfo(char* _name)
{
    //cout<<"eventinfo::eventinfo(name="<<_name<<")"<<endl;
    name=strdup(_name);
    stringstack callers("eventinfo::eventinfo(char* name)");
    clear(callers);
}
/*****************************************************************************/
eventinfo::~eventinfo()
{
    delete[] name;
}
/*****************************************************************************/
void eventinfo::clear(stringstack& caller)
{
//     if (name!=0)
//         cout<<"eventinfo::clear("<<name<<") called from "<<caller<<endl;
//     else
//         cout<<"eventinfo::clear() called from "<<caller<<endl;
    id=-1;
    num_cluster=0;
    num_events=0;
    last_event=0;
    stamp.clear();
}
/*****************************************************************************/
ostream& operator <<(ostream& os, const tstamp& stamp)
{
    return stamp.print(os);
}
/*****************************************************************************/
ostream& tstamp::print(ostream& os) const
{
    if (tv_sec==0)
        os<<"no timestamp";
    else {
        char s[1024];
        struct tm* tm;
        tm=localtime(&tv_sec);
        strftime(s, 1024, "%e. %b %Y %H:%M:%S", tm);
        os<<s;
    }
    return os;
}
/*****************************************************************************/
void Counter::dump_all(void)
{
    cout<<"Counter:"<<endl;
    cout<<"  file "<<file<<endl;
    cout<<"  "<<all_veds.num_cluster<<" eventcluster; "
    	    <<all_veds.num_events<<" events"<<endl;
    cout<<"  last event: "<<all_veds.last_event<<"; "<<all_veds.stamp<<endl;

    cout<<"  "<<num_ved_info<<" ved-infos; "<<stamp_ved_info<<endl;

    cout<<"  "<<num_text<<" textcluster; "<<stamp_text<<endl;

    cout<<"  "<<num_no_more_data<<" *no_more_data; "<<stamp_no_more_data<<endl;

    cout<<"  "<<num_veds<<" veds:"<<endl;

    for (int i=0; i<num_veds; i++) {
        cout<<"    "<<i<<": ";
        cout<<veds[i].num_cluster<<" cluster; "
    	    <<veds[i].num_events<<" events"<<endl;
        cout<<"        last event: "<<veds[i].last_event<<"; "
    	        <<veds[i].stamp<<endl;
    }
}
/*****************************************************************************/
Counter::Counter()
:file(0), veds(0), all_veds("all_veds")
{
    cout<<"Counter::Counter()"<<endl;
    stringstack callers("Counter::Counter()");
    clear(callers);
}
/*****************************************************************************/
void Counter::clear(stringstack& caller)
{
    cout<<"Counter::clear() called from "<<caller<<endl;
    caller.push("Counter::clear");
    //all_veds.clear(caller);
    num_ved_info=0;
    stamp_ved_info.clear();
    num_text=0;
    stamp_text.clear();
    num_no_more_data=0;
    stamp_no_more_data.clear();
    delete[] veds;
    veds=0;
    num_veds=0;
    no_VED_Info_before_event_reported=0;
    no_Text_before_event_reported=0;
    event_after_no_more_data_reported=0;
}
/*****************************************************************************/
void Counter::new_file(stringstack& caller)
{
    caller.push("Counter::new_file");
    clear(caller);
    file++;
}
/*****************************************************************************/
void Counter::got_ved_info(tstamp stamp, int _num_veds, int* _veds)
{
    int problem=0;
    cout<<"VED Info: "<<stamp<<"; stamp.tv_sec="<<stamp.tv_sec<<endl;
    cout<<"          num_veds="<<_num_veds<<endl;
    if (all_veds.num_events) {
        cout<<">>> events before VED INFO <<<"<<endl;
        problem=1;
    }
    if (num_ved_info) {
        cout<<">>> already have VED INFO <<<"<<endl;
        problem=1;
    }
    if (problem) counter.dump_all();
    num_ved_info++;
    stamp_ved_info=stamp;

    delete[] veds;
    num_veds=_num_veds;
    veds=new eventinfo[num_veds];
    
    for (int i=0; i<num_veds; i++) {
        cout<<"  "<<_veds[i];
        veds[i].id=_veds[i];
    }
    cout<<endl;
}
/*****************************************************************************/
void Counter::got_text(tstamp stamp)
{
    num_text++;
    stamp_text=stamp;
}
/*****************************************************************************/
void Counter::got_no_more_data(tstamp stamp)
{
    int problem=0;
    cout<<"no_more_data: "<<stamp<<endl;
    if (!all_veds.num_events) {
        cout<<">>> no events before NO_MORE_DATA <<<"<<endl;
        problem=1;
    }
    if (num_ved_info) {
        cout<<">>> already have NO_MORE_DATA <<<"<<endl;
        problem=1;
    }
    if (problem) counter.dump_all();
    num_no_more_data++;
    stamp_no_more_data=stamp;
}
/*****************************************************************************/
void Counter::got_events(tstamp stamp, int ved, int first_event,
	    	int num_events)
{
//     cout<<"got_events; stamp="<<stamp<<endl;
//     cout<<"            ved="<<ved<<" first_event="<<first_event
//         <<" num_events="<<num_events<<endl;
    int problem=0;
    int idx;
    if (!num_ved_info && !no_VED_Info_before_event_reported) {
        cout<<">>> no VED Info before EVENT <<<"<<endl;
        no_VED_Info_before_event_reported=1;
        problem=1;
    }
    if (!num_text && !no_Text_before_event_reported) {
        cout<<">>> no Text before EVENT <<<"<<endl;
        no_Text_before_event_reported=1;
        problem=1;
    }
    if (num_no_more_data && !event_after_no_more_data_reported) {
        cout<<">>> EVENT after no_more_data <<<"<<endl;
        cout<<"  num_no_more_data="<<num_no_more_data<<endl;
        event_after_no_more_data_reported=1;
        problem=1;
    }
    all_veds.num_cluster++;
    all_veds.num_events+=num_events;
    all_veds.last_event=first_event+num_events-1;
    all_veds.stamp=stamp;
    for (idx=0; idx<num_veds; idx++) if (veds[idx].id==ved) break;
    if ((idx<num_veds) && (veds[idx].last_event+1!=first_event)) {
        cout<<"ved "<<ved<<": jump from event "<<veds[idx].last_event<<" to "
        <<first_event<<endl;
        problem=1;
    }
//     if ((num_events<2) && (idx<num_veds) && veds[idx].num_cluster) {
//         cout<<num_events<<" events in EVENTCLUSTER"<<endl;
//         problem=1;
//     }

    if (problem) {
        cout<<"current event_cluster:"<<endl;
        cout<<"  stamp: "<<stamp<<endl;
        cout<<"  ved  : "<<ved<<endl;
        cout<<"  first event="<<first_event<<" number of events="<<num_events
            <<endl;
        counter.dump_all();
    }

    if (idx==num_veds) {
        cout<<"ved "<<ved<<" not found"<<endl;
        eventinfo* help;
        help=new eventinfo[num_veds+1];
        for (int i=0; i<num_veds; i++) help[i]=veds[i];
        help[num_veds].id=ved;
        delete[] veds;
        veds=help;
        idx=num_veds;
        num_veds++;
        problem=1;
    }
    veds[idx].num_cluster++;
    veds[idx].num_events+=num_events;
    veds[idx].last_event=first_event+num_events-1;
    veds[idx].stamp=stamp;
}
/*****************************************************************************/
static int readargs()
{
args->addoption("verbose", "verbose", 0, "verbose");
args->addoption("debug", "debug", false, "debug");
args->addoption("maxrec", "maxrec", 65536, "maxrec", "max. recordsize");
args->addoption("ilit", "iliterally", false,
    "use name of input file literally", "input file literally");
args->addoption("infile", 0, "/dev/nrmt0h", "input file", "input");

if (args->interpret(1)!=0) return(-1);

//verbose=args->getintopt("verbose");
//debug=args->getboolopt("debug");
infile=args->getstringopt("infile");
maxirec=args->getintopt("maxrec")/4;
maxrec=maxirec*4;
ilit=args->getboolopt("ilit");

return(0);
}
/*****************************************************************************/
int check_events(int* c, int size, tstamp stamp)
{
c++; // flags
int ved=*c++;
c++; // fragment_id
int num_events=*c++;
int first_event=c[1];

counter.got_events(stamp, ved, first_event, num_events);
return 0;
}
/*****************************************************************************/
int check_ved_info(int* c, int size, tstamp stamp)
{
//cout<<"check_ved_info; size="<<size<<"; sec="<<stamp.tv_sec<<endl;
//dump_cluster(c, size);
int version=*c++;
int num_veds=*c++;
//cout<<"                num_veds="<<num_veds<<endl;
int* vedids=new int[num_veds];
for (int i=0; i<num_veds; i++) {
    vedids[i]=*c++;
    //cout<<"                  id="<<vedids[i]<<endl;
    int num_is=*c++;
    //cout<<"                  num_is="<<num_is<<endl;
    c+=num_is;
}
counter.got_ved_info(stamp, num_veds, vedids);
delete[] vedids;
return 0;
}
/*****************************************************************************/
int check_text(int* c, int size, tstamp stamp)
{
counter.got_text(stamp);
return 0;
}
/*****************************************************************************/
int check_file(int* c, int size, tstamp stamp)
{
counter.got_text(stamp);
return 0;
}
/*****************************************************************************/
int check_no_more_data(int* c, int size, tstamp stamp)
{
counter.got_no_more_data(stamp);
return 0;
}
/*****************************************************************************/
static int check_cluster(int* c, int size, clustertypes typ)
{
    tstamp stamp;
    int res=0;

    //dump_cluster(c, size);
    int n_opt=*c++; size--;
    //cout<<"check_cluster: n_opt="<<n_opt<<endl;
    if (n_opt) {
    	int* pp=c;
	int optflags=*pp++;
        //cout<<"check_cluster: optflags="<<optflags<<endl;
	if (optflags&1) { /* timestamp */
            //cout<<"check_cluster: stamp.tv_sec ="<<*pp<<endl;
            //cout<<"check_cluster: stamp.tv_usec="<<pp[1]<<endl;
    	    stamp.tv_sec=*pp++;
    	    stamp.tv_usec=*pp++;
	}    
    }
    c+=n_opt; size-=n_opt;
    switch (typ) {
	case clusterty_events:
    	    res=check_events(c, size, stamp);
    	    break;
	case clusterty_ved_info:
    	    res=check_ved_info(c, size, stamp);
    	    break;
	case clusterty_text:
    	    res=check_text(c, size, stamp);
    	    break;
	case clusterty_file:
    	    res=check_file(c, size, stamp);
    	    break;
	case clusterty_no_more_data:
    	    res=check_no_more_data(c, size, stamp);
    	    break;
	default:
    	    cout<<"unknown clustertype "<<c[2]<<endl;
    }
    return res;
}
/*****************************************************************************/
static int check_record(int* c, clustertypes& typ)
{
    int res, complete;

    try {
	res=inpath->read(c, maxrec);
    }
    catch (C_status_error* e) {
	switch (e->code()) {
    	    case C_status_error::err_filemark:
      		res=0;
      		break;
    	    case C_status_error::err_end_of_file:
    	    case C_status_error::err_none:
    	    default:
      		cout<<"error: "<<(*e)<<endl;
	}
	delete e;
    }
    catch (C_error* e) {
	cout<<"!! error: "<<(*e)<<endl;
	delete e;
    }

    if (res==0) {
	cout<<"Filemark"<<endl;
    } else if (res<0)
	cout<<"read tape: "<<strerror(errno)<<endl;
    else {
	int endien=c[1];
	switch (endien) { // endientest
    	    case 0x12345678: // host byteorder
      		//if (debug) cout<<"host byteorder"<<endl;
      		break;
    	    case 0x78563412: { // verkehrt
      		//if (debug) cout<<"not host byteorder"<<endl;
      		for (int i=0; i<res/4; i++) c[i]=swap_int(c[i]);
      		}
      		break;
    	    default:
      		cout<<"unknown endien: "<<hex<<c[1]<<dec<<endl;
      		return -1;
	}
	int size=c[0]+1;
	if (size*4!=res) {
    	    cout<<"warning: clustersize="<<size<<" but got "<<res<<" bytes"<<endl;
	}
	typ=clustertypes(c[2]);
	check_cluster(c+3, size-3, typ);
    }
    return res;
}
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
    // open inputfile
    try {
	inpath=new C_iopath(infile, C_iopath::iodir_input, ilit);
	if (inpath->typ()==C_iopath::iotype_none) {
	cout<<"cannot open infile \""<<infile<<"\""<<endl;
	return;
	}
	/*if (verbose>0) */cout<<"infile: "<<(*inpath)<<endl;

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
