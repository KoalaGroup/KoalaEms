/*
 * lowlevel/unixvme/sis3100/mem_sis.h
 * $ZEL: mem_sis.h,v 1.3 2005/04/12 18:39:53 wuestner Exp $
 * created: 27.Jun.2003
 */

#ifndef _mem_sis_h_
#define _mem_sis_h_

int sis3100_mem_size(struct vme_dev*);
int sis3100_mem_write(struct vme_dev*, ems_u32*, unsigned int ,ems_u32);
int sis3100_mem_read(struct vme_dev*, ems_u32*, unsigned int ,ems_u32);
void sis3100_mem_set_bufferstart(struct vme_dev*, ems_u32, ems_u32);

#ifdef DELAYED_READ
int sis3100_mem_read_delayed(struct vme_dev*, ems_u32*, unsigned int,
        ems_u32, ems_u32);
#endif

#endif
