/*
 * ems/events++/parse_qdc.cc
 * 
 * created 2006-Jun-12 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <errno.h>
#include <math.h>
#include "compressed_io.hxx"
#include "cluster_data.hxx"
#include <versions.hxx>

VERSION("2006-Jun-12", __FILE__, __DATE__, __TIME__,
"$ZEL: parse_fqdc.cc,v 1.1 2006/09/15 17:29:41 wuestner Exp $")
#define XVERSION

#define swap_int(a) ( ((a) << 24) | \
                      (((a) << 8) & 0x00ff0000) | \
                      (((a) >> 8) & 0x0000ff00) | \
        ((unsigned int)(a) >>24) )

ems_event* event;

int N[16];
double pulse[16][1024];
int pulse_[16][1024];
int qdc_idx[16];

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
static int
subevent_begin(int id)
{
    for (int chan=0; chan<16; chan++) {
        qdc_idx[chan]=0;
        //Q[i]=-1;
        for (int j=0; j<1024; j++)
            pulse_[chan][j]=0;
    }

    return 0;
}
//---------------------------------------------------------------------------//
static int
subevent_end(void)
{
    for (int chan=0; chan<16; chan++) {
        //if (Q[i]>=0)
        //    continue;
        if (qdc_idx[chan]>0) {
            for (int j=0; j<1024; j++)
                pulse[chan][j]+=pulse_[chan][j];
            N[chan]++;
        }
    }
    return 0;
}
//---------------------------------------------------------------------------//
static int
parse_qdc(unsigned int v, int mod, int chan)
{
    int code=(v>>16)&0x3f;
    if (code==0) {
        pulse_[chan][qdc_idx[chan]]=v&0xfff;
        qdc_idx[chan]++;
    //} else if ((code&0x30)==0x20) {
    //    Q[chan]=v&0xfffff;
    //} else {
    }

    return 0;
}
//---------------------------------------------------------------------------//
static int
parse_val(unsigned int v)
{
    int mod, chan, res=0;

    mod=(v>>28)&0xf;
    chan=(v>>22)&0x3f;

    switch (mod) {
    case 0:
        res=parse_qdc(v, mod, chan);
        break;
    default:
        break;
    }

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
        if (sev->sev_id==1) {
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

    for (int i=0; i<16; i++)
        N[i]=0;
    for (int j=0; j<16; j++) {
        for (int i=0; i<1024; i++)
            pulse[j][i]=0;
    }

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
            //if (events>=10) goto raus;
            //if (N[7]>=1) goto raus;
        }
    }
raus:
    cerr<<events<<" events"<<endl;

    for (int j=0; j<16; j++) {
        if (N[j]>0) {
            double n=N[j];
            for (int i=0; i<1024; i++) {
                pulse[j][i]/=n;
            }
        }
    }
    cerr<<"N:";
    for (int j=0; j<16; j++) {
        cerr<<" "<<N[j];
    }
    cerr<<endl;
    for (int i=0; i<1024; i++) {
        for (int j=0; j<16; j++) {
            cout<<pulse[j][i]<<"\t";
        }
        cout<<endl;
    }

#if 0
    for (int i=0; i<1024; i++) {
        cout<<pulse[i]<<endl;
    }

    for (int i=0; i<4096; i++) {
        for (int j=0; j<3; j++) {
            cout<<spectrum[j][i]<<"\t";
        }
        cout<<endl;
    }
#endif
    return 0;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
