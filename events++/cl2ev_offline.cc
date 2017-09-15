/*
 * cl2ev_offline.cc
 * 
 * created: 30.Jul.2002 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <versions.hxx>

#include <locale.h>
#include <readargs.hxx>
#include <errors.hxx>
#include "memory.hxx"
#include "swap_int.h"
#include "clusterformat.h"

VERSION("Aug 06 2002", __FILE__, __DATE__, __TIME__,
"$ZEL: cl2ev_offline.cc,v 1.13 2005/02/11 15:44:50 wuestner Exp $")
#define XVERSION

ems_u64 arr1875[21][64][4096];

struct iargs {
    STRING infile, outfile;
    int ilit, olit;
    int /*swap_out, */irecsize;
    int save_files;
    int tof;
    ems_u32 triggermask;
} iargs;

struct statist {
    size_t clusters;
    size_t events;
    size_t words;
} statist;

struct position {
    size_t cluster; // ==record
    size_t event; // event in cluster
    size_t subevent; // event in event
    size_t pos_cl; // word in cluster
    size_t pos_ev; // word in event
    size_t pos_sev; // word in subevent
} position;

class Event;

int numveds;
struct vedinfo {
    vedinfo():last_filled(0), last_evno(0) {}
    static int numveds;
    Event* last_filled;
    unsigned int last_evno; 
    C_outbuf ved;
} *vedinfo;

class Event {
    public:
        Event(unsigned int evno, unsigned int trigno);
        ~Event();
    private:
        unsigned int evno_;
        unsigned int trigno_;
        Event* next_;
        static unsigned int globvedmask_;
        unsigned int vedmask_;
        int completed_;
    public:
        static Event* root;
        static Event* top;

        static void globvedmaskbit(int bit) {globvedmask_|=(1<<bit);}
        int completed() const {return completed_;}
        unsigned int vedmask() const {return vedmask_;}
        void  vedmask_setbit(int bit);

        Event* next() const {return next_;}
        unsigned int evno() const {return evno_;}
        unsigned int trigno() const {return trigno_;}

        static int find_event(unsigned int evno, Event** event, Event* hint,
                int VED_ID);
        void add();
        void send_event(C_iopath& writer);
        static void send_events(C_iopath& writer);
//        static void scan_events();
        int last_ptr;
        C_outbuf buf;
        void add_sev(const unsigned int* data, unsigned int IS_ID, unsigned int size);
};

Event* Event::root=0;
Event* Event::top=0;
unsigned int Event::globvedmask_=0;

int events_found=0;
int events_new=0;
int events_skipped=0;

enum errorcodes {
    err_ok,
    fatal_event,
    fatal_cluster,
    fatal_file
};

char* errornames[]={
    "err_ok",
    "fatal_event",
    "fatal_cluster",
    "fatal_file"
};

void
dump(C_buf& buf)
{
    const int w=8;
    int i, j, n, num, idx;
    int lines, annotated;

    idx=buf.index();
    num=buf.size();
    if (num>idx+128) num=idx+128;
    lines=num/w;

    for (i=0, n=0; i<lines; i++, n+=w) {
        printf("%5d ", n);
        for (j=0; j<w; j++)  printf(" %08x", buf[n+j]);
        printf("\n");
        printf("      ");
        for (j=0; j<w; j++) {
            if ((unsigned int)buf[n+j]<100000000)
                printf(" %8u", buf[n+j]);
            else
                printf(" ........");
        }
        printf("\n");
        annotated=0;
        for (j=0; j<w; j++) if (buf.annotation(n+j)) annotated++;
        if (annotated) {
            printf("      ");
            for (j=0; j<w; j++) {
                printf(" %8c", buf.annotation(n+j)?buf.annotation(n+j):' ');
            }
            printf("\n");
        }
        if ((idx>=n) && (idx<n+w)) {
            printf("      ");
            for (j=0; j<w; j++) {
                printf(" %8s", n+j==idx?"########":"");
            }
            printf("\n");
        }
    }
    if (n<num) {
        printf("%5d ", n);
        for (j=n; j<num; j++)  printf(" %08x", buf[j]);
        printf("\n");
        printf("      ");
        for (j=n; j<num; j++) {
            if ((unsigned int)buf[j]<100000000)
                printf(" %8u", buf[j]);
            else
                printf(" ........");
        }
        printf("\n");
        annotated=0;
        for (j=n; j<num; j++) if (buf.annotation(j)) annotated++;
        if (annotated) {
            printf("      ");
            for (j=n; j<num; j++) {
                printf(" %8c", buf.annotation(j)?buf.annotation(j):' ');
            }
            printf("\n");
        }
        if ((idx>=n) && (idx<num)) {
            printf("      ");
            for (j=n; j<num; j++) {
                printf(" %8s", j==idx?"########":"");
            }
            printf("\n");
        }
    }
}

void
Event::add_sev(const unsigned int* data, unsigned int IS_ID, unsigned int size)
{
    buf[0]+=size+2;
    buf[3]++;
    buf<<IS_ID;
    buf<<size;
    buf<<put(data, size);
//     dump(buf);
//     cerr<<endl;
//     checkdead();
}

void
Event::vedmask_setbit(int bit)
{
    vedmask_|=(1<<bit);
    if (vedmask_==globvedmask_) completed_=1;
}

void
Event::add()
{
    if (!root)
        root=this;
    else {
        top->next_=this;
    }
    top=this;
}

