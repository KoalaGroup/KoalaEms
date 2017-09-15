/*
 * ems/events++/cl2f1.cc
 * 
 * created 2005-01-11 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <cerrno>
#include <cstring>
#include <sys/wait.h>
#include <xdrstring.h>
#include <clusterformat.h>
#include <readargs.hxx>
#include "compressed_io.hxx"
#include <versions.hxx>

VERSION("Jan 11 2005", __FILE__, __DATE__, __TIME__,
"$ZEL: cl2f1.cc,v 1.2 2010/09/04 21:17:35 wuestner Exp $")
#define XVERSION

#define __xSTRING(x) #x
#define __SSTRING(x) __xSTRING(x)

#define swap_int(a) ( ((a) << 24) | \
                      (((a) << 8) & 0x00ff0000) | \
                      (((a) >> 8) & 0x0000ff00) | \
        ((unsigned int)(a) >>24) )

namespace {

C_readargs* args;
int verbose, debug;
unsigned int subid;

//---------------------------------------------------------------------------//
int readargs()
{
    args->addoption("verbose",  "verbose", false, "verbose output");
    args->addoption("debug",    "debug", false, "debug output");
    args->addoption("subid",    "subid", 0, "subevent id");
    args->addoption("infile",   "infile", 0, "input file (stdin if not given)");

    args->hints(C_readargs::required, "subid", 0);
    args->multiplicity("verbose", 1);

    if (args->interpret(1)!=0) return -1;

    verbose=args->getboolopt("verbose");
    debug=args->getboolopt("debug");
    subid=args->getintopt("subid");

    return 0;
}
//---------------------------------------------------------------------------//
int
parse_sev(ems_u32* buf, int num, unsigned int IS_ID)
{
    static int count=0;
    int i;
    printf("parse_sev");
    if ((buf[2]&0xffffff)==0) printf("event %d\n", buf[2]);
    if (num!=11) {
        for (i=0; i<num; i++) {
            printf("%3d %08x\n", i, buf[i]);
        }
        count++;
        if (count>=100) exit(1);
    }
    return 0;
}
//---------------------------------------------------------------------------//
int
parse_events(ems_u32* buf, int num)
{
    ems_u32 optsize=buf[0];
    //ems_u32 flags=buf[optsize+1];
    //ems_u32 ved_id=buf[optsize+2];
    ems_u32 events=buf[4+optsize];
    ems_u32* p=buf+optsize+5;

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
            if (IS_ID==subid) {
                parse_sev(p, size, IS_ID);
            }
            p+=size;
        }
    }

    return events;
}
//---------------------------------------------------------------------------//
int
parse_no_more_data(ems_u32* buf, int num)
{
    printf("no_more_data\n");
    return 0;
}
//---------------------------------------------------------------------------//
int
parse_record(ems_u32* buf, int num)
{
    int res=0;
    switch (buf[0]) {
    case clusterty_events:
        res=parse_events(buf+1, num-1);
        break;
    case clusterty_ved_info:
        break;
    case clusterty_text:
        break;
    case clusterty_file:
        break;
    case clusterty_no_more_data:
        res=parse_no_more_data(buf+1, num-1);
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
    compressed_input* input;
    ems_u32 *buf;
    int rres, wres;
    int error=0, events=0;

    if (name) {
        input=new compressed_input(name);
    } else {
        input=new compressed_input(0);
    }

    if (!input->good()) {
        cerr<<"error opening input"<<endl;
        return -1;
    }

    do {
        rres=read_record(input->path(), &buf);
        if (rres>0) {
            wres=parse_record(buf+2, rres-2);
            delete[] buf;
            if (wres>0) {
                events+=wres;
            } else if (wres<0) {
                error=1;
                break;
            }
        } else {
            error=1;
            break;
        }
    } while (1);

    input->close();

    cerr<<events<<" events converted"<<endl;

    return 0;
}
//---------------------------------------------------------------------------//

} // end of anonymous namespace

int
main(int argc, char* argv[])
{
    int res, ret;

    args=new C_readargs(argc, argv);
    if (readargs())
        return 1;

    if (args->isdefault("infile")) {
        res=process_file(0);
    } else {
        res=process_file(args->getstringopt("infile"));
    }

    ret=res?1:0;
    return ret;
}
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
