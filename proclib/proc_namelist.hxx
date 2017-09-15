/******************************************************************************
*                                                                             *
* proc_namelist.hxx                                                           *
*                                                                             *
* created: 08.06.97                                                           *
* last changed: 13.06.97                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#ifndef _proc_namelist_hxx_
#define _proc_namelist_hxx_

#include <objecttypes.h>

class C_confirmation;

/*****************************************************************************/

class C_namelist
  {
  public:
    C_namelist(C_confirmation*);
    ~C_namelist();
  protected:
    int size_;
    int* list_;
  public:
    int exists(int) const;
    const int* list() const {return list_;}
    int size() const {return size_;}
    int operator [] (int idx) const {return list_[idx];}
  };

// class P_namelist
//   {
//   private:
//     C_namelist* p;
//   public:
//     P_namelist() :p(0) {}
//     ~P_namelist() {delete p;}
//     C_namelist* operator = (C_namelist* pp) {delete p; p=pp; return pp;}
//     C_namelist* operator -> ();
//   };

#endif

/*****************************************************************************/
/*****************************************************************************/
