/*
 * ved_adr.hxx
 * 
 * $ZEL: ved_addr.hxx,v 2.8 2004/11/18 19:31:48 wuestner Exp $
 * 
 * created 21.01.95
 * 26.03.1998 PW: adapded for <string>
 * 14.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#ifndef _ved_adr_hxx_
#define _ved_adr_hxx_

#include "config.h"
#include "cxxcompat.hxx"
#include <stringlist.hxx>
#include <outbuf.hxx>
#include <inbuf.hxx>

/*****************************************************************************/

class C_VED_addr;

ostream& operator <<(ostream&, const C_VED_addr&);
ostream& operator <<(ostream&, const C_VED_addr*);
C_outbuf& operator <<(C_outbuf&, const C_VED_addr&);

class C_VED_addr
  {
  friend ostream& operator <<(ostream&, const C_VED_addr&);
  friend ostream& operator <<(ostream&, const C_VED_addr*);
  friend C_outbuf& operator <<(C_outbuf&, const C_VED_addr&);
  public:
    C_VED_addr();
    C_VED_addr(const C_VED_addr&);
    C_VED_addr(C_inbuf&);
    virtual ~C_VED_addr();
  protected:
    virtual ostream& print(ostream&) const;
  public:
    typedef enum {notvalid, builtin, local, inet, inet_path, VICbus} types;
    virtual types type() const {return(notvalid);}
    C_VED_addr& operator =(const C_VED_addr&);
    virtual void out_ved_addr(C_outbuf&) const;
    virtual int outsize() const;
    virtual int operator ==(const C_VED_addr&) const;
    virtual int operator !=(const C_VED_addr&) const;
    virtual C_VED_addr* clone() const {return new C_VED_addr(*this);}
  };

class C_VED_addr_builtin: public C_VED_addr
  {
  public:
    C_VED_addr_builtin(const STRING&);
    C_VED_addr_builtin(const C_VED_addr_builtin&);
    C_VED_addr_builtin(C_inbuf&);
    virtual ~C_VED_addr_builtin();
  protected:
    STRING name_;
    virtual ostream& print(ostream&) const;
  public:
    virtual types type() const {return(builtin);}
    const STRING& name() const {return(name_);}
    C_VED_addr_builtin& operator =(const C_VED_addr_builtin&);
    virtual void out_ved_addr(C_outbuf&) const;
    virtual int outsize() const;
    virtual int operator ==(const C_VED_addr&) const;
    virtual int operator !=(const C_VED_addr&) const;
    virtual C_VED_addr* clone() const {return new C_VED_addr_builtin(*this);}
  };

class C_VED_addr_unix: public C_VED_addr
  {
  public:
    C_VED_addr_unix(const STRING&);
    C_VED_addr_unix(const C_VED_addr_unix&);
    C_VED_addr_unix(C_inbuf&);
    virtual ~C_VED_addr_unix();
  protected:
    STRING sockname_;
    virtual ostream& print(ostream&) const;
  public:
    virtual types type() const {return(local);}
    const STRING& sockname() const {return(sockname_);}
    C_VED_addr_unix& operator =(const C_VED_addr_unix&);
    virtual void out_ved_addr(C_outbuf&) const;
    virtual int outsize() const;
    virtual int operator ==(const C_VED_addr&) const;
    virtual int operator !=(const C_VED_addr&) const;
    virtual C_VED_addr* clone() const {return new C_VED_addr_unix(*this);}
  };

class C_VED_addr_inet: public C_VED_addr
  {
  public:
    C_VED_addr_inet(const STRING&, unsigned short);
    C_VED_addr_inet(const C_VED_addr_inet&);
    C_VED_addr_inet(C_inbuf&);
    virtual ~C_VED_addr_inet();
  protected:
    STRING hostname_;
    unsigned short port_;
    virtual ostream& print(ostream&) const;
  public:
    virtual types type() const {return(inet);}
    const STRING& hostname() const {return(hostname_);}
    unsigned short port() const {return(port_);}
    C_VED_addr_inet& operator =(const C_VED_addr_inet&);
    virtual void out_ved_addr(C_outbuf&) const;
    virtual int outsize() const;
    virtual int operator ==(const C_VED_addr&) const;
    virtual int operator !=(const C_VED_addr&) const;
    virtual C_VED_addr* clone() const {return new C_VED_addr_inet(*this);}
  };

class C_VED_addr_inet_path: public C_VED_addr_inet
  {
  public:
//  C_VED_addr_inet_path(const char*, unsigned short, C_strlist*);
    C_VED_addr_inet_path(const STRING&, unsigned short, C_strlist*);
    C_VED_addr_inet_path(const C_VED_addr_inet_path&);
    C_VED_addr_inet_path(C_inbuf&);
    virtual ~C_VED_addr_inet_path();
  protected:
    C_strlist* pathes;
    virtual ostream& print(ostream&) const;
  public:
    virtual types type() const {return(inet_path);}
    int numpathes() const {return(pathes->size());}
    const char* path(int i) const {return((*pathes)[i]);}
    C_VED_addr_inet_path& operator =(const C_VED_addr_inet_path&);
    virtual void out_ved_addr(C_outbuf&) const;
    virtual int outsize() const;
    virtual int operator ==(const C_VED_addr&) const;
    virtual int operator !=(const C_VED_addr&) const;
    virtual C_VED_addr* clone() const {return new C_VED_addr_inet_path(*this);}
  };

class C_VED_addr_vic: public C_VED_addr
  {
  public:
    C_VED_addr_vic();
    C_VED_addr_vic(const C_VED_addr_vic&);
    C_VED_addr_vic(C_inbuf&);
    virtual ~C_VED_addr_vic();
  protected:
    virtual ostream& print(ostream&) const;
  public:
    virtual types type() const {return(VICbus);}
    C_VED_addr_vic& operator =(const C_VED_addr_vic&);
    virtual void out_ved_addr(C_outbuf&) const;
    virtual int outsize() const;
    virtual int operator ==(const C_VED_addr&) const;
    virtual int operator !=(const C_VED_addr&) const;
    virtual C_VED_addr* clone() const {return new C_VED_addr_vic(*this);}
  };

C_VED_addr* extract_ved_addr(C_inbuf&);

#endif

/*****************************************************************************/
/*****************************************************************************/
