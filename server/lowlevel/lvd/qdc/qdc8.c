/*
 * lowlevel/lvd/qdc/qdc8.c
 * created 2012-Aug-30(?) PK
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: qdc8.c,v 1.3 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include "../../../commu/commu.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <modultypes.h>
#include <rcs_ids.h>
#include "../lvdbus.h"
#include "../lvd_access.h"
#include "../lvd_initfuncs.h"
#include "qdc8.h"
#include "qdc_private.h"

typedef plerrcode (*qdcfun)(struct lvd_acard*, ems_u32*, int idx);

struct get_coinc_data {
    ems_u32 *dest;
    int group;
};

/*****************************************************************************/
static plerrcode
access_qdcfun(__attribute__((unused)) struct lvd_dev* dev,
    struct lvd_acard *acard, ems_u32 *val,
    int idx,
    plerrcode (**funlist)(struct lvd_acard*, ems_u32*, int),
    int numfun, __attribute__((unused)) const char *txt)
{
    struct qdc_info *info=(struct qdc_info*)acard->cardinfo;

#ifdef CHECKFUN
    if (numfun<NUMFUN) {
        complain("qdcfun: not enough FUN for %s", txt);
        return plErr_Program;
    }
#endif

    if (numfun>info->qdcver && funlist[info->qdcver]) {
        return funlist[info->qdcver](acard, val, idx);
    } else {
        return plErr_BadModTyp;
    }
}
/*****************************************************************************/
static plerrcode
access_qdc(struct lvd_dev* dev, int addr, ems_u32 *val,
    plerrcode (**funlist)(struct lvd_acard*, ems_u32*, int),
    int numfun,
    ems_u32 *mtypes, const char *txt)
{
    struct lvd_acard *acard;
    int idx;

    idx=lvd_find_acard(dev, addr);
    acard=dev->acards+idx;
    if ((idx<0) ||
                (acard->mtype!=ZEL_LVD_SQDC &&
                acard->mtype!=ZEL_LVD_FQDC &&
                acard->mtype!=ZEL_LVD_VFQDC)) {
        complain("%s: no QDC card with address 0x%x known",
                txt?txt:"access_qdc",
                addr);
        return plErr_Program;
    }
    if (mtypes && mtypes[0]) {
        int ok=0;
        while (!ok && mtypes[0]) {
            if (acard->mtype==mtypes[0])
                ok=1;
            mtypes++;
        }
        if (!ok)
            return plErr_BadModTyp;
    }

    return access_qdcfun(dev, acard, val, 0, funlist, numfun, txt);
}
/*****************************************************************************/
static plerrcode
access_qdcs(struct lvd_dev* dev, int addr, ems_u32 *val,
    plerrcode (**funlist)(struct lvd_acard*, ems_u32*, int),
    int numfun,
    ems_u32 *mtypes, const char *txt)
{
    plerrcode pres=plOK;

    if (addr<0) {
        int idx;
        for (idx=0; idx<dev->num_acards; idx++) {
            struct lvd_acard *acard=dev->acards+idx;
            if ((acard->mtype==ZEL_LVD_SQDC ||
                    acard->mtype==ZEL_LVD_FQDC ||
                    acard->mtype==ZEL_LVD_VFQDC) &&
                    LVD_FWverH(acard->id)>=8) {
                int ok=!mtypes || !mtypes[0];
                while (!ok && mtypes[0]) {
                    if (acard->mtype==mtypes[0])
                        ok=1;
                    mtypes++;
                }
                if (ok) {
                    pres=access_qdcfun(dev, acard, val, acard->addr,
                            funlist, numfun, txt);
                    if (pres!=plOK)
                        return pres;
                }
            }
        }
    } else {
        pres=access_qdc(dev, addr, val, funlist, numfun, mtypes, txt);
    }
    return pres;
}
/*****************************************************************************/
static plerrcode
lvd_qdc80_setc_outp(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_outp set  0xffff in  outp */
  /* Set register outp */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 outp=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
  /* Change only selected bits */
  temp=((temp & (~0xffff)) | ((outp << 0) & 0xffff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.outp, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_outp(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 outp)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_outp
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=outp;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_outp");
}






struct get_outp_data
{
  ems_u32 *outp;
  int channel;
};

static plerrcode
lvd_qdc80_getc_outp(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_outp read  0xffff from  outp */
  /* Set register outp */

  int res=0;
  struct get_outp_data *data=(struct get_outp_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
          /* Extract value */
          data->outp[16*idx+channel]=(temp >> 0) & 0xffff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
      /* Extract value */
      data->outp[idx]=(temp >> 0) & 0xffff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_outp(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* outp)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_outp
                        };

  struct get_outp_data data;
  data.outp=outp;
  data.channel=channel;

  if (addr<0)
    memset(outp, 0, 16*16*sizeof(ems_u32));
  else
    memset(outp, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_outp");
}

static plerrcode
lvd_qdc80_setc_festart(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_festart set  0x0001 in  outp */
  /* Add search window to output data (time,baseline) */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 festart=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0001)) | ((festart << 0) & 0x0001));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.outp, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_festart(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 festart)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_festart
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=festart;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_festart");
}






struct get_festart_data
{
  ems_u32 *festart;
  int channel;
};

static plerrcode
lvd_qdc80_getc_festart(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_festart read  0x0001 from  outp */
  /* Add search window to output data (time,baseline) */

  int res=0;
  struct get_festart_data *data=(struct get_festart_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
          /* Extract value */
          data->festart[16*idx+channel]=(temp >> 0) & 0x0001;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
      /* Extract value */
      data->festart[idx]=(temp >> 0) & 0x0001;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_festart(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* festart)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_festart
                        };

  struct get_festart_data data;
  data.festart=festart;
  data.channel=channel;

  if (addr<0)
    memset(festart, 0, 16*16*sizeof(ems_u32));
  else
    memset(festart, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_festart");
}

static plerrcode
lvd_qdc80_setc_femin(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_femin set  0x0002 in  outp */
  /* Add minimum to output data (time,amplitude) */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 femin=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0002)) | ((femin << 1) & 0x0002));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.outp, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_femin(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 femin)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_femin
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=femin;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_femin");
}






struct get_femin_data
{
  ems_u32 *femin;
  int channel;
};

static plerrcode
lvd_qdc80_getc_femin(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_femin read  0x0002 from  outp */
  /* Add minimum to output data (time,amplitude) */

  int res=0;
  struct get_femin_data *data=(struct get_femin_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
          /* Extract value */
          data->femin[16*idx+channel]=(temp >> 1) & 0x0002;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
      /* Extract value */
      data->femin[idx]=(temp >> 1) & 0x0002;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_femin(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* femin)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_femin
                        };

  struct get_femin_data data;
  data.femin=femin;
  data.channel=channel;

  if (addr<0)
    memset(femin, 0, 16*16*sizeof(ems_u32));
  else
    memset(femin, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_femin");
}

static plerrcode
lvd_qdc80_setc_femax(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_femax set  0x0004 in  outp */
  /* Add maximum to output data (time,amplitude) */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 femax=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0004)) | ((femax << 2) & 0x0004));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.outp, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_femax(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 femax)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_femax
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=femax;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_femax");
}






struct get_femax_data
{
  ems_u32 *femax;
  int channel;
};

static plerrcode
lvd_qdc80_getc_femax(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_femax read  0x0004 from  outp */
  /* Add maximum to output data (time,amplitude) */

  int res=0;
  struct get_femax_data *data=(struct get_femax_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
          /* Extract value */
          data->femax[16*idx+channel]=(temp >> 2) & 0x0004;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
      /* Extract value */
      data->femax[idx]=(temp >> 2) & 0x0004;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_femax(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* femax)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_femax
                        };

  struct get_femax_data data;
  data.femax=femax;
  data.channel=channel;

  if (addr<0)
    memset(femax, 0, 16*16*sizeof(ems_u32));
  else
    memset(femax, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_femax");
}

static plerrcode
lvd_qdc80_setc_feend(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_feend set  0x0008 in  outp */
  /* Add pulse end  to output data (time,amplitude) */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 feend=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0008)) | ((feend << 3) & 0x0008));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.outp, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_feend(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 feend)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_feend
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=feend;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_feend");
}






struct get_feend_data
{
  ems_u32 *feend;
  int channel;
};

static plerrcode
lvd_qdc80_getc_feend(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_feend read  0x0008 from  outp */
  /* Add pulse end  to output data (time,amplitude) */

  int res=0;
  struct get_feend_data *data=(struct get_feend_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
          /* Extract value */
          data->feend[16*idx+channel]=(temp >> 3) & 0x0008;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
      /* Extract value */
      data->feend[idx]=(temp >> 3) & 0x0008;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_feend(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* feend)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_feend
                        };

  struct get_feend_data data;
  data.feend=feend;
  data.channel=channel;

  if (addr<0)
    memset(feend, 0, 16*16*sizeof(ems_u32));
  else
    memset(feend, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_feend");
}

static plerrcode
lvd_qdc80_setc_fetot(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_fetot set  0x0010 in  outp */
  /* Add tot to output data (timeup)(timedown) */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 fetot=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0010)) | ((fetot << 4) & 0x0010));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.outp, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_fetot(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 fetot)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_fetot
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=fetot;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_fetot");
}






struct get_fetot_data
{
  ems_u32 *fetot;
  int channel;
};

static plerrcode
lvd_qdc80_getc_fetot(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_fetot read  0x0010 from  outp */
  /* Add tot to output data (timeup)(timedown) */

  int res=0;
  struct get_fetot_data *data=(struct get_fetot_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
          /* Extract value */
          data->fetot[16*idx+channel]=(temp >> 4) & 0x0010;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
      /* Extract value */
      data->fetot[idx]=(temp >> 4) & 0x0010;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_fetot(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* fetot)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_fetot
                        };

  struct get_fetot_data data;
  data.fetot=fetot;
  data.channel=channel;

  if (addr<0)
    memset(fetot, 0, 16*16*sizeof(ems_u32));
  else
    memset(fetot, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_fetot");
}

static plerrcode
lvd_qdc80_setc_fecfd(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_fecfd set  0x0020 in  outp */
  /* Add cfd to output data (time) */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 fecfd=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0020)) | ((fecfd << 5) & 0x0020));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.outp, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_fecfd(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 fecfd)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_fecfd
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=fecfd;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_fecfd");
}






struct get_fecfd_data
{
  ems_u32 *fecfd;
  int channel;
};

static plerrcode
lvd_qdc80_getc_fecfd(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_fecfd read  0x0020 from  outp */
  /* Add cfd to output data (time) */

  int res=0;
  struct get_fecfd_data *data=(struct get_fecfd_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
          /* Extract value */
          data->fecfd[16*idx+channel]=(temp >> 5) & 0x0020;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
      /* Extract value */
      data->fecfd[idx]=(temp >> 5) & 0x0020;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_fecfd(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* fecfd)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_fecfd
                        };

  struct get_fecfd_data data;
  data.fecfd=fecfd;
  data.channel=channel;

  if (addr<0)
    memset(fecfd, 0, 16*16*sizeof(ems_u32));
  else
    memset(fecfd, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_fecfd");
}

static plerrcode
lvd_qdc80_setc_fezero(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_fezero set  0x0040 in  outp */
  /* Add zero crossing time to output data (time dx(1,2,4)) */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 fezero=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0040)) | ((fezero << 6) & 0x0040));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.outp, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_fezero(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 fezero)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_fezero
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=fezero;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_fezero");
}






struct get_fezero_data
{
  ems_u32 *fezero;
  int channel;
};

