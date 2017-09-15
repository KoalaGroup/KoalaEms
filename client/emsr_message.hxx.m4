/*
 * emsr_message.hxx.m4
 * created: 18.03.95 PW
 * $ZEL: emsr_message.hxx.m4,v 2.5 2007/04/18 19:42:15 wuestner Exp $
 */

#ifndef emsr_message_hxx
#define emsr_message_hxx
#include <ems_message.hxx>

/*****************************************************************************/

class C_emsr_message: public C_ems_message
  {
  public:
    C_emsr_message(const C_EMS_message& msg)
      :C_ems_message(msg), buf(msg.buffer(), msg.size()){}
    C_emsr_message(const C_ems_message& msg)
      :C_ems_message(msg), buf(msg.buffer(), msg.size()){}
    C_emsr_message(const C_emsr_message& msg)
      :C_ems_message(msg), buf(msg.buffer(), msg.size()){}
  protected:
    C_inbuf buf;
  public:
    C_emsr_message& operator = (const C_emsr_message&);
    virtual void print();
  };

/*****************************************************************************/

class C_emsr_unknown_message: public C_emsr_message
  {
  public:
    C_emsr_unknown_message(const C_emsr_message& msg) :C_emsr_message(msg){}
  protected:
  public:
    C_emsr_unknown_message& operator = (const C_emsr_unknown_message&);
    virtual void print();
  };

/*****************************************************************************/

define(`version', `')
define(`Req', `

class C_$1_r_message: public C_emsr_message
  {
  public:
    C_$1_r_message(const C_emsr_message& msg) :C_emsr_message(msg){}
  protected:
  public:
    C_$1_r_message& operator = (const C_$1_r_message&);
    virtual void print();
  };
')

include(COMMON_INCLUDE/requesttypes.inc)

/*****************************************************************************/

#endif

/*****************************************************************************/
/*****************************************************************************/
