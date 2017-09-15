/*
 * procs/fastbus/sfi/sfi_timeout.c
 * 
 * created      15.03.2000 PW
 * last changed 15.03.2000 PW
 * 
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sfi_timeout.c,v 1.6 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../procs.h"
#include "../../procprops.h"

extern ems_u32* outptr;
extern sfi_info sfi;

RCS_REGISTER(cvsid, "procs/fastbus/sfi")

/*****************************************************************************/
/*
SFIled
p[0] : No. of parameters (==2)
p[1] : int long_timeout
p[2] : int short_timeout
*/

plerrcode proc_SFItimeout(ems_u32* p)
{
int val;
val=(p[1]&0xf)<<4;
val|=(p[1]&0xf);
SFI_W(&sfi, fastbus_timeout, val);
return plOK;
}

plerrcode test_proc_SFItimeout(ems_u32* p)
{
if (p[0]!=2) return(plErr_ArgNum);
return plOK;
}
#ifdef PROCPROPS
static procprop SFItimeout_prop={0, 0, 0, 0};

procprop* prop_proc_SFItimeout()
{
return(&SFIled_prop);
}
#endif
char name_proc_SFItimeout[]="SFItimeout";
int ver_proc_SFItimeout=1;

/*****************************************************************************/
/*****************************************************************************/