Event::Event(unsigned int evno, unsigned int trigno)
:evno_(evno), trigno_(trigno), next_(0), last_ptr(0), vedmask_(0), completed_(0)
{
    buf<<3; /* size */
    buf<<evno;
    buf<<trigno;
    buf<<0; /* no. of subevents */
}

Event::~Event() {
    /*
       this is either ==root
       or this is not member of the list
    */
    if (this==root) root=this->next();
    if (this==top) top=0;
    if (last_ptr) {
        for (int i=0; i<numveds; i++)
         if (vedinfo[i].last_filled==this) vedinfo[i].last_filled=0;
    }
}

int
Event::find_event(unsigned int evno, Event** event, Event* hint, int vedID)
{

    Event* ev;
    if (hint)
        ev=hint;
    else
        ev=root;

    while (ev && (ev->evno()<evno)) ev=ev->next();

    if (!ev) return 0;

    if (ev->evno()==evno) {
        *event=ev;
        return 1;
    }

    return -1; /* ev->evno()>evno */
}

int
check_sev_GetPCITrigData(const ems_u32* data, int size)
{
    if (size!=16) {
        cerr<<"  GetPCITrigData: size="<<size<<" (not "<<16<<")"<<endl;
        return -1;
    }
    return 0;
}

int
check_sev_RunNr(const ems_u32* data, int size)
{
    static int runnr=0;
    if (size!=1) {
        cerr<<"  RunNr: size="<<size<<" (not "<<2<<")"<<endl;
        return -1;
    }
    if (runnr==0) {
        runnr=data[0];
        cerr<<"RunNumber="<<runnr<<endl;
    } else {
        if (runnr!=data[0]) {
            cerr<<"RunNumber changed from "<<runnr<<" to "<<data[0]<<endl;
        return -1;
        }
    }
    return 0;
}

int
check_sev_Timestamp(const ems_u32* data, int size)
{
    if (size!=2) {
        cerr<<"  Timestamp: size="<<size<<" (not "<<2<<")"<<endl;
        return -1;
    }
    if ((data[0]<1025820000) || (data[1]>1027375200)) {
        struct tm* tm;
        char str[1024];
        tm=localtime((const time_t*)data);
        strftime(str, 1024, "%c", tm);
        cerr<<"  Timestamp ("<<str<<") outside of beamtime"<<endl;
        return -1;
    }
    return 0;
}

int
check_sev_Latch_C219(const ems_u32* data, int size, int channels)
{
    if (size!=channels) {
        cerr<<"  Latch_C219: size="<<size<<" (not "<<channels<<")"<<endl;
        return -1;
    }
    return 0;
}

int
check_sev_sis3800ShadowUpdate(const ems_u32* data, int size,
    int boards, int channels)
{
    int words=1+boards+channels*2;
    if (size!=words) {
        cerr<<"  sis3800ShadowUpdate: size="<<size<<" (not "<<words<<")"<<endl;
        return -1;
    }
    return 0;
}

int
check_sev_Scaler2551_update_read(const ems_u32* data, int size,
    int boards, int channels)
{
    int words=boards+channels*2;
    if (size!=words) {
        cerr<<"  Scaler2551_update_read: size="<<size<<" (not "<<words<<")"<<endl;
        return -1;
    }
    return 0;
}

int
check_sev_Scaler4434_update_read(const ems_u32* data, int size,
    int boards, int channels)
{
    int words=boards+channels*2;
    if (size!=words) {
        cerr<<"  Scaler4434_update_read: size="<<size<<" (not "<<words<<")"<<endl;
        return -1;
    }
    return 0;
}

