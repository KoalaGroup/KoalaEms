/*
 * ems/events++/cl_consistency.cc
 * 
 * created 2014-Dec-16 PW
 */

#include "config.h"

#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstring>
#include <unistd.h>
//#include <ctime>
#include "swap_int.h"
#include "compressed_io.hxx"
#include <readargs.hxx>
#include <clusterformat.h>
#include <versions.hxx>

VERSION("2014-12-16", __FILE__, __DATE__, __TIME__,
"$ZEL$")
#define XVERSION

#define TIMESTR "%Y-%m-%d %H:%M:%S"

/*
 * cl_consistency is used to check the consistency of files in cluster format.
 * According to the ckeck levels only the basic structure or more and more
 * internal structures are tested.
 * level
 *   0: nothing is checked, the file is read and the words are counted
 *   1: only the sequence of clusters is checked (essentially the word counters)
 *   2: basic features of the type specific headers are checked
 *   3: to be defined
 * 
 * 
 */

static C_readargs* args;
static string infile;
static bool dump;
static int level;
static u_int32_t *xbuf;
static size_t xbufsize;

struct statist {
    long records;
    long records_swapped;
    long records_native;
    long records_max;
    long records_min;
    long events;
    long event_max;
    long event_min;
    long subevents;
    long subevent_max;
    long subevent_min;
    long submax;
    long submin;
    long words;
    long lvdevents;
    long lvdevent_max;
    long lvdevent_min;
    long lvdmax;
    long lvdmin;
};
static statist statist;

