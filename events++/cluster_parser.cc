/*
 * ems/events++/cluster_parser.cc
 * 
 * created 2006-Apr-28 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <errno.h>
#include "compressed_io.hxx"
#include "cluster_data.hxx"
#include <versions.hxx>

VERSION("2006-May-02", __FILE__, __DATE__, __TIME__,
"$ZEL: cluster_parser.cc,v 1.2 2006/05/03 11:53:06 wuestner Exp $")
#define XVERSION

#define swap_int(a) ( ((a) << 24) | \
                      (((a) << 8) & 0x00ff0000) | \
                      (((a) >> 8) & 0x0000ff00) | \
        ((unsigned int)(a) >>24) )

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
        cerr<<"unknown endian "
            <<hex<<showbase<<setw(10)<<setfill('0')<<internal
            <<head[1]
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
static int
parse_event(const ems_event* event)
{
    static ems_u32 ev_no=0;
//     if (event->event_nr!=ev_no+1) {
//         cerr<<"jump from "<<ev_no<<" to "<<event->event_nr<<endl;
//     }
    ev_no=event->event_nr;

    if (!(ev_no&ev_no-1)) {
        event->dump(1);
    }

    return 0;
}
//---------------------------------------------------------------------------//
#define VARIANTE 1

#if VARIANTE==1

int
main(int argc, char* argv[])
{
    string fname=argc>1?argv[1]:"-";
    compressed_input input(fname);
    ems_data ems_data;
    ems_cluster* cluster;
    enum clustertypes last_type;
    ems_u32 *buf;
    int res;

    if (!input.good()) {
        cerr<<"cannot open input file "<<fname<<endl;
        return 1;
    }

// read until we get the first event cluster
    do {
        res=read_record(input.path(), &buf);
        if (res<0)
            return 2;
        cluster=new ems_cluster(buf);

        last_type=cluster->type;
        if (last_type!=clusterty_events) {
            res=ems_data.parse_cluster(cluster);
            delete cluster;
            if (res<0)
                return 3;
        }
    } while (last_type!=clusterty_events);

// here all headers should be stored inside ems_data

    for (int i=0; i<ems_data.nr_texts; i++) {
        if (ems_data.texts[i]->key=="masterheader")
            ems_data.texts[i]->dump(1);
        else
            ems_data.texts[i]->dump(0);
    }
    for (int i=0; i<ems_data.nr_files; i++) {
        if (ems_data.files[i]->name=="/home/anke/setupfiles/em.wad")
            ems_data.files[i]->dump(1);
        else
            ems_data.files[i]->dump(0);
    }

// read all remaining clusters
    do {
        ems_event* event;
        res=ems_data.parse_cluster(cluster);
        delete cluster;
        if (res<0)
            return 3;
        while (ems_data.get_event(&event)>0) {
            // do something useful with the event
            res=parse_event(event);
            delete event;
            if (res<0)
                return 4;
        }
        res=read_record(input.path(), &buf);
        if (res<=0)
            break;
        cluster=new ems_cluster(buf);
    } while (1);
    for (int i=0; i<ems_data.nr_texts; i++) {
        if (ems_data.texts[i]->key=="superheader")
            ems_data.texts[i]->dump(1);
    }

    return 0;
}

#elif VARIANTE==2

int
main(int argc, char* argv[])
{
    string fname=argc>1?argv[1]:"-";
    compressed_input input(fname);
    ems_data ems_data;
    ems_cluster* cluster;
    ems_u32 *buf;

    if (!input.good()) {
        cerr<<"cannot open input file "<<fname<<endl;
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
        }
    }
    return 0;
}

#elif VARIANTE==3

static int p;
static ems_data ems_data;

static ems_event*
get_event(void)
{
    ems_event* event;
    while (!ems_data.events_available()) {
        ems_cluster* cluster;
        ems_u32 *buf;
        int res;
        res=read_record(p, &buf);
        if (res<=0)
            return 0;
        cluster=new ems_cluster(buf);
        res=ems_data.parse_cluster(cluster);
        if (res<0)
            return 0;
        delete cluster;
    }
    ems_data.get_event(&event);
    return event;
}

int
main(int argc, char* argv[])
{
    string fname=argc>1?argv[1]:"-";
    compressed_input input(fname);
    ems_event* event;

    if (!input.good()) {
        cerr<<"cannot open input file "<<fname<<endl;
        return 1;
    }
    p=input.path();

    while ((event=get_event())) {
        if (parse_event(event)<0)
            return 4;
        delete event;
    }

    return 0;
}

#else
#error no valid VARIANTE selected
#endif
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
