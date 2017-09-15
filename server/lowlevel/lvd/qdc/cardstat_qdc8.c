/*
 * lowlevel/lvd/qdc/cardstat_qdc8.c
 * created 2012-Sep-11 PW/PK
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: cardstat_qdc8.c,v 1.2 2013/01/17 22:44:54 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <stdio.h>

#include <modultypes.h>
#include <xprintf.h>
#include <rcs_ids.h>
#include "../lvdbus.h"
#include "../lvd_access.h"
#include "qdc_private.h"

RCS_REGISTER(cvsid, "lowlevel/lvd/qdc")

/*****************************************************************************/
static int
dump_ident(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 val;
    int res, a=0;

    res=lvd_i_r(acard->dev, acard->addr, all.ident, &val);
    if (res) {
        xprintf(xp, "  unable to read ident\n");
        return -1;
    }

    xprintf(xp, "  [%02x] ident       =0x%04x (type=0x%x version=%x.%x)\n",
            a, val, val&0xff, LVD_FWverH(val), LVD_FWverL(val));
    return 0;
}
/*****************************************************************************/
static int
dump_serial(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 val;
    int res, a=2;

    acard->dev->silent_errors=1;
    res=lvd_i_r(acard->dev, acard->addr, all.serial, &val);
    acard->dev->silent_errors=0;
    xprintf(xp, "  [%02x] serial      ", a);
    if (res<0) {
        xprintf(xp, ": not set\n");
    } else {
        xprintf(xp, "=%d\n", val);
    }
    return 0;
}
/*****************************************************************************/
static void
decode_cr(ems_u32 cr, void *xp)
{
    if (cr&QDC_CR_ENA)
        xprintf(xp, " ENA");
    if (cr&QDC_CR_LTRIG)
        xprintf(xp, " LOCAL_TRIGGER");
    if (cr&QDC_CR_LEVTRIG)
        xprintf(xp, " LEVEL_TRIGGER");
    if (cr&QDC_CR_VERBOSE)
        xprintf(xp, " VERBOSE");
    if (cr&QDC_CR_ADCPOL)
        xprintf(xp, " INV_ADC");
    if (cr&QDC_CR_SGRD)
        xprintf(xp, " SGRD");
    if (cr&QDC_CR_TST_SIG)
        xprintf(xp, " TST_SIG");
    if (cr&QDC_CR_ADC_PWR)
        xprintf(xp, " ADC_PWR_OFF");
    if (cr&QDC_CR_F1MODE)
        xprintf(xp, " F1MODE");
    if (cr&QDC_CR_LOCBASE)
        xprintf(xp, " LOCBASE");
    if (cr&QDC_CR_NOPS)
        xprintf(xp, " NOPS");
    if (cr&QDC_CR_LMODE)
        xprintf(xp, " LMODE");
    if (cr&QDC_CR_TRAW)
        xprintf(xp, " TRAW");
}
/*****************************************************************************/
static int
dump_cr(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    ems_u32 cr;
    int res, a=0xc;

    res=lvd_i_r(acard->dev, acard->addr, all.cr, &cr);
    if (res) {
        xprintf(xp, "  unable to read cr\n");
        return -1;
    }

    xprintf(xp, "  [%02x] cr          =0x%04x", a, cr);
    decode_cr(cr, xp);
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
dump_cr_saved(struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
    if (!acard->initialized)
        return 0;
    xprintf(xp, " saved cr          =0x%04x", acard->daqmode);
    decode_cr(acard->daqmode, xp);
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
dump_qdc80_outp (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Set register outp */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register outp */

  offset=  ofs(struct lvd_qdc80_card , outp);

  xprintf(xp, "  [%02x] outp -- bits 0xffff  in outp  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0xffff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_festart (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Add search window to output data (time,baseline) */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register outp */

  offset=  ofs(struct lvd_qdc80_card , outp);

  xprintf(xp, "  [%02x] festart -- bits 0x0001  in outp  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x0001 ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_femin (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Add minimum to output data (time,amplitude) */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register outp */

  offset=  ofs(struct lvd_qdc80_card , outp);

  xprintf(xp, "  [%02x] femin -- bits 0x0002  in outp  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x0002 ) >> 1) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_femax (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Add maximum to output data (time,amplitude) */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register outp */

  offset=  ofs(struct lvd_qdc80_card , outp);

  xprintf(xp, "  [%02x] femax -- bits 0x0004  in outp  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x0004 ) >> 2) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_feend (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Add pulse end  to output data (time,amplitude) */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register outp */

  offset=  ofs(struct lvd_qdc80_card , outp);

  xprintf(xp, "  [%02x] feend -- bits 0x0008  in outp  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x0008 ) >> 3) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_fetot (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Add tot to output data (timeup)(timedown) */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register outp */

  offset=  ofs(struct lvd_qdc80_card , outp);

  xprintf(xp, "  [%02x] fetot -- bits 0x0010  in outp  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x0010 ) >> 4) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_fecfd (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Add cfd to output data (time) */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register outp */

  offset=  ofs(struct lvd_qdc80_card , outp);

  xprintf(xp, "  [%02x] fecfd -- bits 0x0020  in outp  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x0020 ) >> 5) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_fezero (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Add zero crossing time to output data (time dx(1,2,4)) */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register outp */

  offset=  ofs(struct lvd_qdc80_card , outp);

  xprintf(xp, "  [%02x] fezero -- bits 0x0040  in outp  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x0040 ) >> 6) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_feqae (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Add amplitude at the end of pulse integral to output data */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register outp */

  offset=  ofs(struct lvd_qdc80_card , outp);

  xprintf(xp, "  [%02x] feqae -- bits 0x0080  in outp  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x0080 ) >> 7) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_feqpls (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Add pulse integral to output data (Integral 20 bit) */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register outp */

  offset=  ofs(struct lvd_qdc80_card , outp);

  xprintf(xp, "  [%02x] feqpls -- bits 0x0100  in outp  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x0100 ) >> 8) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_feqclab (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Add main Cluster integral,amplitude at begin to output data (Integral 20 bit) */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register outp */

  offset=  ofs(struct lvd_qdc80_card , outp);

  xprintf(xp, "  [%02x] feqclab -- bits 0x0200  in outp  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x0200 ) >> 9) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_feqclae (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Add main Cluster integral,amplitude at end to output data (Integral 20 bit) */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register outp */

  offset=  ofs(struct lvd_qdc80_card , outp);

  xprintf(xp, "  [%02x] feqclae -- bits 0x0400  in outp  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x0400 ) >> 10) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_feiwedge (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Add IW Amplitudes at start and end to output data (Integral 20 bit)  */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register outp */

  offset=  ofs(struct lvd_qdc80_card , outp);

  xprintf(xp, "  [%02x] feiwedge -- bits 0x0800  in outp  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x0800 ) >> 11) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_feiwmax (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Add IW Amplitudes at start and end to output data (Integral 20 bit)  */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register outp */

  offset=  ofs(struct lvd_qdc80_card , outp);

  xprintf(xp, "  [%02x] feiwmax -- bits 0x1000  in outp  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x1000 ) >> 12) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_feiwcenter (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Add IW time of centre  to output data */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register outp */

  offset=  ofs(struct lvd_qdc80_card , outp);

  xprintf(xp, "  [%02x] feiwcenter -- bits 0x2000  in outp  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x2000 ) >> 13) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_feovrun (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Add overruns since last readout to output data */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register outp */

  offset=  ofs(struct lvd_qdc80_card , outp);

  xprintf(xp, "  [%02x] feovrun -- bits 0x4000  in outp  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x4000 ) >> 14) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_fenoise (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Add maximum noise peak since last redout to output data */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register outp */

  offset=  ofs(struct lvd_qdc80_card , outp);

  xprintf(xp, "  [%02x] fenoise -- bits 0x8000  in outp  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x8000 ) >> 15) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_chctrl (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Set register chtrl */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register ch_ctrl */

  offset=  ofs(struct lvd_qdc80_card , ch_ctrl);

  xprintf(xp, "  [%02x] chctrl -- bits 0xffff  in ch_ctrl  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0xffff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_chaena (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Inhibit channel */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register ch_ctrl */

  offset=  ofs(struct lvd_qdc80_card , ch_ctrl);

  xprintf(xp, "  [%02x] chaena -- bits 0x0001  in ch_ctrl  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x0001 ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_plev (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Use level in pulse search algorithm */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register ch_ctrl */

  offset=  ofs(struct lvd_qdc80_card , ch_ctrl);

  xprintf(xp, "  [%02x] plev -- bits 0x0002  in ch_ctrl  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x0002 ) >> 1) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_polarity (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Select positive pulse */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register ch_ctrl */

  offset=  ofs(struct lvd_qdc80_card , ch_ctrl);

  xprintf(xp, "  [%02x] polarity -- bits 0x0004  in ch_ctrl  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x0004 ) >> 2) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_gradient (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Select gradient for zero crossing to 2 */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register ch_ctrl */

  offset=  ofs(struct lvd_qdc80_card , ch_ctrl);

  xprintf(xp, "  [%02x] gradient -- bits 0x0008  in ch_ctrl  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x0008 ) >> 3) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_noext (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Do not extend search window */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register ch_ctrl */

  offset=  ofs(struct lvd_qdc80_card , ch_ctrl);

  xprintf(xp, "  [%02x] noext -- bits 0x0010  in ch_ctrl  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x0010 ) >> 4) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_tot4raw (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Use tot_thr for raw selection */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register ch_ctrl */

  offset=  ofs(struct lvd_qdc80_card , ch_ctrl);

  xprintf(xp, "  [%02x] tot4raw -- bits 0x0020  in ch_ctrl  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x0020 ) >> 5) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_cfqrise (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Set register cf_qrise */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register cf_qrise */

  offset=  ofs(struct lvd_qdc80_card , cf_qrise);

  xprintf(xp, "  [%02x] cfqrise -- bits 0xffff  in cf_qrise  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.cf_qrise, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0xffff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_qrise (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Begin charge value for pulse search algorithm */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register cf_qrise */

  offset=  ofs(struct lvd_qdc80_card , cf_qrise);

  xprintf(xp, "  [%02x] qrise -- bits 0x00ff  in cf_qrise  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.cf_qrise, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x00ff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_cf (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* fraction for CF timing */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register cf_qrise */

  offset=  ofs(struct lvd_qdc80_card , cf_qrise);

  xprintf(xp, "  [%02x] cf -- bits 0x0f00  in cf_qrise  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.cf_qrise, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x0f00 ) >> 8) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_totlevel (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Set register tot_level */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register tot_level */

  offset=  ofs(struct lvd_qdc80_card , tot_level);

  xprintf(xp, "  [%02x] totlevel -- bits 0xffff  in tot_level  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.tot_level, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0xffff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_totthr (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* level for ToT and RAW threshold  */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register tot_level */

  offset=  ofs(struct lvd_qdc80_card , tot_level);

  xprintf(xp, "  [%02x] totthr -- bits 0x0fff  in tot_level  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.tot_level, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x0fff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_totwidth (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* minimal width for ToT and RAW level threshold  */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register tot_level */

  offset=  ofs(struct lvd_qdc80_card , tot_level);

  xprintf(xp, "  [%02x] totwidth -- bits 0xf000  in tot_level  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.tot_level, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0xf000 ) >> 12) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_logiclevel (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Set register logic_level */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register logic_level */

  offset=  ofs(struct lvd_qdc80_card , logic_level);

  xprintf(xp, "  [%02x] logiclevel -- bits 0xffff  in logic_level  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.logic_level, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0xffff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_logicthr (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* level for scaler and logic threshold  */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register logic_level */

  offset=  ofs(struct lvd_qdc80_card , logic_level);

  xprintf(xp, "  [%02x] logicthr -- bits 0x0fff  in logic_level  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.logic_level, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x0fff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_logicwidth (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* width for scaler and logic level threshold  */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register logic_level */

  offset=  ofs(struct lvd_qdc80_card , logic_level);

  xprintf(xp, "  [%02x] logicwidth -- bits 0xf000  in logic_level  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.logic_level, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0xf000 ) >> 12) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_iwqthr (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Q threshold for integral in integral window */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register iw_q_thr */

  offset=  ofs(struct lvd_qdc80_card , iw_q_thr);

  xprintf(xp, "  [%02x] iwqthr -- bits 0xffff  in iw_q_thr  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.iw_q_thr, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0xffff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_swqthr (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Q threshold for cluster  integral  */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register sw_q_thr */

  offset=  ofs(struct lvd_qdc80_card , sw_q_thr);

  xprintf(xp, "  [%02x] swqthr -- bits 0xffff  in sw_q_thr  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.sw_q_thr, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0xffff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_swilen (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Search window pulse integral length */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register sw_ilen */

  offset=  ofs(struct lvd_qdc80_card , sw_ilen);

  xprintf(xp, "  [%02x] swilen -- bits 0x03ff  in sw_ilen  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.sw_ilen, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x03ff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_clqstlen (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Cluster start integral length */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register cl_q_st_len */

  offset=  ofs(struct lvd_qdc80_card , cl_q_st_len);

  xprintf(xp, "  [%02x] clqstlen -- bits 0x01ff  in cl_q_st_len  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.cl_q_st_len, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x01ff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_cllen (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Cluster integral length */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register cl_len */

  offset=  ofs(struct lvd_qdc80_card , cl_len);

  xprintf(xp, "  [%02x] cllen -- bits 0x03ff  in cl_len  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.cl_len, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x03ff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_clqdel (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Delay for Cluster integral */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register cl_q_del */

  offset=  ofs(struct lvd_qdc80_card , cl_q_del);

  xprintf(xp, "  [%02x] clqdel -- bits 0x01ff  in cl_q_del  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.cl_q_del, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x01ff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_coincpar (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Set register coinc_par */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register coinc_par */

  offset=  ofs(struct lvd_qdc80_card , coinc_par);

  xprintf(xp, "  [%02x] coincpar -- bits 0xffff  in coinc_par  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.coinc_par, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0xffff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_coincdel (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Delay for coincidence */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register coinc_par */

  offset=  ofs(struct lvd_qdc80_card , coinc_par);

  xprintf(xp, "  [%02x] coincdel -- bits 0x001f  in coinc_par  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.coinc_par, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x001f ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_coincwidth (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Length for coincidence */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register coinc_par */

  offset=  ofs(struct lvd_qdc80_card , coinc_par);

  xprintf(xp, "  [%02x] coincwidth -- bits 0x3f00  in coinc_par  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.coinc_par, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x3f00 ) >> 8) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_dttau (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Tau value for base line algorithm */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register dttau */

  offset=  ofs(struct lvd_qdc80_card , dttau);

  xprintf(xp, "  [%02x] dttau -- bits 0x00ff  in dttau  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.dttau, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x00ff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_rawtable (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* lookup table for RAW request */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register raw_table */

  offset=  ofs(struct lvd_qdc80_card , raw_table);

  xprintf(xp, "  [%02x] rawtable -- bits 0xffff  in raw_table  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.raw_table, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0xffff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_swstart (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Search window latency */

  u_int32_t vals[4];
  ems_i32  group, res=0;
  int offset=0;
  int error[4]= {0};
  /* error
   * 0 - no error
   * 1 - group set error -- gxxg, xx-group number
   * 2 - value read error  -- vxxv, xx-group number
   * ? - not know case  -- ????
   */


  /* Determine offset of register sw_start */

  offset=  ofs(struct lvd_qdc80_card , sw_start);

  xprintf(xp, "  [%02x] swstart -- bits 0x0fff  in sw_start  offset = %02x \n", offset, offset);

  /* Read all 4 group seting values */


  for (group=0; group<4; group++)
    {
      /* select group */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, group<<2);

      if (res)
        {
          error[group]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.sw_start, &vals[group]);
      if (res)
        {
          error[group]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "         ");

  for (group=0; group<4; group++)
    {
      xprintf(xp, "%4d  ", group);
    }
  xprintf(xp, "\n");

  xprintf(xp, "         ");

  for (group=0; group<4; group++)
    switch (error[group])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[group]);
        break;
      case 1:
        xprintf(xp,  "g%02dg  ",group);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",group);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "         ");

  for (group=0; group<4; group++)
    switch (error[group])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[group] & 0x0fff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_swlen (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Search window length */

  u_int32_t vals[4];
  ems_i32  group, res=0;
  int offset=0;
  int error[4]= {0};
  /* error
   * 0 - no error
   * 1 - group set error -- gxxg, xx-group number
   * 2 - value read error  -- vxxv, xx-group number
   * ? - not know case  -- ????
   */


  /* Determine offset of register sw_len */

  offset=  ofs(struct lvd_qdc80_card , sw_len);

  xprintf(xp, "  [%02x] swlen -- bits 0x07ff  in sw_len  offset = %02x \n", offset, offset);

  /* Read all 4 group seting values */


  for (group=0; group<4; group++)
    {
      /* select group */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, group<<2);

      if (res)
        {
          error[group]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.sw_len, &vals[group]);
      if (res)
        {
          error[group]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "         ");

  for (group=0; group<4; group++)
    {
      xprintf(xp, "%4d  ", group);
    }
  xprintf(xp, "\n");

  xprintf(xp, "         ");

  for (group=0; group<4; group++)
    switch (error[group])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[group]);
        break;
      case 1:
        xprintf(xp,  "g%02dg  ",group);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",group);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "         ");

  for (group=0; group<4; group++)
    switch (error[group])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[group] & 0x07ff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_iwstart (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Integral window latency */

  u_int32_t vals[4];
  ems_i32  group, res=0;
  int offset=0;
  int error[4]= {0};
  /* error
   * 0 - no error
   * 1 - group set error -- gxxg, xx-group number
   * 2 - value read error  -- vxxv, xx-group number
   * ? - not know case  -- ????
   */


  /* Determine offset of register iw_start */

  offset=  ofs(struct lvd_qdc80_card , iw_start);

  xprintf(xp, "  [%02x] iwstart -- bits 0x0fff  in iw_start  offset = %02x \n", offset, offset);

  /* Read all 4 group seting values */


  for (group=0; group<4; group++)
    {
      /* select group */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, group<<2);

      if (res)
        {
          error[group]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.iw_start, &vals[group]);
      if (res)
        {
          error[group]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "         ");

  for (group=0; group<4; group++)
    {
      xprintf(xp, "%4d  ", group);
    }
  xprintf(xp, "\n");

  xprintf(xp, "         ");

  for (group=0; group<4; group++)
    switch (error[group])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[group]);
        break;
      case 1:
        xprintf(xp,  "g%02dg  ",group);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",group);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "         ");

  for (group=0; group<4; group++)
    switch (error[group])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[group] & 0x0fff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_iwlen (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Integral window length */

  u_int32_t vals[4];
  ems_i32  group, res=0;
  int offset=0;
  int error[4]= {0};
  /* error
   * 0 - no error
   * 1 - group set error -- gxxg, xx-group number
   * 2 - value read error  -- vxxv, xx-group number
   * ? - not know case  -- ????
   */


  /* Determine offset of register iw_len */

  offset=  ofs(struct lvd_qdc80_card , iw_len);

  xprintf(xp, "  [%02x] iwlen -- bits 0x01ff  in iw_len  offset = %02x \n", offset, offset);

  /* Read all 4 group seting values */


  for (group=0; group<4; group++)
    {
      /* select group */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, group<<2);

      if (res)
        {
          error[group]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.iw_len, &vals[group]);
      if (res)
        {
          error[group]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "         ");

  for (group=0; group<4; group++)
    {
      xprintf(xp, "%4d  ", group);
    }
  xprintf(xp, "\n");

  xprintf(xp, "         ");

  for (group=0; group<4; group++)
    switch (error[group])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[group]);
        break;
      case 1:
        xprintf(xp,  "g%02dg  ",group);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",group);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "         ");

  for (group=0; group<4; group++)
    switch (error[group])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[group] & 0x01ff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_coinctab (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Lookup table for inputs FPGA coincidences */

  u_int32_t vals[4];
  ems_i32  group, res=0;
  int offset=0;
  int error[4]= {0};
  /* error
   * 0 - no error
   * 1 - group set error -- gxxg, xx-group number
   * 2 - value read error  -- vxxv, xx-group number
   * ? - not know case  -- ????
   */


  /* Determine offset of register coinc_tab */

  offset=  ofs(struct lvd_qdc80_card , coinc_tab);

  xprintf(xp, "  [%02x] coinctab -- bits 0xffff  in coinc_tab  offset = %02x \n", offset, offset);

  /* Read all 4 group seting values */


  for (group=0; group<4; group++)
    {
      /* select group */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, group<<2);

      if (res)
        {
          error[group]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.coinc_tab, &vals[group]);
      if (res)
        {
          error[group]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "         ");

  for (group=0; group<4; group++)
    {
      xprintf(xp, "%4d  ", group);
    }
  xprintf(xp, "\n");

  xprintf(xp, "         ");

  for (group=0; group<4; group++)
    switch (error[group])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[group]);
        break;
      case 1:
        xprintf(xp,  "g%02dg  ",group);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",group);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "         ");

  for (group=0; group<4; group++)
    switch (error[group])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[group] & 0xffff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_cr (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Set cr register */

  u_int32_t val;
  ems_i32   res=0;
  int offset=0;
  int error= 0;
  /* error
   * 0 - no error
   * 2 - value read error  -- vvvv
   */


  /* Determine offset of register cr */

  offset=  ofs(struct lvd_qdc80_card , cr);

  xprintf(xp, "  [%02x] cr -- bits 0xffff  in cr  offset = %02x \n", offset, offset);

  /* Read  value */

  res=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &val);
  if (res)
    {
      error=2;
    }

  /* Output undecoded and decoded  (hex and dec) value */


  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", val);
      break;
    case 2:
      xprintf(xp,  "vvvv  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", ((val & 0xffff ) >> 0) );
      break;
    case 2:
      xprintf(xp,  "hhhh  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_ena (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Enable module */

  u_int32_t val;
  ems_i32   res=0;
  int offset=0;
  int error= 0;
  /* error
   * 0 - no error
   * 2 - value read error  -- vvvv
   */


  /* Determine offset of register cr */

  offset=  ofs(struct lvd_qdc80_card , cr);

  xprintf(xp, "  [%02x] ena -- bits 0x0001  in cr  offset = %02x \n", offset, offset);

  /* Read  value */

  res=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &val);
  if (res)
    {
      error=2;
    }

  /* Output undecoded and decoded  (hex and dec) value */


  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", val);
      break;
    case 2:
      xprintf(xp,  "vvvv  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", ((val & 0x0001 ) >> 0) );
      break;
    case 2:
      xprintf(xp,  "hhhh  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_trgmode (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Select trigger mode */

  u_int32_t val;
  ems_i32   res=0;
  int offset=0;
  int error= 0;
  /* error
   * 0 - no error
   * 2 - value read error  -- vvvv
   */


  /* Determine offset of register cr */

  offset=  ofs(struct lvd_qdc80_card , cr);

  xprintf(xp, "  [%02x] trgmode -- bits 0x0006  in cr  offset = %02x \n", offset, offset);

  /* Read  value */

  res=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &val);
  if (res)
    {
      error=2;
    }

  /* Output undecoded and decoded  (hex and dec) value */


  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", val);
      break;
    case 2:
      xprintf(xp,  "vvvv  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", ((val & 0x0006 ) >> 1) );
      break;
    case 2:
      xprintf(xp,  "hhhh  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_tdcena (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Enable TDC on external trigger input */

  u_int32_t val;
  ems_i32   res=0;
  int offset=0;
  int error= 0;
  /* error
   * 0 - no error
   * 2 - value read error  -- vvvv
   */


  /* Determine offset of register cr */

  offset=  ofs(struct lvd_qdc80_card , cr);

  xprintf(xp, "  [%02x] tdcena -- bits 0x0008  in cr  offset = %02x \n", offset, offset);

  /* Read  value */

  res=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &val);
  if (res)
    {
      error=2;
    }

  /* Output undecoded and decoded  (hex and dec) value */


  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", val);
      break;
    case 2:
      xprintf(xp,  "vvvv  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", ((val & 0x0008 ) >> 3) );
      break;
    case 2:
      xprintf(xp,  "hhhh  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_fixbaseline (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Fix baseline with last value */

  u_int32_t val;
  ems_i32   res=0;
  int offset=0;
  int error= 0;
  /* error
   * 0 - no error
   * 2 - value read error  -- vvvv
   */


  /* Determine offset of register cr */

  offset=  ofs(struct lvd_qdc80_card , cr);

  xprintf(xp, "  [%02x] fixbaseline -- bits 0x0010  in cr  offset = %02x \n", offset, offset);

  /* Read  value */

  res=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &val);
  if (res)
    {
      error=2;
    }

  /* Output undecoded and decoded  (hex and dec) value */


  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", val);
      break;
    case 2:
      xprintf(xp,  "vvvv  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", ((val & 0x0010 ) >> 4) );
      break;
    case 2:
      xprintf(xp,  "hhhh  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_tstsig (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Switch on test signal */

  u_int32_t val;
  ems_i32   res=0;
  int offset=0;
  int error= 0;
  /* error
   * 0 - no error
   * 2 - value read error  -- vvvv
   */


  /* Determine offset of register cr */

  offset=  ofs(struct lvd_qdc80_card , cr);

  xprintf(xp, "  [%02x] tstsig -- bits 0x0040  in cr  offset = %02x \n", offset, offset);

  /* Read  value */

  res=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &val);
  if (res)
    {
      error=2;
    }

  /* Output undecoded and decoded  (hex and dec) value */


  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", val);
      break;
    case 2:
      xprintf(xp,  "vvvv  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", ((val & 0x0040 ) >> 6) );
      break;
    case 2:
      xprintf(xp,  "hhhh  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_adcpwr (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Switch on ADC power */

  u_int32_t val;
  ems_i32   res=0;
  int offset=0;
  int error= 0;
  /* error
   * 0 - no error
   * 2 - value read error  -- vvvv
   */


  /* Determine offset of register cr */

  offset=  ofs(struct lvd_qdc80_card , cr);

  xprintf(xp, "  [%02x] adcpwr -- bits 0x0080  in cr  offset = %02x \n", offset, offset);

  /* Read  value */

  res=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &val);
  if (res)
    {
      error=2;
    }

  /* Output undecoded and decoded  (hex and dec) value */


  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", val);
      break;
    case 2:
      xprintf(xp,  "vvvv  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", ((val & 0x0080 ) >> 7) );
      break;
    case 2:
      xprintf(xp,  "hhhh  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_f1mode (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Switch to F1 mode */

  u_int32_t val;
  ems_i32   res=0;
  int offset=0;
  int error= 0;
  /* error
   * 0 - no error
   * 2 - value read error  -- vvvv
   */


  /* Determine offset of register cr */

  offset=  ofs(struct lvd_qdc80_card , cr);

  xprintf(xp, "  [%02x] f1mode -- bits 0x0100  in cr  offset = %02x \n", offset, offset);

  /* Read  value */

  res=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &val);
  if (res)
    {
      error=2;
    }

  /* Output undecoded and decoded  (hex and dec) value */


  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", val);
      break;
    case 2:
      xprintf(xp,  "vvvv  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", ((val & 0x0100 ) >> 8) );
      break;
    case 2:
      xprintf(xp,  "hhhh  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_grpcoinc (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Lookup table for main coincidences */

  u_int32_t val;
  ems_i32   res=0;
  int offset=0;
  int error= 0;
  /* error
   * 0 - no error
   * 2 - value read error  -- vvvv
   */


  /* Determine offset of register grp_coinc */

  offset=  ofs(struct lvd_qdc80_card , grp_coinc);

  xprintf(xp, "  [%02x] grpcoinc -- bits 0xffff  in grp_coinc  offset = %02x \n", offset, offset);

  /* Read  value */

  res=lvd_i_r(acard->dev, acard->addr, qdc80.grp_coinc, &val);
  if (res)
    {
      error=2;
    }

  /* Output undecoded and decoded  (hex and dec) value */


  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", val);
      break;
    case 2:
      xprintf(xp,  "vvvv  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", ((val & 0xffff ) >> 0) );
      break;
    case 2:
      xprintf(xp,  "hhhh  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_scalerrout (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Set scaler readout rate */

  u_int32_t val;
  ems_i32   res=0;
  int offset=0;
  int error= 0;
  /* error
   * 0 - no error
   * 2 - value read error  -- vvvv
   */


  /* Determine offset of register scaler_rout */

  offset=  ofs(struct lvd_qdc80_card , scaler_rout);

  xprintf(xp, "  [%02x] scalerrout -- bits 0x000f  in scaler_rout  offset = %02x \n", offset, offset);

  /* Read  value */

  res=lvd_i_r(acard->dev, acard->addr, qdc80.scaler_rout, &val);
  if (res)
    {
      error=2;
    }

  /* Output undecoded and decoded  (hex and dec) value */


  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", val);
      break;
    case 2:
      xprintf(xp,  "vvvv  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", ((val & 0x000f ) >> 0) );
      break;
    case 2:
      xprintf(xp,  "hhhh  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_coinmintraw (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Set coinmin_traw register */

  u_int32_t val;
  ems_i32   res=0;
  int offset=0;
  int error= 0;
  /* error
   * 0 - no error
   * 2 - value read error  -- vvvv
   */


  /* Determine offset of register coinmin_traw */

  offset=  ofs(struct lvd_qdc80_card , coinmin_traw);

  xprintf(xp, "  [%02x] coinmintraw -- bits 0xffff  in coinmin_traw  offset = %02x \n", offset, offset);

  /* Read  value */

  res=lvd_i_r(acard->dev, acard->addr, qdc80.coinmin_traw, &val);
  if (res)
    {
      error=2;
    }

  /* Output undecoded and decoded  (hex and dec) value */


  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", val);
      break;
    case 2:
      xprintf(xp,  "vvvv  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", ((val & 0xffff ) >> 0) );
      break;
    case 2:
      xprintf(xp,  "hhhh  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_traw (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Raw cycle frequency */

  u_int32_t val;
  ems_i32   res=0;
  int offset=0;
  int error= 0;
  /* error
   * 0 - no error
   * 2 - value read error  -- vvvv
   */


  /* Determine offset of register coinmin_traw */

  offset=  ofs(struct lvd_qdc80_card , coinmin_traw);

  xprintf(xp, "  [%02x] traw -- bits 0x000f  in coinmin_traw  offset = %02x \n", offset, offset);

  /* Read  value */

  res=lvd_i_r(acard->dev, acard->addr, qdc80.coinmin_traw, &val);
  if (res)
    {
      error=2;
    }

  /* Output undecoded and decoded  (hex and dec) value */


  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", val);
      break;
    case 2:
      xprintf(xp,  "vvvv  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", ((val & 0x000f ) >> 0) );
      break;
    case 2:
      xprintf(xp,  "hhhh  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_coinmin (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Minimum coincidence length */

  u_int32_t val;
  ems_i32   res=0;
  int offset=0;
  int error= 0;
  /* error
   * 0 - no error
   * 2 - value read error  -- vvvv
   */


  /* Determine offset of register coinmin_traw */

  offset=  ofs(struct lvd_qdc80_card , coinmin_traw);

  xprintf(xp, "  [%02x] coinmin -- bits 0x00f0  in coinmin_traw  offset = %02x \n", offset, offset);

  /* Read  value */

  res=lvd_i_r(acard->dev, acard->addr, qdc80.coinmin_traw, &val);
  if (res)
    {
      error=2;
    }

  /* Output undecoded and decoded  (hex and dec) value */


  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", val);
      break;
    case 2:
      xprintf(xp,  "vvvv  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", ((val & 0x00f0 ) >> 4) );
      break;
    case 2:
      xprintf(xp,  "hhhh  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_tpdac (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Test pulse level */

  u_int32_t val;
  ems_i32   res=0;
  int offset=0;
  int error= 0;
  /* error
   * 0 - no error
   * 2 - value read error  -- vvvv
   */


  /* Determine offset of register tp_dac */

  offset=  ofs(struct lvd_qdc80_card , tp_dac);

  xprintf(xp, "  [%02x] tpdac -- bits 0x0ff0  in tp_dac  offset = %02x \n", offset, offset);

  /* Read  value */

  res=lvd_i_r(acard->dev, acard->addr, qdc80.tp_dac, &val);
  if (res)
    {
      error=2;
    }

  /* Output undecoded and decoded  (hex and dec) value */


  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", val);
      break;
    case 2:
      xprintf(xp,  "vvvv  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", ((val & 0x0ff0 ) >> 4) );
      break;
    case 2:
      xprintf(xp,  "hhhh  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_blcorcycle (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Baseline correction cycle */

  u_int32_t val;
  ems_i32   res=0;
  int offset=0;
  int error= 0;
  /* error
   * 0 - no error
   * 2 - value read error  -- vvvv
   */


  /* Determine offset of register bl_cor_cycle */

  offset=  ofs(struct lvd_qdc80_card , bl_cor_cycle);

  xprintf(xp, "  [%02x] blcorcycle -- bits 0x00ff  in bl_cor_cycle  offset = %02x \n", offset, offset);

  /* Read  value */

  res=lvd_i_r(acard->dev, acard->addr, qdc80.bl_cor_cycle, &val);
  if (res)
    {
      error=2;
    }

  /* Output undecoded and decoded  (hex and dec) value */


  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", val);
      break;
    case 2:
      xprintf(xp,  "vvvv  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  xprintf(xp, "        ");

  switch (error)
    {
    case 0:
      xprintf(xp, "%04x  ", ((val & 0x00ff ) >> 0) );
      break;
    case 2:
      xprintf(xp,  "hhhh  ");
      break;
    default:
      xprintf(xp, "????  ");
    }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_baseline (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Read actual baseline */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register baseline */

  offset=  ofs(struct lvd_qdc80_card , baseline);

  xprintf(xp, "  [%02x] baseline -- bits 0xffff  in baseline  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.baseline, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0xffff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_tcornoise (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Read tcornoise register */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register tcor_noise */

  offset=  ofs(struct lvd_qdc80_card , tcor_noise);

  xprintf(xp, "  [%02x] tcornoise -- bits 0xffff  in tcor_noise  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.tcor_noise, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0xffff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
#if 0
static int
dump_qdc80_tcor (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Read max tau correction */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register tcor_noise */

  offset=  ofs(struct lvd_qdc80_card , tcor_noise);

  xprintf(xp, "  [%02x] tcor -- bits 0xff00  in tcor_noise  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.tcor_noise, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0xff00 ) >> 8) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}
#endif
/*****************************************************************************/
#if 0
static int
dump_qdc80_noise (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Read max tau correction */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register tcor_noise */

  offset=  ofs(struct lvd_qdc80_card , tcor_noise);

  xprintf(xp, "  [%02x] noise -- bits 0x00ff  in tcor_noise  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.tcor_noise, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x00ff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}
#endif
/*****************************************************************************/
static int
dump_qdc80_dttauquality (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Capacity correction assessment */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register dttau_quality */

  offset=  ofs(struct lvd_qdc80_card , dttau_quality);

  xprintf(xp, "  [%02x] dttauquality -- bits 0x003f  in dttau_quality  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.dttau_quality, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0x003f ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}

/*****************************************************************************/
static int
dump_qdc80_ovr (struct lvd_acard *acard, struct qdc_info *info, void *xp)
{
  /* Read and clear overruns bits */

  u_int32_t vals[16];
  ems_i32  channel, res=0;
  int offset=0;
  int error[16]= {0};
  /* error
   * 0 - no error
   * 1 - channel set error -- cxxc, xx-channel number
   * 2 - value read error  -- vxxv, xx-channel number
   * ? - not know case  -- ????
   */


  /* Determine offset of register ctrl_ovr */

  offset=  ofs(struct lvd_qdc80_card , ctrl_ovr);

  xprintf(xp, "  [%02x] ovr -- bits 0xffff  in ctrl_ovr  offset = %02x \n", offset, offset);

  /* Read all 16 channel seting values */


  for (channel=0; channel<16; channel++)
    {
      /* select channel */
      res=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);

      if (res)
        {
          error[channel]=1;
        }

      /* read data */
      res=lvd_i_r(acard->dev, acard->addr, qdc80.ctrl_ovr, &vals[channel]);
      if (res)
        {
          error[channel]=2;
        }
    }

  /* Output undecoded and decoded  (hex and dec) values */

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    {
      xprintf(xp, "%4d  ", channel);
    }
  xprintf(xp, "\n");

  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", vals[channel]);
        break;
      case 1:
        xprintf(xp,  "c%02dc  ",channel);
        break;
      case 2:
        xprintf(xp,  "v%02dv  ",channel);
        break;
      default:
        xprintf(xp, "????  ");
      }
  xprintf(xp, "\n");
  xprintf(xp, "       ");

  for (channel=0; channel<16; channel++)
    switch (error[channel])
      {
      case 0:
        xprintf(xp, "%04x  ", ((vals[channel] & 0xffff ) >> 0) );
        break;
      case 1:
        xprintf(xp,  "hhhh  ");
        break;
      case 2:
        xprintf(xp,  "hhhh  ");
        break;
      default:
        xprintf(xp,  "????  ");
      }
  xprintf(xp, "\n");
  return 0;
}
/*****************************************************************************/
int
lvd_cardstat_qdc80(struct lvd_dev* dev, struct lvd_acard* acard, void *xp,
    int level)
{
    struct qdc_info *info=(struct qdc_info*)acard->cardinfo;

    xprintf(xp, "ACQcard 0x%03x %sQDC:\n", 
            acard->addr,
            acard->mtype==ZEL_LVD_SQDC?"S":
            acard->mtype==ZEL_LVD_FQDC?"F":"VF");

    switch (level) {
    case 0:
    case 1:
        dump_ident(acard, info, xp);
        dump_serial(acard, info, xp);
        dump_cr(acard, info, xp);
        dump_cr_saved(acard, info, xp);
        dump_qdc80_cr(acard, info, xp);
        dump_qdc80_swstart(acard, info, xp);
        dump_qdc80_swlen(acard, info, xp);
        dump_qdc80_iwstart(acard, info, xp);
        dump_qdc80_iwlen(acard, info, xp);
        dump_qdc80_coinctab(acard, info, xp);
        dump_qdc80_chctrl(acard, info, xp);
        dump_qdc80_outp(acard, info, xp);
        dump_qdc80_cfqrise(acard, info, xp);
        dump_qdc80_totlevel(acard, info, xp);
        dump_qdc80_logiclevel(acard, info, xp);
        dump_qdc80_iwqthr(acard, info, xp);
        dump_qdc80_swqthr(acard, info, xp);
        dump_qdc80_swilen(acard, info, xp);
        dump_qdc80_clqstlen(acard, info, xp);
        dump_qdc80_clqdel(acard, info, xp);
        dump_qdc80_cllen(acard, info, xp);
        dump_qdc80_coincpar(acard, info, xp);
        dump_qdc80_dttau(acard, info, xp);
        dump_qdc80_rawtable(acard, info, xp);
        dump_qdc80_baseline(acard, info, xp);
        dump_qdc80_tcornoise(acard, info, xp);
        dump_qdc80_dttauquality(acard, info, xp);
        dump_qdc80_coinmintraw(acard, info, xp);
        dump_qdc80_grpcoinc(acard, info, xp);
        dump_qdc80_blcorcycle(acard, info, xp);
        dump_qdc80_ovr(acard, info, xp);
        break;
    case 2:
    default:
        dump_ident(acard, info, xp);
        dump_serial(acard, info, xp);
        dump_cr(acard, info, xp);
        dump_cr_saved(acard, info, xp);
        dump_qdc80_ena(acard, info, xp);
        dump_qdc80_trgmode(acard, info, xp);
        dump_qdc80_tdcena(acard, info, xp);
        dump_qdc80_fixbaseline(acard, info, xp);
        dump_qdc80_tstsig(acard, info, xp);
        dump_qdc80_adcpwr(acard, info, xp);
        dump_qdc80_f1mode(acard, info, xp);
        dump_qdc80_swstart(acard, info, xp);
        dump_qdc80_swlen(acard, info, xp);
        dump_qdc80_iwstart(acard, info, xp);
        dump_qdc80_iwlen(acard, info, xp);
        dump_qdc80_festart(acard, info, xp);
        dump_qdc80_femin(acard, info, xp);
        dump_qdc80_femax(acard, info, xp);
        dump_qdc80_feend(acard, info, xp);
        dump_qdc80_fetot(acard, info, xp);
        dump_qdc80_fecfd(acard, info, xp);
        dump_qdc80_fezero(acard, info, xp);
        dump_qdc80_feqae(acard, info, xp);
        dump_qdc80_feqpls(acard, info, xp);
        dump_qdc80_feqclab(acard, info, xp);
        dump_qdc80_feqclae(acard, info, xp);
        dump_qdc80_feiwedge(acard, info, xp);
        dump_qdc80_feiwmax(acard, info, xp);
        dump_qdc80_feiwcenter(acard, info, xp);
        dump_qdc80_feovrun(acard, info, xp);
        dump_qdc80_fenoise(acard, info, xp);
        dump_qdc80_qrise(acard, info, xp);
        dump_qdc80_cf(acard, info, xp);
        dump_qdc80_plev(acard, info, xp);
        dump_qdc80_polarity(acard, info, xp);
        dump_qdc80_gradient(acard, info, xp);
        dump_qdc80_chaena(acard, info, xp);
        dump_qdc80_noext(acard, info, xp);
        dump_qdc80_tot4raw(acard, info, xp);
        dump_qdc80_totthr(acard, info, xp);
        dump_qdc80_totwidth(acard, info, xp);
        dump_qdc80_logicthr(acard, info, xp);
        dump_qdc80_logicwidth(acard, info, xp);
        dump_qdc80_iwqthr(acard, info, xp);
        dump_qdc80_swqthr(acard, info, xp);
        dump_qdc80_swilen(acard, info, xp);
        dump_qdc80_clqstlen(acard, info, xp);
        dump_qdc80_clqdel(acard, info, xp);
        dump_qdc80_cllen(acard, info, xp);
        dump_qdc80_coincdel(acard, info, xp);
        dump_qdc80_coincwidth(acard, info, xp);
        dump_qdc80_rawtable(acard, info, xp);
        dump_qdc80_baseline(acard, info, xp);
        dump_qdc80_tcornoise(acard, info, xp);
#if 0
        dump_qdc80_tcor(acard, info, xp);
        dump_qdc80_noise(acard, info, xp);
#endif
        dump_qdc80_dttauquality(acard, info, xp);
        dump_qdc80_scalerrout(acard, info, xp);
        dump_qdc80_traw(acard, info, xp);
        dump_qdc80_coinmin(acard, info, xp);
        dump_qdc80_tpdac(acard, info, xp);
        dump_qdc80_grpcoinc(acard, info, xp);
        dump_qdc80_blcorcycle(acard, info, xp);
        dump_qdc80_ovr(acard, info, xp);
        break;
    }

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
