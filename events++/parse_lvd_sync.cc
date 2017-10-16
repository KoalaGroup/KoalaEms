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
"$ZEL: parse_lvd_sync.cc,v 1.5 2007/03/04 19:08:11 wuestner Exp $")
#define XVERSION

#define swap_int(a) ( ((a) << 24) | \
                      (((a) << 8) & 0x00ff0000) | \
                      (((a) >> 8) & 0x0000ff00) | \
        ((unsigned int)(a) >>24) )


class hist_node {
  public:
    hist_node(class tree_hist *t, int v): tree(t), more(0), less(0), id(v)
    {
        for (int i=0; i<10; i++)
            who[i]=0;
    }
    void dump(void);
    class tree_hist *tree;
    hist_node *more;
    hist_node *less;
    int id;
    int who[10];
};

class tree_hist {
  public:
    tree_hist(double quant): quantum(quant), nodecount(0)
    {
        root=new hist_node(this, 0);
    }
    void dump(void);
    void add(double val, int idx, int n=1);
    void add(long int val, int idx, int n=1);
    double quantum;
    hist_node* root;
    int nodecount;
    int last_dumped;
    bool last_dumped_valid;
};

tree_hist histtree(.1);
//tree_hist histtree(-1);

void tree_hist::add(long int id, int idx, int n)
{
    hist_node *node=root;
    while (node->id!=id) {
        if (node->id<id) {
            if (!node->more) {
                node->more=new hist_node(this, id);
                nodecount++;
            }
            node=node->more;
        } else {
            if (!node->less) {
                node->less=new hist_node(this, id);
                nodecount++;
            }
            node=node->less;
        }
    }
    node->who[idx]+=n;
}

void tree_hist::add(double val, int idx, int n)
{
    add(lrint(val/quantum), idx, n);
}

void tree_hist::dump(void)
{
    printf("tree_hist: %d nodes\n", nodecount);
    last_dumped_valid=false;
    root->dump();
}

void hist_node::dump()
{
    if (less)
        less->dump();

    // check for single hits
    int maxhits=0;
    for (int i=0; i<10; i++) {
        if (who[i]>maxhits)
            maxhits=who[i];
    }

    if (maxhits>0) {
        if (tree->last_dumped_valid) {
            int x=tree->last_dumped+1;
            if (x<id) {
                if (tree->quantum<0)
                    printf("%5d ", x);
                else
                    printf("%f ", x*tree->quantum);
                for (int i=0; i<10; i++) {
                    printf("\t%5d", 0);
                }
                printf("\n");
            }
            x=id-1;
            if (x>tree->last_dumped+1) {
                if (tree->quantum<0)
                    printf("%5d ", x);
                else
                    printf("%f ", x*tree->quantum);
                for (int i=0; i<10; i++) {
                    printf("\t%5d", 0);
                }
                printf("\n");
            }
        }

        if (tree->quantum<0)
            printf("%5d ", id);
        else
            printf("%f ", id*tree->quantum);
        for (int i=0; i<10; i++) {
            printf("\t%5d", who[i]);
        }
        printf("\n");
        tree->last_dumped=id;
        tree->last_dumped_valid=true;
    }

    if (more)
        more->dump();
}

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
    char *vedname;
    char *comment;
    double speed;
    //struct evtime evtime;
    struct is_statist statist;
};

