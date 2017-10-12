/*
 * ems/events++/cluster_data.cc
 * 
 * created 2006-Apr-28 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <cerrno>
#include <cmath>
#include "cluster_data_synced.hxx"
#include <xdrstring.h>
#include <versions.hxx>

VERSION("2006-Dec-12", __FILE__, __DATE__, __TIME__,
"$ZEL: cluster_data.cc,v 1.6 2007/04/22 22:14:56 wuestner Exp $")
#define XVERSION

//***************************************************************************//
#if 0
static void
dump_raw(ems_u32 *p, int num, char* text)
{
    printf("%s:\n", text);
    for (int i=0; i<num; i++) {
        printf("[%2d] 0x%08x\n", i, p[i]);
    }
}
#endif
//***************************************************************************//
//ems_data

ems_data::ems_data(void)
:nr_texts(0), texts(0), nr_files(0), files(0)
{}
//---------------------------------------------------------------------------//
ems_data::~ems_data()
{
    for (int i=0; i<nr_texts; i++)
        delete texts[i];
    delete[] texts;
    for (int i=0; i<nr_files; i++)
        delete files[i];
    delete[] files;
    delete sync_connections;
    delete[] ved_slots;
        
}
//---------------------------------------------------------------------------//
ems_data::ved_slot::~ved_slot()
{
    delete[] is_data;
}
//---------------------------------------------------------------------------//
int
ems_data::parse_timestamp(const ems_cluster *cluster, struct timeval *tv)
{
    tv->tv_sec=tv->tv_usec=0;
    ems_u32* options=cluster->data+3;

    if (options[0]==0) // no options exist
        return 0;
    if (!(options[1]&1)) // no timestamp
        return 0;
    tv->tv_sec=options[2];
    tv->tv_usec=options[3];
        return 0;
}
//---------------------------------------------------------------------------//
struct ems_data::is_data_slot*
ems_data::find_is_slot(int VED_ID, int IS_ID) const
{
    int ved_idx=ved_info.find_ved_idx(VED_ID);
    if (ved_idx==-1) {
        cout<<"find_is_slot: VED_ID "<<VED_ID<<" not found"<<endl;
        return 0;
    }
    int is_idx=ved_info.find_is_idx(ved_idx, IS_ID);
    if (is_idx==-1) {
        cout<<"find_is_slot: IS_ID "<<IS_ID<<" not found"<<endl;
        return 0;
    }
    return &ved_slots[ved_idx].is_data[is_idx];
}
//---------------------------------------------------------------------------//
int
ems_data::parse_subevent_lvd(ems_u32* sub_event, int ved_idx, ems_u32 ev_no,
    int trigno)
{
    ems_u32* p=sub_event;

    //dump_raw(p, 10, "parse_subevent_lvd");

    int IS_ID=*p++;
    int sev_size=*p++;

    while (sev_size>=3) {
        int lvd_size=(p[0]&0xffff)/4;

        struct is_data* is_data=new struct is_data;
        is_data->size=lvd_size;
        //int lvd_time=p[1];
        is_data->ev_no=p[2];

        is_data->data=new ems_u32[lvd_size];
        bcopy(p, is_data->data, lvd_size*sizeof(ems_u32));

        struct is_data_slot* slot=find_is_slot(ved_idx, IS_ID);
        if (slot==0)
            return -1;
        slot->add(is_data);

        p+=lvd_size;
        sev_size-=lvd_size;
    }
    if (sev_size!=0) {
        printf("parse_subevent_lvd: illegal size %d\n", sev_size);
        return -1;
    }

    return 0;
}
//---------------------------------------------------------------------------//
int
ems_data::parse_subevent_normal(ems_u32* sub_event, int ved_idx, ems_u32 ev_no,
    int trigno)
{
    ems_u32* p=sub_event;

    //dump_raw(p, 10, "parse_subevent_normal");

    int IS_ID=*p++;
    int sev_size=*p++;

    struct is_data* is_data=new struct is_data;
    is_data->size=sev_size;
    is_data->ev_no=ev_no;

    is_data->data=new ems_u32[sev_size];
    bcopy(p, is_data->data, sev_size*sizeof(ems_u32));

    struct is_data_slot* slot=find_is_slot(ved_idx, IS_ID);
    if (slot==0)
        return -1;
    slot->add(is_data);

    return 0;
}
//---------------------------------------------------------------------------//
int
ems_data::parse_subevent(ems_u32* subevent, int ved_idx, int evno, int trigno)
{
    int res;

    //dump_raw(subevent, 10, "parse_subevent");

    int IS_ID=subevent[0];

    int is_idx=ved_info.find_is_idx(ved_idx, IS_ID);
    //cout<<"parse_subevent: IS_ID="<<IS_ID<<" is_idx="<<is_idx<<endl;
    if (is_idx<0) {
        cout<<"events from unknown IS "<<IS_ID<<endl;
        return -1;
    }

    //cout<<"decoding_hints="<<ved_info.ved_infos[ved_idx].is_infos[is_idx].decoding_hints<<endl;
    if (ved_info.ved_infos[ved_idx].is_infos[is_idx].decoding_hints&1) {
        res=parse_subevent_lvd(subevent, ved_idx, evno, trigno);
    } else {
        res=parse_subevent_normal(subevent, ved_idx, evno, trigno);
    }
    
    return res;
}
//---------------------------------------------------------------------------//
int
ems_data::parse_event(ems_u32* event, int ved_idx)
{
    ems_u32* p=event;

    //dump_raw(p, 20, "parse_event");

    /*int ev_size=*p++;*/ p++;
    int evno=*p++;
    int trigno=*p++;
    int nr_subevents=*p++;

    for (int i=0; i<nr_subevents; i++) {
        int sev_size=p[1]+2;
        if (parse_subevent(p, ved_idx, evno, trigno)<0)
            return -1;
        p+=sev_size;
    }
    
    return 0;
}
//---------------------------------------------------------------------------//
int
ems_data::parse_cluster_events(const ems_cluster *cluster)
{
    ems_u32* p;
    int VED_ID;
    int nr_events;

    //dump_raw(cluster->data, 20, "parse_cluster_events");

    // skip header, options and flags
    p=cluster->data+4+cluster->data[3]+1;

    VED_ID=*p++;
    p++; // skip unused fragment_id
    nr_events=*p++;

    int ved_idx=ved_info.find_ved_idx(VED_ID);
    if (ved_idx<0) {
        cout<<"events from unknown VED "<<VED_ID<<endl;
        return -1;
    }

    for (int i=0; i<nr_events; i++) {
        int ev_size=*p+1;
        if (parse_event(p, ved_idx)<0)
            return -1;
        p+=ev_size;
    }

    return 0;
}
//---------------------------------------------------------------------------//
int
ems_data::parse_cluster_file(const ems_cluster *cluster, ems_file** file)
{
    ems_u32* p;
    char* name;
    ems_file* ef;

    // skip header and options
    p=cluster->data+4+cluster->data[3];

    ems_u32 flags=*p++;
    ems_u32 fragment_id=*p++;
    p=xdrstrcdup(&name, p);

    if (fragment_id==0) {
        ef=new ems_file();
        ef->name=name;
        ef->ctime=*p++;
        ef->mtime=*p++;
        ef->mode=*p++;
        parse_timestamp(cluster, &ef->tv);
        //cout<<"filename "<<name<<endl;
    } else {
        /* find existing file object with the same name */
        int i=0;
        while ((i<nr_files) && (files[i]->name==name))
            i++;
        if (i>=nr_files) {
            cout<<"cannot find previous fragments of file "<<name<<endl;
            free(name);
            return -1;
        }
        ef=files[i];
        if (fragment_id!=ef->fragment+1) {
            cout<<"file "<<name<<": fragment jumps from "<<ef->fragment
                <<" to "<<fragment_id<<endl;
            free(name);
            return -1;
        }
        ef->fragment=fragment_id;
        p+=3; // skip ctime, mtime, perm
    }
    free(name);

    p++; // skip redundant size info

    int osize=*p;
    char* s;
    p=xdrstrcdup(&s, p);
    if (fragment_id==0) {
        ef->content=string(s, osize);
    } else {
        ef->content.append(s, osize);
    }
    free(s);

    if (flags==1) 
        ef->complete=true;

    *file=ef;

    return fragment_id==0?0:1;
}
//---------------------------------------------------------------------------//
int
ems_data::parse_cluster_text(const ems_cluster *cluster, ems_text* text)
{
    ems_u32* p;
    int lnr;
    char* s;

    parse_timestamp(cluster, &text->tv);

    // skip header, options, flags, and fragment_id
    p=cluster->data+4+cluster->data[3]+2;

    text->nr_lines=*p++;
    if (text->nr_lines<1) {
        cout<<"empty text cluster"<<endl;
        return -1;
    }

    text->lines=new string[text->nr_lines];

    p=xdrstrcdup(&s, p);
    text->lines[0]=s;
    free(s);
    if (text->lines[0].substr(0, 5)=="Key: ") {
        text->key=text->lines[0].substr(5, string::npos);
    }

    for (lnr=1; lnr<text->nr_lines; lnr++) {
        p=xdrstrcdup(&s, p);
        text->lines[lnr]=s;
        free(s);
        
    }

    return 0;
}
//---------------------------------------------------------------------------//
int
ems_data::parse_cluster_ved_info(const ems_cluster *cluster, ems_ved_info* info)
{
    ems_u32* p;
    parse_timestamp(cluster, &info->tv);

    // skip header and options
    p=cluster->data+4+cluster->data[3];
    info->version=*p++;
    if (!(info->version&0x80000000)) {
        cout<<"old and unusable version of ved-info"<<endl;
        return -1;
    }
    info->version&=~0x80000000;
#if 0
    cout<<"ved info version "<<info->version<<endl;
#endif

    switch (info->version) {
    case 1:
        info->nr_veds=*p++;
        info->ved_infos=new ems_ved_info::ved_info[info->nr_veds];
        for (int i=0; i<info->nr_veds; i++) {
            info->ved_infos[i].VED_ID=*p++;
            info->ved_infos[i].nr_is=*p++;
            info->ved_infos[i].is_infos=
                    new ems_ved_info::is_info[info->ved_infos[i].nr_is];
            for (int j=0; j<info->ved_infos[i].nr_is; j++) {
	        int is = *p++;
                info->ved_infos[i].is_infos[j].IS_ID=is;
            }
        }
        info->valid=true;
        break;
    case 2:
        info->nr_veds=*p++;
        info->ved_infos=new ems_ved_info::ved_info[info->nr_veds];
        for (int i=0; i<info->nr_veds; i++) {
            info->ved_infos[i].VED_ID=*p++;
            int importance=*p++;
            info->ved_infos[i].nr_is=*p++;
            info->ved_infos[i].is_infos=
                    new ems_ved_info::is_info[info->ved_infos[i].nr_is];
            for (int j=0; j<info->ved_infos[i].nr_is; j++) {
	        int is = *p++;
                info->ved_infos[i].is_infos[j].IS_ID=is;
                info->ved_infos[i].is_infos[j].importance=importance;
            }
        }
        info->valid=true;
        break;
    case 3:
        info->nr_veds=*p++;
        info->ved_infos=new ems_ved_info::ved_info[info->nr_veds];
        for (int i=0; i<info->nr_veds; i++) {
            info->ved_infos[i].VED_ID=*p++;
            info->ved_infos[i].nr_is=*p++;
            info->ved_infos[i].is_infos=
                    new ems_ved_info::is_info[info->ved_infos[i].nr_is];
            for (int j=0; j<info->ved_infos[i].nr_is; j++) {
                info->ved_infos[i].is_infos[j].IS_ID=*p++;
                info->ved_infos[i].is_infos[j].importance=*p++;
                info->ved_infos[i].is_infos[j].decoding_hints=*p++;
#if 0
                cout<<"IS "<<info->ved_infos[i].is_infos[j].IS_ID
                    <<": decoding_hints="
                    <<info->ved_infos[i].is_infos[j].decoding_hints
                    <<endl;
#endif
            }
        }
        info->valid=true;
        break;
    default:
        cout<<"unknown version of ved info: "<<info->version<<endl;
        return -1;
    }
    return 0;
}
//---------------------------------------------------------------------------//
int
ems_data::parse_cluster_no_more_data(const ems_cluster *cluster)
{
    cout<<"no_more_data cluster received"<<endl;
    return 0;
}
//---------------------------------------------------------------------------//
// ems_data::parse_cluster parses the given cluster and copies the data into 
// its own storage. It is save to delete cluster immediate after that.
int
ems_data::parse_cluster(const ems_cluster *cluster)
{
    int res;

    switch (cluster->type) {
    case clusterty_events:
        res=parse_cluster_events(cluster);
        break;
    case clusterty_ved_info:
        // XXX
        // this logic does not work if a second clusterty_ved_info is
        // received!

        res=parse_cluster_ved_info(cluster, &ved_info);
        //ved_info.dump(0);

        ved_slots=new struct ved_slot[ved_info.nr_veds];
        for (int i=0; i<ved_info.nr_veds; i++) {
            ved_slots[i].is_data=new struct is_data_slot[ved_info.ved_infos[i].nr_is];
        }
        break;
    case clusterty_text: {
            ems_text* text=new ems_text();
            res=parse_cluster_text(cluster, text);
            if (res<0) {
                delete text;
            } else {
                ems_text** help=new ems_text*[nr_texts+1];
                for (int i=0; i<nr_texts; i++)
                    help[i]=texts[i];
                help[nr_texts]=text;
                delete[] texts;
                texts=help;
                nr_texts++;
            }
        }
        break;
    case clusterty_file: {
            ems_file* file;
            res=parse_cluster_file(cluster, &file);
            if (res==0) {
                ems_file** help=new ems_file*[nr_files+1];
                for (int i=0; i<nr_files; i++)
                    help[i]=files[i];
                help[nr_files]=file;
                delete[] files;
                files=help;
                nr_files++;
            }
        }
        break;
    case clusterty_no_more_data:
        res=parse_cluster_no_more_data(cluster);
        break;
    default:
        cout<<"unknown clustertype "<<(int)cluster->type<<endl;
        res=-1;
    }
    return res;
}
//---------------------------------------------------------------------------//
void
ems_data::clear_ved_flags(void)
{
    for (int i=0; i<ved_info.nr_veds; i++)
        ved_slots[i].required=false;
}
//***************************************************************************//
// is_data
ems_data::is_data::~is_data()
{
    delete[] data;
}
//***************************************************************************//
//is_data_slot

