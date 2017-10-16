/*
 * ems/events++/cluster_data_timed.cc
 * 
 * created 2006-Nov-14-28 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <errno.h>
#include "cluster_data_timed.hxx"
#include <xdrstring.h>
#include <versions.hxx>

VERSION("22006-Nov-14-28-02", __FILE__, __DATE__, __TIME__,
"$ZEL: cluster_data_timed.cc,v 1.1 2007/02/25 19:52:33 wuestner Exp $")
#define XVERSION

//***************************************************************************//
//ems_data

ems_data::ems_data(void)
:nr_texts(0), texts(0), nr_files(0), files(0),
resols(0), num_resols(0),
nr_IS(0), IS_IDs(0), nr_VED(0),
VED_IDs(0), ved_slots(0)
{}
//---------------------------------------------------------------------------//
ems_data::~ems_data()
{
    delete[] texts;
    delete[] files;
    delete[] IS_IDs;
    delete[] VED_IDs;
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
int
ems_data::find_ved_idx(int VED_ID)
{
    for (int i=0; i<nr_VED; i++) {
        if (VED_IDs[i]==VED_ID)
            return i;
    }
    return -1;
}
//---------------------------------------------------------------------------//
int
ems_data::parse_cluster_events(const ems_cluster *cluster)
{
    ems_u32* p;
    int VED_ID;
    ems_u32 nr_events, ev_size, ev_no, ev_trigno, nr_subevents;

    // skip header, options and flags
    p=cluster->data+4+cluster->data[3]+1;
    VED_ID=*p++;
    p++; // skip unused fragment_id

    int ved_idx=find_ved_idx(VED_ID);
    if (ved_idx<0) {
        cerr<<"events from unknown VED "<<VED_ID<<endl;
        return -1;
    }

    nr_events=*p++;
    for (ems_u32 i=0; i<nr_events; i++) {
        ev_size=*p++;
        ev_no=*p++;
        ev_trigno=*p++;
        nr_subevents=*p++;

        ved_subevents *vsevents=new ved_subevents;
        vsevents->ev_no=ev_no;
        vsevents->trigger=ev_trigno;
        vsevents->nr_subevents=nr_subevents;

        for (ems_u32 j=0; j<nr_subevents; j++) {
            unsigned int IS_ID=*p++;
            int size=*p++;
            ems_u32* data=p;
            p+=size;

            ems_subevent* subevent=new ems_subevent;
            subevent->sev_id=IS_ID;
            subevent->size=size;
            subevent->data=new ems_u32[size];
            bcopy(data, subevent->data, size*sizeof(ems_u32));

            if (ved_slots[ved_idx].trigger_is==IS_ID)
                vsevents->triggertime=subevent->data[2];

            if (vsevents->last_subevent)
                vsevents->last_subevent->next=subevent;
            else
                vsevents->subevents=subevent;
            vsevents->last_subevent=subevent;
        }

        ved_slots[ved_idx].insert_subevent(vsevents);
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
    } else {
        /* find existing file object with the same name */
        int i=0;
        while ((i<nr_files) && (files[i]->name==name))
            i++;
        if (i>=nr_files) {
            cerr<<"cannot find previous fragments of file "<<name<<endl;
            return -1;
        }
        ef=files[i];
        if (fragment_id!=ef->fragment+1) {
            cerr<<"file "<<name<<": fragment jumps from "<<ef->fragment
                <<" to "<<fragment_id<<endl;
            return -1;
        }
        ef->fragment=fragment_id;
        p+=3; // skip ctime, mtime, perm
    }
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
        cerr<<"empty text cluster"<<endl;
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

    switch (info->version) {
    case 0x80000001:
        info->nr_veds=*p++;
        info->ved_infos=new ems_ved_info::ved_info[info->nr_veds];
        for (int i=0; i<info->nr_veds; i++) {
            info->ved_infos[i].VED_ID=*p++;
            info->ved_infos[i].nr_is=*p++;
            info->ved_infos[i].is_infos=
                    new ems_ved_info::is_info[info->ved_infos[i].nr_is];
            for (int j=0; j<info->ved_infos[i].nr_is; j++) {
                info->ved_infos[i].is_infos[j].IS_ID=*p++;
                info->ved_infos[i].is_infos[j].importance=0;
            }
        }
        info->valid=true;
        break;
    case 0x80000002:
        cerr<<"ved info version 2 not yet implemented"<<endl;
        return -1;
        break;
    case 0x80000003:
        cerr<<"ved info version 3 not yet implemented"<<endl;
        return -1;
        break;
    default:
        cerr<<"unknown version of ved info: "<<hex<<showbase<<setw(8)
            <<setfill('0')<<info->version<<dec<<noshowbase<<setfill(' ')<<endl;
        return -1;
    }
    return 0;
}
//---------------------------------------------------------------------------//
int
ems_data::parse_cluster_no_more_data(const ems_cluster *cluster)
{
    cerr<<"no_more_data cluster received"<<endl;
    return 0;
}
//---------------------------------------------------------------------------//

int
ems_data::parse_cluster(const ems_cluster *cluster)
{
    int res;

    switch (cluster->type) {
    case clusterty_events:
        res=parse_cluster_events(cluster);
        break;
    case clusterty_ved_info:
        res=parse_cluster_ved_info(cluster, &ved_info);
        //ved_info.dump(0);
        nr_VED=ved_info.nr_veds;
        nr_IS=0;
        VED_IDs=new int[nr_VED];
        for (int i=0; i<nr_VED; i++) {
            VED_IDs[i]=ved_info.ved_infos[i].VED_ID;
            nr_IS+=ved_info.ved_infos[i].nr_is;
        }
        IS_IDs=new int[nr_IS];
        for (int i=0, is=0; i<nr_VED; i++) {
            for (int j=0; j<ved_info.ved_infos[i].nr_is; j++) {
                IS_IDs[is++]=ved_info.ved_infos[i].is_infos[j].IS_ID;
            }
        }
        ved_slots=new ved_slot[nr_VED];
        for (int i=0; i<nr_VED; i++)
            ved_slots[i].parent=this;
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
        cerr<<"unknown clustertype "<<(int)cluster->type<<endl;
        res=-1;
    }
    return res;
}
//---------------------------------------------------------------------------//
int
ems_data::get_event(ems_event** eventp)
{
    // check whether at least two ved_subevents are available for each VED
    for (int i=0; i<nr_VED; i++) {
        if (ved_slots[i].num_vsubevents<0)
            return 0;
    }

    // check whether all tdiffs are nearly equal
    bool ok=true;
#if 0
    double tdiff0=ved_slots[0].vsubevents->tdiff;
    for (int i=1; i<nr_VED; i++) {
        double tdiff1=ved_slots[i].vsubevents->tdiff;
        if (fabs(tdiff1-tdiff0)>...)
            ok=false;
    }
#endif
    if (!ok) {
        if (resync()<0)
            return 0;
    }
    ems_event* event=new ems_event;
    ved_subevents *vsubevent=ved_slots[0].extract_subevent();
    ved_subevents *last_vsubevent=vsubevent;
    ems_subevent *last_subevent=vsubevent->last_subevent;
    event->vsubevents=vsubevent;
    event->subevents=vsubevent->subevents;
    event->nr_subevents=vsubevent->nr_subevents;
    for (int i=1; i<nr_VED; i++) {
        vsubevent=ved_slots[i].extract_subevent();
        last_vsubevent->next=vsubevent;
        last_vsubevent=vsubevent;
        last_subevent->next=vsubevent->subevents;
        last_subevent=vsubevent->last_subevent;
        event->nr_subevents+=vsubevent->nr_subevents;
    }
    last_vsubevent->next=0;
    last_subevent->next=0;
    
    event->event_nr=event->vsubevents->ev_no;
    event->trigger=event->vsubevents->trigger;
    *eventp=event;
    return 1;
}
//---------------------------------------------------------------------------//
ved_subevents *
ems_data::ved_slot::extract_subevent(void)
{
    if ((vsubevents==0) || (num_vsubevents==0)) {
        cerr<<"program error in ems_data::ved_slot::extract_subevent"<<endl;
        exit(100);
    }
    ved_subevents *vsubevent=vsubevents;
    vsubevents=vsubevent->next;
    num_vsubevents--;
    vsubevent->next=0;
    return vsubevent;
}
//---------------------------------------------------------------------------//
int
ems_data::ved_slot::find_trigger_is(ved_subevents *vsubevent)
{
    ems_subevent *sev=vsubevent->subevents;
    for (unsigned int i=0; i<vsubevent->nr_subevents; i++) {
        for (int j=0; j<parent->num_resols; j++) {
            if (sev->sev_id==parent->resols[j].isId) {
                resolution=parent->resols[j].resolution;
                trigger_is=parent->resols[j].isId;
                vsubevent->triggertime=sev->data[2];
                return 0;
            }
        }
    }
    cerr<<"find_trigger_is: no resolution found"<<endl;
    return -1;
}
//---------------------------------------------------------------------------//
int
ems_data::ved_slot::insert_subevent(ved_subevents *vsubevent)
{
    if (last_vsubevents==0) {
        if (find_trigger_is(vsubevent)<0)
            return -1;
        vsubevent->tdiff=0.;
    } else {
        last_vsubevents->next=vsubevent;
        vsubevent->tdiff=triggerdiff(
                vsubevent->triggertime, last_vsubevents->triggertime);
    }
    last_vsubevents=vsubevent;
    num_vsubevents++;
    return 0;
}
//---------------------------------------------------------------------------//
// returns difference of trigger times in us
double
ems_data::ved_slot::triggerdiff(ems_u32 a, ems_u32 b)
{
    double ah=(a>>16)&0xffff;
    double bh=(b>>16)&0xffff;
    double al=a&0xffff;
    double bl=b&0xffff;
    double diff=(ah-bh)*6.4;
    diff+=(al-bl)*resolution;
    return diff;
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
//ems_event

ems_event::~ems_event()
{
    while (subevents) {
        ems_subevent* help=subevents->next;
        delete subevents;
        subevents=help;
    }
}
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
    cerr<<"File: "<<name<<endl;
    if (level>0) {
        cerr<<"time stamp       : "<<timestring(tv.tv_sec)<<endl;
        cerr<<"creation time    : "<<timestring(ctime)<<endl;
        cerr<<"modification time: "<<timestring(mtime)<<endl;
        cerr<<"mode             : "<<oct<<showbase<<setw(8)<<setfill('0')
            <<mode<<dec<<noshowbase<<setfill(' ')<<endl;
        cerr<<content<<endl;
    }
}
//***************************************************************************//
//ems_text

ems_text::ems_text(void)
:nr_lines(0), lines(0)
{}
//---------------------------------------------------------------------------//
ems_text::~ems_text()
{}
//---------------------------------------------------------------------------//
void
ems_text::dump(int level) const
{
    cerr<<"Text: key="<<key<<", "<<nr_lines<<" lines"<<endl;
    if (level>0) {
        for (int i=0; i<nr_lines; i++)
            cerr<<lines[i]<<endl;
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
    for (int i=0; i<nr_veds; i++) {
        delete[] ved_infos[i].is_infos;
    }
    delete[] ved_infos;
}
//---------------------------------------------------------------------------//
void
ems_ved_info::dump(int level) const
{
    cerr<<"VED info:"<<endl;
    for (int i=0; i<nr_veds; i++) {
        cerr<<"  VED "<<ved_infos[i].VED_ID<<endl;
        for (int j=0; j<ved_infos[i].nr_is; j++) {
            cerr<<"    IS "<<ved_infos[i].is_infos[j].IS_ID<<endl;
            if (ved_infos[i].is_infos[j].importance)
                cerr<<" ("<<ved_infos[i].is_infos[j].importance<<")";
            cerr<<endl;
        }
    }
}
//***************************************************************************//
//***************************************************************************//