int
check_sev_LC_ADC_1881M(const ems_u32* data, int size, int is, unsigned int evno)
{
    if (size<1) {
        cerr<<"  LC_ADC_1881M: size==0"<<endl;
        return -1;
    }
    int words=*data++;
    if (words+1!=size) {
        cerr<<"  event "<<evno<<" IS "<<is<<endl;
        cerr<<"  LC_ADC_1881M: size="
            <<hex<<setiosflags(ios::showbase)
            <<size<<"; words="<<words
            <<dec<<resetiosflags(ios::showbase)<<endl;
        return -1;
    }
#ifdef ORIG
    return 0;
#endif

    while (words) {
        // first word has to be a header
        ems_u32 head=*data++; words--;
        if (head&0x07ff6000) {
            cerr<<"  event "<<evno<<" IS "<<is<<endl;
            cerr<<"  LC_ADC_1881: header contains invalid bits: "
                <<hex<<setiosflags(ios::showbase)
                <<head
                <<dec<<resetiosflags(ios::showbase)<<endl;
            return 0;
        }
        ems_u32 ga_h=(head>>27)&5;
        //ems_u32 par=(head>>15)&1;
        ems_u32 buf_h=(head>>7)&0x3f;
        ems_u32 count=(head&0x7f)-1; // number of following words
        ems_u32 xhead=head^(head>>16);
        xhead^=xhead>>8;
        xhead^=xhead>>4;
        xhead^=xhead>>2;
        xhead^=xhead>>1;
        if (xhead&1) {
            cerr<<"  event "<<evno<<" IS "<<is<<endl;
            cerr<<"  LC_ADC_1881: header has wrong parity: "
                <<hex<<setiosflags(ios::showbase)
                <<head
                <<dec<<resetiosflags(ios::showbase)<<endl;
            return 0;
        }
        if (count>words) {
            cerr<<"  event "<<evno<<" IS "<<is<<endl;
            cerr<<"  LC_ADC_1881: wrong count ("<<count<<") in header: "
                <<hex<<setiosflags(ios::showbase)
                <<head
                <<dec<<resetiosflags(ios::showbase)<<endl;
            return 0;
        }

        for (int i=0; i<count; i++) {
            ems_u32 val=*data++; words--;
            if (val&0x0001c000) {
                cerr<<"  event "<<evno<<" IS "<<is<<endl;
                cerr<<"  LC_ADC_1881: value contains invalid bits: "
                    <<hex<<setiosflags(ios::showbase)
                    <<val
                    <<dec<<resetiosflags(ios::showbase)<<endl;
                return 0;
            }
            ems_u32 ga=(val>>27)&5;
            //ems_u32 par=(val>>26)&1;
            ems_u32 buf=(val>>24)&3;
            ems_u32 chan=(val>>17)&0x3f;
            ems_u32 charge=val&0x3fff;
            ems_u32 xval=val^(val>>16);
            xval^=xval>>8;
            xval^=xval>>4;
            xval^=xval>>2;
            xval^=xval>>1;
            if (xval&1) {
                cerr<<"  event "<<evno<<" IS "<<is<<endl;
                cerr<<"  LC_ADC_1881: value has wrong parity: "
                    <<hex<<setiosflags(ios::showbase)
                    <<val
                    <<dec<<resetiosflags(ios::showbase)<<endl;
                return 0;
            }
            if (ga!=ga_h) {
                cerr<<"  event "<<evno<<" IS "<<is<<endl;
                cerr<<"  LC_ADC_1881: value has wrong geographical address: "
                    <<ga_h<<"-->"<<ga<<endl;
                return 0;
            }
            if ((buf_h&3)!=buf) {
                cerr<<"  event "<<evno<<" IS "<<is<<endl;
                cerr<<"  LC_ADC_1881: value has wrong buffer number: "
                    <<buf_h<<"-->"<<buf<<endl;
                return 0;
            }
        }
    }
    return 0;
}

int
check_sev_LC_TDC_1877(const ems_u32* data, int size, int is, unsigned int evno)
{
    if (size<1) {
        cerr<<"  LC_TDC_1877: size==0"<<endl;
        return -1;
    }
    int words=*data++;
    if (words+1!=size) {
        cerr<<"  LC_TDC_1877: size="
            <<hex<<setiosflags(ios::showbase)
            <<size<<"; words="<<words
            <<dec<<resetiosflags(ios::showbase)<<endl;
        return -1;
    }
#ifdef ORIG
    return 0;
#endif

    while (words) {
        // first word has to be a header
        ems_u32 head=*data++; words--;
        if (head&0x07ff4000) {
            cerr<<"  LC_TDC_1877: header contains invalid bits: "
                <<hex<<setiosflags(ios::showbase)
                <<head
                <<dec<<resetiosflags(ios::showbase)<<endl;
            return -1;
        }
        ems_u32 ga_h=(head>>27)&5;
        //ems_u32 par=(head>>15)&1;
        ems_u32 buf_h=(head>>11)&7;
        ems_u32 count=(head&0x7ff)-1; // number of following words
        ems_u32 xhead=head^(head>>16);
        xhead^=xhead>>8;
        xhead^=xhead>>4;
        xhead^=xhead>>2;
        xhead^=xhead>>1;
        if (xhead&1) {
            cerr<<"  LC_TDC_1877: header has wrong parity: "
                <<hex<<setiosflags(ios::showbase)
                <<head
                <<dec<<resetiosflags(ios::showbase)<<endl;
            return -1;
        }
        if (count>words) {
            cerr<<"  LC_TDC_1877: wrong count ("<<count<<") in header: "
                <<hex<<setiosflags(ios::showbase)
                <<head
                <<dec<<resetiosflags(ios::showbase)<<endl;
            return -1;
        }

        for (int i=0; i<count; i++) {
            ems_u32 val=*data++; words--;
            ems_u32 ga=(val>>27)&5;
            //ems_u32 par=(val>>26)&1;
            ems_u32 buf=(val>>24)&3;
            ems_u32 chan=(val>>17)&0x3f;
            ems_u32 edge=(val>>16)&1;
            ems_u32 time=val&0xffff;
            ems_u32 xval=val^(val>>16);
            xval^=xval>>8;
            xval^=xval>>4;
            xval^=xval>>2;
            xval^=xval>>1;
            if (xval&1) {
                cerr<<"  LC_TDC_1877: value has wrong parity: "
                    <<hex<<setiosflags(ios::showbase)
                    <<val
                    <<dec<<resetiosflags(ios::showbase)<<endl;
                return -1;
            }
            if (ga!=ga_h) {
                cerr<<"  LC_TDC_1877: value has wrong geographical address: "
                    <<ga_h<<"-->"<<ga<<endl;
                return -1;
            }
            if ((buf_h&3)!=buf) {
                cerr<<"  LC_TDC_1877: value has wrong buffer number: "
                    <<buf_h<<"-->"<<buf<<endl;
                return -1;
            }
        }
    }
    return 0;
}

