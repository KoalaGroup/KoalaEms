/*
 * ems/events++/parse_koala.cc
 * 
 * created 2015-Oct-19 PeWue
 * $ZEL: parse_koala.cc,v 1.5 2016/02/04 12:40:08 wuestner Exp $
 */

/*
 * This program reads ems cluster data from stdin, fills some structures
 * with decoded data and dumps some statistics.
 * It is specialised to read decode the data of the mesytec modules used
 * for the KOALA experiment.
 * When a KOALA event is completely collected a procedure 'use_koala_event'
 * is called, which should consume this data.
 */

#define _DEFAULT_SOURCE

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <limits>       // std::numeric_limits
#include <unistd.h>
#include <fcntl.h>
#include "parse_koala.hxx"
#include "use_koala.hxx"

using namespace std;

#define TIMESTR "%Y-%m-%d %H:%M:%S %Z"

#define U32(a) (static_cast<uint32_t>(a))
#define swap_int(a) ( (U32(a) << 24) | \
                      (U32((a) << 8) & 0x00ff0000) | \
                      (U32((a) >> 8) & 0x0000ff00) | \
                      (U32(a) >>24) )

enum clustertypes {
    clusterty_events=0,
    clusterty_ved_info=1,
    clusterty_text=2,
    clusterty_wendy_setup=3,
    clusterty_file=4,
    clusterty_async_data=5,
    clusterty_async_data2=6, /* includes mqtt */
    clusterty_no_more_data=0x10000000,
};

typedef int (parsefunc)(const uint32_t *buf, int size,
        const struct is_info* info);
static parsefunc parse_scaler;
static parsefunc parse_mxdc32;
static parsefunc parse_time;

struct is_info {
    const char *name;// use defined name for this IS
    parsefunc *parse;// parse function for this IS data
    uint32_t is_id;//should match this IS's id in the wad file
};

struct is_info is_info[]={
    {"scaler",    parse_scaler,   1},
    {"mxdc32",    parse_mxdc32,  10},
    {"timestamp", parse_time,   100},
    {"",          0, static_cast<uint32_t>(-1)}
};

// statistics for a single module
struct mxdc32_statist {
    //struct timeval updated;
    //u_int64_t scanned;  /* how often scan_events was called */
    u_int64_t events;   /* number of event headers found */
    u_int64_t words;    /* number of data words */
    //u_int32_t triggers; /* trigger counter in footer */
    //u_int32_t last_stamp[2];
    //bool last_stamp_valid[2];
};

// data for a single module including a linked list of events
struct mxdc32_private {
    mxdc32_private():last_time(0), prepared(0), first(0), last(0) {};
    struct mxdc32_event* prepare_event(int);
    void store_event(void);
    struct mxdc32_event* drop_event(void);
    //u_int32_t marking_type;
    int64_t last_time;
    //int64_t last_diff;
    //int64_t offset;
    struct mxdc32_event *prepared;
    struct mxdc32_event *first;
    struct mxdc32_event *last;
    struct mxdc32_statist statist;
};

// depot for empty event structures (to avoid 'delete' and 'new')
struct mxdc32_depot {
    mxdc32_depot(void): first(0), last(0) {};
    struct mxdc32_event* get(void);
    void put(struct mxdc32_event*);
    struct mxdc32_event *first;
    struct mxdc32_event *last;
};

// statistics
struct global_statist {
    uint32_t evclusters; // number of event cluster parsed
    uint32_t events;     // number of ems events parsed(==multievents for mxdc32)
    uint32_t subevents;  // number of ems subevents paresd totally(=events*ISNumber)
    // uint32_t koala_events;
    // uint32_t complete_events;
    // uint32_t without_qdc;
    // uint32_t only_dqc;
};

static int quiet;
static struct event_data event_data;
static struct mxdc32_private mxdc32_private[nr_mesymodules];
static struct global_statist global_statist;
static struct mxdc32_depot depot;
static char* const *files;
static bool use_simplestructure;
static const char* outputfile;
static int max_tsdiff;

