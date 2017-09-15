/*
 * objects/do/dataout.h
 * 
 * $ZEL: dataout.h,v 1.3 2004/06/18 23:26:50 wuestner Exp $
 */

#ifndef _obj_do_dataout_h_
#define _obj_do_dataout_h_

errcode do_init(void);
errcode do_done(void);
errcode WindDataout(ems_u32* p, int num);
errcode WriteDataout(ems_u32* p, int num);
errcode GetDataoutStatus(ems_u32* p, int num);
errcode EnableDataout(ems_u32* p, int num);
errcode DisableDataout(ems_u32* p, int num);
plerrcode flush_dataout(ems_u32* p, int num);

#endif
