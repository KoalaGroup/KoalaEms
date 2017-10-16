/*
 * ems/events++/parse_strawtest.cc
 * 
 * created 2014-Oct-29 PW
 */

#include "config.h"

#include <cerrno>
#include <cmath>
#include <cstring>
#include <climits>
#include <unistd.h>
#include <vector>
#include "modultypes.h"
#include "swap_int.h"
#include "compressed_io.hxx"
#include "cluster_data.hxx"
#include "strawtest_data.hxx"
#include <readargs.hxx>
#include <versions.hxx>

VERSION("2014-11-03", __FILE__, __DATE__, __TIME__,
"$ZEL$")
#define XVERSION
#define NOWSTR "%Y-%m-%d %H:%M:%S %Z"

enum mtype {
    t_none, t_unknown, t_scaler, t_timestamp, t_lvd,
    t_tdc, t_stdc, t_ftdc, t_qdc, t_sqdc, t_fqdc, t_sync
};

struct ved_statist {
    unsigned long long events;
    unsigned long long subevents;
    unsigned long long words;
    unsigned long long maxwords;
    int errors;
};

typedef int (parsefunc)(const ems_event *event, const ems_subevent* sev,
        struct ved_info* info);
static parsefunc parse_scaler;
static parsefunc parse_lvd_qdc;
static parsefunc parse_tdc;
static parsefunc parse_qdc;
static parsefunc parse_sync;
static parsefunc parse_time;
static parsefunc parse_err;

typedef void (dumpfunc)(u_int32_t);
static dumpfunc dump_lvd;
static dumpfunc dump_time;
static dumpfunc dump_unknown;
static dumpfunc dump_scaler;
static dumpfunc dump_tdc;
static dumpfunc dump_qdc;

struct ved_info {
    const char *ved;
    enum mtype mtype;
    parsefunc *parse;
    dumpfunc *dump;
    int crate;
    int is;
    u_int32_t* modtypes;
    const char *comment;
    struct ved_statist statist;
    void *p;
};