//---------------------------------------------------------------------------//
koala_event::koala_event(void) :
 // next(0),
 mesypattern(0)
{
    /* events[] will be filled in collect_koala_event */
}
//---------------------------------------------------------------------------//
koala_event::~koala_event(void)
{
    for (int mod=0; mod<nr_mesymodules; mod++) {
        if (events[mod])
            depot.put(events[mod]);
    }
}
//---------------------------------------------------------------------------//
/*
 * mxdc32_depot stores empty mxdc32_event structures for recycling.
 * mxdc32_depot::get hands out such a structure or allocates a new one
 * the new structure is not cleared or initialised, this is the task
 * of mxdc32_private::prepare_event
 */
struct mxdc32_event*
mxdc32_depot::get(void)
{
    struct mxdc32_event *event;

    if (first) {
        event=first;
        first=event->next;
    } else {
        event=new struct mxdc32_event;
    }
    if (event==0) {
        cout<<"mxdc32_depot:get: cannot allocate new event structure"<<endl;
    }
    return event;
}
//---------------------------------------------------------------------------//
/*
 * mxdc32_depot stores empty mxdc32_event structures for recycling.
 * put stores unused mesytec events inside mxdc32_depot for later use
 */
void
mxdc32_depot::put(struct mxdc32_event *event)
{
    if (!event) {
        cout<<"depot::put: event=0"<<endl;
        return;
    }
    //cout<<"depot::put: store "<<event->seq<<endl;
    event->next=first;
    first=event;
}
//---------------------------------------------------------------------------//
void
mxdc32_event::invalidate(void)
{
    bzero(this, sizeof(*this));
}
//---------------------------------------------------------------------------//
/*
 * prepare_event prepares a new, empty event structure
 */
struct mxdc32_event*
mxdc32_private::prepare_event(int ID)
{
    // if there is an unused event (previous data where invalid), return this
    // otherwise fetch one from the depot
    if (prepared==0)
        prepared=depot.get();
    if (prepared==0) { // no more memory?
        cout<<"cannot create storage for the next event"<<endl;
        return 0;
    }
    prepared->invalidate();
    return prepared;
}
//---------------------------------------------------------------------------//
/*
 * store_event stores the previously prepared event at the end of
 * the liked event list
 */
void
mxdc32_private::store_event(void)
{
    // just a sanity check (can never be true :-)
    if (prepared==0) {
        cout<<"store_event: no 'prepared' event pending"<<endl;
        exit(1);
    }
//cout<<"private::store seq "<<prepared->seq<<endl;
    if (last) {
        last->next=prepared;
        last=prepared;
    } else {
        last=prepared;
        first=prepared;
    }
    prepared=0;
}
//---------------------------------------------------------------------------//
/*
 * store_event releases the first (oldest) event from the event queue
 */
struct mxdc32_event*
mxdc32_private::drop_event(void)
{
    struct mxdc32_event *help;

    /* idiotic, but save */
    if (!first)
        return 0;

    if (last==first)
        last=0;
    help=first;
    first=first->next;

    return help;
}
//---------------------------------------------------------------------------//
static void
printusage(const char* argv0)
{
    fprintf(stderr, "usage: %s [-h] [-q] [-s] [-m max_tsdiff ][-o outputfile] [file ...]\n", argv0);
    fprintf(stderr, "       -h: print this help and exit\n");
    fprintf(stderr, "       -q: do not print messages about nonfatal errors\n");
    fprintf(stderr, "       -s: output simple flat tree structure\n");
    fprintf(stderr, "       -m: QDC included in the DAQ, and max_tsdiff is the max timestamp diff between QDC and other modules (which should be the master gate width/clock width)");
    fprintf(stderr, "       outputfile: the basename of the output files when the data input is stdin");
    fprintf(stderr, "       file ...: one or more ems cluster file(s)\n");
    fprintf(stderr, "            data input may also come from stdin\n");
}
//---------------------------------------------------------------------------//
static int
readargs(int argc, char* const argv[])
{
    int c;
    bool err=false;

    while (!err && ((c=getopt(argc, argv, "hqsm:o:")) != -1)) {
        switch (c) {
        case 'h':
            printusage(argv[0]);
            return 1;
        case 'q':
            quiet++;
            break;
        case 's':
            use_simplestructure=true;
            break;
        case 'm':
          {
            max_tsdiff = atoi(optarg);
            break;
          }
        case 'o':
          outputfile=optarg;
          break;
        default:
            err=true;
        }
    }
    if (err || optind == 1) {
        printusage(argv[0]);
        return -1;
    }

    files=argv+optind;

    return 0;
}

