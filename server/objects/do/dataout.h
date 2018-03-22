/*
 * objects/do/dataout.h
 * 
 * $ZEL: dataout.h,v 1.4 2017/10/20 23:21:31 wuestner Exp $
 */

#ifndef _obj_do_dataout_h_
#define _obj_do_dataout_h_

errcode do_init(void);
errcode do_done(void);
errcode WindDataout(ems_u32* p, unsigned int num);
errcode WriteDataout(ems_u32* p, unsigned int num);
errcode GetDataoutStatus(ems_u32* p, unsigned int num);
errcode EnableDataout(ems_u32* p, unsigned int num);
errcode DisableDataout(ems_u32* p, unsigned int num);
plerrcode flush_dataout(ems_u32* p, unsigned int num);

#endif