int
check_sev_PH10CX(const ems_u32* data, int size, int is,
        unsigned int evno)
{
    if (size<1) {
        cerr<<"  event "<<evno<<" IS "<<is<<endl;
        cerr<<"  PH10CX     : size==0"<<endl;
        return -1;
    }
    int words=*data++;
    if (words+1!=size) {
        cerr<<"  event "<<evno<<" IS "<<is<<endl;
        cerr<<"  PH10CX     : size="
            <<hex<<setiosflags(ios::showbase)
            <<size
            <<"; words="
            <<words
            <<dec<<resetiosflags(ios::showbase)<<endl;
        return -1;
    }
#ifdef ORIG
    return 0;
#endif

    while (words) {
        // first word has to be a (super?) header
        ems_u32 head=*data++; words--;
        //if (head&0x00e0f800) {
        if (head&0x00e00000) {
            cerr<<"  event "<<evno<<" IS "<<is<<endl;
            cerr<<"  PH10CX: header contains invalid bits: "
                <<hex<<setiosflags(ios::showbase)
                <<head
                <<dec<<resetiosflags(ios::showbase)<<endl;
            return 0;
        }
        ems_u32 id=(head>>24)&0xff;
        ems_u32 ga=(head>>16)&0x1f;
        //ems_u32 count=(head&0x7ff)-1; // number of following words
        ems_u32 chan=((head>>10)&0x3f); // number of channels
        ems_u32 count=(chan+1)/2; // number of following words
        if (id!=(ga<<3)) {
            cerr<<"  event "<<evno<<" IS "<<is<<endl;
            cerr<<"  PH10CX     : header has wrong user id "
                <<hex<<setiosflags(ios::showbase)
                <<head
                <<dec<<resetiosflags(ios::showbase)<<endl;
            return 0;
        }
        if (count>words) {
            cerr<<"  event "<<evno<<" IS "<<is<<endl;
            cerr<<"  PH10CX     : wrong count ("<<count<<") in header: "
                <<hex<<setiosflags(ios::showbase)
                <<head
                <<dec<<resetiosflags(ios::showbase)<<endl;
            return 0;
        }

        for (int i=0; i<count; i++) {
            ems_u32 val=*data++; words--;
            if (val&0x80000000) {
                cerr<<"  event "<<evno<<" IS "<<is<<endl;
                cerr<<"  PH10CX     : value contains invalid bits: "
                    <<hex<<setiosflags(ios::showbase)
                    <<val
                    <<dec<<resetiosflags(ios::showbase)<<endl;
                return 0;
            }
        }
    }
    return 0;
}

int
check_sev_LC_TDC_1875A(const ems_u32* data, int size, int is,
        unsigned int evno)
{
static int counter=0;
// expected format:
//     no. of words
//     datawords
//     ...
// dataword:
//     <31..27> geographical address
//     <26..24> event
//     <23>     range
//     <22..16> channel
//     <11...0> data
//
    if (size<1) {
        cerr<<"  LC_TDC_1875A: size="<<size<<" (<1)"<<endl;
        return -2;
    }
    ems_u32 words=*data++;
    if (size!=words+1) {
        cerr<<"  LC_TDC_1875A: size="<<size<<" (!="<<words-1<<")"<<endl;
        return -2;
    }
    ems_u32 last_geo=0;
    ems_u32 last_channel=0;
    for (int i=words; i; i--) {
        ems_u32 d=*data++;
        ems_u32 geo=(d>>27)&0x1f;
        ems_u32 event=(d>>24)&7;
        ems_u32 r=(d>>23)&1;
        ems_u32 channel=(d>>16)&0x7f;
        ems_u32 data=d&0xfff;
        if (++counter<1000)
            cerr<<evno<<' '<<hex<<d<<dec<<' '<<geo<<' '<<event<<' '<<r<<' '<<channel<<' '<<data<<endl;
        if ((geo<13) || (geo>20) || (channel>64) || (data>4096)) {
            cerr<<"1875: geo="<<geo<<" last geo="<<last_geo<<endl;
            cerr<<"1875: "<<evno<<' '<<hex<<d<<dec<<' '<<is<<' '<<geo<<' '<<event<<' '<<r<<' '<<channel<<' '<<data<<endl;
            return 0;
        }
        if (geo<last_geo) {
            cerr<<"1875: geo="<<geo<<" last geo="<<last_geo<<endl;
            cerr<<"1875: "<<evno<<' '<<hex<<d<<dec<<' '<<is<<' '<<geo<<' '<<event<<' '<<r<<' '<<channel<<' '<<data<<endl;
            return 0;
        }
        if ((geo==last_geo) && (channel<=last_channel)) {
            cerr<<"1875: channel="<<channel<<" last channel="<<last_channel<<endl;
            cerr<<"1875: "<<evno<<' '<<hex<<d<<dec<<' '<<is<<' '<<geo<<' '<<event<<' '<<r<<' '<<channel<<' '<<data<<endl;
            return 0;
        }
        last_geo=geo; last_channel=channel;
        arr1875[geo][channel][data]++;
    }
    return 0;
}

void
Event::send_event(C_iopath& writer)
{
    writer<<buf;
}

void
Event::send_events(C_iopath& writer)
{

    while (root) {
        if (root->completed()) { /* event complete */
            root->send_event(writer);
            statist.events++;
        } else { /* event not (yet?) complete */
            for (int i=0; i<numveds; i++) {
                if (!(root->vedmask()&(1<<i))) { /* ved i missing */
                    if (vedinfo[i].last_evno>=root->evno()) { /* no hope */
                        //cerr<<"send: root="<<root<<"/"<<root->evno()
                        //    <<" deleted"<<endl;
                        break;
                    }
                    return;
                }
            }
        }
        Event* help=root;
        root=root->next();
        delete help;
    }
}

