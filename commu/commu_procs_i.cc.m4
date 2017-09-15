/*
 * commu_procs_i.cc.m4
 * 
 * created 17.11.94 PW
 */

#include "commu_local_server.hxx"
#include "commu_local_server_i.hxx"
#include "versions.hxx"

VERSION("Nov 17 1994", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_procs_i.cc.m4,v 2.4 2004/11/26 15:14:26 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

define(`proc',`{&C_local_server_i::proc_$1,
&C_local_server_i::test_proc_$1,
C_local_server_i::name_proc_$1,
&C_local_server_i::ver_proc_$1},')

C_local_server_i::listproc C_local_server_i::Proc[]=
{
include(SRC/commu_procs_i.inc)
};

define(`proc',`{&C_local_server_i::prop_proc_$1},')
C_local_server_i::listprop C_local_server_i::Prop[]=
{
include(SRC/commu_procs_i.inc)
};

int C_local_server_i::NrOfProcs=
    sizeof(C_local_server_i::Proc)/sizeof(C_local_server_i::listproc);

/*****************************************************************************/
/*****************************************************************************/
