/*
 * objects/domain/domain.h
 *
 * $ZEL: domain.h,v 1.4 2007/10/15 15:25:33 wuestner Exp $
 */

#ifndef _domain_h_
#define _domain_h_

#include <emsctypes.h>

errcode domain_init(void);
errcode domain_done(void);
errcode DownloadDomain(ems_u32* p, unsigned int num);
errcode UploadDomain(ems_u32* p, unsigned int num);
errcode DeleteDomain(ems_u32* p, unsigned int num);

#endif