// void
// Event::scan_events()
// {
//     Event* ev=root;
//     while (ev) {
//         if (ev->completed())
//             z.add(0, ev->evno());
//         else {
//             int mask=0;
//             for (int i=0; i<numveds; i++) {
//                 if (ev->evno()>vedinfo[i].last_evno) mask|=(1<<i);
//             }
//             if ((mask|ev->vedmask())==globvedmask_)
//                 z.add(1, ev->evno());
//             else
//                 z.add(2, ev->evno());
//         }
//         ev=ev->next();
//     }
//     z.flush();
// }

enum errorcodes
process_subevent(C_inbuf& buf, unsigned VED_ID, Event* event, ems_u32 trigger)
{
    unsigned IS_ID, size;

    if (buf.remaining()<2) {
        cerr<<"event "<<event->evno()<<" subevent idx "<<position.subevent
            <<": no 'size' available"<<endl;
        return fatal_cluster;
    }

    buf>>IS_ID>>size;

    if (size>buf.remaining()) {
        cerr<<"event "<<event->evno()<<" subeventID "<<IS_ID<<": size="<<size
            <<" but only "<<buf.remaining()<<" words available"<<endl;
        return fatal_cluster;
    }

    if (iargs.tof) {
        int r;
        switch (IS_ID) {
            case 0:
                r=check_sev_RunNr(buf.idx_ubuf(), size);
                break;
            case 1:
                r=check_sev_Timestamp(buf.idx_ubuf(), size);
                break;
            case 3:
                r=check_sev_sis3800ShadowUpdate(buf.idx_ubuf(), size, 6, 6*32);
                break;
            case 4:
                r=check_sev_Scaler2551_update_read(buf.idx_ubuf(), size, 2, 24);
                break;
            case 5:
                switch (trigger) {
                    case 1: /* nobreak */
                    case 9:
                        r=check_sev_Scaler4434_update_read(
                            buf.idx_ubuf(), size, 1, 1);
                        break;
                    case 8:
                        r=check_sev_Scaler4434_update_read(
                            buf.idx_ubuf(), size, 3, 96);
                        break;
                }
                break;
            case 10:
                r=check_sev_Latch_C219(buf.idx_ubuf(), size, 1);
                break;
            case 20: /* t03 */
            case 22: /* t04 */
            case 26: /* t06 */
            case 32: /* t02 */
                r=check_sev_LC_ADC_1881M(buf.idx_ubuf(), size,
                        IS_ID, event->evno());
                break;
            case 21: /* t03 */
            case 23: /* t04 */
            case 25: /* t06 */
            case 33: /* t02 */
                r=check_sev_PH10CX(buf.idx_ubuf(), size,
                        IS_ID, event->evno());
                break;
            case 24: /* t04 */
            case 27: /* t03 */
            case 29: /* t06 */
            case 34: /* t02 */
                r=check_sev_LC_TDC_1877(buf.idx_ubuf(), size,
                        IS_ID, event->evno());
                break;
            case 28: /* t03 */
            case 30: /* t04 */
            case 31: /* t06 */
            case 35: /* t02 */
                r=check_sev_LC_TDC_1875A(buf.idx_ubuf(), size,
                        IS_ID, event->evno());
                break;
            case 1000:
                r=check_sev_GetPCITrigData(buf.idx_ubuf(), size);
                break;
            default:
                cerr<<"cluster "<<position.cluster
                    <<" event "<<event->evno()<<" subevent idx "
                    <<position.subevent
                    <<": unknown ID "
                    <<hex<<setiosflags(ios::showbase)
                    <<IS_ID
                    <<resetiosflags(ios::showbase)<<dec<<endl;
                return fatal_file;
                return fatal_event;
        }
        if (r) {
            cerr<<"cluster "<<position.cluster
                <<" event "<<position.event
                <<" subeventID "<<IS_ID
                <<": wrong content"<<endl;
            if (r<-1) {dump(buf); return fatal_file;}
            return fatal_event;
        }
    }
    event->add_sev(buf.idx_ubuf(), IS_ID, size);
    buf+=size;
    
    return err_ok;
}

