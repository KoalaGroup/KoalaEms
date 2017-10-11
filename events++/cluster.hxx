/*
 * cluster.hxx
 * created: 15.03.1998
 * 10.May.2001 PW: datasize() added
 */

#ifndef _cluster_hxx_
#define _cluster_hxx_

#include "config.h"
#include <iostream>
#include <clusterformat.h>

using namespace std;

class C_cluster
  {
  public:
    C_cluster(int* cluster, int size, int& num):d(cluster), size_(size),
        initialised(0), num_clusters(num)
      {num_clusters++;}
    virtual ~C_cluster() {if (initialised) destroy(); delete d; num_clusters--;}
  protected:
    int *d, size_;
    int initialised;
    int ved_num, is_num;
    int *ved_ids, *is_ids;
    int& num_clusters;
    void initialise();
    void destroy();
  public:
    clustertypes typ() const {return static_cast<clustertypes>(ClTYPE(d));} 
    int* get_all() const {return d;}
    int* get_top() const {return d+size_;}// first word above clusterdata
    int* get_opts() const {return d+3;}
    int* get_data() const {return d+4+ClOPTSIZE(d);}   // data following options
    int  get_flags() const {return ClFLAGS(d);}        // valid only for events
    int  get_ved_id() const {return ClVEDID(d);}       // valid only for events
    int  get_fragment_id() const {return ClFRAGID(d);} // valid only for events
    int  get_num_events() const {return ClEVNUM(d);}   // valid only for events
    int* get_events() const {return d+8+ClOPTSIZE(d);} // valid only for events
    int  get_vednum();                                 // valid only for info
    int* get_ved_ids();                                // valid only for info
    int  get_isnum();                                  // valid only for info
    int* get_is_ids();                                 // valid only for info
    int  size() const {return size_;}
    int  datasize() const {return size_-4-ClOPTSIZE(d);}
    ostream& dump(ostream& os, int max) const;
    ostream& xdump(ostream& os, int max) const;
    ostream& decode(ostream& os, int max) const;
    ostream& print(ostream&);
  };

ostream& operator <<(ostream&, C_cluster&);

#endif
