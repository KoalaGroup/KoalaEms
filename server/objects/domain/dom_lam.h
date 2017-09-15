/*
 * objects/domain/dom_lam.h
 * $ZEL: dom_lam.h,v 1.4 2007/10/15 15:23:40 wuestner Exp $
 */

#ifndef _dom_lam_h_
#define _dom_lam_h_

errcode dom_lam_init(void);
errcode dom_lam_done(void);
errcode downloadlamlist(ems_u32*, unsigned int);
errcode uploadlamlist(ems_u32*, unsigned int);
errcode deletelamlist(ems_u32*, unsigned int);

#endif