enum errorcodes
process_event(C_inbuf& buf, unsigned VED_ID)
{
    unsigned int size, evno, trigno, subeventnum;
    unsigned idx;
    enum errorcodes err=err_ok;
    Event* event=0;
    int i, findres;

    if (buf.remaining()<1) {
        cerr<<"cluster "<<position.cluster<<" no event size available"<<endl;
        err=fatal_cluster;
        goto fehler;
    }
    buf>>size;
    if (size<3) {
        cerr<<"cluster "<<position.cluster<<" event size too small: "
            <<size<<endl;
        err=fatal_cluster;
        goto fehler;
    }
    if (size>buf.remaining()) {
        cerr<<"cluster "<<position.cluster<<" event size="
            <<size<<" but only "
            <<buf.remaining()<<" words available"<<endl;
        err=fatal_cluster;
        goto fehler;
    }
    idx=buf.index();
    buf>>evno>>trigno>>subeventnum;

    // Hier muesste die eventnummer getestet werden.

    if (trigno&~iargs.triggermask) {
        cerr<<"cluster "<<position.cluster
            <<" event "<<evno
            <<" invalid trigno: "
            <<hex<<setiosflags(ios::showbase)
            <<trigno
            <<dec<<resetiosflags(ios::showbase)<<endl;
        err=fatal_event;
        goto fehler;
    }

    findres=Event::find_event(evno, &event, vedinfo[VED_ID].last_filled, VED_ID);
    switch (findres) {
        case -1: // old event; skip it
            //cerr<<"skipped event "<<evno<<" for ved "<<VED_ID<<endl;
            events_skipped++;
            goto fehler;
        case  0: // not found; we have to create a new one;
            event=new Event(evno, trigno);
            if (!event) {
                cerr<<"cannot allocate new event"<<endl;
                err=fatal_file;
                goto fehler;
            }
            events_new++;
            break;
        case  1: // found
            events_found++;
            break;
    }

    if (event->trigno()!=trigno) {
        cerr<<"trigger mismatch in event "<<evno<<" VED "<<VED_ID<<": "
            <<hex<<setiosflags(ios::showbase)
            <<event->trigno()<<"-->"<<trigno
            <<dec<<resetiosflags(ios::showbase)<<endl;
        err=fatal_event;
        goto fehler;
    }
    if (event->vedmask()&(1<<VED_ID)) {
        cerr<<"VED "<<VED_ID<<" already in event "<<evno<<endl;
        err=fatal_event;
        goto fehler;
    }
    if (subeventnum>vedinfo[VED_ID].ved.size()) {
        cerr<<"cluster "<<position.cluster
            <<" event "<<evno
            <<" invalid number of subevents "<<subeventnum<<" for ved "<<VED_ID
            <<endl;
        err=fatal_event;
        goto fehler;
    }

    position.subevent=0;
    position.pos_ev=0;
    for (i=0; i<subeventnum; i++) {
        position.pos_sev=0;
        
        buf.annotate('s');
        position.pos_ev=buf.index();
        try {
            err=process_subevent(buf, VED_ID, event, trigno);
        } catch (C_error* e) {
            cerr<<endl<<"caught in process_event/2 "<<(*e)<<endl;
            delete e;
            //dump(buf);
            err=fatal_cluster;
            goto fehler;
        }
        if (err) goto fehler;
        position.subevent++;
    }
    event->vedmask_setbit(VED_ID);
    if (findres==0) event->add();

    if (vedinfo[VED_ID].last_filled) {
        vedinfo[VED_ID].last_filled->last_ptr--;
    }
    vedinfo[VED_ID].last_filled=event;
    event->last_ptr++;
    vedinfo[VED_ID].last_evno=event->evno();

    goto raus;

fehler:
//dump(buf);
    if ((findres==0) && event) delete event;
raus:
    if (err<fatal_cluster) {
        buf.index(idx+size);
    }
    return err;
}

enum errorcodes
process_events(C_inbuf& buf)
{
    unsigned int flags, VED_ID, fragment_id, eventnum, i;
    enum errorcodes err=err_ok;

    try {
        buf>>flags>>VED_ID>>fragment_id>>eventnum;
    } catch (C_error* e) {
        cerr<<endl<<"caught in process_events/1 "<<(*e)<<endl;
        delete e;
        err=fatal_file;
        goto fehler;
    }
    if (flags) {
        cerr<<"flags="<<flags<<endl;
        return fatal_cluster;
    }
    if (fragment_id) {
        cerr<<"fragment_id="<<fragment_id<<endl;
        return fatal_cluster;
    }
    if (VED_ID>=numveds) {
        cerr<<"invalid vedID "<<VED_ID<<endl;
        return fatal_cluster;
    }

    int first;
    try {
        first=buf[buf.index()+1];
    } catch (C_error* e) {
        cerr<<endl<<"caught in process_events/2 "<<(*e)<<endl;
        delete e;
        err=fatal_file;
        goto fehler;
    }

    {
    position.event=0;
    position.pos_cl=0;
    for (i=0; i<eventnum; i++) {
        position.subevent=0;
        position.pos_ev=0;
        position.pos_sev=0;

        buf.annotate('e');
        position.pos_cl=buf.index();
        try {
            err=process_event(buf, VED_ID);
        } catch (C_error* e) {
            cerr<<endl<<"caught in process_events/process_event: "<<(*e)<<endl;
            delete e;
            err=fatal_file;
            goto fehler;
        }
        if (err>=fatal_cluster)
                goto fehler;
        position.event++;
    }
    }

fehler:
    return err>=fatal_cluster?err:err_ok;
}

enum errorcodes
process_ved_info(C_inbuf& buf)
{
    unsigned int version;

    buf>>version>>numveds;
    if (version!=0x80000001) {
        cerr<<"wrong version of ved_info: "<<hex<<version<<dec<<endl;
        return fatal_file;
    }

    vedinfo=new struct vedinfo[numveds];

    for (int i=0; i<numveds; i++) {
        int ved_id, isnum;
        buf>>ved_id>>isnum;
        if (ved_id!=i) {
            cerr<<"wrong vedID: "<<ved_id<<" ("<<i<<")"<<endl;
            return fatal_file;
        }
        cerr<<"VED "<<ved_id<<":";
        Event::globvedmaskbit(ved_id);
        for (int j=0; j<isnum; j++) {
            int is_id;
            buf>>is_id;
            cerr<<" "<<is_id;
            vedinfo[i].ved<<is_id;
        }
        cerr<<endl;
    }
    /*for (i=0; i<vedinfo.vednum; i++) cerr<<vedinfo.ved[i]<<endl;*/
    cerr<<endl;
    return err_ok;
}

enum errorcodes
process_text(C_inbuf& buf)
{
    //int flags, fragment_id, count;
    //STRING line;
    //buf>>flags>>fragment_id>>count;
    //cerr<<"Text; "<<count<<" lines"<<endl;
    //buf>>line;
    //cerr<<"  "<<line<<endl;
    return err_ok;
}