static plerrcode
lvd_qdc80_getc_fezero(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_fezero read  0x0040 from  outp */
  /* Add zero crossing time to output data (time dx(1,2,4)) */

  int res=0;
  struct get_fezero_data *data=(struct get_fezero_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
          /* Extract value */
          data->fezero[16*idx+channel]=(temp >> 6) & 0x0040;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
      /* Extract value */
      data->fezero[idx]=(temp >> 6) & 0x0040;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_fezero(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* fezero)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_fezero
                        };

  struct get_fezero_data data;
  data.fezero=fezero;
  data.channel=channel;

  if (addr<0)
    memset(fezero, 0, 16*16*sizeof(ems_u32));
  else
    memset(fezero, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_fezero");
}

static plerrcode
lvd_qdc80_setc_feqae(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_feqae set  0x0080 in  outp */
  /* Add amplitude at the end of pulse integral to output data */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 feqae=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0080)) | ((feqae << 7) & 0x0080));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.outp, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_feqae(struct lvd_dev* dev, int addr, ems_i32 channel,
    ems_u32 feqae)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_feqae
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=feqae;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_feqae");
}






struct get_feqae_data
{
  ems_u32 *feqae;
  int channel;
};

static plerrcode
lvd_qdc80_getc_feqae(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_feqae read  0x0080 from  outp */
  /* Add amplitude at the end of pulse integral to output data */

  int res=0;
  struct get_feqae_data *data=(struct get_feqae_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
          /* Extract value */
          data->feqae[16*idx+channel]=(temp >> 7) & 0x0080;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
      /* Extract value */
      data->feqae[idx]=(temp >> 7) & 0x0080;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_feqae(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* feqae)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_feqae
                        };

  struct get_feqae_data data;
  data.feqae=feqae;
  data.channel=channel;

  if (addr<0)
    memset(feqae, 0, 16*16*sizeof(ems_u32));
  else
    memset(feqae, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_feqae");
}

static plerrcode
lvd_qdc80_setc_feqpls(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_feqpls set  0x0100 in  outp */
  /* Add pulse integral to output data (Integral 20 bit) */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 feqpls=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0100)) | ((feqpls << 8) & 0x0100));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.outp, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_feqpls(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 feqpls)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_feqpls
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=feqpls;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_feqpls");
}






struct get_feqpls_data
{
  ems_u32 *feqpls;
  int channel;
};

static plerrcode
lvd_qdc80_getc_feqpls(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_feqpls read  0x0100 from  outp */
  /* Add pulse integral to output data (Integral 20 bit) */

  int res=0;
  struct get_feqpls_data *data=(struct get_feqpls_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
          /* Extract value */
          data->feqpls[16*idx+channel]=(temp >> 8) & 0x0100;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
      /* Extract value */
      data->feqpls[idx]=(temp >> 8) & 0x0100;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_feqpls(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* feqpls)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_feqpls
                        };

  struct get_feqpls_data data;
  data.feqpls=feqpls;
  data.channel=channel;

  if (addr<0)
    memset(feqpls, 0, 16*16*sizeof(ems_u32));
  else
    memset(feqpls, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_feqpls");
}

static plerrcode
lvd_qdc80_setc_feqclab(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_feqclab set  0x0200 in  outp */
  /* Add main Cluster integral,amplitude at begin to output data
     (Integral 20 bit) */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 feqclab=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0200)) | ((feqclab << 9) & 0x0200));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.outp, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_feqclab(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 feqclab)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_feqclab
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=feqclab;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_feqclab");
}






struct get_feqclab_data
{
  ems_u32 *feqclab;
  int channel;
};

static plerrcode
lvd_qdc80_getc_feqclab(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_feqclab read  0x0200 from  outp */
  /* Add main Cluster integral,amplitude at begin to output data
     (Integral 20 bit) */

  int res=0;
  struct get_feqclab_data *data=(struct get_feqclab_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
          /* Extract value */
          data->feqclab[16*idx+channel]=(temp >> 9) & 0x0200;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
      /* Extract value */
      data->feqclab[idx]=(temp >> 9) & 0x0200;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_feqclab(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* feqclab)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_feqclab
                        };

  struct get_feqclab_data data;
  data.feqclab=feqclab;
  data.channel=channel;

  if (addr<0)
    memset(feqclab, 0, 16*16*sizeof(ems_u32));
  else
    memset(feqclab, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_feqclab");
}

static plerrcode
lvd_qdc80_setc_feqclae(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_feqclae set  0x0400 in  outp */
  /* Add main Cluster integral,amplitude at end to output data
     (Integral 20 bit) */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 feqclae=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0400)) | ((feqclae << 10) & 0x0400));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.outp, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_feqclae(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 feqclae)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_feqclae
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=feqclae;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_feqclae");
}






struct get_feqclae_data
{
  ems_u32 *feqclae;
  int channel;
};

static plerrcode
lvd_qdc80_getc_feqclae(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_feqclae read  0x0400 from  outp */
  /* Add main Cluster integral,amplitude at end to output data
     (Integral 20 bit) */

  int res=0;
  struct get_feqclae_data *data=(struct get_feqclae_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
          /* Extract value */
          data->feqclae[16*idx+channel]=(temp >> 10) & 0x0400;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
      /* Extract value */
      data->feqclae[idx]=(temp >> 10) & 0x0400;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_feqclae(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* feqclae)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_feqclae
                        };

  struct get_feqclae_data data;
  data.feqclae=feqclae;
  data.channel=channel;

  if (addr<0)
    memset(feqclae, 0, 16*16*sizeof(ems_u32));
  else
    memset(feqclae, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_feqclae");
}

static plerrcode
lvd_qdc80_setc_feiwedge(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_feiwedge set  0x0800 in  outp */
  /* Add IW Amplitudes at start and end to output data (Integral 20 bit)  */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 feiwedge=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0800)) | ((feiwedge << 11) & 0x0800));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.outp, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_feiwedge(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 feiwedge)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_feiwedge
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=feiwedge;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_feiwedge");
}






struct get_feiwedge_data
{
  ems_u32 *feiwedge;
  int channel;
};

static plerrcode
lvd_qdc80_getc_feiwedge(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_feiwedge read  0x0800 from  outp */
  /* Add IW Amplitudes at start and end to output data (Integral 20 bit)  */

  int res=0;
  struct get_feiwedge_data *data=(struct get_feiwedge_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
          /* Extract value */
          data->feiwedge[16*idx+channel]=(temp >> 11) & 0x0800;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
      /* Extract value */
      data->feiwedge[idx]=(temp >> 11) & 0x0800;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_feiwedge(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* feiwedge)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_feiwedge
                        };

  struct get_feiwedge_data data;
  data.feiwedge=feiwedge;
  data.channel=channel;

  if (addr<0)
    memset(feiwedge, 0, 16*16*sizeof(ems_u32));
  else
    memset(feiwedge, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_feiwedge");
}

static plerrcode
lvd_qdc80_setc_feiwmax(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_feiwmax set  0x1000 in  outp */
  /* Add IW Amplitudes at start and end to output data (Integral 20 bit)  */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 feiwmax=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x1000)) | ((feiwmax << 12) & 0x1000));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.outp, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_feiwmax(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 feiwmax)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_feiwmax
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=feiwmax;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_feiwmax");
}






struct get_feiwmax_data
{
  ems_u32 *feiwmax;
  int channel;
};

static plerrcode
lvd_qdc80_getc_feiwmax(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_feiwmax read  0x1000 from  outp */
  /* Add IW Amplitudes at start and end to output data (Integral 20 bit)  */

  int res=0;
  struct get_feiwmax_data *data=(struct get_feiwmax_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
          /* Extract value */
          data->feiwmax[16*idx+channel]=(temp >> 12) & 0x1000;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
      /* Extract value */
      data->feiwmax[idx]=(temp >> 12) & 0x1000;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_feiwmax(struct lvd_dev* dev, int addr, ems_i32 channel,
    ems_u32* feiwmax)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_feiwmax
                        };

  struct get_feiwmax_data data;
  data.feiwmax=feiwmax;
  data.channel=channel;

  if (addr<0)
    memset(feiwmax, 0, 16*16*sizeof(ems_u32));
  else
    memset(feiwmax, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_feiwmax");
}

static plerrcode
lvd_qdc80_setc_feiwcenter(struct lvd_acard* acard, ems_u32 *vals,
    __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_feiwcenter set  0x2000 in  outp */
  /* Add IW time of centre  to output data */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 feiwcenter=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x2000)) | ((feiwcenter << 13) & 0x2000));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.outp, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_feiwcenter(struct lvd_dev* dev, int addr, ems_i32 channel,
    ems_u32 feiwcenter)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_feiwcenter
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=feiwcenter;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_feiwcenter");
}






struct get_feiwcenter_data
{
  ems_u32 *feiwcenter;
  int channel;
};

static plerrcode
lvd_qdc80_getc_feiwcenter(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_feiwcenter read  0x2000 from  outp */
  /* Add IW time of centre  to output data */

  int res=0;
  struct get_feiwcenter_data *data=(struct get_feiwcenter_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
          /* Extract value */
          data->feiwcenter[16*idx+channel]=(temp >> 13) & 0x2000;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
      /* Extract value */
      data->feiwcenter[idx]=(temp >> 13) & 0x2000;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_feiwcenter(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* feiwcenter)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_feiwcenter
                        };

  struct get_feiwcenter_data data;
  data.feiwcenter=feiwcenter;
  data.channel=channel;

  if (addr<0)
    memset(feiwcenter, 0, 16*16*sizeof(ems_u32));
  else
    memset(feiwcenter, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_feiwcenter");
}

static plerrcode
lvd_qdc80_setc_feovrun(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_feovrun set  0x4000 in  outp */
  /* Add overruns since last readout to output data */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 feovrun=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x4000)) | ((feovrun << 14) & 0x4000));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.outp, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_feovrun(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 feovrun)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_feovrun
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=feovrun;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_feovrun");
}






struct get_feovrun_data
{
  ems_u32 *feovrun;
  int channel;
};

static plerrcode
lvd_qdc80_getc_feovrun(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_feovrun read  0x4000 from  outp */
  /* Add overruns since last readout to output data */

  int res=0;
  struct get_feovrun_data *data=(struct get_feovrun_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
          /* Extract value */
          data->feovrun[16*idx+channel]=(temp >> 14) & 0x4000;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
      /* Extract value */
      data->feovrun[idx]=(temp >> 14) & 0x4000;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_feovrun(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* feovrun)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_feovrun
                        };

  struct get_feovrun_data data;
  data.feovrun=feovrun;
  data.channel=channel;

  if (addr<0)
    memset(feovrun, 0, 16*16*sizeof(ems_u32));
  else
    memset(feovrun, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_feovrun");
}

static plerrcode
lvd_qdc80_setc_fenoise(struct lvd_acard* acard, ems_u32 *vals,
    __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_fenoise set  0x8000 in  outp */
  /* Add maximum noise peak since last redout to output data */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 fenoise=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x8000)) | ((fenoise << 15) & 0x8000));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.outp, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_fenoise(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 fenoise)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_fenoise
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=fenoise;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_fenoise");
}






struct get_fenoise_data
{
  ems_u32 *fenoise;
  int channel;
};

static plerrcode
lvd_qdc80_getc_fenoise(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_fenoise read  0x8000 from  outp */
  /* Add maximum noise peak since last redout to output data */

  int res=0;
  struct get_fenoise_data *data=(struct get_fenoise_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
          /* Extract value */
          data->fenoise[16*idx+channel]=(temp >> 15) & 0x8000;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.outp, &temp);
      /* Extract value */
      data->fenoise[idx]=(temp >> 15) & 0x8000;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_fenoise(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* fenoise)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_fenoise
                        };

  struct get_fenoise_data data;
  data.fenoise=fenoise;
  data.channel=channel;

  if (addr<0)
    memset(fenoise, 0, 16*16*sizeof(ems_u32));
  else
    memset(fenoise, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_fenoise");
}

