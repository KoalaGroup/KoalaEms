/*
 * tapescan_c.cc
 * 
 * created: 10.07.1998 PW
 * 13.Aug.2001 PW: clusterty_file added
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <readargs.hxx>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <events.hxx>
#include <compat.h>
#include "swap_int.h"
#include <clusterformat.h>
#include <xdrstrdup.hxx>
#include "versions.hxx"

VERSION("Aug 13 2001", __FILE__, __DATE__, __TIME__,
"$ZEL: tapescan_c.cc,v 1.14 2004/11/26 14:40:21 wuestner Exp $")
#define XVERSION

extern int verbose;
extern int debug;
extern int ask, oldtext;
extern int protocol;
extern STRING protocolname;
extern int single;
extern int fast;
extern int maxrec, maxirec;
extern int ilit, olit;
extern int runnr;
extern STRING headline;
extern STRING infile;

static C_iopath* inpath;

class s_info {
  public:
    s_info();
    //~s_info();
  protected:
    int _clusternu_events;
    int _clusternu_events_after_stop;
    int _clusternu_ved_info;
    int _clusternu_text_header;
    int _clusternu_file_header;
    int _clusternu_text_trailer;
    int _clusternu_file_trailer;
    int _clusternu_no_more_data;
    int _header;
    unsigned int _ev_next_log;
    static const int _ev_log_num;
    int _file_nr;
    int* ved_ids;
    int ved_num;
    struct ev {
        int _ev_found;
        unsigned int _ev_first, _ev_last;
    } *evs;
  public:
    void new_file();
    void print(ostream&, int fast);
    int clusternu_events() {return _clusternu_events;}
    int clusternu_events_after_stop() {return _clusternu_events_after_stop;}
    int clusternu_ved_info() {return _clusternu_ved_info;}
    int clusternu_text_header() {return _clusternu_text_header;}
    int clusternu_file_header() {return _clusternu_file_header;}
    int clusternu_text_trailer() {return _clusternu_text_trailer;}
    int clusternu_file_trailer() {return _clusternu_file_trailer;}
    int clusternu_no_more_data() {return _clusternu_no_more_data;}
    int headernum() {return _clusternu_text_header+_clusternu_file_header;}
    int header() {return _header;}
    void header_found();
    void ev_incr(int ved_id, unsigned int first, unsigned int last);
    void set_vednum(int vednum);
    void set_ved_id(int ved_idx, int ved_id);
    int ev_found();
    int ev_first();
    int ev_last();
    int file_nr() {return _file_nr;}
    void incr_events();
    void incr_ved_info() {_clusternu_ved_info++;}
    void incr_text() {
        if (_header) _clusternu_text_header++; else _clusternu_text_trailer++;
    }
    void incr_file() {
        if (_header) _clusternu_file_header++; else _clusternu_file_trailer++;
    }
    void incr_no_more_data() {_clusternu_no_more_data++;}
};

static s_info info;
const int s_info::_ev_log_num=100000;

/*****************************************************************************/
s_info::s_info()
:_file_nr(0), ved_ids(0), ved_num(0), evs(0)
{}
/*****************************************************************************/
void s_info::new_file()
{
_clusternu_events=0;
_clusternu_events_after_stop=0;
_clusternu_ved_info=0;
_clusternu_text_header=0;
_clusternu_file_header=0;
_clusternu_text_trailer=0;
_clusternu_file_trailer=0;
_clusternu_no_more_data=0;
delete[] ved_ids;
ved_ids=0;
delete[] evs;
evs=0;
ved_num=0;
_header=1;
_ev_next_log=_ev_log_num;
_file_nr++;
}
/*****************************************************************************/
void s_info::incr_events()
{
_clusternu_events++;
if (clusternu_no_more_data()>0)
  {
  _clusternu_events_after_stop++;
  if (clusternu_events_after_stop()==1)
      cout<<"  events after stop_cluster; event records="
          <<clusternu_events()
          <<"; event "<<ev_last()
          <<endl;
  }
}
/*****************************************************************************/
void s_info::print(ostream& os, int fast)
{
if (!fast)
  {
  os<<" event records       : "<<clusternu_events()<<endl;
  if (clusternu_events_after_stop()>0)
    os<<" event records after stop: "<<clusternu_events_after_stop()<<endl;
  }
os<<" header records/text      : "<<clusternu_text_header()<<endl;
os<<" header records/file      : "<<clusternu_file_header()<<endl;
os<<" trailer records/text     : "<<clusternu_text_trailer()<<endl;
os<<" trailer records/file     : "<<clusternu_file_trailer()<<endl;
if (ev_found())
  {
  os<<"first event     : "<<ev_first()<<endl;
  os<<"last  event     : "<<ev_last()<<endl;
  }
else
  os<<"no events found"<<endl;
}
/*****************************************************************************/
void s_info::header_found()
{
if (headernum()==0)
  {
  if (clusternu_file_header()==0)
    cout<<"  no header records"<<endl;
  else
    cout<<"  only file header records"<<endl;
  }
_header=0;
}
/*****************************************************************************/
void s_info::set_vednum(int vednum)
{
delete[] ved_ids;
ved_ids=new int[vednum];
delete[] evs;
evs=new ev[vednum];
ved_num=vednum;
for (int i=0; i<vednum; i++) evs[i]._ev_found=0;
}
/*****************************************************************************/
void s_info::set_ved_id(int ved_idx, int ved_id)
{
ved_ids[ved_idx]=ved_id;
}
/*****************************************************************************/
int bitcount(unsigned int v)
{
int n=0, i;
for (i=0; i<32; i++, v>>=1) if (v&1) n++;
return n;
}
/*****************************************************************************/
void s_info::ev_incr(int ved_id, unsigned int first, unsigned int last)
{
    int i;
    if (verbose>0) {
        if (bitcount(_clusternu_events)==1)
            cout<<"events: ved="<<ved_id<<" first="<<first
                    <<" last="<<last<<endl;
    }
    for (i=0; (i<ved_num) && (ved_ids[i]!=ved_id); i++);
    if (i==ved_num) {
        cout<<"ved_id "<<ved_id<<" not found"<<endl;
        return;
    }
    if (evs[i]._ev_found) {
        if (first<=evs[i]._ev_last) {
            cout<<"jump from "<<evs[i]._ev_last+1<<" to "<<first
                    <<" for ved "<<ved_id<<endl;
            evs[i]._ev_first=first;
            _ev_next_log=(first/_ev_log_num+1)*_ev_log_num;
        }
    } else {
        evs[i]._ev_first=first;
        evs[i]._ev_found=1;
    }
    evs[i]._ev_last=last;
    if (verbose>0) {
        if (evs[i]._ev_last>=_ev_next_log) {
            cout<<"  "<<evs[i]._ev_last<<" events so far"<<endl;
            do {
                _ev_next_log+=_ev_log_num;
            } while (_ev_next_log<=evs[i]._ev_last);
        }
    }
}
/*****************************************************************************/
int s_info::ev_first()
{
int i;
unsigned int n=0xffffffff;
for (i=0; i<ved_num; i++)
  {
  if ((evs[i]._ev_found)&&(evs[i]._ev_first<n)) n=evs[i]._ev_first;
  }
return n;
}
/*****************************************************************************/
int s_info::ev_last()
{
int i;
unsigned int n=0;
for (i=0; i<ved_num; i++)
  {
  if ((evs[i]._ev_found)&&(evs[i]._ev_last>n)) n=evs[i]._ev_last;
  }
return n;
}
/*****************************************************************************/
int s_info::ev_found()
{
int i;
for (i=0; i<ved_num; i++)
  {
  if (evs[i]._ev_found) return 1;
  }
return 0;
}
/*****************************************************************************/
int complete_read(ems_u32* c, int requested, int existing, int endien)
{
if (debug) {cerr<<"complete_read("<<requested<<", "<<existing<<")"<<endl;}
int res, s;
s=(requested-existing)*4;
res=inpath->read(c+existing, s);
if (res<s)
  {
  if (res<0)
    cerr<<"complete_read: "<<strerror(errno)<<endl;
  else
    cerr<<"complete_read: short read("<<res<<" of "<<s<<" bytes)"<<endl;
  return -1;
  }
else
  {
  if (endien==0x78563412)
    {
    for (int i=existing; i<requested-existing; i++) c[i]=swap_int(c[i]);
    }
  return 0;
  }
}
/*****************************************************************************/
int complete_seek(int requested, int existing)
{
off_t res=inpath->seek((requested-existing)*4, SEEK_CUR);
if (res==(off_t)-1)
  {
  cerr<<"complete_seek: "<<strerror(errno)<<endl;
  return -1;
  }
else
  return 0;
}
/*****************************************************************************/
static int
check_text(ems_u32* c, int s, int& complete, int endien)
{
    ems_u32* ptr;
    int size=c[0]+1;
    char key[100]="";
    char run[100]="";
    char start[100]="";
    char stop[100]="";
    char start_coded[100]="";
    char stop_coded[100]="";

    if (verbose>0) {
        cout<<"Text Cluster: ";
    }
    if (!complete) {
        ptr=new ems_u32[size];
        if (complete_read(ptr, size, s, endien)!=0) return -1;
        for (int i=0; i<s; i++) ptr[i]=c[i];
    } else {
        ptr=c;
    }

    ems_u32* p=ptr+3;
    int n_opt=*p++;  // options<>
    if (debug) cout<<n_opt<<" options"<<endl;
    p+=n_opt;
    ems_u32 flags=*p++;
    int fragment_id=*p++;
    if (debug) cout<<"flags="<<flags<<", fragment_id="<<fragment_id<<endl;
    int n_str=*p++; // Anzahl der Strings
    if (debug) cout<<n_str<<" strings"<<endl;
    for (int ni=0; ni<n_str; ni++) {
        char* s;
        p=xdrstrdup(s, p);
        if (strncmp("Key: ", s, 5)==0) {
            strncpy(key, s+5, 100);
        } else if (strncmp("Run: ", s, 5)==0) {
            strncpy(run, s+5, 100);
        } else if (strncmp("Start Time: ", s, 12)==0) {
            strncpy(start, s+12, 100);
        } else if (strncmp("Start Time_coded: ", s, 18)==0) {
            strncpy(start_coded, s+18, 100);
        } else if (strncmp("Stop Time: ", s, 11)==0) {
            strncpy(stop, s+11, 100);
        } else if (strncmp("Stop Time_coded: ", s, 17)==0) {
            strncpy(stop_coded, s+17, 100);
        }
        if (verbose>1) {
            if (oldtext) {
                cout<<s<<" ";
            } else if (key[0]==0) {
                cout<<"vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"<<endl;
                cout<<s<<endl;
                cout<<"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"<<endl;
            } else {
                cout<<"  "<<s<<endl;
            }
        }
        delete[] s;
    }
    if (verbose>0) cout<<key<<endl;
    if (strcmp(key, "superheader")==0) {
        cout<<"Superheader:"<<endl;
        if (run[0]!=0)   cout<<"-->  Run Nr.   :"<<run<<endl;
        if (start[0]!=0) cout<<"-->  Start Time:"<<start<<endl;
        if (stop[0]!=0)  cout<<"-->  Stop Time :"<<stop<<endl;
        if (start[0]!=0) cout<<"-->  Start Time_coded:"<<start_coded<<endl;
        if (stop[0]!=0)  cout<<"-->  Stop Time_coded :"<<stop_coded<<endl;
    }
    if (verbose>1) {
        if (oldtext) cout<<endl;
        cout<<"-----------------------------------------------"<<endl;
    }
    if (!complete) {
        delete[] ptr;
        complete=1;
    }
    return 0;
}
/*****************************************************************************/
ostream& tstring(int time, ostream& os)
{
if (time<=0)
    {
    os<<"never";
    return os;
    }
time_t zeit=(time_t)time;
struct tm* tms;
tms=localtime(&zeit);
char str[1024];
strftime(str, 1024, "%Y-%m-%d %H:%M:%S", tms);
os<<str;
return os;
}
/*****************************************************************************/
static int check_file(ems_u32* c, int s, int& complete, int endien)
{
ems_u32* ptr;
int size=c[0]+1;

if (verbose>0)
  {
  cout<<"File Cluster: ";
  }
if (!complete)
  {
  ptr=new ems_u32[size];
  if (complete_read(ptr, size, s, endien)!=0) return -1;
  for (int i=0; i<s; i++) ptr[i]=c[i];
  }
else
  ptr=c;

ems_u32* p=ptr+3;
int n_opt=*p++;  // options<>
if (debug) cout<<n_opt<<" options"<<endl;
p+=n_opt;
int flags=*p++;
int fragment_id=*p++;
if (debug) cout<<"flags="<<flags<<", fragment_id="<<fragment_id<<endl;
char* name;
p=xdrstrdup(name, p);
int ctime, mtime, perm, fsize;
ctime=*p++;
mtime=*p++;
perm=*p++;
fsize=*p++;

if (verbose>0)
  {
  cout<<"\""<<name<<"\""<<endl;
  if (verbose>1)
    {
    if (fragment_id==0)
      {
      cout<<"  ctime: "; tstring(ctime, cout); cout<<endl;
      cout<<"  mtime: "; tstring(mtime, cout); cout<<endl;
      cout<<"  perm: 0"<<oct<<perm<<dec<<endl;
      }
    cout<<"  size: "<<fsize<<endl;
    if ((fragment_id&0x80000000)==0) cout<<"  last fragment"<<endl;
    }
  }
if (!complete)
  {
  delete[] ptr;
  complete=1;
  }
return 0;
}
/*****************************************************************************/
static int check_events(ems_u32* c, int s, int& complete, int endien)
{
    if (!complete) return 0;
    ems_u32* ptr;
    int size=c[0]+1;

    if (!complete) {
        ptr=new ems_u32[size];
        if (complete_read(ptr, size, s, endien)!=0) return -1;
        for (int i=0; i<s; i++) ptr[i]=c[i];
    } else {
        ptr=c;
    }

    ems_u32* p=ptr+3;
    int n_opt=*p++;  // options<>
    if (debug) cout<<n_opt<<" options"<<endl;
    p+=n_opt;
    p++; /* int flags=*p++; */
    int ved_id=*p++;
    p++; /* int fragment_id=*p++; */
    int num_events=*p++;
    if (num_events==0) {
        cout<<"event cluster empty"<<endl;
        return 0;
    }
    p++; // size of first event
    int first_event=*p;
    int last_event=first_event+num_events-1;
    info.ev_incr(ved_id, first_event, last_event);
    return 0;
}
/*****************************************************************************/
static int check_ved_info(ems_u32* c, int s, int& complete, int endien)
{
    if (!complete) return 0;
    ems_u32* ptr;
    int size=c[0]+1;

    if (verbose>0) {
        cout<<"VED Info Cluster: "<<endl;
    }
    if (!complete) {
        ptr=new ems_u32[size];
        if (complete_read(ptr, size, s, endien)!=0) return -1;
        for (int i=0; i<s; i++) ptr[i]=c[i];
    } else {
        ptr=c;
    }

    ems_u32* p=ptr+3;
    int n_opt=*p++;  // options<>
    if (debug) cout<<n_opt<<" options"<<endl;
    p+=n_opt;

    p++; /* int version=*p++; not used here */
    int num_info=*p++;
    info.set_vednum(num_info);
    ems_u32* saved_p=p;
    for (int i=0; i<num_info; i++) {
        int VEDid=*p++;
        int num_is=*p++;
        p+=num_is;
        info.set_ved_id(i, VEDid);
    }
    p=saved_p;
    if (verbose>1) {
        for (int i=0; i<num_info; i++) {
            int VEDid=*p++;
            cout<<"ISs of VED "<<VEDid<<":";
            int num_is=*p++;
            for (int j=0; j<num_is; j++) {
                int IS_ID=*p++;
                cout<<" "<<IS_ID;
            }
            cout<<endl;
        }
    }
    return 0;
}
/*****************************************************************************/
static int check_cluster(ems_u32* c, int si, int& complete, int endien)
{
int res=0;
switch (c[2]) // type
  {
  case clusterty_events:
    if (debug) cout<<"clustertype events"<<endl;
    if ((info.clusternu_events()==0) && (info.clusternu_ved_info()==0))
      {
      cout<<"  no vedinfo record"<<endl;
      }
    if (info.header()) info.header_found();
    res=check_events(c, si, complete, endien);
    info.incr_events();
    break;
  case clusterty_ved_info:
    if (debug) cout<<"clustertype ved_info"<<endl;
    //if (info.header) header_found();
    info.incr_ved_info();
    res=check_ved_info(c, si, complete, endien);
    break;
  case clusterty_text:
    if (debug) cout<<"clustertype text"<<endl;
    info.incr_text();
    if (info.clusternu_no_more_data()>0)
      {
      cout<<"  text after stop-cluster"<<endl;
      }
    res=check_text(c, si, complete, endien);
    break;
  case clusterty_file:
    if (debug) cout<<"clustertype file"<<endl;
    info.incr_file();
    if (info.clusternu_no_more_data()>0)
      {
      cout<<"  file after stop-cluster"<<endl;
      }
    res=check_file(c, si, complete, endien);
    break;
  case clusterty_no_more_data:
    if (verbose>0) cout<<"clusterty_no_more_data"<<endl;
    info.incr_no_more_data();
    break;
  default:
    cout<<"unknown clustertype "<<c[2]<<endl;
  }
return res;
}
/*****************************************************************************/
static int check_record(ems_u32* c, clustertypes& typ)
{
    int res, complete;
    try {
        res=inpath->read(c, maxrec);
        if (debug )cout<<"size="<<res<<endl;
    } catch (C_status_error* e) {
        switch (e->code()) {
        case C_status_error::err_filemark:
            res=0;
            break;
        case C_status_error::err_end_of_file:
        case C_status_error::err_none:
        default:
            cout<<"error: "<<(*e)<<endl;
            res=-1;
        }
        delete e;
        if (res<0) return -1;
    } catch (C_error* e) {
        cout<<"!! error: "<<(*e)<<endl;
        delete e;
        return -1;
    }
    if (res==0) { /* impossible; C_iopath::read throws C_status_error */
        if (verbose>0) cout<<"Filemark"<<endl;
    } else if (res<0) {
        cerr<<"read tape: "<<strerror(errno)<<endl;
    } else {
        int endien=c[1];
        switch (endien) // endientest
        {
        case 0x12345678: // host byteorder
            if (debug) cout<<"host byteorder"<<endl;
            break;
        case 0x78563412: // verkehrt
            {
                if (debug) cout<<"not host byteorder"<<endl;
                for (int i=0; i<res/4; i++) c[i]=swap_int(c[i]);
            }
          break;
        default:
            cerr<<"unknown endien: "<<hex<<c[1]<<dec<<endl;
            return -1;
        }
        int size=c[0]+1;
        if (size*4!=res) {
            cout<<"warning: clustersize="<<size<<" but got "<<res<<" bytes"<<endl;
        }
        typ=clustertypes(c[2]);
        complete=1;
        check_cluster(c, size, complete, endien);
    }
    return res;
}
/*****************************************************************************/
static int scan_tapefile_slow()
{
    ems_u32* c=new ems_u32[maxirec];
    int res;
    clustertypes typ;

    info.new_file();
    cout<<"-----------------------------------------------------------------------"
          "-------"<<endl
        <<"File "<<info.file_nr()<<endl;

    do { // foreach cluster
        res=check_record(c, typ);
    }
    while (res>0);

    delete c;
    info.print(cout, 0);
    return res;
}
/*****************************************************************************/
static int scan_tapefile_fast()
{
ems_u32* c=new ems_u32[maxirec];
int res;
clustertypes typ;
struct mtop mt_com;

info.new_file();
cout<<"-----------------------------------------------------------------------"
      "-------"<<endl
    <<"File "<<info.file_nr()<<endl;

// read header records
cout<<"header:"<<endl;
do
  {
  res=check_record(c, typ);
  }
while ((res>0) && (typ!=clusterty_events));
if (res<0) goto fehler;
if (res==0)
  {
  if (verbose>0) cout<<"no event records found"<<endl;
  goto fehler;
  }
// seek over next filemark
mt_com.mt_op=MTFSF;
mt_com.mt_count=1;
res=ioctl(inpath->path(), MTIOCTOP, &mt_com);
if (res<0)
  {
  cout<<"wind tape forward: "<<strerror(errno)<<endl;
  goto fehler;
  }
// seek backward
mt_com.mt_op=MTBSF;
mt_com.mt_count=1;
res=ioctl(inpath->path(), MTIOCTOP, &mt_com);
if (res<0)
  {
  cout<<"wind tape backward: "<<strerror(errno)<<endl;
  goto fehler;
  }
mt_com.mt_op=MTBSR;
mt_com.mt_count=info.headernum()+2;
res=ioctl(inpath->path(), MTIOCTOP, &mt_com);
if (res<0)
  {
  cout<<"wind "<<info.headernum()+2<<" records backward: "<<strerror(errno)<<endl;
  goto fehler;
  }

// read trailer records
cout<<"trailer:"<<endl;
do
  {
  res=check_record(c, typ);
  }
while (res>0);

fehler:
delete c;
info.print(cout, 1);
return res;
}
/*****************************************************************************/
static int scan_tapefile_for_runnr()
{
    ems_u32* c=new ems_u32[maxirec];
    int records_read=0, run_nr_found=0, res;

    do {
        clustertypes typ;

        try {
            res=inpath->read(c, maxrec);
            records_read++;
        }
        catch (C_status_error* e) {
            cerr<<"error: "<<(*e)<<endl;
            res=-1;
            break;
        }
        if (res<0) {
            cerr<<"read tape: "<<strerror(errno)<<endl;
            break;
        }
        int endien=c[1];
        switch (endien) { // endientest
            case 0x12345678: // host byteorder
                if (debug) cout<<"host byteorder"<<endl;
                break;
            case 0x78563412: { // verkehrt
                if (debug) cout<<"not host byteorder"<<endl;
                for (int i=0; i<res/4; i++) c[i]=swap_int(c[i]);
                }
                break;
            default:
                cerr<<"unknown endien: "<<hex<<c[1]<<dec<<endl;
                return -1;
        }
        int size=c[0]+1;
        if (size*4!=res) {
            cerr<<"warning: clustersize="<<size<<" but got "<<res<<" bytes"<<endl;
        }
        typ=clustertypes(c[2]);
        switch (typ) {
            case clusterty_ved_info: /* ignored */ break;
            case clusterty_file:     /* ignored */ break;
            case clusterty_text: {
                char key[100]="";
                char run[100]="";
                char start[100]="";
                //int size=c[0]+1;
                ems_u32* p=c+3;
                int n_opt=*p++;  // options<>
                p+=n_opt;
                p++; /* int flags=*p++; not used */
                p++; /* int fragment_id=*p++; not used */
                int n_str=*p++; // Anzahl der Strings
                for (int ni=0; ni<n_str; ni++) {
                    char* s;
                    p=xdrstrdup(s, p);
                    if (strncmp("Key: ", s, 5)==0) {
                        strncpy(key, s+5, 100);
                    } else if (strncmp("Run: ", s, 5)==0) {
                        strncpy(run, s+5, 100);
                    } else if (strncmp("Start Time: ", s, 12)==0) {
                        strncpy(start, s+5, 100);
                    }
                  delete[] s;
                }
                if (strcmp(key, "superheader")==0) {
                    if (run[0]!=0)
                        cout<<run<<endl;
                    else
                        cout<<"none"<<endl;
                    //if (start[0]!=0) cout<<"-->  Start Time:"<<start<<endl;
                    run_nr_found=1;
                }
                }
                break;
            case clusterty_events:
            case clusterty_no_more_data:
                cerr<<"No Superheader found"<<endl;
                res=-1;
        }
    } while ((res>0) && !run_nr_found);
    if (res<=0) cout<<"error"<<endl;
    delete c;
    return records_read;
}
/*****************************************************************************/
static void scan_file_slow()
{
    ems_u32 head[2];
    ems_u32 *c=0, c_size=0;
    int complete;
    ssize_t res;

    info.new_file();
    do { // foreach cluster
        res=inpath->read(head, 2*4);
        if (res!=2*4) {
            if (res==0)
                cout<<"no more data"<<endl;
            else if (res<0)
                cerr<<"read file: "<<strerror(errno)<<endl;
            else
                cerr<<"read file: short read("<<res<<" of "<<2*4<<" bytes)"<<endl;
            goto fehler;
        }
        int endien=head[1];
        switch (endien) { // endientest
        case 0x12345678: // host byteorder
            if (debug) cout<<"host byteorder"<<endl;
            break;
        case 0x78563412: // verkehrt
            {
                if (debug) cout<<"not host byteorder"<<endl;
                for (int i=0; i<2; i++) head[i]=swap_int(head[i]);
            }
            break;
        default:
            cerr<<"unknown endien: "<<hex<<c[1]<<dec<<endl;
            delete[] c;
            goto fehler;
        }
        ems_u32 size=head[0]+1;
        if (c_size<size) {
            delete[] c;
            if ((c=new ems_u32[size])==0) {
                cerr<<"new ems_u32["<<size<<"]: "<<strerror(errno)<<endl;
                goto fehler;
            }
            c_size=size;
        }
        res=inpath->read(c+2, (size-2)*4);
        if (res!=(ssize_t)(size-2)*4) {
            if (res<0)
                cerr<<"read file: "<<strerror(errno)<<endl;
            else
                cerr<<"read file: short read("<<res<<" of "<<(size-2)*4<<" bytes)"<<endl;
            goto fehler;
        }
        if (endien==0x78563412) {
            for (ems_u32 i=2; i<size; i++) c[i]=swap_int(c[i]);
        }
        for (int i=0; i<2; i++) c[i]=head[i];
        complete=1;
        res=check_cluster(c, size, complete, endien);
        if (res!=0) goto fehler;
    }
    while (1);
    fehler:
    delete[] c;
    info.print(cout, 0);
}
/*****************************************************************************/
static void scan_file_fast()
{
ems_u32 head[3];
int res, complete;
info.new_file();
do // foreach cluster
  {
  res=inpath->read(head, 3*4);
  if (res!=3*4)
    {
    if (res==0)
      cout<<"no more data"<<endl;
    else if (res<0)
      cerr<<"read file: "<<strerror(errno)<<endl;
    else
      cerr<<"read file: short read("<<res<<" of "<<3*4<<" bytes)"<<endl;
    goto raus;
    }
  else
    {
    int endien=head[1];
    switch (endien) // endientest
      {
      case 0x12345678: // host byteorder
        if (debug) cout<<"host byteorder"<<endl;
        break;
      case 0x78563412: // verkehrt
        {
        if (debug) cout<<"not host byteorder"<<endl;
        for (int i=0; i<3; i++) head[i]=swap_int(head[i]);
        }
        break;
      default:
        cerr<<"unknown endien: "<<hex<<head[1]<<dec<<endl;
        goto raus;
      }
    int size=head[0]+1;
    complete=0;
    res=check_cluster(head, 3, complete, endien);
    if (res!=0) goto raus;
    if (!complete)
      {
      res=complete_seek(size, 3);
      if (res!=0) goto raus;
      }
    }
  }
while (1);
raus:
info.print(cout, 0);
}
/*****************************************************************************/
static int do_ask()
{
int dummy;
cerr << "press enter for next file! " << flush;
cin >> dummy;
return 1;
}
/*****************************************************************************/
static void scan_tape_fast()
{
int weiter;
do
  {
  int res;
  res=scan_tapefile_fast();
  weiter=(res==0) && !single && (!ask || do_ask());
  }
while (weiter);
}
/*****************************************************************************/
static void scan_tape_slow()
{
int weiter;
do
  {
  int res;
  res=scan_tapefile_slow();
  weiter=(res==0) && !single && (!ask || do_ask());
  }
while (weiter);
}
/*****************************************************************************/
static void scan_for_runnr()
{
    int res;
    res=scan_tapefile_for_runnr();
    if (res>0) {
        struct mtop mt_com;

        mt_com.mt_op=MTBSR;
        mt_com.mt_count=res;
        res=ioctl(inpath->path(), MTIOCTOP, &mt_com);
        if (res) perror("wind tape backward");
    }
}
/*****************************************************************************/
void scan_tape_with_cluster()
{
// open inputfile
try
  {
  inpath=new C_iopath(infile, C_iopath::iodir_input, ilit);
  if (inpath->typ()==C_iopath::iotype_none) {
    cerr<<"cannot open infile \""<<infile<<"\""<<endl;
    return;
  }
  if (verbose>0) cerr<<"infile: "<<(*inpath)<<endl;

  switch (inpath->typ()) {
    case C_iopath::iotype_file:
      if (fast)
        scan_file_fast();
      else
        scan_file_slow();
      break;
    case C_iopath::iotype_tape:
      if (runnr)
        scan_for_runnr();
      else {
        if (fast)
          scan_tape_fast();
        else
          scan_tape_slow();
      }
      break;
    case C_iopath::iotype_socket: // nobreak
    case C_iopath::iotype_fifo: // nobreak
    default: scan_file_slow(); break;
  }
  delete inpath;
  }
catch (C_error* e)
  {
  cerr<<(*e)<<endl;
  delete e;
  }
}
/*****************************************************************************/
/*****************************************************************************/
