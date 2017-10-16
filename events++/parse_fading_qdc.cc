/*
 * ems/events++/parse_fading_qdc.cc
 * 
 * created 2006-Oct-22 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <errno.h>
#include <math.h>
#include "compressed_io.hxx"
#include "cluster_data.hxx"
#include <versions.hxx>

VERSION("2006-Oct-22", __FILE__, __DATE__, __TIME__,
"$ZEL$")
#define XVERSION

#define swap_int(a) ( ((a) << 24) | \
                      (((a) << 8) & 0x00ff0000) | \
                      (((a) >> 8) & 0x0000ff00) | \
        ((unsigned int)(a) >>24) )

ems_event* event;

class spectrum {
    public:
        spectrum(int, int, int, const char*);
        void add(int);
        void print(void);
        void calculate(void);
        const char* name;
        int min, max, scale;
        long long int num, overflow, underflow;
        double sum, qsum;
        double average, sigma;
        int *hist;
};

// supply  ser.  ved    type  crate    IS
//               scaler                1
// ikpwc01    6  w09      F1  Crate21  2
// ikpwc04    9  w10     GPX  Crate13  3
// ikpwc05   10  w17    SQDC1 Crate4   4
// ikpwc06   11  w15    SQDC5 Crate12  9
// ikpwc07   12  w16    FQDC1 Crate11  5
// ikpwc08   13  w18    SQDC2 Crate3   6
// ikpwc09   14  w08    SQDC3 Crate1   7
// ikpwc20    3  w14    SQDC4 Crate2   8

// 6 crates * 16 modules * 16 channels
// histograms for base, q, Q, t
// new histogram after 100000 events (max. 10 times)

struct cs {
    cs(void)
        :base(0, 4096, 1, "b"),
        q(0, 4096, 128, "q"),
        Q(0, 4096, 128, "Q"),
        t(-2048, 2048, 4, "t")
        {
            //cerr<<"cs: this="<<(void*)this<<endl;
        }
    spectrum base;
    spectrum q;
    spectrum Q;
    spectrum t;
};

struct cs *sss[100];
unsigned int ns=0;
struct cs *ss=0;

int x37[256];
int ccc=0;
int cccc=0;
//---------------------------------------------------------------------------//
spectrum::spectrum(int min_, int max_, int scale_, const char* name_)
:name(name_), min(min_), max(max_), scale(scale_), 
num(0), overflow(0), underflow(0),
sum(0), qsum(0)
{
    hist=new int[max-min];
    //cerr<<name<<"["<<ccc<<", "<<cccc<<"]: "<<hist<<" this="<<(void*)this<<endl;
    cccc++;
}
//---------------------------------------------------------------------------//
void spectrum::add(int val)
{
    sum+=(double)val;
    qsum+=(double)val*(double)val;
    num++;
    val/=scale;
    if (val<min)
        underflow++;
    else if (val>max)
        overflow++;
    else
        hist[val-min]++;
}
//---------------------------------------------------------------------------//
void spectrum::calculate(void)
{
    double dn=num;
    double sum2=sum*sum;
    double nqsum=dn*qsum;
    average=sum/dn;
    sigma=sqrt((nqsum-sum2)/(dn*(dn-1)));
}
//---------------------------------------------------------------------------//
void spectrum::print(void)
{
    calculate();
    cout<<"n="<<num<<" av="<<average<<" sig="<<sigma<<endl;
}
//---------------------------------------------------------------------------//
int
xread(int p, char* b, int num)
{
    int da=0, rest=num, res;
    while (rest) {
        res=read(p, b+da, rest);
        if (res==0)
            return 0;
        if (res<0) {
            if (errno!=EINTR) {
                printf("xread: %s\n", strerror(errno));
                return -1;
            } else {
                res=0;
            }
        }
        rest-=res;
        da+=res;
    }
    return num;
}
//---------------------------------------------------------------------------//
/*
 * returns:
 * -1: fatal error
 *  0: no more data
 * >0: size of record (in words)
 */
