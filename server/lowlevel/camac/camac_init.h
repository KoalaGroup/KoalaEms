/*
 * lowlevel/camac/camac_init.h
 * created: 2007-Jul-16 PW
 * $ZEL: camac_init.h,v 1.1 2007/08/07 10:53:17 wuestner Exp $
 */

#ifndef _camac_init_h_
#define _camac_init_h_

errcode camac_init_pcicamac(struct camac_dev* dev, char* rawmempart);
errcode camac_init_cc16(struct camac_dev* dev, char* rawmempart);
errcode camac_init_orway73a(struct camac_dev* dev, char* rawmempart);
errcode camac_init_lnxdsp(struct camac_dev* dev, char* rawmempart);
errcode camac_init_vcc2117(struct camac_dev* dev, char* rawmempart);
errcode camac_init_sis5100(struct camac_dev* dev, char* rawmempart);

#endif
