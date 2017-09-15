/*
 * procs/fastbus/fb_lecroy/fb_lc_util.h
 * 
 * $ZEL: fb_lc_util.h,v 1.7 2005/07/19 19:31:02 wuestner Exp $
 * 
 * created: 13.06.2000
 * 03.06.2002 PW multi crate support                                            *
 * 
 */
#ifndef _fb_lc_util_h_
#define _fb_lc_util_h_

int lc_pat(int type, char* caller);

#define BUFCHECK_MODULES 3 /* lc1876 lc1877 lc1881(M) */

plerrcode lc18XX_buf_A(struct fastbus_dev* dev, int idx, ems_u32 pat, int p_diff,
    int n_buf, ems_u32 modultype, ems_u32 evt, int secs, int blockcheck);
plerrcode lc18XX_buf_B(struct fastbus_dev* dev, int idx, ems_u32 pat, int p_diff,
    int n_buf, ems_u32 modultype, ems_u32 evt, int secs, int blockcheck);

plerrcode lc1876_buf_converted(struct fastbus_dev* dev, ems_u32 pat,
    int p_diff, int evt);
plerrcode lc1877_buf_converted(struct fastbus_dev* dev, ems_u32 pat,
    int p_diff, int evt);
plerrcode lc1881_buf_converted(struct fastbus_dev* dev, ems_u32 pat,
    int p_diff, int evt);

#define lc1876_buf(dev, pat, p_diff, evt, secs, blockcheck) \
    lc18XX_buf_B(dev, 0, pat, p_diff, 8, 0x4310103B, evt, secs, blockcheck)
#define lc1877_buf(dev, pat, p_diff, evt, secs, blockcheck) \
    lc18XX_buf_B(dev, 1, pat, p_diff, 8, 0x4310103C, evt, secs, blockcheck)
#define lc1881_buf(dev, pat, p_diff, evt, secs, blockcheck) \
    lc18XX_buf_B(dev, 2, pat, p_diff, 64, 0x4110104B, evt, secs, blockcheck)

#endif