static plerrcode
lvd_qdc80_setc_chctrl(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_chctrl set  0xffff in  ch_ctrl */
  /* Set register chtrl */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 chctrl=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &temp);
  /* Change only selected bits */
  temp=((temp & (~0xffff)) | ((chctrl << 0) & 0xffff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.ch_ctrl, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_chctrl(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 chctrl)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_chctrl
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=chctrl;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_chctrl");
}






struct get_chctrl_data
{
  ems_u32 *chctrl;
  int channel;
};

static plerrcode
lvd_qdc80_getc_chctrl(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_chctrl read  0xffff from  ch_ctrl */
  /* Set register chtrl */

  int res=0;
  struct get_chctrl_data *data=(struct get_chctrl_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &temp);
          /* Extract value */
          data->chctrl[16*idx+channel]=(temp >> 0) & 0xffff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &temp);
      /* Extract value */
      data->chctrl[idx]=(temp >> 0) & 0xffff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_chctrl(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* chctrl)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_chctrl
                        };

  struct get_chctrl_data data;
  data.chctrl=chctrl;
  data.channel=channel;

  if (addr<0)
    memset(chctrl, 0, 16*16*sizeof(ems_u32));
  else
    memset(chctrl, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_chctrl");
}

static plerrcode
lvd_qdc80_setc_chaena(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_chaena set  0x0001 in  ch_ctrl */
  /* Inhibit channel */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 chaena=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0001)) | ((chaena << 0) & 0x0001));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.ch_ctrl, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_chaena(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 chaena)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_chaena
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=chaena;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_chaena");
}






struct get_chaena_data
{
  ems_u32 *chaena;
  int channel;
};

static plerrcode
lvd_qdc80_getc_chaena(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_chaena read  0x0001 from  ch_ctrl */
  /* Inhibit channel */

  int res=0;
  struct get_chaena_data *data=(struct get_chaena_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &temp);
          /* Extract value */
          data->chaena[16*idx+channel]=(temp >> 0) & 0x0001;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &temp);
      /* Extract value */
      data->chaena[idx]=(temp >> 0) & 0x0001;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_chaena(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* chaena)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_chaena
                        };

  struct get_chaena_data data;
  data.chaena=chaena;
  data.channel=channel;

  if (addr<0)
    memset(chaena, 0, 16*16*sizeof(ems_u32));
  else
    memset(chaena, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_chaena");
}

static plerrcode
lvd_qdc80_setc_plev(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_plev set  0x0002 in  ch_ctrl */
  /* Use level in pulse search algorithm */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 plev=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0002)) | ((plev << 1) & 0x0002));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.ch_ctrl, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_plev(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 plev)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_plev
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=plev;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_plev");
}






struct get_plev_data
{
  ems_u32 *plev;
  int channel;
};

static plerrcode
lvd_qdc80_getc_plev(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_plev read  0x0002 from  ch_ctrl */
  /* Use level in pulse search algorithm */

  int res=0;
  struct get_plev_data *data=(struct get_plev_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &temp);
          /* Extract value */
          data->plev[16*idx+channel]=(temp >> 1) & 0x0002;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &temp);
      /* Extract value */
      data->plev[idx]=(temp >> 1) & 0x0002;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_plev(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* plev)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_plev
                        };

  struct get_plev_data data;
  data.plev=plev;
  data.channel=channel;

  if (addr<0)
    memset(plev, 0, 16*16*sizeof(ems_u32));
  else
    memset(plev, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_plev");
}

static plerrcode
lvd_qdc80_setc_polarity(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_polarity set  0x0004 in  ch_ctrl */
  /* Select positive pulse */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 polarity=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0004)) | ((polarity << 2) & 0x0004));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.ch_ctrl, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_polarity(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 polarity)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_polarity
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=polarity;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_polarity");
}






struct get_polarity_data
{
  ems_u32 *polarity;
  int channel;
};

static plerrcode
lvd_qdc80_getc_polarity(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_polarity read  0x0004 from  ch_ctrl */
  /* Select positive pulse */

  int res=0;
  struct get_polarity_data *data=(struct get_polarity_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &temp);
          /* Extract value */
          data->polarity[16*idx+channel]=(temp >> 2) & 0x0004;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &temp);
      /* Extract value */
      data->polarity[idx]=(temp >> 2) & 0x0004;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_polarity(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* polarity)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_polarity
                        };

  struct get_polarity_data data;
  data.polarity=polarity;
  data.channel=channel;

  if (addr<0)
    memset(polarity, 0, 16*16*sizeof(ems_u32));
  else
    memset(polarity, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_polarity");
}

static plerrcode
lvd_qdc80_setc_gradient(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_gradient set  0x0008 in  ch_ctrl */
  /* Select gradient for zero crossing to 2 */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 gradient=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0008)) | ((gradient << 3) & 0x0008));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.ch_ctrl, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_gradient(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 gradient)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_gradient
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=gradient;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_gradient");
}






struct get_gradient_data
{
  ems_u32 *gradient;
  int channel;
};

static plerrcode
lvd_qdc80_getc_gradient(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_gradient read  0x0008 from  ch_ctrl */
  /* Select gradient for zero crossing to 2 */

  int res=0;
  struct get_gradient_data *data=(struct get_gradient_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &temp);
          /* Extract value */
          data->gradient[16*idx+channel]=(temp >> 3) & 0x0008;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &temp);
      /* Extract value */
      data->gradient[idx]=(temp >> 3) & 0x0008;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_gradient(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* gradient)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_gradient
                        };

  struct get_gradient_data data;
  data.gradient=gradient;
  data.channel=channel;

  if (addr<0)
    memset(gradient, 0, 16*16*sizeof(ems_u32));
  else
    memset(gradient, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_gradient");
}

static plerrcode
lvd_qdc80_setc_noext(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_noext set  0x0010 in  ch_ctrl */
  /* Do not extend search window */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 noext=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0010)) | ((noext << 4) & 0x0010));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.ch_ctrl, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_noext(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 noext)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_noext
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=noext;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_noext");
}






struct get_noext_data
{
  ems_u32 *noext;
  int channel;
};

static plerrcode
lvd_qdc80_getc_noext(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_noext read  0x0010 from  ch_ctrl */
  /* Do not extend search window */

  int res=0;
  struct get_noext_data *data=(struct get_noext_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &temp);
          /* Extract value */
          data->noext[16*idx+channel]=(temp >> 4) & 0x0010;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &temp);
      /* Extract value */
      data->noext[idx]=(temp >> 4) & 0x0010;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_noext(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* noext)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_noext
                        };

  struct get_noext_data data;
  data.noext=noext;
  data.channel=channel;

  if (addr<0)
    memset(noext, 0, 16*16*sizeof(ems_u32));
  else
    memset(noext, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_noext");
}

static plerrcode
lvd_qdc80_setc_tot4raw(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_tot4raw set  0x0020 in  ch_ctrl */
  /* Use tot_thr for raw selection */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 tot4raw=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0020)) | ((tot4raw << 5) & 0x0020));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.ch_ctrl, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_tot4raw(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 tot4raw)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_tot4raw
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=tot4raw;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_tot4raw");
}






struct get_tot4raw_data
{
  ems_u32 *tot4raw;
  int channel;
};

static plerrcode
lvd_qdc80_getc_tot4raw(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_tot4raw read  0x0020 from  ch_ctrl */
  /* Use tot_thr for raw selection */

  int res=0;
  struct get_tot4raw_data *data=(struct get_tot4raw_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &temp);
          /* Extract value */
          data->tot4raw[16*idx+channel]=(temp >> 5) & 0x0020;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.ch_ctrl, &temp);
      /* Extract value */
      data->tot4raw[idx]=(temp >> 5) & 0x0020;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_tot4raw(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* tot4raw)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_tot4raw
                        };

  struct get_tot4raw_data data;
  data.tot4raw=tot4raw;
  data.channel=channel;

  if (addr<0)
    memset(tot4raw, 0, 16*16*sizeof(ems_u32));
  else
    memset(tot4raw, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_tot4raw");
}

static plerrcode
lvd_qdc80_setc_cfqrise(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_cfqrise set  0xffff in  cf_qrise */
  /* Set register cf_qrise */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 cfqrise=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cf_qrise, &temp);
  /* Change only selected bits */
  temp=((temp & (~0xffff)) | ((cfqrise << 0) & 0xffff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.cf_qrise, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_cfqrise(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 cfqrise)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_cfqrise
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=cfqrise;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_cfqrise");
}






struct get_cfqrise_data
{
  ems_u32 *cfqrise;
  int channel;
};

static plerrcode
lvd_qdc80_getc_cfqrise(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_cfqrise read  0xffff from  cf_qrise */
  /* Set register cf_qrise */

  int res=0;
  struct get_cfqrise_data *data=(struct get_cfqrise_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.cf_qrise, &temp);
          /* Extract value */
          data->cfqrise[16*idx+channel]=(temp >> 0) & 0xffff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.cf_qrise, &temp);
      /* Extract value */
      data->cfqrise[idx]=(temp >> 0) & 0xffff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_cfqrise(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* cfqrise)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_cfqrise
                        };

  struct get_cfqrise_data data;
  data.cfqrise=cfqrise;
  data.channel=channel;

  if (addr<0)
    memset(cfqrise, 0, 16*16*sizeof(ems_u32));
  else
    memset(cfqrise, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_cfqrise");
}

static plerrcode
lvd_qdc80_setc_qrise(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_qrise set  0x00ff in  cf_qrise */
  /* Begin charge value for pulse search algorithm */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 qrise=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cf_qrise, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x00ff)) | ((qrise << 0) & 0x00ff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.cf_qrise, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_qrise(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 qrise)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_qrise
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=qrise;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_qrise");
}






struct get_qrise_data
{
  ems_u32 *qrise;
  int channel;
};

static plerrcode
lvd_qdc80_getc_qrise(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_qrise read  0x00ff from  cf_qrise */
  /* Begin charge value for pulse search algorithm */

  int res=0;
  struct get_qrise_data *data=(struct get_qrise_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.cf_qrise, &temp);
          /* Extract value */
          data->qrise[16*idx+channel]=(temp >> 0) & 0x00ff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.cf_qrise, &temp);
      /* Extract value */
      data->qrise[idx]=(temp >> 0) & 0x00ff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_qrise(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* qrise)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_qrise
                        };

  struct get_qrise_data data;
  data.qrise=qrise;
  data.channel=channel;

  if (addr<0)
    memset(qrise, 0, 16*16*sizeof(ems_u32));
  else
    memset(qrise, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_qrise");
}

static plerrcode
lvd_qdc80_setc_cf(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_cf set  0x0f00 in  cf_qrise */
  /* fraction for CF timing */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 cf=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cf_qrise, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0f00)) | ((cf << 8) & 0x0f00));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.cf_qrise, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_cf(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 cf)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_cf
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=cf;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_cf");
}






struct get_cf_data
{
  ems_u32 *cf;
  int channel;
};

static plerrcode
lvd_qdc80_getc_cf(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_cf read  0x0f00 from  cf_qrise */
  /* fraction for CF timing */

  int res=0;
  struct get_cf_data *data=(struct get_cf_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.cf_qrise, &temp);
          /* Extract value */
          data->cf[16*idx+channel]=(temp >> 8) & 0x0f00;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.cf_qrise, &temp);
      /* Extract value */
      data->cf[idx]=(temp >> 8) & 0x0f00;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_cf(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* cf)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_cf
                        };

  struct get_cf_data data;
  data.cf=cf;
  data.channel=channel;

  if (addr<0)
    memset(cf, 0, 16*16*sizeof(ems_u32));
  else
    memset(cf, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_cf");
}