#define RUN_FEB
//#define RUN_NOV
//#define RUN_1211
//#define RUN_1230
//#define RUN_Q15
#ifdef RUN_Q15
struct is_info is_info[]={
    {   1,  true, 0,              0, "leer", "leer"},
    {  -1, false, 0,             -1,    "",    "nix"}
};
#endif
#ifdef RUN_NOV
struct is_info is_info[]={
    {   2, false, 0,             21, "w09",    "F1/FPC"},
    {   3,  true, modtypes_3,    13, "w10",    "GPX"},
    {   4,  true, modtypes_4,     4, "w17",    "SQDC aeussere Halbschale hinten"},
    {   5,  true, modtypes_5,    11, "w16",    "FQDC"},
    {   6,  true, modtypes_6,     3, "w18",    "SQDC aeussere Halbschale vorne"},
    {   7,  true, modtypes_7,     1, "w08",    "SQDC innere Halbschale vorne"},
    {   8,  true, modtypes_8,     2, "w14",    "SQDC innere Halbschale hinten"},
    {   9,  true, modtypes_9,    12, "w15",    "FQDC"},
    {  10,  false, 0,            22, "w11",    "F1/MDC1"},
    {  11, false, 0,             23, "w12",    "F1/MDC2"},
    {  12, false, 0,             24, "w13",    "F1/MDC3"},
    {  13,  true, modtypes_13,    5, "w07",    "SQDC SEB und Summierer"},
    { 100, false, 0,             -1, "w14",    "timestamp"},
    {  -1, false, 0,             -1,    "",    "nix"}
};
#endif
#ifdef RUN_FEB
struct is_info is_info[]={
    {   1, false, 0,             31, "scaler", "scaler"},
    {   2,  true, 0,             21, "w09",    "F1/FPC"},
    {   3,  true, modtypes_3,    13, "w10",    "GPX"},
    {   4,  true, modtypes_4,     4, "w17",    "SQDC aeussere Halbschale hinten"},
    {   5,  true, modtypes_5,    11, "w16",    "FQDC"},
    {   6,  true, modtypes_6,     3, "w18",    "SQDC aeussere Halbschale vorne"},
    {   7,  true, modtypes_7,     1, "w08",    "SQDC innere Halbschale vorne"},
    {   8,  true, modtypes_8,     2, "w14",    "SQDC innere Halbschale hinten"},
    {   9,  true, modtypes_9,    12, "w15",    "FQDC"},
    {  13,  true, modtypes_13,    5, "w07",    "SQDC SEB und Summierer"},
    {  10,  true, 0,             22, "w11",    "F1/MDC1"},
    {  11,  true, 0,             23, "w12",    "F1/MDC2"},
    {  12,  true, 0,             24, "w13",    "F1/MDC3"},
    {  50,  true, 0,             -1, "w06",    "F1/pellet structure test"},
    { 100, false, 0,             -1, "w14",    "timestamp"},
    {2000, true, modtypes_2000, -1, "w01",    "sync"},
    {  -1, false, 0,             -1,    "",    "nix"}
};
#endif

C_readargs* args;
STRING infile;
int selected_crate, selected_is;

C_ems_data ems_data;

