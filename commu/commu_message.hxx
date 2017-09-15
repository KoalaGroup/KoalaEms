/*
 * commu_message.hxx
 *
 * $ZEL: commu_message.hxx,v 2.8 2004/11/18 19:31:26 wuestner Exp $
 *
 * created 30.07.93 PW
 * 31.08.1998 PW: C_message::C_message(const msgheader& header, int *body)
 */

#ifndef _commu_message_hxx_
#define _commu_message_hxx_
#include <msg.h>
#include "commu_list_t.hxx"

/*****************************************************************************/
class C_message
  {
  public:
    C_message(const msgheader*, ems_u32*);
    C_message(const msgheader&, ems_u32*);
    C_message();
    C_message(const C_message&);
    C_message(C_message&);
    C_message& operator =(const C_message&);
    C_message& operator =(C_message&);
    virtual ~C_message();
    msgheader header;
    ems_u32 *body;
    virtual ostream& print(ostream&) const;
};
ostream& operator <<(ostream&, const C_message&);
/*****************************************************************************/
class C_messagelist: public C_list<C_message>
  {
  private:
    int lowwatermark;
    int highwatermark;
    int stopped;
  public:
    C_messagelist(int, int, int);
    virtual ~C_messagelist() {}
    int add(C_message*);
  };
#endif
/*****************************************************************************/
/*****************************************************************************/
