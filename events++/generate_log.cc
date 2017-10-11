/*
 * generate_log.cc
 * 
 * created: 2004-Aug-04 PW
 *
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <unistd.h>
#include <string.h>
#include <readargs.hxx>
#include <events.hxx>
#include "compat.h"
#include "swap_int.h"
#include <clusterformat.h>
#include <xdrstrdup.hxx>
#include "versions.hxx"

VERSION("2006-Sep-22", __FILE__, __DATE__, __TIME__,
"$ZEL: generate_log.cc,v 1.3 2012/08/28 15:23:35 wuestner Exp $")
#define XVERSION

C_readargs* args;
STRING infile;
int ilit;
static C_iopath* inpath;
int maxrec, maxirec;
int headernum;
unsigned long long eventnum;
unsigned long long int run;
unsigned long long start, stop;
struct clustertimes {
    unsigned long long start;
    unsigned long long stop;
    bool valid;
};
struct clustertimes *clustertimes=0;
int max_ved;

/*****************************************************************************/
static int readargs()
{
    args->addoption("ilit", "iliterally", false,
        "use name of input file literally", "input file literally");
    args->addoption("maxrec", "maxrec", 65536, "maxrec", "max. recordsize");
    args->addoption("filename", "filename", "", "file name");
    args->addoption("infile", 0, "-", "input file", "input");

    if (args->interpret()!=0)
        return -1;

    infile=args->getstringopt("infile");
    maxirec=args->getintopt("maxrec")/4;
    maxrec=maxirec*4;
    ilit=args->getboolopt("ilit");
    return 0;
}
/*****************************************************************************/
static int
complete_read(ems_u32* c, int requested, int existing, int endien)
{
    int res, s;
    s=(requested-existing)*4;
    res=inpath->read(c+existing, s);
    if (res<s) {
        if (res<0)
            cerr<<"complete_read: "<<strerror(errno)<<endl;
        else
            cerr<<"complete_read: short read("<<res<<" of "<<s<<" bytes)"<<endl;
        return -1;
    } else {
        if (endien==0x78563412) {
            for (int i=existing; i<requested-existing; i++) c[i]=swap_int(c[i]);
        }
        return 0;
    }
}
/*****************************************************************************/
static int
check_text(ems_u32* c, int s, int& complete, int endien)
{
    ems_u32* ptr;
    int size=c[0]+1;
    char key[100]="";

    if (!complete) {
        ptr=new ems_u32[size];
        if (complete_read(ptr, size, s, endien)!=0) return -1;
        for (int i=0; i<s; i++) ptr[i]=c[i];
    } else {
        ptr=c;
    }

    ems_u32* p=ptr+3;
    int n_opt=*p++;  // options<>
    p+=n_opt;
    p++; /* ems_u32 flags=*p++; not used here */
    p++; /* int fragment_id=*p++; not used here */
    int n_str=*p++; // Anzahl der Strings
    for (int ni=0; ni<n_str; ni++) {
        char* s;
        p=xdrstrdup(s, p);
        if (strncmp("Key: ", s, 5)==0) {
            strncpy(key, s+5, 100);
        } else if (strncmp("Run: ", s, 5)==0) {
            sscanf(s+5, "%Lu", &run);
        } else if (strncmp("Start Time_coded: ", s, 18)==0) {
            sscanf(s+18, "%Lu", &start);
        } else if (strncmp("Stop Time_coded: ", s, 17)==0) {
            sscanf(s+17, "%Lu", &stop);
        }
        delete[] s;
    }
    //if (strcmp(key, "superheader")==0) {
    //    printf("run=%Lu start=%Lu stop=%Lu\n", run, start, stop);
    //}
    if (!complete) {
        delete[] ptr;
        complete=1;
    }
    return 0;
}
/*****************************************************************************/
static void
check_options(ems_u32* c, int size, int ved_id)
{
    if (size<3)      // no large enough for timestamp
        return;
    if ((c[0]&1)==0) // timestamp lag not set
        return;

    if (max_ved<ved_id) {
        struct clustertimes *help=new struct clustertimes[ved_id+1];
        int i;
        for (i=0; i<=max_ved; i++)
            help[i]=clustertimes[i];
        for (; i<=ved_id; i++)
            help[i].valid=false;
        delete[] clustertimes;
        clustertimes=help;
        max_ved=ved_id;
    }

    if (clustertimes[ved_id].valid) {
        clustertimes[ved_id].stop=c[1];
    } else {
        clustertimes[ved_id].start=c[1];
        clustertimes[ved_id].stop=0;
        clustertimes[ved_id].valid=true;
    }

}
/*****************************************************************************/
static int check_events(ems_u32* c, int s, int& complete, int endien)
{
    ems_u32* ptr;
    ems_u32 size=c[0]+1;

    if (!complete) {
        ptr=new ems_u32[size];
        if (complete_read(ptr, size, s, endien)!=0) return -1;
        for (int i=0; i<s; i++) ptr[i]=c[i];
    } else {
        ptr=c;
    }

    ems_u32* p=ptr+3;
    ems_u32 n_opt=*p++;  // options<>
    check_options(p, n_opt, (int)(p[n_opt+1]));
    p+=n_opt;
    p++; /* ems_u32 flags=*p++; not used here */
    p++; /* ems_u32 ved_id=*p++; not used here */
    p++; /* ems_u32 fragment_id=*p++; not used here */
    ems_u32 num_events=*p++;
    p++; // size of first event
    ems_u32 first_event=*p;

    ems_u32 last_event=first_event+num_events-1;

    if (last_event>eventnum) eventnum=last_event;

    return 0;
}
/*****************************************************************************/
static int
check_cluster(ems_u32* c, int si, int& complete, int endien)
{
    int res=0;
    switch (c[2]) { // type
    case clusterty_events:
        res=check_events(c, si, complete, endien);
        break;
    case clusterty_ved_info:
        //res=check_ved_info(c, si, complete, endien);
        break;
    case clusterty_text:
        headernum++;
        res=check_text(c, si, complete, endien);
        break;
    case clusterty_file:
        headernum++;
        break;
    case clusterty_no_more_data:
        break;
    default:
        cout<<"unknown clustertype "<<c[2]<<endl;
    }
    return res;
}
/*****************************************************************************/
static void
scan_file_slow()
{
    ems_u32 head[2];
    ems_u32 *c=0, c_size=0;
    ssize_t res;
    int complete;

    do { // foreach cluster
        res=inpath->read(head, 2*4);
        if (res!=2*4) {
            if (res==0)
                cout<<"no more data"<<endl;
            else if (res<0)
                cerr<<"read file: "<<strerror(errno)<<endl;
            else
                cerr<<"read file: short read("<<res<<" of "<<2*4<<" bytes)"<<endl;
            goto fehler;
        }
        ems_u32 endien=head[1];
        switch (endien) { // endientest
        case 0x12345678U: // host byteorder
            break;
        case 0x78563412U: // verkehrt
            {
                for (int i=0; i<2; i++) head[i]=swap_int(head[i]);
            }
            break;
        default:
            cerr<<"unknown endien: "<<hex<<endien<<dec<<endl;
            goto fehler;
        }
        ems_u32 size=head[0]+1;
        if (c_size<size) {
            delete[] c;
            if ((c=new ems_u32[size])==0) {
                cerr<<"new ems_u32["<<size<<"]: "<<strerror(errno)<<endl;
                goto fehler;
            }
            c_size=size;
        }
        res=inpath->read(c+2, (size-2)*4);
        if (res!=(ssize_t)(size-2)*4) {
            if (res<0)
                cerr<<"read file: "<<strerror(errno)<<endl;
            else
                cerr<<"read file: short read("<<res<<" of "<<(size-2)*4<<" bytes)"<<endl;
            goto fehler;
        }
        if (endien==0x78563412) {
            for (ems_u32 i=2; i<size; i++) c[i]=swap_int(c[i]);
        }
        for (int i=0; i<2; i++) c[i]=head[i];
        complete=1;
        res=check_cluster(c, size, complete, endien);
        if (res!=0)
            goto fehler;
    } while (1);

fehler:
    delete[] c;
}
/*****************************************************************************/
static int
check_record(ems_u32* c, clustertypes& typ)
{
    int res, complete;
    try {
        res=inpath->read(c, maxrec);
    } catch (C_status_error* e) {
        switch (e->code()) {
        case C_status_error::err_filemark:
            res=0;
            break;
        case C_status_error::err_end_of_file:
        case C_status_error::err_none:
        default:
            cout<<"error: "<<(*e)<<endl;
            res=-1;
        }
        delete e;
        if (res<0) return -1;
    } catch (C_error* e) {
        cout<<"!! error: "<<(*e)<<endl;
        delete e;
        return -1;
    }
    if (res==0) { /* impossible; C_iopath::read throws C_status_error */
        cout<<"Filemark"<<endl;
    } else if (res<0) {
        cerr<<"read tape: "<<strerror(errno)<<endl;
    } else {
        int endien=c[1];
        switch (endien) { // endientest
        case 0x12345678: // host byteorder
            break;
        case 0x78563412: // verkehrt
            {
                for (int i=0; i<res/4; i++) c[i]=swap_int(c[i]);
            }
            break;
        default:
            cerr<<"unknown endien: "<<hex<<c[1]<<dec<<endl;
            return -1;
        }
        int size=c[0]+1;
        if (size*4!=res) {
            cout<<"warning: clustersize="<<size<<" but got "<<res<<" bytes"<<endl;
        }
        typ=clustertypes(c[2]);
        complete=1;
        check_cluster(c, size, complete, endien);
    }
    return res;
}
/*****************************************************************************/
static int scan_tapefile_fast()
{
    ems_u32* c=new ems_u32[maxirec];
    int res;
    clustertypes typ;
    struct mtop mt_com;

    do {
        res=check_record(c, typ);
    } while ((res>0) && (typ!=clusterty_events));

    if (res<0) goto fehler;

    if (res==0) {
      cout<<"no event records found"<<endl;
      goto fehler;
    }
    // seek over next filemark
    mt_com.mt_op=MTFSF;
    mt_com.mt_count=1;
    res=ioctl(inpath->path(), MTIOCTOP, &mt_com);
    if (res<0) {
        cout<<"wind tape forward: "<<strerror(errno)<<endl;
        goto fehler;
    }
    // seek backward
    mt_com.mt_op=MTBSF;
    mt_com.mt_count=1;
    res=ioctl(inpath->path(), MTIOCTOP, &mt_com);
    if (res<0) {
        cout<<"wind tape backward: "<<strerror(errno)<<endl;
        goto fehler;
    }
    mt_com.mt_op=MTBSR;
    mt_com.mt_count=headernum+2;
    res=ioctl(inpath->path(), MTIOCTOP, &mt_com);
    if (res<0) {
        cout<<"wind "<<headernum+2<<" records backward: "<<strerror(errno)<<endl;
        goto fehler;
    }

    // read trailer records
    do {
        res=check_record(c, typ);
    } while (res>0);

fehler:
    delete c;
    return res;
}
/*****************************************************************************/
static void
scan_file(const char* infile)
{
    struct timeval tv;
    struct tm* tm;
    time_t tt;
    char st[25];

    gettimeofday(&tv, 0);
    tt=tv.tv_sec;
    tm=gmtime(&tt);
    strftime(st, 25, "%Y-%m-%d_%H-%M-%S", tm);

    try {
        inpath=new C_iopath(infile, C_iopath::iodir_input, ilit);
        if (inpath->typ()==C_iopath::iotype_none) {
            cerr<<"cannot open infile \""<<infile<<"\""<<endl;
            return;
        }
        inpath->fadvise_sequential(true);

        eventnum=0;
        headernum=0;
        run=0;
        start=0;
        stop=0;
        delete[] clustertimes;
        clustertimes=0;
        max_ved=-1;

        switch (inpath->typ()) {
        case C_iopath::iotype_tape:
            scan_tapefile_fast();
            break;
        case C_iopath::iotype_file:
        case C_iopath::iotype_socket: // nobreak
        case C_iopath::iotype_fifo: // nobreak
        default:
            scan_file_slow();
            break;
        }

        delete inpath;
    } catch (C_error* e) {
        if ((e->type()!=C_error::e_status) ||
           (((C_status_error*)e)->code()!=C_status_error::err_end_of_file)) {
            cerr<<(*e)<<endl;
        }
        delete e;
    }
    // find first valid ved_id
    int ved_id=0;
    while (ved_id<max_ved && !clustertimes[ved_id].valid)
        ved_id++;

    if ((ved_id<max_ved) && clustertimes[ved_id].valid) {
        if (start==0)
            start=clustertimes[ved_id].start;
        if (stop==0)
            stop=clustertimes[ved_id].stop;
    }

    cout<<st<<" SCANNED ";
    if (args->isdefault("filename")) {
        cout<<infile;
    } else {
        cout<<args->getstringopt("filename");
    }
    cout<<" "<<run<<" "<<start<<" "<<stop<<" "<<eventnum<<endl;

#if 0
    cout<<"clustertimes:"<<endl;
    for (int i=0; i<=max_ved; i++) {
        if (clustertimes[i].valid) {
            cout<<"ved "<<i<<" start="<<clustertimes[i].start
                <<" stop="<<clustertimes[i].stop<<endl;
        }
    }
#endif
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
    args=new C_readargs(argc, argv);
    if (readargs()!=0)
        return 1;

    for (int i=0; i<args->numnames(); i++) {
        const char* name=args->getnames(i);
        scan_file(name);
    }

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
