/*
 * ems/events++/parse_lvd.cc
 * 
 * created 2006-Jun-21 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <errno.h>
#include <math.h>
#include "compressed_io.hxx"
#include "cluster_data.hxx"
#include <versions.hxx>

VERSION("2006-Jun-21", __FILE__, __DATE__, __TIME__,
"$ZEL: cluster2raw.cc,v 1.3 2006/06/22 21:41:05 wuestner Exp $")
#define XVERSION

#define swap_int(a) ( ((a) << 24) | \
                      (((a) << 8) & 0x00ff0000) | \
                      (((a) >> 8) & 0x0000ff00) | \
        ((unsigned int)(a) >>24) )

int modules[16];

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
#if 0
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
        write(1, sev->data, sev->size*sizeof(ems_u32));
        sev=sev->next;
    }

    return 0;
}
#endif
//---------------------------------------------------------------------------//
static int
parse_subevent(const ems_subevent* sev)
{
    static ems_u32 *data=0;
    static unsigned int len=0;
    ems_u32 length, id, evt;

    length=sev->data[0];
    id=sev->data[1];
    evt=sev->data[2];

cerr<<"sev->size="<<sev->size<<endl;
cerr<<"length   ="<<sev->data[0]<<endl;
cerr<<"evt      ="<<sev->data[2]<<endl;

    if (length!=sev->size-1) {
        cerr<<"length="<<length<<" sev size="<<sev->size<<endl;
        return -1;
    }

    if (length>len) {
        delete[] data;
        data=new ems_u32[length];
        if (!data) {
            cerr<<"new ems_u32["<<length<<"]: "<<strerror(errno)<<endl;
            return -1;
        }
        len=length;
    }


    length=0;
    for (unsigned int i=3; i<sev->size; i++) {
        int mod=(sev->data[i]>>28)&0xf;
        modules[mod]++;
        if (mod==0) {
            //cerr<<"mod="<<mod<<endl;
            data[length++]=sev->data[i];
        }
    }
    length*=4;

    write(1, &length, sizeof(ems_u32));
    write(1, &id, sizeof(ems_u32));
    write(1, &evt, sizeof(ems_u32));

    for (unsigned int i=0; i<length; i++) {
        write(1, data, length);
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
    long int events=0;

    if (!input.good()) {
        cerr<<"cannot open input file "<<fname<<endl;
        return 1;
    }

    for (int i=0; i<16; i++) {
        modules[i]=0;
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
        if (events>10)
            break;
    }
    cerr<<events<<" events converted"<<endl;
    for (int i=0; i<16; i++) {
        cerr<<"module "<<i<<": "<<modules[i]<<endl;
    }

    return 0;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