int
read_record(int p, ems_u32** buf)
{
    ems_u32 head[2], *b;
    ems_u32 size;
    int wenden;

    switch (xread(p, (char*)head, 8)) {
    case -1: // error
        cerr<<"read header: "<<strerror(errno)<<endl;
        return -1;
    case 0:
        cerr<<"read: end of file"<<endl;
        return 0;
    default: // OK
        break;
    }

    //for (int i=0; i<2; i++) printf("head: 0x%08x\n", head[i]);
    switch (head[1]) {
    case 0x12345678:
        wenden=0;
        size=head[0];
        break;
    case 0x78563412:
        wenden=1;
        size=swap_int(head[0]);
        break;
    default:
        cerr<<"unknown endian "<<hex<<showbase<<setw(8)<<setfill('0')<<head[1]
            <<dec<<noshowbase<<setfill(' ')<<endl;
        return -1;
    }

    b=new ems_u32[size+1];
    if (!b) {
        cerr<<"new buf: "<<strerror(errno)<<endl;
        return -1;
    }

    b[0]=head[0];
    b[1]=head[1];
    switch (xread(p, (char*)(b+2), (size-1)*4)) {
    case -1: // error
        cerr<<"read body: "<<strerror(errno)<<endl;
        return -1;
    case 0:
        cerr<<"read body: unexpected end of file"<<endl;
        return -1;
    default: // OK
        break;
    }

    if (wenden) {
        for (int i=size; i>=0; i--) {
            ems_u32 v=b[i];
            b[i]=swap_int(v);
        }
    }
    //for (int i=0; i<6; i++) printf("0x%08x\n", b[i]);
    *buf=b;
    return size+1;
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
static int
subevent_begin(int id)
{
    for (int ch=0; ch<256; ch++) {
        x37[ch]=0;
    }

    return 0;
}
//---------------------------------------------------------------------------//
static int
subevent_end(void)
{
    return 0;
}
//---------------------------------------------------------------------------//
static int
parse_val(ems_u32 v)
{
    int mod=(v>>28)&0xf;
    int chan=(v>>22)&0xf;
    int ch=(mod<<4)|chan;
    struct cs &chs=ss[ch];

    int ident=(v>>16)&0x3f;
    if (ident==0) { // raw
        // ignored
    } else if (ident<0x30) { // Q
        int val=v&0xfffff;
        if (val&0x80000)
            val-=0x100000;
        if (x37[ch]==2)
            chs.q.add(val);
        else
            chs.Q.add(val);
    } else {
        switch (ident) {
        case 0x30: // window start
            break;
        case 0x31: // mean level
            break;
        case 0x32: // minimum time
            break;
        case 0x33: // minimum level
            break;
        case 0x34: // maximum time
            break;
        case 0x35: // maximum level
        case 0x3d: // maximum level overflow
            break;
        case 0x36: { // zero crossing
                int val=v&0x3fff;
                if (val&0x2000)
                    val-=0x4000;
                chs.t.add(val);
            }
            break;
        case 0x37: { // fixed window
                int val=v&0xfff;
                if (x37[ch]==0)
                    chs.base.add(val);
                x37[ch]++;
            }
            break;
        default: {
            //cerr<<"unknown ident "<<hex<<"0x"<<ident<<dec<<endl;
            }
        }
    }
    return 0;
}
//---------------------------------------------------------------------------//
static int
parse_subevent(const ems_subevent* sev)
{
    unsigned int* d=sev->data+4;
    if (sev->size<4) {
        cerr<<"sev size="<<sev->size<<endl;
        return 0;
    }

    if ((sev->data[1]&0xffff)!=(sev->size-1)*4) {
        cerr<<"sev size="<<sev->size<<" bytes="<<(sev->data[1]&0xffff)<<endl;
        return 0;
    }

    if (subevent_begin(sev->sev_id)<0)
        return -1;
    for (unsigned int i=0; i<sev->size-4; i++) {
        if (parse_val(*d++)<0)
            return -1;
    }
    if (subevent_end()<0)
        return -1;
    return 0;
}
//---------------------------------------------------------------------------//
static int
parse_event(const ems_event* event)
{
    static ems_u32 ev_no=0;
    if (event->event_nr!=ev_no+1) {
        cerr<<"jump from "<<ev_no<<" to "<<event->event_nr<<endl;
    }
    ev_no=event->event_nr;

    ems_subevent* sev=event->subevents;
    while (sev) {
        if (sev->sev_id==4) {
            if (parse_subevent(sev)<0)
                return -1;
        }
        sev=sev->next;
    }
        
    return 0;
}
//---------------------------------------------------------------------------//
int
main(int argc, char* argv[])
{
    string fname=argc>1?argv[1]:"-";
    compressed_input input(fname);
    ems_data ems_data;
    ems_cluster* cluster;
    ems_u32 *buf;
    int events=0;

    if (!input.good()) {
        cerr<<"cannot open input file "<<fname<<endl;
        return 1;
    }

    while (read_record(input.path(), &buf)>0) {
        cluster=new ems_cluster(buf);
        if (ems_data.parse_cluster(cluster)<0)
            goto raus;
        delete cluster;
        while (ems_data.get_event(&event)>0) {
            if (event->event_nr>=ns*10000) {
                if (ns>=10)
                    goto raus;
                ccc++;
                sss[ns]=new struct cs[256];
                cerr<<"sss["<<ns<<", "<<ccc<<"]="<<sss[ns]<<endl;;
                if (!sss[ns]) {
                    cerr<<"alloc struct cs; ns="<<ns<<": "<<strerror(errno)<<endl;
                    goto raus;
                }
                ss=sss[ns];
                ns++;
            }
            if (parse_event(event)<0)
                goto raus;
            delete event;
            events++;
            //if (events>=10000) goto raus;
            //if (N[7]>=1) goto raus;
        }
    }
raus:
    cerr<<events<<" events"<<endl;

    cout<<"spectrum \"base\""<<endl;
    for (int ch=0; ch<256; ch++) {
        cout<<"channel "<<ch<<endl;
        for (unsigned int n=0; n<ns; n++) {
            ss=sss[n];
            struct cs *chs=ss+ch;
            spectrum *sp=&chs->base;
            sp->calculate();
            cout<<setw(2)<<n
                <<"\t"<<sp->num
                <<"\t"<<sp->average
                <<"\t"<<sp->sigma
                <<endl;
        }
    }
    cout<<"spectrum \"q\""<<endl;
    for (int ch=0; ch<256; ch++) {
        cout<<"channel "<<ch<<endl;
        for (unsigned int n=0; n<ns; n++) {
            ss=sss[n];
            struct cs *chs=ss+ch;
            spectrum *sp=&chs->q;
            sp->calculate();
            cout<<setw(2)<<n
                <<"\t"<<sp->num
                <<"\t"<<sp->average
                <<"\t"<<sp->sigma
                <<endl;
        }
    }
    cout<<"spectrum \"Q\""<<endl;
    for (int ch=0; ch<256; ch++) {
        cout<<"channel "<<ch<<endl;
        for (unsigned int n=0; n<ns; n++) {
            ss=sss[n];
            struct cs *chs=ss+ch;
            spectrum *sp=&chs->Q;
            sp->calculate();
            cout<<setw(2)<<n
                <<"\t"<<sp->num
                <<"\t"<<sp->average
                <<"\t"<<sp->sigma
                <<endl;
        }
    }
    cout<<"spectrum \"t\""<<endl;
    for (int ch=0; ch<256; ch++) {
        cout<<"channel "<<ch<<endl;
        for (unsigned int n=0; n<ns; n++) {
            ss=sss[n];
            struct cs *chs=ss+ch;
            spectrum *sp=&chs->t;
            sp->calculate();
            cout<<setw(2)<<n
                <<"\t"<<sp->num
                <<"\t"<<sp->average
                <<"\t"<<sp->sigma
                <<endl;
        }
    }

    return 0;
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
