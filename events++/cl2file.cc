/*
 * cl2fle.cc
 * 
 * created: 26.03.2002 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <string.h>
#include <versions.hxx>

/*#include <locale.h>*/
#include "memory.hxx"
#include <readargs.hxx>
#include <errors.hxx>
#include <clusterformat.h>
#include "swap_int.h"
#include <xdrstring.h>

VERSION("2014-07-07", __FILE__, __DATE__, __TIME__,
"$ZEL: cl2file.cc,v 1.5 2014/07/07 20:14:04 wuestner Exp $")
#define XVERSION

C_readargs* args;
STRING infile;
int loop, recsize;
int g_infiles, g_outfiles, g_clusters;
int outfiles, clusters;
int filenumber;
char dir_name[1024];
char pathname[1024];

class Cluster {
    public:
        Cluster(C_iopath*, int recsize);
        ~Cluster();
    protected:
        void decode_ved_info() const;
        void decode_text() const;
        void decode_file() const;
        void decode_no_more_data() const;
        int decode_flags(unsigned int* ptr, int silent) const;
        int decode_options(unsigned int* ptr, int silent) const;
        bool valid_;
        unsigned int* data_;
        int size_;
        unsigned int endian_;
        bool filemark_;
        clustertypes typ;
    public:
        bool valid() const {return valid_;}
        bool filemark() const {return filemark_;}
        //unsigned int* data() const {return data_;}
        //unsigned int size() const {return size_;}
        //unsigned int endian() const {return endian_;}
        void decode();
        void dump_data(unsigned int* ptr, int len) const;
        void dump(int length) const;
};

