/******************************************************************************
*                                                                             *
* ems_unsol_message.hxx                                                       *
*                                                                             *
* OSF1/ULTRIX                                                                 *
*                                                                             *
* created: 17.03.95                                                           *
* last changed: 23.03.95                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/
// $Id: ems_unsol_message.hxx,v 2.4 1999/02/05 12:26:38 wuestner Exp $

#ifndef ems_unsol_message_hxx
#define ems_unsol_message_hxx
#include <ems_message.hxx>

/*****************************************************************************/

class C_unsol_nichts_message: public C_unsol_message
  {
  public:
    C_unsol_nichts_message(const C_unsol_message&);
    virtual ~C_unsol_nichts_message();
  protected:
  public:
    virtual void print();
  };

/*****************************************************************************/

class C_unsol_disco_message: public C_unsol_message
  {
  public:
    C_unsol_disco_message(const C_unsol_message&);
    virtual ~C_unsol_disco_message();
  protected:
  public:
    virtual void print();
  };

/*****************************************************************************/

class C_unsol_bye_message: public C_unsol_message
  {
  public:
    C_unsol_bye_message(const C_unsol_message&);
    virtual ~C_unsol_bye_message();
  protected:
  public:
    virtual void print();
  };

/*****************************************************************************/

class C_unsol_test_message: public C_unsol_message
  {
  public:
    C_unsol_test_message(const C_unsol_message&);
    virtual ~C_unsol_test_message();
  protected:
  public:
    virtual void print();
  };

/*****************************************************************************/

class C_unsol_lam_message: public C_unsol_message
  {
  public:
    C_unsol_lam_message(const C_unsol_message&);
    virtual ~C_unsol_lam_message();
  protected:
  public:
    virtual void print();
  };

/*****************************************************************************/

class C_unsol_logtext_message: public C_unsol_message
  {
  public:
    C_unsol_logtext_message(const C_unsol_message&);
    virtual ~C_unsol_logtext_message();
  protected:
  public:
    virtual void print();
  };

/*****************************************************************************/

class C_unsol_logmsg_message: public C_unsol_message
  {
  public:
    C_unsol_logmsg_message(const C_unsol_message&);
    virtual ~C_unsol_logmsg_message();
  protected:
  public:
    virtual void print();
  };

/*****************************************************************************/

class C_unsol_rterr_message: public C_unsol_message
  {
  public:
    C_unsol_rterr_message(const C_unsol_message&);
    virtual ~C_unsol_rterr_message();
  protected:
  public:
    virtual void print();
  };

/*****************************************************************************/

class C_unsol_data_message: public C_unsol_message
  {
  public:
    C_unsol_data_message(const C_unsol_message&);
    virtual ~C_unsol_data_message();
  protected:
  public:
    virtual void print();
  };

/*****************************************************************************/

class C_unsol_text_message: public C_unsol_message
  {
  public:
    C_unsol_text_message(const C_unsol_message&);
    virtual ~C_unsol_text_message();
  protected:
  public:
    virtual void print();
  };

/*****************************************************************************/

class C_unsol_warn_message: public C_unsol_message
  {
  public:
    C_unsol_warn_message(const C_unsol_message&);
    virtual ~C_unsol_warn_message();
  protected:
  public:
    virtual void print();
  };

/*****************************************************************************/

class C_unsol_patience_message: public C_unsol_message
  {
  public:
    C_unsol_patience_message(const C_unsol_message&);
    virtual ~C_unsol_patience_message();
  protected:
  public:
    virtual void print();
  };

/*****************************************************************************/

class C_unsol_unknown_message: public C_unsol_message
  {
  public:
    C_unsol_unknown_message(const C_unsol_message&);
    virtual ~C_unsol_unknown_message();
  protected:
  public:
    virtual void print();
  };

/*****************************************************************************/

#endif

/*****************************************************************************/
/*****************************************************************************/
