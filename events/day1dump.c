/*
 * ems/events/day1dump.c
 * 
 * 2014-May-05 PW
 * 
 * $ZEL$
 */

#define _GNU_SOURCE

#ifdef __linux__
#define LINUX_LARGEFILE O_LARGEFILE
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#else
#define LINUX_LARGEFILE 0
#endif

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <cdefs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>

/* copied from ems/common/clusterformat.h */
enum clustertypes {
    clusterty_events=0,
    clusterty_ved_info=1,
    clusterty_text=2,
    /*clusterty_x_wendy_setup=3,*/
    clusterty_file=4,
    clusterty_no_more_data=0x10000000
};

#define SWAP_32(n) (((n & 0xff000000) >> 24) | \
    ((n & 0x00ff0000) >>  8) | \
    ((n & 0x0000ff00) <<  8) | \
    ((n & 0x000000ff) << 24))

#ifdef WORDS_BIGENDIAN
#define swap_falls_noetig(x,y)
#else
#define swap_falls_noetig(x,y) swap(x,y)
#endif /* WORDS_BIGENDIAN */

enum lCode {
    Lnone=0,
    Lall=0xffff,
    Ldata=1,
    Linfo=2,
    Ldebug=4,
    Lwarning=8,
    Lfatal=16,
};

FILE *fo;
u_int16_t logmask=Lall;
//u_int16_t logmask=Lfatal|Ldata|Lwarning;