/*****************************************************************************/
static void
printheader(ostream& os)
{
os<<"Reads EMS cluster file(s) and dumps header records."<<endl<<endl;
}
/*****************************************************************************/
static void
printstreams(ostream& os)
{
os<<endl;
os<<"input may be: "<<endl;
os<<"  - for stdin ;"<<endl;
os<<"  <filename> for regular file or tape;"<<endl;
os<<"      (if filename contains a colon use -iliterally)"<<endl;
os<<"  :<port#> or <host>:<port#> for tcp socket;"<<endl;
os<<"  :<name> for unix socket (passive);"<<endl;
os<<"  @:<name> for unix socket (active);"<<endl;
}
/*****************************************************************************/
static int
readargs(void)
{
args->helpinset(3, printheader);
args->helpinset(5, printstreams);

args->addoption("infile", 0, "-", "input file", "input");
args->addoption("outdir", 1, ".", "output directory", "outdir");

args->addoption("ilit", "iliterally", false,
    "use name of input file literally", "input file literally");
args->addoption("loop", "loop", true, "reopen input if exhausted", "loop");

args->addoption("recsize", "recsize", 65536, "maximum record size of input (tape only)",
    "recsize");

args->addoption("verbose", "verbose", false, "put out some comments", "verbose");

if (args->interpret(1)!=0) return(-1);
loop=args->getboolopt("loop");

recsize=args->getintopt("recsize")/sizeof(unsigned int);
return(0);
}
/*****************************************************************************/
static int
makedir(int nr)
{
    sprintf(dir_name, "%s/%03d", args->getstringopt("outdir"), nr);
    if (mkdir(dir_name, 0750)<0) {
        cout<<"cannot create directory "<<dir_name<<": "<<strerror(errno)<<endl;
        return -1;
    }
    return 0;
}
/*****************************************************************************/
static FILE*
make_file(char* filename)
{
    FILE* f;
    int p, count=0, errnum;

    sprintf(pathname, "%s/%s;%d", dir_name, filename, ++count);
    do {
        p=open(pathname, O_WRONLY|O_CREAT|O_EXCL, 0666);
        if (p<0) {
            if ((errnum=errno)==EEXIST)
                sprintf(pathname, "%s/%s;%d", dir_name, filename, ++count);
            else
                cout<<"cannot create file "<<pathname<<": "<<strerror(errno)
                    <<endl;
        }
    } while ((p<0) && (errnum==EEXIST));
    if (p>=0) {
        f=fdopen(p, "w");
    } else
        f=0;
    return f;
}
/*****************************************************************************/
Cluster::Cluster(C_iopath* path, int recsize)
:valid_(false), data_(0), size_(0), filemark_(false)
{
    try {
        if (path->typ()==C_iopath::iotype_tape) {
            bool wenden;
            int res;
            data_=new unsigned int[recsize];
            res=path->read(data_, recsize);
            if (res<8) {
                cout<<"read: res="<<res<<endl;
                return;
            }
            size_=res/sizeof(unsigned int);
            switch (data_[1]) {
            case 0x12345678: wenden=false; break;
            case 0x78563412: wenden=true; break;
            default:
                cout<<"unknown endian 0x"<<hex<<data_[1]<<dec<<endl;
                cout<<"size="<<size_<<" (res="<<res<<")"<<endl;
                dump(128);
                return;
            }
            if (wenden) {
                for (int i=0; i<size_; i++) data_[i]=swap_int(data_[i]);
            }
        } else {
            bool wenden;
            unsigned int head[2];
            path->read(head, 2*sizeof(int));
            endian_=head[1];
            switch (endian_) {
            case 0x12345678: wenden=false; break;
            case 0x78563412: wenden=true; break;
            default:
                cerr<<"unknown endian "<<hex<<endian_<<dec<<endl;
                return ;
            }
            size_=wenden?swap_int(head[0]):head[0];
            data_=new unsigned int[size_+1];
            data_[0]=size_;
            data_[1]=wenden?swap_int(endian_):endian_;
            path->read(data_+2, (size_-1)*sizeof(unsigned int));
            size_++;
            if (wenden) {
                for (int i=2; i<size_; i++) data_[i]=swap_int(data_[i]);
            }
        }
        typ=static_cast<clustertypes>(data_[2]);
        valid_=true;
        return;
    } catch (C_status_error* e) {
        switch (e->code()) {
        case C_status_error::err_filemark:
            cout<<"tapereader: filemark reached"<<endl;
            filemark_=true;
            break;
        default:
            cout<<(*e)<<endl;
        }
        delete e;
        if (data_) delete[] data_;
    }
}
/*****************************************************************************/
Cluster::~Cluster()
{
    delete[] data_;
}
/*****************************************************************************/
void Cluster::dump_data(unsigned int* ptr, int len) const
{
if (!len) {
    len=size_-(ptr-data_);
}
int lines=len/4;
int rest=len%4;
int i, l, o=0;
cout<<hex; cout.fill('0');
for (l=0; l<lines; l++)
  {
  cout<<setw(8)<<o<<": ";
  for (i=0; i<4; i++, o++) cout<<setw(8)<<ptr[o]<<' ';
  cout<<endl;
  }
if (rest>0)
  {
  cout<<setw(8)<<o<<": ";
  for (i=0; i<rest; i++, o++) cout<<setw(8)<<ptr[o]<<' ';
  cout<<endl;
  }
cout<<dec; cout.fill(' ');
}
/*****************************************************************************/
void Cluster::dump(int length) const
{
int s=(length>0)?(length<size_?length:size_):size_;
dump_data(data_, s);
}
/*****************************************************************************/
int Cluster::decode_flags(unsigned int* ptr, int silent) const
{
//os<<"decode_flags("<<ptr-data_<<")"<<endl;
    unsigned int* sptr=ptr;
    int flags=*ptr++;
    if (flags && !silent) cout<<"flags="<<hex<<flags<<dec<<endl;
    return ptr-sptr;
}
/*****************************************************************************/
int Cluster::decode_options(unsigned int* ptr, int silent) const
{
//os<<"decode_options("<<ptr-data_<<")"<<endl;
    unsigned int* sptr=ptr;
    int s=*ptr++;
    if (silent)
        ptr+=s;
    else {
        if (s) {
            int f=*ptr++;
            if (f&1) { // timestamp
                struct timeval tv;
                tv.tv_sec=*ptr++;
                tv.tv_usec=*ptr++;
                struct tm *tm;
                char ss[1024];
                tm=localtime(&tv.tv_sec);
                strftime(ss, 1024, "%e. %b %H:%M:", tm);
                float sec=tm->tm_sec+tv.tv_usec/1000000.;
                cout.setf(ios::fixed, ios::floatfield);
                cout.precision(6);
                cout.fill('0');
                cout<<"timestamp: "<<ss<<setw(9)<<sec<<endl;
                cout.fill(' ');
                //cout<<"timestamp: sec="<<tv.tv_sec<<" usec="<<tv.tv_usec<<endl;
            }
            if (f&2) { // checksum
                int c=*ptr++;
                cout<<"checksum: "<<hex<<c<<dec<<endl;
            }
        }
    }
    return ptr-sptr;
}
/*****************************************************************************/
void Cluster::decode_text() const
{
    unsigned int* ptr=data_+3;
    char filename[1024];
    int len;
    char *line1=0, *line2=0;
    char *p0, *p1, *p2;
    FILE* f;

    ptr+=decode_options(ptr, 1);
    ptr+=decode_flags(ptr, 1);
    ptr++; // skip fragment_id

    int lines=*ptr++;
    if (!lines) {
        cout<<"empty text cluster"<<endl;
        return;
    }
    
    len=xdrstrclen(ptr);
    line1=new char[len+1];
    ptr=reinterpret_cast<unsigned int*>(extractstring(line1, ptr));
    if (strncmp(line1, "Key: ", 5)) {
        cout<<"text cluster without key"<<endl;
        strcpy(filename, "noname");
    }
    p0=line1+5;
    p1=strchr(p0, ':');
    if (p1) {
        p1++;
    }
    if (strcmp(p0, "masterheader")==0) {
        strcpy(filename, "masterheader");
    } else if (strcmp(p0, "superheader")==0) {
        strcpy(filename, "superheader");
    } else if (strcmp(p0, "run_notes")==0) {
        strcpy(filename, "run_notes");
    } else if (strcmp(p0, "configuration")==0) {
        len=xdrstrclen(ptr);
        line2=new char[len+1];
        ptr=reinterpret_cast<unsigned int*>(extractstring(line2, ptr));
        if (strncmp(line2, "name ", 5)) {
            cout<<"text cluster configuration without name"<<endl;
            strcpy(filename, "configuration_noname");
        } else
            sprintf(filename, "configuration_%s", line2+5);
    } else if (strncmp(p0, "userfile", p1-p0-1)==0) {
        p2=strrchr(p0, '/');
        if (p2)
            p2++;
        else
            p2=p1;
        strcpy(filename, p2);
    }
    if ((f=make_file(filename))==0) return;
    cout<<"writing "<<pathname<<endl;
    fprintf(f, "%s\n", line1); lines--;
    if (line2) {fprintf(f, "%s\n", line2); lines--;}
    for (int l=0; l<lines; l++) {
        len=xdrstrclen(ptr);
        char* str=new char[len+1];
        ptr=reinterpret_cast<unsigned int*>(extractstring(str, ptr));
        fprintf(f, "%s\n", str);
        delete[] str;
    }
    
    fclose(f);
    delete[] line1;
    if (line2) delete[] line2;

    g_outfiles++;
    outfiles++;
}
/*****************************************************************************/
void Cluster::decode_file() const
{
    decode_options(data_+3, 1);
    decode_flags(data_+3, 1);
    cout<<"cluster file not yet decoded."<<endl;

    g_outfiles++;
    outfiles++;
}
/*****************************************************************************/
void Cluster::decode_ved_info() const
{
    unsigned int* ptr=data_+3;
    ptr+=decode_options(ptr, 1);

    unsigned int version=*ptr++;

    switch (version) {
    case 0x80000001: {
        int nved=*ptr++;
        cout<<nved<<" VEDs:"<<endl;
        for (int ved=0; ved<nved; ved++) {
            int vedid=*ptr++;
            cout<<"  "<<vedid<<" :";
            int nis=*ptr++;
            for (int is=0; is<nis; is++) {
                int isid=*ptr++;
                cout<<" "<<isid;
            }
            cout<<endl;
        }
    }
    break;
    default:
        cout<<"ved_info: version "<<version<<" unknown"<<endl;
        dump(0);
    }
}
/*****************************************************************************/
void Cluster::decode()
{
    switch (typ) {
    case clusterty_ved_info:
        /*decode_ved_info();*/
        break;
    case clusterty_text:
        decode_text();
        break;
    case clusterty_file:
        decode_file();
        break;
    case clusterty_no_more_data:
        cout<<clusters<<" clusters read; "<<outfiles<<" files written"<<endl;
        clusters=outfiles=0;
        break;
    case clusterty_events: break;
    case clusterty_async_data: break;
    // no default
    }
}
/*****************************************************************************/
static void
do_scan(C_iopath* inpath)
{
    bool filemark;
    Cluster* cluster;

    filenumber=0;
    do {
        g_infiles++;
        filenumber++;
        if (makedir(filenumber)<0) return;
        outfiles=clusters=0;
        cluster=new Cluster(inpath, recsize);
        while (cluster->valid()) {
            g_clusters++;
            clusters++;
            cluster->decode();
            delete cluster;
            cluster=new Cluster(inpath, recsize);
        }
        filemark=cluster->filemark();
        //if (!filemark) delete cluster;
    } while (filemark);
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
    C_iopath* inpath=0;
    int res=0;
    args=0;

    g_infiles=g_outfiles=g_clusters=0;

    try {
        args=new C_readargs(argc, argv);
        if (readargs()!=0) return(0);
        inpath=new C_iopath(args->getstringopt("infile"),
                            C_iopath::iodir_input,
                            args->getboolopt("ilit"));
        if (inpath->path()<0) {
            delete inpath;
            return 1;
        }
        cerr<<"inpath: "<<(*inpath)<<endl;
        do_scan(inpath);
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        res=2;
    }
    cout<<g_infiles<<" files with "<<g_clusters<<" clusters read; "
        <<g_outfiles<<" files written"<<endl;
    delete inpath;
    delete args;
    return res;
}
/*****************************************************************************/
/*****************************************************************************/
