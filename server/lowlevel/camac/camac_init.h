/*
 * lowlevel/camac/camac_init.h
 * created: 2007-Jul-16 PW
 * $ZEL: camac_init.h,v 1.2 2017/10/09 20:51:44 wuestner Exp $
 */

#ifndef _camac_init_h_
#define _camac_init_h_

errcode camac_init_pcicamac(struct camac_dev* dev, char* rawmempart);
errcode camac_init_cc16(struct camac_dev* dev, char* rawmempart);
errcode camac_init_jorway73a(struct camac_dev* dev, char* rawmempart);
errcode camac_init_lnxdsp(struct camac_dev* dev, char* rawmempart);
errcode camac_init_vcc2117(struct camac_dev* dev, char* rawmempart);
errcode camac_init_sis5100(struct camac_dev* dev, char* rawmempart);

#endif