#define LOG_(fmt, arg...)         \
    do {                          \
        fprintf(fo, fmt, ## arg); \
        fflush(fo);               \
    } while (0)

#define LOG(code, fmt, arg...)    \
    do {                          \
        if (code&logmask)         \
            LOG_(fmt, ## arg);    \
    } while (0)

#define fLOG(fmt, arg...) LOG(Lfatal, fmt, ## arg)
#define dLOG(fmt, arg...) LOG(Ldata, fmt, ## arg)
#define iLOG(fmt, arg...) LOG(Linfo, fmt, ## arg)

struct cl_record {
    u_int32_t head[3];
    u_int32_t *data;
    int swap;
} record;

struct is_info {
    int IS_ID;
    int importance;
    int decoding_hints;
    u_int64_t maskbit;
};
struct ved_info {
    int VED_ID;
    int nr_is;
    struct is_info *is_infos;
};
struct ems_ved_info {
    int valid;
    //timeval tv;
    //int version;
    int nr_veds;
    struct ved_info *ved_infos;
} vinfo;

struct subevent {
    //struct subevent *next;
    u_int32_t evno;
    u_int32_t trigno;
    u_int32_t ID;
    u_int32_t size;
    u_int32_t *data;
};

struct subevent_queue {
    struct is_info *info;
    u_int32_t last_evnr;
    int nr_sev;
    int nr_empty_sev;
    u_int32_t last_trigger;
    u_int32_t last_val[32];
    int nr_const_trigger;
    int nr_const_val[32];
};

struct subevent_queue *sub_queues;
int nr_subqueues; /* == number of instrumentation systems */

struct statistics {
    int records;
    int subevents;
    int events;
    int words;
} statistics;

int warning=0;
const char *inname=0;
const char *outname=0;
int *sev_selectors=0;
int nr_sev_selectors=0;
int *sev_extinguishers=0;
int nr_sev_extinguishers=0;

/******************************************************************************/
//#define xdrstrlen(p) ((*(p) +3)/4+1)
#define xdrstrclen(p) (*(p))
//#define strxdrlen(s) ((strlen(s) +3)/4+1)

//ems_u32* outstring __P((ems_u32*, const char *));
//ems_u32* outnstring __P((ems_u32*, const char *, int));
//ems_u32* extractstring __P((char *, const ems_u32*));
/*****************************************************************************/
static void
dump(u_int32_t* data, int num)
{
    fprintf(fo, "num: %d\n", num);
    while (num) {
        int i=8;
        while (num && i) {
            fprintf(fo, "%08x ", *data);
            data++; num--; i--;
        }
        fprintf(fo, "\n");
    }
}
/*****************************************************************************/
#if 0
/*
swap konvertiert ein Integerarray der Laenge len mit Adresse adr von Network
in Host-Byteorder (oder andersrum)
*/
static void
swap(ems_u32* adr, unsigned int len)
{
    while (len--) {
        *adr=ntohl(*adr);
        adr++;
    }
}
#endif
/******************************************************************************/
#if 0
static u_int32_t *
extractstring(char *s, const u_int32_t *p)
{
    register int size, len;

    size= *p++;                 /* Stringlaenge im Normalformat = xdrstrclen */
    len=(size+3)>>2;            /* Anzahl der Worte = xdrstrlen-1 */
    swap_falls_noetig((u_int32_t*)p, len);  /* Host-byte-ordering herstellen*/
    memcpy(s, p, size);
    s[size]=0;                            /* String abschliessen */
    swap_falls_noetig((u_int32_t*)p, len);  /* zurueckdrehen, Original ist wiederhergestellt */
    return (u_int32_t*)p+len;             /* Pointer auf naechstes XDR-Element */
}
#endif
/******************************************************************************/
#if 0
static u_int32_t *
xdrstrcdup(char** s, const u_int32_t* ptr)
{
    u_int32_t *help;
    *s=(char*)malloc(xdrstrclen(ptr)+1);
    if (*s)
        help=extractstring(*s, ptr);
    return help;
}
#endif
/******************************************************************************/
#if 0
static int
parity(u_int32_t d)
{
    d ^= d>>16;
    d ^= d>>8;
    d ^= d>>4;
    d ^= d>>2;
    d ^= d>>1;
    return d & 1;
}
#endif
/******************************************************************************/
static void
un_extinguish_sev(int s)
{
    int i, j;
    for (i=0; i<nr_sev_extinguishers; i++) {
        if (sev_extinguishers[i]==s) {
            for (j=i; j<nr_sev_extinguishers-1; j++) {
                sev_extinguishers[j]=sev_extinguishers[j+1];
            }
            nr_sev_extinguishers--;
            break;
        }
    }
}
/******************************************************************************/
static void
un_select_sev(int s)
{
    int i, j;
    for (i=0; i<nr_sev_selectors; i++) {
        if (sev_selectors[i]==s) {
            for (j=i; j<nr_sev_selectors-1; j++) {
                sev_selectors[j]=sev_selectors[j+1];
            }
            nr_sev_selectors--;
            break;
        }
    }
}
/******************************************************************************/
static void
add_sev_selector(int s)
{
    if (nr_sev_extinguishers) {
        un_extinguish_sev(s);
    } else {
        int i, *help=malloc((nr_sev_selectors+1)*sizeof(int));
        for (i=0; i<nr_sev_selectors; i++)
            help[i]=sev_selectors[i];
        free(sev_selectors);
        sev_selectors=help;
        sev_selectors[nr_sev_selectors++]=s;
    }
}
/******************************************************************************/
static void
add_sev_extinguisher(int s)
{
    if (nr_sev_selectors) {
        un_select_sev(s);
    } else {
        int i, *help=malloc((nr_sev_extinguishers+1)*sizeof(int));
        for (i=0; i<nr_sev_extinguishers; i++)
            help[i]=sev_extinguishers[i];
        free(sev_extinguishers);
        sev_extinguishers=help;
        sev_extinguishers[nr_sev_extinguishers++]=s;
    }
}
/******************************************************************************/
static int
sev_is_selected(int ID)
{
    int i;

    /* only one of nr_sev_selectors and nr_sev_extinguishers can be
       nonzero */

    for (i=0; i<nr_sev_extinguishers; i++) {
        if (sev_extinguishers[i]==ID)
            return 0;
    }

    for (i=0; i<nr_sev_selectors; i++) {
        if (sev_selectors[i]==ID)
            return 1;
    }

    /* ID not found:
      nr_sev_extinguishers!=0 : return 1
      nr_sev_selectors!=0     : return 0;
      both !=0                : impossible
      both ==0                : return 1
    */

    return !nr_sev_selectors;
}
/******************************************************************************/
static void
printusage(char* argv0)
{
    fprintf(stderr, "usage: %s [-h] [-q] [-f] [-s IS] [-S IS] [infile [outfile]]\n",
        basename(argv0));
    fprintf(stderr, "  -h: this help\n");
    fprintf(stderr, "  -q: quiet (no informational output)\n");
    fprintf(stderr, "  -f: warnings are fatal\n");
    fprintf(stderr, "  -s: select IS (can occur multiple times)\n");
    fprintf(stderr, "  -S: skip IS (can occur multiple times)\n");
    fprintf(stderr, "  infile: file to be read from, empty or '-' for stdin\n");
    fprintf(stderr, "  outfile: file to be written to, empty or '-' for stdout\n");
}
/******************************************************************************/
static int
readargs(int argc, char* argv[])
{
    int c, err=0, s;

    while (!err && ((c=getopt(argc, argv, "hqs:S:fe:")) != -1)) {
        switch (c) {
        case 'h': printusage(argv[0]); return 1;
        case 'q': logmask=/*Ldata|*/Lfatal|Lwarning; break;
        case 's': s=atoi(optarg); add_sev_selector(s); break;
        case 'S': s=atoi(optarg); add_sev_extinguisher(s); break;
        case 'f': warning=-1; break;
        default: err=1;
        }
    }
    if (err) {
        printusage(argv[0]);
        return -1;
    }

    if (optind<argc) {
        inname=argv[optind];
        optind++;
    }
    if (optind<argc) {
        outname=argv[optind];
        optind++;
    }

    return 0;
}
/******************************************************************************/
static int
decode_timestamp(u_int32_t* data, int size)
{
    struct timeval tv;
    char s[1024];
    struct tm* time;
    time_t tt;

    if (size<2) {
        fLOG("timestamp: too short");
        return -1;
    }
    tv.tv_sec=*data++;
    tv.tv_usec=*data;
    tt=tv.tv_sec;
    time=localtime(&tt);
    strftime(s, 1024, "%c", time);
    fLOG("timestamp (header): %ld:%06ld (%s)\n",
            (long)tv.tv_sec, (long)tv.tv_usec, s);

    return 2;
}
/******************************************************************************/
static int
decode_checksum(u_int32_t* data, int size)
{
    fLOG("checksum (not implemented)\n");
    return -1;
}
/******************************************************************************/
static int
decode_options(u_int32_t* data, int size)
{
    int idx=0, optsize, flags, res;
    static int old_optsize=0;

    if (size<1) {
        fLOG("options: too short\n");
        return -1;
    }
    optsize=data[idx++];
    if (optsize!=old_optsize) {
        fLOG("option size: %d --> %d (record %d)\n", old_optsize, optsize,
                statistics.records);
        old_optsize=optsize;
    }

    if (size<optsize+1) {
        fLOG("options: size=%d, optsize=%d (record %d)\n",
                size, optsize, statistics.records);
        return -1;
    }
    if (!optsize)
        return idx;

    flags=data[idx++];
    if (flags&1) {
        res=decode_timestamp(data+idx, size-idx);
        if (res<0)
            return -1;
        idx+=res;
    }
    if (flags&2) {
        res=decode_checksum(data+idx, size-idx);
        if (res<0)
            return -1;
        idx+=res;
    }
    if (flags&~3) {
        fLOG("options: unknown flags 0x%08x\n", flags);
        return -1;
    }

    return idx;
}
/******************************************************************************/
#if 0 /* it uses C++ code */
static int
parse_file(FILE *po)
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
#endif
/******************************************************************************/
#if 0 /* it uses C++ code */
static int
parse_text(FILE *po)
{
    int size=record.head[0]-2;
    u_int32_t *p;
    int idx, lnr, nr_lines;
    char* s;

    idx=decode_options(po, record.data, size);
    if (idx<0)
        return -1;

    if (size-idx<3) {
        fprintf(po, "cluster_text too short\n");
        return -1;
    }

    // skip flags and fragment_id
    p=record.data+2;

    nr_lines=*p++;
    if (nr_lines<1) {
        fprintf(po, "empty text cluster\n");
        return warning;
    }

    //text->lines=new string[text->nr_lines];

    p=xdrstrcdup(&s, p);
    if (!strncmp(s, "Key: ", 5)) {
        //text->key=text->lines[0].substr(5, string::npos);
        fprintf(po, "text: %s\n", s);
    }
    free(s);

    for (lnr=1; lnr<text->nr_lines; lnr++) {
        p=xdrstrcdup(&s, p);
        text->lines[lnr]=s;
        free(s);
        
    }

    return 0;
}
#endif
/******************************************************************************/
#if 0
static struct ved_info*
find_ved_info(int ved_id)
{
    int i;

    for (i=0; i<vinfo.nr_veds; i++) {
        if (vinfo.ved_infos[i].VED_ID==ved_id)
            return vinfo.ved_infos+i;
    }
    fLOG("find_ved_info: id %d (0x%x) not known\n", ved_id, ved_id);
    return 0;
}
#endif
/******************************************************************************/
static struct subevent_queue*
find_sev_queue(int is_id)
{
    int i;

    for (i=0; i<nr_subqueues; i++) {
        if (sub_queues[i].info->IS_ID==is_id)
            return sub_queues+i;
    }
    fLOG("find_sev_queue: id %d (0x%x) not known\n", is_id, is_id);
    return 0;
}
/******************************************************************************/
static int
decode_is_timestamp(struct subevent *sev, struct subevent_queue *queue)
{
    int res=0;

    struct timeval tv;
    char s[1024];
    struct tm* time;
    time_t tt;

    if (sev->size!=2) {
        fLOG("timestamp: unsupported size %d\n", sev->size);
        return -1;
    }
    tv.tv_sec=sev->data[0];
    tv.tv_usec=sev->data[1];
    tt=tv.tv_sec;
    time=localtime(&tt);
    strftime(s, 1024, "%c", time);
    dLOG("timestamp (data): %ld:%06ld (%s)\n",
            (long)tv.tv_sec, (long)tv.tv_usec, s);

    return res;
}
/******************************************************************************/
#if 0
num: 36
00000001 00000022 40024021 04000034 04010037 04100175 04110077 0408002c 
nr_mod   nr_words head
04090035 0418002f 04190028 04020036 04030034 04120034 041303f7 040a003c 

040b0034 041a002f 041b002c 04040031 04050032 0414002b 0415002b 040c002e 

040d0032 041c002e 041d0029 0406002f 04070029 0416002b 0417002c 040e002f 

040f002c 041e002d 041f0028 c499790a
                           trail 
#endif
static int
decode_MADC32(struct subevent *sev, struct subevent_queue *queue)
{
    u_int32_t *d=sev->data;
    //int geo, code, channel, adc, un, ov, crate, cnt, event;
    u_int32_t head, trail, subheader, mod_id, out_format, resolution,
            words, trigger, chan, oor, val, code;
    int nr_mod, num, i, res=-1;

    dLOG("MADC32\n");
    //dump(sev->data, sev->size);

    nr_mod=*d++;
    num=*d++;
    if (num<2) {
        fLOG("not enough words for header and trailer");
        return res;
    }

    if (nr_mod!=1) {
        fLOG("illegal number of modules: %d\n", nr_mod);
        return res;
    }

    head=d[0];
    trail=d[num-1];

    if (((head>>30)&0x3)!=1) {
        fLOG("no header signature in %08x\n", head);
        return res;
    }
    if (((trail>>30)&0x3)!=3) {
        fLOG("no trailer signature in %08x\n", trail);
        return res;
    }

    subheader=(head>>24)&0x3f; // 6 bit --> 30
    mod_id=(head>>16)&0xff;    //  8 bit --> 24
    out_format=(head>>15)&1;   //  1 bit --> 16
    resolution=(head>>12)&7;   //  3 bit --> 15
    words=(head&0xfff)-1;      // 12 bit

    trigger=trail&0x3fffffff;  // 30 bit

    dLOG("mod %d, form %d, res %d trigger %d\n",
            mod_id, out_format, resolution, trigger);

    if (subheader!=0) {
        fLOG("subheader is %02x\n", subheader);
        return res;
    }

    if (words!=num-2) {
        fLOG("wrong wordcount: %d instead of %d\n", words, num-2);
        return res;
    }

    if (queue->last_trigger==trigger) {
        queue->nr_const_trigger++;
        if (queue->nr_const_trigger>2) {
            fLOG("mod %d: constant trigger %08x\n", mod_id, trigger);
        }
    } else {
        if (queue->last_trigger+1!=trigger) {
            fLOG("mod %d: trigger %d --> %d\n",
                    mod_id, queue->last_trigger, trigger);
        }
        queue->last_trigger=trigger;
        queue->nr_const_trigger=0;
    }

    d++; // skip header
    for (i=0; i<words; i++) {
        u_int32_t word=d[i];
        if (((word>>30)&3)!=0){
            fLOG("no data signature in %08x\n", word);
            return res;
        }
        code=(word>>21)&0x1ff;
        switch (code) {
        case 0x20: // data
            chan=(word>>16)&0x1f; // 5 bit --> 21
                                  // 1 dummy bit --> 16
            oor=(word>>14)&1;     // 1 bit --> 15
            val=word&0x3fff;      // 14 bit
            if (queue->last_val[chan]==val) {
                queue->nr_const_val[chan]++;
                if (queue->nr_const_val[chan]>50) {
                    fLOG("mod %d:%d: constant val %08x\n", mod_id, chan, val);
                }
            } else {
                queue->last_val[chan]=val;
                queue->nr_const_val[chan]=0;
            }
            dLOG("chan %2d oor %d val %5d\n", chan, oor, val);
            break;
        case 0x24: // timestamp
            dLOG("adc timestamp\n");
            break;
        case 0x00: // dummy
            break;
        default:
            fLOG("illegal signature %02x in %08x\n", code, word);
            return res;
        }
        
    }

    res=0;

    return res;
}
/******************************************************************************/
static int
decode_SCALER_32(struct subevent *sev, struct subevent_queue *queue)
{
    int res=0;

    dLOG("SCALER\n");
    if (logmask&Ldata)
        dump(sev->data, sev->size);

    return res;
}
/******************************************************************************/
#if 0
static int
decode_UNKNOWN_yes(struct subevent *sev, struct subevent_queue *queue)
{
    fLOG("UNKNOWN\n");
    dump(sev->data, sev->size);

    return 0;
}
#endif
/******************************************************************************/
#if 0
static int
decode_UNKNOWN_no(struct subevent *sev, struct subevent_queue *queue)
{
    fLOG("UNKNOWN\n");
    dump(sev->data, sev->size);

    return -1;
}
#endif
/******************************************************************************/
struct decoder {
    int ID;
    int (*func)(struct subevent *sev, struct subevent_queue *queue);
    const char *name;
};
struct decoder decolist[]={
    { 10, decode_SCALER_32, "SCALER_32"},
    {  1, decode_MADC32, "MADC32"},
    {  2, decode_MADC32, "MADC32"},
    {  3, decode_MADC32, "MADC32"},
    {  4, decode_MADC32, "MADC32"},
    {  5, decode_MADC32, "MADC32"},
    {  6, decode_MADC32, "MADC32"},
    {100, decode_is_timestamp, "TIMESTAMP"},
    {0, 0, 0}
};
/******************************************************************************/
static int
decode_subevent(u_int32_t *data, struct subevent_queue *queue,
        u_int32_t evno, u_int32_t trigno, int size)
{
    struct subevent sev;
    struct decoder *deco;
    int res=-1;

    /* fill subevent structure */
    sev.evno=evno;
    sev.trigno=trigno;
    sev.data=data;
    sev.ID=queue->info->IS_ID;
    sev.size=size;

    deco=decolist;
    while (deco->ID!=sev.ID && deco->func)
        deco++;

    iLOG("EV %d IS %d %s %d words\n",
            sev.evno, sev.ID,
            deco->name?deco->name:"(unknown)",
            sev.size);

    /* check evnr */
    //if (evno<=queue->last_evnr) {
    //if (abs(evno-queue->last_evnr-1)<2)
    //    evno=queue->last_evnr+1;
    if (evno!=queue->last_evnr+1) {
        iLOG("wrong event no. in IS %d: 0x%x --> 0x%x (%+d)\n",
                queue->info->IS_ID, queue->last_evnr, evno, evno-queue->last_evnr-1);
    }

    queue->last_evnr=evno;
    queue->nr_sev++;
    if (!size)
        queue->nr_empty_sev++;

    if (deco->func)
        res=deco->func(&sev, queue);
    else
        fLOG("no decoder for IS %d found\n", sev.ID);

    return res;
}
/******************************************************************************/
static int
parse_subevent_norm(u_int32_t *data, struct subevent_queue *queue,
        u_int32_t evno, u_int32_t trigno)
{
    return decode_subevent(data+2, queue, evno, trigno, data[1]);
}
/******************************************************************************/
static int
parse_subevent_lvds(u_int32_t *data, struct subevent_queue *queue,
        u_int32_t evno, u_int32_t trigno)
{
    int size=data[1];

#if 0
{
int n=size, i;
if (n>100)
    n=100;

iLOG("IS %d evno %d size %d\n", queue->info->IS_ID, evno, size);
for (i=0; i<n; i++)
    iLOG("[%03d] %08x\n", i, data[i]);
}
#endif

    data+=2; /* skip IS_ID and size */

    while (size>=3) {
        u_int32_t head/*, timestamp*/, evno;
        int evsize;
        head=data[0];
        /*timestamp=data[1];*/
        evno=data[2];
        evsize=(head&0xffff)/4;

        decode_subevent(data, queue, evno, trigno, evsize);

        data+=evsize;
        size-=evsize;
    }

    if (size) {
        fLOG("uncomplete LVDS subevent in IS %d event %d\n",
                queue->info->IS_ID, evno);
        return -1;
    }

    return 0;
}
/******************************************************************************/
static int
parse_subevent(u_int32_t *data, u_int32_t evno, u_int32_t trigno)
{
    int is_id;
    struct subevent_queue *queue;

    is_id=*data;

    if (!sev_is_selected(is_id))
        return 0;

    if ((queue=find_sev_queue(is_id))==0) {
        fLOG("find_sev_queue returned 0\n");
        return -1;
    }

    if (queue->info->decoding_hints&1)
        return parse_subevent_lvds(data, queue, evno, trigno);
    else
        return parse_subevent_norm(data, queue, evno, trigno);
}
/******************************************************************************/
static int
parse_event(u_int32_t *data, int ved_id)
{
    int nr_subevents, i;
    u_int32_t ev_size, sev_size, evno, trigno;
    u_int32_t *data_stop; /* first illegal word */

    ev_size=*data++;
    data_stop=data+ev_size;
    evno=*data++;
    trigno=*data++;
    nr_subevents=*data++;


    for (i=0; i<nr_subevents; i++) {
        sev_size=data[1];
#if 0
iLOG("ved %d evno %d size %d sev %d size %d\n",
ved_id, evno, ev_size, data[0], sev_size);
#endif
        if (data+2+sev_size>data_stop) {
            fLOG("cluster_events too short (C)\n");
            return -1;
        }
if (sev_size) { /* XXX empty subevents ignored (should be optional) */
        if (parse_subevent(data, evno, trigno)<0) {
            fLOG("parse_subevent returned -1\n");
            return -1;
        }
}
        data+=sev_size+2;
    }

    return 0;
}
/******************************************************************************/
static int
parse_events(void)
{
    int size=record.head[0]-2;
    u_int32_t *data=record.data;
    u_int32_t *data_stop=data+size; /* first illegal word */
    int ved_id, evsize, i, nr_events;

    data+=decode_options(record.data, size);

    /* flags */
    if (data[0]!=0) {
        fLOG("flags=%08x\n", data[0]);
    }
    data+=1;

    /* VED_ID */
    ved_id=*data++;
    /* fragment_id */
    data+=1;
    if (data>=data_stop) {
        fLOG("cluster_events too short (A)\n");
        return -1;
    }

    nr_events=*data++;
    for (i=0; i<nr_events; i++) {
        evsize=*data;
        if (data+1+evsize>data_stop) {
            fLOG("cluster_events too short (B)\n");
            fLOG("data+1+evsize=%p\n", data+1+evsize);
            fLOG("data=%p evsize=0x%x data_stop=%p\n",
                    data, evsize, data_stop);
            return -1;
        }
        if (parse_event(data, ved_id)<0) {
            fLOG("parse_event returned -1\n");
            return -1;
        }
        data+=1+evsize;
    }

    return 0;
}
/******************************************************************************/
static int
decode_ved_infos1(u_int32_t* data, int size)
{
    int i, j, idx;
 
    iLOG("ved_info type 1 found\n");
    if (logmask&Ldebug)
        dump(data, size);

    nr_subqueues=0;
    vinfo.nr_veds=*data++;
    vinfo.ved_infos=calloc(vinfo.nr_veds, sizeof(struct ved_info));
    for (i=0; i<vinfo.nr_veds; i++) {
        vinfo.ved_infos[i].VED_ID=*data++;
        vinfo.ved_infos[i].nr_is=*data++;
        vinfo.ved_infos[i].is_infos=
                calloc(vinfo.ved_infos[i].nr_is, sizeof(struct is_info));
	iLOG("Found VED %d:", vinfo.ved_infos[i].VED_ID);
        for (j=0; j<vinfo.ved_infos[i].nr_is; j++) {
            struct is_info *is_info=vinfo.ved_infos[i].is_infos+j;
            is_info->IS_ID=*data++;
            is_info->importance=0;
            is_info->decoding_hints=0;
            is_info->maskbit=1ULL<<nr_subqueues;
            nr_subqueues++;
        }
	iLOG("\n");;
    }

    if (nr_subqueues>64) {
        fLOG("too many instrumentation systems (%d),\n"
                        "  only 64 are allowed\n",  nr_subqueues);
        return -1;
    }

    sub_queues=calloc(nr_subqueues, sizeof(struct subevent_queue));
    idx=0;
    for (i=0; i<vinfo.nr_veds; i++) {
        for (j=0; j<vinfo.ved_infos[i].nr_is; j++) {
            sub_queues[idx].info=&vinfo.ved_infos[i].is_infos[j];
            sub_queues[idx].last_evnr=-1;
            sub_queues[idx].nr_sev=0;
            sub_queues[idx].nr_empty_sev=0;
            idx++;
        }
    }

    vinfo.valid=1;

    return 0;
}
/******************************************************************************/
static int
decode_ved_infos2(u_int32_t* data, int size)
{
    fLOG("ved_info type 2 not implemented\n");
    return -1;
}
/******************************************************************************/
static int
decode_ved_infos3(u_int32_t* data, int size)
{
    int i, j, idx;

    nr_subqueues=0;
    vinfo.nr_veds=*data++;
    vinfo.ved_infos=malloc(vinfo.nr_veds*sizeof(struct ved_info));
    for (i=0; i<vinfo.nr_veds; i++) {
        vinfo.ved_infos[i].VED_ID=*data++;
        vinfo.ved_infos[i].nr_is=*data++;
        vinfo.ved_infos[i].is_infos=
                malloc(vinfo.ved_infos[i].nr_is*sizeof(struct is_info));
	iLOG("Found VED %d:", vinfo.ved_infos[i].VED_ID);
        for (j=0; j<vinfo.ved_infos[i].nr_is; j++) {
            struct is_info *is_info=vinfo.ved_infos[i].is_infos+j;
            is_info->IS_ID=*data++;
            is_info->importance=*data++;
            is_info->decoding_hints=*data++;
            is_info->maskbit=1ULL<<nr_subqueues;
	    int is = is_info->IS_ID;
	    iLOG(" %d hint=%d ",
                is, is_info->decoding_hints);
            nr_subqueues++;
        }
	iLOG("\n");;
    }

    if (nr_subqueues>64) {
        fLOG("too many instrumentation systems (%d),\n"
                        "  only 64 are allowed\n",  nr_subqueues);
        return -1;
    }

    sub_queues=calloc(nr_subqueues, sizeof(struct subevent_queue));
    idx=0;
    for (i=0; i<vinfo.nr_veds; i++) {
        for (j=0; j<vinfo.ved_infos[i].nr_is; j++) {
            sub_queues[idx].info=&vinfo.ved_infos[i].is_infos[j];
            sub_queues[idx].last_evnr=-1;
            sub_queues[idx].nr_sev=0;
            sub_queues[idx].nr_empty_sev=0;
            idx++;
        }
    }

    vinfo.valid=1;

    return 0;
}
/******************************************************************************/
static int
parse_ved_info(void)
{
    int version, idx, res;
    int size=record.head[0]-2;

    idx=decode_options(record.data, size);
    if (idx<0)
        return -1;

    if (size-idx<1) {
        fLOG("cluster_ved_info too short\n");
        return -1;
    }

    if (vinfo.valid) {
        iLOG("second ved_info found, ignored\n");
        return 0;
    }

    version=record.data[idx++];
    switch (version) { /* version */
    case 0x80000001:
        res=decode_ved_infos1(record.data+idx, size-idx);
        break;
    case 0x80000002:
        res=decode_ved_infos2(record.data+idx, size-idx);
        break;
    case 0x80000003:
        res=decode_ved_infos3(record.data+idx, size-idx);
        break;
    default:
        fLOG("cluster_ved_info: unknown version 0x%08x\n", version);
        res=-1;
    }

    return res;
}
/******************************************************************************/
/*
 * parse_record parses a record (==cluster) and calls functions parsing
 * the different cluster types.
 * It returns -1 in case of a fatal error.
 * It returns 0 if it seems to be save to parse the next record even if the
 * actual record could not be parsed.
 */
static int
parse_record(void)
{
    int res=0;

    switch (record.head[2]) {
    case clusterty_events:
        res=parse_events();
        if (res)
            fLOG("parse_events returned %d\n", res);
        break;
    case clusterty_ved_info:
        iLOG("parse ved_info\n");
        res=parse_ved_info();
        if (res)
            fLOG("parse_ved_info returned %d\n", res);
        break;
#if 0
    case clusterty_text:
        iLOG("parse text\n");
        res=parse_text();
        if (res)
            fLOG("parse_text returned %d\n", res);
        break;
#endif
#if 0
    case clusterty_file:
        iLOG("parse file\n");
        res=parse_file();
        if (res)
            fLOG("parse_file returned %d\n", res);
        break;
#endif
    case clusterty_no_more_data: /* no_more_data */
        iLOG("no more data\n");
        res=-1;
    default:
        {/* do nothing */}
    }

    if (record.data)
        free(record.data);
    record.data=0;
    return res;
}
/******************************************************************************/
/*
 * read_record reads a record (==cluster) from a data file in cluster
 * format. Unlike previous versions of similar programs it is only able
 * to read uncompressed files.
 * It returns -1 if the read failed and 1 if the read succeeded.
 * It returns 0 at end of file.
 */
static int
read_record(int pi)
{
    ssize_t res;
    int l;
    /* read three words (size, endian, clustertype) */
    if ((res=read(pi, record.head, 12))!=12) {
        if (res==0) {
            fLOG("read head: end of file\n");
            return 0;
        } else {
            fLOG("read head: %s (res=%lld)\n",
                    strerror(errno), (long long)res);
            return -1;
        }
    }
    switch (record.head[1]) {
    case 0x12345678: record.swap=0; break;
    case 0x78563412: record.swap=1; break;
    default:
        fLOG("unkown endien 0x%08x\n", record.head[1]);
        return -1;
    }
    if (record.swap) {
        record.head[0]=SWAP_32(record.head[0]);
        record.head[1]=SWAP_32(record.head[1]);
        record.head[2]=SWAP_32(record.head[2]);
    }

    l=(record.head[0]-2)*4;
    record.data=malloc(l);
    if (!record.data) {
        fLOG("malloc %d bytes: %s\n", l, strerror(errno));
        return -1;
    }
    if ((res=read(pi, record.data, l))!=l) {
        fLOG("read %d bytes: %s (res=%lld)\n",
                l, strerror(errno), (long long)res);
        return -1;
    }

    if (record.swap) {
        int i;
        for (i=record.head[0]-3; i>=0; i--)
            record.data[i]=SWAP_32(record.data[i]);
    }

    statistics.records++;

#if 0
printf("record %d at word %d\n", statistics.records, statistics.words);
printf("head[0]=%08x\n", record.head[0]);
printf("head[1]=%08x\n", record.head[1]);
dump(record.data, record.head[0]-2);
#endif

    return 1;
}
/******************************************************************************/
static int
cluster_loop(int pi)
{
    int count=0, res=0;
    record.data=0;
    while (!res && --count) {
        if (read_record(pi)>0) {
            res=parse_record();
            if (res)
                fLOG("parse_record returned %d\n", res);
            statistics.words+=record.head[0]+1;
        } else {
            res=-1;
        }
    }

    return res;
}
/******************************************************************************/
int
main(int argc, char* argv[])
{
    int pi, ret=0, i;

    if (readargs(argc, argv))
        return 1;

    statistics.records=0;
    statistics.subevents=0;
    statistics.events=0;
    statistics.words=0;

    /* open the files */
    pi=0;
    if (inname && strcmp(inname, "-")) {
        if ((pi=open(inname, O_RDONLY|LINUX_LARGEFILE, 0))<0) {
            fprintf(stderr, "open \"%s\": %s\n", inname, strerror(errno));
            return 2;
        }
    }

    fo=stdout;
    if (outname && strcmp(outname, "-")) {
        if ((fo=fopen(outname, "w"))==0) {
            fprintf(stderr, "fopen \"%s\": %s\n", outname, strerror(errno));
            close(pi);
            return 2;
        }
    }

    sub_queues=0;
    vinfo.valid=0;

    cluster_loop(pi);

    fLOG("records read: %d\n", statistics.records);
    fLOG("subevents   : %d\n", statistics.subevents);
    fLOG("events      : %d\n", statistics.events);
    fLOG("bytes       : %d\n", statistics.words*4);
    for (i=0; i<nr_subqueues; i++) {
        struct subevent_queue *queue=sub_queues+i;
        fLOG("IS %d: %d events (%d empty)\n",
                queue->info->IS_ID, 
                queue->nr_sev,
                queue->nr_empty_sev);
    }

    if (pi!=0)
        close(pi);
    if (fo!=stdout)
        fclose(fo);
    return ret;
}
/******************************************************************************/
/******************************************************************************/
