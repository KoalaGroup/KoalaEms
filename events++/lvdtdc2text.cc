/*
 * ems/events++/lvdtdc2text.cc
 * 
 * created 2006-Jun-22 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <errno.h>
#include <math.h>
#include "compressed_io.hxx"
#include "cluster_data.hxx"
#include <versions.hxx>

VERSION("2006-Jun-22", __FILE__, __DATE__, __TIME__,
"$ZEL: lvdtdc2text.cc,v 1.1 2006/06/27 18:35:01 wuestner Exp $")
#define XVERSION

#define swap_int(a) ( ((a) << 24) | \
                      (((a) << 8) & 0x00ff0000) | \
                      (((a) >> 8) & 0x0000ff00) | \
        ((unsigned int)(a) >>24) )

#define MAXHITS 8
#define LETRA 1

struct hitdata {
    int val;
    int edge;
};

struct chandata {
    int num_hits;
    struct hitdata hits[MAXHITS];
};

struct chandata cdata[1024];

//---------------------------------------------------------------------------//
static void
clear_chandata(void)
{
    for (int i=0; i<1024; i++) {
        cdata[i].num_hits=0;
    }
}
//---------------------------------------------------------------------------//
static void
dump_chandata(void)
{
    cout.setf(ios::fixed, ios::floatfield);
    for (int i=0; i<1024; i++) {
        if (!cdata[i].num_hits)
            continue;
        int hits=cdata[i].num_hits;
        if (hits>MAXHITS)
            cdata[i].num_hits=MAXHITS;
        cout<<setw(4)<<i<<" "<<setw(2)<<hits;
        for (int j=0; j<cdata[i].num_hits; j++) {
            cout<<(cdata[i].hits[j].edge?" R":" F");
            cout<<cdata[i].hits[j].val<<" ";
            float dval=cdata[i].hits[j].val*.12995;
            cout<<setw(7)<<dval;
        }
        if (cdata[i].num_hits==2) {
            int diff=cdata[i].hits[0].val-cdata[i].hits[1].val;
            float dval=diff*.12995;
            cout<<" ("<<setw(7)<<dval<<")";
        }
        cout<<endl;
    }
}
//---------------------------------------------------------------------------//
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
    switch (xread(p, (char*)(b+2), (size-1)*4)) {
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
parse_tdc(const ems_subevent* sev)
{
    for (unsigned int i=4; i<sev->size; i++) {
        ems_u32 v=sev->data[i];
        int mod_chan, t0, /*t1, */edge;

        mod_chan=(v>>22)&0x3ff;
        //t1=(v>>16)&0x3f;
        if (LETRA) {
            t0=v&0xfffe;
            edge=v&1;
        } else {
            t0=v&0xffff;
            edge=0;
        }
        if (t0&0x8000)
            t0-=0x10000;
        int n=cdata[mod_chan].num_hits;
        if (n<MAXHITS) {
            cdata[mod_chan].hits[n].val=t0;
            cdata[mod_chan].hits[n].edge=edge;
        }
        cdata[mod_chan].num_hits++;
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
        clear_chandata();
        if (parse_tdc(sev)<0)
            return -1;
        cout<<"event "<<event->event_nr<<endl;
        dump_chandata();
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
    long long int events=0;

    if (!input.good()) {
        cout<<"cannot open input file "<<fname<<endl;
        return 1;
    }

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
            events++;
        }
        //if (events>10)
        //    break;
    }
    cerr<<events<<" events converted"<<endl;

    return 0;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
