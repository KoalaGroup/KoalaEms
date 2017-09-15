/*
 * ems/events++/cluster_data.cc
 * 
 * created 2006-Apr-28 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <errno.h>
#include "cluster_data.hxx"
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

ems_data::ems_data(enum modus modus_)
:nr_texts(0), texts(0), nr_files(0), files(0), nr_skip_is(0), skip_is(0), 
allow_uncomplete(0), purged_uncomplete(0), modus(modus_),
smallest_ev_no(0), last_complete(0), /*nr_IS(0), IS_IDs(0), nr_VED(0),
VED_IDs(0),*/ events(0), last_event(0), last_ved_event(0)
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
    //delete[] IS_IDs;
    //delete[] VED_IDs;
    while (events) {
        event_info *help=events->next;
        delete events;
        events=help;
    }
    delete[] last_ved_event;
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
ems_data::event_info*
ems_data::find_ev_info(ems_u32 ev_no, int ved_idx)
{
    event_info *ev_info;

    if (last_ved_event[ved_idx])
        ev_info=last_ved_event[ved_idx];
    else
        ev_info=events;
    while (ev_info && (ev_info->ev_no<ev_no)) {
        ev_info=ev_info->next;
    }
    if ((ev_info!=0) && (ev_info->ev_no==ev_no)) {
        return ev_info;
    } else {
        return 0;
    }
}
//---------------------------------------------------------------------------//
#if 0
int
ems_data::find_ved_idx(int VED_ID)
{
    for (int i=0; i<nr_VED; i++) {
        if (VED_IDs[i]==VED_ID)
            return i;
    }
    return -1;
}
#else
int
ems_ved_info::find_ved_idx(int VED_ID)
{
    for (int i=0; i<nr_veds; i++) {
        if (ved_infos[i].VED_ID==VED_ID)
            return i;
    }
    return -1;
}
#endif
//---------------------------------------------------------------------------//
int
ems_ved_info::find_is_idx(int ved_idx, int IS_ID)
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
ems_data::event_info*
ems_data::get_ev_info(int ev_no, int trigno, int ved_idx)
{
    event_info* ev_info;
    ems_event* event;

    ev_info=find_ev_info(ev_no, ved_idx);
    if (ev_info==0) {
        event=new ems_event;
        event->event_nr=ev_no;
        event->trigger=trigno;
        event->nr_subevents=0;
        event->subevents=0;

        ev_info=new event_info(ved_info.nr_veds, ev_no);
        if (ev_info==0) {
            cerr<<"new event_info for event "<<ev_no<<": "
                <<strerror(errno)<<endl;
            return 0;
        }
        for (int j=0; j<ved_info.nr_veds; j++) 
            ev_info->veds[j]=ved_info.ved_infos[j].skip;
        ev_info->event=event;

        if (last_event)
            last_event->next=ev_info;
        else
            events=ev_info;
        last_event=ev_info;
    }
    return ev_info;
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

    event_info* ev_info;
    ems_event* event;

    while (sev_size>=3) {
        int lvd_size=(p[0]&0xffff)/4;
        //int lvd_time=p[1];
        ems_u32 lvd_evno=p[2];

        if (lvd_evno<smallest_ev_no) {
            // skip this event (because it is too old)
//          cerr<<"unexpected event number "<<lvd_evno<<" (smaller than "
//              <<smallest_ev_no<<")"<<endl;
        } else {
            ev_info=get_ev_info(lvd_evno, trigno, ved_idx);
            if (ev_info==0)
                return -1;
            event=ev_info->event;

            last_ved_event[ved_idx]=ev_info;

            ev_info->veds[ved_idx]=true;
            bool complete=true;
            if (modus==sorted) {
                for (int j=0; j<ved_info.nr_veds; j++) {
                    if (!ev_info->veds[j])
                        complete=false;
                }
            }

            if (complete) {
                ev_info->complete=complete;
                last_complete=ev_info->ev_no;
            }

            ems_subevent* subevent=new ems_subevent;
            subevent->next=0;
            subevent->sev_id=IS_ID;
            subevent->size=lvd_size+1;
            subevent->data=new ems_u32[lvd_size+1];
            subevent->data[0]=lvd_size;
            bcopy(p, subevent->data+1, lvd_size*sizeof(ems_u32));

            if (ev_info->last_subevent)
                ev_info->last_subevent->next=subevent;
            else
                event->subevents=subevent;
            ev_info->last_subevent=subevent;
            event->nr_subevents++;
        }
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

    event_info* ev_info;
    ems_event* event;

#if 0
    if (ev_no<smallest_ev_no) {
        cerr<<"unexpected event number "<<ev_no<<" (smaller than "
            <<smallest_ev_no<<")"<<endl;
	return 0;
    }
#endif

    ev_info=get_ev_info(ev_no, trigno, ved_idx);
    if (ev_info==0)
        return -1;
    event=ev_info->event;

    last_ved_event[ved_idx]=ev_info;

    ev_info->veds[ved_idx]=true;
    bool complete=true;
    if (modus==sorted) {
        for (int j=0; j<ved_info.nr_veds; j++) {
            if (!ev_info->veds[j])
                complete=false;
        }
    }
    if (complete) {
        ev_info->complete=complete;
        last_complete=ev_info->ev_no;
    }

    ems_subevent* subevent=new ems_subevent;
    subevent->next=0;
    subevent->sev_id=IS_ID;
    subevent->size=sev_size;
    subevent->data=new ems_u32[sev_size];
    bcopy(p, subevent->data, sev_size*sizeof(ems_u32));
    //printf("subevent saved at %p size %d\n", subevent->data, sev_size);

    if (ev_info->last_subevent)
        ev_info->last_subevent->next=subevent;
    else
        event->subevents=subevent;
    ev_info->last_subevent=subevent;
    event->nr_subevents++;

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
        cerr<<"events from unknown IS "<<IS_ID<<endl;
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
        cerr<<"events from unknown VED "<<VED_ID<<endl;
        return -1;
    }

    if (ved_info.ved_infos[ved_idx].skip)
        return 0;

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
        //cerr<<"filename "<<name<<endl;
    } else {
        /* find existing file object with the same name */
        int i=0;
        while ((i<nr_files) && (files[i]->name==name))
            i++;
        if (i>=nr_files) {
            cerr<<"cannot find previous fragments of file "<<name<<endl;
            free(name);
            return -1;
        }
        ef=files[i];
        if (fragment_id!=ef->fragment+1) {
            cerr<<"file "<<name<<": fragment jumps from "<<ef->fragment
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
    if (!(info->version&0x80000000)) {
        cerr<<"old and unusable version of ved-info"<<endl;
        return -1;
    }
    info->version&=~0x80000000;
    cout<<"ved info version "<<info->version<<endl;

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
	        for (int c=0; c<nr_skip_is; c++) {
		    if (is == skip_is[c]) {
		        cerr << "Skipping VED " << info->ved_infos[i].VED_ID
		            << " with IS " << is << endl;
		        info->ved_infos[i].skip = true;
                        break;
                    }
                }
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
	        for (int c=0; c<nr_skip_is; c++) {
		    if (is == skip_is[c]) {
		        cerr << "Skipping VED " << info->ved_infos[i].VED_ID
		            << " with IS " << is << endl;
		        info->ved_infos[i].skip = true;
                        break;
                    }
                }
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
                cout<<"IS "<<info->ved_infos[i].is_infos[j].IS_ID
                    <<": decoding_hints="
                    <<info->ved_infos[i].is_infos[j].decoding_hints
                    <<endl;
	        int is = info->ved_infos[i].is_infos[j].IS_ID;
	        for (int c=0; c<nr_skip_is; c++) {
		    if (is == skip_is[c]) {
		        cerr << "Skipping VED " << info->ved_infos[i].VED_ID
		            << " with IS " << is << endl;
		        info->ved_infos[i].skip = true;
                        break;
                    }
                }
            }
        }
        info->valid=true;
        break;
    default:
        cerr<<"unknown version of ved info: "<<info->version<<endl;
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
        res=parse_cluster_ved_info(cluster, &ved_info);
        ved_info.dump(0);
