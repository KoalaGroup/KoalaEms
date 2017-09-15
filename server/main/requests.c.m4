/*
 * requests.c.m4
 * created: 1993-Jan-14
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: requests.c.m4,v 1.11 2011/04/06 20:30:28 wuestner Exp $";

#include "requests.h"
#include <requesttypes.h>
#include <errorcodes.h>
#include "rcs_ids.h"

RCS_REGISTER(cvsid, "main")

/*****************************************************************************/

define(`version', `')
define(`Req',`extern errcode $1(const ems_u32*, unsigned int);')
include(EMSCOMMON/requesttypes.inc)

/*****************************************************************************/

define(`Req', `$1,')
RequestHandler DoRequest[]=
{
include(EMSCOMMON/requesttypes.inc)
};

define(`Req', `"$1",')
const char* RequestName[]=
{
include(EMSCOMMON/requesttypes.inc)
0,
};

/*****************************************************************************/
/*****************************************************************************/
