/*
 * ems/events++/parse_gpx.cc
 * 
 * created 2006-May-03 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <cstring>
#include "compressed_io.hxx"
#include "cluster_data.hxx"
#include "swap_int.h"
#include <versions.hxx>

VERSION("2014-07-07", __FILE__, __DATE__, __TIME__,
"$ZEL: parse_gpx.cc,v 1.2 2014/07/07 20:22:04 wuestner Exp $")
#define XVERSION

struct spect {
    spect(void);
    void compute(void);
    int dump(void);
    long long data[65536];
    string name;
    long long num;
    long long sum;
    long long qsum;
    int minimum;
    int maximum;
};

struct spect spects[7][2];

long long N=0;

//---------------------------------------------------------------------------//
spect::spect(void)
{
    bzero(data, sizeof(data));
    num=0; sum=0; qsum=0;
    maximum=-32769;
    minimum=65536;
}
//---------------------------------------------------------------------------//
void
spect::compute(void)
{
    double dnum=num;
    double dsum=sum;
    double dqsum=qsum;
    double average=dsum/dnum;
    double sigma=sqrt(dqsum/dnum-average*average);
    cout<<"["<<name<<"]:"
        <<" num="<<num
        <<" min="<<minimum
        <<" max="<<maximum
        <<" average="<<average
        <<" sigma="<<sigma
        <<endl;
}
//---------------------------------------------------------------------------//
int
spect::dump(void)
{
    string st=name;
    FILE* f=fopen(st.c_str(), "w");
    if (!f) {
        cout<<"open "<<name<<": "<<strerror(errno)<<endl;
        return -1;
    }
    for (int i=minimum; i<maximum; i++) {
        fprintf(f, "%d %lld\n", -(i-32768)*2, data[i]);
    }
    fclose(f);
    return 0;
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
static int
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
static int
read_record(int p, ems_u32** buf)
{
    ems_u32 head[2], *b;
    ems_u32 size;
    int wenden;

    switch (xread(p, reinterpret_cast<char*>(head), 8)) {
    case -1: // error
        cout<<"read header: "<<strerror(errno)<<endl;
        return -1;
    case 0:
        cout<<"read: end of file"<<endl;
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
        cout<<"unknown endian "<<hex<<showbase<<setw(8)<<setfill('0')<<head[1]
            <<dec<<noshowbase<<setfill(' ')<<endl;
        return -1;
    }

    b=new ems_u32[size+1];
    if (!b) {
        cout<<"new buf: "<<strerror(errno)<<endl;
        return -1;
    }

    b[0]=head[0];
    b[1]=head[1];
    switch (xread(p, reinterpret_cast<char*>(b+2), (size-1)*4)) {
    case -1: // error
        cout<<"read body: "<<strerror(errno)<<endl;
        return -1;
    case 0:
        cout<<"read body: unexpected end of file"<<endl;
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
static int
parse_tdc_val(unsigned int v)
{
    int mod, chan, t0, edge;

    mod=v>>28;
    chan=(v>>22)&0x3f;
    switch (mod) {
    case 0: // GPX
        if ((chan!=0)&&(chan!=16)&&(chan!=32)&&(chan!=48)) {
            cout<<"illegal channel "<<chan<<" in module "<<mod<<endl;
            return -1;
        }
        chan/=16;
        break;
    case 1: // F1
        if (chan!=0) {
            cout<<"illegal channel "<<chan<<" in module "<<mod<<endl;
            return -1;
        }
        chan=4;
        break;
    case 2: // F1
        if (chan!=0) {
            cout<<"illegal channel "<<chan<<" in module "<<mod<<endl;
            return -1;
        }
        chan=5;
        break;
    case 3: // F1
        if (chan!=0) {
            cout<<"illegal channel "<<chan<<" in module "<<mod<<endl;
            return -1;
        }
        chan=6;
        break;
    case 4: // SQDC
        // ignored
        chan=-1;
        break;
    default:
        cout<<"unknown module "<<mod<<endl;
        return -1;
    }

    if (chan>=0) {
        //t1=(v>>16)&0x3f;
        t0=v&0xffff;
        if (t0&0x8000)
            t0-=0x10000;
        t0/=2;
        edge=v&1;

        if (t0==0) {
            printf("mod %d chan %d v 0x%08x\n", mod, chan, v);
        }
        struct spect* spect=&spects[chan][edge];
        spect->num++;
        spect->sum+=t0;
        spect->qsum+=t0*t0;
        t0+=32768; // shift to non negative indices
        spect->data[t0]++;
        if (t0>spect->maximum)
            spect->maximum=t0;
        if (t0<spect->minimum)
            spect->minimum=t0;
    }

    return 0;
}
//---------------------------------------------------------------------------//
static int
parse_tdc(const ems_subevent* sev)
{
    unsigned int* d=sev->data+4;

    for (unsigned int i=0; i<sev->size-4; i++) {
        if (parse_tdc_val(*d++)<0)
            return -1;
    }
    return 0;
}
//---------------------------------------------------------------------------//
static int
parse_event(const ems_event* event)
{
    static ems_u32 ev_no=0;
    if (event->event_nr!=ev_no+1) {
        cout<<"jump from "<<ev_no<<" to "<<event->event_nr<<endl;
    }
    ev_no=event->event_nr;

    ems_subevent* sev=event->subevents;
    while (sev) {
        if (parse_tdc(sev)<0)
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

    if (!input.good()) {
        cout<<"cannot open input file "<<fname<<endl;
        return 1;
    }

    spects[0][0].name="GPX-0-te-3";
    spects[0][1].name="GPX-0-le-3";
    spects[1][0].name="GPX-1-te-3";
    spects[1][1].name="GPX-1-le-3";
    spects[2][0].name="GPX-2-te-3";
    spects[2][1].name="GPX-2-le-3";
    spects[3][0].name="GPX-3-te-3";
    spects[3][1].name="GPX-3-le-3";
    spects[4][0].name="F1-0-te-3";
    spects[4][1].name="F1-0-le-3";
    spects[5][0].name="F1-2-te-3";
    spects[5][1].name="F1-2-le-3";
    spects[6][0].name="F1-3-te-3";
    spects[6][1].name="F1-3-le-3";

    while (read_record(input.path(), &buf)>0) {
        ems_event* event;
        cluster=new ems_cluster(buf);
        if (ems_data.parse_cluster(cluster)<0)
            return 3;
        delete cluster;
        while (ems_data.get_event(&event)>0) {
            if (parse_event(event)<0)
                return 4;
            delete event;
            //if (N>=10) goto raus;
        }
    }

    for (int i=0; i<7; i++) {
        for (int j=0; j<2; j++) {
            spects[i][j].compute();
            spects[i][j].dump();
        }
    }

    return 0;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
