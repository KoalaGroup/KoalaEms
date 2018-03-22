/*
 * dataout/cluster/do_dummy.c
 * created      14.04.97
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: do_dummy.c,v 1.9 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <objecttypes.h>
#include <actiontypes.h>
#include <rcs_ids.h>
#include "do_cluster.h"
#include "../../objects/notifystatus.h"
#include "../../objects/pi/readout.h"

RCS_REGISTER(cvsid, "dataout/cluster")

/*****************************************************************************/
static errcode do_dummy_start(int idx)
{
T(dataout/cluster/do_dummy.c:do_dummy_start)
dataout_cl[idx].do_status=Do_running;
dataout_cl[idx].suspended=0;

do_statist[idx].clusters=0;
do_statist[idx].words=0;
do_statist[idx].events=0;
do_statist[idx].suspensions=0;

return OK;
}
/*****************************************************************************/
static errcode do_dummy_reset(int idx)
{
T(dataout/cluster/do_dummy.c:do_dummy_reset)
dataout_cl[idx].do_status=Do_neverstarted;
return OK;
}
/*****************************************************************************/
static void do_dummy_cleanup(__attribute__((unused)) int idx)
{
T(dataout/cluster/do_dummy.c:do_dummy_cleanup)
}
/*****************************************************************************/
static void do_dummy_freeze(__attribute__((unused)) int do_idx)
{}
/*****************************************************************************/
static void do_dummy_advertise(int idx, struct Cluster* cl)
{
    T(dataout/cluster/do_dummy.c:do_dummy_advertise)
    do_statist[idx].clusters++;
    do_statist[idx].words+=cl->size;
    if (cl->type==clusterty_no_more_data) {
        printf("do_dummy_advertise: final cluster\n");
        dataout_cl[idx].do_status=Do_done;
        dataout_cl[idx].procs.reset(idx);
        notify_do_done(idx);
    }
}
/*****************************************************************************/
static struct do_procs dummy_procs={
  do_dummy_start,
  do_dummy_reset,
  do_dummy_freeze,
  do_dummy_advertise,
  /*patherror*/ do_NotImp_void_ii,
  do_dummy_cleanup,
  /*wind*/ do_NotImp_err_ii,
  /*status*/ 0,
  /*filename*/ 0,
  };
/*****************************************************************************/
errcode do_dummy_init(int do_idx)
{
T(dataout/cluster/do_dummy.c:do_dummy_init)

dataout_cl[do_idx].procs=dummy_procs;
dataout_cl[do_idx].suspended=0;
dataout_cl[do_idx].do_status=Do_neverstarted;
dataout_cl[do_idx].doswitch=
    readout_active==Invoc_notactive?Do_enabled:Do_disabled;
return OK;
}
/*****************************************************************************/
/*****************************************************************************/