#if 0
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
#endif
        last_ved_event=new event_info*[ved_info.nr_veds];
        for (int i=0; i<ved_info.nr_veds; i++)
                last_ved_event[i]=0;
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
void
ems_data::delist_fist_event(void)
{
    event_info* help=events;

    events=events->next; // second element will become the root of the list
    if (events)
        smallest_ev_no=events->ev_no;
    else
        smallest_ev_no=help->ev_no+1;

    for (int i=0; i<ved_info.nr_veds; i++) {
        if (last_ved_event[i]==help)
            last_ved_event[i]=0;
    }
    if (last_event==help)
        last_event=0;

    delete help; // delete previous first element of linked list
}
//---------------------------------------------------------------------------//
void
ems_data::purge_uncomplete(void)
{
    // purge uncomplete events
    while (events && !events->complete && events->ev_no<last_complete) {
        //cerr<<"purge uncomplete event "<<events->ev_no<<endl;
        delist_fist_event();
        purged_uncomplete++;
    }
}
//---------------------------------------------------------------------------//
bool
ems_data::events_available(void)
{
    purge_uncomplete();
    return events && events->complete;
}
//---------------------------------------------------------------------------//
int
ems_data::get_event(ems_event** eventp)
{
    purge_uncomplete();

    if (!events)
        return 0;

    if (!events->complete && allow_uncomplete>0) {
        if (last_event && (last_event->ev_no - last_complete)>allow_uncomplete) {
            // mark all skippable(?) subevents as existing
            // and recompute completeness
            event_info* event = events;
            while (event) {
                bool complete=true;
                for (int i=0; i<ved_info.nr_veds; i++) {
                    event->veds[i] |= ved_info.ved_infos[i].skip;
                    if (!event->veds[i])
                        complete=false;
                }
                event->complete=complete;
                if (complete && last_complete<event->ev_no)
                    last_complete=event->ev_no;
                event = event->next;
            }
	}
    }
  
    if (events->complete) {
        *eventp=events->event;
        events->event=0;
        delist_fist_event();
        return 1;
    } else {
        return 0;
    }
}
//---------------------------------------------------------------------------//
ems_data::event_info::event_info(int nr_VED, int _ev_no)
:next(0), complete(false), veds(0), ev_no(_ev_no),
last_subevent(0), event(0)
{
    veds=new bool[nr_VED];
}
//---------------------------------------------------------------------------//
ems_data::event_info::~event_info()
{
    if (event)
        delete event;
    delete[] veds;
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
//---------------------------------------------------------------------------//
void
ems_event::dump(int level) const
{
    cerr<<"event "<<event_nr<<" trigger "<<trigger<<endl;
    if (level>0) {
        cerr<<nr_subevents<<" subevents:";
        ems_subevent* sev=subevents;
        while (sev) {
            cerr<<" "<<sev->sev_id;
            sev=sev->next;
        }
        cerr<<endl;
        sev=subevents;
        while (sev) {
            sev->dump(level);
            sev=sev->next;
        }
    }
}
//***************************************************************************//
//ems_subevent

void
ems_subevent::dump(int level) const
{
    cerr<<"subevent "<<sev_id<<" size "<<size<<endl;
    for (unsigned int i=0; i<size; i+=8) {
        fprintf(stderr, "%4d", i);
        unsigned int n=i+8;
        if (n>size)
            n=size;
        for (unsigned int j=i; j<n; j++) {
            fprintf(stderr, " %08x", data[j]);
        }
        fprintf(stderr, "\n");
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
{
    delete[] lines;
}
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
    cerr<<"VED info: version "<<version<<endl;
    for (int i=0; i<nr_veds; i++) {
        cerr<<"  VED "<<ved_infos[i].VED_ID<<"  skip="<<ved_infos[i].skip<<endl;
        for (int j=0; j<ved_infos[i].nr_is; j++) {
            cerr<<"    IS "<<ved_infos[i].is_infos[j].IS_ID
                <<" hints="<<ved_infos[i].is_infos[j].decoding_hints<<endl;
            if (ved_infos[i].is_infos[j].importance)
                cerr<<" ("<<ved_infos[i].is_infos[j].importance<<")";
            cerr<<endl;
        }
    }
}
//***************************************************************************//
//***************************************************************************//
