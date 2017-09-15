/*
 * commu_procs_l.cc.m4
 * 
 * created 05.02.95 PW
 */

#include "commu_local_server.hxx"
#include "commu_local_server_l.hxx"
#include "versions.hxx"

VERSION("Sep 11 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_procs_l.cc.m4,v 2.4 2004/11/26 15:14:27 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

define(`proc',`{&C_local_server_l::proc_$1,
&C_local_server_l::test_proc_$1,
C_local_server_l::name_proc_$1,
&C_local_server_l::ver_proc_$1},')

C_local_server_l::listproc C_local_server_l::Proc[]=
{
include(SRC/commu_procs_l.inc)
};

define(`proc',`{&C_local_server_l::prop_proc_$1},')
C_local_server_l::listprop C_local_server_l::Prop[]=
{
include(SRC/commu_procs_l.inc)
};

int C_local_server_l::NrOfProcs=
    sizeof(C_local_server_l::Proc)/sizeof(C_local_server_l::listproc);

/*****************************************************************************/
/*****************************************************************************/
