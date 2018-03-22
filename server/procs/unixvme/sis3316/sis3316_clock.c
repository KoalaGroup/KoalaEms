/*
 * procs/unixvme/sis3316/sis3316_clock.c
 * created 2016-Feb-09 PeWue
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3316_clock.c,v 1.2 2016/05/12 21:52:57 wuestner Exp $";

/*
 * This code manipulates the clock chip of the sis3316 module.
 * The chip is: Silicon Labs Si750 ACB000115G (7 ppm temperature stability)
 */

#include "sis3316.h"
#include <errno.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../../lowlevel/oscompat/oscompat.h"
#include "../../../lowlevel/unixvme/vme.h"

RCS_REGISTER(cvsid, "procs/unixvme/sis3316")

#define SIS3316_ADC_CLK_OSC_I2C_REG 0x40

#define I2C_ACK        8
#define I2C_START      9
#define I2C_REP_START 10
#define I2C_STOP      11
#define I2C_WRITE     12
#define I2C_READ      13
#define I2C_BUSY      31

#define OSC_ADR	0x55

/*****************************************************************************/
static plerrcode
__attribute__((unused))
sis3316_read_reg(ml_entry* module, ems_u32 reg, ems_u32* value)
{
    struct sis3316_priv_data *priv;
    priv=(struct sis3316_priv_data*)module->private_data;

    return priv->sis3316_read_reg(module, reg, value);
}
/*****************************************************************************/
static plerrcode
__attribute__((unused))
sis3316_read_regs(ml_entry* module, ems_u32 *regs, ems_u32* values, int num)
{
    struct sis3316_priv_data *priv;
    priv=(struct sis3316_priv_data*)module->private_data;

    return priv->sis3316_read_regs(module, regs, values, num);
}
/*****************************************************************************/
static plerrcode
__attribute__((unused))
sis3316_write_reg(ml_entry* module, ems_u32 reg, ems_u32 value)
{
    struct sis3316_priv_data *priv;
    priv=(struct sis3316_priv_data*)module->private_data;

    return priv->sis3316_write_reg(module, reg, value);
}
/*****************************************************************************/
static plerrcode
__attribute__((unused))
sis3316_write_regs(ml_entry* module, ems_u32 *regs, ems_u32 *values, int num)
{
    struct sis3316_priv_data *priv;
    priv=(struct sis3316_priv_data*)module->private_data;

    return priv->sis3316_write_regs(module, regs, values, num);
}
/*****************************************************************************/
static plerrcode
I2cStart(ml_entry* module, ems_u32 addr)
{
    plerrcode pres;
    ems_u32 v;
    int ii;

    pres=sis3316_write_reg(module, addr, 1<<I2C_START);
    if (pres!=plOK)
        goto error;

    ii=1000;
    do {
        pres=sis3316_read_reg(module, addr, &v);
        if (pres!=plOK)
            goto error;
    } while (v&(1<<I2C_BUSY) && --ii>0);

    if (v&(1<<I2C_BUSY)) {
        complain("sis3316:I2cStart: busy");
        pres=plErr_Timeout;
    }

error:
    return pres;
}
/*****************************************************************************/
static plerrcode
I2cStop(ml_entry* module, ems_u32 addr)
{
    plerrcode pres;
    ems_u32 v;
    int ii;

    pres=sis3316_write_reg(module, addr, 1<<I2C_STOP);
    if (pres!=plOK)
        goto error;

    ii=1000;
    do {
        pres=sis3316_read_reg(module, addr, &v);
        if (pres!=plOK)
            goto error;
    } while (v&(1<<I2C_BUSY) && --ii>0);

    if (v&(1<<I2C_BUSY)) {
        complain("sis3316:I2cStop: busy");
        pres=plErr_Timeout;
    }

error:
    return pres;
}
/*****************************************************************************/
static plerrcode
I2cWriteByte(ml_entry* module, ems_u32 addr, u_int8_t val, int* ack)
{
    plerrcode pres;
    ems_u32 v;
    int ii;

    pres=sis3316_write_reg(module, addr, 1<<I2C_WRITE ^ val);
    if (pres!=plOK)
        goto error;

    ii=1000;
    do {
        pres=sis3316_read_reg(module, addr, &v);
        if (pres!=plOK)
            goto error;
    } while (v&(1<<I2C_BUSY) && --ii>0);

    if (v&(1<<I2C_BUSY)) {
        complain("sis3316:I2cWriteByte: busy");
        pres=plErr_Timeout;
    }
    *ack=!!(v&(1<<I2C_ACK));
    if (!*ack) {
        complain("sis3316:I2cWriteByte: no acknowledge");
        pres=plErr_HW;
    }

error:
    return pres;
}
/*****************************************************************************/
static plerrcode
I2cReadByte(ml_entry* module, ems_u32 addr, u_int8_t* val, int ack)
{
    plerrcode pres;
    ems_u32 v;
    int ii;

    v=1<<I2C_READ;
    if (ack)
        v|=1<<I2C_ACK;
    pres=sis3316_write_reg(module, addr, v);
    if (pres!=plOK)
        goto error;

    ii=1000;
    do {
        pres=sis3316_read_reg(module, addr, &v);
        if (pres!=plOK)
            goto error;
    } while (v&(1<<I2C_BUSY) && --ii>0);

    if (v&(1<<I2C_BUSY)) {
        complain("sis3316:I2cReadByte: busy");
        pres=plErr_Timeout;
    }

    *val=v&0xff;

error:
    return pres;
}
/*****************************************************************************/
static plerrcode
si750_get_frequency(ml_entry* module, ems_u32 addr, u_int8_t regs[6])
{
    plerrcode pres=plErr_System;
    int ack, i;

    /* start */
    if ((pres=I2cStart(module, addr))!=plOK)
        goto error;

    /* I2C address */
    if ((pres=I2cWriteByte(module, addr, OSC_ADR<<1, &ack))!=plOK)
        goto error;

    /* register offset (High Speed/N1 Dividers) */
    if ((pres=I2cWriteByte(module, addr, 0x0D, &ack))!=plOK)
        goto error;

    if ((pres=I2cStart(module, addr))!=plOK)
        goto error;

    /* address+1 */
    if ((pres=I2cWriteByte(module, addr, (OSC_ADR<<1)+1, &ack))!=plOK)
        goto error;

    /* read data */
    for (i=0; i<6; i++) {
        ack=1;
        if (i==5)
            ack=0;
        if ((pres=I2cReadByte(module, addr, regs+i, ack))!=plOK)
            goto error;
    }
    pres=plOK;

error:
    I2cStop(module, addr);
    return pres;
}
/*****************************************************************************/
static plerrcode
__attribute__((unused))
si750_get_bytes(ml_entry* module, ems_u32 addr, int regaddr, u_int8_t *regs,
        int num)
{
    plerrcode pres=plErr_System;
    int ack, i;

    /* start */
    if ((pres=I2cStart(module, addr))!=plOK)
        goto error;

    /* I2C address */
    if ((pres=I2cWriteByte(module, addr, OSC_ADR<<1, &ack))!=plOK)
        goto error;

    /* register offset */
    if ((pres=I2cWriteByte(module, addr, regaddr, &ack))!=plOK)
        goto error;

    if ((pres=I2cStart(module, addr))!=plOK)
        goto error;

    /* address+1 */
    if ((pres=I2cWriteByte(module, addr, (OSC_ADR<<1)+1, &ack))!=plOK)
        goto error;

    /* read data */
    for (i=0; i<num; i++) {
        ack=1;
        if (i+1==num)
            ack=0;
        if ((pres=I2cReadByte(module, addr, regs+i, ack))!=plOK)
            goto error;
    }
    pres=plOK;

error:
    I2cStop(module, addr);
    return pres;
}
/*****************************************************************************/
static plerrcode
si750_get_initial_clock(ml_entry* module, unsigned int clockidx)
{
    struct sis3316_priv_data *priv;
    struct sis3316_clock *clock;
    u_int64_t rfreq;
    ems_u32 addr;
    plerrcode pres;

    priv=(struct sis3316_priv_data*)module->private_data;
    clock=priv->clocks+clockidx;

    if (clock->initial_clock_valid)
        return plOK;

    addr=SIS3316_ADC_CLK_OSC_I2C_REG+4*clockidx;

    /* read and store old values */
    pres=si750_get_frequency(module, addr, clock->initial_clock);
    if (pres!=plOK)
        return pres;

    rfreq=clock->initial_clock[1]&0x3f;
    rfreq<<=8;
    rfreq|=clock->initial_clock[2];
    rfreq<<=8;
    rfreq|=clock->initial_clock[3];
    rfreq<<=8;
    rfreq|=clock->initial_clock[4];
    rfreq<<=8;
    rfreq|=clock->initial_clock[5];

    clock->rfreq5g=rfreq;

    clock->initial_clock_valid=1;

    return plOK;
}
/*****************************************************************************/
static plerrcode
si750_set_frequency(ml_entry* module, ems_u32 addr, u_int8_t regs[6])
{
    plerrcode pres=plErr_System;
    int ack, i;

    /* start */
    if ((pres=I2cStart(module, addr))!=plOK)
        goto error;

    /* I2C address */
    if ((pres=I2cWriteByte(module, addr, OSC_ADR<<1, &ack))!=plOK)
        goto error;

    /* register offset (High Speed/N1 Dividers) */
    if ((pres=I2cWriteByte(module, addr, 0x0D, &ack))!=plOK)
        goto error;

    for (i=0; i<6; i++) {
        if ((pres=I2cWriteByte(module, addr, regs[i], &ack))!=plOK)
            goto error;
    }
    pres=plOK;

error:
    I2cStop(module, addr);
    return pres;
}
/*****************************************************************************/
static plerrcode
si750_newfrequency(ml_entry* module, ems_u32 addr)
{
    plerrcode pres=plErr_System;
    struct timespec ts={0, 0};
    int ack;

    /* start */
    if ((pres=I2cStart(module, addr))!=plOK)
        goto error;

    /* I2C address */
    if ((pres=I2cWriteByte(module, addr, OSC_ADR<<1, &ack))!=plOK)
        goto error;

    /* register offset (135) */
    if ((pres=I2cWriteByte(module, addr, 0x87, &ack))!=plOK)
        goto error;

    /* bit 6 */
    if ((pres=I2cWriteByte(module, addr, 0x40, &ack))!=plOK)
        goto error;
    pres=plOK;

    /* wait min. 10 ms */
    ts.tv_nsec=15000000; /* 15 ms */
    if (nanosleep(&ts, 0)<0) {
        printf("nanosleep: %s\n", strerror(errno));
    }

error:
    I2cStop(module, addr);
    return pres;
}
/*****************************************************************************/
static plerrcode
si750_freezeDCO(ml_entry* module, ems_u32 addr)
{
    plerrcode pres=plErr_System;
    int ack;

    /* start */
    if ((pres=I2cStart(module, addr))!=plOK)
        goto error;

    /* I2C address */
    if ((pres=I2cWriteByte(module, addr, OSC_ADR<<1, &ack))!=plOK)
        goto error;

    /* register offset (Freeze DCO), 137 */
    if ((pres=I2cWriteByte(module, addr, 0x89, &ack))!=plOK)
        goto error;

    /* bit 4 */
    if ((pres=I2cWriteByte(module, addr, 0x10, &ack))!=plOK)
        goto error;
    pres=plOK;

error:
    I2cStop(module, addr);
    return pres;
}
/*****************************************************************************/
static plerrcode
si750_unfreezeDCO(ml_entry* module, ems_u32 addr)
{
    plerrcode pres=plErr_System;
    int ack;

    /* start */
    if ((pres=I2cStart(module, addr))!=plOK)
        goto error;

    /* I2C address */
    if ((pres=I2cWriteByte(module, addr, OSC_ADR<<1, &ack))!=plOK)
        goto error;

    /* register offset (Freeze DCO), 137 */
    if ((pres=I2cWriteByte(module, addr, 0x89, &ack))!=plOK)
        goto error;

    /* bit 4 */
    if ((pres=I2cWriteByte(module, addr, 0x00, &ack))!=plOK)
        goto error;
    pres=plOK;

error:
    I2cStop(module, addr);
    return pres;
}
/*****************************************************************************/
static plerrcode
si750_recall(ml_entry* module, ems_u32 addr)
{
    plerrcode pres=plErr_System;
    struct timespec ts={0, 0};
    int ack;

    /* start */
    if ((pres=I2cStart(module, addr))!=plOK)
        goto error;

    /* I2C address */
    if ((pres=I2cWriteByte(module, addr, OSC_ADR<<1, &ack))!=plOK)
        goto error;

    /* register offset (Reset/Freeze/Memory Control), 135 */
    if ((pres=I2cWriteByte(module, addr, 0x87, &ack))!=plOK)
        goto error;

    /* bit 0 */
    if ((pres=I2cWriteByte(module, addr, 0x01, &ack))!=plOK)
        goto error;

    /* not sure whether this is necessary; wait min. 5 ms */
    ts.tv_nsec=5000000;
    if (nanosleep(&ts, 0)<0) {
        printf("nanosleep: %s\n", strerror(errno));
    }

    pres=plOK;

error:
    I2cStop(module, addr);
    return pres;
}
/*****************************************************************************/
/*
 * This procedure must be called after any change of the sample clock frequency.
 * The frequency is given in Hz.
 */
