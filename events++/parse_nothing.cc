/*
 * ems/events++/parse_nothing.cc
 * 
 * created 2006-Dec-11 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include "compressed_io.hxx"
#include "cluster_data.hxx"
#include "swap_int.h"
#include <versions.hxx>

VERSION("2014-07-07", __FILE__, __DATE__, __TIME__,
"$ZEL: parse_nothing.cc,v 1.3 2014/07/14 16:18:17 wuestner Exp $")
#define XVERSION

ems_event* event;
int jumps=0;
int jump_dist=0;

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
 *
 * *buf is set to a pointer of a new allocated array of ems_u32 containing the
 * record data
 * it has to be freed using delete[]
 */
static int
read_record(int p, ems_u32** buf)
{
    ems_u32 head[2], *b;
    ems_u32 size;
    int wenden;

    switch (xread(p, reinterpret_cast<char*>(head), 8)) {
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
    switch (xread(p, reinterpret_cast<char*>(b+2), (size-1)*4)) {
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
parse_subevent(const ems_subevent* sev)
{
    if (sev->size<4) {
        cerr<<"sev size="<<sev->size<<endl;
        return 0;
    }

    if ((sev->data[1]&0xffff)!=(sev->size-1)*4) {
        return 0;
    }

    return 0;
}
//---------------------------------------------------------------------------//
static int
parse_event(const ems_event* event)
{
    static ems_u32 ev_no=0;
    if (event->event_nr!=ev_no+1) {
        //cerr<<"jump from "<<ev_no<<" to "<<event->event_nr<<endl;
        jumps++;
        jump_dist+=event->event_nr-ev_no-1;
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
    int events=0, res=0;

    if (!input.good()) {
        cerr<<"cannot open input file "<<fname<<endl;
        res=1;
        goto raus;
    }

    while (read_record(input.path(), &buf)>0) {
        cluster=new ems_cluster(buf);
        res=ems_data.parse_cluster(cluster);
        delete cluster;
        if (res<0) {
            res=3;
            goto raus;
        }
        while (ems_data.get_event(&event)>0) {
            res=parse_event(event);
            delete event;
            if (res<0) {
                res=4;
                goto raus;
            }
            events++;
        }
    }

raus:
    cout<<events<<" events parsed"<<endl;
    cout<<ems_data.purged_uncomplete<<" uncomplete events purged"<<endl;
    cout<<jumps<<" event jumps"<<endl;
    cout<<"jumped over "<<jump_dist<<" events"<<endl;
    return res;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
