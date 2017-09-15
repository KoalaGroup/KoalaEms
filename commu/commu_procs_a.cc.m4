/*
 * commu_procs_a.cc.m4
 * 
 * created 07.12.94 PW
 */

#include "commu_local_server.hxx"
#include "commu_local_server_a.hxx"
#include "versions.hxx"

VERSION("Dec 07 1994", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_procs_a.cc.m4,v 2.4 2004/11/26 15:14:26 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

define(`proc',`{&C_local_server_a::proc_$1,
&C_local_server_a::test_proc_$1,
C_local_server_a::name_proc_$1,
&C_local_server_a::ver_proc_$1},')

C_local_server_a::listproc C_local_server_a::Proc[]=
{
include(SRC/commu_procs_a.inc)
};

define(`proc',`{&C_local_server_a::prop_proc_$1},')
C_local_server_a::listprop C_local_server_a::Prop[]=
{
include(SRC/commu_procs_a.inc)
};

int C_local_server_a::NrOfProcs=
    sizeof(C_local_server_a::Proc)/sizeof(C_local_server_a::listproc);

/*****************************************************************************/
/*****************************************************************************/