//---------------------------------------------------------------------------//
__attribute__((unused))
static void
dump_event(struct mxdc32_event *event)
{
    printf("%08x %08x %08lx %08lx %c\n",
        event->header,
        event->footer,
        event->ext_stamp,
        event->timestamp,
        event->ext_stamp_valid?'T':'F');
    printf("%08x %08x %08x %08x\n",
        event->data[0], event->data[1], event->data[2], event->data[3]);
    printf("%08x %08x %08x %08x\n",
        event->data[4], event->data[5], event->data[6], event->data[7]);
    printf("%08x %08x %08x %08x\n",
        event->data[8], event->data[9], event->data[10], event->data[11]);
    printf("%08x %08x %08x %08x\n",
        event->data[12], event->data[13], event->data[14], event->data[15]);
}
//---------------------------------------------------------------------------//
/*
 * collect_koala_event collects the mxdc subevents with the same timestamp,
 * creates a more or less usefull structure and throws it away ;-)
 * between the creation and throwing it away the data should be used
 * for filling histograms, analysis or some other usefull things
 *
 * collect_koala_event returns:
 *   -1: error
 *    0: no error and no more stored data available
 *    1: still data available --> call it again!
 * collect_and_use_event is in a loop as long as it returns '1'
 */
#if 1
/*
 * This version of code really collects the koala events
 * (and does not print much).
 *
 * For each completed koala_event a procedure 'use_koala_event' is called.
 * use_koala_event can use the data and/or store the koala events.
 * use_koala_event has to delete each koala event after use.
 */
static int
collect_koala_event(void)
{
    bool empty=true;
    bool notfull=false;

    for (int mod=0; mod<nr_mesymodules; mod++) {
        struct mxdc32_event *event=mxdc32_private[mod].first;
        if (event) {
            empty=false;
        } else {
            notfull=true;
        }
    }
    if (empty) {
      if(!quiet){
        cout<<global_statist.events<<":"<<" event empty"<<endl;
      }
        return 0;
    }
    if (notfull) {
      if(!quiet){
        cout<<global_statist.events<<":"<<" event not full"<<endl;
        cout<<"(lack: ";
        for(int mod=0;mod<nr_mesymodules;mod++){
          struct mxdc32_event *event=mxdc32_private[mod].first;
          if(!event){
            cout<<" "<<mod;
          }
        }
        cout<<")"<<endl;
      }
        return 0;
    }

    /*
     * At this point we know that we have data from all modules and
     * we know the oldest time stamp.
     */

    /* Create a koala_event and store all module data in this event if
       timestamp-oldest<3000.
       This 3000 is specific to this particular KOALA runtime!
       It has to be adjusted for each different module configuration!
     */
    koala_event *koala=new koala_event;
    koala->event_data=event_data;
    for (int mod=0; mod<nr_mesymodules; mod++) {
        koala->mesypattern|=1<<mod;
        koala->events[mod]=mxdc32_private[mod].drop_event();
    }

    if (use_koala_event(koala)<0)
        return -1;

    empty = true;
    for (int mod=0; mod<nr_mesymodules; mod++) {
      struct mxdc32_event *event=mxdc32_private[mod].first;
      if (event) {
        empty=false;
      } else {
        notfull=true;
      }
    }

    return empty?0:1;
}
#endif
//---------------------------------------------------------------------------//
static int
mxdc32_get_idx_by_id(u_int32_t id)
{
    int mod;

    for (mod=0; mod<nr_mesymodules; mod++) {
        if (mesymodules[mod].ID==id) {
            return mod;
        }
    }
    if (!quiet)
        cout<<"mxdc32_get_idx_by_id: ID "<<id<<" not known"<<endl;
    return -1;
}
//---------------------------------------------------------------------------////---------------------------------------------------------------------------//
static int
check_size(const char *txt, int idx, int size, int needed)
{
    if (size>=idx+needed)
        return 0;

    cout<<txt<<" at idx "<<idx<<" too short: "<<needed<<" words needed, "
        <<size-idx<<" available"<<endl;
    return -1;
}
//---------------------------------------------------------------------------//
static void
dump_data(const char *txt, const uint32_t *buf, int size, int idx)
{
    int i;

    printf("data dump: %s\n", txt);

    for (i=max(idx-3, 0); i<size; i++) {
        printf("%c[%5x] %08x\n", i==idx?'>':' ', i, buf[i]);
    }
    printf("\n");
}
//---------------------------------------------------------------------------//
/*
 * parse_options should parse the cluster options (e.g. timestamp)
 * for now it just calculates the size
 */
