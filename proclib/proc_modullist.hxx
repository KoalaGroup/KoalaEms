/*
 * proc_modullist.hxx
 * created 08.06.97
 * 22.Jan.2001 PW: multicrate support
 * $Id: proc_modullist.hxx,v 2.4 2006/02/14 21:37:13 wuestner Exp $
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
        ~C_modullist();
    C_modullist(const C_confirmation*, int version);
#if 0
/* already in common/objecttypes.h */
    typedef enum {
        modul_none, modul_unspec, modul_generic, modul_camac,
        modul_fastbus, modul_vme
    } modul_class;
#endif
    struct ml_entry {
        int modultype;
        enum Modulclass modulclass;
        union { /* selected by modulclass */
            struct {
                int crate;
                int addr;
            } adr2;
            struct {
                int length;
                int* data;
            } generic; /* future? */
            struct {
                int addr;
            } unspec; /* backward compatibility */
        } address;
    };
  protected:
    int max;
    int size_;
    ml_entry* entry;
  public:
    void add(int, int);
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
