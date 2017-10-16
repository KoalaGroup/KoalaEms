/*
 * ems/events++/parse_sqdc.cc
 * 
 * created 2006-May-11 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <errno.h>
#include <math.h>
#include "compressed_io.hxx"
#include "cluster_data.hxx"
#include <versions.hxx>

VERSION("2006-May-11", __FILE__, __DATE__, __TIME__,
"$ZEL: parse_sqdc1000.cc,v 1.2 2006/08/31 21:32:32 wuestner Exp $")
#define XVERSION

#define swap_int(a) ( ((a) << 24) | \
                      (((a) << 8) & 0x00ff0000) | \
                      (((a) >> 8) & 0x0000ff00) | \
        ((unsigned int)(a) >>24) )

ems_event* event;

int N;
double pulse[1000][1024];
int sqdc_idx;

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
    sqdc_idx=0;
    return 0;
}
//---------------------------------------------------------------------------//
static int
subevent_end(void)
{
    if (sqdc_idx>0)
        N++;
    cerr<<"N="<<N<<" sqdc_idx="<<sqdc_idx<<endl;
    return 0;
}
//---------------------------------------------------------------------------//
static int
parse_sqdc(unsigned int v, int mod, int chan)
{
    //cerr<<"mod="<<mod<<" chan="<<chan<<" v="<<hex<<v<<dec<<endl;
    int code=(v>>16)&0x3f;
    if (code==0) {
        pulse[N][sqdc_idx++]=v&0xfff;
    }

    return 0;
}
//---------------------------------------------------------------------------//
static int
parse_val(unsigned int v)
{
    int mod, chan, res=0;

    mod=v>>28;
    chan=(v>>22)&0x3f;

    switch (mod) {
    case 0:
        if (chan==7)
            res=parse_sqdc(v, mod, chan);
        break;
    case 1:
    case 4:
    case 8:
        break;
    default:
        cerr<<"unknown module "<<mod<<endl;
        res=-1;
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

    N=0;
    for (int j=0; j<1000; j++) {
        for (int i=0; i<1024; i++)
            pulse[j][i]=17;
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
            if ((N>=16) || (events>100)) goto raus;
        }
    }
raus:
    cerr<<events<<" events"<<endl;

    for (int i=0; i<1024; i++) {
        for (int j=0; j<N; j++) {
            cout<<pulse[j][i]<<"\t";
        }
        cout<<endl;
    }

    return 0;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