ems_data::is_data_slot::~is_data_slot()
{
    while (first) {
        struct is_data* help=first;
        first=first->next;
        delete help;
    }
}
//---------------------------------------------------------------------------//
void
ems_data::is_data_slot::add(struct is_data* data)
{
    if (first==0)
        first=data;
    else
        last->next=data;
    last=data;
    records++;
}
//---------------------------------------------------------------------------//
void
ems_data::is_data_slot::purge_before(ems_u32 ev_no)
{
    ems_u32 old;
    int purged=0;
    if (first)
        old=first->ev_no;
    while (first && first->ev_no<ev_no) {
        struct is_data* help=first;
        first=first->next;
        delete help;
        purged++;
    }
    if (purged>1) {
        cout<<purged<<" events removed, old="<<old<<" next="<<ev_no<<endl;
    }
}
//---------------------------------------------------------------------------//
void
ems_data::is_data_slot::purge_first()
{
    if (first) {
        struct is_data* help=first;
        first=first->next;
        delete help;
    }
}
//***************************************************************************//
static string
timestring(time_t t)
{
    static char ts[50];
    struct tm* tm;
    tm=localtime(&t);
    strftime(ts, 50, "%Y-%m-%d %H:%M:%S %Z", tm);
    return string(ts);
}
//***************************************************************************//
#if 0
//ems_subevent

