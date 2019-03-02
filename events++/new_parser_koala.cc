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
#include "global.hxx"
#include "KoaLoguru.hxx"
#include "decoder_koala.hxx"
#include "KoaAssembler.hxx"
#include "KoaAnalyzer.hxx"

using namespace std;
using namespace DecodeUtil;

#define U32(a) (static_cast<uint32_t>(a))
#define swap_int(a) ( (U32(a) << 24) |                \
                      (U32((a) << 8) & 0x00ff0000) |  \
                      (U32((a) >> 8) & 0x0000ff00) |  \
                      (U32(a) >>24) )


static bool use_simplestructure;
static const char* outputfile;
static int max_tsdiff;
static char* const *files;

static EmsIsInfo ISes[]={
  {"scalor",parserty_scalor,1},
  {"mxdc32",parserty_mxdc32,10},
  {"time",parserty_time,100},
  {"",parserty_invalid,static_cast<uint32_t>(-1)}
};

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
int
parse_file(int p)
{
  Decoder* decoder=new Decoder();

  //
  private_list* evtlist=new private_list();
  decoder->SetPrivateList(evtlist);

  //
  decoder->SetParsers(ISes);

  //
  KoaAssembler* assembler=new KoaAssembler();
  decoder->SetAssembler(assembler);

  //
  KoaAnalyzer*  analyzer=new KoaSimpleAnalyzer(outputfile,use_simplestructure,max_tsdiff);
  decoder->SetAnalyzer(analyzer);

  //
  int res;
  decoder->Init();

  do {
    uint32_t *buf;
    int num;

    num=read_cluster(p, &buf);
    if (num<=0)  { /* no buf allocated */
      res=-1;
      break;
    }

    res=decoder->DecodeCluster(buf,num);
    // res=num;
    
    delete[] buf;
  } while (res>0);

  if(res<0){
    cout << endl;
    cout << "****************************************************" << endl;
    cout << "* FATAL ERROR IN DECODING! PLEASE CHECK YOUR DATA! *" << endl;
    cout << "****************************************************" << endl;
  } 
  else
    cout << "Decoding Successfully" <<endl;

  decoder->Print();
  decoder->Done();

  delete evtlist;
  delete decoder;

  return res;

}

//---------------------------------------------------------------------------//
void
printusage(const char* argv0)
{
  fprintf(stderr, "usage: %s [-h] [-v verbosity] [-s] [-m max_tsdiff ][-o outputfile] [file ...]\n", argv0);
  fprintf(stderr, "       -h: print this help and exit\n");
  fprintf(stderr, "       -v: verbosity\n");
  fprintf(stderr, "       -s: output simple flat tree structure\n");
  fprintf(stderr, "       -m: QDC included in the DAQ, and max_tsdiff is the max timestamp diff between QDC and other modules (which should be the master gate width/clock width)");
  fprintf(stderr, "       outputfile: the basename of the output files when the data input is stdin");
  fprintf(stderr, "       file ...: one or more ems cluster file(s)\n");
  fprintf(stderr, "            data input may also come from stdin\n");
}

//---------------------------------------------------------------------------//
int
readargs(int argc, char* argv[])
{
  int c;
  bool err=false;

  while (!err && ((c=getopt(argc, argv, "hsm:o:")) != -1)) {
    switch (c) {
    case 'h':
      printusage(argv[0]);
      return 1;
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
void
prepare_globals()
{
  use_simplestructure=false;
  outputfile="koala_data";
  max_tsdiff=2;
}

//---------------------------------------------------------------------------//
int
main(int argc, char* argv[])
{
  loguru::g_stderr_verbosity = loguru::Verbosity_WARNING;
  // loguru::g_flush_interval_ms=1;
  // loguru::g_preamble = false;
  // loguru::g_preamble_verbose = true;
  // loguru::g_preamble_date = false;
  loguru::g_preamble_thread = false;
  loguru::g_preamble_uptime = false;

  loguru::init(argc,argv);
  loguru::add_file("everything.log",loguru::Append, loguru::Verbosity_MAX);
  loguru::add_file("latest.log",loguru::Truncate, loguru::Verbosity_INFO);

  //
  int res;

  prepare_globals();

  if ((res=readargs(argc, argv)))
    return res<0?1:0;

  if (*files) {
    while (*files) {
      int p;
      p=open(*files, O_RDONLY);
      if (p<0) {
        LOG_F(ERROR,"cannot open \"%s\": %s\n", *files, strerror(errno));
        return 2;
      }

      printf("FILE %s\n", *files);
      outputfile=*files;
      res=parse_file(p);
      close(p);
      if (res<0)
        break;

      files++;
    }
  } else {
    res=parse_file(0);
  }

  return res<0?3:0;
}