u_int32_t modtypes_2000[16]={
    ZEL_LVD_MSYNCH,
    ZEL_LVD_OSYNCH,
    ZEL_LVD_OSYNCH,
    ZEL_LVD_OSYNCH,
    ZEL_LVD_OSYNCH,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

struct ved_info ved_info[]={
    {"O0O", t_lvd,       parse_lvd_qdc, dump_lvd,  1,   1, 0, "LVDS"},
    {"OtO", t_timestamp, parse_time,   dump_time,  1, 100, 0, "timestamp"},
    {"",    t_none,      parse_err,            0, -1,  -1, 0, "nix"}
};

C_readargs* args;
string infile;
enum mtype selected_mtype;
int selected_crate, selected_is;

struct statist {
    int records;
    int words;
    int events;
};

struct statist statist;

parsefunc *unused_parse[]={
    parse_scaler,
    parse_lvd_qdc,
    parse_tdc,
    parse_qdc,
    parse_sync,
    parse_time,
    parse_err,
};
dumpfunc *unused_dump[]={
    dump_lvd,
    dump_time,
    dump_unknown,
    dump_scaler,
    dump_tdc,
    dump_qdc,
};
//---------------------------------------------------------------------------//
struct smtype {
    const char *name;
    enum mtype mtype;
};
struct smtype mtypes[]={
    {"SCALER", t_scaler},
    {"QDC", t_qdc},
    {"SQDC", t_sqdc},
    {"FQDC", t_fqdc},
    {"TDC", t_tdc},
    {"STDC", t_stdc},
    {"F1", t_stdc},
    {"FTDC", t_ftdc},
    {"GPX", t_ftdc},
    {"SYNC", t_sync},
    {"", t_none}
};

static int
readargs(void)
{
    args->addoption("crate", "crate", -1, "crate id", "crate");
    args->addoption("is", "is", -1, "IS id", "is");
    args->addoption("type", "type", "", "module type", "type");
    args->addoption("infile", 0, "-", "input file", "input");

    args->hints(C_readargs::exclusive, "crate", "is", 0);

    if (args->interpret()!=0)
        return -1;

    infile=args->getstringopt("infile");
    selected_crate=args->getintopt("crate");
    selected_is=args->getintopt("is");
    if (args->isdefault("type")) {
        selected_mtype=t_none;
    } else {
        const char *mname=args->getstringopt("type");
        struct smtype *mtype=mtypes;
        while ((mtype->mtype!=t_none)&&strcasecmp(mname, mtype->name))
            mtype++;
        if (mtype->mtype==t_none) {
            cout<<"illegal module type "<<mname<<", known module types are: ";
            mtype=mtypes;
            while (mtype->mtype!=t_none) {
                cout<<mtype->name<<" ";
                mtype++;
            }
            cout<<endl;
            return -1;
        }
        selected_mtype=mtype->mtype;
    }

    return 0;
}
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
 */
static int
read_record(int p, u_int32_t** buf)
{
    u_int32_t head[2], *b;
    u_int32_t size;
    int wenden;

    switch (xread(p, reinterpret_cast<char*>(head), 8)) {
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

    b=new u_int32_t[size+1];
    if (!b) {
        cout<<"new buf: "<<strerror(errno)<<endl;
        return -1;
    }

    b[0]=head[0];
    b[1]=head[1];
    switch (xread(p, reinterpret_cast<char*>(b+2), (size-1)*4)) {
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
            u_int32_t v=b[i];
            b[i]=swap_int(v);
        }
    }

    statist.records++;
    cout<<"### got record "<<statist.records<<" size "<<size+1
        <<" word "<<statist.words<<endl;
    statist.words+=size+1;
    //for (int i=0; i<6; i++) printf("0x%08x\n", b[i]);
    *buf=b;
    return size+1;
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
#if 1
static void
dump_raw(u_int32_t *p, int num, const char* text)
{
    printf("%s:\n", text);
    for (int i=0; i<num; i++) {
        printf("[%2d] 0x%08x\n", i, p[i]);
    }
}
#endif

static void
dump_unknown(u_int32_t v)
{
    printf("dump_unknown\n");
}

static void
dump_scaler(u_int32_t v)
{
    printf("dump_scaler (unimplemeted)\n");
}

static void
dump_time(u_int32_t v)
{
    printf("dump_time (unimplemeted)\n");
}

static void
dump_tdc(u_int32_t v)
{
    printf(" %x %02x %02x %04x\n", (v>>28)&0xf, (v>>22)&0x3f, (v>>16)&0x3f,
            v&0xffff);
}

static void
dump_qdc(u_int32_t v)
{
    int selector=(v>>16)&0x3f;
    int val=v&0xffff;
    if (val&0x8000)
        val-=0x10000;
    printf(" %x %x %02x ", (v>>28)&0xf, (v>>22)&0x3f, selector);
    if (!(selector&0x20)) { // raw
        val=v&0xfff;
        if (val&0x800)
            val-=0x1000;
    } else if ((selector&0x30)==0x20) { // Q
        val=v&0xfffff;
        if (val&0x80000)
            val-=0x100000;
    } else if (selector==0x36) {
        val=v&0x1fff;
        if (val&0x1000)
            val-=0x2000;    
    }
    printf("%d\n", val);
}

static void
dump_lvd(u_int32_t v)
{
    printf(" %08x %02x %08x\n", v, (v>>28)&0xf, v&0xfffffff);
}
//---------------------------------------------------------------------------//
static int dumpcount=10;
static void
dump_subevent(const ems_event *ev, const ems_subevent* sev,
        struct ved_info* info, int idx=-1)
{
    printf("event %d, sev ID %d %s\n", ev->event_nr, sev->sev_id,
            info->comment);
    printf("size=%d\n", sev->size);
    u_int32_t* d=sev->data;
    int n=sev->size, min, max;
    u_int32_t modchan=0xffffffff;

    if (idx<0) {
        min=0;
        max=n;
        if (max>10)
            max=10;
    } else {
        printf("error at %d\n", idx);
        min=idx-20;
        if (min<0)
            min=0;
        max=min+100;
        if (max>n)
            max=n;
        modchan=(d[idx]>>22)&0x3ff;
    }

    for (int i=0; i<4; i++) {
        printf("  [H%d] 0x%08x\n", i, d[i]);
    }
    if (min<4)
        min=4;
    for (int i=min; i<max; i++) {
        int c=' ';
        if (((d[i]>>22)&0x3ff)==modchan)
            c='-';
        if (i==idx)
            c='#';
        printf("%c [%2d] 0x%08x", c, i, d[i]);
        if (info->dump)
            info->dump(d[i]);
    }

    if (--dumpcount<0)
        exit(0);
}
//---------------------------------------------------------------------------//
static int
check_lvds_header(const ems_event *ev, const ems_subevent* sev,
        struct ved_info* info)
{
    u_int32_t* d=sev->data;
    static int count=10;

    if (d[0]!=sev->size-1) {
        printf("illegal datacounter: 0x%08x, sevsize=%d\n", d[0], sev->size);
        return -1;
    }

    if (!(d[1]&0x80000000)) {
        printf("event %d, is %d; illegal header=0x%08x ",
                ev->event_nr, sev->sev_id, d[1]);
        return -1;
    }

    unsigned int bytelength=d[1]&0x0fffffff; /* not sure how many bits */
    if (bytelength!=(sev->size-1)*4) {
        printf("illegal length: sevsize=%d bytes=%d\n",
            sev->size, bytelength);
        return -1;
    }

    if ((count>=0)&&(d[1]&0xf0000000)!=0x80000000) {
        printf("event %d, is %d; header=0x%08x ", ev->event_nr, sev->sev_id,
                d[1]);
        if (d[1]&0x40000000)
            printf("ill_length ");
        if (d[1]&0x20000000)
            printf("full ");
        if (d[1]&0x10000000)
            printf("trigger_lost ");
        if (d[1]&0x08000000)
            printf("error ");
        if (d[1]&0x04000000)
            printf("PLL ");
        printf("\n");
        count--;
    }

    return 0;
}
//---------------------------------------------------------------------------//
static int
parse_err(const ems_event *ev, const ems_subevent* sev, struct ved_info* info)
{
    cout<<"cannot parse IS without valid type information"<<endl;
    return -1;
}
//---------------------------------------------------------------------------//
static int
parse_scaler(const ems_event *ev, const ems_subevent* sev, struct ved_info* info)
{
    cout<<"parse_scaler not (yet) implemented"<<endl;
    return -1;
}
//---------------------------------------------------------------------------//
static int
parse_time(const ems_event *ev, const ems_subevent* sev, struct ved_info* info)
{
    char ts[50];
    struct tm* tm;
    time_t tt;

    if (sev->size!=2) {
        printf("timestamp has wrong size: %d instead of 2\n", sev->size);
        dump_subevent(ev, sev, info);
        return -1;
    }

    tt=sev->data[0];
    tm=localtime(&tt);
    strftime(ts, 50, NOWSTR, tm);
    printf("timestamp: %s %06d\n", ts, sev->data[1]);
    return 0;
}
//---------------------------------------------------------------------------//
struct spect {
    spect();
    unsigned int max;
    unsigned int scale;
    unsigned int s[10000];
    double num;
    double sum;
    double qsum;
    void add(unsigned int val);
    double average(void);
    double sigma(void);
};
spect::spect()
:max(10000), num(0), sum(0), qsum(0)
{
    for (int i=0; i<10000; i++) s[i]=0;
}
void spect::add(unsigned int val)
{
    val/=scale;
    if (val<max) {
        s[val]++;
        sum+=val;
        qsum+=val*val;
        num++;
    }
}
double spect::sigma(void)
{
#if 0
    cout<<"num="<<num<<endl;
    cout<<"sum="<<sum<<endl;
    cout<<"qsum="<<qsum<<endl;
    cout<<"num*qsum="<<num*qsum<<endl;
    cout<<"sum*sum="<<sum*sum<<endl;
    cout<<"num*(num-1)="<<num*(num-1)<<endl;
    cout<<"num*qsum-sum*sum="<<num*qsum-sum*sum<<endl;
    cout<<"num*(num-1)="<<num*(num-1)<<endl;
    cout<<"(num*qsum-sum*sum)/(num*(num-1))="<<(num*qsum-sum*sum)/(num*(num-1))<<endl;
#endif
    return sqrt((num*qsum-sum*sum)/(num*(num-1)));
}
double spect::average(void)
{
    return sum/num;
}

struct multi_spect {
    multi_spect();
    int multi_num;
    struct spect s[4];
    void add(unsigned int val);
};
multi_spect::multi_spect()
:multi_num(4)
{
    int scale=1;
    for (int i=0; i<4; i++) {
        s[i].scale=scale;
        scale*=10;
    }
}
void multi_spect::add(unsigned int val)
{
    for (int i=0; i<multi_num; i++) {
        s[i].add(val);
    }
}

struct synch_statist {
    struct multi_spect total;
    struct multi_spect local[16][8];
};

struct synch_statist sstatist;

static int sum_accepted=0;
static int sum_rejected=0;
//---------------------------------------------------------------------------//
static int
parse_sync(const ems_event *ev, const ems_subevent* sev, struct ved_info* info)
{
    int num=sev->size-4;
    u_int32_t* d=sev->data+4;

    unsigned int numdump=10;
    if (sev->size<numdump)
        numdump=sev->size;
    //dump_raw(sev->data, numdump, "USER parse_sync");
    //cout<<"synch subevent; eventcnt="<<event->event_nr<<endl;

    if (sev->size<4) {
        printf("parse_sync: subevent too short (%d words)\n", sev->size);
        return 0;
    }

    if (check_lvds_header(ev, sev, info)<0) {
        dump_subevent(ev, sev, info);
        return -1;
    }

    if (!info->modtypes) {
        cout<<"!info->modtypes"<<endl;
        return 0;
    }

    for (int i=0; i<num; i++) {
        u_int32_t v=d[i];
        int module=(v>>28)&0xf;
        int trigger=(v>>22)&0xf;
        bool tdt     =v&0x04000000;
        bool rejected=v&0x00100000;
        bool evcount =v&0x00040000;

        switch (info->modtypes[module]) {
        case ZEL_LVD_MSYNCH: {
                int time=v&0xfffff;
                int error=!!(v&0xf00000);
#if SYNCHDEBUG
                printf("M[%x] 0x%08x", module, v);
                if (tdt) {
                    printf(" tdt %f us\n", time*.1);
                } else {
                    printf(" trigger %d at %f us%s\n", trigger, time*.1,
                        rejected?" (rejected)":"");
                }
#endif
                if (tdt) {
                    sstatist.total.add(time);
                    if (error)
                        printf(" tdt %f us (%f us): 0x%08x\n",
                            time*.1, (v&0xFfffff)*.1, v);
                } else {
                    if (rejected)
                        sum_rejected++;
                    else
                        sum_accepted++;
                }
            }
            break;
        case ZEL_LVD_OSYNCH: {
                int val=v&0xffff;
                int error=!!(v&0x10000);
#if SYNCHDEBUG
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
                if (!evcount) {
                    sstatist.local[module][trigger].add(val);
                    if (error)
                        printf(" error in ldt for module %d port %d: 0x%08x\n",
                            module, trigger, v);
                }
            }
            break;
        default:
            cout<<"unexpected module type "<<hex<<info->modtypes[module]
                <<" at addr "<<module<<dec<<" in VED "<<sev->sev_id<<endl;
            return 0;
        }     
    }
    return 0;
}
//---------------------------------------------------------------------------//
static int tdc_cache_v[1024][128];
static int tdc_cache_i[1024][128];
static int tdc_idx[1024];
static bool tdc_err[1024];
static int tdc_err_i[1024];

static int
parse_tdc(const ems_event *ev, const ems_subevent* sev, struct ved_info* info)
{
    int num=sev->size-4;
    u_int32_t* d=sev->data+4;
    bool errors=false;
    int err_idx=0;

    if (check_lvds_header(ev, sev, info)<0) {
        dump_subevent(ev, sev, info);
        return -1;
    }

    // initialise tdc_idx
    for (int i=0; i<1024; i++) {
        tdc_idx[i]=0;
        tdc_err[i]=false;
    }

    // fill cache
    for (int i=0; i<num; i++) {
        u_int32_t v=d[i];
        //int module=(v>>28)&0xf;
        //int channel=(v>>22)&0x3f;
        int modchan=(v>>22)&0x3ff;
        int errs=(v>>16)&0x3f;
        if (errs) {
            printf("tdc_error 0x%02x mod=%x chan=%02x idx=%d\n",
                        errs, (modchan>>6)&0xf, modchan&0x3f, i+4);
            if (!errors)
                err_idx=i+4;
            errors=true;
            continue;
        }

        int val=v&0xffff;
        if (val==0x7fff) {
            if (tdc_err[modchan]) {
                printf("tdc_overflows mod=%x chan=%02x idx=%d and idx=%d\n",
                        (modchan>>6)&0xf, modchan&0x3f,
                        tdc_err_i[modchan]+4, i+4);
                if (!errors)
                    err_idx=tdc_err_i[modchan]+4;
                errors=true;
            }
            tdc_err[modchan]=true;
            tdc_err_i[modchan]=i;
            continue;
        }

        if (val&0x8000)
            val-=0x10000;
        if (tdc_idx[modchan]<128) {
            tdc_cache_v[modchan][tdc_idx[modchan]]=val;
            tdc_cache_i[modchan][tdc_idx[modchan]]=i;
        }
        tdc_idx[modchan]++;
        tdc_err[modchan]=false;
    }

    // check for time inversion
    for (int chan=0; chan<1024; chan++) {
        if (tdc_idx[chan]==0)
            continue;
        for (int i=1; i<tdc_idx[chan]; i++) {
            if (tdc_cache_v[chan][i-1]<=tdc_cache_v[chan][i]) {
                if (!errors) {
                    printf("tdc-error mod=%x chan=%02x idx=%d, 0x%x<->0x%x\n",
                        (chan>>6)&0xf, chan&0x3f,
                        tdc_cache_i[chan][i-1]+4,
                        tdc_cache_v[chan][i-1], tdc_cache_v[chan][i]);
                    err_idx=tdc_cache_i[chan][i-1]+4;
                }
                errors=true;
            }
        }
    }

    if (errors) {
        dump_subevent(ev, sev, info, err_idx);
        info->statist.errors++;
    }

    return 0;
}
//---------------------------------------------------------------------------//
enum qdc_codes {
    q_q,         // 20
    q_win_start, // 30
    q_mean,      // 31
    q_min_time,  // 32
    q_min_level, // 33
    q_max_time,  // 34
    q_max_level, // 35
    q_zero,      // 36
    q_fixed      // 37
};
struct qdc_data {
    qdc_codes code;
    int val;
    int idx;
};
static qdc_data qdc_cache_v[16][16][128];
static int qdc_idx[16][16];

static int
parse_qdc(const ems_event *ev, const ems_subevent* sev, struct ved_info* info)
{
    int num=sev->size-4;
    u_int32_t* d=sev->data+4;
    bool errors=false;
    int err_idx=0;

    if (check_lvds_header(ev, sev, info)<0) {
        dump_subevent(ev, sev, info);
        return -1;
    }

    // initialise qdc_idx
    for (int i=0; i<16; i++) {
        for (int j=0; j<16; j++) {
            qdc_idx[i][j]=0;
        }
    }

    // fill cache
    for (int i=0; i<num; i++) {
        u_int32_t v=d[i];
        int module=(v>>28)&0xf;
        int channel=(v>>22)&0x3f;
        //int modchan=(v>>22)&0x3ff;

        if (channel>15) {
            printf("qdc_ghost_channel %d in module %d idx=%d\n",
                channel, module, i+4);
            if (!errors)
                err_idx=i+4;
            errors=true;
            continue;
        }

        qdc_data *data=&qdc_cache_v[module][channel][qdc_idx[module][channel]];
        if (qdc_idx[module][channel]<128) {
            int selector=(v>>16)&0x3f;
            if (!(selector&0x20)) { // raw data
                if (qdc_idx[module][channel]) {
                    cout<<"qdc: raw data after analysis, idx="<<i+4<<endl;
                    if (!errors)
                        err_idx=i+4;
                    errors=true;
                    break;
                }
                continue;
            }
            if ((selector&0x30)==0x20) { // Q
                int q=v&0xfffff;
                if (q&0x80000)
                    q-=0x100000;
                data->code=q_q;
                data->val=q;
            } else {
                int val=v&0xffff;
                if (val&0x8000)
                    val-=0x10000;
                switch (selector) {
                case 0x30:
                    data->code=q_win_start; break;
                case 0x31:
                    data->code=q_mean; break;
                case 0x32:
                    data->code=q_min_time; break;
                case 0x33:
                    data->code=q_min_level; break;
                case 0x34:
                    data->code=q_max_time; break;
                case 0x35:
                    data->code=q_max_level; break;
                case 0x3d:
                    data->code=q_max_level; break;
                case 0x36:
                    val=v&0x1fff;
                    if (val&0x1000)
                        val-=0x2000;
                    data->code=q_zero;
                    break;
                case 0x37:
                    data->code=q_fixed; break;
                default:
                    printf("illegal selector 0x%2x at idx %d\n", selector, i+4);
                    if (!errors)
                        err_idx=i+4;
                    errors=true;
                }
                data->val=val;
            }
            data->idx=i;
            qdc_idx[module][channel]++;
        }
    }

    // decode and check for errors
    for (int module=0; module<16; module++) {
        for (int channel=0; channel<16; channel++) {
            int N=qdc_idx[module][channel];
            if (!N)
                continue;
            
            qdc_data *ddata=qdc_cache_v[module][channel];

            int idx=0;
            if (ddata[0].code==q_fixed) {
                if (N<4) {
                    printf("not enough data for \"fixed window at idx %d\"\n",
                        ddata[0].idx+4);
                    if (!errors)
                        err_idx=ddata[0].idx+4;
                    errors=true;
                    break;
                }
                if ((ddata[1].code!=q_fixed)||
                    (ddata[2].code!=q_q)||
                    (ddata[3].code!=q_fixed)) {
                        printf("wrong \"fixed window\" sequence at idx %d\n",
                            ddata[0].idx+4);
                        if (!errors)
                            err_idx=ddata[0].idx+4;
                        errors=true;
                }
                idx+=4;
            }

            if (idx>=N)
                continue;

#if 0
            for (; idx<n; idx++) {
                switch (ddata[idx].code) {
                case q_q:
                    n_q++;
                    break;
                case q_zero:
                    n_z++;
                    break;
                default:
                    printf("unexpected selector at idx %d\n", ddata[0].idx+4);
                    if (!errors)
                        err_idx=ddata[0].idx+4;
                    errors=true;
                }
            }
#endif

        }
    }

    //err_idx=0;
    //errors=true;

    if (errors) {
        dump_subevent(ev, sev, info, err_idx);
        info->statist.errors++;
    }

    return 0;
}
//---------------------------------------------------------------------------//

struct private_lvd {
    channel_data cd[256];

    void clear(void) {
        for (int i=0; i<256; i++)
            cd[i].clear();
    }
#if 0
    u_int32_t *rawdata[256];
    int rawnum[256];
    int rawsize[256];
    channel_data cd[256];
#endif
};

#if 0
static private_lvd*
lvd_private_init(struct ved_info* info)
{
    struct private_lvd *p=reinterpret_cast<private_lvd*>(info->p);
    if (p) {
        bzero(p->rawnum, 256*sizeof(u_int32_t));
        return p;
    }

    p=new private_lvd;
    if (!p) {
        cout<<"alloc lvds data: "<<strerror(errno)<<endl;
        return 0;
    }
    bzero(p, sizeof(private_lvd));
    info->p=p;
    return p;
}
#else
static private_lvd*
lvd_private_init(struct ved_info* info)
{
    struct private_lvd *p=reinterpret_cast<private_lvd*>(info->p);
    if (p) {
        p->clear();
        return p;
    }

    p=new private_lvd;
    if (!p) {
        cout<<"alloc qdc data: "<<strerror(errno)<<endl;
        return 0;
    }
    p->clear();
    info->p=p;
    return p;
}
#endif

#if 0
static int
lvd_private_grow(struct ved_info* info, int chan)
{
    struct private_lvd *p=reinterpret_cast<private_lvd*>(info->p);
    int max=p->rawsize[chan];

    //printf("grow chan %d from %d\n", chan, max);

    if (max) {
        u_int32_t *help=p->rawdata[chan];
        p->rawdata[chan]=new u_int32_t[2*max];
        if (!p->rawdata[chan])  {
            cout<<"alloc lvds data: "<<strerror(errno)<<endl;
            return -1;
        }
        bcopy(help, p->rawdata[chan], max*sizeof(u_int32_t));
        delete help;
        p->rawsize[chan]=max*2;
    } else {
        max=1;
        p->rawdata[chan]=new u_int32_t[max];
        if (!p->rawdata[chan])  {
            cout<<"alloc lvds data: "<<strerror(errno)<<endl;
            return -1;
        }
        p->rawsize[chan]=max;
    }
    return 0;
}
#endif

static int32_t
int13int32(u_int32_t v)
{
    int32_t v32;
    int8_t v8;
    // convert signed int13 to int32_t
    v8=v>>5;
    v32=v8;
    v32=(v32&0xffffe000)|(v&0x1fff);
    return v32;
}

static int
decode_time_amp(timeampdata *timeamp, u_int32_t v, const char *txt)
{
    if (timeamp->valid) {
        printf(" %s already set\n", txt);
        return -1;
    } else if (timeamp->half) {
        timeamp->bits=(v>>13)&0x7;
        timeamp->ampl=int13int32(v);
        timeamp->valid=true;
        printf(" time=%d ampl=%d %c %d\n",
                timeamp->time, timeamp->ampl,
                timeamp->bits&1?'O':' ', timeamp->bits);
    } else {
        timeamp->time=v&0xffff;
        timeamp->half=true;
        printf("\n");
    }
    return 0;
}

static int
decode_qdc_chan(struct ved_info* info, int chan)
{
    struct private_lvd *p=reinterpret_cast<private_lvd*>(info->p);
    channel_data *cd=p->cd+chan;
    vtainer<u_int32_t> &rawdata=cd->rawdata;
    int num=rawdata.size();
    u_int32_t v, vx;
    int i54, i50;
    int i;

    printf("*** chan %d (%2x)\n", chan, chan);
    for (i=0; i<num; i++) {
        v=rawdata[i];
        i54=(v>>20)&0x3;
        i50=(v>>16)&0x1f;
        printf("%4d %08x  ", i, v);
        if (i54==2) {
            printf("  integral A\n");
        } else if (i54==3) {
            printf("  integral B\n");
        } else {
            switch (i50) {
            case 0x00: printf("RAW\n");
                {
                    sampledata sd;
                    int32_t v32;
                    int8_t v8;
                    // convert signed int13 to int32_t
                    v8=v>>5;
                    v32=v8;
                    sd.value=(v32&0xffffe000)|(v&0x1fff);
                    sd.overflow=!!(v&0x00020000);
                    cd->samples.append(sd);
                }
                break;
            case 0x01: printf("Scaler");
                vx=v&0xffff;
                if (cd->scaler.valid) {
                    printf(" scaler already set\n");
                    return -1;
                } else if (cd->scaler.half) {
                    cd->scaler.value|=vx<<16;
                    cd->scaler.valid=true;
                    printf(" %u\n", cd->scaler.value);
                } else {
                    cd->scaler.value=vx;
                    cd->scaler.half=true;
                    printf("\n");
                }
                break;
            case 0x02: printf("Trigger --> return -1\n"); return -1;
            case 0x03: printf("free --> return -1\n"); return -1;
            case 0x04:
                if (cd->misc.overruns_valid) {
                    printf(" misc.overruns already set\n");
                    return -1;
                }
                cd->misc.overruns=v&0xffff;
                cd->misc.overruns_valid=true;
                printf("overruns since last readout=%d\n", cd->misc.overruns);
                break;;
            case 0x05: printf("noise");
                if (cd->noise.valid) {
                    printf(" noise already set\n");
                    return -1;
                }
                cd->noise.max=v&0xff;
                cd->noise.tau=(v>>8)&0x3f;
                cd->noise.working=v&0x8000;
                cd->noise.valid=true;
                printf(" %d %d %c\n", cd->noise.max, cd->noise.tau,
                        cd->noise.working?'T':'F');
                break;
            case 0x06: printf("IW Start");
                if (cd->iwstart.valid) {
                    printf(" iwstart already set\n");
                    return -1;
                } else if (cd->iwstart.half) {
                    cd->iwstart.ampl=v&0xffff;
                    cd->iwstart.valid=true;
                    printf(" time=%d baseline=%.4f\n",
                            cd->iwstart.time, cd->iwstart.ampl/16.);
                } else {
                    cd->iwstart.time=v&0xffff;
                    cd->iwstart.half=true;
                    printf("\n");
                }
                break;
            case 0x07: printf("IW start and end");
                if (cd->iwampl.valid) {
                    printf(" iwampl already set\n");
                    //return -1;
                } else if (cd->iwampl.half) {
                    cd->iwampl.end=int13int32(v);
                    cd->iwampl.valid=true;
                    printf(" start=%d end=%d\n",
                            cd->iwampl.start, cd->iwampl.end);
                } else {
                    cd->iwampl.start=int13int32(v);
                    cd->iwampl.half=true;
                    printf("\n");
                }
                break;
            case 0x08: printf("IW time and amplitude of maximum");
                if (decode_time_amp(&cd->iwmax, v, "iwmax")<0)
                    return 0;
                break;
            case 0x09: printf("IW time and amplitude of center");
                if (decode_time_amp(&cd->iwcenter, v, "iwcenter")<0)
                    return -1;
                break;
            case 0x0a: printf("Overruns in IW --> return -1\n"); return -1;
            case 0x0b: printf("amplitude at end of Cluster --> return -1\n"); return -1;
            case 0x0c: printf("Overruns in Start Integral --> return -1\n"); return -1;
            case 0x0d: printf("amplitude at begin of Cluster --> return -1\n"); return -1;
            case 0x0e: printf("amplitude at end of Cluster --> return -1\n"); return -1;
            case 0x0f: printf("Overruns in Cluster --> return -1\n"); return -1;
            case 0x10: printf("SW Window Start");
                if (cd->swstart.valid) {
                    printf(" swstart already set\n");
                    return -1;
                } else if (cd->swstart.half) {
                    cd->swstart.ampl=v&0xffff;
                    cd->swstart.valid=true;
                    printf(" time=%d baseline=%.4f\n",
                            cd->swstart.time, cd->swstart.ampl/16.);
                } else {
                    cd->swstart.time=v&0xffff;
                    cd->swstart.half=true;
                    printf("\n");
                }
                break;
            case 0x11: printf("Minimum --> return -1\n"); return -1;
            case 0x12: printf("Maximum --> return -1\n"); return -1;
            case 0x13:
                if (cd->pulse.end_amplB_valid) {
                    printf("end_amplB already set\n");
                    return -1;
                } else if (cd->pulse.end_amplA_valid) {
                    cd->pulse.end_amplB=v&0xffff;
                    cd->pulse.end_amplB_valid=true;
                    printf("end_amplB=%d\n", cd->pulse.end_amplB);
                } else {
                    cd->pulse.end_amplA=v&0xffff;
                    cd->pulse.end_amplA_valid=true;
                    printf("end_amplA=%d\n", cd->pulse.end_amplA);
                }
                break;
            case 0x14:
                if (cd->misc.gradient_valid) {
                    printf("misc.gradient already set\n");
                    //return -1;
                }
                cd->misc.gradient=v&0xffff;
                cd->misc.gradient_valid=true;
                printf("misc.gradient=%d\n", cd->misc.gradient);
                break;
            case 0x15: printf("Constant Fraction --> return -1\n"); return -1;
            case 0x16:
                if (cd->misc.cross_over_level_valid) {
                    printf("misc.cross_over_level already set\n");
                    //return -1;
                }
                cd->misc.cross_over_level=v&0xffff;
                cd->misc.cross_over_level_valid=true;
                printf("misc.cross_over_level=%d\n", cd->misc.cross_over_level);
                break;
            case 0x17:
                if (cd->misc.cross_under_level_valid) {
                    printf("misc.cross_under_level already set\n");
                    //return -1;
                }
                cd->misc.cross_under_level=v&0xffff;
                cd->misc.cross_under_level_valid=true;
                printf("misc.cross_under_level=%d\n", cd->misc.cross_under_level);
                break;
            case 0x18: printf("zero crossing 1 --> return -1\n"); return -1;
            case 0x19: printf("zero crossing 2 --> return -1\n"); return -1;
            case 0x1a: printf("zero crossing 4 --> return -1\n"); return -1;
            case 0x1b: printf("overruns in integral range --> return -1\n"); return -1;
            case 0x1c: printf("pile-up zero crossing 1 --> return -1\n"); return -1;
            case 0x1d: printf("pile-up zero crossing 2 --> return -1\n"); return -1;
            case 0x1e: printf("pile-up zero crossing 4 --> return -1\n"); return -1;
            case 0x1f: printf("Amplitude at end of Integral range --> return -1\n"); return -1;
            }
        }
    }

#if 0
if (cd->raw.size()>0) {
    printf("raw: %d\n", cd->raw.size());
    for (int i=0; i<cd->raw.size(); i++) {
        printf("%c %d\n", cd->raw[i].valid?'T':'F', cd->raw[i].value);
    }
}
#endif

    return 0;
}

static int
parse_lvd_qdc(const ems_event *ev, const ems_subevent* sev,
        struct ved_info* info)
{
    struct private_lvd *p;
    int num=sev->size-4;
    u_int32_t* d=sev->data+4;
    unsigned int chan;

    cout<<"### parse_lvd"<<endl;

    if (check_lvds_header(ev, sev, info)<0) {
        dump_subevent(ev, sev, info);
        return -1;
    }

    if ((p=lvd_private_init(info))==0)
        return -1;

    // fill channel data
    for (int i=0; i<num; i++) {
        u_int32_t v=d[i];
        int chan=((v>>24)&0xf0) | ((v>>22)&0xf);
        //printf("v=%08x a=%x b=%x chan %2x %d\n",
        //        v, (v>>24)&0xf0, (v>>22)&0xf, chan, chan);
        p->cd[chan].rawdata.append(v);
    }

    // statistics
    unsigned int min=UINT_MAX, max=0, sum=0, empty=0;
    for (chan=0; chan<256; chan++) {
        unsigned int n=p->cd[chan].rawdata.size();
        if (n<min) min=n;
        if (n>max) max=n;
        if (!n) empty++;
        sum+=n;
    }
    printf("min=%u max=%u, sum=%u, empty=%u\n", min, max, sum, empty);

    for (chan=0; chan<256; chan++) {
        if (p->cd[chan].rawdata.size()) {
            if (decode_qdc_chan(info, chan)<0)
                return -1;
        }
    }

    if (empty==256) {
        return 0;
    } else {
        cout<<"empty channels: "<<empty<<"  --> return -1"<<endl;
        return 0;
    }
}
//---------------------------------------------------------------------------//
#if 0
static bool
is_wanted(struct ved_info* info)
{
    if (selected_crate>=0) {
        if (info->crate!=selected_crate)
            return false;
    }
    if (selected_is>=0) {
        if (info->is!=selected_is)
            return false;
    }
    if (selected_mtype!=t_none) {
        enum mtype mtype=info->mtype;
        if (selected_mtype==mtype)
            return true;
        if ((mtype==t_stdc)||(mtype==t_ftdc))
            mtype=t_tdc;
        if ((mtype==t_sqdc)||(mtype==t_fqdc))
            mtype=t_qdc;
        if (selected_mtype==mtype)
            return true;
        return false;
    }
    return true;
}
#else
static bool
is_wanted(struct ved_info* info)
{
    return true;
}
#endif
//---------------------------------------------------------------------------//
static struct ved_info*
find_ved_info(int id)
{
    struct ved_info *info=ved_info;
    while ((info->mtype!=t_none) && (id!=info->is))
        info++;
    if (info->mtype==t_none) {
        cout<<"find_ved_info: unknown IS Id "<<id<<endl;
        return 0;
    } else {
        return info;
    }
}
//---------------------------------------------------------------------------//
static int
parse_subevent(const ems_event *ev, const ems_subevent* sev)
{
// expected structure:
//  d[0] word counter
//  d[1] header0: 0x800size
//  d[2] header1: timestamp
//  d[3] header2: eventnr

    struct ved_info *info;

    cout<<"### got subevent, id="<<sev->sev_id<<", size="<<sev->size<<endl;
    unsigned int numdump=10;
    if (sev->size<numdump)
        numdump=sev->size;

    if ((info=find_ved_info(sev->sev_id))==0)
        return -1;

#if 1
    if (true/*info->mtype==t_sync*/)
        dump_raw(sev->data, numdump, "parse_subevent");
#endif

#if 0
    printf("info=%p\n", info);
    printf("parse=%p\n", info->parse);
    printf("dump=%p\n", info->dump);
#endif

    if (!is_wanted(info))
        return 0;

    if (info->parse) {
        if (info->parse(ev, sev, info)<0)
            return -1;
    }

    info->statist.subevents++;
    info->statist.words+=sev->size;
    if (sev->size>info->statist.maxwords)
        info->statist.maxwords=sev->size;

    return 0;
}
//---------------------------------------------------------------------------//
static int
parse_event(const ems_event* event)
{
#if 0
    static u_int32_t ev_no=0;
    if (event->event_nr!=ev_no+1) {
        cout<<"jump from "<<ev_no<<" to "<<event->event_nr<<endl;
    }
    ev_no=event->event_nr;
#endif

printf("### got event: %p nr=%d\n", event, event->event_nr);
    ems_subevent* sev=event->subevents;
    while (sev) {
        if (parse_subevent(event, sev)<0)
            return -1;
        sev=sev->next;
    }
    statist.events++;
        
    return 0;
}
//---------------------------------------------------------------------------//
static void
prepare_globals()
{
    bzero(&statist, sizeof(statist));

    struct ved_info *info=ved_info;
    while (info->mtype!=t_none) {
        bzero(&info->statist, sizeof(info->statist));
        info->p=0;
        info++;
    }
}
//---------------------------------------------------------------------------//
static void
print_statistics()
{
    cout<<statist.events<<" events"<<endl;

    struct ved_info *info=ved_info;
    while (info->mtype!=t_none) {
        if (info->statist.events>0) {
            unsigned long long len=info->statist.words/info->statist.events;
            cout<<"Crate"<<info->crate<<"(ID "<<info->is<<")"<<": "<<len<<" words"
                <<", max "<<info->statist.maxwords<<endl;
            cout<<"  "<<info->statist.events<<"events, "
                <<info->statist.errors<<" errors"<<endl;
        }
        info++;
    }
    cout<<"accepted events="<<sum_accepted<<endl;
    cout<<"rejected events="<<sum_rejected<<endl;
    cout<<"total deadtime: avr="
        <<sstatist.total.s[0].average()/10.<<" us"
        <<" sigma="
        <<sstatist.total.s[0].sigma()/10.<<" us"
        <<endl;
    for (int i=0; i<4; i++) {
        for (int j=0; j<4; j++) {
            if (sstatist.local[i][j].s[0].num) {
                cout<<"local deadtime ["<<i<<"]["<<j<<"]: avr="
                <<sstatist.local[i][j].s[0].average()/10.<<" us"
                <<" sigma="
                <<sstatist.local[i][j].s[0].sigma()/10.<<" us"
                <<endl;
            }
        }
    }
}
//---------------------------------------------------------------------------//
int
main(int argc, char* argv[])
{
    int num_events=0;

    args=new C_readargs(argc, argv);
    if (readargs()!=0)
        return 1;

    compressed_input input(infile);
    ems_data ems_data(ems_data::unsorted);
    ems_cluster* cluster;
    enum clustertypes last_type;
    u_int32_t *buf;

    if (!input.good()) {
        cout<<"cannot open input file "<<infile<<endl;
        return 1;
    }

    prepare_globals();

#if 1
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
        if (ems_data.texts[i]->key=="masterheader")
            ems_data.texts[i]->dump(1);
        else
            ems_data.texts[i]->dump(0);
    }
#if 0
    for (int i=0; i<ems_data.nr_files; i++) {
        if (ems_data.files[i]->name=="/home/wasa/ems_setup/daq/sync_connections.data")
            ems_data.files[i]->dump(1);
        else if (ems_data.files[i]->name=="/home/dkl/ems_setup/daq/comment")
            ems_data.files[i]->dump(1);
        else if (ems_data.files[i]->name=="/home/dkl/ems_setup/daq/dkl.wad")
            ems_data.files[i]->dump(1);
        else if (ems_data.files[i]->name=="/home/dkl/ems_setup/daq/f1.wad")
            ems_data.files[i]->dump(1);
    }
#endif

// read all remaining clusters
    do {
        ems_event* event;
        int res;

        res=ems_data.parse_cluster(cluster);
        delete cluster;
        if (res<0)
            return 3;
        while (ems_data.get_event(&event)>0) {
            // do something useful with the event
            res=parse_event(event);
            delete event;
            num_events++;
            if (res<0)
                return 4;
        }
//        if (num_events>100000)
//            break;
        res=read_record(input.path(), &buf);
        if (res<=0)
            break;
        cluster=new ems_cluster(buf);
    } while (1);

#else

    while (read_record(input.path(), &buf)>0) {
        cluster=new ems_cluster(buf);
        if (ems_data.parse_cluster(cluster)<0)
            return 2;
        delete cluster;
        while (ems_data.get_event(&event)>0) {
            //cout<<"call parse_event"<<endl;
            if (parse_event(event)<0)
                return 3;
            delete event;
            num_events++;
        }
//        if (num_events>10000)
//            break;
    }
#endif

    print_statistics();

    return 0;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
