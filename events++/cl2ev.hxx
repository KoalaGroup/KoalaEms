/*
 * cl2ev.hxx
 * 
 * created: 25.05.1998 PW
 * 07.Apr.2003 PW: cxxcompat.hxx used
 */

#ifndef _cl2ev_hxx_
#define _cl2ev_hxx_

#include "config.h"
#include "cxxcompat.hxx"
#include <readargs.hxx>
#include "clusterreader.hxx"

class C_cluster2event
  {
  public:
    C_cluster2event(int, char*[]);
    ~C_cluster2event();
    int run();
  protected:
    int argc;
    char** argv;
    C_readargs* args;
    STRING infile, outfile;
    int ilit, olit;
    int swap_out, recsize, sloppy, async, maxmem, evnum, maxev,
        statclust, statsec, loop;

    int readargs();
    int do_convert_async(C_clusterreader&, C_iopath&);
    void do_convert_sloppy(C_clusterreader&, C_iopath&);
    void do_convert_complete_with_mem(C_clusterreader&, C_iopath&);
    void do_convert_complete(C_clusterreader&, C_iopath&);
    static void printheader(ostream&);
    static void printstreams(ostream&);
    void write_event(int*, C_iopath&);
  };

#endif