enum errorcodes
process_file(C_inbuf& buf)
{
    //int flags, fragment_id, size;
    //STRING name;
    //buf>>flags>>fragment_id;
    //buf>>name;
    //buf+=3; // skip ctime mtime perm
    //buf>>size;
    //cerr<<"File "<<name<<" size="<<size<<" fragment "<<fragment_id<<endl;
    return err_ok;
}

enum errorcodes
process_options(C_inbuf& buf)
{
    unsigned int optsize, optflags, size;
    buf>>optsize;
    if (!optsize) return err_ok;
    buf>>optflags;
    size=0;
    if (optflags&!1) {
        cerr<<"wrong optflag "<<optflags<<endl;
        return fatal_cluster;
    }
    if (optflags&1) size+=2; // timestamp
    /* if (optflags&2) size+=?; // checksum */
    if (size+1!=optsize) {
        cerr<<"optsize mismatch:  optsize="<<optsize
            <<" optflags="<<optflags<<endl;
        return fatal_cluster;
    }
    if (optflags&1) {
        struct timeval tv;
        tv.tv_sec=buf.getint();
        tv.tv_usec=buf.getint();
        static int count=1;
        if (count) {
            count--;
            struct tm* tm;
            char str[1024];
            tm=localtime(&tv.tv_sec);
            strftime(str, 1024, "%c", tm);
            cerr<<str<<endl;
        }
    }
    return err_ok;
}

enum errorcodes
process_cluster(C_inbuf& buf)
{
    clustertypes clustertype;
    enum errorcodes err;

    buf+=1; /* skip endian */
    clustertype=(clustertypes)buf.getint();

    err=process_options(buf);
    if (err/*>=fatal_cluster*/) {
        dump(buf);
        return err;
    }
    try {
        switch (clustertype) {
            case clusterty_events:
                return process_events(buf);
            case clusterty_ved_info:
                return process_ved_info(buf);
            case clusterty_text:
                return process_text(buf);
            case clusterty_file:
                return process_file(buf);
//             case clusterty_wendy_setup:
//                 cerr<<"Garlic!!! Instantaneous please!!!"<<endl;
//                 return fatal_file;
            case clusterty_no_more_data:
                cerr<<"no_more_data cluster found"<<endl;
                return err_ok;
            default:
                cerr<<"unknown clustertype "<<clustertype<<endl;
                return fatal_cluster;
        }
    } catch (C_error* e) {
        cerr<<endl<<"caught in process_cluster; type="<<clustertype<<endl
            <<(*e)<<endl;
        delete e;
        return fatal_file;
    }
    /* never reached */
    cerr<<"Close Encounter of the Third Kind!"<<endl;
    return fatal_file;
}

enum errorcodes
do_convert(C_iopath& reader, C_iopath& writer)
{
    enum errorcodes err=err_ok;
    ems_u32* rc;

    rc=new ems_u32[iargs.irecsize];
    if (!rc) {
        cerr<<"cannot allocate "<<iargs.irecsize<<" words for raw cluster"<<endl;
        return fatal_file;
    }
    /* some init */
    statist.clusters=0;
    statist.events=0;
    statist.words=0;

    position.cluster=0;
    do {
        ssize_t res;
        ems_u32 size;

        position.event=0;
        position.subevent=0;
        position.pos_cl=0;
        position.pos_ev=0;
        position.pos_sev=0;

        if (reader.typ()==C_iopath::iotype_tape) {
            res=reader.read(rc, iargs.irecsize*sizeof(ems_u32));
            if (res<=0) {
                cerr<<endl<<"error reading tape"<<endl;
                goto raus;
            }
        } else {
            res=reader.read(rc, 2*sizeof(ems_u32));
            if (res!=2*sizeof(ems_u32)) {
                cerr<<endl<<"error(1) reading file"<<endl;
                cerr<<"expected "<<2*sizeof(ems_u32)<<" Bytes";
                if (res<0)
                    cerr<<": "<<strerror(errno)<<endl;
                else
                    cerr<<"; got only "<<res<<endl;
                goto raus;
            }
            switch (rc[1]) {
                case 0x12345678: size=rc[0]; break;
                case 0x78563412: size=swap_int(rc[0]); break;
                default:
                    cerr<<"unknown endian(1) "<<hex<<rc[1]<<dec<<endl;
                    goto raus;
            }
            if (size>=iargs.irecsize) {
                cerr<<endl<<"size of cluster="<<size<<endl;
                goto raus;
            }
            res=reader.read(rc+2, (size-1)*sizeof(ems_u32));
            if (res!=(size-1)*sizeof(ems_u32)) {
                cerr<<endl<<"error(2) reading file"<<endl;
                cerr<<"expected "<<(size-1)*sizeof(ems_u32)<<" Bytes";
                if (res<0)
                    cerr<<": "<<strerror(errno)<<endl;
                else
                    cerr<<"; got only "<<res<<endl;
                goto raus;
            }
            res+=2*sizeof(ems_u32);
        }
        if (res%sizeof(ems_u32)) {
            cerr<<endl<<"size not a multiple of "<<sizeof(ems_u32)<<endl;
            goto raus;
        }
        size=res/sizeof(ems_u32);
        switch (rc[1]) {
            case 0x12345678: break;
            case 0x78563412: {
                    for (ems_u32 i=0; i<size; i++) rc[i]=swap_int(rc[i]);
                }
                break;
            default:
                cerr<<"unknown endian(2) "<<hex<<rc[1]<<dec<<endl;
                goto raus;
        }
        if (rc[0]+1!=size) {
            cerr<<endl<<"cluster size mismatch: "<<rc[0]+1<<"<->"<<size<<endl;
            goto raus;
        }
        C_inbuf buf(rc);
        try {
            err=process_cluster(buf);
        } catch (C_error* e) {
            cerr<<endl<<"caught in do_convert/process_cluster: "<<(*e)<<endl;
            delete e;
            err=fatal_file;
            goto raus;
        }

        statist.clusters++;
        statist.words+=size;
        position.cluster++;
        Event::send_events(writer);
    } while (err<fatal_file);
    cerr<<"do_convert: err="<<err<<endl;

raus:
    cerr<<"loop finished"<<endl;
    cerr<<"clusters= "<<statist.clusters<<endl;
    cerr<<"events  = "<<statist.events<<endl;
    cerr<<"words   = "<<statist.words<<endl;
    cerr<<"events_found  ="<<events_found<<endl;
    cerr<<"events_new    ="<<events_new<<endl;
    cerr<<"events_skipped="<<events_skipped<<endl;
    delete[] rc;
    return err;
    /* aufraeumen? */
}

