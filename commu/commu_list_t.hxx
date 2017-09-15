 /*
 * commu_list.hxx
 *
 * $ZEL: commu_list_t.hxx,v 2.7 2006/02/16 21:03:40 wuestner Exp $
 *
 * created 28.07.94 PW
 * 14.06.1998 PW: adapted for STD_STRICT_ANSI
 * 15.06.1998 PW: rewritten
 */

#ifndef _commu_list_hxx_
#define _commu_list_hxx_
#include "config.h"
#include "cxxcompat.hxx"
#include <nocopy.hxx>

/*****************************************************************************/
template <class T>
class C_list:public nocopy
  {
  public:
    C_list(int, STRING name);
    virtual ~C_list();
  protected:
    STRING name_;                         // fuer debugging
    int growsize;
    T **list;
    int listsize;
    int firstfree;
    void growlist();
    void shrinklist();
    virtual void counter() {}
  public:
    T* operator [] (int i) const {return(list[i]);}
    int operator [] (T*);
    int size() const {return(firstfree);}
    void insert(T*);
    void remove(int idx);
    void remove(T*);
    void remove_delete(int idx);
    void remove_delete(T*);
    void delete_all();
    T* extract_first();
    T* extract_last();
    virtual ostream& print(ostream&) const;
  };
template <class T> ostream& operator <<(ostream&, const C_list<T>&);
/*****************************************************************************/
class C_int:public nocopy
  {
  public:
    C_int(int ia): a(ia)/*, valid(0)*/ {}
    C_int(const C_int& i): a(i.a)/*,valid(i.valid)*/ {}
    virtual ~C_int() {}
    C_int& operator = (const C_int&);
    int a;
    //int valid;
    virtual ostream& print(ostream&) const;
  };
ostream& operator <<(ostream&, const C_int&);
/*****************************************************************************/
class C_intpair:public nocopy
  {
  public:
    C_intpair(int ia, int ib): a(ia), b(ib)/*, valid(0)*/{}
    virtual ~C_intpair() {}
    C_intpair& operator = (const C_intpair&);
    int a;
    int b;
    //int valid;
    virtual ostream& print(ostream&) const;
  };
ostream& operator <<(ostream&, const C_intpair&);
/*****************************************************************************/
class C_ints:public C_list<C_int>
  {
  public:
    C_ints(int, STRING name);
    C_ints(const C_ints&);
    virtual ~C_ints() {}
    void free(int);                    // a --> free
    void xfree(int);                    // a --> free
    void add(int);                     // a --> new
    int  exists(int);                  // a exists
    C_intpair* get(int);               // a --> T*       
    int get_idx(int);                  // a --> idx
  };
/*****************************************************************************/
class C_intpairs: public C_list<C_intpair>
  {
  public:
    C_intpairs(int, STRING name);
    virtual ~C_intpairs() {}
    int a(int) const;                  // b --> a
    int b(int) const;                  // a --> b
    void free_a(int);                  // a --> free
    void free_b(int);                  // b --> free
    void add(int, int);                // a, b --> new
    int exists_a(int);                 // a exists
    int exists_b(int);                 // b exists
    C_intpair* get_a(int);             // a --> T*       
    C_intpair* get_b(int);             // b --> T*
    int get_idx_a(int);                // a --> idx
    int get_idx_b(int);                // b --> idx
  };
#endif
/*****************************************************************************/
/*****************************************************************************/
