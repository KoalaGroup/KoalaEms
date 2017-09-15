/******************************************************************************
*                                                                             *
* emsc_message.hxx.m4                                                         *
*                                                                             *
* OSF1/ULTRIX                                                                 *
*                                                                             *
* created: 18.03.95                                                           *
* last changed: 23.03.95                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/
// $Id: emsc_message.hxx.m4,v 2.4 1999/02/05 12:26:39 wuestner Exp $

#ifndef emsc_message_hxx
#define emsc_message_hxx
#include <ems_message.hxx>

/*****************************************************************************/

class C_emsc_message: public C_ems_message
  {
  public:
    C_emsc_message(const C_EMS_message& msg)
      :C_ems_message(msg), buf(msg.buffer(), msg.size()){}
    C_emsc_message(const C_ems_message& msg)
      :C_ems_message(msg), buf(msg.buffer(), msg.size()){}
    C_emsc_message(const C_emsc_message& msg)
      :C_ems_message(msg), buf(msg.buffer(), msg.size()){}
  protected:
    C_inbuf buf;
  public:
    C_emsc_message& operator = (const C_emsc_message&);
    virtual void print();
  };

/*****************************************************************************/

class C_emsc_unknown_message: public C_emsc_message
  {
  public:
    C_emsc_unknown_message(const C_emsc_message& msg)
      :C_emsc_message(msg){}
  protected:
  public:
    C_emsc_unknown_message& operator = (const C_emsc_unknown_message&);
    virtual void print();
  };

/*****************************************************************************/

define(`version', `')
define(`Req', `

class C_$1_c_message: public C_emsc_message
  {
  public:
    C_$1_c_message(const C_emsc_message& msg)
      :C_emsc_message(msg){}
  protected:
  public:
    C_$1_c_message& operator = (const C_$1_c_message&);
    virtual void print();
  };
')

include(COMMON_INCLUDE/requesttypes.inc)

/*****************************************************************************/

#endif

/*****************************************************************************/
/*****************************************************************************/
