/*
 * ems/events++/parse_synced_lvd.cc
 * 
 * created 2007-Aug-02 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <cerrno>
#include <cmath>
#include <sstream>
#include <cstdlib>
#include "modultypes.h"
#include "compressed_io.hxx"
#include "cluster_data_synced.hxx"
#include <readargs.hxx>
#include <versions.hxx>

VERSION("2007-Aug-05", __FILE__, __DATE__, __TIME__,
"$ZEL: parse_synced_lvd.cc,v 1.1 2010/09/10 22:53:02 wuestner Exp $")
#define XVERSION

#define swap_int(a) ( ((a) << 24) | \
                      (((a) << 8) & 0x00ff0000) | \
                      (((a) >> 8) & 0x0000ff00) | \
        ((unsigned int)(a) >>24) )

C_readargs* args;
STRING infile;
//---------------------------------------------------------------------------//
int readargs()
{
    args->addoption("infile", 0, "-", "input file", "input");

    if (args->interpret()!=0)
        return -1;

    infile=args->getstringopt("infile");

    return 0;
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
//---------------------------------------------------------------------------//
static int
parse_text_configuration(ems_data* ems_data, ems_text *text)
{
#if 0
    text->dump(1);
#endif

    ved_configuration* conf=new ved_configuration(text);

    for (int i=0; i<text->nr_lines; i++) {
        string line=text->lines[i];
        if (line.substr(0, 5)=="name ") {
            conf->name=line.substr(5, string::npos);
        }
        if (line.substr(0, 8)=="vedname ") {
            conf->vedname=line.substr(8, string::npos);
        }
        if (line.substr(0, 13)=="eventbuilder ") {
            string val=line.substr(13, string::npos);
            istringstream is(val);
            is>>conf->eventbuilder;
        }
        if (line.substr(0, 14)=="triggermaster ") {
            string val=line.substr(14, string::npos);
            istringstream is(val);
            is>>conf->triggermaster;
       }
        if (line.substr(0, 4)=="isid") {
            char ch;
            string val=line.substr(4, string::npos);
            istringstream is(val);
            int idx, id, count=0;
            do {
                is.get(ch);
            } while (ch!='(' && count++<20);
            is>>idx;
            is.get(ch); // ')'
            is>>id;
            conf->add_isid(idx, id);
        }
    }
#if 0
    conf->dump(1);
#endif
    ems_data->ved_configurations.add_conf(conf);

    return 0;
}
//---------------------------------------------------------------------------//
static int
parse_file_sync_connections(ems_data* ems_data, ems_file *file)
{
    file->dump(0);

    ems_data->sync_connections=new sync_connections(file);

    string line;
    istringstream isf(file->content);
    while (!isf.eof()) {
        size_t p0, p1, p2;
        int addr, port;
        getline(isf, line);
        //cout<<"###"<<line<<"###"<<endl;
        p0=line.find("set sync_connections(");
        if (p0==string::npos)
            continue;
        p1=line.find(')', p0);
        string name=line.substr(p0+21, p1-(p0+21));
        //cout<<"name is >"<<name<<"<"<<endl;
        p2=line.find('{', p1);
        istringstream is(line.substr(p2+1, string::npos));
        //cout<<"rest is >"<<line.substr(p2+1, string::npos)<<"<"<<endl;
        is>>hex>>addr>>dec>>port;
        if (addr>=0x100)
            addr-=0x100;
        //cout<<"addr="<<addr<<" port="<<port<<endl;
        ems_data->sync_connections->add_conn(name, addr, port);
    }
    return 0;
}
//---------------------------------------------------------------------------//
static int
parse_file_crates(ems_file *file)
{
    file->dump(0);
    return 0;
}
//---------------------------------------------------------------------------//
static int
read_and_parse_record(int p, ems_data* ems_data)
{
    ems_cluster* cluster;
    ems_u32 *buf;
    int res;

    res=read_record(p, &buf);
    if (res<=0) {
        cout<<"Fehler 1 in read_and_parse_record"<<endl;
        return res;
    }

    cluster=new ems_cluster(buf);
    res=ems_data->parse_cluster(cluster);
    delete cluster;
    if (res<0)
        cout<<"Fehler 2 in read_and_parse_record"<<endl;
    return res<0?-1:1;
}
//---------------------------------------------------------------------------//
static int
read_record_for_ved(int p, ems_data* ems_data, int ved_idx,
        ems_u32 ev_no)
{
    bool empty;
    int nr_is=ems_data->ved_info.ved_infos[ved_idx].nr_is;
    struct ems_data::ved_slot* ved_slot=&ems_data->ved_slots[ved_idx];

    do {
        for (int i=0; i<nr_is; i++) {
            ved_slot->is_data[i].purge_before(ev_no);            
        }
        empty=true;
        for (int i=0; i<nr_is; i++) {
            if (ved_slot->is_data[i].first)
                empty=false;
        }
        if (empty) {
            if (read_and_parse_record(p, ems_data)<0) {
                cout<<"Fehler in read_record_for_ved"<<endl;
                return -1;
            }
        }
    } while (empty);
    return 0;
}
//---------------------------------------------------------------------------//
static int
complete_informations(ems_data* ems_data)
{
    // find synchronisation IS
    if (ems_data->ved_info.find_is(2000,
            &ems_data->sync_ved.ved_idx,
            &ems_data->sync_ved.is_idx)<0) {
        cout<<"sync IS not found"<<endl;
        return -1;
    }
    cout<<"sync IS found at ved "<<ems_data->sync_ved.ved_idx
        <<", is "<<ems_data->sync_ved.is_idx<<endl;
    ems_data->sync_ved.slot=&ems_data->ved_slots[ems_data->sync_ved.ved_idx].
            is_data[ems_data->sync_ved.is_idx];

    if (ems_data->sync_connections==0) {
        cout<<"no synchronisation connections known"<<endl;
        return -1;
    }
    if (ems_data->ved_configurations.confs==0) {
        cout<<"no ved configuraions known"<<endl;
        return -1;
    }

    // connect information in ems_ved_info and ved_configuration
    // we use IS_IDs to match the VEDs
    ved_configuration* conf=ems_data->ved_configurations.confs;
    while (conf) {
        if (conf->num_isids) {
            int ved_idx, is_idx;
            ems_data->ved_info.find_is(conf->isids[0].ID, &ved_idx, &is_idx);
            ems_data->ved_info.ved_infos[ved_idx].name=conf->name;
        }
        conf=conf->next;
    }


    // clear slotlist
    for (int i=0; i<16*8; i++)
        ems_data->slotlist[i]=0;

    // iterate over all synchronisation connections
    for (int i=0; i<ems_data->sync_connections->num_conn; i++) {
        struct sync_connections::conn* conn=&ems_data->sync_connections->conn[i];
        int idx=conn->addr*8+conn->port;
        // find ved_info
        int ved_idx=ems_data->ved_info.find_ved_idx(conn->crate_name);
        if (ved_idx<0) {
            cout<<"no ved info for "<<conn->crate_name<<" found"<<endl;
            return -1;
        }
        ems_data->ved_info.ved_infos[ved_idx].sync=idx;
        ems_data->slotlist[idx]=&ems_data->ved_slots[ved_idx];
    }
#if 0
    for (int i=0; i<16*8; i++) {
        if (ems_data->slotlist[i])
            cout<<"mod "<<((i>>3)&0xf)<<" port "<<(i&7)<<" idx "
                <<ems_data->slotlist[i]-ems_data->ved_slots<<endl;
    }
#endif
    ems_data->ved_info.dump(2);

    return 0;
}
//---------------------------------------------------------------------------//
#undef VERBOSE_SYNC
//#ifdef VERBOSE_SYNC
#if 1
static void
parse_sync_subevent_verbose(ems_data* ems_data,
    struct ems_data::is_data* is_data)
{
    int accepted=0;
    int ttime;

    if (is_data->size>100) {
        return;
    }
    cout<<endl<<"### Sync Error in event "<<is_data->ev_no<<endl;
    for (int i=0; i<3; i++)
        printf("%08x\n", is_data->data[i]);
    for (int i=3; i<is_data->size; i++) {
        ems_u32 d=is_data->data[i];
        printf("%08x", d);
        int addr=(d>>28)&0xf;
        if (addr==0) { // master
            if (d&0x04000000) { // tdt
                printf(" tdt %.1f us", (d&0xfffff)*0.1);
            } else {            // trigger time
                int trigger=(d>>22)&0xf;
                int rejected=!!(d&0x00100000);
                int time=(d&0xfffff); // *12.5ns
                printf(" %sted trigger %d %.2f us",
                        rejected?"rejec":"accep",
                        trigger,
                        time*.0125);
                if (!rejected) {
                    if (accepted>0 && time!=ttime)
                        cout<<endl<<"         "<<"trigger time mismatch in event "
                            <<is_data->ev_no;
                    accepted++;
                    ttime=d&0xfffff;
                }
            }
        } else {       // output
            if (d&0x00040000) { // event counter
                cout<<" event "<<(d&0xffff);
            } else {            // dead time
                if (d&0x00030000) {
                    cout<<" trigger error in event "<<is_data->ev_no
                        <<endl<<"        ";
                }
                int time=(d&0xffff); // *100ns
                int port=(d>>22)&0x7;
                printf(" dead time mod %d port %d: %5.1f us",
                        addr, port, time*0.1);

                int idx=(addr<<3)+port;
                if (idx>=128 || ems_data->slotlist[idx]==0) {
                    cout<<endl<<"         illegal sync index "<<idx<<" in event "
                        <<is_data->ev_no;
                }
            }
        }
        cout<<endl;
    }
    cout<<endl;
}
#endif
//---------------------------------------------------------------------------//
static int
parse_sync_subevent(ems_data* ems_data, struct ems_data::is_data* is_data,
        struct event_info* event)
{
    if (is_data->ev_no<10)
        return -1;
    event->rejected=0;
    event->accepted=0;
    event->trigger=0;
    event->ev_no=is_data->ev_no-1;

    if (is_data->size>50) {
        cout<<"event "<<is_data->ev_no<<" size="<<is_data->size<<endl;
        goto error;
    }

    for (int i=3; i<is_data->size; i++) {
        ems_u32 d=is_data->data[i];
        int addr=(d>>28)&0xf;
        if ((d&0xffff)==((d>>16)&0xffff)) { // probably wrong
            parse_sync_subevent_verbose(ems_data, is_data);
            return -1;
        }
        if (addr==0) { // master
            if (d&0x04000000) { // tdt
                event->tdt=(d&0xfffff)*0.1; // us
            } else {            // trigger time
                int trigger=(d>>22)&0xf;
                int rejected=!!(d&0x00100000);
                int time=(d&0xfffff); // *12.5ns
                if (rejected) {
                    event->rejected++;
                } else {
                    if (event->accepted>0) {
                        if (event->time!=time) {
                            cout<<"trigger time mismatch in event "
                                <<is_data->ev_no<<endl;
                            goto error;
                        }
                    }
                    event->accepted++;
                    event->trigger|=(1<<(trigger));
                    event->time=time;
                }
            }
        } else {       // output
            if (d&0x00040000) { // event counter
            } else {            // dead time
                if (d&0x00030000) {
                    cout<<"trigger error in event "<<is_data->ev_no<<endl;
                    goto error;
                }
                double time=(d&0xffff)*0.1; // us
                int port=(d>>22)&0x7;
                int idx=(addr<<3)+port;
                if (idx>=128 || ems_data->slotlist[idx]==0) {
                    cout<<"illegal sync index "<<idx<<" in event "
                        <<is_data->ev_no<<endl;
                    goto error;
                }
                ems_data->slotlist[idx]->required=true;
                ems_data->slotlist[idx]->new_ldt=time;
            }
        }
    }
    if (event->accepted<1) {
        cout<<"no accepted trigger in event "<<is_data->ev_no<<endl;
        goto error;
    }
    if (event->accepted>1) {
        cout<<event->accepted<<" accepted triggers in event "<<is_data->ev_no
            <<endl;
    }
if (event->ev_no==125897
||event->ev_no==125880
||event->ev_no==125881
||event->ev_no==241662
||event->ev_no==241663
||event->ev_no==241664
||event->ev_no==1128189
||event->ev_no==1128190
||event->ev_no==1128191
)
parse_sync_subevent_verbose(ems_data, is_data);
    return 0;

error:
#ifdef VERBOSE_SYNC
    parse_sync_subevent_verbose(ems_data, is_data);
#endif
    return -1;
}
//---------------------------------------------------------------------------//
static int
do_event(ems_data* ems_data, struct event_info* event)
{
    //cout<<"check event "<<event->ev_no<<endl;
double max=0;    
    int esize=0;
    for (int i=0; i<ems_data->ved_info.nr_veds; i++) {
        if (ems_data->ved_slots[i].required) {
            //cout<<"check ved "<<ems_data->ved_info.ved_infos[i].name<<endl;
            struct ems_data::ved_slot* ved_slot=&ems_data->ved_slots[i];
            int num_sevs=0;
            int size=0;
            for (int j=0; j<ems_data->ved_info.ved_infos[i].nr_is; j++) {
                struct ems_data::is_data_slot* is_slot=&ved_slot->is_data[j];
                if (is_slot->first) {
                    if (is_slot->first->ev_no!=event->ev_no) {
                        cout<<"wrong ev_no "<<is_slot->first->ev_no<<" in "
                            <<ems_data->ved_info.ved_infos[i].name<<" IS "
                            <<ems_data->ved_info.ved_infos[i].is_infos[j].IS_ID
                            <<endl;
                    } else {
                        size+=is_slot->first->size;
                        num_sevs++;
                        is_slot->purge_first();
                    }
                }
            }
            if (num_sevs==0) {
                cout<<"no data for "<<ems_data->ved_info.ved_infos[i].name
                    <<" in event "<<event->ev_no<<endl;
            } else {
                //cout<<"OK"<<endl;
            }
            if (max<ved_slot->new_ldt)
                max=ved_slot->new_ldt;
            ved_slot->ldt.add(ved_slot->new_ldt);
            ved_slot->size.add(size);
            esize+=size;
        }
    }
    ems_data->tdt.add(event->tdt);
    ems_data->max.add(max);
    ems_data->size.add(esize);
    if (fabs(max-event->tdt)>20) {
        cout<<"event "<<event->ev_no<<": max-tdt="<<max-event->tdt<<endl;
    }
    return 0;
}
//---------------------------------------------------------------------------//
static void
dump_statistic(ems_data* ems_data)
{
    cout<<"Statistics:"<<endl;
    for (int i=0; i<ems_data->ved_info.nr_veds; i++) {
        struct ems_data::ved_slot* ved_slot=&ems_data->ved_slots[i];
        cout<<"ved "<<i<<" "<<ems_data->ved_info.ved_infos[i].name<<endl;
        cout<<"  ldt : "; ved_slot->ldt.dump(); cout<<endl;
        cout<<"  size: "; ved_slot->size.dump(); cout<<endl;
        for (int j=0; j<ems_data->ved_info.ved_infos[i].nr_is; j++) {
            struct ems_data::is_data_slot* slot=&ved_slot->is_data[j];
            cout<<"  is "<<j<<endl;
            cout<<"  records="<<slot->records<<endl;
        }
    }
    cout<<"tdt : "; ems_data->tdt.dump(); cout<<endl;
    cout<<"max : "; ems_data->max.dump(); cout<<endl;
    cout<<"size: "; ems_data->size.dump(); cout<<endl;
}
//---------------------------------------------------------------------------//
int
main(int argc, char* argv[])
{
    args=new C_readargs(argc, argv);
    if (readargs()!=0)
        return 1;

    compressed_input input(infile);
    ems_data ems_data;
    ems_cluster* cluster;
    enum clustertypes last_type;
    ems_u32 *buf;

    if (!input.good()) {
        cout<<"cannot open input file "<<infile<<endl;
        return 1;
    }

    //prepare_globals();

// read until we get the first event cluster
    do {
        int res;
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

// here all headers are stored inside ems_data

    for (int i=0; i<ems_data.nr_texts; i++) {
        if (ems_data.texts[i]->key=="superheader")
            ems_data.texts[i]->dump(0);
        else if (ems_data.texts[i]->key=="masterheader")
            ems_data.texts[i]->dump(0);
        else if (ems_data.texts[i]->key=="configuration")
            parse_text_configuration(&ems_data, ems_data.texts[i]);
        else
            ems_data.texts[i]->dump(0);
    }
    for (int i=0; i<ems_data.nr_files; i++) {
        if (ems_data.files[i]->name=="/home/wasa/ems_setup/daq/sync_connections.data")
            parse_file_sync_connections(&ems_data, ems_data.files[i]);
        else if (ems_data.files[i]->name=="/home/wasa/ems_setup/daq/crates")
            parse_file_crates(ems_data.files[i]);
        else
            ems_data.files[i]->dump(0);
    }

    if (complete_informations(&ems_data)<0)
        return 4;

    int events=0;
    int errors=0;
    while (events<10000000) {
        // read data until we have two sync subevents
        // (we need only the second one)
        while (ems_data.sync_ved.slot->first==0 ||
                ems_data.sync_ved.slot->first->next==0) {
            if (read_and_parse_record(input.path(), &ems_data)<=0) {
                cout<<"Fehler 1 in main"<<endl;
                goto error;
            }
        }

        struct ems_data::is_data* sdata=ems_data.sync_ved.slot->first->next;
        struct event_info event;

        ems_data.clear_ved_flags();
        if (parse_sync_subevent(&ems_data, sdata, &event)<0) {
            ems_data.sync_ved.slot->purge_first();
            errors++;
            continue;
        }
        // skip scaler events
        if (ems_data.slotlist[(3<<3)+3]->required) {
            ems_data.sync_ved.slot->purge_first();
            continue;
        }

        for (int i=0; i<ems_data.ved_info.nr_veds; i++) {
            if (ems_data.ved_slots[i].required) {
                if (read_record_for_ved(input.path(), &ems_data,
                        i, event.ev_no)<0) {
                    cout<<"Fehler 2 in main"<<endl;
                    goto error;
                }
            }
        }
        // hier sollten alle subevents da sein
        do_event(&ems_data, &event);
        events++;
    }

error:
    cout<<events<<" events analysed"<<endl;
    cout<<errors<<" events have sync errors"<<endl;
    dump_statistic(&ems_data);

    delete args;
    return 0;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
