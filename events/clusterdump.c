/*
 * ems/events/clusterdump.c
 * 
 * 2008-Aug-10 PW
 * 
 * $ZEL: clusterdump.c,v 1.4 2010/09/04 21:25:59 wuestner Exp $
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
};

struct subevent_queue *sub_queues;
int nr_subqueues; /* == number of instrumentation systems */

struct statistics {
    int records;
    int subevents;
    int events;
} statistics;

int quiet=0;
int warning=0;
const char *inname=0;
const char *outname=0;
const char *experiment=0;
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
    fprintf(stderr, "usage: %s [-h] [-e experiment] [-q] [-f] [-s IS] [-S IS] [infile [outfile]]\n",
        basename(argv0));
    fprintf(stderr, "  -h: this help\n");
    fprintf(stderr, "  -e: name of experimenteriment (tof)|(wasa)|(anke)|(pax)\n");
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
        case 'q': quiet=1; break;
        case 's': s=atoi(optarg); add_sev_selector(s); break;
        case 'S': s=atoi(optarg); add_sev_extinguisher(s); break;
        case 'f': warning=-1; break;
        case 'e': experiment=optarg; break;
        default: err=1;
        }
    }
    if (err) {
        printusage(argv[0]);
        return -1;
    }

    if (!experiment) {
        fprintf(stderr, "name of experimenteriment is required\n"
                "use -e tof|wasa|anke|pax\n");
        return -1;
    }
    if (strcmp(experiment, "tof")) {
        fprintf(stderr, "experiment \"%s\" is not implemented yet\n", experiment);
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
decode_timestamp(FILE *po, u_int32_t* data, int size)
{
    struct timeval tv;
    char s[1024];
    struct tm* time;
    time_t tt;

    if (size<2) {
        fprintf(po, "timestamp: too short");
        return -1;
    }
    tv.tv_sec=*data++;
    tv.tv_usec=*data;
    tt=tv.tv_sec;
    time=localtime(&tt);
    strftime(s, 1024, "%c", time);
    fprintf(po, "timestamp: %ld:%06ld (%s)\n",
            (long)tv.tv_sec, (long)tv.tv_usec, s);
    return 2;
}
/******************************************************************************/
static int
decode_checksum(FILE *po, u_int32_t* data, int size)
{
    fprintf(po, "checksum (not implemented)\n");
    return -1;
}
/******************************************************************************/
static int
decode_options(FILE *po, u_int32_t* data, int size)
{
    int idx=0, optsize, flags, res;

    if (size<1) {
        fprintf(po, "options: too short\n");
        return -1;
    }
    optsize=data[idx++];
    if (size<optsize+1) {
        fprintf(po, "options: size=%d, optsize=%d\n", size, optsize);
        return -1;
    }
    if (!optsize) return idx;
    flags=data[idx++];
    if (flags&1) {
        res=decode_timestamp(po, data+idx, size-idx);
        if (res<0)
            return -1;
        idx+=res;
    }
    if (flags&2) {
        res=decode_checksum(po, data+idx, size-idx);
        if (res<0)
            return -1;
        idx+=res;
    }
    if (flags&~3) {
        fprintf(po, "options: unknown flags 0x%08x\n", flags);
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
find_ved_info(FILE *po, int ved_id)
{
    int i;

    for (i=0; i<vinfo.nr_veds; i++) {
        if (vinfo.ved_infos[i].VED_ID==ved_id)
            return vinfo.ved_infos+i;
    }
    fprintf(po, "find_ved_info: id %d (0x%x) not known\n", ved_id, ved_id);
    return 0;
}
#endif
/******************************************************************************/
static struct subevent_queue*
find_sev_queue(FILE *po, int is_id)
{
    int i;

    for (i=0; i<nr_subqueues; i++) {
        if (sub_queues[i].info->IS_ID==is_id)
            return sub_queues+i;
    }
    fprintf(po, "find_sev_queue: id %d (0x%x) not known\n", is_id, is_id);
    return 0;
}
/******************************************************************************/
static int
decode_run_nr(FILE *po, struct subevent *sev, struct subevent_queue *queue)
{
    u_int32_t *d=sev->data;
    int res=0;

    fprintf(po, "  %4d %08x == %d\n", 0, *d, *d);

    if (sev->size!=1) {
        fprintf(po, "  illegal number of words\n");
        res=-1;
    }

    return 0;
}
/******************************************************************************/
static int
decode_LC_4434(FILE *po, struct subevent *sev, struct subevent_queue *queue)
{
    u_int32_t *d=sev->data;
    u_int64_t low, high, date;
    int res=0, channels, i;

    channels=*d;
    d++;
    fprintf(po, "  %4d %08x (no. of channels)\n", 0, channels);
    if (channels*2+1!=sev->size) {
        fprintf(po, "ERROR: channel count (%d) does not match sev size (%d)\n",
                channels, sev->size);
        res=warning;
    }

    for (i=1; i<sev->size; i++) {
        fprintf(po, "  %4d %08x", i, *d);
        fflush(po);
        if (i&1) {
            low=*d;
        } else {
            high=*d;
            date=(high<<32)|low;
            fprintf(po, " chan %3d: %20llu\n",
                    (i/2)-1, (unsigned long long)date);
        }
        fprintf(po, "\n");
        d++;
    }

    return res;
}
/******************************************************************************/
static int
decode_CAEN_C219(FILE *po, struct subevent *sev, struct subevent_queue *queue)
{
    u_int32_t *d=sev->data;
    int res=0, i;

    for (i=0; i<sev->size; i++) {
        fprintf(po, "  %4d %08x", i, *d);
        fflush(po);
        fprintf(po, "\n");
        d++;
    }

    return res;
}
/******************************************************************************/
static int
decode_ZEL_SYNC(FILE *po, struct subevent *sev, struct subevent_queue *queue)
{
    u_int32_t *d=sev->data;
    u_int32_t wordcount, h0, h1, h2, bytes;
    int i, addr, fragment, res=0;

    if (sev->size<3) {
        fprintf(po, "E subevent too short\n");
        for (i=0; i<sev->size; i++) {
            fprintf(po, "  %4d %08x", i, *d);
        }
        return -1;
    }

    wordcount=*d;
    d++;
    fprintf(po, "  %4d %08x (wordcount)\n", 0, wordcount);
    if (wordcount+1!=sev->size) {
        fprintf(po, "wordcount (%d) does not match sev size (%d)\n",
                wordcount, sev->size);
        res=-1;
    }

    h0=d[0];
    h1=d[1];
    h2=d[2];
    d+=3;

    bytes=h0&0x3ffff;
    fragment=!!(h0&0x40000000);
    fprintf(po, "  %4d %08x head_0 %d bytes%s\n",
            1, h0, bytes, fragment?" fragmented":"");
    fprintf(po, "  %4d %08x head_1 (trigger time) \n",
            2, h1);
    fprintf(po, "  %4d %08x head_2 event %d\n",
            3, h2, h2);

    if (bytes!=(sev->size-1)*4) {
        fprintf(po, "ERROR: byte count does not match subevent length\n");
        res=warning;
    }

    for (i=4; i<sev->size; i++) {
        addr=(*d>>28)&0xf;
        fprintf(po, "  %4d %08x %x", i, *d, addr);
        fflush(po);
        fprintf(po, "\n");
        d++;
    }

    return res;
}
/******************************************************************************/
static int
decode_PHIL_10Cx(FILE *po, struct subevent *sev, struct subevent_queue *queue)
{
    u_int32_t *d=sev->data;
    u_int32_t dd[2];
    int wordcount, uid, geo, chans, event, dummy, channel, date;
    int i, j, res=0;

    fprintf(po, "  %4d %08x (wordcount)\n", 0, *d);
    wordcount=*d;
    d++; 
    if (wordcount+1!=sev->size) {
        fprintf(po, "ERROR: wordcount (%d) does not match sev size (%d)\n",
                wordcount, sev->size);
        res=warning;
    }

    chans=0; /* first word must be a header, not channel data */
    for (i=1; i<sev->size; i++) {
        fprintf(po, "  %4d %08x", i, *d);
        fflush(po);

#if 0 /* simple dump */
        uid=(*d>>24)&0xff;
        geo=(*d>>16)&0x1f;
        cnt=*d&0x7ff;
        chans=(*d>>10)&0x3f;
        event=*d&0x3ff;
        fprintf(po, " uid=%d geo=%d cnt=%d chans=%d event=%d\n",
                uid, geo, cnt, chans, event);
        
        dd[0]=(*d>>16)&0xffff;
        dd[1]=*d&0xffff;

        dummy=!!(dd[0]&0x8000);
        channel=(dd[0]>>10)&0x1f;
        date=dd[0]&0x3ff;
        fprintf(po, "                [0] dummy=%d channel=%d date=%d\n",
                dummy, channel, date);

        dummy=!!(dd[1]&0x8000);
        channel=(dd[1]>>10)&0x1f;
        date=dd[1]&0x3ff;
        fprintf(po, "                [1] dummy=%d channel=%d date=%d\n",
                dummy, channel, date);
#else

        /* we don't have a super header */
#if 0
        super header
        uid=(*d>>24)&0xff;
        geo=(*d>>16)&0x1f;
        cnt=*d&0x7ff;
#endif

        if (!chans) { /* no channels experimentected, mut be a header */
            uid=(*d>>24)&0xff;
            geo=(*d>>16)&0x1f;
            chans=(*d>>10)&0x3f;
            event=*d&0x3ff;
            fprintf(po, " uid=%3d geo=%2d chans=%2d event=%d\n",
                uid, geo, chans, event);
            if (uid!=geo*8) {
                fprintf(po, "ERROR: wrong uid (!=8*geo)\n");
                res=warning;
            }
        } else {
            dd[0]=(*d>>16)&0xffff;
            dd[1]=*d&0xffff;
            for (j=0; j<2; j++) {
                dummy=!!(dd[j]&0x8000);
                channel=(dd[j]>>10)&0x1f;
                date=dd[j]&0x3ff;
                if (j)
                    fprintf(po, "               ");
                if (dummy) {
                    fprintf(po, " dummy\n");
                } else {
                    fprintf(po, " %2d %4d\n", channel, date);
                    chans--;
                }
            }
        }
#endif
        d++;
    }

    return res;
}
/******************************************************************************/
static int
decode_LC_1881M(FILE *po, struct subevent *sev, struct subevent_queue *queue)
{
    u_int32_t *d=sev->data;
    int wordcount, geo, chans, words, buffer, buf, channel, date;
    int i, res=0;

    fprintf(po, "               geo buf chan  adc\n");

    fprintf(po, "  %4d %08x (wordcount)\n", 0, *d);
    wordcount=*d;
    d++;
    if (wordcount+1!=sev->size) {
        fprintf(po, "ERROR: wordcount (%d) does not match sev size (%d)\n",
                wordcount, sev->size);
        res=warning;
    }

    chans=0; /* first word must be a header, not channel data */
    for (i=1; i<sev->size; i++) {
        fprintf(po, "  %4d %08x", i, *d);
        fflush(po);

        if (parity(*d)) {
            fprintf(po, "\nERROR: wrong parity\n");
            res=warning;
            continue;
        }

        geo=(*d>>27)&0x1f;

        if (!chans) {
            buffer=(*d>>7)&0x3f;
            words=*d&0x7f;
            fprintf(po, " header: geo=%2d buffer=%2d words=%3d\n",
                    geo, buffer, words);
            if (!words) {
                fprintf(po, "ERROR: illegal word count\n");
                res=warning;
            }
            chans=words-1;
        } else {
            buf=(*d>>24)&0x3;
            channel=(*d>>17)&0x3f;
            date=*d&0x3fff;
            fprintf(po, " %2d  %d   %2d %4d\n", geo, buffer, channel, date);
            chans--;
        }

        d++;
    }

    return res;
}
/******************************************************************************/
static int
decode_LC_1877(FILE *po, struct subevent *sev, struct subevent_queue *queue)
{
    u_int32_t *d=sev->data;
    int wordcount, geo, chans, words, buffer, buf, channel, edge, date;
    int i, res=0;

    fprintf(po, "               geo buf chan  adc\n");

    fprintf(po, "  %4d %08x (wordcount)\n", 0, *d);
    wordcount=*d;
    d++;
    if (wordcount+1!=sev->size) {
        fprintf(po, "ERROR: wordcount (%d) does not match sev size (%d)\n",
                wordcount, sev->size);
        res=warning;
    }

    chans=0; /* first word must be a header, not channel data */
    for (i=1; i<sev->size; i++) {
        fprintf(po, "  %4d %08x", i, *d);
        fflush(po);

        if (parity(*d)) {
            fprintf(po, "\nERROR: wrong parity\n");
            res=warning;
            continue;
        }

        geo=(*d>>27)&0x1f;

        if (!chans) {
            buffer=(*d>>11)&0x7;
            words=*d&0x7f;
            fprintf(po, " header: geo=%2d buffer=%2d words=%3d\n",
                    geo, buffer, words);
            chans=words-1;
        } else {
            buf=(*d>>24)&0x3;
            channel=(*d>>17)&0x7f;
            edge=!!(*d&0x10000);
            date=*d&0xffff;
            fprintf(po, " %2d  %d   %2d %c %5d\n", geo, buffer, channel,
                    edge?'L':'T', date);
            chans--;
        }

        d++;
    }

    return res;
}
/******************************************************************************/
static int
decode_LC_1875A(FILE *po, struct subevent *sev, struct subevent_queue *queue)
{
    u_int32_t *d=sev->data;
    int wordcount, geo, event, channel, range, date;
    int i, res=0;

    fprintf(po, "               geo event range chan tdc\n");

    wordcount=*d;
    d++;
    fprintf(po, "  %4d %08x (wordcount)\n", 0, wordcount);
    if (wordcount+1!=sev->size) {
        fprintf(po, "ERROR: wordcount (%d) does not match sev size (%d)\n",
                wordcount, sev->size);
        res=warning;
    }

    for (i=1; i<sev->size; i++) {
        fprintf(po, "  %4d %08x", i, *d);
        fflush(po);

        geo=(*d>>27)&0x1f;

        event=(*d>>24)&0x7;
        range=!!(*d&0x800000);
        channel=(*d>>16)&0x3f;
        date=*d&0xfff;
        fprintf(po, " %2d     %d     %d %3d %4d\n", geo, event, range, channel,
                date);

        d++;
    }

    return res;
}
/******************************************************************************/
static int
decode_CAEN_V785(FILE *po, struct subevent *sev, struct subevent_queue *queue)
{
    u_int32_t *d=sev->data;
    int geo, code, channel, adc, un, ov, crate, cnt, event;
    int i, res=0;

    fprintf(po, "               geo code chan fl adc\n");
    for (i=0; i<sev->size; i++) {
        code=(*d>>24)&0x7;
        geo=(*d>>27)&0x1f;
        fprintf(po, "  %4d %08x %2d %d", i, *d, geo, code);
        fflush(po);
        switch (code) {
        case 0: /* valid datum */
            channel=(*d>>16)&0x1f;
            un=!!(*d&0x2000);
            ov=!!(*d&0x1000);
            adc=*d&0xfff;
            fprintf(po, "      %2d %c%c %4d\n",
                    channel, un?'u':' ', ov?'o':' ', adc);
            break;
        case 2: /* header */
            crate=(*d>>16)&0xff;
            cnt=(*d>>8)&0x3f;
            fprintf(po, " header: crate=%d count=%d\n", crate, cnt);
            break;
        case 4: /* end of block */
            event=*d&0xffffff;
            fprintf(po, " EOB: event=%d\n", event);
            break;
        case 6: /* not valid */
            fprintf(po, " not valid\n");
            break;
        default:
            fprintf(po, " illegal code %d\n", code);
            res=warning;
        }
        d++;
    }

    return res;
}
/******************************************************************************/
static int
decode_ZEL_GPX(FILE *po, struct subevent *sev, struct subevent_queue *queue)
{
    u_int32_t *d=sev->data;
    u_int32_t wordcount, h0, h1, h2, bytes;
    int16_t time;
    int i, addr, channel, fragment, error, slope, res=0;

    if (sev->size<3) {
        fprintf(po, "E subevent too short\n");
        for (i=0; i<sev->size; i++) {
            fprintf(po, "  %4d %08x", i, *d);
        }
        return warning;
    }

    wordcount=*d;
    d++;
    fprintf(po, "  %4d %08x (wordcount)\n", 0, wordcount);
    if (wordcount+1!=sev->size) {
        fprintf(po, "wordcount (%d) does not match sev size (%d)\n",
                wordcount, sev->size);
        res=warning;
    }

    h0=d[0];
    h1=d[1];
    h2=d[2];
    d+=3;

    bytes=h0&0x3ffff;
    fragment=!!(h0&0x40000000);
    fprintf(po, "  %4d %08x head_0 %d bytes%s\n",
            1, h0, bytes, fragment?" fragmented":"");
    fprintf(po, "  %4d %08x head_1 (trigger time) \n",
            2, h1);
    fprintf(po, "  %4d %08x head_2 event %d\n",
            3, h2, h2);

    if (bytes!=(sev->size-1)*4) {
        fprintf(po, "E byte count does not match subevent length\n");
        res=warning;
    }

    if (sev->size>4)
        fprintf(po, "             addr chan slope time\n");
    for (i=4; i<sev->size; i++) {
        addr=(*d>>28)&0xf;
        channel=(*d>>22)&0x3f;
        fprintf(po, "  %4d %08x %x   %2d", i, *d, addr, channel);
        fflush(po);
        error=!!(*d&0x00020000);
        if (error) {
            fprintf(po, " error in GPX %d", channel>>3);
            if (*d&0x400)
                fprintf(po, ", not locked");
            if (*d&0x300)
                fprintf(po, ", overflow of interface FIFO %d", (*d>>8)&3);
            if (*d&0xff)
                fprintf(po, ", overflow of hit FIFO %d", *d&0xff);
            fprintf(po, "\n");
        } else {
            slope=!!(*d&0x00010000);
            time=*d&0xffff;
            fprintf(po, "     %d % 5hd\n", slope, time);
        }

        d++;
    }

    return res;
}
/******************************************************************************/
struct decoder {
    int ID;
    int (*func)(FILE *po, struct subevent *sev, struct subevent_queue *queue);
    const char *name;
};
struct decoder decolist[]={
    {   0, decode_run_nr,    "RUN_NR"},
    {   5, decode_LC_4434,   "LC_4434"},
    {  10, decode_CAEN_C219, "C219"},
    {  20, decode_LC_1881M,  "LC_1881M"},
    {  21, decode_PHIL_10Cx, "PHILLIPS_10Cx"},
    {  22, decode_LC_1881M,  "LC_1881M"},
    {  23, decode_PHIL_10Cx, "PHILLIPS_10Cx"},
    {  24, decode_LC_1877,   "LC_1877"},
    {  25, decode_PHIL_10Cx, "PHILLIPS_10Cx"},
    {  26, decode_LC_1881M,  "LC_1881M"},
    {  27, decode_LC_1877,   "LC_1877"},
    {  28, decode_LC_1875A,  "LC_1875A"},
    {  29, decode_LC_1877,   "LC_1877"},
    {  30, decode_LC_1875A,  "LC_1875A"},
    {  31, decode_LC_1875A,  "LC_1875A"},
    {  32, decode_LC_1881M,  "LC_1881M,"},
    {  33, decode_PHIL_10Cx, "PHILLIPS_10Cx"},
    {  34, decode_LC_1877,   "LC_1877"},
    {  35, decode_LC_1875A,  "LC_1875A"},
    {  37, decode_ZEL_GPX,   "ZEL_GPX"},
    {  38, decode_ZEL_GPX,   "ZEL_GPX"},
    {  39, decode_ZEL_GPX,   "ZEL_GPX"},
    {  40, decode_ZEL_GPX,   "ZEL_GPX"},
    {  41, decode_CAEN_V785, "CAEN_V785"},
    {1000, decode_ZEL_SYNC,  "SYNC"},
    {0, 0, 0}
};
/******************************************************************************/
static int
decode_subevent(FILE *po, u_int32_t *data, struct subevent_queue *queue,
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

    fprintf(po, "EV %d IS %d %s %d words\n",
            sev.evno, sev.ID,
            deco->name?deco->name:"(unknown)",
            sev.size);

    /* check evnr */
    //if (evno<=queue->last_evnr) {
    //if (abs(evno-queue->last_evnr-1)<2)
    //    evno=queue->last_evnr+1;
    if (evno!=queue->last_evnr+1) {
        fprintf(po, "wrong event no. in IS %d: 0x%x --> 0x%x (%+d)\n",
                queue->info->IS_ID, queue->last_evnr, evno, evno-queue->last_evnr-1);
    }

    queue->last_evnr=evno;
    queue->nr_sev++;
    if (size<=3)
        queue->nr_empty_sev++;

    if (deco->func)
        res=deco->func(po, &sev, queue);
    else
        fprintf(po, "no decoder for IS %d found\n", sev.ID);

    return res;
}
/******************************************************************************/
static int
parse_subevent_norm(FILE *po, u_int32_t *data, struct subevent_queue *queue,
        u_int32_t evno, u_int32_t trigno)
{
    return decode_subevent(po, data+2, queue, evno, trigno, data[1]);
}
/******************************************************************************/
static int
parse_subevent_lvds(FILE *po, u_int32_t *data, struct subevent_queue *queue,
        u_int32_t evno, u_int32_t trigno)
{
    int size=data[1];

#if 0
{
int n=size, i;
if (n>100)
    n=100;

fprintf(po, "IS %d evno %d size %d\n", queue->info->IS_ID, evno, size);
for (i=0; i<n; i++)
    fprintf(po, "[%03d] %08x\n", i, data[i]);
}
#endif

    data+=2; /* skip IS_ID and size */

    while (size>=3) {
        u_int32_t head, timestamp, evno;
        int evsize;
        head=data[0];
        timestamp=data[1];
        evno=data[2];
        evsize=(head&0xffff)/4;

        decode_subevent(po, data, queue, evno, trigno, evsize);

        data+=evsize;
        size-=evsize;
    }

    if (size) {
        fprintf(po, "uncomplete LVDS subevent in IS %d event %d\n",
                queue->info->IS_ID, evno);
        return -1;
    }

    return 0;
}
/******************************************************************************/
static int
parse_subevent(FILE *po, u_int32_t *data, u_int32_t evno, u_int32_t trigno)
{
    int is_id;
    struct subevent_queue *queue;

    is_id=*data;

    if (!sev_is_selected(is_id))
        return 0;

    if ((queue=find_sev_queue(po, is_id))==0) {
        fprintf(po, "find_sev_queue returned 0\n");
        return -1;
    }

    if (queue->info->decoding_hints&1)
        return parse_subevent_lvds(po, data, queue, evno, trigno);
    else
        return parse_subevent_norm(po, data, queue, evno, trigno);
}
/******************************************************************************/
static int
parse_event(FILE *po, u_int32_t *data, int ved_id)
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
fprintf(po, "ved %d evno %d size %d sev %d size %d\n",
ved_id, evno, ev_size, data[0], sev_size);
#endif
        if (data+2+sev_size>data_stop) {
            fprintf(po, "cluster_events too short (C)\n");
            return -1;
        }
if (sev_size) { /* XXX empty subevents ignored (should be optional) */
        if (parse_subevent(po, data, evno, trigno)<0) {
            fprintf(po, "parse_subevent returned -1\n");
            return -1;
        }
}
        data+=sev_size+2;
    }

    return 0;
}
/******************************************************************************/
static int
parse_events(FILE *po)
{
    u_int32_t *data=record.data;
    u_int32_t *data_stop=data+record.head[0]-2; /* first illegal word */
    int ved_id, evsize, i, nr_events;

    /* skip options */
    data+=data[0]+1;
    /* flags */
    data+=1;
    /* VED_ID */
    ved_id=*data++;
    /* fragment_id */
    data+=1;
    if (data>=data_stop) {
        fprintf(po, "cluster_events too short (A)\n");
        return -1;
    }

    nr_events=*data++;
    for (i=0; i<nr_events; i++) {
        evsize=*data;
        if (data+1+evsize>data_stop) {
            fprintf(po, "cluster_events too short (B)\n");
            fprintf(po, "data+1+evsize=%p\n", data+1+evsize);
            fprintf(po, "data=%p evsize=0x%x data_stop=%p\n",
                    data, evsize, data_stop);
            return -1;
        }
        if (parse_event(po, data, ved_id)<0) {
            fprintf(po, "parse_event returned -1\n");
            return -1;
        }
        data+=1+evsize;
    }

    return 0;
}
/******************************************************************************/
static int
decode_ved_infos1(FILE *po, u_int32_t* data, int size)
{
    fprintf(po, "ved_info type 1 not implemented\n");
    return -1;
}
/******************************************************************************/
static int
decode_ved_infos2(FILE *po, u_int32_t* data, int size)
{
    fprintf(po, "ved_info type 2 not implemented\n");
    return -1;
}
/******************************************************************************/
static int
decode_ved_infos3(FILE *po, u_int32_t* data, int size)
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
	fprintf(po, "Found VED %d:", vinfo.ved_infos[i].VED_ID);
        for (j=0; j<vinfo.ved_infos[i].nr_is; j++) {
            struct is_info *is_info=vinfo.ved_infos[i].is_infos+j;
            is_info->IS_ID=*data++;
            is_info->importance=*data++;
            is_info->decoding_hints=*data++;
            is_info->maskbit=1ULL<<nr_subqueues;
	    int is = is_info->IS_ID;
	    fprintf(po, " %d hint=%d ",
                is, is_info->decoding_hints);
            nr_subqueues++;
        }
	fprintf(po, "\n");;
    }

    if (nr_subqueues>64) {
        fprintf(po, "too many instrumentation systems (%d),\n"
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
parse_ved_info(FILE* po)
{
    int version, idx, res;
    int size=record.head[0]-2;

    idx=decode_options(po, record.data, size);
    if (idx<0)
        return -1;

    if (size-idx<1) {
        fprintf(po, "cluster_ved_info too short\n");
        return -1;
    }
    version=record.data[idx++];
    switch (version) { /* version */
    case 0x80000001:
        res=decode_ved_infos1(po, record.data+idx, size-idx);
        break;
    case 0x80000002:
        res=decode_ved_infos2(po, record.data+idx, size-idx);
        break;
    case 0x80000003:
        res=decode_ved_infos3(po, record.data+idx, size-idx);
        break;
    default:
        fprintf(po, "cluster_ved_info: unknown version 0x%08x\n", version);
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
parse_record(FILE *po)
{
    int res=0;

    switch (record.head[2]) {
    case clusterty_events:
        res=parse_events(po);
        if (res)
            fprintf(po, "parse_events returned %d\n", res);
        break;
    case clusterty_ved_info:
        fprintf(po, "parse ved_info\n");
        res=parse_ved_info(po);
        if (res)
            fprintf(po, "parse_ved_info returned %d\n", res);
        break;
#if 0
    case clusterty_text:
        fprintf(po, "parse text\n");
        res=parse_text(po);
        if (res)
            fprintf(po, "parse_text returned %d\n", res);
        break;
#endif
#if 0
    case clusterty_file:
        fprintf(po, "parse file\n");
        res=parse_file(po);
        if (res)
            fprintf(po, "parse_file returned %d\n", res);
        break;
#endif
    case clusterty_no_more_data: /* no_more_data */
        fprintf(po, "no more data\n");
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
read_record(int pi, FILE *po)
{
    ssize_t res;
    int l;
    /* read three words (size, endian, clustertype) */
    if ((res=read(pi, record.head, 12))!=12) {
        if (res==0) {
            fprintf(po, "read head: end of file\n");
            return 0;
        } else {
            fprintf(po, "read head: %s (res=%lld)\n",
                    strerror(errno), (long long)res);
            return -1;
        }
    }
    switch (record.head[1]) {
    case 0x12345678: record.swap=0; break;
    case 0x78563412: record.swap=1; break;
    default:
        fprintf(po, "unkown endien 0x%08x\n", record.head[1]);
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
        fprintf(po, "malloc %d bytes: %s\n", l, strerror(errno));
        return -1;
    }
    if ((res=read(pi, record.data, l))!=l) {
        fprintf(po, "read %d bytes: %s (res=%lld)\n",
                l, strerror(errno), (long long)res);
        return -1;
    }

    if (record.swap) {
        int i;
        for (i=record.head[0]-3; i>=0; i--)
            record.data[i]=SWAP_32(record.data[i]);
    }

    statistics.records++;

    return 1;
}
/******************************************************************************/
static int
cluster_loop(int pi, FILE *po)
{
    int count=0, res=0;
    record.data=0;
    while (!res && --count) {
        if (read_record(pi, po)>0) {
            res=parse_record(po);
            if (res)
                fprintf(po, "parse_record returned %d\n", res);
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
    FILE *po;

    if (readargs(argc, argv))
        return 1;

    statistics.records=0;
    statistics.subevents=0;
    statistics.events=0;

    /* open the files */
    pi=0;
    if (inname && strcmp(inname, "-")) {
        if ((pi=open(inname, O_RDONLY|LINUX_LARGEFILE, 0))<0) {
            fprintf(stderr, "open \"%s\": %s\n", inname, strerror(errno));
            return 2;
        }
    }

    po=stdout;
    if (outname && strcmp(outname, "-")) {
        if ((po=fopen(outname, "w"))==0) {
            fprintf(stderr, "fopen \"%s\": %s\n", outname, strerror(errno));
            close(pi);
            return 2;
        }
    }

    sub_queues=0;
    vinfo.valid=0;

    cluster_loop(pi, po);

    fprintf(po, "records read: %d\n", statistics.records);
    fprintf(po, "subevents   : %d\n", statistics.subevents);
    fprintf(po, "events      : %d\n", statistics.events);
    for (i=0; i<nr_subqueues; i++) {
        struct subevent_queue *queue=sub_queues+i;
        fprintf(po, "IS %d: %d events (%d empty)\n",
                queue->info->IS_ID, 
                queue->nr_sev,
                queue->nr_empty_sev);
    }

    if (pi!=0)
        close(pi);
    if (po!=stdout)
        fclose(po);
    return ret;
}
