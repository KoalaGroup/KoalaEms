/*
 * templates.cc
 * 
 * created 15.06.1998 PW
 */

#include "commu_list_t.hxx"
#include "commu_client.hxx"
#include "commu_message.hxx"
#include "commu_list_t.cc"
#include "versions.hxx"

#ifndef XVERSION
VERSION("Jun 15 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_templates.cc,v 2.3 2004/11/26 15:14:30 wuestner Exp $")
#define XVERSION
#endif

/*****************************************************************************/
#ifdef __GNUC__
template class C_list<C_client>;
template class C_list<C_server>;
template class C_list<C_server_info>;
template class C_list<C_message>;
#else
#endif
/*****************************************************************************/
/*****************************************************************************/
