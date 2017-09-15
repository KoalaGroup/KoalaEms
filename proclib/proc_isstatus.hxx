/******************************************************************************
*                                                                             *
* proc_isstatus.hxx                                                           *
*                                                                             *
* created: 08.06.97                                                           *
* last changed: 08.06.97                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#ifndef _proc_isstatus_hxx_
#define _proc_isstatus_hxx_

#include <emsctypes.h>
#include <nocopy.hxx>

class C_confirmation;

/*****************************************************************************/

class C_isstatus
  {
  public:
    C_isstatus(int, int, int, int, const ems_u32*);
    C_isstatus(const C_confirmation*);
    ~C_isstatus() {delete[] trigger_;}
  protected:
    int id_;
    int enabled_;
    int members_;
    int num_trigger_;
    ems_u32* trigger_;
  public:
    int id() const {return id_;}
    int enabled() const {return enabled_;}
    int members() const {return members_;}
    int num_trigger() const {return num_trigger_;}
    const ems_u32* triggerlist() const {return trigger_;}
    int trigger(int i) const {return trigger_[i];}
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