/*
given in the manual for SIS3316-250MHz-14bit (for fw version>=4) 
MHz add 1/2 sample tap delay
250.00           1      0x02
227.273          1      0x1F
208.333          1      0x35
178.571          0      0x12
166.667          0      0x20
138.889          0      0x35
125.000          0      0x50
119.048          0      0x60
113.636          1      0x10
104.167          1      0x20
100.000          1      0x20
 83.333          1      0x30
 71.429          1      0x60
 62.500          1      0x60
 50.000          0      0x20
 25.000          0      0x20

given in the manual for SIS3316-125MHz-16bit (for fw version>=4) 
MHz add 1/2 sample tap delay
125.000          1      0x20
119.048          1      0x20
113.636          1      0x20
104.167          1      0x30
100.000          1      0x30
 83.333          1      0x40
 71.429          1      0x60
 62.500          0      0x20
 50.000          0      0x30
 25.000          0      0x30
*/
struct tapdata {
    int freq;
    int half;
    int delay;
};
static struct tapdata tapdata[]={
    {250000000, 1, 0x02},
    {227273000, 1, 0x1F},
    {208333000, 1, 0x35},
    {178571000, 0, 0x12},
    {166667000, 0, 0x20},
    {138889000, 0, 0x35},
    {125000000, 0, 0x50},
    {119048000, 0, 0x60},
    {113636000, 1, 0x10},
    {104167000, 1, 0x20},
    {100000000, 1, 0x20},
    { 83333000, 1, 0x30},
    { 71429000, 1, 0x60},
    { 62500000, 1, 0x60},
    { 50000000, 0, 0x20},
    { 25000000, 0, 0x20},
};
static plerrcode
sis3316_sample_clock_changed(ml_entry* module, int frequency)
{
    ems_u32 tap_del_regs[4]={0x1000, 0x2000, 0x3000, 0x4000};
    ems_u32 status_regs[4] ={0x1104, 0x2104, 0x3104, 0x4104};
    ems_u32 vals[4], val;
    struct timespec ts={0, 0};
    plerrcode pres;
    int half, del, do_it=0, exact=0, n, i;

    /* DCM/PLL reset */
    if ((pres=sis3316_write_reg(module, 0x438, 0))!=plOK)
        return pres;

    /* wait min. 5 ms */
    ts.tv_nsec=10000000;
    if (nanosleep(&ts, 0)<0) {
        printf("nanosleep: %s\n", strerror(errno));
    }

    /* request calibration, clear errors and select all channels */
    for (i=0; i<4; i++)
        vals[i]=0xf00;
    if ((pres=sis3316_write_regs(module, tap_del_regs, vals, 4))!=plOK)
        goto error;

    ts.tv_nsec=10000; /* 10 us */
    if (nanosleep(&ts, 0)<0) {
        printf("nanosleep: %s\n", strerror(errno));
    }

    if ((pres=sis3316_read_regs(module, status_regs, vals, 4))!=plOK)
        goto error;
    for (i=0; i<4; i++) {
        if ((vals[i]&0xff)!=0x18) {
            complain("sis3316_sample_clock: bad FPGA[%d] status: %08x",
                    i, vals[i]);
            pres=plErr_Verify;
            goto error;
        }
    }

    /* find the frequency in the table */
    n=sizeof(tapdata)/sizeof(struct tapdata);
    for (i=0; i<n; i++) {
        if (tapdata[i].freq==frequency) {
            do_it=1;
            exact=1;
            break;
        }
    }
    if (!do_it || !exact)
        complain("sis3316_sample_clock: frequency %d not supported", frequency);
    if (!do_it)
        goto error;
    if (!exact)
        complain("sis3316_sample_clock: using interpolated values "
                "for ADC tap delay");
        /* but we don't have interpolation yet */

    half=tapdata[i].half;
    del=tapdata[i].delay;

    /* select all channels and set delay and "add 1/2 sample" */
    val=0x300+del;
    if (half)
        val|=1<<12;
    for (i=0; i<4; i++)
        vals[i]=val;
    if ((pres=sis3316_write_regs(module, tap_del_regs, vals, 4))!=plOK)
        goto error;

    printf("sis3316_sample_clock: frequency %d, add 1/2:%d, delay=%d\n",
            frequency, half, del);

error:
    return pres;
}
/*****************************************************************************/
/*****************************************************************************/
plerrcode
sis3316_reset_clock(ml_entry* module, unsigned int clockidx)
{
    struct sis3316_priv_data *priv;
    struct sis3316_clock *clock;
    plerrcode pres=plErr_System;
    ems_u32 addr;

    priv=(struct sis3316_priv_data*)module->private_data;
    clock=priv->clocks+clockidx;

    if (clockidx>3) {
        complain("sis3316_reset_clock: illegal argument");
        return plErr_ArgRange;
    }

    addr=SIS3316_ADC_CLK_OSC_I2C_REG+4*clockidx;

    if ((pres=si750_freezeDCO(module, addr))!=plOK)
        goto error;

    if ((pres=si750_recall(module, addr))!=plOK)
        goto error;

    if ((pres=si750_unfreezeDCO(module, addr))!=plOK)
        goto error;

    if ((pres=si750_newfrequency(module, addr))!=plOK)
        goto error;

    if ((pres=sis3316_sample_clock_changed(module, 125000000))!=plOK)
        goto error;

    clock->initial_clock_valid=0;
    if ((pres=si750_get_initial_clock(module, clockidx))!=plOK)
        goto error;

    pres=plOK;

error:
    I2cStop(module, addr);
    return pres;
}
/*****************************************************************************/
/*
 * clock:
 *     0: sample clock
 *     1: MGT1 clock
 *     2: MGT2 clock
 *     3: DDR3 clock
 * freq:
 *     0: 62.5 MHz
 *     1: 128 MHz
 *     2: 250 MHz
 */
