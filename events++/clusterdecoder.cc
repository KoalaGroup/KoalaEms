/*
 * clusterdecoder.cc
 * created 25.Feb.2001 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <emsctypes.h>
#include <readargs.hxx>
#include <iopathes.hxx>
#include <errors.hxx>
#include "versions.hxx"

VERSION("Sep 25 2003", __FILE__, __DATE__, __TIME__,
"$ZEL: clusterdecoder.cc,v 1.2 2005/02/11 15:44:54 wuestner Exp $")
#define XVERSION

const int maxrec=16777216; // 16 MByte

#define swap_int(a) ( ((a) << 24) | \
		      (((a) << 8) & 0x00ff0000) | \
		      (((a) >> 8) & 0x0000ff00) | \
	((unsigned int)(a) >>24) )

C_readargs* args;

C_iopath* inpath;
ostream_withassign outpath;

int maxrecord;
int level;

class Cluster {
    public:
        Cluster(C_iopath*, ostream& os);
        ~Cluster();
        typedef enum {clusterty_events=0, clusterty_ved_info, clusterty_text,
            clusterty_wendy_setup,
            clusterty_no_more_data=0x10000000} clustertypes;
    protected:
        int decode_ev_fb_ph10Cx(unsigned int* ptr, int size, int level, ostream& os) const;
        int decode_ev_fb_lc1875a(unsigned int* ptr, int size, int level, ostream& os) const;
        int decode_ev_fb_lc1877(unsigned int* ptr, int size, int level, ostream& os) const;
        int decode_ev_fb_lc1881(unsigned int* ptr, int size, int level, ostream& os) const;
        int decode_ev_fb_lc1881_tof(unsigned int* ptr, int size, int level, ostream& os) const;
        int decode_ev_word(unsigned int* ptr, int size, const char* txt, int level, ostream& os) const;
        int decode_ev_scaler(unsigned int* ptr, int size, const char* txt, int level, ostream& os) const;
        int decode_ev_timestamp(unsigned int* ptr, int size, int level, ostream& os) const;
        int decode_ev_pcitrigtime(unsigned int* ptr, int size, int level, ostream& os) const;
        int decode_ev_drams(unsigned int* ptr, int size, int level, ostream& os) const;
        int decode_tof(unsigned int* ptr, int id, int size, int level, ostream& os) const;
        int decode_c11(unsigned int* ptr, int id, int size, int level, ostream& os) const;
        int decode_options(unsigned int* ptr, int level, ostream& os) const;
        int decode_flags(unsigned int* ptr, int level, ostream& os) const;
        int decode_subevent(unsigned int* ptr, int level, ostream& os) const;
        int decode_event(unsigned int* ptr, int level, ostream& os) const;
        int decode_events(unsigned int* ptr, int level, ostream& os) const;
        int decode_cl_events(unsigned int* ptr, int level, ostream& os) const;
        int decode_cl_ved_info(unsigned int* ptr, int level, ostream& os) const;
        int decode_cl_text(unsigned int* ptr, int level, ostream& os) const;
        int decode_cl_wendy_setup(unsigned int* ptr, int level, ostream& os) const;
        int decode_cl_no_more_data(unsigned int* ptr, int level, ostream& os) const;
        bool valid_;
        unsigned int* data_;
        unsigned int size_;
        unsigned int endian_;
    
        clustertypes typ;
    
    public:
        bool valid() const {return valid_;}
        unsigned int* data() const {return data_;}
        unsigned int size() const {return size_;}
        unsigned int endian() const {return endian_;}
        void decode(int level, ostream& os);
        void count() const;
        void dump_data(unsigned int* ptr, int len, ostream& os) const;
        void dump(int length, ostream& os) const;
        static void print_counts(ostream& os);
    
        static int num_clusters;
        static int num_words;
};

int Cluster::num_clusters=0;
int Cluster::num_words=0;

class Statist {
    public:
        Statist(int max_ved);
        ~Statist();
        int events(int ved, int firstevent, int lastevent, int size, ostream& os);
    protected:
        int max_ved;
        struct Ved {
            int id;
            long words;
            int lastevent;
        };
        Ved* ved;
};

Statist statist(20);

/*****************************************************************************/

