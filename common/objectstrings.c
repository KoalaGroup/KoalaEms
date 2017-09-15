/*
 * common/objectstrings.c
 * created 2006-Feb-05 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: objectstrings.c,v 2.6 2011/04/07 14:07:08 wuestner Exp $";

#include "objecttypes.h"
#include "rcs_ids.h"

RCS_REGISTER(cvsid, "common")

const char* Modulclass_names[]={
    "none",
    "unspec",
    "generic",
    "camac",
    "fastbus",
    "vme",
    "lvd",
    "perf",
    "can",
    "caenet",
    "sync",
    "pcihl",
    "invalid",
    0,
};
