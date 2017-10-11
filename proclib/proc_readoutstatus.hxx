/*
 * proc_readoutstatus.hxx
 * 
 * created: 10.06.97 PW
 *
 * $ZEL: proc_readoutstatus.hxx,v 2.7 2014/07/14 15:11:54 wuestner Exp $
 */

#ifndef _proc_readoutstatus_hxx_
#define _proc_readoutstatus_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <sys/time.h>
#include <objecttypes.h>
#include <inbuf.hxx>

using namespace std;

/*****************************************************************************/

class C_readoutstatus
  {
  public:
    C_readoutstatus(C_inbuf&, int useaux=1);
    C_readoutstatus(InvocStatus, int, struct timeval&);
    ~C_readoutstatus();
  protected:
    InvocStatus status_;
    ems_u32 eventcount_;
    int time_valid_;
    struct timeval timestamp_;
    int numaux_;
    int* aux_key_;
    int* aux_size_;
    int** aux_;
  public:
    ems_u32 eventcount() const {return(eventcount_);}
    InvocStatus status() const {return(status_);}
    int time_valid() const {return time_valid_;}
    void settime(const struct timeval&);
    double gettime() const;
    double evrate(const C_readoutstatus&) const;
    int numaux() const {return numaux_;}
    int auxkey(int i) const {return aux_key_[i];}
    int auxsize(int i) const {return aux_size_[i];}
    int aux(int i, int j) const {return  aux_[i][j];}
    int* aux(int i) const {return  aux_[i];}
    ostream& print(ostream&) const;
  };

ostream& operator <<(ostream&, const C_readoutstatus&);

#endif

/*****************************************************************************/
/*****************************************************************************/