static plerrcode
lvd_qdc80_setc_totlevel(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_totlevel set  0xffff in  tot_level */
  /* Set register tot_level */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 totlevel=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.tot_level, &temp);
  /* Change only selected bits */
  temp=((temp & (~0xffff)) | ((totlevel << 0) & 0xffff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.tot_level, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_totlevel(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 totlevel)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_totlevel
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=totlevel;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_totlevel");
}






struct get_totlevel_data
{
  ems_u32 *totlevel;
  int channel;
};

static plerrcode
lvd_qdc80_getc_totlevel(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_totlevel read  0xffff from  tot_level */
  /* Set register tot_level */

  int res=0;
  struct get_totlevel_data *data=(struct get_totlevel_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.tot_level, &temp);
          /* Extract value */
          data->totlevel[16*idx+channel]=(temp >> 0) & 0xffff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.tot_level, &temp);
      /* Extract value */
      data->totlevel[idx]=(temp >> 0) & 0xffff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_totlevel(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* totlevel)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_totlevel
                        };

  struct get_totlevel_data data;
  data.totlevel=totlevel;
  data.channel=channel;

  if (addr<0)
    memset(totlevel, 0, 16*16*sizeof(ems_u32));
  else
    memset(totlevel, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_totlevel");
}

static plerrcode
lvd_qdc80_setc_totthr(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_totthr set  0x0fff in  tot_level */
  /* level for ToT and RAW threshold  */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 totthr=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.tot_level, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0fff)) | ((totthr << 0) & 0x0fff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.tot_level, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_totthr(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 totthr)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_totthr
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=totthr;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_totthr");
}






struct get_totthr_data
{
  ems_u32 *totthr;
  int channel;
};

static plerrcode
lvd_qdc80_getc_totthr(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_totthr read  0x0fff from  tot_level */
  /* level for ToT and RAW threshold  */

  int res=0;
  struct get_totthr_data *data=(struct get_totthr_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.tot_level, &temp);
          /* Extract value */
          data->totthr[16*idx+channel]=(temp >> 0) & 0x0fff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.tot_level, &temp);
      /* Extract value */
      data->totthr[idx]=(temp >> 0) & 0x0fff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_totthr(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* totthr)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_totthr
                        };

  struct get_totthr_data data;
  data.totthr=totthr;
  data.channel=channel;

  if (addr<0)
    memset(totthr, 0, 16*16*sizeof(ems_u32));
  else
    memset(totthr, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_totthr");
}

static plerrcode
lvd_qdc80_setc_totwidth(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_totwidth set  0xf000 in  tot_level */
  /* minimal width for ToT and RAW level threshold  */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 totwidth=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.tot_level, &temp);
  /* Change only selected bits */
  temp=((temp & (~0xf000)) | ((totwidth << 12) & 0xf000));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.tot_level, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_totwidth(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 totwidth)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_totwidth
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=totwidth;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_totwidth");
}






struct get_totwidth_data
{
  ems_u32 *totwidth;
  int channel;
};

static plerrcode
lvd_qdc80_getc_totwidth(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_totwidth read  0xf000 from  tot_level */
  /* minimal width for ToT and RAW level threshold  */

  int res=0;
  struct get_totwidth_data *data=(struct get_totwidth_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.tot_level, &temp);
          /* Extract value */
          data->totwidth[16*idx+channel]=(temp >> 12) & 0xf000;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.tot_level, &temp);
      /* Extract value */
      data->totwidth[idx]=(temp >> 12) & 0xf000;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_totwidth(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* totwidth)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_totwidth
                        };

  struct get_totwidth_data data;
  data.totwidth=totwidth;
  data.channel=channel;

  if (addr<0)
    memset(totwidth, 0, 16*16*sizeof(ems_u32));
  else
    memset(totwidth, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_totwidth");
}

static plerrcode
lvd_qdc80_setc_logiclevel(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_logiclevel set  0xffff in  logic_level */
  /* Set register logic_level */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 logiclevel=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.logic_level, &temp);
  /* Change only selected bits */
  temp=((temp & (~0xffff)) | ((logiclevel << 0) & 0xffff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.logic_level, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_logiclevel(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 logiclevel)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_logiclevel
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=logiclevel;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_logiclevel");
}






struct get_logiclevel_data
{
  ems_u32 *logiclevel;
  int channel;
};

static plerrcode
lvd_qdc80_getc_logiclevel(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_logiclevel read  0xffff from  logic_level */
  /* Set register logic_level */

  int res=0;
  struct get_logiclevel_data *data=(struct get_logiclevel_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.logic_level, &temp);
          /* Extract value */
          data->logiclevel[16*idx+channel]=(temp >> 0) & 0xffff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.logic_level, &temp);
      /* Extract value */
      data->logiclevel[idx]=(temp >> 0) & 0xffff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_logiclevel(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* logiclevel)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_logiclevel
                        };

  struct get_logiclevel_data data;
  data.logiclevel=logiclevel;
  data.channel=channel;

  if (addr<0)
    memset(logiclevel, 0, 16*16*sizeof(ems_u32));
  else
    memset(logiclevel, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_logiclevel");
}

static plerrcode
lvd_qdc80_setc_logicthr(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_logicthr set  0x0fff in  logic_level */
  /* level for scaler and logic threshold  */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 logicthr=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.logic_level, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0fff)) | ((logicthr << 0) & 0x0fff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.logic_level, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_logicthr(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 logicthr)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_logicthr
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=logicthr;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_logicthr");
}






struct get_logicthr_data
{
  ems_u32 *logicthr;
  int channel;
};

static plerrcode
lvd_qdc80_getc_logicthr(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_logicthr read  0x0fff from  logic_level */
  /* level for scaler and logic threshold  */

  int res=0;
  struct get_logicthr_data *data=(struct get_logicthr_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.logic_level, &temp);
          /* Extract value */
          data->logicthr[16*idx+channel]=(temp >> 0) & 0x0fff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.logic_level, &temp);
      /* Extract value */
      data->logicthr[idx]=(temp >> 0) & 0x0fff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_logicthr(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* logicthr)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_logicthr
                        };

  struct get_logicthr_data data;
  data.logicthr=logicthr;
  data.channel=channel;

  if (addr<0)
    memset(logicthr, 0, 16*16*sizeof(ems_u32));
  else
    memset(logicthr, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_logicthr");
}

static plerrcode
lvd_qdc80_setc_logicwidth(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_logicwidth set  0xf000 in  logic_level */
  /* width for scaler and logic level threshold  */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 logicwidth=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.logic_level, &temp);
  /* Change only selected bits */
  temp=((temp & (~0xf000)) | ((logicwidth << 12) & 0xf000));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.logic_level, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_logicwidth(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 logicwidth)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_logicwidth
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=logicwidth;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_logicwidth");
}






struct get_logicwidth_data
{
  ems_u32 *logicwidth;
  int channel;
};

static plerrcode
lvd_qdc80_getc_logicwidth(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_logicwidth read  0xf000 from  logic_level */
  /* width for scaler and logic level threshold  */

  int res=0;
  struct get_logicwidth_data *data=(struct get_logicwidth_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.logic_level, &temp);
          /* Extract value */
          data->logicwidth[16*idx+channel]=(temp >> 12) & 0xf000;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.logic_level, &temp);
      /* Extract value */
      data->logicwidth[idx]=(temp >> 12) & 0xf000;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_logicwidth(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* logicwidth)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_logicwidth
                        };

  struct get_logicwidth_data data;
  data.logicwidth=logicwidth;
  data.channel=channel;

  if (addr<0)
    memset(logicwidth, 0, 16*16*sizeof(ems_u32));
  else
    memset(logicwidth, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_logicwidth");
}

static plerrcode
lvd_qdc80_setc_iwqthr(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_iwqthr set  0xffff in  iw_q_thr */
  /* Q threshold for integral in integral window */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 iwqthr=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.iw_q_thr, &temp);
  /* Change only selected bits */
  temp=((temp & (~0xffff)) | ((iwqthr << 0) & 0xffff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.iw_q_thr, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_iwqthr(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 iwqthr)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_iwqthr
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=iwqthr;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_iwqthr");
}






struct get_iwqthr_data
{
  ems_u32 *iwqthr;
  int channel;
};

static plerrcode
lvd_qdc80_getc_iwqthr(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_iwqthr read  0xffff from  iw_q_thr */
  /* Q threshold for integral in integral window */

  int res=0;
  struct get_iwqthr_data *data=(struct get_iwqthr_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.iw_q_thr, &temp);
          /* Extract value */
          data->iwqthr[16*idx+channel]=(temp >> 0) & 0xffff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.iw_q_thr, &temp);
      /* Extract value */
      data->iwqthr[idx]=(temp >> 0) & 0xffff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_iwqthr(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* iwqthr)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_iwqthr
                        };

  struct get_iwqthr_data data;
  data.iwqthr=iwqthr;
  data.channel=channel;

  if (addr<0)
    memset(iwqthr, 0, 16*16*sizeof(ems_u32));
  else
    memset(iwqthr, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_iwqthr");
}

static plerrcode
lvd_qdc80_setc_swqthr(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_swqthr set  0xffff in  sw_q_thr */
  /* Q threshold for cluster  integral  */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 swqthr=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.sw_q_thr, &temp);
  /* Change only selected bits */
  temp=((temp & (~0xffff)) | ((swqthr << 0) & 0xffff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.sw_q_thr, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_swqthr(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 swqthr)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_swqthr
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=swqthr;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_swqthr");
}






struct get_swqthr_data
{
  ems_u32 *swqthr;
  int channel;
};

static plerrcode
lvd_qdc80_getc_swqthr(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_swqthr read  0xffff from  sw_q_thr */
  /* Q threshold for cluster  integral  */

  int res=0;
  struct get_swqthr_data *data=(struct get_swqthr_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.sw_q_thr, &temp);
          /* Extract value */
          data->swqthr[16*idx+channel]=(temp >> 0) & 0xffff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.sw_q_thr, &temp);
      /* Extract value */
      data->swqthr[idx]=(temp >> 0) & 0xffff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_swqthr(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* swqthr)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_swqthr
                        };

  struct get_swqthr_data data;
  data.swqthr=swqthr;
  data.channel=channel;

  if (addr<0)
    memset(swqthr, 0, 16*16*sizeof(ems_u32));
  else
    memset(swqthr, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_swqthr");
}

static plerrcode
lvd_qdc80_setc_swilen(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_swilen set  0x03ff in  sw_ilen */
  /* Search window pulse integral length */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 swilen=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.sw_ilen, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x03ff)) | ((swilen << 0) & 0x03ff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.sw_ilen, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_swilen(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 swilen)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_swilen
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=swilen;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_swilen");
}






struct get_swilen_data
{
  ems_u32 *swilen;
  int channel;
};

static plerrcode
lvd_qdc80_getc_swilen(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_swilen read  0x03ff from  sw_ilen */
  /* Search window pulse integral length */

  int res=0;
  struct get_swilen_data *data=(struct get_swilen_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.sw_ilen, &temp);
          /* Extract value */
          data->swilen[16*idx+channel]=(temp >> 0) & 0x03ff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.sw_ilen, &temp);
      /* Extract value */
      data->swilen[idx]=(temp >> 0) & 0x03ff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_swilen(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* swilen)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_swilen
                        };

  struct get_swilen_data data;
  data.swilen=swilen;
  data.channel=channel;

  if (addr<0)
    memset(swilen, 0, 16*16*sizeof(ems_u32));
  else
    memset(swilen, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_swilen");
}