void
ems_subevent::dump(int level) const
{
    cout<<"subevent "<<sev_id<<" size "<<size<<endl;
    for (unsigned int i=0; i<size; i+=8) {
        printf("%4d", i);
        unsigned int n=i+8;
        if (n>size)
            n=size;
        for (unsigned int j=i; j<n; j++) {
            printf(" %08x", data[j]);
        }
        printf("\n");
    }
}
#endif
//***************************************************************************//
//ems_file

ems_file::ems_file()
:fragment(0), complete(false)
{}
//---------------------------------------------------------------------------//
ems_file::~ems_file()
{}
//---------------------------------------------------------------------------//
void
ems_file::dump(int level) const
{
    cout<<"File: "<<name<<endl;
    if (level>0) {
        cout<<"time stamp       : "<<timestring(tv.tv_sec)<<endl;
        cout<<"creation time    : "<<timestring(ctime)<<endl;
        cout<<"modification time: "<<timestring(mtime)<<endl;
        cout<<"mode             : "<<oct<<showbase<<setw(8)<<setfill('0')
            <<mode<<dec<<noshowbase<<setfill(' ')<<endl;
        cout<<content<<endl;
    }
}
//***************************************************************************//
//ems_text

ems_text::ems_text(void)
:nr_lines(0), lines(0)
{}
//---------------------------------------------------------------------------//
ems_text::~ems_text()
{
    delete[] lines;
}
//---------------------------------------------------------------------------//
void
ems_text::dump(int level) const
{
    cout<<"Text: key="<<key<<", "<<nr_lines<<" lines"<<endl;
    if (level>0) {
        for (int i=0; i<nr_lines; i++)
            cout<<lines[i]<<endl;
    }
}
//***************************************************************************//
//ems_ved_info