static int
parse_options(const uint32_t *buf, int size)
{
    int idx=0;
    int32_t optsize;

    if (check_size("options", idx, size, 1)<0)
        return -1;
    optsize=buf[idx++];
    if (check_size("options", idx, size, optsize)<0)
        return -1;
    return optsize+1;
}
//---------------------------------------------------------------------------//
static struct is_info*
find_is_info(uint32_t id)
{
    struct is_info *info=is_info;
    while ((info->is_id>0) && (id!=info->is_id))
        info++;
    if (info->is_id<=0) {
        cout<<"find_is_info: unknown IS Id "<<id<<endl;
        return 0;
    } else {
        return info;
    }
}
//---------------------------------------------------------------------------//
static int
parse_time(const uint32_t *buf, int size, const struct is_info* info)
{
    if (size!=2) {
        printf("timestamp has wrong size: %d instead of 2\n", size);
        dump_data(info->name, buf, size, 0);
        return -1;
    }

    event_data.tv.tv_sec =buf[0];
    event_data.tv.tv_usec=buf[1];
    event_data.tv_valid=true;

#if 0
    char ts[50];
    struct tm* tm;
    time_t tt;

    tt=event_data.tv.tv_sec;
    tm=localtime(&tt);
    strftime(ts, 50, TIMESTR, tm);
    printf("timestamp[%d]: %s %06ld\n", event_data.event_nr,
            ts, event_data.tv.tv_usec);
#endif

    return 0;
}
//---------------------------------------------------------------------------//
static int
parse_scaler(const uint32_t *buf, int size, const struct is_info* info)
{
    int i;

    if (size!=32) {
        printf("scaler data have wrong size: %d instead of 32\n",
                size);
        dump_data(info->name, buf, size, 0);
        return -1;
    }

    for (i=0; i<32; i++) {
        event_data.scaler[i]=buf[i];
    }
    event_data.scaler_valid=true;

#if 0
    printf("scaler:\n");
    for (i=0; i<32; i++) {
        printf("%10x%s", event_data.scaler[i], (i+1)%4?"":"\n");
    }
#endif

    return 0;
}
//---------------------------------------------------------------------------//
static int
parse_mxdc32(const uint32_t *buf, int size, const struct is_info* info)
{
    struct mxdc32_private *priv=0;
    struct mxdc32_event *event=0;
    int mod=-1;

    for (int i=0; i<size; i++) {
        uint32_t d=buf[i];

        switch ((d>>30)&3) {
        case 0x1: { /* header */
                int id=(d>>16)&0xff;
                mod=mxdc32_get_idx_by_id(id);
                if (mod<0) {
                    cout<<"mxdc32 header: private data not found"<<endl;
                    return 0;
                }
                priv=mxdc32_private+mod;
                event=priv->prepare_event(id);
                if (event==0) {
                    cout<<"mxdc32 header: event storage not valid"<<endl;
                    return -1;
                }
                event->header=d;
                event->len=d&0xfff;
                priv->statist.events++;
                priv->statist.words+=event->len;
                event->evnr=priv->statist.events;
            }
            break;
        case 0x0: /* data, ext. timestamp or dummy */
            switch ((d>>22)&0xff) {
            case 0x00: /* dummy */
                /* do nothing */
                break;
            case 0x10: { /* data */
                    if (event==0) {
                        if (!quiet) {
                            cout<<"mxdc32 data: event storage not valid"<<endl;
                        }
                        // break  /*return -1*/;
                        return -1;
                    }
                    if (mod>=0 && mesymodules[mod].mesytype==mesytec_mqdc32) {
                        int chan=(d>>16)&0x1f;
                        if (event->data[chan]) {
                            if (!quiet) {
                                cout<<"QDC: chan "<<chan<<" already filled"
                                    <<endl;
#if 0
                                printf("===> %08x\n", d);
                                for (int i=0; i<32; i++) {
                                    printf("[%2d] %08x\n", i, event->data[i]);
                                }
#endif
                            }
                            return -1;
                        } else {
                            event->data[chan]=d;
                        }
                    } else {
                        int chan=(d>>16)&0x3f;
                        if (chan>33) {
                            printf("channel %d not valid in %08x\n",
                                    chan, d);
                            return -1;
                        }
                        event->data[chan]=d;
                    }
                }
                break;
            case 0x12: { /* extended timestamp */
                    if (event==0) {
                        if (!quiet) {
                            cout<<"mxdc32 timestamp: event storage not valid"
                                <<endl;
                        }
                        // break  /*return -1*/;
                        return -1;
                    }
                    event->ext_stamp=d&0xffff;
                    event->ext_stamp_valid=true;
                }
                break;
            default:
                printf("mxdc32: illegal data word %08x\n", d);
            }
            break;
        case 0x3: { /* footer */
                if (event==0) {
                    if (!quiet) {
                        printf("mxdc32 footer(0x%x): event storage not valid\n",d);
                    }
                    // break  /*return -1*/;
                    return -1;
                }
                event->footer=d;
                int64_t stamp=d&0x3fffffff;
                event->timestamp=stamp;
                if (event->ext_stamp_valid)
                    event->timestamp|=event->ext_stamp<<30;
                priv->store_event();
                event=0;
            		mod=-1;
            }
            break;
        default: /* does not exist */
            printf("mxdc32: illegal data word %08x\n", d);
            break;
        }
    }

    return 0;
}
//---------------------------------------------------------------------------//
static int
parse_subevent(const uint32_t *buf, int size, uint32_t sev_id)
{
    struct is_info *info;

    if ((info=find_is_info(sev_id))==0)
        return -1;

    if (info->parse) {
        if (info->parse(buf, size, info)<0)
            return -1;
    } else {
        cout<<"no parser for SevID "<<sev_id<<" found"<<endl;
    }

    return 0;
}
//---------------------------------------------------------------------------//
static int
parse_event(const uint32_t *buf, int size)
{
    int nr_sev, idx=0;
    int res;

//dump_data("event", buf, min(size, 10), 0);

    event_data.evnr_valid=false;
    event_data.tv_valid=false;
    event_data.scaler_valid=false;
    event_data.bct_valid=false;

    // we need event_number, trigger id and number of subevents
    if (check_size("event", idx, size, 3)<0)
        return -1;
    // store event_number
    event_data.event_nr=buf[idx++];
    event_data.evnr_valid=true;
    // skip trigger id
    idx++;
    nr_sev=buf[idx++];
    for (int i=0; i<nr_sev; i++) {
        int sev_size;
        uint32_t sev_id;

        // we need instrumentation systen ID (==subevent ID) and the size
        if (check_size("event", idx, size, 2)<0)
            return -1;
        sev_id=buf[idx++];
        sev_size=buf[idx++];
        // and we need sev_size words
        if (check_size("event", idx, size, sev_size)<0)
            return -1;
        if (parse_subevent(buf+idx, sev_size, sev_id)<0)
            return -1;
        idx+=sev_size;
    }
    if (idx!=size) {
        cout<<"parse_event: size="<<size<<", used words="<<idx<<endl;
        return -1;
    }

    global_statist.subevents+=nr_sev;

    do {
        res=collect_koala_event();
    } while (res>0);

    return res<0?-1:0;
}
//---------------------------------------------------------------------------//
static int
parse_events(const uint32_t *buf, int size)
{
    int num, nr_events, idx=0;

    if ((num=parse_options(buf, idx+size))<0)
        return -1;
    idx+=num;

    // we need flags, VED_ID, fragment_id and number_of_events
    if (check_size("events", idx, size, 4)<0)
        return -1;
    // skip flags, VED_ID, fragment_id
    idx+=3;

    nr_events=buf[idx++];
    for (int i=0; i<nr_events; i++) {
        int ev_size;
        // we need the event size
        if (check_size("events", idx, size, 1)<0)
            return -1;
        ev_size=buf[idx++];
        // and we need ev_size words
        if (check_size("events", idx, size, ev_size)<0)
            return -1;

        if (parse_event(buf+idx, ev_size)<0)
            return -1;
        idx+=ev_size;
    }
    if (idx!=size) {
        cout<<"parse_events: size="<<size<<", used words="<<idx<<endl;
        return -1;
    }

    global_statist.events+=nr_events;
    return 0;
}
//---------------------------------------------------------------------------//
/*
 * parse_cluster parses an ems cluster
 * the format is defined in server/dataout/cluster/CLUSTER
 * 
 * returns:
 * -1: (fatal) error
 *  0: no more data
 *  1: success
 */