static plerrcode
lvd_qdc80_setc_clqstlen(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_clqstlen set  0x01ff in  cl_q_st_len */
  /* Cluster start integral length */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 clqstlen=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cl_q_st_len, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x01ff)) | ((clqstlen << 0) & 0x01ff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.cl_q_st_len, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_clqstlen(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 clqstlen)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_clqstlen
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=clqstlen;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_clqstlen");
}






struct get_clqstlen_data
{
  ems_u32 *clqstlen;
  int channel;
};

static plerrcode
lvd_qdc80_getc_clqstlen(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_clqstlen read  0x01ff from  cl_q_st_len */
  /* Cluster start integral length */

  int res=0;
  struct get_clqstlen_data *data=(struct get_clqstlen_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.cl_q_st_len, &temp);
          /* Extract value */
          data->clqstlen[16*idx+channel]=(temp >> 0) & 0x01ff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.cl_q_st_len, &temp);
      /* Extract value */
      data->clqstlen[idx]=(temp >> 0) & 0x01ff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_clqstlen(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* clqstlen)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_clqstlen
                        };

  struct get_clqstlen_data data;
  data.clqstlen=clqstlen;
  data.channel=channel;

  if (addr<0)
    memset(clqstlen, 0, 16*16*sizeof(ems_u32));
  else
    memset(clqstlen, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_clqstlen");
}

static plerrcode
lvd_qdc80_setc_cllen(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_cllen set  0x03ff in  cl_len */
  /* Cluster integral length */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 cllen=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cl_len, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x03ff)) | ((cllen << 0) & 0x03ff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.cl_len, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_cllen(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 cllen)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_cllen
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=cllen;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_cllen");
}






struct get_cllen_data
{
  ems_u32 *cllen;
  int channel;
};

static plerrcode
lvd_qdc80_getc_cllen(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_cllen read  0x03ff from  cl_len */
  /* Cluster integral length */

  int res=0;
  struct get_cllen_data *data=(struct get_cllen_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.cl_len, &temp);
          /* Extract value */
          data->cllen[16*idx+channel]=(temp >> 0) & 0x03ff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.cl_len, &temp);
      /* Extract value */
      data->cllen[idx]=(temp >> 0) & 0x03ff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_cllen(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* cllen)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_cllen
                        };

  struct get_cllen_data data;
  data.cllen=cllen;
  data.channel=channel;

  if (addr<0)
    memset(cllen, 0, 16*16*sizeof(ems_u32));
  else
    memset(cllen, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_cllen");
}

static plerrcode
lvd_qdc80_setc_clqdel(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_clqdel set  0x01ff in  cl_q_del */
  /* Delay for Cluster integral */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 clqdel=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cl_q_del, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x01ff)) | ((clqdel << 0) & 0x01ff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.cl_q_del, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_clqdel(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 clqdel)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_clqdel
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=clqdel;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_clqdel");
}






struct get_clqdel_data
{
  ems_u32 *clqdel;
  int channel;
};

static plerrcode
lvd_qdc80_getc_clqdel(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_clqdel read  0x01ff from  cl_q_del */
  /* Delay for Cluster integral */

  int res=0;
  struct get_clqdel_data *data=(struct get_clqdel_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.cl_q_del, &temp);
          /* Extract value */
          data->clqdel[16*idx+channel]=(temp >> 0) & 0x01ff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.cl_q_del, &temp);
      /* Extract value */
      data->clqdel[idx]=(temp >> 0) & 0x01ff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_clqdel(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* clqdel)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_clqdel
                        };

  struct get_clqdel_data data;
  data.clqdel=clqdel;
  data.channel=channel;

  if (addr<0)
    memset(clqdel, 0, 16*16*sizeof(ems_u32));
  else
    memset(clqdel, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_clqdel");
}

static plerrcode
lvd_qdc80_setc_coincpar(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_coincpar set  0xffff in  coinc_par */
  /* Set register coinc_par */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 coincpar=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.coinc_par, &temp);
  /* Change only selected bits */
  temp=((temp & (~0xffff)) | ((coincpar << 0) & 0xffff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.coinc_par, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_coincpar(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 coincpar)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_coincpar
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=coincpar;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_coincpar");
}






struct get_coincpar_data
{
  ems_u32 *coincpar;
  int channel;
};

static plerrcode
lvd_qdc80_getc_coincpar(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_coincpar read  0xffff from  coinc_par */
  /* Set register coinc_par */

  int res=0;
  struct get_coincpar_data *data=(struct get_coincpar_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.coinc_par, &temp);
          /* Extract value */
          data->coincpar[16*idx+channel]=(temp >> 0) & 0xffff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.coinc_par, &temp);
      /* Extract value */
      data->coincpar[idx]=(temp >> 0) & 0xffff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_coincpar(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* coincpar)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_coincpar
                        };

  struct get_coincpar_data data;
  data.coincpar=coincpar;
  data.channel=channel;

  if (addr<0)
    memset(coincpar, 0, 16*16*sizeof(ems_u32));
  else
    memset(coincpar, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_coincpar");
}

static plerrcode
lvd_qdc80_setc_coincdel(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_coincdel set  0x001f in  coinc_par */
  /* Delay for coincidence */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 coincdel=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.coinc_par, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x001f)) | ((coincdel << 0) & 0x001f));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.coinc_par, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_coincdel(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 coincdel)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_coincdel
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=coincdel;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_coincdel");
}






struct get_coincdel_data
{
  ems_u32 *coincdel;
  int channel;
};

static plerrcode
lvd_qdc80_getc_coincdel(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_coincdel read  0x001f from  coinc_par */
  /* Delay for coincidence */

  int res=0;
  struct get_coincdel_data *data=(struct get_coincdel_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.coinc_par, &temp);
          /* Extract value */
          data->coincdel[16*idx+channel]=(temp >> 0) & 0x001f;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.coinc_par, &temp);
      /* Extract value */
      data->coincdel[idx]=(temp >> 0) & 0x001f;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_coincdel(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* coincdel)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_coincdel
                        };

  struct get_coincdel_data data;
  data.coincdel=coincdel;
  data.channel=channel;

  if (addr<0)
    memset(coincdel, 0, 16*16*sizeof(ems_u32));
  else
    memset(coincdel, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_coincdel");
}

static plerrcode
lvd_qdc80_setc_coincwidth(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_coincwidth set  0x3f00 in  coinc_par */
  /* Length for coincidence */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 coincwidth=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.coinc_par, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x3f00)) | ((coincwidth << 8) & 0x3f00));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.coinc_par, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_coincwidth(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 coincwidth)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_coincwidth
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=coincwidth;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_coincwidth");
}






struct get_coincwidth_data
{
  ems_u32 *coincwidth;
  int channel;
};

static plerrcode
lvd_qdc80_getc_coincwidth(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_coincwidth read  0x3f00 from  coinc_par */
  /* Length for coincidence */

  int res=0;
  struct get_coincwidth_data *data=(struct get_coincwidth_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.coinc_par, &temp);
          /* Extract value */
          data->coincwidth[16*idx+channel]=(temp >> 8) & 0x3f00;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.coinc_par, &temp);
      /* Extract value */
      data->coincwidth[idx]=(temp >> 8) & 0x3f00;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_coincwidth(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* coincwidth)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_coincwidth
                        };

  struct get_coincwidth_data data;
  data.coincwidth=coincwidth;
  data.channel=channel;

  if (addr<0)
    memset(coincwidth, 0, 16*16*sizeof(ems_u32));
  else
    memset(coincwidth, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_coincwidth");
}

static plerrcode
lvd_qdc80_setc_dttau(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_dttau set  0x00ff in  dttau */
  /* Tau value for base line algorithm */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 dttau=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.dttau, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x00ff)) | ((dttau << 0) & 0x00ff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.dttau, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_dttau(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 dttau)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_dttau
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=dttau;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_dttau");
}






struct get_dttau_data
{
  ems_u32 *dttau;
  int channel;
};

static plerrcode
lvd_qdc80_getc_dttau(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_dttau read  0x00ff from  dttau */
  /* Tau value for base line algorithm */

  int res=0;
  struct get_dttau_data *data=(struct get_dttau_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.dttau, &temp);
          /* Extract value */
          data->dttau[16*idx+channel]=(temp >> 0) & 0x00ff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.dttau, &temp);
      /* Extract value */
      data->dttau[idx]=(temp >> 0) & 0x00ff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_dttau(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32* dttau)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_dttau
                        };

  struct get_dttau_data data;
  data.dttau=dttau;
  data.channel=channel;

  if (addr<0)
    memset(dttau, 0, 16*16*sizeof(ems_u32));
  else
    memset(dttau, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_dttau");
}

static plerrcode
lvd_qdc80_setc_rawtable(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /*lvd_qdc80_setc_rawtable set  0xffff in  raw_table */
  /* lookup table for RAW request */

  int res=0;
  ems_i32 channel=vals[0];
  ems_u32 rawtable=vals[1];
  ems_u32 temp;

  if (channel<0)  channel=0x10;

  /* Select channel */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.raw_table, &temp);
  /* Change only selected bits */
  temp=((temp & (~0xffff)) | ((rawtable << 0) & 0xffff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.raw_table, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setc_rawtable(struct lvd_dev* dev, int addr, ems_i32 channel,
        ems_u32 rawtable)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setc_rawtable
                        };
  ems_u32 vals[2];
  vals[0]=channel;
  vals[1]=rawtable;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setc_rawtable");
}






struct get_rawtable_data
{
  ems_u32 *rawtable;
  int channel;
};

static plerrcode
lvd_qdc80_getc_rawtable(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getc_rawtable read  0xffff from  raw_table */
  /* lookup table for RAW request */

  int res=0;
  struct get_rawtable_data *data=(struct get_rawtable_data*)data_;
  u_int32_t temp;

  if (data->channel<0)
    {
      int channel;
      for (channel=0; channel<16; channel++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, channel);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.raw_table, &temp);
          /* Extract value */
          data->rawtable[16*idx+channel]=(temp >> 0) & 0xffff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->channel);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.raw_table, &temp);
      /* Extract value */
      data->rawtable[idx]=(temp >> 0) & 0xffff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getc_rawtable(struct lvd_dev* dev, int addr, ems_i32 channel,
    ems_u32* rawtable)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getc_rawtable
                        };

  struct get_rawtable_data data;
  data.rawtable=rawtable;
  data.channel=channel;

  if (addr<0)
    memset(rawtable, 0, 16*16*sizeof(ems_u32));
  else
    memset(rawtable, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, (ems_u32*) & data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getc_rawtable");
}

static plerrcode
lvd_qdc80_setg_swstart(struct lvd_acard* acard, ems_u32 *vals,
    __attribute__((unused)) int xxx)
{

  /* lvd_qdc80_setg_swstart set  0x0fff in  sw_start */
  /* Search window latency */

  int res=0;
  ems_i32 group=vals[0];
  ems_u32 swstart=vals[1];
  ems_u32 temp;

  if (group<0)
    group=0x10;
  else
    group=(group &0x3) << 2;

  /* Select group */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, group);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.sw_start, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0fff)) | ((swstart << 0) & 0x0fff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.sw_start, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setg_swstart(struct lvd_dev* dev, int addr, ems_i32 group,
    ems_u32 swstart)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setg_swstart
                        };
  ems_u32 vals[2];
  vals[0]=group;
  vals[1]=swstart;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setg_swstart");
}






struct get_swstart_data
{
  ems_u32 *swstart;
  int group;
};