plerrcode
sis3316_set_clock(ml_entry* module, unsigned int clockidx, unsigned int freqidx)
{
    struct sis3316_priv_data *priv;
    struct sis3316_clock *clock;
    u_int8_t regs[6];
    ems_u32 addr;
    int hsdiv, n1, freck;
    plerrcode pres;

    priv=(struct sis3316_priv_data*)module->private_data;
    clock=priv->clocks+clockidx;

    if (clockidx>3 || freqidx>2) {
        complain("sis3316_set_clock: illegal argument(s)");
        return plErr_ArgRange;
    }

    addr=SIS3316_ADC_CLK_OSC_I2C_REG+4*clockidx;

    /* read and store old values if necessary */
    if ((pres=si750_get_initial_clock(module, clockidx))!=plOK)
        return pres;

    switch (freqidx) {
    case 0:
        freck=62500000;
        hsdiv= 5;
        n1   =16;
        break;
    case 1:
         freck=125000000;
       hsdiv= 5;
        n1   = 8;
        break;
    case 2:
        freck=250000000;
        hsdiv= 5;
        n1   = 4;
        break;
    }

    n1-=1;
    hsdiv-=4;

    /*
     * here we should check whether the hsdiv and n1 are allowed:
     * hsdiv: [4, 5, 6, 7, 9, 11]
     * n1:    [1, 2, 4, 6, .... 124, 126]
     * but we use constants, no check necessary
     */
    regs[0]=((hsdiv&0x7)<<5)+((n1&0x7f)>>2);
    regs[1]=((n1&0x3)<<6)+(clock->initial_clock[1]&0x3f);
    regs[2]=clock->initial_clock[2];
    regs[3]=clock->initial_clock[3];
    regs[4]=clock->initial_clock[4];
    regs[5]=clock->initial_clock[5];

    if ((pres=si750_freezeDCO(module, addr))!=plOK)
        return pres;

    if ((pres=si750_set_frequency(module, addr, regs))!=plOK)
        return pres;

    if ((pres=si750_unfreezeDCO(module, addr))!=plOK)
        return pres;

    if ((pres=si750_newfrequency(module, addr))!=plOK)
        return pres;

    pres=sis3316_sample_clock_changed(module, freck);

    return pres;
}
/*****************************************************************************/
static plerrcode
sis3316_set_clock_all(ml_entry* module, unsigned int clockidx, int hsdiv,
        int n1, u_int64_t rfreq)
{
    struct sis3316_priv_data *priv;
    struct sis3316_clock* clock;
    u_int8_t regs[6];
    ems_u32 addr;
    plerrcode pres;
    int freq, i;

    priv=(struct sis3316_priv_data*)module->private_data;
    clock=priv->clocks+clockidx;

    addr=SIS3316_ADC_CLK_OSC_I2C_REG+4*clockidx;

    printf("set_clock_all[%d]: hsdiv=%d n1=%d rfreq=%llu\n",
                clockidx, hsdiv, n1, (unsigned long long int)rfreq);

    n1-=1;
    hsdiv-=4;

    /*
     * here we should check whether the hsdiv and n1 are allowed:
     * hsdiv: [4, 5, 6, 7, 9, 11]
     * n1:    [1, 2, 4, 6, .... 124, 126]
     */
    regs[0]=((hsdiv&0x7)<<5)+((n1&0x7f)>>2);
    regs[1]=((n1&0x3)<<6)+((rfreq>>32)&0x3f);
    regs[2]=(rfreq>>24)&0xff;
    regs[3]=(rfreq>>16)&0xff;
    regs[4]=(rfreq>>8)&0xff;
    regs[5]=rfreq&0xff;

    printf("set_clock_all[%d]: regs to be written:\n", clockidx);
    for (i=0; i<6; i++) {
        printf("[%d] %02x\n", i, regs[i]);
    }

    if ((pres=si750_freezeDCO(module, addr))!=plOK)
        return pres;

    if ((pres=si750_set_frequency(module, addr, regs))!=plOK)
        return pres;

    if ((pres=si750_unfreezeDCO(module, addr))!=plOK)
        return pres;

    if ((pres=si750_newfrequency(module, addr))!=plOK)
        return pres;

    n1+=1;
    hsdiv+=4;

    freq=round(5E9/(n1*hsdiv)*(rfreq/clock->rfreq5g));

    pres=sis3316_sample_clock_changed(module, freq);

    return pres;
}
/*****************************************************************************/
/*
 * this should set the clock generator to an arbitrary frequency
 * The frequency (Fnew) is given in Hz
 */
