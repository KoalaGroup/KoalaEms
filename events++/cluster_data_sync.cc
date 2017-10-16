/*
 * ems/events++/cluster_data_sync.cc
 * 
 * created 2007-Feb-16 PW
 */

#include "config.h"
#include <limits.h>
#include "cxxcompat.hxx"

#include <errno.h>
#include "cluster_data_sync.hxx"
#include <xdrstring.h>
#include <versions.hxx>

VERSION("2007-Feb-16", __FILE__, __DATE__, __TIME__,
"$ZEL: cluster_data_sync.cc,v 1.1 2007/02/25 19:52:33 wuestner Exp $")
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
//C_ems_data

C_ems_data::C_ems_data(void)
    :nr_texts(0),
    texts(0),
    nr_files(0),
    files(0),
    max_fifodepth(1000),
    nr_is(0),
    fifo(0)
{}
//---------------------------------------------------------------------------//
C_ems_data::~C_ems_data()
{
    for (int i=0; i<nr_texts; i++)
        delete texts[i];
    delete[] texts;
    for (int i=0; i<nr_files; i++)
        delete files[i];
    delete[] files;
    delete[] fifo;
}
//---------------------------------------------------------------------------//
int
C_ems_data::parse_timestamp(const ems_cluster *cluster, struct timeval *tv)
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
C_ems_data::disable_empty_fifos(void)
{
    int n=0;
    cerr<<"try to disable empty fifos"<<endl;
    for (int i=0; i<nr_is; i++) {
        cerr<<"  IS "<<fifo[i].is_id<<" depth="<<fifo[i].depth()
            <<" ignore="<<fifo[i].ignore<<endl;
        if (fifo[i].ignore) {
            cerr<<"  IS "<<fifo[i].is_id<<" already ignored"<<endl;
        } else if (fifo[i].depth()<min_fifodepth) {
            fifo[i].ignore=true;
            fifo[i].skip=true;
            n++;
            cerr<<"  IS "<<fifo[i].is_id<<" now ignored"<<endl;
        }
    }
    if (n==0) {
        cerr<<"  no empty fifo found"<<endl;
        return -1;
    }
    return 0;
}
//---------------------------------------------------------------------------//
int
C_ems_data::parse_subevent_lvd(ems_u32* sub_event, int ved_idx, ems_u32 ev_no,
    ems_u32 trigno)
{
    ems_u32* p=sub_event;

    //dump_raw(p, 10, "parse_subevent_lvd");

    ems_u32 IS_ID=*p++;
    int sev_size=*p++;

    int is_index=find_is_index(IS_ID);
    if (is_index<0) {
        cerr<<"parse_subevent_lvd: unknown IS_ID "<<IS_ID<<endl;
        return -1;
    }

    while (sev_size>=3) {
        int lvd_size=(p[0]&0xffff)/4;

        ems_subevent* subevent=new ems_subevent;
        subevent->ev_nr=p[2];
        subevent->trigger_id=trigno;
        subevent->sev_id=IS_ID;
        subevent->size=lvd_size+1;
        subevent->data=new ems_u32[lvd_size+1];
        subevent->data[0]=lvd_size;
        bcopy(p, subevent->data+1, lvd_size*sizeof(ems_u32));

        if (fifo[is_index].add(subevent)>0) {
            if (fifo[is_index].depth()>=max_fifodepth) {
                cerr<<"overfull fifo for IS "<<fifo[is_index].is_id<<endl;
                if (disable_empty_fifos()<0)
                    return -1;
            }
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
C_ems_data::parse_subevent_normal(ems_u32* sub_event, int ved_idx, ems_u32 ev_no,
    int trigno)
{
    ems_u32* p=sub_event;

    //dump_raw(p, 10, "parse_subevent_normal");

    int IS_ID=*p++;
    int sev_size=*p++;

    int is_index=find_is_index(IS_ID);
    if (is_index<0) {
        cerr<<"parse_subevent_lvd: unknown IS_ID "<<IS_ID<<endl;
        return -1;
    }

    if (fifo[is_index].skip)
        return 0;

    ems_subevent* subevent=new ems_subevent;
    subevent->ev_nr=ev_no;
    subevent->trigger_id=trigno;
    subevent->sev_id=IS_ID;
    subevent->size=sev_size;
    subevent->data=new ems_u32[sev_size];
    bcopy(p, subevent->data, sev_size*sizeof(ems_u32));

    fifo[is_index].ignore=false;
    if (fifo[is_index].add(subevent)>0) {
        if (fifo[is_index].depth()>=max_fifodepth) {
            cerr<<"overfull fifo for IS "<<fifo[is_index].is_id<<endl;
            if (disable_empty_fifos()<0)
                return -1;
        }
    }

    return 0;
}
//---------------------------------------------------------------------------//
int
C_ems_data::parse_subevent(ems_u32* subevent, int ved_idx, int evno, int trigno)
{
    int res;

    //dump_raw(subevent, 10, "parse_subevent");

    int IS_ID=subevent[0];

    int is_idx=ved_info.ved_infos[ved_idx].find_is_idx(IS_ID);
    if (is_idx<0) {
        cerr<<"events from unknown IS "<<IS_ID<<endl;
        return -1;
    }

    if (ved_info.ved_infos[ved_idx].is_infos[is_idx].decoding_hints&1) {
        res=parse_subevent_lvd(subevent, ved_idx, evno, trigno);
    } else {
        res=parse_subevent_normal(subevent, ved_idx, evno, trigno);
    }
    
    return res;
}
//---------------------------------------------------------------------------//
int
C_ems_data::parse_event(ems_u32* event, int ved_idx)
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
C_ems_data::parse_cluster_events(const ems_cluster *cluster)
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
C_ems_data::parse_cluster_file(const ems_cluster *cluster, ems_file** file)
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
C_ems_data::parse_cluster_text(const ems_cluster *cluster, ems_text* text)
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
C_ems_data::parse_cluster_ved_info(const ems_cluster *cluster, ems_ved_info* info)
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
            }
        }
        break;
    default:
        cerr<<"unknown version of ved info: "<<info->version<<endl;
        return -1;
    }
    info->valid=true;

    return 0;
}
//---------------------------------------------------------------------------//
int
C_ems_data::parse_cluster_no_more_data(const ems_cluster *cluster)
{
    cerr<<"no_more_data cluster received"<<endl;
    return 0;
}
//---------------------------------------------------------------------------//
int
C_ems_data::find_is_index(ems_u32 is_id) const
{
    for (int i=0; i<nr_is; i++) {
        if (fifo[i].is_id==is_id)
            return i;
    }
    return -1;
}
//---------------------------------------------------------------------------//
// C_ems_data::parse_cluster parses the given cluster and copies the data into 
// its own storage. It is save to delete cluster immediate after that.
int
C_ems_data::parse_cluster(const ems_cluster *cluster)
{
    int res;

    switch (cluster->type) {
    case clusterty_events:
        res=parse_cluster_events(cluster);
        break;
    case clusterty_ved_info:
        res=parse_cluster_ved_info(cluster, &ved_info);
        ved_info.dump(0);

        nr_is=0;
        for (int i=0; i<ved_info.nr_veds; i++) {
            nr_is+=ved_info.ved_infos[i].nr_is;
        }
        cerr<<"nr_is="<<nr_is<<endl;
        fifo=new ems_fifo[nr_is];
        for (int i=0, is=0; i<ved_info.nr_veds; i++) {
            for (int j=0; j<ved_info.ved_infos[i].nr_is; j++) {
                cerr<<"is_ids["<<is<<"]="<<ved_info.ved_infos[i].is_infos[j].IS_ID<<endl;
                fifo[is++].is_id=ved_info.ved_infos[i].is_infos[j].IS_ID;
            }
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
        cerr<<"unknown clustertype "<<(int)cluster->type<<endl;
        res=-1;
    }
    return res;
}
//---------------------------------------------------------------------------//
int
C_ems_data::depth(void) const
{
    int N=INT_MAX, n, nn=0;

    if (nr_is==0)
        return 0;

    for (int i=0; i<nr_is; i++) {
        if (!fifo[i].ignore) {
            nn++;
            n=fifo[i].depth();
            if (n<N)
                N=n;
        }
    }
    if (nn==0) {
        cerr<<"C_ems_data::depth: no valid fifo"<<endl;
        for (int i=0; i<nr_is; i++) {
            cerr<<"IS "<<fifo[i].is_id<<" depth="<<fifo[i].depth()
                <<" ignore="<<fifo[i].ignore<<endl;
        }
        return -1;
    }
    return N;
}
//---------------------------------------------------------------------------//
bool
C_ems_data::events_available(void) const
{
    return depth()>=min_fifodepth;
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
//FIFO
/*
 * first_used==-1: ring is empty (first_free is undefined)
 * first_used==first_free: ring is full
 */
template<class Q> FIFO<Q>::FIFO()
:ringmask(0xff),
 first_used(-1),
 _last(0)
{
    ring=new Q*[ringmask+1];
}
//---------------------------------------------------------------------------//
template<class Q> FIFO<Q>::~FIFO()
{
    if (first_used>=0) {
        int end=first_free>first_used?first_free:first_free+(ringmask+1);
        for (int i=first_used; i<end; i++) {
            delete ring[i&ringmask];
        }
    }
    delete[] ring;
}
//---------------------------------------------------------------------------//
template<class Q> void
FIFO<Q>::dump(const char* text) const
{
    cerr<<"fifo("<<text<<") this="<<this<<": ring="<<ring
        <<" mask=0x"<<hex<<ringmask<<dec<<endl
        <<"               first_used="<<first_used
        <<" first_free="<<first_free<<endl;
}
//---------------------------------------------------------------------------//
template<class Q> Q*
FIFO<Q>::operator [](int idx) const
{
    int size=first_free-first_used;
    if (size<=0)
        size+=ringmask+1;
    if ((idx<0) || (first_used<0) || (idx>=size)) {
        cerr<<"fifo["<<idx<<"]: invalid index; used="
            <<first_used<<" free="<<first_free<<" size="<<(ringmask+1)<<endl;
        return 0;
    }
    return ring[(first_used+idx)&ringmask];
}
//---------------------------------------------------------------------------//
/*
 * return value: 0: ok
 *               1: ringbuffer is full; next add will cause reallocation
 *               -1: no space (not implemented yet)
 */
template<class Q> int
FIFO<Q>::add(Q* item)
{
    if (first_used>=0) {
        if (first_used==first_free) { // ring full
            //dump("full(A)");
            int newmask=(ringmask<<1)|1;
            Q **newring=new Q*[newmask+1];
            int end=first_free>first_used?first_free:first_free+(ringmask+1);
            int nn=0;
            for (int i=first_used, j=0; i<end; i++, j++) {
                newring[j]=ring[i&ringmask];
                nn++;
            }
            //cerr<<nn<<" items copied"<<endl;
            delete[] ring;
            ring=newring;
            first_used=0;
            first_free=ringmask+1;
            ringmask=newmask;
            //dump("full(B)");
        }
        ring[first_free]=item;
        first_free=(first_free+1)&ringmask;
    } else { // ring empty
        ring[0]=item;
        first_used=0;
        first_free=1;
    }
    return first_used==first_free?1:0;
}
//---------------------------------------------------------------------------//
template<class Q> void
FIFO<Q>::remove(void)
{
    if (first_used==-1) {
        cerr<<"fifo::remove: removing from empty fifo"<<endl;
        return;
    }
    if (_last)
        delete _last;
    _last=ring[first_used];
    //delete ring[first_used];
    ring[first_used]=0;
    first_used=(first_used+1)&ringmask;
    if (first_used==first_free)
        first_used=-1;
}
//---------------------------------------------------------------------------//
template<class Q> int
FIFO<Q>::depth(void) const
{
    if (first_used==-1)
        return 0;
    int size=first_free-first_used;
    if (size<=0)
        size+=ringmask+1;
    return size;
}
template class FIFO<ems_subevent>;
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
ems_ved_info::ved_info::find_is_idx(int IS_ID) const
{
    for (int i=0; i<nr_is; i++) {
        if (is_infos[i].IS_ID==IS_ID)
            return i;
    }
    return -1;
}
//---------------------------------------------------------------------------//
void
ems_ved_info::dump(int level) const
{
    cerr<<"VED info: version "<<version<<endl;
    for (int i=0; i<nr_veds; i++) {
        cerr<<"  VED "<<ved_infos[i].VED_ID<<endl;
        for (int j=0; j<ved_infos[i].nr_is; j++) {
            cerr<<"    IS "<<ved_infos[i].is_infos[j].IS_ID
                <<" hints="<<ved_infos[i].is_infos[j].decoding_hints;
            if (ved_infos[i].is_infos[j].importance)
                cerr<<" ("<<ved_infos[i].is_infos[j].importance<<")";
            cerr<<endl;
        }
    }
}
//***************************************************************************//
//***************************************************************************//
