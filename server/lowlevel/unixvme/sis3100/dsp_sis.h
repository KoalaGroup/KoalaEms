/*
 * lowlevel/unixvme/sis3100/dsp_sis.h
 * $ZEL: dsp_sis.h,v 1.1 2003/07/03 19:22:27 wuestner Exp $
 * created: 27.Jun.2003
 */

#ifndef _dsp_sis_h_
#define _dsp_sis_h_

int sis3100_dsp_present (struct vme_dev*);
int sis3100_dsp_reset (struct vme_dev*);
int sis3100_dsp_start (struct vme_dev*);
int sis3100_dsp_load (struct vme_dev*, const char*);

#endif
