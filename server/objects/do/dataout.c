/*
 * objects/do/dataout.c
 * created: 22.09.93
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: dataout.c,v 1.18 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <emsctypes.h>
#include <rcs_ids.h>
#include "dataout.h"
#include "../domain/dom_dataout.h"
#include "../../dataout/dataout.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "objects/do")

/*****************************************************************************/

errcode do_init()
{
T(do_init)
return(init_dataout_handler());
}

/*****************************************************************************/

errcode do_done()
{
T(do_done)
return(done_dataout_handler());
}

/*****************************************************************************/
/*
 * num==2
 * p[0]: DO-Index
 * p[1]: wieviel
 */
errcode WindDataout(ems_u32* p, unsigned int num)
{
T(WindDataout)
if (num!=2) return(Err_ArgNum);
if (p[0]>=MAX_DATAOUT) return(Err_IllDo);
return(windtape(p[0], (int)p[1]));
}

/*****************************************************************************/
/*
 * p[0]: DO-Index
 * p[1]: write default header (should normally be '1')
 *       if 0: header information has to precede data:
            h[0]: clustertype
            h[1]: ClOPTFLAGS (only the flags, options will be generated)
            h[2]: flags
            h[3]: fragment_id
            ...
 * p[2]: Number of following data
 * p[3...]: data to be written
 */
errcode WriteDataout(ems_u32* p, unsigned int num)
{
T(WriteDataout)
if ((num<3)||(p[2]!=num-3)) return(Err_ArgNum);
#ifdef HANDLERCOMSIZE
if(num>(HANDLERCOMSIZE/4))return(Err_NoMem);
#endif
if (p[0]>=MAX_DATAOUT) return(Err_IllDo);
return(writeoutput(p[0], (int)p[1], p+2));
}

/*****************************************************************************/

errcode GetDataoutStatus(ems_u32* p, unsigned int num)
{
int format;
T(GetDataoutStatus)

if (num<1) return(Err_ArgNum);
if (p[0]>=MAX_DATAOUT) return(Err_IllDo);
format=num<2?0:p[1];
return(getoutputstatus(p[0], format));
}

/*****************************************************************************/

errcode EnableDataout(ems_u32* p, unsigned int num)
{
errcode res;
T(EnableDataout)
if (num!=1) return(Err_ArgNum);
if (p[0]>=MAX_DATAOUT) return(Err_IllDo);
res=enableoutput(p[0]);
return(res);
}

/*****************************************************************************/

errcode DisableDataout(ems_u32* p, unsigned int num)
{
errcode res;
T(DisableDataout)
if (num!=1) return(Err_ArgNum);
if (p[0]>=MAX_DATAOUT) return(Err_IllDo);
res=disableoutput(p[0]);
return(res);
}

/*****************************************************************************/
/*
 * p[0...]: list of dataouts to be waited for (not yet implemented)
 */
plerrcode flush_dataout(ems_u32* p, unsigned int num)
{
    unsigned int i;

    T(flush_dataout)
    for (i=0; i<num; i++) {
        if (p[i]>=MAX_DATAOUT) return Err_IllDo;
    }
#ifdef READOUT_CC
    flush_databuf(0);
#endif
/*
 * here we should wait for the listed dataouts do have flushed all data
 */
    return plOK;
}
/*****************************************************************************/
#if 0
#ERROR
/*
p[0] : object (==Object_do)
*/
errcode do_getnamelist(p, num)
{
int i, *help;

T(do_getnamelist)
if (num!=1) return(Err_ArgNum);
get_dataout_list();
/*
help=outptr++;
for (i=0; i<MAX_DATAOUT; i++)
  if (dataout[i].bufftyp!=-1) *outptr++=i;
*help=outptr-help-1;
*/
return(OK);
}

#endif

/*****************************************************************************/
/*****************************************************************************/