static plerrcode
lvd_qdc80_getg_swstart(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getg_swstart read bit(s) 0x0fff from register sw_start for
     selected FPGA(s)(group(s)) */
  /* Search window latency */

  int res=0;
  struct get_swstart_data *data=(struct get_swstart_data*)data_;
  u_int32_t temp;

  if (data->group<0)
    {
      int group;
      for (group=0; group<4; group++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, group);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.sw_start, &temp);
          /* Extract value */
          data->swstart[4*idx+group]=(temp >> 0 ) & 0x0fff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->group);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.sw_start, &temp);
      /* Extract value */
      data->swstart[idx]=(temp >> 0) & 0x0fff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getg_swstart(struct lvd_dev* dev, int addr, ems_i32 group,
        ems_u32* swstart)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getg_swstart
                        };

  struct get_coinc_data data;
  data.dest=swstart;
  data.group=group;

  if (addr<0)
    memset(swstart, 0, 4*16*sizeof(ems_u32));
  else
    memset(swstart, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr,(ems_u32*) &data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getg_swstart");
}

static plerrcode
lvd_qdc80_setg_swlen(struct lvd_acard* acard, ems_u32 *vals,
    __attribute__((unused)) int xxx)
{

  /* lvd_qdc80_setg_swlen set  0x07ff in  sw_len */
  /* Search window length */

  int res=0;
  ems_i32 group=vals[0];
  ems_u32 swlen=vals[1];
  ems_u32 temp;

  if (group<0)
    group=0x10;
  else
    group=(group &0x3) << 2;

  /* Select group */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, group);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.sw_len, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x07ff)) | ((swlen << 0) & 0x07ff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.sw_len, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setg_swlen(struct lvd_dev* dev, int addr, ems_i32 group, ems_u32 swlen)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setg_swlen
                        };
  ems_u32 vals[2];
  vals[0]=group;
  vals[1]=swlen;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setg_swlen");
}






struct get_swlen_data
{
  ems_u32 *swlen;
  int group;
};

static plerrcode
lvd_qdc80_getg_swlen(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getg_swlen read bit(s) 0x07ff from register sw_len for selected
     FPGA(s)(group(s)) */
  /* Search window length */

  int res=0;
  struct get_swlen_data *data=(struct get_swlen_data*)data_;
  u_int32_t temp;

  if (data->group<0)
    {
      int group;
      for (group=0; group<4; group++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, group);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.sw_len, &temp);
          /* Extract value */
          data->swlen[4*idx+group]=(temp >> 0 ) & 0x07ff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->group);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.sw_len, &temp);
      /* Extract value */
      data->swlen[idx]=(temp >> 0) & 0x07ff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getg_swlen(struct lvd_dev* dev, int addr, ems_i32 group, ems_u32* swlen)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getg_swlen
                        };

  struct get_coinc_data data;
  data.dest=swlen;
  data.group=group;

  if (addr<0)
    memset(swlen, 0, 4*16*sizeof(ems_u32));
  else
    memset(swlen, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr,(ems_u32*) &data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getg_swlen");
}

static plerrcode
lvd_qdc80_setg_iwstart(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /* lvd_qdc80_setg_iwstart set  0x0fff in  iw_start */
  /* Integral window latency */

  int res=0;
  ems_i32 group=vals[0];
  ems_u32 iwstart=vals[1];
  ems_u32 temp;

  if (group<0)
    group=0x10;
  else
    group=(group &0x3) << 2;

  /* Select group */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, group);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.iw_start, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0fff)) | ((iwstart << 0) & 0x0fff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.iw_start, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setg_iwstart(struct lvd_dev* dev, int addr, ems_i32 group,
        ems_u32 iwstart)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setg_iwstart
                        };
  ems_u32 vals[2];
  vals[0]=group;
  vals[1]=iwstart;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setg_iwstart");
}






struct get_iwstart_data
{
  ems_u32 *iwstart;
  int group;
};

static plerrcode
lvd_qdc80_getg_iwstart(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getg_iwstart read bit(s) 0x0fff from register iw_start for
     selected FPGA(s)(group(s)) */
  /* Integral window latency */

  int res=0;
  struct get_iwstart_data *data=(struct get_iwstart_data*)data_;
  u_int32_t temp;

  if (data->group<0)
    {
      int group;
      for (group=0; group<4; group++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, group);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.iw_start, &temp);
          /* Extract value */
          data->iwstart[4*idx+group]=(temp >> 0 ) & 0x0fff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->group);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.iw_start, &temp);
      /* Extract value */
      data->iwstart[idx]=(temp >> 0) & 0x0fff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getg_iwstart(struct lvd_dev* dev, int addr, ems_i32 group,
        ems_u32* iwstart)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getg_iwstart
                        };

  struct get_coinc_data data;
  data.dest=iwstart;
  data.group=group;

  if (addr<0)
    memset(iwstart, 0, 4*16*sizeof(ems_u32));
  else
    memset(iwstart, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr,(ems_u32*) &data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getg_iwstart");
}

static plerrcode
lvd_qdc80_setg_iwlen(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /* lvd_qdc80_setg_iwlen set  0x01ff in  iw_len */
  /* Integral window length */

  int res=0;
  ems_i32 group=vals[0];
  ems_u32 iwlen=vals[1];
  ems_u32 temp;

  if (group<0)
    group=0x10;
  else
    group=(group &0x3) << 2;

  /* Select group */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, group);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.iw_len, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x01ff)) | ((iwlen << 0) & 0x01ff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.iw_len, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setg_iwlen(struct lvd_dev* dev, int addr, ems_i32 group, ems_u32 iwlen)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setg_iwlen
                        };
  ems_u32 vals[2];
  vals[0]=group;
  vals[1]=iwlen;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setg_iwlen");
}






struct get_iwlen_data
{
  ems_u32 *iwlen;
  int group;
};

static plerrcode
lvd_qdc80_getg_iwlen(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getg_iwlen read bit(s) 0x01ff from register iw_len for selected
     FPGA(s)(group(s)) */
  /* Integral window length */

  int res=0;
  struct get_iwlen_data *data=(struct get_iwlen_data*)data_;
  u_int32_t temp;

  if (data->group<0)
    {
      int group;
      for (group=0; group<4; group++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, group);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.iw_len, &temp);
          /* Extract value */
          data->iwlen[4*idx+group]=(temp >> 0 ) & 0x01ff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->group);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.iw_len, &temp);
      /* Extract value */
      data->iwlen[idx]=(temp >> 0) & 0x01ff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getg_iwlen(struct lvd_dev* dev, int addr, ems_i32 group, ems_u32* iwlen)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getg_iwlen
                        };

  struct get_coinc_data data;
  data.dest=iwlen;
  data.group=group;

  if (addr<0)
    memset(iwlen, 0, 4*16*sizeof(ems_u32));
  else
    memset(iwlen, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr,(ems_u32*) &data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getg_iwlen");
}

static plerrcode
lvd_qdc80_setg_coinctab(struct lvd_acard* acard, ems_u32 *vals,
        __attribute__((unused)) int xxx)
{

  /* lvd_qdc80_setg_coinctab set  0xffff in  coinc_tab */
  /* Lookup table for inputs FPGA coincidences */

  int res=0;
  ems_i32 group=vals[0];
  ems_u32 coinctab=vals[1];
  ems_u32 temp;

  if (group<0)
    group=0x10;
  else
    group=(group &0x3) << 2;

  /* Select group */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, group);
  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.coinc_tab, &temp);
  /* Change only selected bits */
  temp=((temp & (~0xffff)) | ((coinctab << 0) & 0xffff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.coinc_tab, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setg_coinctab(struct lvd_dev* dev, int addr, ems_i32 group,
        ems_u32 coinctab)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setg_coinctab
                        };
  ems_u32 vals[2];
  vals[0]=group;
  vals[1]=coinctab;
  return access_qdcs(dev, addr, vals,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setg_coinctab");
}






struct get_coinctab_data
{
  ems_u32 *coinctab;
  int group;
};

static plerrcode
lvd_qdc80_getg_coinctab(struct lvd_acard* acard, ems_u32* data_, int idx)
{

  /* lvd_qdc80_getg_coinctab read bit(s) 0xffff from register coinc_tab for
     selected FPGA(s)(group(s)) */
  /* Lookup table for inputs FPGA coincidences */

  int res=0;
  struct get_coinctab_data *data=(struct get_coinctab_data*)data_;
  u_int32_t temp;

  if (data->group<0)
    {
      int group;
      for (group=0; group<4; group++)
        {
          /* Select FPGA */
          res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, group);
          /* Read register value */
          res|=lvd_i_r(acard->dev, acard->addr, qdc80.coinc_tab, &temp);
          /* Extract value */
          data->coinctab[4*idx+group]=(temp >> 0 ) & 0xffff;
        }
    }
  else
    {
      res|=lvd_i_w(acard->dev, acard->addr, qdc80.channel, data->group);
      /* Read register value */
      res|=lvd_i_r(acard->dev, acard->addr, qdc80.coinc_tab, &temp);
      /* Extract value */
      data->coinctab[idx]=(temp >> 0) & 0xffff;
    }

  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getg_coinctab(struct lvd_dev* dev, int addr, ems_i32 group,
        ems_u32* coinctab)
{

  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getg_coinctab
                        };

  struct get_coinc_data data;
  data.dest=coinctab;
  data.group=group;

  if (addr<0)
    memset(coinctab, 0, 4*16*sizeof(ems_u32));
  else
    memset(coinctab, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr,(ems_u32*) &data,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getg_coinctab");
}

static plerrcode
lvd_qdc80_setm_cr(struct lvd_acard* acard, ems_u32 *val,
        __attribute__((unused)) int xxx)
{

  /* lvd_qdc80_setm_cr set  0xffff in  cr */
  /* Set cr register */

  int res=0;
  ems_u32 temp;
  ems_u32 cr=*val;

  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &temp);
  /* Change only selected bits */
  temp=((temp & (~0xffff)) | ((cr << 0) & 0xffff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.cr, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setm_cr(struct lvd_dev* dev, int addr, ems_u32 cr)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setm_cr
                        };
  return access_qdcs(dev, addr, &cr,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setm_cr");
}




static plerrcode
lvd_qdc80_getm_cr(struct lvd_acard* acard, ems_u32* cr, int idx)
{

  /* lvd_qdc80_getm_cr read  0xffff from  cr */
  /* Set cr register */

  int res=0;
  u_int32_t temp;

  /* Read   register value */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &temp);
  /* Extract value */
  cr[idx]=(temp >> 0 ) & 0xffff;
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getm_cr(struct lvd_dev* dev, int addr, ems_u32* cr)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getm_cr
                        };
  if (addr<0)
    memset(cr, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, cr,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getm_cr");
}

static plerrcode
lvd_qdc80_setm_ena(struct lvd_acard* acard, ems_u32 *val,
        __attribute__((unused)) int xxx)
{

  /* lvd_qdc80_setm_ena set  0x0001 in  cr */
  /* Enable module */

  int res=0;
  ems_u32 temp;
  ems_u32 ena=*val;

  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0001)) | ((ena << 0) & 0x0001));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.cr, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setm_ena(struct lvd_dev* dev, int addr, ems_u32 ena)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setm_ena
                        };
  return access_qdcs(dev, addr, &ena,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setm_ena");
}




static plerrcode
lvd_qdc80_getm_ena(struct lvd_acard* acard, ems_u32* ena, int idx)
{

  /* lvd_qdc80_getm_ena read  0x0001 from  cr */
  /* Enable module */

  int res=0;
  u_int32_t temp;

  /* Read   register value */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &temp);
  /* Extract value */
  ena[idx]=(temp >> 0 ) & 0x0001;
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getm_ena(struct lvd_dev* dev, int addr, ems_u32* ena)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getm_ena
                        };
  if (addr<0)
    memset(ena, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, ena,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getm_ena");
}

