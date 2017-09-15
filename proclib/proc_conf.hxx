/*
 * proc_conf.hxx
 * 
 * created: 19.08.94 PW
 * 12.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#ifndef _proc_conf_hxx_
#define _proc_conf_hxx_

#include "config.h"
#include "cxxcompat.hxx"

#include <requesttypes.h>
#include <nocopy.hxx>
#include <msg.h>

class C_VED;

/*****************************************************************************/

class C_confirmation: public nocopy
  {
  public:
    C_confirmation(ems_i32* conf, int size, Request request);
    C_confirmation(ems_i32* conf, int size, Request request, void(*)(ems_i32*));
    C_confirmation(ems_i32* conf, msgheader header);
    C_confirmation(ems_i32* conf, msgheader header, void(*)(ems_i32*));
    C_confirmation(const C_confirmation&);
    C_confirmation();
    ~C_confirmation();
  protected:
    class C_conf
      {
      friend class C_confirmation;
      protected:
        int count;
        C_conf(ems_i32* conf, int size, Request request, void(*)(ems_i32*)=0);
        C_conf(ems_i32* conf, msgheader header, void(*)(ems_i32*)=0);
        ~C_conf();
        const C_VED* ved_;
        ems_i32* buffer;
        msgheader* header;
        void (*deleter)(ems_i32*);
        int size;
        Request request;
        void operator ++();
        void operator ++(int);
        void operator --();
        void operator --(int);
      };
    C_conf* conf;
  public:
    int valid() const {return conf!=0;}
    C_confirmation& operator = (const C_confirmation&);
    ems_i32* buffer() const {return(conf->buffer);}
    ems_i32 buffer(int i) const {return(conf->buffer[i]);}
    msgheader* header() const {return(conf->header);}
    int size() const {return(conf->size);}
    Request request() const {return(conf->request);}
    const C_VED* ved() const {return conf->ved_;}
    void ved(const C_VED* v) {conf->ved_=v;}
    ostream& print(ostream&) const;
    ostream& print_body(ostream&) const;
    ostream& print_error(ostream&) const;
    ostream& print_header(ostream&) const;
 };

ostream& operator <<(ostream&, const C_confirmation&);

#endif

/*****************************************************************************/
/*****************************************************************************/
