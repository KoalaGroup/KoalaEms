/*
 * ems/events++/parse_lvd.cc
 * 
 * created 2006-May-03 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <errno.h>
#include <math.h>
#include "modultypes.h"
#include "compressed_io.hxx"
#include "cluster_data.hxx"
#include <readargs.hxx>
#include <versions.hxx>

VERSION("2007-Feb-15", __FILE__, __DATE__, __TIME__,
"$ZEL: parse_lvd.cc,v 1.5 2010/09/04 21:21:27 wuestner Exp $")
#define XVERSION

#define swap_int(a) ( ((a) << 24) | \
                      (((a) << 8) & 0x00ff0000) | \
                      (((a) >> 8) & 0x0000ff00) | \
        ((unsigned int)(a) >>24) )

enum mtype {
    t_none, t_unknown, t_scaler, t_timestamp,
    t_tdc, t_stdc, t_ftdc, t_qdc, t_sqdc, t_fqdc, t_sync
};

struct ved_statist {
    unsigned long long events;
    unsigned long long words;
    unsigned long long maxwords;
    int errors;
};

struct ved_info {
    char *ved;
    enum mtype mtype;
    int crate;
    int is;
    ems_u32* modtypes;
    char *comment;
    struct ved_statist statist;
};

#if 1
ems_u32 modtypes_2000[16]={
    ZEL_LVD_MSYNCH,
    ZEL_LVD_OSYNCH,
    ZEL_LVD_OSYNCH,
    ZEL_LVD_OSYNCH,
    ZEL_LVD_OSYNCH,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
struct ved_info ved_info[]={
    {"w08",    t_sqdc,    1,  7, 0, "SEC/SEF right"},
    {"w14",    t_sqdc,    2,  8, 0, "SEC right"},
    {"w17",    t_sqdc,    3,  4, 0, "SEC/SEF left"},
    {"w18",    t_sqdc,    4,  6, 0, "SEC left"},
    {"w07",    t_sqdc,    5, 13, 0, "SEB/SESum"},
    {"w16",    t_fqdc,   11,  5, 0, "Plastics QDC"},
    {"w15",    t_fqdc,   12,  9, 0, "Plastics QDC"},
    {"w10",    t_ftdc,   13,  3, 0, "Plastics TDC"},
    {"w01",    t_sync,   14, 2000, modtypes_2000, "sync"},
    {"w04",    t_stdc,   21, 14, 0, "F1/FPC1"},
    {"w12",    t_stdc,   22, 11, 0, "F1/MDC1"},
    {"w11",    t_stdc,   23, 10, 0, "F1/MDC2"},
    {"w13",    t_stdc,   24, 12, 0, "F1/MDC3"},
    {"w05",    t_stdc,   25, 15, 0, "F1/FPC2"},
    {"w09",    t_stdc,   26,  2, 0, "F1/FPC3"},
    {"scaler", t_timestamp, 31, 100, 0, "timestamp"},
    {"scaler", t_scaler, 31, 101, 0, "scaler"},
    {"scaler", t_scaler, 31, 102, 0, "bct"},
    {"w06",    t_stdc,   -1, 50, 0, "F1/pellet structure test"},
    {"",       t_none, -1, -1, 0, "nix"}
};
#endif
#if 0
struct ved_info ved_info[]={
    {"q15",    t_sync,  0,  1, "Sync"},
    {"",       t_none, -1, -1, "nix"}
};
#endif

C_readargs* args;
STRING infile;
enum mtype selected_mtype;
int selected_crate, selected_is;

ems_event* event;

struct statist {
    int events;
};

struct statist statist;

//---------------------------------------------------------------------------//
struct smtype {
    char *name;
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

int readargs()
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
            cerr<<"illegal module type "<<mname<<", known module types are: ";
            mtype=mtypes;
            while (mtype->mtype!=t_none) {
                cerr<<mtype->name<<" ";
                mtype++;
            }
            cerr<<endl;
            return -1;
        }
        selected_mtype=mtype->mtype;
    }

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
static void
dump_unknown(ems_u32 v)
{
    printf("dump_unknown\n");
}
static void
dump_scaler(ems_u32 v)
{
    printf("dump_scaler\n");
}
static void
dump_tdc(ems_u32 v)
{
    printf(" %x %02x %02x %04x\n", (v>>28)&0xf, (v>>22)&0x3f, (v>>16)&0x3f,
            v&0xffff);
}
static void
dump_qdc(ems_u32 v)
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
//---------------------------------------------------------------------------//
static int dumpcount=10;
static void
dump_subevent(const ems_subevent* sev, struct ved_info* info, int idx=-1)
{
    printf("event %d, sev ID %d %s\n", event->event_nr, sev->sev_id,
            info->comment);
    printf("size=%d\n", sev->size);
    ems_u32* d=sev->data;
    int n=sev->size, min, max;
    void (*proc)(ems_u32)=dump_unknown;
    ems_u32 modchan=0xffffffff;

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

    switch (info->mtype) {
    case t_scaler:
        proc=dump_scaler;
        break;
    case t_tdc:
    case t_stdc:
    case t_ftdc:
        proc=dump_tdc;
        break;
    case t_qdc:
    case t_sqdc:
    case t_fqdc:
        proc=dump_qdc;
        break;
    case t_sync:
    case t_timestamp:
    case t_unknown:
    case t_none:
        proc=dump_unknown;
        break;
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
        proc(d[i]);
    }

    if (--dumpcount<0)
        exit(0);
}
//---------------------------------------------------------------------------//
static int
check_lvds_header(const ems_subevent* sev, struct ved_info* info)
{
    ems_u32* d=sev->data;
    static int count=10;

    if (d[0]!=sev->size-1) {
        printf("illegal datacounter: 0x%08x, sevsize=%d\n", d[0], sev->size);
        return -1;
    }

    if (!(d[1]&0x80000000)) {
        printf("event %d, is %d; illegal header=0x%08x ",
                event->event_nr, sev->sev_id, d[1]);
        return -1;
    }

    unsigned int bytelength=d[1]&0x0000ffff;
    if (bytelength!=(sev->size-1)*4) {
        printf("illegal length: sevsize=%d bytes=%d\n",
            sev->size, bytelength);
        return -1;
    }

    if ((count>=0)&&(d[1]&0xffff0000)!=0x80000000) {
        printf("event %d, is %d; header=0x%08x ", event->event_nr, sev->sev_id,
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
parse_scaler(const ems_subevent* sev, struct ved_info* info)
{
    cerr<<"parse_scaler not (yet) implemented"<<endl;
    return -1;
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
parse_sync(const ems_subevent* sev, struct ved_info* info)
{
    int num=sev->size-4;
    ems_u32* d=sev->data+4;

    unsigned int numdump=10;
    if (sev->size<numdump)
        numdump=sev->size;
    //dump_raw(sev->data, numdump, "USER parse_sync");
    //cout<<"synch subevent; eventcnt="<<event->event_nr<<endl;

    if (sev->size<4) {
        printf("parse_sync: subevent too short (%d words)\n", sev->size);
        return 0;
    }

    if (check_lvds_header(sev, info)<0) {
        dump_subevent(sev, info);
        return -1;
    }

    if (!info->modtypes) {
        cout<<"!info->modtypes"<<endl;
        return 0;
    }

    for (int i=0; i<num; i++) {
        ems_u32 v=d[i];
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
            cerr<<"unexpected module type "<<hex<<info->modtypes[module]
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
parse_tdc(const ems_subevent* sev, struct ved_info* info)
{
    int num=sev->size-4;
    ems_u32* d=sev->data+4;
    bool errors=false;
    int err_idx=0;

    if (check_lvds_header(sev, info)<0) {
        dump_subevent(sev, info);
        return -1;
    }

    // initialise tdc_idx
    for (int i=0; i<1024; i++) {
        tdc_idx[i]=0;
        tdc_err[i]=false;
    }

    // fill cache
    for (int i=0; i<num; i++) {
        ems_u32 v=d[i];
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
        dump_subevent(sev, info, err_idx);
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
parse_qdc(const ems_subevent* sev, struct ved_info* info)
{
    int num=sev->size-4;
    ems_u32* d=sev->data+4;
    bool errors=false;
    int err_idx=0;

    if (check_lvds_header(sev, info)<0) {
        dump_subevent(sev, info);
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
        ems_u32 v=d[i];
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
        dump_subevent(sev, info, err_idx);
        info->statist.errors++;
    }

    return 0;
}
//---------------------------------------------------------------------------//
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
//---------------------------------------------------------------------------//
static struct ved_info*
find_ved_info(int id)
{
    struct ved_info *info=ved_info;
    while ((info->mtype!=t_none) && (id!=info->is))
        info++;
    if (info->mtype==t_none) {
        cerr<<"find_ved_info: unknown IS Id "<<id<<endl;
        return 0;
    } else {
        return info;
    }
}
//---------------------------------------------------------------------------//
static int
parse_subevent(const ems_subevent* sev)
{
// expected structure:
//  d[0] word counter
//  d[1] header0: 0x800size
//  d[2] header1: timestamp
//  d[3] header2: eventnr

    struct ved_info *info;
    int res=0;

    //cout<<"### got subevent, data="<<sev->data<<", size="<<sev->size<<endl;
    unsigned int numdump=10;
    if (sev->size<numdump)
        numdump=sev->size;

    if ((info=find_ved_info(sev->sev_id))==0)
        return -1;

#if 0
    if (info->mtype==t_sync)
        dump_raw(sev->data, numdump, "USER parse_subevent");
#endif

    if (!is_wanted(info))
        return 0;

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
//---------------------------------------------------------------------------//
static int
parse_event(const ems_event* event)
{
    static ems_u32 ev_no=0;
//     if (event->event_nr!=ev_no+1) {
//         cout<<"jump from "<<ev_no<<" to "<<event->event_nr<<endl;
//     }
    ev_no=event->event_nr;

//printf("### got event\n");
    ems_subevent* sev=event->subevents;
    while (sev) {
    if (parse_subevent(sev)<0)
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
    statist.events=0;

    struct ved_info *info=ved_info;
    while (info->mtype!=t_none) {
        info->statist.events=0;
        info->statist.words=0;
        info->statist.maxwords=0;
        info->statist.errors=0;
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
    ems_u32 *buf;

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
        //else
            //ems_data.texts[i]->dump(0);
    }
    for (int i=0; i<ems_data.nr_files; i++) {
        if (ems_data.files[i]->name=="/home/wasa/ems_setup/daq/sync_connections.data")
            ems_data.files[i]->dump(1);
        //else
            //ems_data.files[i]->dump(0);
    }

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
        if (num_events>100000)
            break;
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
        if (num_events>10000)
            break;
    }
#endif

    print_statistics();

    return 0;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
