/*
 * ems_unsol_message.cc  
 * 
 * created: 17.03.95 PW
 * 29.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <ems_unsol_message.hxx>
#include <errors.hxx>
#include "versions.hxx"

VERSION("Jun 29 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: ems_unsol_message.cc,v 2.6 2004/11/26 22:20:38 wuestner Exp $")
#define XVERSION

extern int running;
 
/*****************************************************************************/

C_unsol_nichts_message::C_unsol_nichts_message(const C_unsol_message& msg)
:C_unsol_message(msg)
{}

/*****************************************************************************/

C_unsol_nichts_message::~C_unsol_nichts_message(void)
{}

/*****************************************************************************/

void C_unsol_nichts_message::print()
{
rest(buf);
}

/*****************************************************************************/

C_unsol_disco_message::C_unsol_disco_message(const C_unsol_message& msg)
:C_unsol_message(msg)
{}

/*****************************************************************************/

C_unsol_disco_message::~C_unsol_disco_message(void)
{}

/*****************************************************************************/

void C_unsol_disco_message::print()
{
int size, i, j;
size=buf.size();
cout << "  message size: "<<size<<endl<<hex;
if (size<=50)
  {
  cout << "  all words(hex):"<<endl<<hex;
  for (i=0; i<size; i++)
    {
    cout << ' ' << buf[i];
    if (i%10==9) cout << endl;
    }
  cout <<dec<<endl;
  }
else
  {
  cout << "  first words(hex):"<<endl<<hex;
  for (i=0; i<20; i++)
    {
    cout << ' ' << buf[i];
    if (i%10==9) cout << endl;
    }
  cout <<dec<<endl;
  cout << "  last words(hex):"<<endl<<hex;
  for (i=size-20, j=0; i<size; i++, j++)
    {
    cout << ' ' << buf[i];
    if (j%10==9) cout << endl;
    }
  cout <<dec<<endl;
  }
}

/*****************************************************************************/

C_unsol_bye_message::C_unsol_bye_message(const C_unsol_message& msg)
:C_unsol_message(msg)
{}

/*****************************************************************************/

C_unsol_bye_message::~C_unsol_bye_message(void)
{}

/*****************************************************************************/

void C_unsol_bye_message::print()
{
rest(buf);
running=0;
throw new C_program_error("Bye request from commu.");
}

/*****************************************************************************/

C_unsol_test_message::C_unsol_test_message(const C_unsol_message& msg)
:C_unsol_message(msg)
{}

/*****************************************************************************/

C_unsol_test_message::~C_unsol_test_message(void)
{}

/*****************************************************************************/

void C_unsol_test_message::print()
{
rest(buf);
}

/*****************************************************************************/

C_unsol_lam_message::C_unsol_lam_message(const C_unsol_message& msg)
:C_unsol_message(msg)
{}

/*****************************************************************************/

C_unsol_lam_message::~C_unsol_lam_message(void)
{}

/*****************************************************************************/

void C_unsol_lam_message::print()
{
int size, i, j;
size=buf.size();
cout << "  message size: "<<size<<endl<<hex;
if (size<=50)
  {
  cout << "  all words(hex):"<<endl<<hex;
  for (i=0; i<size; i++)
    {
    cout << ' ' << buf[i];
    if (i%10==9) cout << endl;
    }
  cout <<dec<<endl;
  }
else
  {
  cout << "  first words(hex):"<<endl<<hex;
  for (i=0; i<20; i++)
    {
    cout << ' ' << buf[i];
    if (i%10==9) cout << endl;
    }
  cout <<dec<<endl;
  cout << "  last words(hex):"<<endl<<hex;
  for (i=size-20, j=0; i<size; i++, j++)
    {
    cout << ' ' << buf[i];
    if (j%10==9) cout << endl;
    }
  cout <<dec<<endl;
  }
}

/*****************************************************************************/

C_unsol_logtext_message::C_unsol_logtext_message(const C_unsol_message& msg)
:C_unsol_message(msg)
{}

/*****************************************************************************/

C_unsol_logtext_message::~C_unsol_logtext_message(void)
{}

/*****************************************************************************/

void C_unsol_logtext_message::print()
{
rest(buf);
}

/*****************************************************************************/

C_unsol_logmsg_message::C_unsol_logmsg_message(const C_unsol_message& msg)
:C_unsol_message(msg)
{}

/*****************************************************************************/

C_unsol_logmsg_message::~C_unsol_logmsg_message(void)
{}

/*****************************************************************************/

void C_unsol_logmsg_message::print()
{
int direction;
STRING ved, client;
msgheader header;
int* body;

buf >> direction >> ved >> client;

int idx, i;
idx=buf.index();
for (i=0; i<headersize; i++) ((int*)&header)[i]=buf[idx++];
body=new int[header.size];
for (i=0; i<header.size; i++) body[i]=buf[idx++];

cout << "    " << client << (direction?" <== ":" ==> ") << ved << "   xid "
    << header.transid << endl;

C_EMS_message msg(body, header, loop, 1);
msg.setnames(client, ved);
msg.print();
}

/*****************************************************************************/

C_unsol_rterr_message::C_unsol_rterr_message(const C_unsol_message& msg)
:C_unsol_message(msg)
{}