ems_ved_info::ems_ved_info(void)
:valid(false), nr_veds(0), ved_infos(0)
{}
//---------------------------------------------------------------------------//
ems_ved_info::~ems_ved_info()
{
    delete[] ved_infos;
}
//---------------------------------------------------------------------------//
ems_ved_info::ved_info::~ved_info()
{
    delete[] is_infos;
}
//---------------------------------------------------------------------------//
int
ems_ved_info::find_ved_idx(int VED_ID) const
{
    for (int i=0; i<nr_veds; i++) {
        if (ved_infos[i].VED_ID==VED_ID)
            return i;
    }
    return -1;
}
//---------------------------------------------------------------------------//
int
ems_ved_info::find_ved_idx(const string name) const
{
    for (int i=0; i<nr_veds; i++) {
        if (ved_infos[i].name==name)
            return i;
    }
    return -1;
}
//---------------------------------------------------------------------------//
int
ems_ved_info::find_is_idx(int ved_idx, int IS_ID) const
{
    ved_info *ved_inf=ved_infos+ved_idx;
    int nr_is=ved_inf->nr_is;
    for (int i=0; i<nr_is; i++) {
        if (ved_inf->is_infos[i].IS_ID==IS_ID)
            return i;
    }
    return -1;
}
//---------------------------------------------------------------------------//
int
ems_ved_info::find_is(int IS_ID, int* ved_idx, int* is_idx) const
{
    for (int i=0; i<nr_veds; i++) {
        for (int j=0; j<ved_infos[i].nr_is; j++) {
            if (ved_infos[i].is_infos[j].IS_ID==IS_ID) {
                *ved_idx=i;
                *is_idx=j;
                return 0;
            }
        }
    }
    return -1;
}
//---------------------------------------------------------------------------//
void
ems_ved_info::dump(int level) const
{
    cout<<"VED info: version "<<version<<endl;
    for (int i=0; i<nr_veds; i++) {
        if (ved_infos[i].name!="")
            cout<<"  "<<ved_infos[i].name<<endl;
        cout<<"  VED "<<ved_infos[i].VED_ID;
        if (ved_infos[i].sync>=0)
            cout<<" sync="<<((ved_infos[i].sync>>3)&0xf)
                <<" "<<(ved_infos[i].sync&0x7);
        for (int j=0; j<ved_infos[i].nr_is; j++) {
            if (j>0)
                cout<<",";
            cout<<" IS "<<ved_infos[i].is_infos[j].IS_ID
                <<" hints="<<ved_infos[i].is_infos[j].decoding_hints;
            if (ved_infos[i].is_infos[j].importance)
                cout<<" ("<<ved_infos[i].is_infos[j].importance<<")";
        }
        cout<<endl;
    }
}
//***************************************************************************//
// ved_configuration

