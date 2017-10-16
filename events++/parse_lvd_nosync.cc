/*
 * ems/events++/parse_lvd.cc
 * 
 * created 2007-Feb-15 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <errno.h>
#include <math.h>
#include "modultypes.h"
#include "compressed_io.hxx"
#include "cluster_data_sync.hxx"
#include <readargs.hxx>
#include <versions.hxx>

#include "expconfig_wasa.inc"

VERSION("2007-Feb-18", __FILE__, __DATE__, __TIME__,
"$ZEL: parse_lvd_nosync.cc,v 1.2 2007/03/02 23:26:53 wuestner Exp $")
#define XVERSION

#define swap_int(a) ( ((a) << 24) | \
                      (((a) << 8) & 0x00ff0000) | \
                      (((a) >> 8) & 0x0000ff00) | \
        ((unsigned int)(a) >>24) )

struct is_statist {
    is_statist():events(0), words(0), maxwords(0), removed_single(0),
            errors(0), n(0), s(0) {}
    unsigned long long events;
    unsigned long long words;
    unsigned long long maxwords;
    int removed_single;
    int errors;
    int n;
    double s;
};

struct evtime {
    evtime():last_eventnr(-1), act_eventnr(-1) {};
    int last_eventnr;
    int last_timestamp;
    int act_eventnr;
    int act_timestamp;
};

struct is_info {
    int is;
    bool is_lvd;
    ems_u32 *modtypes;
    int crate;
    double speed;
    char *vedname;
    char *comment;
    struct evtime evtime;
    struct is_statist statist;
};

//#define RUN_1190
#define Q15
#ifdef Q15
struct is_info is_info[]={
    {   1,  true, 0,              0,  0., "leer", "leer"},
    {  -1, false, 0,             -1,  0.,    "",    "nix"}
};
#endif
#ifdef RUN_1190
struct is_info is_info[]={
    {   1, false, 0,             31,  0., "scaler", "scaler"},
    {   2,  true, 0,             21,  0., "w09",    "F1/FPC"},
    {   3,  true, modtypes_3,    13,  5.047362e-06, "w10",    "GPX"},
    {   4,  true, modtypes_4,     4,  2.606809e-06, "w17",    "SQDC aeussere Halbschale hinten"},
    {   5,  true, modtypes_5,    11, -8.182480e-07, "w16",    "FQDC"},
    {   6,  true, modtypes_6,     3,  5.855271e-07, "w18",    "SQDC aeussere Halbschale vorne"},
    {   7,  true, modtypes_7,     1,  1.603863e-08, "w08",    "SQDC innere Halbschale vorne"},
    {   8,  true, modtypes_8,     2, -1.772749e-06, "w14",    "SQDC innere Halbschale hinten"},
    {   9,  true, modtypes_9,    12,  1.162707e-05, "w15",    "FQDC"},
    {  13,  true, modtypes_13,    5,  1.154911e-06, "w07",    "SQDC SEB und Summierer"},
    {  10,  true, 0,             22,  0., "w11",    "F1/MDC1"},
    {  11,  true, 0,             23,  0., "w12",    "F1/MDC2"},
    {  12,  true, 0,             24,  0., "w13",    "F1/MDC3"},
    {  50,  true, 0,             -1,  0., "w06",    "F1/pellet structure test"},
    { 100, false, 0,             -1,  0., "w14",    "timestamp"},
    {2000,  true, modtypes_2000, -1, -1.772288e-06, "w01",    "sync"},
    {  -1, false, 0,             -1,  0.,    "",    "nix"}
};
#endif
#ifdef RUN_1211
struct is_info is_info[]={
    {   1,  true, 0,             31,  0., "scaler", "scaler"},
    {   2,  true, 0,             21,  0., "w09",    "F1/FPC"},
    {   3,  true, modtypes_3,    13,  3.547568e-06, "w10",    "GPX"},
    {   4,  true, modtypes_4,     4,  2.742642e-06, "w17",    "SQDC aeussere Halbschale hinten"},
    {   5,  true, modtypes_5,    11, -2.143026e-06, "w16",    "FQDC"},
    {   6,  true, modtypes_6,     3,  2.904280e-06, "w18",    "SQDC aeussere Halbschale vorne"},
    {   7,  true, modtypes_7,     1,  5.587001e-04, "w08",    "SQDC innere Halbschale vorne"},
    {   8,  true, modtypes_8,     2, -3.373901e-06, "w14",    "SQDC innere Halbschale hinten"},
    {   9,  true, modtypes_9,    12,  9.105238e-06, "w15",    "FQDC"},
    {  13,  true, modtypes_13,    5, -2.914105e-07, "w07",    "SQDC SEB und Summierer"},
    {  10,  true, 0,             22,  0., "w11",    "F1/MDC1"},
    {  11,  true, 0,             23,  0., "w12",    "F1/MDC2"},
    {  12,  true, 0,             24,  0., "w13",    "F1/MDC3"},
    {  50,  true, 0,             -1,  0., "w06",    "F1/pellet structure test"},
    { 100, false, 0,             -1,  0., "w14",    "timestamp"},
    {2000,  true, modtypes_2000, -1, -3.277736e-06, "w01",    "sync"},
    {  -1, false, 0,             -1,  0.,    "",    "nix"}
};
#endif
#ifdef RUN_1212
struct is_info is_info[]={
    {   1,  true, 0,             31,  0., "scaler", "scaler"},
    {   2,  true, 0,             21,  0., "w09",    "F1/FPC"},
    {   3,  true, modtypes_3,    13,  3.408259e-06, "w10",    "GPX"},
    {   4,  true, modtypes_4,     4,  2.664772e-06, "w17",    "SQDC aeussere Halbschale hinten"},
    {   5,  true, modtypes_5,    11, -2.265436e-06, "w16",    "FQDC"},
    {   6,  true, modtypes_6,     3,  3.012740e-06, "w18",    "SQDC aeussere Halbschale vorne"},
    {   7,  true, modtypes_7,     1, -1.821683e-07, "w08",    "SQDC innere Halbschale vorne"},
    {   8,  true, modtypes_8,     2, -3.403956e-06, "w14",    "SQDC innere Halbschale hinten"},
    {   9,  true, modtypes_9,    12,  9.156846e-06, "w15",    "FQDC"},
    {  13,  true, modtypes_13,    5, -3.605938e-07, "w07",    "SQDC SEB und Summierer"},
    {  10,  true, 0,             22,  0., "w11",    "F1/MDC1"},
    {  11,  true, 0,             23,  0., "w12",    "F1/MDC2"},
    {  12,  true, 0,             24,  0., "w13",    "F1/MDC3"},
    {  50,  true, 0,             -1,  0., "w06",    "F1/pellet structure test"},
    { 100, false, 0,             -1,  0., "w14",    "timestamp"},
    {2000,  true, modtypes_2000, -1, -3.178811e-06, "w01",    "sync"},
    {  -1, false, 0,             -1,  0.,    "",    "nix"}
};
#endif

C_readargs* args;
STRING infile;
int selected_crate, selected_is;

C_ems_data ems_data;

struct statist {
    statist(): events(0) {}
    unsigned int events;
};

struct statist statist;

//---------------------------------------------------------------------------//
int readargs()
{
    args->addoption("crate", "crate", -1, "crate id", "crate");
    args->addoption("is", "is", -1, "IS id", "is");
    args->addoption("infile", 0, "-", "input file", "input");

    args->hints(C_readargs::exclusive, "crate", "is", 0);

    if (args->interpret()!=0)
        return -1;

    infile=args->getstringopt("infile");
    selected_crate=args->getintopt("crate");
    selected_is=args->getintopt("is");

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
//---------------------------------------------------------------------------//
static int
check_lvds_header(const ems_subevent* sev, struct is_info* info)
{
    ems_u32* d=sev->data;
    static int count=10;

    if (d[0]!=sev->size-1) {
        printf("illegal datacounter: 0x%08x, sevsize=%d\n", d[0], sev->size);
        return -1;
    }

    if (!(d[1]&0x80000000)) {
        printf("event %d, is %d; illegal header=0x%08x\n",
                sev->ev_nr, sev->sev_id, d[1]);
        return -1;
    }

    unsigned int bytelength=d[1]&0x0000ffff;
    if (bytelength!=(sev->size-1)*4) {
        printf("illegal length: sevsize=%d bytes=%d\n",
            sev->size, bytelength);
        return -1;
    }

    if ((count>=0)&&(d[1]&0xffff0000)!=0x80000000) {
#if 0
        printf("event %d, is %d; header=0x%08x ", event->event_nr, sev->sev_id,
                d[1]);
#endif
        if (d[1]&0x40000000)
            printf("ill_length ");
        if (d[1]&0x20000000)
            printf("full ");
#if 0
        if (d[1]&0x10000000)
            printf("trigger_lost ");
#endif
        if (d[1]&0x08000000)
            printf("error ");
        if (d[1]&0x04000000)
            printf("PLL ");
#if 0
        printf("\n");
#endif
        count--;
    }

    return 0;
}
//---------------------------------------------------------------------------//
#if 0
static int
parse_sync(const ems_subevent* sev, struct is_info* info)
{
    int num=sev->size-4;
    ems_u32* d=sev->data+4;

    unsigned int numdump=10;
    if (sev->size<numdump)
        numdump=sev->size;
    //dump_raw(sev->data, numdump, "USER parse_sync");

    if (sev->size<4) {
        printf("parse_sync: subevent too short (%d words)\n", sev->size);
        return 0;
    }

    if (!info->modtypes)
        return 0;

    for (int i=0; i<num; i++) {
        ems_u32 v=d[i];
        int module=(v>>28)&0xf;

        switch (info->modtypes[module])    {
        case ZEL_LVD_MSYNCH: {
#if 0
        int trigger=(v>>22)&0xf;
        int error=(v>>16)&3;
        bool tdt     =v&0x04000000;
        bool rejected=v&0x00400000;
        bool evcount =v&0x00040000;
                int time=v&0xfffff;
                printf("M[%x] 0x%08x", module, v);
                if (tdt) {
                    printf(" tdt %f us\n", time*.1);
                } else {
                    printf(" trigger %d at %f us%s\n", trigger, time*.1,
                        rejected?" (rejected)":"");
                }
#endif
            }
            break;
        case ZEL_LVD_OSYNCH: {
#if 0
                int val=v&0xffff;
                printf("S[%x] 0x%08x", module, v);
                if (evcount) {
                    printf(" event %d\n", val);
                } else {
                    printf(" ldt for port %d: %f us", trigger, val*.1);
                    if (error)
                        printf(" error %d", error);
                    printf("\n");
                }
#endif
            }
            break;
        default:
            cerr<<"unexpected module type "<<hex<<info->modtypes[module]
                <<" at addr "<<module<<dec<<" in VED "<<sev->sev_id<<endl;
            return -1;
        }     
    }
    return 0;
}
#endif
//---------------------------------------------------------------------------//
#if 0
static bool
is_wanted(struct is_info* info)
{
    if (selected_crate>=0) {
        if (info->crate!=selected_crate)
            return false;
    }
    if (selected_is>=0) {
        if (info->is!=selected_is)
            return false;
    }
    return true;
}
#endif
//---------------------------------------------------------------------------//
static struct is_info*
find_is_info(int id)
{
    struct is_info *info=is_info;
    while ((info->is!=-1) && (id!=info->is))
        info++;
    if (info->is==-1) {
        cerr<<"find_is_info: unknown IS Id "<<id<<endl;
        return 0;
    } else {
        return info;
    }
}
//---------------------------------------------------------------------------//
#if 0
static int
extract_headerinfo(const ems_subevent* sev)
{
// expected structure:
//  d[0] word counter
//  d[1] header0: 0x800size
//  d[2] header1: timestamp
//  d[3] header2: eventnr

    struct ved_info *info;
    int res=0;

//printf("### got subevent, data=%p, size=%d\n", sev->data, sev->size);
    unsigned int numdump=10;
    if (sev->size<numdump)
        numdump=sev->size;

    if ((info=find_ved_info(sev->sev_id))==0)
        return -1;

    if (info->mtype==t_sync)
        dump_raw(sev->data, numdump, "USER parse_subevent");

    if (!info->is_lvd)
        return 0;

    if (check_lvds_header(sev, info)<0) {
        dump_subevent(sev, info);
        return -1;
    }





    switch (info->mtype) {
    case t_scaler:
        res=parse_scaler(sev, info);
        break;
    case t_tdc:
        res=parse_tdc(sev, info);
        break;
    case t_stdc:
        res=parse_tdc(sev, info);
        break;
    case t_ftdc:
        res=parse_tdc(sev, info);
        break;
    case t_qdc:
    case t_sqdc:
    case t_fqdc:
        res=parse_qdc(sev, info);
        break;
    case t_sync:
        res=parse_sync(sev, info);
        break;
    case t_timestamp:
    case t_unknown:
        break;
    case t_none:
        cerr<<"parse_subevent: type of subevent is t_none"<<endl;
        return -1;
    }

    info->statist.events++;
    info->statist.words+=sev->size;
    if (sev->size>info->statist.maxwords)
        info->statist.maxwords=sev->size;

    return res;
}
#endif
//---------------------------------------------------------------------------//
#if 0
static int
extract_headerinfo(const ems_subevent* sev)
{
// expected structure:
//  d[0] word counter
//  d[1] header0: 0x800size
//  d[2] header1: timestamp
//  d[3] header2: eventnr

    struct ved_info *ved_info;
    int res=0;

//printf("### got subevent, data=%p, size=%d\n", sev->data, sev->size);
    unsigned int numdump=10;
    if (sev->size<numdump)
        numdump=sev->size;

    if ((ved_info=find_ved_info(sev->sev_id))==0)
        return -1;

    if (info->mtype==t_sync)
        dump_raw(sev->data, numdump, "USER parse_subevent");

    if (!info->is_lvd)
        return 0;

    if (check_lvds_header(sev, info)<0) {
        constant unsigned int maxdump=100;
        unsigned int numdump=sev->size;
        if (numdump>maxdump)
            numdump=maxdump;
        dump_raw(sev->data, numdump, "illegal header in extract_headerinfo");
        return -1;
    }

    if (ved_info->evtime.last_eventnr<0) {
        ved_info->evtime.last_eventnr=
    }

    double time_l=(sev->data[2]&0xffff)*.12995 /*97.47*/;
    double time_h=((sev->data[2]>>16)&0xffff)*6.4;
    double time=time_l+time_h;



    switch (info->mtype) {
    case t_scaler:
        res=parse_scaler(sev, info);
        break;
    case t_tdc:
        res=parse_tdc(sev, info);
        break;
    case t_stdc:
        res=parse_tdc(sev, info);
        break;
    case t_ftdc:
        res=parse_tdc(sev, info);
        break;
    case t_qdc:
    case t_sqdc:
    case t_fqdc:
        res=parse_qdc(sev, info);
        break;
    case t_sync:
        res=parse_sync(sev, info);
        break;
    case t_timestamp:
    case t_unknown:
        break;
    case t_none:
        cerr<<"parse_subevent: type of subevent is t_none"<<endl;
        return -1;
    }

    info->statist.events++;
    info->statist.words+=sev->size;
    if (sev->size>info->statist.maxwords)
        info->statist.maxwords=sev->size;

    return res;
}
#endif
//---------------------------------------------------------------------------//
static void
print_statistics()
{
    cout<<statist.events<<" events"<<endl;
    struct is_info *info;
    int i;

    info=is_info; i=0;
    while (info->is!=-1) {
        double speed=info->statist.s/info->statist.n;
        printf("i %d is %4d: %.6e\n", i, info->is, speed);
        info++; i++;
    }


    info=is_info;
    while (info->is!=-1) {
        if (info->statist.events>0) {
            unsigned long long len=info->statist.words/info->statist.events;
            cout<<"Crate"<<info->crate<<"(ID "<<info->is<<")"<<": "<<len<<" words"
                <<", max "<<info->statist.maxwords<<endl;
            cout<<"  "<<info->statist.events<<"events, "
                <<info->statist.errors<<" errors"<<endl;
        }
        info++;
    }
}
//---------------------------------------------------------------------------//
static void
print_is_statist(void)
{
    struct is_info *info=is_info;
    while (info->is!=-1) {
        if (info->statist.removed_single) {
            cerr<<"removed "<<info->statist.removed_single
                <<" single events of IS "<<info->is<<endl;
        }
        info++;
    }
}
//---------------------------------------------------------------------------//
static void
clear_is_statist(void)
{
    struct is_info *info=is_info;
    while (info->is!=-1) {
        info->statist.removed_single=0;
        info++;
    }
}
//---------------------------------------------------------------------------//
#undef COMPACT
static int
check_evnumbers(void)
{
    bool err=false;
    for (int i=0; i<ems_data.nr_is; i++) {
        int removed=0;
        if (ems_data.fifo[i].ignore)
            continue;
        while ((ems_data.fifo[i].depth()>1) && 
                (ems_data.fifo[i][0]->ev_nr<statist.events)) {
            ems_data.fifo[i].remove();
            removed++;
            err=true;
        }
        if (removed) {
#ifdef COMPACT
            if (removed==1) {
                struct is_info *is_info;
                if ((is_info=find_is_info(ems_data.fifo[i].is_id))==0)
                    return -1;
                is_info->statist.removed_single++;
            } else {
#endif
                cerr<<"event "<<statist.events<<": "
                    <<removed<<" subevents removed from IS "
                    <<ems_data.fifo[i][0]->sev_id<<endl;
#ifdef COMPACT
            }
#endif
        }
    }
    if (err) { // check it again
        err=false;
        for (int i=0; i<ems_data.nr_is; i++) {
            if (ems_data.fifo[i].ignore)
                continue;
            if (ems_data.fifo[i][0]->ev_nr!=statist.events)
                err=true;
        }
    }
    if (err) {
        cerr<<"ev_nr mismatch in "<<statist.events<<"th event:"<<endl;
        for (int i=0; i<ems_data.nr_is; i++) {
            if (ems_data.fifo[i].ignore)
                continue;
            cerr<<"IS "<<ems_data.fifo[i][0]->sev_id<<": ";
            cerr<<" ("<<ems_data.fifo[i].last()->ev_nr<<")";
            for (int j=0; (j<4)&&(j<ems_data.fifo[i].depth()); j++)
                cerr<<" "<<ems_data.fifo[i][j]->ev_nr;
            cerr<<endl;
        }
        return -1;
    }
    return 0;
}
#undef COMPACT
//---------------------------------------------------------------------------//
static int
check_lvds_headers(void)
{
    for (int i=0; i<ems_data.nr_is; i++) {
        if (ems_data.fifo[i].ignore)
            continue;
        struct is_info *is_info;
        is_info=static_cast<struct is_info*>(ems_data.fifo[i].private_ptr);
        if (is_info->is_lvd) {
            if (check_lvds_header(ems_data.fifo[i][0], is_info)<0)
                return -1;
        }
    }
    return 0;
}
//---------------------------------------------------------------------------//
bool away(double val, double center, double deviation)
{
    return fabs(val-center)>deviation;
}
//---------------------------------------------------------------------------//
bool away_x(double val, double base, double deviation)
{
    double fact=round(val/base);
    return fabs(val-base*fact)>deviation;
}
//---------------------------------------------------------------------------//
#define VERBOSE
static int
check_evtimes(void)
{
    for (int i=0; i<ems_data.nr_is; i++) {
        struct is_info *is_info;
        is_info=static_cast<struct is_info*>(ems_data.fifo[i].private_ptr);
        if (ems_data.fifo[i].ignore||!is_info->is_lvd)
            continue;

        const ems_subevent *sev=ems_data.fifo[i][0];
        ems_u32 trigtime=sev->data[2];
        int trigtime_l=trigtime&0xffff;
        int trigtime_h=(trigtime>>16)&0xffff;
        if (ems_data.fifo[i].trigtime_l>=0) {
            double tdiff_l, tdiff_h, tdiff;
            tdiff_l=(trigtime_l-ems_data.fifo[i].trigtime_l)*.12995;
            tdiff_h=(trigtime_h-ems_data.fifo[i].trigtime_h)*6400.;
            tdiff=(tdiff_l+tdiff_h);
            ems_data.fifo[i].trigdiff0=tdiff;
            if ((statist.events<10) || away_x(tdiff, 80898., 500.)) {
                fprintf(stderr, "%6d: %10.2f %10.2f\n", statist.events, tdiff, tdiff-80898.);
            }
        }
        ems_data.fifo[i].trigtime_l=trigtime_l;
        ems_data.fifo[i].trigtime_h=trigtime_h;
    }
    return 0;
}
#undef VERBOSE
//---------------------------------------------------------------------------//
static int
parse_event()
{
//     if ((statist.events%10000)==0) {
//         cerr<<"parse_event "<<statist.events<<endl;
//         print_is_statist();
//         clear_is_statist();
//     }

#if 0
    if (check_evnumbers()<0)
        return -1;
    if (!ems_data.events_available())
        return 0;
#endif

    if (check_lvds_headers()<0) {
        goto weiter;
    }

    if (check_evtimes()<0)
        return -1;

#if 0

    if (check_synctime(event)<0)
        return -1;
#endif
        
weiter:
    statist.events++;
    for (int i=0; i<ems_data.nr_is; i++) {
        if (!ems_data.fifo[i].ignore)
            ems_data.fifo[i].remove();
    }

    return 0;
}
//---------------------------------------------------------------------------//
static int
check_is_info(void)
{
    int err=0;
    for (int i=0; i<ems_data.nr_is; i++) {
        struct is_info *is_info;
        cerr<<"i="<<i<<" is_id="<<ems_data.fifo[i].is_id<<endl;
        if ((is_info=find_is_info(ems_data.fifo[i].is_id))==0)
            err++;
        ems_data.fifo[i].private_ptr=is_info;
    }
    return err?-1:0;
}
//---------------------------------------------------------------------------//
static void
prepare_globals()
{
    statist.events=0;
#if 0
    struct is_info *info=is_info;
    while (info->is!=-1) {
        info->statist.events=0;
        info->statist.words=0;
        info->statist.maxwords=0;
        info->statist.errors=0;
        info++;
    }
#endif
}
//---------------------------------------------------------------------------//
int
main(int argc, char* argv[])
{
    const unsigned int maxevents=0;
    args=new C_readargs(argc, argv);
    if (readargs()!=0)
        return 1;

    compressed_input input(infile);
    ems_cluster* cluster;
    ems_u32 *buf;

    if (!input.good()) {
        cout<<"cannot open input file "<<infile<<endl;
        return 1;
    }

    prepare_globals();
    ems_data.max_fifodepth=100000;
    ems_data.min_fifodepth=16;

    bool is_info_ok=false;

    while (read_record(input.path(), &buf)>0) {
        cluster=new ems_cluster(buf);
        if (ems_data.parse_cluster(cluster)<0)
            goto raus;
        delete cluster;

        while (ems_data.events_available()) {
            if (!is_info_ok) {
                if (check_is_info()<0)
                    goto raus;
                is_info_ok=true;
            }
            if (parse_event()<0) {
                cerr<<"parse_event returned -1"<<endl;
                goto raus;
            }
            if ((maxevents>0) && (statist.events>=maxevents)) {
                cerr<<"maximum number of "<<maxevents<<" events parsed"<<endl;
                    goto raus;
            }
        }
    }

raus:
    print_statistics();

    return 0;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