struct statist {
    statist(): events(0), good_events(0), ignored_events(0), bad_events(0),
        good_events_local(0) {}
    unsigned int events;
    unsigned int good_events;
    unsigned int ignored_events;
    unsigned int bad_events;
    unsigned int good_events_local;
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
static void
print_statistics()
{
    struct is_info *info;

    cerr<<statist.events<<" events"<<endl;
    cerr<<statist.good_events<<" good events"<<endl;
    cerr<<statist.ignored_events<<" ignored events"<<endl;
    cerr<<statist.bad_events<<" bad events"<<endl;

    for (int i=0; i<ems_data.nr_is; i++) {
        struct is_info *is_info;
        is_info=static_cast<struct is_info*>(ems_data.fifo[i].private_ptr);
        if (ems_data.fifo[i].ignore||!is_info->is_lvd)
            continue;
        double speed=is_info->statist.s/is_info->statist.n;
        fprintf(stderr, "i %d is %4d: %.6e\n", i, is_info->is, speed);
    }

    histtree.dump();

    info=is_info;
    while (info->is!=-1) {
        if (info->statist.events>0) {
            unsigned long long len=info->statist.words/info->statist.events;
            cerr<<"Crate"<<info->crate<<"(ID "<<info->is<<")"<<": "<<len<<" words"
                <<", max "<<info->statist.maxwords<<endl;
            cerr<<"  "<<info->statist.events<<"events, "
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
static double
sev_tdiff(ems_fifo &fifo, int idx)
{
    struct is_info *is_info;
    ems_subevent *sev, *sev0;
    double tdiff_l, tdiff_h, tdiff;

    sev=fifo[idx];
    if (sev->trigdiff>0)
        return sev->trigdiff;

    if (idx>0)
        sev0=fifo[idx-1];
    else
        sev0=fifo.last();

    if (sev==0 || sev0==0) {
        cerr<<"sev_tdiff(IS="<<fifo.is_id<<"): no sev"<<endl;
        return -1.;
    }

    is_info=static_cast<struct is_info*>(fifo.private_ptr);

    ems_u32 trigtime=sev->data[2];
    ems_u32 trigtime0=sev0->data[2];
    tdiff_l=(signed int)(trigtime&0xffff)-(signed int)(trigtime0&0xffff);
    tdiff_l*=.12995;
    int dd=((trigtime>>16)&0xffff)-((trigtime0>>16)&0xffff);
    if (dd<0)
        dd+=65536;
    tdiff_h=dd;
    tdiff_h*=6400.;
    tdiff=tdiff_l+tdiff_h;
    sev->trigdiff0=tdiff;
    tdiff*=(1-is_info->speed);
    sev->trigdiff=tdiff;
    return tdiff;
}
//---------------------------------------------------------------------------//
static void
dump_times(int start, int stop)
{
    int indices[100], num=0;

    for (int idx=0; idx<ems_data.nr_is; idx++) {
        struct is_info *is_info;
        is_info=static_cast<struct is_info*>(ems_data.fifo[idx].private_ptr);
        if (!ems_data.fifo[idx].ignore&&is_info->is_lvd)
            indices[num++]=idx;
    }

    for (int line=start; line<=stop; line++) {
        //double t=0.;
        for (int i=0; i<num; i++) {
            int idx=indices[i];
            sev_tdiff(ems_data.fifo[idx], line);
            //t+=ems_data.fifo[idx][0]->trigdiff0;
        }
        //t/=num;
        fprintf(stderr, "#%5d#", line);
        for (int i=0; i<num; i++) {
            fprintf(stderr, " %10.2f", ems_data.fifo[indices[i]][line]->trigdiff0);
        }
        fprintf(stderr, "\n");
    }
}
//---------------------------------------------------------------------------//
static double
sigma(int num, double sum, double qsum)
{
    return sqrt((num*qsum-sum*sum)/(num*(num-1)));
}
//---------------------------------------------------------------------------//
static const char*
bits(unsigned int v)
{
    static char s[33];
    s[32]=0;
    for (int i=31; i>=0; i--) {
        s[i]=v&1?'1':'0';
        v>>=1;
        if (!v)
            return s+i;
    }
    return 0; // never reached (if sizeof(unsigned int)<=4)
}
//---------------------------------------------------------------------------//
static int
match_subevents(int *indices, int num)
{
    const int max_s=2;
    const int max_m=2;
    int size=max_s*1<<num;
    float *sums=new float[size];
    fprintf(stderr, "max_s=%d 1<<num=%d size=%d sums=%p\n",
            max_s, (1<<num),  max_s*1<<num, sums);

    fprintf(stderr, "try to resort event %d\n", statist.events);
    dump_times(0, 8);

    // fuer alle Verschiebungen von 1 bis 4
    for (int s=1; s<max_s; s++) {
        fprintf(stderr, "s=%d\n", s);
        for (int k=0; k<1<<num; k++) {
            float sum=0;
            // teste die ersten 8 moeglichen Events
            for (int m=0; m<max_m; m++) {
                // Sigma fuer eine Zeile
                float zsum=0, zqsum=0;
                for (int i=0; i<num; i++) {
                    int t=m;
                    if ((k>>i)&1)
                        t+=s;
                    float diff=ems_data.fifo[indices[i]][t]->trigdiff;
                    zsum+=diff;
                    zqsum+=diff*diff;
                }
                sum+=sqrtf(num*zqsum/(zsum*zsum)-1);
            }
            sums[k*max_s+s]=sum;
            //if (k<10) {
            //    fprintf(stderr, "k=b%s sum=%f\n", bits(k), sum);
            //}
        }
    }

    // find smallest value
    int best_s=1, best_k=0;
    float best=sums[best_k*max_s+best_s];
    for (int s=1; s<max_s; s++) {
        for (int k=0; k<1<<num; k++) {
            if (sums[k*max_s+s]<best) {
                best_s=s; best_k=k;
                best=sums[best_k*max_s+best_s];
            }
        }
    }

    fprintf(stderr, "best modification: s=%d k=0x%x\n", best_s, best_k);
    if ((best_k==0) || (best_k==((1<<num)-1))) {
        fprintf(stderr, "--> not shifted\n");
    } else {
        for (int i=0; i<num; i++) {
            if ((best_k>>i)&1) {
                for (int j=0; j<best_s; j++)
                    ems_data.fifo[indices[i]].remove();
            }
        }
    }

    delete[] sums;
    return 0;
}
//---------------------------------------------------------------------------//
static int
check_evtimes(bool print_it)
{
    int indices[100], num/*, nn*/;
    //bool remove_sev[100];
    double t, qt, sd, thresh;
    bool valid=true, print=print_it, suspect=false;

    // create list of indices to be used
    num=0;
    for (int idx=0; idx<ems_data.nr_is; idx++) {
        struct is_info *is_info;
        is_info=static_cast<struct is_info*>(ems_data.fifo[idx].private_ptr);
        if (!ems_data.fifo[idx].ignore&&is_info->is_lvd) {
            //cerr<<"add index "<<idx<<" is "<<ems_data.fifo[idx].is_id<<endl;
            indices[num++]=idx;
        }
    }
    if (num==0) {
        cerr<<"check_evtimes: no valid ISs found"<<endl;
        return -1;
    }

    // compute average time and standard deviation
    t=0.;
    qt=0.;
    for (int i=0; i<num; i++) {
        int idx=indices[i];
        if (sev_tdiff(ems_data.fifo[idx], 0)>0.) {
            double diff=ems_data.fifo[idx][0]->trigdiff;
            t+=diff;
            qt+=diff*diff;
        } else {
            valid=false;
        }
    }
    if (!valid)
        return 0;

    sd=sigma(num, t, qt);
    t/=num;
    thresh=sd/10.;
    //cerr<<setprecision(9);
    //cerr<<"A sd="<<sd<<" thresh="<<thresh<<" t="<<t<<" min="<<t-thresh<<" max="<<t+thresh<<endl;

    if (t>1000000.) { // too large, we ignore this event
        statist.ignored_events++;
        //cerr<<"event "<<statist.events<<" ignored, t="<<t<<endl;
        //dump_times(0, 0);
        return 0;
    }

    if (sd/t<1e-3) { // OK; good event
        statist.good_events++;
        statist.good_events_local++;
        // calculate diffs for speed correction
        if ((statist.events>2) && (statist.events<100)) {
            double t0=0;
            for (int i=0; i<num; i++) {
                t0+=ems_data.fifo[indices[i]][0]->trigdiff0;
            }
            t0/=num;
            for (int i=0; i<num; i++) {
                struct is_info *is_info;
                is_info=static_cast<struct is_info*>(ems_data.fifo[indices[i]].private_ptr);
                double diff=ems_data.fifo[indices[i]][0]->trigdiff0-t0;
                is_info->statist.n++;
                is_info->statist.s+=diff/t0;
                if (is_info->statist.n>5) {
                    is_info->speed=is_info->statist.s/is_info->statist.n;
                }
            }
        }
        goto improvement_skipped;
    }
#if 0
    if (statist.good_events_local>10)
        cerr<<"burst of good events: "<<statist.good_events_local<<endl;
    statist.good_events_local=0;
    statist.bad_events++;

    fprintf(stderr, "%6d#", statist.events);
    for (int i=0; i<num; i++) {
        fprintf(stderr, " %10.2f", ems_data.fifo[indices[i]][0]->trigdiff0);
    }
    fprintf(stderr, "\n");
    print=true;
    dump_times(1, 5);

    // find bad values (sd without value is much better)
    for (int i=0; i<num; i++)
        remove_sev[i]=false;
    for (int i=0; i<num; i++) {
        double sum=0, qsum=0, s, a;
        for (int j=0; j<num; j++) {
            if (j!=i) {
                double diff=ems_data.fifo[indices[j]][0]->trigdiff;
                sum+=diff;
                qsum+=diff*diff;
            }
        }
        s=sigma(num-1, sum, qsum);
        a=sum/(num-1);
        cerr<<"i="<<i<<" s="<<s<<" a="<<a;
        if (s/a<1e-3) {
            remove_sev[i]=true;
            //cerr<<" marked"<<endl;
        } else {
            //cerr<<" NOT marked"<<endl;
        }
    }

    // recalculate average and threshold
    t=0.;
    qt=0.;
    nn=0;
    for (int i=0; i<num; i++) {
        if (!remove_sev[i]) {
            double diff=ems_data.fifo[indices[i]][0]->trigdiff;
            t+=diff;
            qt+=diff*diff;
            nn++;
        }
    }
    sd=sigma(nn, t, qt);
    t/=nn;
    thresh=sd/10.;

    cerr<<"thresh="<<thresh<<" t="<<t<<" min="<<t-thresh<<" max="<<t+thresh<<endl;
    // try to remove the bad values (eleminate the subevents)
    for (int i=0; i<num; i++) {
        if (remove_sev[i]) {
            double diff;
            int idx=indices[i];
            diff=sev_tdiff(ems_data.fifo[idx], 1);
            cerr<<"i="<<i<<" diff="<<diff<<" t="<<t<<" thresh="<<thresh<<endl;
            if (fabs(diff-t)<=thresh) {
                ems_data.fifo[idx].remove();
                cerr<<"removed sev from IS "<<ems_data.fifo[idx].is_id<<endl;
            }
        }
    }
#endif
    match_subevents(indices, num);

improvement_skipped:

    // print if necessary
    //if (!print) {
        for (int i=0; i<num; i++) {
            if (fabs(ems_data.fifo[indices[i]][0]->trigdiff-t)>10.) {
                print=true;
                suspect=true;
            }
        }
    //}
    if (print) {
        double deviation;
        int i0, i1;
        // find the worst IS
        deviation=-1;
        i0=-1;
        for (int i=0; i<num; i++) {
            double xdeviation=fabs(ems_data.fifo[indices[i]][0]->trigdiff-t);
            if (xdeviation>deviation) {
                deviation=xdeviation;
                i0=i;
            }
        }
        // recalculate average
        t=0.;
        for (int i=0; i<num; i++) {
            if (i!=i0)
                t+=ems_data.fifo[indices[i]][0]->trigdiff;
        }
        t/=(num-1);
        // find the 2nd worst IS
        deviation=-1;
        i1=-1;
        for (int i=0; i<num; i++) {
            if (i!=i0) {
                double xdeviation=fabs(ems_data.fifo[indices[i]][0]->trigdiff-t);
                if (xdeviation>deviation) {
                    deviation=xdeviation;
                    i1=i;
                }
            }
        }
        // recalculate average
        t=0.;
        double t0=0;
        for (int i=0; i<num; i++) {
            if ((i!=i0)&&(i!=i1)) {
                t+=ems_data.fifo[indices[i]][0]->trigdiff;
                t0+=ems_data.fifo[indices[i]][0]->trigdiff0;
            }
        }
        t/=(num-2);
        t0/=(num-2);

        fprintf(stderr, "%6d ", statist.events);
        for (int i=0; i<num; i++) {
            fprintf(stderr, " %10.2f", ems_data.fifo[indices[i]][0]->trigdiff0);
        }
        fprintf(stderr, "\n       ");
        for (int i=0; i<num; i++) {
            double diff;
            diff=ems_data.fifo[indices[i]][0]->trigdiff0-t0;
            fprintf(stderr, "  %8.2f ", diff);
            struct is_info *is_info;
            is_info=static_cast<struct is_info*>(ems_data.fifo[indices[i]].private_ptr);
            histtree.add(diff, i);
        }
        fprintf(stderr, "  %f\n       ", t0);
        for (int i=0; i<num; i++) {
            double diff;
            diff=ems_data.fifo[indices[i]][0]->trigdiff-t;
            fprintf(stderr, "  %8.2f ", diff);
        }
        fprintf(stderr, "  %f %f\n", t, sd);
    }

    return suspect?1:0;
}
//---------------------------------------------------------------------------//
static int
parse_event()
{
    bool print_it=false;
    int res;

    if ((statist.events%1000000)==0) {
        cerr<<"parse_event "<<statist.events<<endl;
        print_is_statist();
        clear_is_statist();
        print_it=true;
    }

#if 0
    if (check_evnumbers()<0)
        return -1;
    if (!ems_data.events_available())
        return 0;
#endif

    if (check_lvds_headers()<0) {
        goto weiter;
    }

    res=check_evtimes(print_it);
    if (res<0) {
        cerr<<"check_evtimes failed"<<endl;
        return -1;
    }


weiter:
    // remove event
    statist.events++;
    for (int i=0; i<ems_data.nr_is; i++) {
        if (!ems_data.fifo[i].ignore)
            ems_data.fifo[i].remove();
    }
#if 0
    if (res>0) {
        for (int i=0; i<ems_data.nr_is; i++) {
            if (!ems_data.fifo[i].ignore)
                ems_data.fifo[i].remove();
        }
    }
#endif
        
    return 0;
}
//---------------------------------------------------------------------------//
static int
check_is_info(void)
{
    int err=0;
    for (int i=0; i<ems_data.nr_is; i++) {
        struct is_info *is_info;
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
    struct is_info *info=is_info;
    while (info->is!=-1) {
        info->speed=0.;
        info->statist.events=0;
        info->statist.words=0;
        info->statist.maxwords=0;
        info->statist.errors=0;
        info++;
    }

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
//     for (float d=709.75488; d<20000.; d+=709.75488) {
//         histtree.add(d, 9, 10);
//         histtree.add(-d, 9, 10);
//     }
    print_statistics();

    return 0;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
