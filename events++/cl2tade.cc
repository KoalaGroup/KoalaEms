/*
 * ems/events++/cl2tade.cc
 * 
 * created 2004-12-03 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <xdrstring.h>
#include <clusterformat.h>
#include <readargs.hxx>
#include "compressed_io.hxx"
#include "ems_objects.hxx"
#include "event2tade.hxx"
#include <versions.hxx>

VERSION("Dec 04 2004", __FILE__, __DATE__, __TIME__,
"$ZEL: cl2tade.cc,v 1.1 2005/02/11 15:44:53 wuestner Exp $")
#define XVERSION

#define __xSTRING(x) #x
#define __SSTRING(x) __xSTRING(x)

#define swap_int(a) ( ((a) << 24) | \
                      (((a) << 8) & 0x00ff0000) | \
                      (((a) >> 8) & 0x0000ff00) | \
        ((unsigned int)(a) >>24) )

namespace {

const char comp_ext[]=".bz2";
const compressed_io::compresstype comp_type=compressed_io::cpt_bzip2;
//const char comp_ext[]="";
//const compressed_io::compresstype comp_type=compressed_io::cpt_plain;

C_readargs* args;
int verbose, debug;
const char* outdir;
bool ignore_missing;
bool ignore_duplicate;
bool force;

//---------------------------------------------------------------------------//
int readargs()
{
    args->addoption("verbose",  "verbose", false, "verbose output");
    args->addoption("debug",    "debug", false, "debug output");
    args->addoption("savefile", "savefile", "", "save config file");
    //args->addoption("savetext", "savetext", "", "save config record");
    args->addoption("ignore_missing",   "ignore_missing", false, "ignore missing detector definitions");
    args->addoption("ignore_duplicate", "ignore_duplicate", false, "ignore duplicate definition in detdef");
    args->addoption("force",    "force", false, "force overwrite existing files");
    args->addoption("detdef",   "detdef", "", "replacement detdef file");

    args->addoption("outdir",     "dir", ".", "directory for uotput files");

    args->multiplicity("verbose", 1);
    args->multiplicity("savefile", 0);
    //args->multiplicity("savetext", 0);

    if (args->interpret(1)!=0) return -1;

    verbose=args->getboolopt("verbose");
    debug=args->getboolopt("debug");
    outdir=args->getstringopt("outdir");
    ignore_missing=args->getboolopt("ignore_missing");
    ignore_duplicate=args->getboolopt("ignore_duplicate");
    force=args->getboolopt("force");

    return 0;
}
//---------------------------------------------------------------------------//
int
try_extract(ems_object& ems, int final, compressed_output* of)
{
    while (1) {
        if (!final) {
            // check whether a queue is empty; this is the regular exit
            // of the loop
            for (int i=0; i<ems.num_ved; i++) {
                if (ems.ved_slots[i].empty()) {
                    //printf("slot %d empty\n", i);
                    return 0;
                }
            }
        } else {
            // same as above but until all queues are empty
            int n=0;
            for (int i=0; i<ems.num_ved; i++) {
                if (!ems.ved_slots[i].empty()) n++;
            }
            if (!n) return 0;
        }

        deque<sev_object> event;
        for (int i=0; i<ems.num_ved; i++) {
            deque<sev_object>* que=ems.ved_slots+i;
            while (!que->empty() && (que->front().event_nr<=ems.next_evno)) {
                sev_object& sevo=que->front();
                if (sevo.event_nr<ems.next_evno) {
                    printf("queue %d: event %d (%d expected)\n",
                            i, sevo.event_nr, ems.next_evno);
                    return -1;
                }
                event.push_back(sevo);
                que->pop_front();
            }
        }
        int res=cl2tade::decode_to_tade(event, ems,
            of[0].file(), of[1].file(), of[2].file());
        if (res<0)
            return res;
        ems.next_evno++;
    }
}
//---------------------------------------------------------------------------//
int
parse_events(ems_u32* buf, int num, ems_object& ems, compressed_output* of)
{
    ems_u32 optsize=buf[0];
    //ems_u32 flags=buf[optsize+1];
    ems_u32 ved_id=buf[optsize+2];
    ems_u32 events=buf[4+optsize];
    ems_u32* p=buf+optsize+5;

    int ved_idx=ems.get_vedindex(ved_id);
    if (ved_idx<0) {
        printf("unknown ved_id %d\n", ved_id);
        return -1;
    }
    deque<sev_object>* que=ems.ved_slots+ved_idx;
    bool empty=que->empty();

    for (ems_u32 event=0; event<events; event++) {
        /* ems_u32 evsize=* */p++;
        ems_u32 evno=*p++;
        ems_u32 trigno=*p++;
        ems_u32 subevents=*p++;
        if (((trigno!=1) && (trigno!=8)) || (evno==0)) {
            printf("  ev %d trigger %x %d subevents\n", evno, trigno, subevents);
        }
        for (ems_u32 subevent=0; subevent<subevents; subevent++) {
            ems_u32 IS_ID=*p++;
            ems_u32 size=*p++;
            sev_object sevo;
            sevo.event_nr=evno;
            sevo.trigger=trigno;
            sevo.sev_id=IS_ID;
            sevo.data.reserve(size);
            for (ems_u32 i=0; i<size; i++) {
                sevo.data.push_back(*p++);
            }
            que->push_back(sevo);
        }
    }
    if (empty) {
        int res;
        res=try_extract(ems, 0, of);
        if (res<0)
            return -1;
    }

    //printf("ved %d events: %d\n", ved, events);
    return 0;
}
//---------------------------------------------------------------------------//
int
parse_no_more_data(ems_u32* buf, int num, ems_object& ems, compressed_output* of)
{
    printf("no_more_data\n");
    try_extract(ems, 1, of);
    return 0;
}
//---------------------------------------------------------------------------//
int
parse_record(ems_u32* buf, int num, ems_object& ems, compressed_output* of)
{
    int res=0;
    switch (buf[0]) {
    case clusterty_events:
        res=parse_events(buf+1, num-1, ems, of);
        break;
    case clusterty_ved_info:
        res=ems.parse_ved_info(buf+1, num-1);
        break;
    case clusterty_text:
        res=ems.parse_text(buf+1, num-1);
        break;
    case clusterty_file:
        res=ems.parse_file(buf+1, num-1);
        break;
    case clusterty_no_more_data:
        res=parse_no_more_data(buf+1, num-1, ems, of);
        break;
    default:
        printf("unknown clustertype %d\n", buf[2]);
        res=-1;
    }
    return res;
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
        printf("read header: %s\n", strerror(errno));
        return -1;
    case 0:
        printf("read: end of file\n");
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
        printf("unknown endian 0x%08x\n", head[1]);
        return -1;
    }

    b=new ems_u32[size+1];
    if (!b) {
        printf("new buf: %s\n", strerror(errno));
        return -1;
    }

    b[0]=head[0];
    b[1]=head[1];
    switch (xread(p, (char*)(b+2), (size-1)*4)) {
    case -1: // error
        printf("read body: %s\n", strerror(errno));
        return -1;
    case 0:
        printf("read body: unexpected end of file\n");
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
int
process_file(const char* name)
{
    const char *stub[]={"even", "odd", "scaler"};
    compressed_input input(name);
    compressed_output output[3];
    ems_object ems;
    ems_u32 *buf;
    int rres, wres, res;
    int error=0;

    if (!input.good()) {
        cerr<<"error opening input for "<<name<<endl;
        return -1;
    }

    // we need the run number before we can open the output files
    // parse initial records but stop at the first event cluster
    do {
        rres=read_record(input.path(), &buf);
        if (rres>0) {
            if (buf[2]==clusterty_events) {
                // we have to open the output files first
                break;
            }
            wres=parse_record(buf+2, rres-2, ems, 0);
            delete[] buf;
            if (wres) {
                error=1;
                break;
            }
        } else {
            error=1;
            break;
        }
    } while (1);

    if (error) {
        cerr<<"file "<<name<<" does not contain events"<<endl;
        return -1;
    }

    if (args->multiplicity("savefile")>0) {
        for (int i=0; i<args->multiplicity("savefile"); i++) {
            string name(args->getstringopt("savefile", i));
            cerr<<"save file "<<name<<endl;
            ems.save_file(name, true);
        }
    }

//     if (args->multiplicity("savetext")>0) {
//         for (int i=0; i<args->multiplicity("savetext"); i++) {
//             string name(args->getstringopt("savetext", 0));
//             cerr<<"save config text "<<name<<endl;
//             ems.save_text(name);
//         }
//     }

    // read the replacement detdef, if any
    if (!args->isdefault("detdef")) {
        res=ems.replace_file_object("detdef.lis", args->getstringopt("detdef"));
        if (res<0)
            return -1;
    }

    if (!ems.runnr_valid()) {
        cerr<<"file "<<name<<" does not contain the run number"<<endl;
        return -1;
    }

    file_object& detdef=ems.find_file_object("detdef.lis");
    res=cl2tade::parse_detdef(detdef, ignore_duplicate);
    if (res<0)
        return -1;

    // create three tfiles for 'even' 'odd' and 'scaler'
    for (int i=0; i<3; i++) {
        char tname[1024];
        //snprintf(tname, 1024, "%s/%s_%s", tmpdir, "tsm_split", "XXXXXX");
        snprintf(tname, 1024, "%s/run_%04d_%s.tade%s",
                outdir, ems.runnr(), stub[i], comp_ext);

        output[i].open(tname, comp_type, force);
        output[i].fdopen();
        if (!output[i].good()) {
            cerr<<"error opening output for "<<tname<<endl;
            // leave all other descriptors open, the program will terminate
            return -1;
        }
    }

    ems.ignore_missing=ignore_missing;

    // process the first buffered event record
    wres=parse_record(buf+2, rres-2, ems, output);
    delete[] buf;
    if (wres) {
        return -1;
    }

    do {
        rres=read_record(input.path(), &buf);
        if (rres>0) {
            wres=parse_record(buf+2, rres-2, ems, output);
            delete[] buf;
            if (wres) {
                error=1;
                break;
            }
        } else {
            error=1;
            break;
        }
    } while (1);

    input.close();

    for (int i=0; i<3; i++) {
        output[i].close();
    }

    cerr<<ems.next_evno-1<<" events converted"<<endl;
    if (ems.ignored_missings)
        cerr<<ems.ignored_missings<<" hits ignored (no detector definition)"<<endl;

    return 0;
}
//---------------------------------------------------------------------------//

} // end of anonymous namespace

int
main(int argc, char* argv[])
{
    int res, ret=0, i;

    args=new C_readargs(argc, argv);
    if (readargs())
        return 1;

    for (i=0; i<args->numnames(); i++) {
        res=process_file(args->getnames(i));
    }    

    return ret;
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