void
printheader(ostream& os)
{
    os<<"Converts EMS cluster format into EMS legacy format."<<endl<<endl;
}

void
printstreams(ostream& os)
{
    os<<endl;
    os<<"input and output may be: "<<endl;
    os<<"  - for stdin/stdout ;"<<endl;
    os<<"  <filename> for regular files or tape;"<<endl;
    os<<"      (if filename contains a colon use -iliterally or -oliterally)"
        <<endl;
    os<<"  :<port#> or <host>:<port#> for tcp sockets;"<<endl;
    os<<"  :<name> for unix sockets (passive);"<<endl;
    os<<"  @:<name> for unix sockets (active);"<<endl;
}

int
readargs(C_readargs& args)
{
    args.helpinset(3, printheader);
    args.helpinset(5, printstreams);

    args.addoption("infile", 0, "-", "input file", "input");
    args.addoption("outfile", 1, "-", "output file", "output");

    args.addoption("ilit", "iliterally", false,
        "use name of input file literally", "input file literally");
    args.addoption("olit", "oliterally", false,
        "use name of output file literally", "output file literally");

    args.addoption("irecsize", "irecsize", 65536,
            "maximum record size of input (tape only)", "irecsize");

    args.addoption("tof", "tof", true,
        "apply special checks for TOF July 2002 beamtime",
        "checks for TOF");

/*
    args.addoption("save_files", "save_files", false, "save haeder and trailer as files",
        "save");
*/

    if (args.interpret(1)!=0) return -1;
    iargs.ilit=args.getboolopt("ilit");
    iargs.olit=args.getboolopt("olit");
    iargs.infile=args.getstringopt("infile");
    iargs.outfile=args.getstringopt("outfile");

    iargs.irecsize=args.getintopt("irecsize")/sizeof(ems_u32);
    //iargs.save_files=args.getboolopt("save_files");
    iargs.tof=args.getboolopt("tof");

    if (iargs.tof)
        iargs.triggermask=9;
    else
        iargs.triggermask=0xffffffff;

    return 0;
}

void
init_1875_arrs()
{
    for (int i=0; i<21; i++)
        for (int j=0; j<64; j++)
            for (int k=0; k<4096; k++)
                arr1875[i][j][k]=0;
}

void
dump_1875_arrs()
{
    int i, j, k;
    for (i=0; i<21; i++) {
        for (j=0; j<64; j++) {
            ems_u64 sum=0;
            for (k=0; k<4096; k++) sum+=arr1875[i][j][k];
            if (sum) {
                char s[100];
                FILE* p;
                sprintf(s, "%02d_%02d", i, j);
                p=fopen(s, "w");
                if (!p) {
                    fprintf(stderr, "open %s: %s\n", s, strerror(errno));
                    return;
                }
                for (int k=0; k<4096; k++)
                    fprintf(p, "%4d  %10d\n", k, arr1875[i][j][k]);
            }
        }
    }
}

main(int argc, char* argv[])
{
    setlocale(LC_TIME, "");

    try {
        C_readargs args(argc, argv);
        if (readargs(args)) return 1;

        C_iopath reader(iargs.infile, C_iopath::iodir_input, iargs.ilit);
        if (reader.typ()==C_iopath::iotype_none) {
            return 1;
        }

        C_iopath writer(iargs.outfile, C_iopath::iodir_output, iargs.olit);
        if (writer.typ()==C_iopath::iotype_none) {
            reader.close();
            return 1;
        }
        init_1875_arrs();
        enum errorcodes err=do_convert(reader, writer);
        cerr<<"main: err="<<err<<endl;
    } catch (C_error* e) {
        cerr<<endl<<"caught in main: "<<(*e)<<endl;
        delete e;

        cerr<<"clusters: "<<statist.clusters<<endl;
        cerr<<"events  : "<<statist.events<<endl;
        cerr<<"words   : "<<statist.words<<endl;

        cerr<<"record    "<<position.cluster<<endl;
        cerr<<"event     "<<position.event<<endl;
        cerr<<"subevent  "<<position.subevent<<endl;
        //return 2;
    }
    dump_1875_arrs();
    return 0;
}