plerrcode
sis3316_set_clock_any(ml_entry* module, unsigned int clockidx, int Fnew)
{
    struct sis3316_priv_data *priv;
    struct sis3316_clock* clock;
    static const int hsdivs[]={11, 9, 7, 6, 5, 4};
    const int nhsdivs=sizeof(hsdivs)/sizeof(int);
    double dco[nhsdivs], best_dco, rfreqnewf;
    u_int64_t rfreqnew;
    int n1[nhsdivs];
    int pres, i, best_idx;

    printf("Fnew=%d\n", Fnew);

    priv=(struct sis3316_priv_data*)module->private_data;
    clock=priv->clocks+clockidx;

    /* read and store old values if necessary */
    if ((pres=si750_get_initial_clock(module, clockidx))!=plOK)
        return pres;

    for (i=0; i<nhsdivs; i++) {
        double n1f;

        n1f=5E9/(double)Fnew/(double)hsdivs[i];
        /* round to next even number or 1 */
        if (n1f<.5) {
            n1[i]=1;
        } else {
            n1[i]=2*round(n1f/2);
        }
        dco[i]=(double)Fnew*(double)hsdivs[i]*(double)n1[i];

        printf("[%d] hsdiv=%2d n1f=%.6f n1=%3d dco=%.6f\n",
                i, hsdivs[i], n1f, n1[i], dco[i]);
    }

    best_idx=-1;
    for (i=0; i<nhsdivs; i++) {
        if (dco[i]<4.85E9 || dco[i]>5.67E9) /* not allowed */
            continue;
        if (best_idx<0 || dco[i]<best_dco) {
            best_idx=i;
            best_dco=dco[i];
        }
    }

    if (best_idx<0) {
        printf("sis3316_set_clock_any: no valid combination found for %d Hz\n",
                Fnew);
        return plErr_Verify;
    }

    printf("will use combination %d\n", best_idx);

    rfreqnewf=clock->rfreq5g*best_dco/5E9;
    rfreqnew=round(rfreqnewf);

    pres=sis3316_set_clock_all(module, clockidx, hsdivs[best_idx],
            n1[best_idx], rfreqnew);

    return pres;
}
/*****************************************************************************/
plerrcode
sis3316_get_clock(ml_entry* module, unsigned int clockidx, ems_u32* val)
{
    struct sis3316_priv_data *priv;
    struct sis3316_clock* clock;
    u_int8_t regs[6];
    int hsdiv, n1, i;
    u_int64_t rfreq;
    ems_u32 addr;
    plerrcode pres;

    if (clockidx>3) {
        complain("sis3316_get_clock: illegal argument");
        pres=plErr_ArgRange;
        goto error;
    }

    priv=(struct sis3316_priv_data*)module->private_data;
    clock=priv->clocks+clockidx;

    addr=SIS3316_ADC_CLK_OSC_I2C_REG+4*clockidx;
    if ((pres=si750_get_frequency(module, addr, regs))!=plOK)
        goto error;

    printf("\nsis3316_get_clock\n");
    for (i=0; i<6; i++) {
        printf("[%d] %02x\n", i, regs[i]);
    }

    hsdiv=((regs[0]>>5)&7)+4;
    n1=regs[0]&0x1f; /* do not shift up here, reg is 8 bit only */
    n1=((n1<<2)|((regs[1]>>6)&3))+1;
    rfreq=regs[1]&0x3f;
    rfreq<<=8;
    rfreq|=regs[2];
    rfreq<<=8;
    rfreq|=regs[3];
    rfreq<<=8;
    rfreq|=regs[4];
    rfreq<<=8;
    rfreq|=regs[5];
    printf("hsdiv=%d n1=%d rfreq=%llu (%llx)\n",
            hsdiv, n1, (unsigned long long)rfreq, (unsigned long long)rfreq);

    *val=round(5E9/(n1*hsdiv)*rfreq/clock->rfreq5g);

error:
    return pres;
}
/*****************************************************************************/
/*****************************************************************************/
