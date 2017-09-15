/******************************************************************************
*                                                                             *
* proc_memberlist.hxx                                                         *
*                                                                             *
* created: 08.06.97                                                           *
* last changed: 08.06.97                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#ifndef _proc_memberlist_hxx_
#define _proc_memberlist_hxx_
#include <nocopy.hxx>

class C_confirmation;

/*****************************************************************************/

class C_memberlist: public nocopy
  {
  public:
    C_memberlist(int);
    C_memberlist(const C_confirmation*);
    ~C_memberlist();
  protected:
    int max;
    int size_;
    int* addresses;
  public:
    void add(int);
    int size() const {return(size_);}
    int operator[](int i) const {return(addresses[i]);}
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