int readargs()
{
    args->addoption("infile", 0, "-", "input file", "input");
    args->addoption("outfile", 1, "-", "output file", "output");
    args->addoption("maxrec", "maxrec", maxrec, "maxrec", "max. recordsize");
    args->addoption("level", "level", 0, "decoding level");
    args->addoption("exp", "exp", "c11", "experiment");

    /*
    level 0: only statistic for all clusters
          1: statistic for each cluster
          2: cluster header
          3: Text for textclusters; structure for eventclusters
          4: all data
    */
    if (args->interpret()!=0) return -1;

    level=args->getintopt("level");
    maxrecord=args->getintopt("maxrec");
    return 0;
}
/*****************************************************************************/
static int
parity(ems_u32 v)
{
    v=v^(v>>16);
    v=v^(v>>8);
    v=v^(v>>4);
    v=v^(v>>2);
    v=v^(v>>1);
    return v&1;
}
/*****************************************************************************/
int open_input()
{
    inpath=new C_iopath(args->getstringopt("infile"), C_iopath::iodir_input);
    if (inpath->path()<0) {
        delete inpath;
        return -1;
    }
    cerr<<"inpath: "<<(*inpath)<<endl;
    return 0;
}
/*****************************************************************************/
int open_output()
{
    outpath=cout;
    return 0;
}
/*****************************************************************************/
Statist::Statist(int max_ved)
:max_ved(max_ved)
{
    ved=new Ved[max_ved];
    for (int i=0; i<max_ved; i++) {
        ved[i].id=-1;
        ved[i].words=0;
        ved[i].lastevent=0;
    }
}
/*****************************************************************************/
Statist::~Statist()
{
    delete[] ved;
}
/*****************************************************************************/
int Statist::events(int id, int firstevent, int lastevent, int size,
        ostream& os)
{
    int i;
    for (i=0; i<max_ved; i++) {
        if ((ved[i].id==id) || (ved[i].id==-1)) break;
    }
    if (i==max_ved) {
        os<<"VED "<<id<<": no space available"<<endl;
        return -1;
    }
    ved[i].id=id;
    if (firstevent!=ved[i].lastevent+1) {
        os<<"###### VED "<<id<<": jump from "<<ved[i].lastevent<<" to "<<firstevent<<" ######"<<endl;
    }
    if (ved[i].lastevent/1000!=lastevent/1000)
        cout<<"VED "<<id<<" event "<<lastevent<<" ok, words="<<ved[i].words+size<<endl;
    ved[i].lastevent=lastevent;
    ved[i].words+=size;
    return 0;
}
/*****************************************************************************/
Cluster::Cluster(C_iopath* path, ostream& os)
:valid_(false), data_(0), size_(0)
{
    try {
        if (path->typ()==C_iopath::iotype_tape) {
            os<<"Cluster::Cluster(C_iopath==iotype_tape) not supported"<<endl;
            exit(1);
        } else {
            bool wenden;
            unsigned int head[2];
            path->read(head, 2*sizeof(int));
            endian_=head[1];
            switch (endian_) {
            case 0x12345678:
                wenden=false; break;
            case 0x78563412:
                wenden=true; break;
            default:
                os<<"unknown endian "<<hex<<endian_<<dec<<endl;
                exit(1) ;
            }
            size_=wenden?swap_int(head[0]):head[0];
            data_=new unsigned int[size_+1];
            data_[0]=size_;
            data_[1]=wenden?swap_int(endian_):endian_;
            path->read(data_+2, (head[0]-1)*sizeof(int));
            size_++;
            if (wenden) {
                for (int i=2; i<size_; i++) data_[i]=swap_int(data_[i]);
            }
            valid_=true;
            return;
        }
    } catch (C_status_error* e) {
        os<<(*e)<<endl;
        delete e;
        return;
    }
}
/*****************************************************************************/
void Cluster::dump_data(unsigned int* ptr, int len, ostream& os) const
{
    int lines=len/4;
    int rest=len%4;
    int i, l, o=0;
    os<<hex; os.fill('0');
    for (l=0; l<lines; l++)
      {
      os<<setw(8)<<o<<": ";
      for (i=0; i<4; i++, o++) os<<setw(8)<<ptr[o]<<' ';
      os<<endl;
      }
    if (rest>0)
      {
      os<<setw(8)<<o<<": ";
      for (i=0; i<rest; i++, o++) os<<setw(8)<<ptr[o]<<' ';
      os<<endl;
      }
    os<<dec; os.fill(' ');
}
/*****************************************************************************/
void Cluster::dump(int length, ostream& os) const
{
    int s=(length>0)?(length<size_?length:size_):size_;
    dump_data(data_, s, os);
}
/*****************************************************************************/
int Cluster::decode_ev_fb_ph10Cx(unsigned int* ptr, int size, int level, ostream& os) const
{
    unsigned int* sptr=ptr;
    int wc=*ptr++;

    os<<"fb_ph10Cx: "<<endl;
    //dump_data(sptr-4, size+10, os);
    while (ptr-sptr<wc+1) // ignore last word
      {
    //   unsigned superheader=*ptr++; used++;
    //   os<<"uid="<<((superheader>>24)&0xff);
    //   os<<" ga="<< ((superheader>>16)&0x1f);
    //   int words=superheader&0x3ff;
    //   os<<" words="<<words<<endl;

      unsigned int header=*ptr++;
      os<<hex<<header<<dec;
      os<<" uid="<<((header>>24)&0xff);
      os<<" ga="<<((header>>16)&0x1f);
      int chans=(header>>10)&0x3f;
      os<<" channels="<<chans;
      os<<" event="<<(header&0x3ff)<<endl;
      int words=(chans+1)/2;
      if (level>4)
        {
        for (int word=0; word<words; word++)
          {
          unsigned int w=*ptr++;
          unsigned int a=(w>>16)&0xffff;
          unsigned int b=w&0xffff;
          if ((a&0x8000)==0)
            {
            int c=(a>>10)&0x1f;
            int d=a&0x3ff;
            os<<'('<<c<<','<<d<<')'<<' ';
            }
          if ((b&0x8000)==0)
            {
            int c=(b>>10)&0x1f;
            int d=b&0x3ff;
            os<<'('<<c<<','<<d<<')'<<' ';
            }
          }
        os<<endl;
        }
      else
        {
        ptr+=words;
        }
      }
    return wc+1;
}
/*****************************************************************************/
int Cluster::decode_ev_fb_lc1875a(unsigned int* ptr, int size, int level,
    ostream& os) const
{
    unsigned int* sptr=ptr;
    int wc=*ptr++;

    if (level>3) {
        os<<"FB_lc1875a: "<<endl;

        if (level>4) {
            while (ptr-sptr<wc) {
                ems_u32 w=*ptr++;
                int ga=(w>>27)&0x1f;
                int event=(w>>24)&1;
                int range=(w>>23)&1;
                int c=(w>>16)&0x7f;
                int d=w&0xfff;
                os<<"ga="<<ga<<" event "<<event<<" range="<<range<<" channel="<<c
                  <<" data="<<d<<endl;
            }
        }
    }
    return wc+1;
}
/*****************************************************************************/
int Cluster::decode_ev_fb_lc1877(unsigned int* ptr, int size, int level,
    ostream& os) const
{
    unsigned int* sptr=ptr;
    int wc=*ptr++;

    if (level>3) {
        os<<"FB_lc1877: "<<endl;
        //dump_data(sptr, size+8, os);
        while (ptr-sptr<wc) {
            ems_u32 header=*ptr++;
            int ga=(header>>27)&0x1f;
            int buffer=(header>>11)&0x7;
            int words=(header&0x7ff)-1;
            if (parity(header)) {
                os<<"######  wrong parity  ######"<<endl;
            }
            os<<hex; os.fill('0');
            os<<setw(8)<<header<<": ";
            os<<dec; os.fill(' ');
            os<<" ga="<<ga;
            os<<" buffer="<<buffer;
            os<<" channels="<<words<<endl;
            if (level>4) {
                for (int word=0; word<words; word++) {
                    ems_u32 w=*ptr++;
                    int w_ga=(w>>27)&0x1f;
                    int w_buffer=(w>>24)&0x3;
                    int c=(w>>17)&0x7f;
                    int edge=!!(w&0x10000);
                    int d=w&0xffff;
                    if (parity(w)) {
                        os<<"######  wrong parity  ######"<<endl;
                    }
                    if ((w_ga!=ga) || (w_buffer!=(buffer&3))) {
                        os<<"######    ######"<<endl;
                    }
                    os<<'('<<c<<','<<edge<<','<<d<<')'<<' ';
                }
                os<<endl;
            } else {
                ptr+=words;
            }
        }
    }
    return wc+1;
}
/*****************************************************************************/
int Cluster::decode_ev_fb_lc1881(unsigned int* ptr, int size, int level,
    ostream& os) const
{
    unsigned int* sptr=ptr;
    int wc=*ptr++;

    if (level>3) {
        os<<"FB_lc1881: "<<endl;
        //dump_data(sptr, size+8, os);
        while (ptr-sptr<wc) {
            unsigned int header=*ptr++;
            os<<hex; os.fill('0');
            os<<setw(8)<<header<<": ";
            os<<dec; os.fill(' ');
            int ga=(header>>27)&0x1f;
            os<<" ga="<<ga;
            os<<" buffer="<<((header>>7)&0x3f);
            int words=(header&0x7f)-1;
            os<<" channels="<<words<<endl;
            if (level>4) {
                for (int word=0; word<words; word++) {
                    unsigned int w=*ptr++;
                    int c=(w>>17)&0x7f;
                    int d=w&0x3fff;
                    os<<'('<<c<<','<<d<<')'<<' ';
                }
                os<<endl;
            } else {
                ptr+=words;
            }
        }
    }
    return wc+1;
}
/*****************************************************************************/
int Cluster::decode_ev_fb_lc1881_tof(unsigned int* ptr, int size, int level, ostream& os) const
{
    unsigned int* sptr=ptr;

    os<<"FB_lc1881: "<<endl;
    //dump_data(sptr, size+8, os);
    int nummod=*ptr++;
    os<<nummod<<"*(rp,wp)"<<endl;
    for (int mod=0; mod<nummod; mod++) {
        unsigned int ss, data;
        ss=*ptr++; data=*ptr++;
        if (ss==0) {
            int rp, wp;
            rp=(data>>8)&0x3f;
            wp=data&0x3f;
            os<<"("<<rp<<", "<<wp<<") ";
        } else {
            os<<"ss=="<<ss<<" ";
        }
    }
    os<<endl;

    int wc=*ptr++;
    if (level>3) {
        while (ptr-sptr<wc) {
            unsigned int header=*ptr++;
            os<<hex; os.fill('0');
            os<<setw(8)<<header<<": ";
            os<<dec; os.fill(' ');
            int ga=(header>>27)&0x1f;
            os<<" ga="<<ga;
            os<<" buffer="<<((header>>7)&0x3f);
            int words=(header&0x7f)-1;
            os<<" channels="<<words<<endl;
            if (level>4) {
                for (int word=0; word<words; word++) {
                    unsigned int w=*ptr++;
                    int c=(w>>17)&0x7f;
                    int d=w&0x3fff;
                    os<<'('<<c<<','<<d<<')'<<' ';
                }
                os<<endl;
            } else {
                ptr+=words;
            }
        }
    }
    return wc+1;
}
/*****************************************************************************/
int Cluster::decode_ev_word(unsigned int* ptr, int size, const char* txt,
        int level, ostream& os) const
{
    if (level>3) {
        os<<txt<<": ";
        os<<hex; os.fill('0');
        for (int i=0; i<size; i++) os<<setw(8)<<ptr[i]<<' ';
        os<<endl;
        os<<dec; os.fill(' ');
    }
    return size;
}
/*****************************************************************************/
int Cluster::decode_ev_scaler(unsigned int* ptr, int size, const char* txt,
        int level, ostream& os) const
{
    int used=0;
    if (level>3) os<<txt<<": "<<endl;

    while (used<size) {
        int num=*ptr++; used++;
        if (level>3) os<<"num="<<num<<endl;
        if (size-used<2*num) {
            os<<"need "<<2*num<<" datawords, got only "<<size-used<<endl;
            return 0;
        }
        if (level>4) {
            for (int i=0; i<num; i++) {
                int h, l;
                h=*ptr++; l=*ptr++; used+=2;
                long w=(((long)h)<<32)+l;
                os<<w; if (i+1<num) cout<<' ';
            }
            os<<endl;
        } else {
            ptr+=2*num;
            used+=2*num;
        }
    }
    if (used!=size) {
        os<<"decode_ev_scaler("<<txt<<"): size="<<size<<" used="<<used<<endl;
        exit(1);
    }
    return size;
}
/*****************************************************************************/
int Cluster::decode_ev_timestamp(unsigned int* ptr, int size, int level,
        ostream& os) const
{
    struct timeval tv;
    tv.tv_sec=*ptr++;
    tv.tv_usec=*ptr++;
    struct tm *tm;
    char ss[1024];
    tm=localtime(&tv.tv_sec);
    strftime(ss, 1024, "%e. %b %H:%M:", tm);
    float sec=tm->tm_sec+tv.tv_usec/1000000.;
    if (level>3) {
        os.setf(ios::fixed,ios::floatfield);
        os.precision(6);
        os.fill('0');
        os<<"Timestamp: "<<ss<<setw(9)<<sec<<endl;
        os.fill(' ');
    }
    return 2;
}
/*****************************************************************************/
int
Cluster::decode_ev_drams(unsigned int* ptr, int size, int level, ostream& os) const
{
    if (level>3) {
        os<<"DRAMS:"<<endl;
        if (level>4) dump_data(ptr, size, os);
    }
    return size;
}
/*****************************************************************************/
int Cluster::decode_ev_pcitrigtime(unsigned int* ptr, int size, int level, ostream& os) const
{
    struct timeval tv;

    tv.tv_sec=*ptr++;
    tv.tv_usec=*ptr++;
    int tdt=*ptr++;
    int rejected=*ptr++;

    if (level>3) {
        struct tm *tm;
        char ss[1024];
        tm=localtime(&tv.tv_sec);
        strftime(ss, 1024, "%e. %b %H:%M:", tm);
        float sec=tm->tm_sec+tv.tv_usec/1000000.;
        os.setf(ios::fixed,ios::floatfield);
        os.precision(6);
        os.fill('0');
        os<<"GetPCITrigTime: "<<ss<<setw(9)<<sec;
        os.fill(' ');
        os<<" tdt="<<tdt<<" rejected="<<rejected<<endl;
    }
    return 4;
}
/*****************************************************************************/
int
Cluster::decode_c11(unsigned int* ptr, int id, int size, int level, ostream& os) const
{
    unsigned int* sptr=ptr;

    switch (id) {
    case 2:
    // fb_lc1810_check.2 8
    // fb_lc1881_readout.5 0xbf8 0 1
    // fb_lc1881_check_buf.1 0xbf8 0
    // fb_lc1877_readout.5 0x7f000 0 0 0 18 7
    // fb_lc1877_check_buf.1 0x7f000 0
    // fb_lc1810_reset.1
        if (level>3) os<<"fb4"<<endl;
        ptr+=decode_ev_fb_lc1881(ptr, size, level, os);
        ptr+=decode_ev_fb_lc1877(ptr, size, level, os);
        break;
    case 1:
    // fb_lc1810_check.2 9
    // fb_lc1875a_readout.3 2040 0
    // fb_lc1810_reset.1 9
        if (level>3) os<<"fb3"<<endl;
        ptr+=decode_ev_fb_lc1875a(ptr, size, level, os);
        break;
    case 3: // DRAMS_Readout
        ptr+=decode_ev_drams(ptr, size, level, os);
        break;
    case 4: // Scaler2551_update_read {1 1 0xfff} Scaler2551_update_read {1 2 0xfff}
        ptr+=decode_ev_scaler(ptr, size, "Scaler2551", level, os);
        break;
    case 5: // Scaler4434_update_read {2 1 0xffffffff}
        ptr+=decode_ev_scaler(ptr, size, "Scaler4434", level, os);
        break;
    case 6: // 2*nAFread.1 (LC_REGISTER_2371)
        if (size!=2) {
            os<<"### REGISTER_2371: size="<<size<<" ###"<<endl;
            exit(1);
        }
        ptr+=decode_ev_word(ptr, size, "REGISTER_2371", level, os);
        break;
    case 7: // nix? LC_DISC_4413
        os<<"nix; not decoded"<<endl; exit(1);
        break;
    case 8: // Timestamp.1 {1}
        ptr+=decode_ev_timestamp(ptr, size, level, os);
        break;
    case 1000: // GetPCITrigTime {}
        ptr+=decode_ev_pcitrigtime(ptr, size, level, os);
        break;
    default:
        os<<"unknown IS "<<id<<endl;
    }
    return ptr-sptr;
}
/*****************************************************************************/
int
Cluster::decode_tof(unsigned int* ptr, int id, int size, int level, ostream& os) const
{
    unsigned int* sptr=ptr;

    switch (id) {
    case 1: // Timestamp 1
        ptr+=decode_ev_timestamp(ptr, size, level, os);
        break;
    case 2: // sis3600read
        os<<"sis3600read; not decoded"<<endl; exit(1);
        break;
    case 3: // sis3800ShadowUpdate
        os<<"sis3800ShadowUpdate; not decoded"<<endl; exit(1);
        break;
    case 4: // 2* Scaler2551_update_read
        ptr+=decode_ev_scaler(ptr, size, "Scaler2551", level, os);
        break;
    case 5: // 3*Scaler4434_update_read
        ptr+=decode_ev_scaler(ptr, size, "Scaler4434", level, os);
        break;
    case 10: // nAFread (Latch C219)
        ptr+=decode_ev_word(ptr, size, "C219", level, os);
        break;
    case 20: // fb_lc1881_readout.1
        ptr+=decode_ev_fb_lc1881_tof(ptr, size, level, os);
        break;
    case 21: // fb_ph10Cx_read
        ptr+=decode_ev_fb_ph10Cx(ptr, size, level, os);
        break;
    case 22: // fb_lc1881_readout.1
        ptr+=decode_ev_fb_lc1881_tof(ptr, size, level, os);
        break;
    case 23: // fb_ph10Cx_read
        ptr+=decode_ev_fb_ph10Cx(ptr, size, level, os);
        break;
    default:
        os<<"unknown IS "<<id<<endl;
        exit(1);
    }
    return ptr-sptr;
}
/*****************************************************************************/
int
Cluster::decode_subevent(unsigned int* ptr, int level, ostream& os) const
{
    unsigned int* sptr=ptr;
    int isid=*ptr++;
    int size=*ptr++;
    if (level>3) {
        os<<"isid "<<isid<<", size "<<size<<endl;
        //dump_data(ptr, size, os);
    }
    //ptr+=decode_tof(ptr, isid, size, level, os);
    ptr+=decode_c11(ptr, isid, size, level, os);

    int used=ptr-sptr;
    if (used!=size+2) {
        os<<"decode_subevent: used="<<used<<" size="<<size<<endl;
        exit(1);
    }

    return used;
}
/*****************************************************************************/
int Cluster::decode_event(unsigned int* ptr, int level, ostream& os) const
{
    unsigned int* sptr=ptr;
    int used;
    int size=*ptr++;
    int evno=*ptr++;
    int trigno=*ptr++;
    int nsubevent=*ptr++;
    if (level>2) os<<"event "<<evno<<", trigger "<<trigno<<", "
                   <<nsubevent<<" subevents"<<endl;
    for (int subevent=0; subevent<nsubevent; subevent++) {
        int used=decode_subevent(ptr, level, os);
        ptr+=used;
    }
    used=ptr-sptr;
    if (used!=size+1) {
        os<<"decode_event: used="<<used<<" size="<<size<<endl;
        exit(1);
    }
    return used;
}
/*****************************************************************************/
int Cluster::decode_events(unsigned int* ptr, int level, ostream& os) const
{
    unsigned int* sptr=ptr;
    int ved_id=*ptr++;
    int fragment_id=*ptr++;
    int nevent=*ptr++;
    if (level>0) {
        os<<"VED "<<ved_id<<", "<<nevent<<" events; "<<ptr[1]
            <<"..."<<ptr[1]+nevent-1<<endl;
    }
    statist.events(ved_id, ptr[1], ptr[1]+nevent-1, size_, os);
    for (int event=0; event<nevent; event++) {
        int used=decode_event(ptr, level, os);
        ptr+=used;
    }
    return ptr-sptr;
}
/*****************************************************************************/
int Cluster::decode_options(unsigned int* ptr, int level, ostream& os) const
{
    //os<<"decode_options("<<ptr-data_<<")"<<endl;
    unsigned int* sptr=ptr;
    int s=*ptr++;
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
            os.setf(ios::fixed,ios::floatfield);
            os.precision(6);
            os.fill('0');
            os<<"timestamp: "<<ss<<setw(9)<<sec<<endl;
            os.fill(' ');
            //os<<"timestamp: sec="<<tv.tv_sec<<" usec="<<tv.tv_usec<<endl;
        }
        if (f&2) { // checksum
            int c=*ptr++;
            os<<"checksum: "<<hex<<c<<dec<<endl;
        }
    }
    return ptr-sptr;
}
/*****************************************************************************/
int Cluster::decode_flags(unsigned int* ptr, int level, ostream& os) const
{
    //os<<"decode_flags("<<ptr-data_<<")"<<endl;
    unsigned int* sptr=ptr;
    int flags=*ptr++;
    if (flags!=0) os<<"flags="<<hex<<flags<<dec<<endl;
    return ptr-sptr;
}
/*****************************************************************************/
int Cluster::decode_cl_events(unsigned int* ptr, int level, ostream& os) const
{
    //os<<"decode_cl_events("<<ptr-data_<<")"<<endl;
    unsigned int* sptr=ptr;
    int used;

    used=decode_options(ptr, level, os);
    ptr+=used;

    used=decode_flags(ptr, level, os);
    ptr+=used;

    used=decode_events(ptr, level, os);
    ptr+=used;

    return ptr-sptr;
}
/*****************************************************************************/
int Cluster::decode_cl_ved_info(unsigned int* ptr, int level, ostream& os) const
{
    unsigned int* sptr=ptr;
    int used;
    used=decode_options(ptr, level, os);
    ptr+=used;
    unsigned int version=*ptr++;
    os<<"version "<<hex<<setiosflags(ios::showbase)<<setw(8)
      <<version
      <<resetiosflags(ios::showbase)<<dec<<endl;
    switch (version) {
    case 0x80000001:
        {
        int nved=*ptr++;
        os<<nved<<" VEDs:"<<endl;
        for (int ved=0; ved<nved; ved++) {
            int vedid=*ptr++;
            os<<"  "<<vedid<<" :";
            int nis=*ptr++;
            for (int is=0; is<nis; is++) {
                int isid=*ptr++;
                os<<" "<<isid;
            }
            os<<endl;
        }
        }
        break;
    default:
        os<<"version unknown"<<endl;
        dump(-1, os);
        exit(1);
    }
    return ptr-sptr;
}
/*****************************************************************************/
int Cluster::decode_cl_text(unsigned int* ptr, int level, ostream& os) const
{
    unsigned int* sptr=ptr;
    int used;
    used=decode_options(ptr, level, os);
    ptr+=used;
    used=decode_flags(ptr, level, os);
    ptr+=used;
    return ptr-sptr;
}
/*****************************************************************************/
int Cluster::decode_cl_wendy_setup(unsigned int* ptr, int level, ostream& os) const
{
    unsigned int* sptr=ptr;
    int used;
    used=decode_options(ptr, level, os);
    ptr+=used;
    return ptr-sptr;
}
/*****************************************************************************/
int Cluster::decode_cl_no_more_data(unsigned int* ptr, int level, ostream& os) const
{
    unsigned int* sptr=ptr;
    int used;
    used=decode_options(ptr, level, os);
    ptr+=used;
    return ptr-sptr;
}
/*****************************************************************************/
void Cluster::decode(int level, ostream& os)
{
    //os<<"decode("<<(void*)data_<<")"<<endl;
    unsigned int* ptr=data_+2;
    char* tt;
    int used;
    //dump_data(ptr, 100, os);
    typ=(clustertypes)*ptr++;
    switch (typ) {
    case clusterty_events:
        tt="events";       break;
    case clusterty_ved_info:
        tt="ved_info";     break;
    case clusterty_text:
        tt="text";         break;
    case clusterty_wendy_setup:
        tt="wendy_setup";  break;
    case clusterty_no_more_data:
        tt="no_more_data"; break;
    default:
        os<<"unknown clustertype "<<(int)typ<<endl;
        exit(1);
    }
    if (level>0) {
        os<<"===== "<<"type= "<<tt<<" size="<<size_<<" ====="<<endl;
    }
    switch (typ) {
    case clusterty_events:
        used=decode_cl_events(ptr, level, os); break;
    case clusterty_ved_info:
        used=decode_cl_ved_info(ptr, level, os); break;
    case clusterty_text:
        used=decode_cl_text(ptr, level, os); break;
    case clusterty_wendy_setup:
        used=decode_cl_wendy_setup(ptr, level, os); break;
    case clusterty_no_more_data:
        used=decode_cl_no_more_data(ptr, level, os); break;
    default: // never reached
        os<<"unknown clustertype "<<(int)typ<<endl;
        exit(1);
    }
    ptr+=used;
    if (ptr-data_!=size_) {
        os<<"### *** size="<<size_<<"; used="<<ptr-data_<<" *** ###"<<endl;
        exit(1);
    }
}
/*****************************************************************************/
void Cluster::count() const
{
    num_clusters++;
    num_words+=size_;
}
/*****************************************************************************/
void Cluster::print_counts(ostream& os)
{
    os<<num_clusters<<" clusters"<<endl;
    os<<num_words<<" words"<<endl;
}
/*****************************************************************************/
void do_scan()
{
    int weiter=1;
    Cluster* cluster;

    cluster=new Cluster(inpath, outpath);
    while (cluster->valid()) {
        cluster->decode(level, outpath);
        cluster->count();
        cluster=new Cluster(inpath, outpath);
    }
    Cluster::print_counts(outpath);
}
/*****************************************************************************/
main(int argc, char* argv[])
{
    args=new C_readargs(argc, argv);
    if (readargs()!=0) return 0;

    if (open_input()<0) return 1;
    if (open_output()<0) return 1;

    do_scan();

    inpath->close();
    //outpath->close();
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
