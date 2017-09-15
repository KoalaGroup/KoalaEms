/*
 * lowlevel/camac/sis5100/lnxdspcamac.h
 *
 * $ZEL: lnxdspcamac.h,v 1.1 2005/12/04 21:59:12 wuestner Exp $
 *
 * created: 31.Jul.2005 PW
 */

#ifndef _lnxdspcamac_h_
#define _lnxdspcamac_h_

#include <sconf.h>
#include <sys/types.h>
#include <emsctypes.h>
#include "../../../objects/domain/dom_ml.h"

struct camac_lnxdsp_info {
    int camac_path;
};

struct lnxdsp_camadr_t {
    int n, a, f;
};

errcode camac_init_lnxdsp(char* pathname, char* semi2, struct camac_dev* dev);

#endif