static plerrcode
lvd_qdc80_setm_trgmode(struct lvd_acard* acard, ems_u32 *val,
        __attribute__((unused)) int xxx)
{

  /* lvd_qdc80_setm_trgmode set  0x0006 in  cr */
  /* Select trigger mode */

  int res=0;
  ems_u32 temp;
  ems_u32 trgmode=*val;

  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0006)) | ((trgmode << 1) & 0x0006));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.cr, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setm_trgmode(struct lvd_dev* dev, int addr, ems_u32 trgmode)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setm_trgmode
                        };
  return access_qdcs(dev, addr, &trgmode,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setm_trgmode");
}




static plerrcode
lvd_qdc80_getm_trgmode(struct lvd_acard* acard, ems_u32* trgmode, int idx)
{

  /* lvd_qdc80_getm_trgmode read  0x0006 from  cr */
  /* Select trigger mode */

  int res=0;
  u_int32_t temp;

  /* Read   register value */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &temp);
  /* Extract value */
  trgmode[idx]=(temp >> 1 ) & 0x0006;
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getm_trgmode(struct lvd_dev* dev, int addr, ems_u32* trgmode)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getm_trgmode
                        };
  if (addr<0)
    memset(trgmode, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, trgmode,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getm_trgmode");
}

static plerrcode
lvd_qdc80_setm_tdcena(struct lvd_acard* acard, ems_u32 *val,
        __attribute__((unused)) int xxx)
{

  /* lvd_qdc80_setm_tdcena set  0x0008 in  cr */
  /* Enable TDC on external trigger input */

  int res=0;
  ems_u32 temp;
  ems_u32 tdcena=*val;

  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0008)) | ((tdcena << 3) & 0x0008));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.cr, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setm_tdcena(struct lvd_dev* dev, int addr, ems_u32 tdcena)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setm_tdcena
                        };
  return access_qdcs(dev, addr, &tdcena,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setm_tdcena");
}




static plerrcode
lvd_qdc80_getm_tdcena(struct lvd_acard* acard, ems_u32* tdcena, int idx)
{

  /* lvd_qdc80_getm_tdcena read  0x0008 from  cr */
  /* Enable TDC on external trigger input */

  int res=0;
  u_int32_t temp;

  /* Read   register value */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &temp);
  /* Extract value */
  tdcena[idx]=(temp >> 3 ) & 0x0008;
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getm_tdcena(struct lvd_dev* dev, int addr, ems_u32* tdcena)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getm_tdcena
                        };
  if (addr<0)
    memset(tdcena, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, tdcena,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getm_tdcena");
}

static plerrcode
lvd_qdc80_setm_fixbaseline(struct lvd_acard* acard, ems_u32 *val,
        __attribute__((unused)) int xxx)
{

  /* lvd_qdc80_setm_fixbaseline set  0x0010 in  cr */
  /* Fix baseline with last value */

  int res=0;
  ems_u32 temp;
  ems_u32 fixbaseline=*val;

  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0010)) | ((fixbaseline << 4) & 0x0010));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.cr, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setm_fixbaseline(struct lvd_dev* dev, int addr, ems_u32 fixbaseline)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setm_fixbaseline
                        };
  return access_qdcs(dev, addr, &fixbaseline,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setm_fixbaseline");
}




static plerrcode
lvd_qdc80_getm_fixbaseline(struct lvd_acard* acard, ems_u32* fixbaseline,
        int idx)
{

  /* lvd_qdc80_getm_fixbaseline read  0x0010 from  cr */
  /* Fix baseline with last value */

  int res=0;
  u_int32_t temp;

  /* Read   register value */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &temp);
  /* Extract value */
  fixbaseline[idx]=(temp >> 4 ) & 0x0010;
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getm_fixbaseline(struct lvd_dev* dev, int addr, ems_u32* fixbaseline)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getm_fixbaseline
                        };
  if (addr<0)
    memset(fixbaseline, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, fixbaseline,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getm_fixbaseline");
}

static plerrcode
lvd_qdc80_setm_tstsig(struct lvd_acard* acard, ems_u32 *val,
        __attribute__((unused)) int xxx)
{

  /* lvd_qdc80_setm_tstsig set  0x0040 in  cr */
  /* Switch on test signal */

  int res=0;
  ems_u32 temp;
  ems_u32 tstsig=*val;

  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0040)) | ((tstsig << 6) & 0x0040));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.cr, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setm_tstsig(struct lvd_dev* dev, int addr, ems_u32 tstsig)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setm_tstsig
                        };
  return access_qdcs(dev, addr, &tstsig,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setm_tstsig");
}




static plerrcode
lvd_qdc80_getm_tstsig(struct lvd_acard* acard, ems_u32* tstsig, int idx)
{

  /* lvd_qdc80_getm_tstsig read  0x0040 from  cr */
  /* Switch on test signal */

  int res=0;
  u_int32_t temp;

  /* Read   register value */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &temp);
  /* Extract value */
  tstsig[idx]=(temp >> 6 ) & 0x0040;
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getm_tstsig(struct lvd_dev* dev, int addr, ems_u32* tstsig)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getm_tstsig
                        };
  if (addr<0)
    memset(tstsig, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, tstsig,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getm_tstsig");
}

static plerrcode
lvd_qdc80_setm_adcpwr(struct lvd_acard* acard, ems_u32 *val,
        __attribute__((unused)) int xxx)
{

  /* lvd_qdc80_setm_adcpwr set  0x0080 in  cr */
  /* Switch on ADC power */

  int res=0;
  ems_u32 temp;
  ems_u32 adcpwr=*val;

  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0080)) | ((adcpwr << 7) & 0x0080));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.cr, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setm_adcpwr(struct lvd_dev* dev, int addr, ems_u32 adcpwr)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setm_adcpwr
                        };
  return access_qdcs(dev, addr, &adcpwr,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setm_adcpwr");
}




static plerrcode
lvd_qdc80_getm_adcpwr(struct lvd_acard* acard, ems_u32* adcpwr, int idx)
{

  /* lvd_qdc80_getm_adcpwr read  0x0080 from  cr */
  /* Switch on ADC power */

  int res=0;
  u_int32_t temp;

  /* Read   register value */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &temp);
  /* Extract value */
  adcpwr[idx]=(temp >> 7 ) & 0x0080;
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getm_adcpwr(struct lvd_dev* dev, int addr, ems_u32* adcpwr)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getm_adcpwr
                        };
  if (addr<0)
    memset(adcpwr, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, adcpwr,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getm_adcpwr");
}

static plerrcode
lvd_qdc80_setm_f1mode(struct lvd_acard* acard, ems_u32 *val,
        __attribute__((unused)) int xxx)
{

  /* lvd_qdc80_setm_f1mode set  0x0100 in  cr */
  /* Switch to F1 mode */

  int res=0;
  ems_u32 temp;
  ems_u32 f1mode=*val;

  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0100)) | ((f1mode << 8) & 0x0100));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.cr, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setm_f1mode(struct lvd_dev* dev, int addr, ems_u32 f1mode)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setm_f1mode
                        };
  return access_qdcs(dev, addr, &f1mode,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setm_f1mode");
}




static plerrcode
lvd_qdc80_getm_f1mode(struct lvd_acard* acard, ems_u32* f1mode, int idx)
{

  /* lvd_qdc80_getm_f1mode read  0x0100 from  cr */
  /* Switch to F1 mode */

  int res=0;
  u_int32_t temp;

  /* Read   register value */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.cr, &temp);
  /* Extract value */
  f1mode[idx]=(temp >> 8 ) & 0x0100;
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getm_f1mode(struct lvd_dev* dev, int addr, ems_u32* f1mode)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getm_f1mode
                        };
  if (addr<0)
    memset(f1mode, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, f1mode,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getm_f1mode");
}

static plerrcode
lvd_qdc80_setm_grpcoinc(struct lvd_acard* acard, ems_u32 *val,
        __attribute__((unused)) int xxx)
{

  /* lvd_qdc80_setm_grpcoinc set  0xffff in  grp_coinc */
  /* Lookup table for main coincidences */

  int res=0;
  ems_u32 temp;
  ems_u32 grpcoinc=*val;

  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.grp_coinc, &temp);
  /* Change only selected bits */
  temp=((temp & (~0xffff)) | ((grpcoinc << 0) & 0xffff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.grp_coinc, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setm_grpcoinc(struct lvd_dev* dev, int addr, ems_u32 grpcoinc)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setm_grpcoinc
                        };
  return access_qdcs(dev, addr, &grpcoinc,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setm_grpcoinc");
}




static plerrcode
lvd_qdc80_getm_grpcoinc(struct lvd_acard* acard, ems_u32* grpcoinc, int idx)
{

  /* lvd_qdc80_getm_grpcoinc read  0xffff from  grp_coinc */
  /* Lookup table for main coincidences */

  int res=0;
  u_int32_t temp;

  /* Read   register value */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.grp_coinc, &temp);
  /* Extract value */
  grpcoinc[idx]=(temp >> 0 ) & 0xffff;
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getm_grpcoinc(struct lvd_dev* dev, int addr, ems_u32* grpcoinc)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getm_grpcoinc
                        };
  if (addr<0)
    memset(grpcoinc, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, grpcoinc,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getm_grpcoinc");
}

static plerrcode
lvd_qdc80_setm_scalerrout(struct lvd_acard* acard, ems_u32 *val,
        __attribute__((unused)) int xxx)
{

  /* lvd_qdc80_setm_scalerrout set  0x000f in  scaler_rout */
  /* Set scaler readout rate */

  int res=0;
  ems_u32 temp;
  ems_u32 scalerrout=*val;

  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.scaler_rout, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x000f)) | ((scalerrout << 0) & 0x000f));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.scaler_rout, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setm_scalerrout(struct lvd_dev* dev, int addr, ems_u32 scalerrout)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setm_scalerrout
                        };
  return access_qdcs(dev, addr, &scalerrout,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setm_scalerrout");
}




static plerrcode
lvd_qdc80_getm_scalerrout(struct lvd_acard* acard, ems_u32* scalerrout, int idx)
{

  /* lvd_qdc80_getm_scalerrout read  0x000f from  scaler_rout */
  /* Set scaler readout rate */

  int res=0;
  u_int32_t temp;

  /* Read   register value */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.scaler_rout, &temp);
  /* Extract value */
  scalerrout[idx]=(temp >> 0 ) & 0x000f;
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getm_scalerrout(struct lvd_dev* dev, int addr, ems_u32* scalerrout)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getm_scalerrout
                        };
  if (addr<0)
    memset(scalerrout, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, scalerrout,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getm_scalerrout");
}

static plerrcode
lvd_qdc80_setm_coinmintraw(struct lvd_acard* acard, ems_u32 *val,
        __attribute__((unused)) int xxx)
{

  /* lvd_qdc80_setm_coinmintraw set  0xffff in  coinmin_traw */
  /* Set coinmin_traw register */

  int res=0;
  ems_u32 temp;
  ems_u32 coinmintraw=*val;

  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.coinmin_traw, &temp);
  /* Change only selected bits */
  temp=((temp & (~0xffff)) | ((coinmintraw << 0) & 0xffff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.coinmin_traw, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setm_coinmintraw(struct lvd_dev* dev, int addr, ems_u32 coinmintraw)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setm_coinmintraw
                        };
  return access_qdcs(dev, addr, &coinmintraw,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setm_coinmintraw");
}




static plerrcode
lvd_qdc80_getm_coinmintraw(struct lvd_acard* acard, ems_u32* coinmintraw,
        int idx)
{

  /* lvd_qdc80_getm_coinmintraw read  0xffff from  coinmin_traw */
  /* Set coinmin_traw register */

  int res=0;
  u_int32_t temp;

  /* Read   register value */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.coinmin_traw, &temp);
  /* Extract value */
  coinmintraw[idx]=(temp >> 0 ) & 0xffff;
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getm_coinmintraw(struct lvd_dev* dev, int addr, ems_u32* coinmintraw)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getm_coinmintraw
                        };
  if (addr<0)
    memset(coinmintraw, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, coinmintraw,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getm_coinmintraw");
}

