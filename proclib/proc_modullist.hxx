/*
 * proc_modullist.hxx
 * created 08.06.97
 * 22.Jan.2001 PW: multicrate support
 * $Id: proc_modullist.hxx,v 2.6 2016/05/02 15:27:43 wuestner Exp $
 */

#ifndef _proc_modullist_hxx_
#define _proc_modullist_hxx_
#include <nocopy.hxx>
#include <objecttypes.h>

class C_confirmation;

/*****************************************************************************/

class C_modullist: public nocopy {
  public:
    C_modullist(int);
    C_modullist(const C_confirmation*, int version);
    ~C_modullist();
    struct ml_entry {
        ml_entry(void):modulclass(modul_none) {};
        ml_entry(const ml_entry&);
        ~ml_entry();
        ml_entry& operator=(const ml_entry&);
        int modultype;
        enum Modulclass modulclass; // common/objecttypes.h
        union { /* selected by modulclass */
            struct {
                int crate;
                int addr;
            } adr2;
            struct {
                char *address;
                char *protocol;
                char *rport;
                char *lport;
            } ip;
#if 0 // not used (???)
            struct {
                int length;
                int* data;
            } generic; /* future? */
            struct {
                int addr;
            } unspec; /* backward compatibility */
#endif
        } address;
    };
  protected:
    int max;
    int size_;
    ml_entry* entry;
  public:
#if 0 /* modul_unspec no longer exists (old protocol version) */
    void add(int, int);
#endif
    void add(ml_entry&);
    int size() const {return(size_);}
    int type(int i) const {return(entry[i].modultype);}
    enum Modulclass modulclass(int i) const {return(entry[i].modulclass);}
    ml_entry& get(int i) const {return entry[i];};
};

/*
class C_modullist: public nocopy
  {
  public:
    C_modullist(int);
    C_modullist(const C_confirmation*);
    ~C_modullist();
  protected:
    int max;
    int size_;
    int* types;
    int* addresses;
  public:
    void add(int, int);
    int size() const {return(size_);}
    int type(int i) const {return(types[i]);}
    int type_addr(int);
    int address(int i) const {return(addresses[i]);}
  };
*/
#endif

/*****************************************************************************/
/*****************************************************************************/
