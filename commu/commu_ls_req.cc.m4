/*
 * commu_ls_req.cc.m4
 * 
 * created 08.11.94 PW
 * 
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <commu_local_server.hxx>
#include <outbuf.hxx>
#include <commu_client.hxx>
#include "versions.hxx"

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_ls_req.cc.m4,v 2.14 2014/07/14 15:12:19 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

extern C_clientlist clientlist;

dnl define(`version', `')
dnl define(`Req', `&C_local_server::$1,')
dnl //errcode (C_local_server::*DoRequest[])(const int* ptr, int num)=
dnl C_local_server::reqfunc C_local_server::DoRequest[]=
dnl {
dnl include(COMMON/requesttypes.inc)
dnl };

/*****************************************************************************/

void C_local_server::do_message_ems(C_message* message)
{
ems_u32* body;
#if 0
int req, size, x;
#else
int size, x;
#endif
errcode res;
msgheader* header;

TR(C_local_server::do_message_ems)
body=message->body;
header=&message->header;
#if 0
req=header->type.reqtype;
#endif
size=header->size;
client_id=header->client;
if ((client=clientlist.get(client_id))==0)
  {
//  elog  << "C_io_server::proceed_message_ems: client " << ident
//      << " existiert nicht" << flush;
  cerr << "C_io_server::proceed_message_ems: client " << client_id
      << " existiert nicht" << endl;
  delete message;
  return;
  }

//res=(this->*DoRequest[req])(body, size);

outbuf.clear();
outbuf << space_(x);
define(`version', `')
define(`Req',`case Req_$1: /*cerr << "$1" << endl;*/ res=$1(body, size); break;')

switch (message->header.type.reqtype)
  {
include(COMMON/requesttypes.inc)

  default:
    cerr << "Req_default" << endl;
    res=Err_NotImpl;
    break;
  }
outbuf << put(x, res);

undefine(`index')

header->flags|=Flag_Confirmation;
header->size=outbuf.index();
delete body;
message->body=outbuf.getbuffer();

client->addmessage(message, 0);
}

/*****************************************************************************/
/*****************************************************************************/
