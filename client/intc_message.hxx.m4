/*
 * intc_message.hxx.m4
 * created: 21.03.95 PW
 * $ZEL: intc_message.hxx.m4,v 2.5 2007/04/18 19:42:15 wuestner Exp $
 */

#ifndef intc_message_hxx
#define intc_message_hxx
#include <ems_message.hxx>

/*****************************************************************************/

class C_intc_message: public C_ems_message
  {
  public:
    C_intc_message(const C_EMS_message& msg)
      :C_ems_message(msg), buf(msg.buffer(), msg.size()){}
    C_intc_message(const C_ems_message& msg)
      :C_ems_message(msg), buf(msg.buffer(), msg.size()){}
    C_intc_message(const C_intc_message& msg)
      :C_ems_message(msg), buf(msg.buffer(), msg.size()){}
  protected:
    C_inbuf buf;
  public:
    C_intc_message& operator = (const C_intc_message&);
    virtual void print();
  };

/*****************************************************************************/

class C_intc_unknown_message: public C_intc_message
  {
  public:
    C_intc_unknown_message(const C_intc_message& msg) :C_intc_message(msg){}
  protected:
  public:
    C_intc_unknown_message& operator = (const C_intc_unknown_message&);
    virtual void print();
  };

/*****************************************************************************/

define(`version', `')
define(`Intmsg', `

class C_$1_ic_message: public C_intc_message
  {
  public:
    C_$1_ic_message(const C_intc_message& msg) :C_intc_message(msg){}
  protected:
  public:
    C_$1_ic_message& operator = (const C_$1_ic_message&);
    virtual void print();
  };
')

include(COMMON_INCLUDE/intmsgtypes.inc)

/*****************************************************************************/

#endif

/*****************************************************************************/
/*****************************************************************************/