static plerrcode
lvd_qdc80_setm_traw(struct lvd_acard* acard, ems_u32 *val,
        __attribute__((unused)) int xxx)
{

  /* lvd_qdc80_setm_traw set  0x000f in  coinmin_traw */
  /* Raw cycle frequency */

  int res=0;
  ems_u32 temp;
  ems_u32 traw=*val;

  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.coinmin_traw, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x000f)) | ((traw << 0) & 0x000f));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.coinmin_traw, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setm_traw(struct lvd_dev* dev, int addr, ems_u32 traw)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setm_traw
                        };
  return access_qdcs(dev, addr, &traw,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setm_traw");
}




static plerrcode
lvd_qdc80_getm_traw(struct lvd_acard* acard, ems_u32* traw, int idx)
{

  /* lvd_qdc80_getm_traw read  0x000f from  coinmin_traw */
  /* Raw cycle frequency */

  int res=0;
  u_int32_t temp;

  /* Read   register value */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.coinmin_traw, &temp);
  /* Extract value */
  traw[idx]=(temp >> 0 ) & 0x000f;
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getm_traw(struct lvd_dev* dev, int addr, ems_u32* traw)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getm_traw
                        };
  if (addr<0)
    memset(traw, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, traw,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getm_traw");
}

static plerrcode
lvd_qdc80_setm_coinmin(struct lvd_acard* acard, ems_u32 *val,
        __attribute__((unused)) int xxx)
{

  /* lvd_qdc80_setm_coinmin set  0x00f0 in  coinmin_traw */
  /* Minimum coincidence length */

  int res=0;
  ems_u32 temp;
  ems_u32 coinmin=*val;

  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.coinmin_traw, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x00f0)) | ((coinmin << 4) & 0x00f0));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.coinmin_traw, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setm_coinmin(struct lvd_dev* dev, int addr, ems_u32 coinmin)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setm_coinmin
                        };
  return access_qdcs(dev, addr, &coinmin,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setm_coinmin");
}




static plerrcode
lvd_qdc80_getm_coinmin(struct lvd_acard* acard, ems_u32* coinmin, int idx)
{

  /* lvd_qdc80_getm_coinmin read  0x00f0 from  coinmin_traw */
  /* Minimum coincidence length */

  int res=0;
  u_int32_t temp;

  /* Read   register value */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.coinmin_traw, &temp);
  /* Extract value */
  coinmin[idx]=(temp >> 4 ) & 0x00f0;
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getm_coinmin(struct lvd_dev* dev, int addr, ems_u32* coinmin)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getm_coinmin
                        };
  if (addr<0)
    memset(coinmin, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, coinmin,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getm_coinmin");
}

static plerrcode
lvd_qdc80_setm_tpdac(struct lvd_acard* acard, ems_u32 *val,
        __attribute__((unused)) int xxx)
{

  /* lvd_qdc80_setm_tpdac set  0x0ff0 in  tp_dac */
  /* Test pulse level */

  int res=0;
  ems_u32 temp;
  ems_u32 tpdac=*val;

  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.tp_dac, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0ff0)) | ((tpdac << 4) & 0x0ff0));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.tp_dac, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setm_tpdac(struct lvd_dev* dev, int addr, ems_u32 tpdac)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setm_tpdac
                        };
  return access_qdcs(dev, addr, &tpdac,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setm_tpdac");
}




static plerrcode
lvd_qdc80_getm_tpdac(struct lvd_acard* acard, ems_u32* tpdac, int idx)
{

  /* lvd_qdc80_getm_tpdac read  0x0ff0 from  tp_dac */
  /* Test pulse level */

  int res=0;
  u_int32_t temp;

  /* Read   register value */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.tp_dac, &temp);
  /* Extract value */
  tpdac[idx]=(temp >> 4 ) & 0x0ff0;
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getm_tpdac(struct lvd_dev* dev, int addr, ems_u32* tpdac)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getm_tpdac
                        };
  if (addr<0)
    memset(tpdac, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, tpdac,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getm_tpdac");
}

static plerrcode
lvd_qdc80_setm_blcorcycle(struct lvd_acard* acard, ems_u32 *val,
        __attribute__((unused)) int xxx)
{

  /* lvd_qdc80_setm_blcorcycle set  0x00ff in  bl_cor_cycle */
  /* Baseline correction cycle */

  int res=0;
  ems_u32 temp;
  ems_u32 blcorcycle=*val;

  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.bl_cor_cycle, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x00ff)) | ((blcorcycle << 0) & 0x00ff));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.bl_cor_cycle, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setm_blcorcycle(struct lvd_dev* dev, int addr, ems_u32 blcorcycle)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setm_blcorcycle
                        };
  return access_qdcs(dev, addr, &blcorcycle,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setm_blcorcycle");
}




static plerrcode
lvd_qdc80_getm_blcorcycle(struct lvd_acard* acard, ems_u32* blcorcycle, int idx)
{

  /* lvd_qdc80_getm_blcorcycle read  0x00ff from  bl_cor_cycle */
  /* Baseline correction cycle */

  int res=0;
  u_int32_t temp;

  /* Read   register value */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.bl_cor_cycle, &temp);
  /* Extract value */
  blcorcycle[idx]=(temp >> 0 ) & 0x00ff;
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getm_blcorcycle(struct lvd_dev* dev, int addr, ems_u32* blcorcycle)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getm_blcorcycle
                        };
  if (addr<0)
    memset(blcorcycle, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, blcorcycle,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getm_blcorcycle");
}


/* lvd_qdc80_getm_baseline get  0xffff in  baseline */
/* Read actual baseline */


static plerrcode
lvd_qdc80_getm_baseline(struct lvd_acard* acard, ems_u32* baseline, int idx)
{

  /* lvd_qdc80_getm_baseline read  0xffff from  baseline */

  int res=0;
  u_int32_t temp;

  /* Read   register value */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.baseline, &temp);
  /* Extract value */
  baseline[idx]=(temp >> 0) & 0xffff;
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getm_baseline(struct lvd_dev* dev, int addr, ems_u32* baseline)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getm_baseline
                        };
  if (addr<0)
    memset(baseline, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, baseline,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getm_baseline");
}


/* lvd_qdc80_getm_tcornoise get  0xffff in  tcor_noise */
/* Read tcornoise register */


static plerrcode
lvd_qdc80_getm_tcornoise(struct lvd_acard* acard, ems_u32* tcornoise, int idx)
{

  /* lvd_qdc80_getm_tcornoise read  0xffff from  tcor_noise */

  int res=0;
  u_int32_t temp;

  /* Read   register value */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.tcor_noise, &temp);
  /* Extract value */
  tcornoise[idx]=(temp >> 0) & 0xffff;
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getm_tcornoise(struct lvd_dev* dev, int addr, ems_u32* tcornoise)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getm_tcornoise
                        };
  if (addr<0)
    memset(tcornoise, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, tcornoise,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getm_tcornoise");
}


/* lvd_qdc80_getm_tcor get  0xff00 in  tcor_noise */
/* Read max tau correction */


static plerrcode
lvd_qdc80_getm_tcor(struct lvd_acard* acard, ems_u32* tcor, int idx)
{

  /* lvd_qdc80_getm_tcor read  0xff00 from  tcor_noise */

  int res=0;
  u_int32_t temp;

  /* Read   register value */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.tcor_noise, &temp);
  /* Extract value */
  tcor[idx]=(temp >> 8) & 0xff00;
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getm_tcor(struct lvd_dev* dev, int addr, ems_u32* tcor)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getm_tcor
                        };
  if (addr<0)
    memset(tcor, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, tcor,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getm_tcor");
}


/* lvd_qdc80_getm_noise get  0x00ff in  tcor_noise */
/* Read max tau correction */


static plerrcode
lvd_qdc80_getm_noise(struct lvd_acard* acard, ems_u32* noise, int idx)
{

  /* lvd_qdc80_getm_noise read  0x00ff from  tcor_noise */

  int res=0;
  u_int32_t temp;

  /* Read   register value */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.tcor_noise, &temp);
  /* Extract value */
  noise[idx]=(temp >> 0) & 0x00ff;
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getm_noise(struct lvd_dev* dev, int addr, ems_u32* noise)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getm_noise
                        };
  if (addr<0)
    memset(noise, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, noise,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getm_noise");
}


/* lvd_qdc80_getm_dttauquality get  0x003f in  dttau_quality */
/* Capacity correction assessment */


static plerrcode
lvd_qdc80_getm_dttauquality(struct lvd_acard* acard, ems_u32* dttauquality,
        int idx)
{

  /* lvd_qdc80_getm_dttauquality read  0x003f from  dttau_quality */

  int res=0;
  u_int32_t temp;

  /* Read   register value */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.dttau_quality, &temp);
  /* Extract value */
  dttauquality[idx]=(temp >> 0) & 0x003f;
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getm_dttauquality(struct lvd_dev* dev, int addr, ems_u32* dttauquality)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getm_dttauquality
                        };
  if (addr<0)
    memset(dttauquality, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, dttauquality,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getm_dttauquality");
}


/* lvd_qdc80_getm_ovr get  0xffff in  ctrl_ovr */
/* Read and clear overruns bits */


static plerrcode
lvd_qdc80_getm_ovr(struct lvd_acard* acard, ems_u32* ovr, int idx)
{

  /* lvd_qdc80_getm_ovr read  0xffff from  ctrl_ovr */

  int res=0;
  u_int32_t temp;

  /* Read   register value */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.ctrl_ovr, &temp);
  /* Extract value */
  ovr[idx]=(temp >> 0) & 0xffff;
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_getm_ovr(struct lvd_dev* dev, int addr, ems_u32* ovr)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_getm_ovr
                        };
  if (addr<0)
    memset(ovr, 0, 16*sizeof(ems_u32));
  return access_qdcs(dev, addr, ovr,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_getm_ovr");
}

static plerrcode
lvd_qdc80_setm_ctrl(struct lvd_acard* acard, ems_u32 *val,
        __attribute__((unused)) int xxx)
{

  /* lvd_qdc80_set_ctrl set  0x0008 in  ctrl_ovr */
  /* Generate test pulse */

  int res=0;
  ems_u32 temp;
  ems_u32 ctrl=*val;

  /* Read  register  */
  res|=lvd_i_r(acard->dev, acard->addr, qdc80.ctrl_ovr, &temp);
  /* Change only selected bits */
  temp=((temp & (~0x0008)) | ((ctrl << 3) & 0x0008));
  /* Write register */
  res|=lvd_i_w(acard->dev, acard->addr, qdc80.ctrl_ovr, temp);
  return res?plErr_HW:plOK;
}




plerrcode
lvd_qdc_setm_ctrl(struct lvd_dev* dev, int addr, ems_u32 ctrl)
{
  static qdcfun funs[]= {0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         lvd_qdc80_setm_ctrl
                        };
  return access_qdcs(dev, addr, &ctrl,
                     funs, sizeof(funs)/sizeof(qdcfun),
                     0,
                     "lvd_qdc_setm_ctrl");
}

