/*
 * ems/events++/parse_ftdc.cc
 * 
 * created 2006-Jul-27 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <errno.h>
#include <math.h>
#include "compressed_io.hxx"
#include "cluster_data.hxx"
#include <versions.hxx>

VERSION("2006-Oct-09", __FILE__, __DATE__, __TIME__,
"$ZEL: parse_f1raw.cc,v 1.3 2007/03/02 23:26:53 wuestner Exp $")
#define XVERSION

#define swap_int(a) ( ((a) << 24) | \
                      (((a) << 8) & 0x00ff0000) | \
                      (((a) >> 8) & 0x0000ff00) | \
        ((unsigned int)(a) >>24) )

ems_event* event;

unsigned long long int binsize_fs=10000000000;

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
#if 0
static unsigned long long int last_vl=0;
static unsigned long long int last_vh=0;

static void
do_diff(ems_u32 v0, ems_u32 v1, int vl, int vh)
{
    double vld=vl-last_vl;
    double vhd=vh-last_vh;


    //cout<<endl<<"last_vl="<<last_vl<<" last_vh="<<last_vh<<endl;
    //cout<<"vld="<<vld<<" vhd="<<vhd;
    vld*=.0001203; // 120.3 ps; vld is now in us
    vld+=vhd*6.4;
    cout<<hex<<setfill('0')<<setw(8)<<v0<<" "<<setw(8)<<v1<<setfill(' ')<<dec
        <<"  diff = "<<vld<<endl;

    last_vl=vl;
    last_vh=vh;
}
#endif
//---------------------------------------------------------------------------//
#if 0
static unsigned long long int last=0;
static unsigned long long int last_vh=0;
static unsigned long long int current_bin=0;
static int bin_content=0;

static void
do_diff(ems_u32 v0, ems_u32 v1,
        unsigned long long int vl, unsigned long long int vh)
{
    unsigned long long int v;

    if (vh<last_vh) {
        cout<<"vh="<<vh<<" last_vh="<<last_vh<<endl;
    }

    v=vh*6400000000; // fs
    v+=vl*120300;

    double dv=(double)v/1000000000.; //us
    unsigned long long int diff=v-last;
    double ddiff=(double)diff/1000000000.; //us

//     cout<<hex<<setfill('0')<<setw(8)<<v0<<" "<<setw(8)<<v1<<setfill(' ')<<dec
//         <<"  dv = "<<setw(7)<<dv<<" ddiff = "<<ddiff<<endl;

    unsigned long long int bin=(v/binsize_fs)*binsize_fs;
    while (bin>current_bin) {
        if (bin_content)
            cout<<current_bin/1000000000<<"\t"<<bin_content<<endl;
        current_bin+=binsize_fs;
        bin_content=0;
    }
    bin_content++;

    last=v;
    last_vh=vh;
}
#endif
//---------------------------------------------------------------------------//
static unsigned long long int last=0;
static unsigned long long int last_vh=0;
static unsigned long long int current_bin=0;
static int bin_content=0;
static ems_u32 last_v0=0, last_v1=0;

static void
do_diff(ems_u32 v0, ems_u32 v1,
        unsigned long long int vl, unsigned long long int vh)
{
    unsigned long long int v;

#if 0
    cout<<"                  vh="<<vh<<" last_vh="<<last_vh;
    if (vh<last_vh) {
        cout<<" XXX";
    }
    cout<<endl;
    cout<<hex<<setfill('0')<<setw(8)<<last_v1<<" "
        <<setw(8)<<last_v0<<setfill(' ')<<dec<<endl;
    cout<<hex<<setfill('0')<<setw(8)<<     v1<<" "
        <<setw(8)<<     v0<<setfill(' ')<<dec<<endl;
#endif

    v=vh*6400000000; // fs
    v+=vl*120300;

    //double dv=(double)v/1000 000 000.; //us
    //unsigned long long int diff=v-last;
    //double ddiff=(double)diff/1000 000 000.; //us

//     cout<<hex<<setfill('0')<<setw(8)<<v0<<" "<<setw(8)<<v1<<setfill(' ')<<dec
//         <<"  dv = "<<setw(7)<<dv<<" ddiff = "<<ddiff<<endl;

    unsigned long long int bin=(v/binsize_fs)*binsize_fs;
    while (bin>current_bin) {
        if (bin_content)
            cout<<current_bin/1000000000<<"\t"<<bin_content<<endl;
        current_bin+=binsize_fs;
        bin_content=0;
    }
    bin_content++;

    last=v;
    last_vh=vh;
    last_v0=v0, last_v1=v1;
}
//---------------------------------------------------------------------------//
static unsigned int last_vl=0;
static unsigned int vh=0;
static int
parse_val(ems_u32 v0, ems_u32 v1)
{
    int res=0;

    if ((v0&0xffff)==0xffff) {
        return 0;
    }

    if ((v1&0xffff0000)!=0x0fff0000) {
        cerr<<"illegal data words: "
            <<hex<<"0x"<<setfill('0')<<setw(8)<<v0<<" "
            <<"0x"<<setw(8)<<v1<<setfill(' ')<<dec<<endl;
        return -1;
    }

    //int mod=(v0>>28)&0xf;
    //int chan=(v0>>22)&0x3f;

    unsigned int vl=v0&0xffff;
    //unsigned int vh0=(v0>>16)&0x3f;
    //unsigned int vh1=v1&0xffff;
    //unsigned int vh=vh0|(vh1<<6);

#if 0
    cout<<hex<<setfill('0')
        <<"0x"<<setw(8)<<v0<<" "
        <<"0x"<<setw(8)<<v1<<"  "
        <<setfill(' ')<<dec
        <<"vh="<<setw(7)<<vh<<" "
        <<"vh1="<<setw(5)<<vh1<<" "
        <<"vh0="<<setw(2)<<vh0<<" "
        <<"vl="<<setw(5)<<vl<<" ";
#endif

    if (vl<last_vl)
        vh++;
    do_diff(v0, v1, vl, vh);
    
    last_vl=vl;
    return res;
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
        return 0;
    }

    if (sev->size&1) {
        cerr<<"uneven subevent size: "<<sev->size<<endl;
        return -1;
    }

    for (unsigned int i=0; i<sev->size-4; i+=2) {
        if (parse_val(d[0], d[1])<0)
            return -1;
        d+=2;
    }

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
        if (parse_subevent(sev)<0)
                return -1;
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

    unsigned long long int binsize;
    binsize=binsize_fs/1000000000;
    cerr<<"binsize_fs="<<binsize_fs<<" fs"<<endl;
    cerr<<"binsize="<<binsize<<" us"<<endl;

    while (read_record(input.path(), &buf)>0) {
        cluster=new ems_cluster(buf);
        if (ems_data.parse_cluster(cluster)<0)
            return 3;
        delete cluster;
        while (ems_data.get_event(&event)>0) {
            if (parse_event(event)<0)
                return 4;
            delete event;
            events++;
            //if (events>=10)
            //    goto raus;
        }
    }
//raus:
    cerr<<events<<" events"<<endl;

    return 0;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