/*****************************************************************************/

C_unsol_rterr_message::~C_unsol_rterr_message(void)
{}

/*****************************************************************************/

void C_unsol_rterr_message::print()
{
int size, i, j;
size=buf.size();
cout << "  message size: "<<size<<endl<<hex;
if (size<=50)
  {
  cout << "  all words(hex):"<<endl<<hex;
  for (i=0; i<size; i++)
    {
    cout << ' ' << buf[i];
    if (i%10==9) cout << endl;
    }
  cout <<dec<<endl;
  }
else
  {
  cout << "  first words(hex):"<<endl<<hex;
  for (i=0; i<20; i++)
    {
    cout << ' ' << buf[i];
    if (i%10==9) cout << endl;
    }
  cout <<dec<<endl;
  cout << "  last words(hex):"<<endl<<hex;
  for (i=size-20, j=0; i<size; i++, j++)
    {
    cout << ' ' << buf[i];
    if (j%10==9) cout << endl;
    }
  cout <<dec<<endl;
  }
}

/*****************************************************************************/

C_unsol_data_message::C_unsol_data_message(const C_unsol_message& msg)
:C_unsol_message(msg)
{}

/*****************************************************************************/

C_unsol_data_message::~C_unsol_data_message(void)
{}

/*****************************************************************************/

void C_unsol_data_message::print()
{
int size, i, j;
size=buf.size();
cout << "  message size: "<<size<<endl<<hex;
if (size<=50)
  {
  cout << "  all words(hex):"<<endl<<hex;
  for (i=0; i<size; i++)
    {
    cout << ' ' << buf[i];
    if (i%10==9) cout << endl;
    }
  cout <<dec<<endl;
  }
else
  {
  cout << "  first words(hex):"<<endl<<hex;
  for (i=0; i<20; i++)
    {
    cout << ' ' << buf[i];
    if (i%10==9) cout << endl;
    }
  cout <<dec<<endl;
  cout << "  last words(hex):"<<endl<<hex;
  for (i=size-20, j=0; i<size; i++, j++)
    {
    cout << ' ' << buf[i];
    if (j%10==9) cout << endl;
    }
  cout <<dec<<endl;
  }
}

/*****************************************************************************/

C_unsol_text_message::C_unsol_text_message(const C_unsol_message& msg)
:C_unsol_message(msg)
{}

/*****************************************************************************/

C_unsol_text_message::~C_unsol_text_message(void)
{}

/*****************************************************************************/

void C_unsol_text_message::print()
{
rest(buf);
}

/*****************************************************************************/

C_unsol_warn_message::C_unsol_warn_message(const C_unsol_message& msg)
:C_unsol_message(msg)
{}

/*****************************************************************************/

C_unsol_warn_message::~C_unsol_warn_message(void)
{}

/*****************************************************************************/

void C_unsol_warn_message::print()
{
int size, i, j;
size=buf.size();
cout << "  message size: "<<size<<endl<<hex;
if (size<=50)
  {
  cout << "  all words(hex):"<<endl<<hex;
  for (i=0; i<size; i++)
    {
    cout << ' ' << buf[i];
    if (i%10==9) cout << endl;
    }
  cout <<dec<<endl;
  }
else
  {
  cout << "  first words(hex):"<<endl<<hex;
  for (i=0; i<20; i++)
    {
    cout << ' ' << buf[i];
    if (i%10==9) cout << endl;
    }
  cout <<dec<<endl;
  cout << "  last words(hex):"<<endl<<hex;
  for (i=size-20, j=0; i<size; i++, j++)
    {
    cout << ' ' << buf[i];
    if (j%10==9) cout << endl;
    }
  cout <<dec<<endl;
  }
}

/*****************************************************************************/

C_unsol_patience_message::C_unsol_patience_message(const C_unsol_message& msg)
:C_unsol_message(msg)
{}

/*****************************************************************************/

C_unsol_patience_message::~C_unsol_patience_message(void)
{}

/*****************************************************************************/

void C_unsol_patience_message::print()
{
int size, i, j;
size=buf.size();
cout << "  message size: "<<size<<endl<<hex;
if (size<=50)
  {
  cout << "  all words(hex):"<<endl<<hex;
  for (i=0; i<size; i++)
    {
    cout << ' ' << buf[i];
    if (i%10==9) cout << endl;
    }
  cout <<dec<<endl;
  }
else
  {
  cout << "  first words(hex):"<<endl<<hex;
  for (i=0; i<20; i++)
    {
    cout << ' ' << buf[i];
    if (i%10==9) cout << endl;
    }
  cout <<dec<<endl;
  cout << "  last words(hex):"<<endl<<hex;
  for (i=size-20, j=0; i<size; i++, j++)
    {
    cout << ' ' << buf[i];
    if (j%10==9) cout << endl;
    }
  cout <<dec<<endl;
  }
}

/*****************************************************************************/

C_unsol_unknown_message::C_unsol_unknown_message(const C_unsol_message& msg)
:C_unsol_message(msg)
{}

/*****************************************************************************/

C_unsol_unknown_message::~C_unsol_unknown_message(void)
{}

/*****************************************************************************/

void C_unsol_unknown_message::print()
{
rest(buf);
}

/*****************************************************************************/
/*****************************************************************************/