//---------------------------------------------------------------------------//
static int
readargs(void)
{
    args->addoption("level", "level", -1, "test level", "level");
    args->addoption("dump", "dump", false, "dump in case of error", "dump");
    args->addoption("infile", 0, "-", "input file", "input");

    if (args->interpret()!=0)
        return -1;

    infile=args->getstringopt("infile");
    level=args->getintopt("level");
    dump=args->getboolopt("dump");

    if (args->isdefault("level")) {
        cout<<"no test level given"<<endl;
        return -1;
    }
    if (level<0 || level>2) {
        cout<<"unknown level "<<level<<endl;
        return -1;
    }

    return 0;
}
//---------------------------------------------------------------------------//
static void
prepare_globals()
{
    bzero(&statist, sizeof(statist));
    statist.records_min=LONG_MAX;
    statist.event_min=LONG_MAX;
    statist.subevent_min=LONG_MAX;
    statist.submin=LONG_MAX;
    statist.lvdevent_min=LONG_MAX;
    statist.lvdmin=LONG_MAX;
    xbuf=0;
    xbufsize=0;
}
//---------------------------------------------------------------------------//
static const char*
timestr(struct timeval *tv)
{
    static char ts[60];
    struct tm* tm;
    time_t tt;

    tt=tv->tv_sec;
    tm=localtime(&tt);
    strftime(ts, 50, TIMESTR, tm);
    sprintf(ts+strlen(ts), ".%06lld", static_cast<long long>(tv->tv_usec));
    return ts;
}
//---------------------------------------------------------------------------//
static int
realloc_buf(size_t size)
{
    if (xbufsize>=size)
        return 0;

    delete xbuf;
    xbuf=new u_int32_t[size];
    if (!xbuf) {
        cout<<"new xbuf: "<<strerror(errno)<<endl;
        return -1;
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
                cout<<"xread: "<<strerror(errno)<<endl;
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
 * read_record ist the test for level 1.
 * Word counters and endian are checket for consistency.
 * If one record has a wrong length, the next record will probably be invalid.
 *
 * read_record returns:
 * -1: fatal error
 *  0: no more data
 * >0: size of record (in words)
 */
static int
read_record(int p, u_int32_t** buf)
{
    u_int32_t head[2];
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

    if (xbufsize<size+1) {
        delete xbuf;
        xbuf=new u_int32_t[size+1];
        if (!xbuf) {
            cout<<"new xbuf: "<<strerror(errno)<<endl;
            return -1;
        }
    }
    *buf=xbuf;

    xbuf[0]=head[0];
    xbuf[1]=head[1];
    switch (xread(p, reinterpret_cast<char*>(xbuf+2), (size-1)*4)) {
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
            u_int32_t v=xbuf[i];
            xbuf[i]=swap_int(v);
        }
    }

    size++; // add the length word itself

    statist.words+=size;
    statist.records++;
    if (wenden)
        statist.records_swapped++;
    else
        statist.records_native++;
    if (size>statist.records_max)
        statist.records_max=size;
    if (size<statist.records_min)
        statist.records_min=size;

    cout<<"### got record "<<statist.records<<" size "<<size
        <<" word "<<statist.words<<endl;

    return size;
}
//---------------------------------------------------------------------------//
/*
 * we just want to know whether we can read the file and if its size
 * is a multiple of four
 */
static int
do_level_0(int p)
{
    const size_t size=4096;
    ssize_t res;
    size_t filesize=0;
    bool again=true;

    if (realloc_buf(size)<0)
        return -1;

    do {
        res=read(p, xbuf, size);
        if (res<0) {
            if (errno!=EINTR) {
                cout<<"xread: "<<strerror(errno)<<endl;
                again=false;
            } else {
                res=0; /* no end of file yet! */
            }
        } else if (res==0) {/* end of file */
            cout<<"end of file reached"<<endl;
            again=0;
        } else {
            filesize+=res;
        }
    } while (again);

    cout<<"filesize: "<<filesize<<" bytes, "<<filesize/4<<" words, remainder "
        <<filesize%4<<endl;

    return 0;
}
//---------------------------------------------------------------------------//
static void
dump_it(u_int32_t *p, int size, int num)
{
    int i;

    if (size<num)
        num=size;

    for (i=0; i<num; i++) {
        if (i && (i%4)==0)
            printf("\n");
        printf("%08x ", p[i]);
    }
    printf("\n");
}
//---------------------------------------------------------------------------//
/*
 * All clusters start with an options field.
 * We check its consistency and return the size.
 * 
 * check_options returns:
 * >=0: size in words
 *  -1: error
 */
static int
check_options(u_int32_t *buf, int pos)
{
    u_int32_t *p=buf+pos;
    int size, optsize, optflags;

    size=buf[0]+1-pos;
    if (size<1) {
        cout<<"check_options: no space for optsize ("<<size<<" words)"<<endl;
        return -1;
    }

    p=buf+pos;
    optsize=p[0];
    if (optsize>size-1) {
        cout<<"check_options: optsize("<<optsize<<")>size("<<size-1<<")"<<endl;
        return -1;
    }

    if (optsize==0) {
        // unlikely, but correct
        return optsize+1;
    }
    optflags=p[1];
    if (optflags&1) { // timestamp
        if (optsize<3) {
            cout<<"check_options: no space for timestamp ("<<size<<" words)"
                <<endl;
            return -1;
        }
        struct timeval tv;
        tv.tv_sec=p[2];
        tv.tv_usec=p[3];
        cout<<"Rec Time :"<<timestr(&tv)<<endl;
    }
    if (optflags&2) { // checksum
        // not checked yet; therefore issuing an error
        cout<<"check_options: checksum not decoded yet"<<endl;
        return -1;
    }
    if (optflags&(~3)) { // illegal or unknown
        printf("check_options: illegal flags %08x\n", optflags);
        return -1;
    }

    return optsize+1;
}
//---------------------------------------------------------------------------//
static int
do_sev_timestamp(u_int32_t *p, int size)
{
#if 0
    cout<<"do_sev_timestamp"<<endl;
    dump_it(p, 32, size);
#endif

    if (size!=2) {
        cout<<"illegal size for sev timestamp: "<<size<<endl;
        return -1;
    }

    struct timeval tv;
    tv.tv_sec=p[0];
    tv.tv_usec=p[1];
    cout<<"sev Time :"<<timestr(&tv)<<endl;

    return 0;
}
//---------------------------------------------------------------------------//
static int
do_sev_lvd_sub(u_int32_t *p, int size)
{
    u_int32_t *head=p;
    int lvds_size;

#if 0
    cout<<"do_sev_lvd_sub, size="<<size<<endl;
    dump_it(p, 16, size);
#endif

    if ((head[0]&0xf0000000)!=0x80000000) {
        printf("illegal LVDS header: %08x\n", head[0]);
        dump_it(p, 16, size);
        return -1;
    }

    lvds_size=head[0]&0x0fffffff;
    statist.lvdevents++;
    if (lvds_size>statist.lvdevent_max)
        statist.lvdevent_max=lvds_size;
    if (lvds_size<statist.lvdevent_min)
        statist.lvdevent_min=lvds_size;

    if (lvds_size&3) {
        printf("illegal LVDS size:\n");
        printf("    head[0]: %08x, lvds size: %d, sev size: %d\n",
            head[0], lvds_size/4, size);
        dump_it(p, 16, size);
        return -1;
    }
    lvds_size/=4;
    if (lvds_size>size) {
        printf("illegal LVDS size:\n");
        printf("    head[0]: %08x, lvds size: %d, sev size: %d\n",
            head[0], lvds_size, size);
        dump_it(p, 16, size);
        return -1;
    }

    return lvds_size;
}
//---------------------------------------------------------------------------//
static int
do_sev_lvd(u_int32_t *p, int size)
{
    int res, num=0;

    if (size==0) { // is this legal?
        cout<<"sev lvd: empty"<<endl;
        return 0;
    }

    while (size) {
        num++;
        res=do_sev_lvd_sub(p, size);
        if (res<0)
            return -1;
        p+=res;
        size-=res;
    }

    if (num>statist.lvdmax)
        statist.lvdmax=num;
    if (num<statist.lvdmin)
        statist.lvdmin=num;

    return 0;
}
//---------------------------------------------------------------------------//
static int
do_subevent(u_int32_t *p, int size)
{
    int sevsize, sevID, res;
    if (size<2) {
        cout<<"do_subevent: subevent to short for header: size="<<size<<endl;
        return -1;
    }

    sevID=p[0];
    sevsize=p[1];

#if 0
    cout<<"sev: ID="<<sevID<<" size="<<sevsize<<" (size="<<size<<")"<<endl;
    dump_it(p, 32, size);
#endif

    statist.subevents++;
    if (sevsize>statist.subevent_max)
        statist.subevent_max=sevsize;
    if (sevsize<statist.subevent_min)
        statist.subevent_min=sevsize;

    if (sevsize+2>size) {
        cout<<"do_subevent: subevent too short: size="<<size
            <<" sevsize="<<sevsize<<" ID="<<sevID<<endl;
        return -1;
    }

    p+=2;

    // here we should use the real type of subevent, but we are too lazy to
    // decode clusterty_ved_info...

    switch (sevID) {
    case 1:   // lvd
        res=do_sev_lvd(p, sevsize);
        break;
    case 100: // tiimestamp
        res=do_sev_timestamp(p, sevsize);
        break;
    default:
        cout<<"unknown subevent ID "<<sevID<<endl;
        res=-1;
    }

    if (res<0) {
        cout<<"error in subevent"<<endl;
        return -1;
    }

    return sevsize+2;
}
//---------------------------------------------------------------------------//
static int
do_event(u_int32_t *p_, int size_)
{
    int evsize_, evsize, evno, trigno, nr_subevents, sev, res;
    u_int32_t *p=p_;
    int size=size_;

    if (size_<4) {
        cout<<"do_event: event to short for header: size="<<size_<<endl;
        return -1;
    }

    evsize_=p[0];
    evno=p[1];
    trigno=p[2];
    nr_subevents=p[3];

    statist.events++;
    if (evsize_>statist.event_max)
        statist.event_max=evsize_;
    if (evsize_<statist.event_min)
        statist.event_min=evsize_;
    if (nr_subevents>statist.submax)
        statist.submax=nr_subevents;
    if (nr_subevents<statist.submin)
        statist.submin=nr_subevents;

    if (evsize_+1>size) {
        cout<<"do_event: event too short: size="<<size
            <<" evsize="<<evsize_<<endl;
        cout<<"evno="<<evno<<" trigno="<<trigno
            <<" nr_subevents="<<nr_subevents<<endl;
        return -1;
    }

#if 0
    cout<<"event: size="<<size<<endl;
    cout<<"evsize="<<evsize_<<" evno="<<evno<<" trigno="<<trigno
        <<" nr_subevents="<<nr_subevents<<endl;
#endif

    p+=4;
    size-=4;
    evsize=evsize_-3;

    for (sev=0; sev<nr_subevents; sev++) {
        res=do_subevent(p, size);
        if (res<0) {
            cout<<"do_events: error in subevent "<<sev<<endl;
            return -1;
        }
        p+=res;
        size-=res;
        evsize-=res;
    }

    if (evsize!=0) {
        cout<<"event not exhausted: remaining size: "<<evsize<<endl;
        dump_it(p_, size_, 64);
        return -1;
    }

    return evsize_+1;
}
//---------------------------------------------------------------------------//
static int
do_cl_no_more_data(u_int32_t *buf, int pos)
{
// no_more_data does not contain real data, but we should check
// the options.
    if (check_options(buf, pos)<0) {
        dump_it(buf, buf[0]+1, buf[0]+1);
        return -1;
    }
    return 0;
}
//---------------------------------------------------------------------------//
static int
do_cl_async_data2(u_int32_t *buf, int pos)
{
    u_int32_t *p;
    int size, res;

    res=check_options(buf, pos);
    if (res<0)
        return -1;

    pos+=res;
    size=buf[0]+1-pos;
    p=buf+pos;

    cout<<"dummy: "<<p<<" "<<size<<endl;

    return 0;
}
//---------------------------------------------------------------------------//
static int
do_cl_text(u_int32_t *buf, int pos)
{
#if 0
    u_int32_t *p;
    int size;
#endif
    int res;

    res=check_options(buf, pos);
    if (res<0)
        return -1;

#if 0
    pos+=res;
    size=buf[0]+1-pos;
    p=buf+pos;

    cout<<"dummy: "<<p<<" "<<size<<endl;
#endif

    return 0;
}
//---------------------------------------------------------------------------//
static int
do_cl_file(u_int32_t *buf, int pos)
{
#if 0
    u_int32_t *p;
    int size;
#endif
    int res;

    res=check_options(buf, pos);
    if (res<0)
        return -1;

#if 0
    pos+=res;
    size=buf[0]+1-pos;
    p=buf+pos;

    cout<<"dummy: "<<p<<" "<<size<<endl;
#endif

    return 0;
}
//---------------------------------------------------------------------------//
static int
do_cl_ved_info(u_int32_t *buf, int pos)
{
    u_int32_t *p;
    int size, res;

    res=check_options(buf, pos);
    if (res<0)
        return -1;

    pos+=res;
    size=buf[0]+1-pos;
    p=buf+pos;

    cout<<"dummy: "<<p<<" "<<size<<endl;

    return 0;
}
//---------------------------------------------------------------------------//
static int
do_cl_events(u_int32_t *buf, int pos)
{
    u_int32_t *p;
    int size, res;
    int flags, fragment, nr_events, ev;

    res=check_options(buf, pos);
    if (res<0)
        return -1;

    pos+=res;
    size=buf[0]+1-pos;
    p=buf+pos;
    if (size<4) {
        cout<<"do_cl_events: record too short for header: size="<<size<<endl;
        return -1;
    }

#if 1
    printf("flags=%08x VED=%d fragment=%d events: %d\n",
        p[0], p[1], p[2], p[3]);
#endif
    flags=p[0];
    fragment=p[2];
    if (flags!=0 || fragment!=0) {
        printf("flags or fragment illegal\n");
        printf("flags=%08x VED=%d fragment=%d events: %d\n",
            p[0], p[1], p[2], p[3]);
        return -1;
    }

    nr_events=p[3];
    p+=4;
    size-=4;
    for (ev=0; ev<nr_events; ev++) {
        res=do_event(p, size);
        if (res<0) {
            cout<<"do_cl_events: error in event "<<ev<<endl;
            return -1;
        }
        p+=res;
        size-=res;
    }

    if (size!=0) {
        cout<<"cluster not exhausted: remaining size: "<<size<<endl;
        return -1;
    }

    return 0;
}
//---------------------------------------------------------------------------//
/*
 * Here we decode the record type (==cluster type).
 * If the type is valid a type dependend procedure is called.
 */
static int
do_level_2(u_int32_t *buf)
{
    int ret=-1;

    if (buf[0]<2) {
        cout<<"do_level_2: cluster too short ("<<buf[0]<<" words)"<<endl;
        return -1;
    }

    switch (buf[2]) {
    case clusterty_events:       ret=do_cl_events(buf, 3);       break;
    case clusterty_ved_info:     ret=do_cl_ved_info(buf, 3);     break;
    case clusterty_text:         ret=do_cl_text(buf, 3);         break;
    case clusterty_file:         ret=do_cl_file(buf, 3);         break;
    //case clusterty_wendy_setup:  ret=do_cl_wendy(buf, 3);        break;
    //case clusterty_async_data:   ret=do_cl_async_data(buf, 3);   break;
    case clusterty_async_data2:  ret=do_cl_async_data2(buf, 3);  break;
    case clusterty_no_more_data: ret=do_cl_no_more_data(buf, 3); break;
    default:
        printf("unknown cluster type %08x\n", buf[2]);
        ret=-1;
    }

    return ret;
}
//---------------------------------------------------------------------------//
int
main(int argc, char* argv[])
{
    int res=0, ret=2;

    prepare_globals();

    args=new C_readargs(argc, argv);
    if (readargs()!=0)
        return 1;
    compressed_input input(infile);

    // level 0
    // because it is really trivial we do it in a seperate function
    if (level<1) {
        res=do_level_0(input.path());
        ret=res?2:0;
        goto error;
    }

    // level 1
    // read clusters (==records) until end of file or error
    do {
        u_int32_t *buf;
        /* read_record returns the record size in words */
        res=read_record(input.path(), &buf);
        if (res==0) {
            cout<<"end of file reached"<<endl;
            break;
        }
        if (res<0)
            goto error;

        if (level>1) {
            res=do_level_2(buf);
            if (res<0)
                goto error;
        }
        

    } while (1);

    cout<<"records          : "<<statist.records<<endl;
    cout<<"records, native  : "<<statist.records_native<<endl;
    cout<<"records, swapped : "<<statist.records_swapped<<endl;
    cout<<"smallest record  : "<<statist.records_min<<" words"<<endl;
    cout<<"largest  record  : "<<statist.records_max<<" words"<<endl;
    cout<<"events           : "<<statist.events<<endl;
    cout<<"smallest event   : "<<statist.event_min<<" words"<<endl;
    cout<<"largest  event   : "<<statist.event_max<<" words"<<endl;
    cout<<"subevents        : "<<statist.subevents<<endl;
    cout<<"smallest subevent: "<<statist.subevent_min<<" words"<<endl;
    cout<<"largest  subevent: "<<statist.subevent_max<<" words"<<endl;
    cout<<"min subev/event  : "<<statist.submin<<endl;
    cout<<"max subev/event  : "<<statist.submax<<endl;
    cout<<"lvdevents        : "<<statist.lvdevents<<endl;
    cout<<"smallest lvdevent: "<<statist.lvdevent_min<<" words"<<endl;
    cout<<"largest  lvdevent: "<<statist.lvdevent_max<<" words"<<endl;
    cout<<"min lvd/subevent : "<<statist.lvdmin<<endl;
    cout<<"max lvd/subevent : "<<statist.lvdmax<<endl;

    ret=0;

error:
    return ret;
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