ved_configuration::ved_configuration(const ems_text* text_)
:next(0), text(text_), eventbuilder(0), triggermaster(0), num_isids(0)
{}
//---------------------------------------------------------------------------//
ved_configuration::~ved_configuration()
{
    if (num_isids)
        delete[] isids;
}
//---------------------------------------------------------------------------//
void
ved_configuration::add_isid(int idx, int id)
{
    struct isid* help=new struct isid[num_isids+1];
    if (num_isids) {
        for (int i=0; i<num_isids; i++)
            help[i]=isids[i];
        delete[] isids;
    }
    isids=help;
    isids[num_isids].idx=idx;
    isids[num_isids].ID=id;
    num_isids++;
}
//---------------------------------------------------------------------------//
void
ved_configuration::dump(int level) const
{
    cout<<"ved_configuration "<<name<<endl;
    cout<<"  vedname="<<vedname<<endl;
    cout<<"  eventbuilder="<<eventbuilder<<endl;
    cout<<"  triggermaster="<<triggermaster<<endl;
    for (int i=0; i<num_isids; i++) {
        cout<<"  isid["<<isids[i].idx<<"]="<<isids[i].ID<<endl;
    }
}
//***************************************************************************//
// ved_configurations

ved_configurations::~ved_configurations()
{
    while (confs) {
        ved_configuration* help=confs->next;
        delete confs;
        confs=help;
    }
}
//---------------------------------------------------------------------------//
void
ved_configurations::add_conf(ved_configuration* conf)
{
    conf->next=confs;
    confs=conf;
}
//***************************************************************************//
// sync_connections

sync_connections::sync_connections(const ems_file *file_)
:file(file_), num_conn(0), conn(0)
{}
//---------------------------------------------------------------------------//
sync_connections::~sync_connections()
{
    delete[] conn;
}
//---------------------------------------------------------------------------//
void
sync_connections::add_conn(string crate_name, int addr, int port)
{
    struct conn* help=new struct conn[num_conn+1];
    for (int i=0; i<num_conn; i++)
        help[i]=conn[i];
    delete[] conn;
    conn=help;
    conn[num_conn].crate_name=crate_name;
    conn[num_conn].addr=addr;
    conn[num_conn].port=port;
    num_conn++;
}
//***************************************************************************//
// hist

void
hist::add(double v)
{
    num++;
    sum+=v;
    qsum+=v*v;
}
//---------------------------------------------------------------------------//
void
hist::dump(void) const
{
    double average, sigma;
    if (num<1)
        average=0;
    else
        average=sum/num;

    if (num<2)
        sigma=0;
    else
        sigma=sqrt((num*qsum-sum*sum)/(num*(num-1)));
    cout<<"num="<<num<<" mean="<<average<<" sigma="<<sigma;
}
//---------------------------------------------------------------------------//
//***************************************************************************//
//***************************************************************************//
