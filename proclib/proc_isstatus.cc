/******************************************************************************
*                                                                             *
* proc_isstatus.cc                                                            *
*                                                                             *
* created: 08.06.97                                                           *
* last changed: 08.06.97                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include <errno.h>
#include <stdlib.h>
#include <proc_isstatus.hxx>
#include <proc_conf.hxx>
#include <errors.hxx>
#include "versions.hxx"

VERSION("Jun 05 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: proc_isstatus.cc,v 2.5 2016/05/10 16:24:46 wuestner Exp $")
#define XVERSION

/*****************************************************************************/
C_isstatus::C_isstatus(int id, int enabled, int members, int num_trigger,
    const ems_u32* trigger)
:id_(id), enabled_(enabled), members_(members), num_trigger_(num_trigger)
{
    trigger_=new ems_u32[num_trigger_];
    if (trigger_==0)
        throw new C_unix_error(errno, "alloc triggerlist in isstatus");
    for (int i=0; i<num_trigger_; i++) trigger_[i]=trigger[i];
}
/*****************************************************************************/
C_isstatus::C_isstatus(const C_confirmation* conf)
{
    if (conf->size()<5)
        throw new C_program_error("C_isstatus::C_isstatus: wrong confirmation");

    const ems_i32* b=conf->buffer();
    id_=b[1];
    enabled_=b[2];
    members_=b[3];
    num_trigger_=b[4];
    trigger_=new ems_u32[num_trigger_];
    if (trigger_==0)
        throw new C_unix_error(errno, "alloc triggerlist in isstatus");
    for (int i=0; i<num_trigger_; i++)
        trigger_[i]=static_cast<ems_u32>(b[i+5]);
}
/*****************************************************************************/
/*****************************************************************************/