static int
parse_cluster(const uint32_t *buf, int size)
{
    int idx=0;
    clustertypes clustertype;

    idx+=2; // size and endiantest are already known
    if (check_size("cluster", idx, size, 1)<0)
        return -1;
    clustertype=static_cast<clustertypes>(buf[idx++]);
    switch (clustertype) {
    case clusterty_events:
        global_statist.evclusters++;
        if(!quiet)
          cout<<"cluster: events No."<< global_statist.evclusters << endl;
        if (parse_events(buf+idx, size-idx)<0)
            return -1;
        break;
    case clusterty_ved_info:
        // currently ignored
        if (!quiet)
            cout<<"cluster: ved_info"<<endl;
        break;
    case clusterty_text:
        // currently ignored
        if(!quiet)
          cout<<"cluster: text"<<endl;
        break;
    case clusterty_file:
        // currently ignored
        if(!quiet)
          cout<<"cluster: file"<<endl;
        break;
    case clusterty_no_more_data:
        if (!quiet)
            cout<<"no more data"<<endl;
        break;
    case clusterty_wendy_setup:  // no break
    case clusterty_async_data:   // no break
    case clusterty_async_data2:  // no break
    default:
        cout<<"unknown or unhandled clustertype"<<clustertype<<endl;
        return -1;
    }

    return clustertype==clusterty_no_more_data?0:1;
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
 * read_cluster reads one cluster and swaps the words if necessary
 * buf contains one whole cluster, and it is allocated here and must be deleted after use
 *
 * returns:
 * -1: fatal error
 *  0: no more data
 * >0: size of record (in words)
 */
static int
read_cluster(int p, uint32_t** buf)
{
    uint32_t head[2], *b;
    uint32_t size;
    int swap_flag;

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
        swap_flag=0;
        size=head[0];
        break;
    case 0x78563412:
        swap_flag=1;
        size=swap_int(head[0]);
        break;
    default:
        cout<<"unknown endian "<<hex<<showbase<<setw(8)<<setfill('0')<<head[1]
            <<dec<<noshowbase<<setfill(' ')<<endl;
        return -1;
    }

    b=new uint32_t[size+1];
    if (!b) {
        cout<<"new buf: "<<strerror(errno)<<endl;
        return -1;
    }

    b[0]=head[0];
    b[1]=head[1];
    switch (xread(p, reinterpret_cast<char*>(b+2), (size-1)*4)) {
    case -1: // error
        cout<<"read body: "<<strerror(errno)<<endl;
        delete[] b;
        return -1;
    case 0:
        cout<<"read body: unexpected end of file"<<endl;
        delete[] b;
        return -1;
    default: // OK
        break;
    }

    if (swap_flag) {
        for (int i=size; i>=0; i--) {
            u_int32_t v=b[i];
            b[i]=swap_int(v);
        }
    }
    //for (int i=0; i<6; i++) printf("0x%08x\n", b[i]);
    *buf=b;
    return size+1;
}
//---------------------------------------------------------------------------//
static int
parse_file(int p)
{
    int res;

    use_koala_init();

    do {
        u_int32_t *buf;
        int num;

        num=read_cluster(p, &buf);
        if (num<=0)  { /* no buf allocated */
            res=-1;
            break;
        }

        res=parse_cluster(buf, num);

        delete[] buf;

    } while (res>0);

    if(res<0){
      cout << endl;
      cout << "****************************************************" << endl;
      cout << "* FATAL ERROR IN DECODING! PLEASE CHECK YOUR DATA! *" << endl;
      cout << "****************************************************" << endl;
    } 
    else printf("Decoding Successfully!\n");

    // print some statistics here
    if (quiet<2) {
      cout<<"  ems clusters   : "<<setw(8)<<global_statist.evclusters<<endl;
      cout<<"  ems events           : "<<setw(8)<<global_statist.events<<endl;
      cout<<"  ems subevents        : "<<setw(8)<<global_statist.subevents<<endl;
    }
    for (int mod=0; mod<nr_mesymodules; mod++) {
        printf(" experiment events[%d]: %8ld\n", mod, mxdc32_private[mod].statist.events);
    }
    printf("\n");
    for (int mod=0; mod<nr_mesymodules; mod++) {
      printf(" words [%d] (sum/average): %8ld / %.4f\n", mod, mxdc32_private[mod].statist.words,(float)mxdc32_private[mod].statist.words/mxdc32_private[mod].statist.events);
    }

    use_koala_done();

    return res;
}
//---------------------------------------------------------------------------//
static void
prepare_globals()
{
    bzero(&global_statist, sizeof(struct global_statist));
    use_simplestructure=false;
    outputfile="koala_data";
    quiet=0;
    max_tsdiff=2;
}
//---------------------------------------------------------------------------//
int
main(int argc, char* const argv[])
{
    int res;

    prepare_globals();

    if ((res=readargs(argc, argv)))
        return res<0?1:0;

    if (*files) {
        while (*files) {
            int p;
            p=open(*files, O_RDONLY);
            if (p<0) {
                fprintf(stderr, "cannot open \"%s\": %s\n",
                        *files, strerror(errno));
                return 2;
            }

            printf("FILE %s\n", *files);
            use_koala_setup(*files,use_simplestructure, max_tsdiff);
            res=parse_file(p);
            close(p);
            if (res<0)
                break;

            files++;
        }
    } else {
      use_koala_setup(outputfile,use_simplestructure,max_tsdiff);
        res=parse_file(0);
    }

    return res<0?3:0;
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
