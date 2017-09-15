/*
 * lowlevel/lvd/trigger/trig_i2c.c
 * created 2010-Feb-21 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: trig_i2c.c,v 1.2 2011/04/06 20:30:27 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <stdio.h>
#include <rcs_ids.h>
#include "../lvdbus.h"
#include "../lvd_access.h"
#include "trig_i2c.h"

RCS_REGISTER(cvsid, "lowlevel/lvd/trigger")

static char*
string_16(ems_u32 *d, int l)
{
    static char s[17];
    int i;
    for (i=0; i<l; i++)
        s[i]=d[i];
    s[i--]='\0';
    while (i>=0 && s[i]==' ')
        s[i--]='\0';
    return s;
}

static void
trig_i2c_decode_0_00(ems_u32 *d)
{
    const char *t="reserved or vendor specific";
    const char *tt[]={
        "unknown or unspecified",
        "GBIC",
        "soldered",
        "SFP"
    };
    if (*d<4)
        t=tt[*d];
    printf("%02x %02x Identifier: %s\n", 0, *d, t);
}
static void
trig_i2c_decode_0_01(ems_u32 *d)
{
    const char *tt[]={
        "non-costume",
        "unknown"
    };
    printf("%02x %02x Ext. Identifier: %s\n", 1, *d, *d==4?tt[0]:tt[1]);
}
static void
trig_i2c_decode_0_02(ems_u32 *d)
{
    const char *t="reserved or vendor specific";
    const char *tt[]={
        "unknown or unspecified",
        "SC",
        "Fibre Channel Style 1 copper",
        "Fibre Channel Style 2 copper",
        "BNC/TNC",
        "Fibre Channel coaxial",
        "FiberJack",
        "LC",
        "MT-RJ",
        "MU",
        "SG",
        "Optical pigtail",
        "reserved", /* 0xc..0x1f */
        "HSSDC II", /* 0x20 */
        "Copper pigtail"
    };
    if (*d<0xc)
        t=tt[*d];
    else if (*d<0x20)
        t=tt[0xc];
    else if (*d<0x22)
        t=tt[*d-0x13];
    printf("%02x %02x Connector: %s\n", 2, *d, t);
}
static void
trig_i2c_decode_0_03(ems_u32 *d)
{
    int i;
    printf("%02x    Transceiver:", 3);
    for (i=0; i<8; i++)
        printf(" %02x", d[i]);
    printf("\n");
}
static void
trig_i2c_decode_0_0b(ems_u32 *d)
{
    const char *t="reserved";
    const char *tt[]={
        "unspecified",
        "8B10B",
        "4B5B",
        "NRZ",
        "Manchester",
        "SONET Scrambled"
    };
    if (*d<6)
        t=tt[*d];
    printf("%02x %02x Encoding: %s\n", 0xb, *d, t);
}
static void
trig_i2c_decode_0_0c(ems_u32 *d)
{
    printf("%02x %02x Nominal Bitrate: ", 0xc, *d);
    if (*d)
        printf("%.1f GBit/s\n", *d/10.);
    else
        printf("not specified\n");
}
static void
trig_i2c_decode_0_0e(ems_u32 *d)
{
    printf("%02x %02x Length (9\265m): ", 0xe, *d);
    if (*d)
        printf("%d km\n", *d);
    else
        printf("not supported or unspecified\n");
}
static void
trig_i2c_decode_0_0f(ems_u32 *d)
{
    printf("%02x %02x Length (9\265m): ", 0xf, *d);
    if (*d)
        printf("%.1f km\n", *d/10.);
    else
        printf("not supported or unspecified\n");
}
static void
trig_i2c_decode_0_10(ems_u32 *d)
{
    printf("%02x %02x Length (50\265m): ", 0x10, *d);
    if (*d)
        printf("%.1f km\n", *d/10.);
    else
        printf("not supported or unspecified\n");
}
static void
trig_i2c_decode_0_11(ems_u32 *d)
{
    printf("%02x %02x Length (62.5\265m): ", 0x11, *d);
    if (*d)
        printf("%.1f km\n", *d/10.);
    else
        printf("not supported or unspecified\n");
}
static void
trig_i2c_decode_0_12(ems_u32 *d)
{
    printf("%02x %02x Length (copper): ", 0x12, *d);
    if (*d)
        printf("%.d m\n", *d);
    else
        printf("not supported or unspecified\n");
}
static void
trig_i2c_decode_0_14(ems_u32 *d)
{
    printf("%02x    Vendor name: %s\n", 0x14, string_16(d, 16));
}
static void
trig_i2c_decode_0_25(ems_u32 *d)
{
    int i;
    printf("%02x    Vendor OUI:", 0x25);
    for (i=0; i<3; i++)
        printf(" %02x", d[i]);
    printf("\n");
}
static void
trig_i2c_decode_0_28(ems_u32 *d)
{
    printf("%02x    Vendor PN: %s\n", 0x28, string_16(d, 16));
}
static void
trig_i2c_decode_0_38(ems_u32 *d)
{
    printf("%02x    Vendor Rev: %s\n", 0x38, string_16(d, 4));
}
static void
trig_i2c_decode_0_3c(ems_u32 *d)
{
    printf("%02x    Wavelength: %d nm\n", 0x3c, d[1]+(d[0]<<8));
}
static void
trig_i2c_decode_0_40(ems_u32 *d)
{
    int i;
    printf("%02x    Options:", 0x40);
    for (i=0; i<2; i++)
        printf(" %02x", d[i]);
    printf("\n");
}
static void
trig_i2c_decode_0_42(ems_u32 *d)
{
    printf("%02x %02x max. BitRate: ", 0x42, *d);
    if (*d)
        printf("+%d%%\n", *d);
    else
        printf("not specified\n");
}
static void
trig_i2c_decode_0_43(ems_u32 *d)
{
    printf("%02x %02x min. BitRate: ", 0x43, *d);
    if (*d)
        printf("-%d%%\n", *d);
    else
        printf("not specified\n");
}
static void
trig_i2c_decode_0_44(ems_u32 *d)
{
    printf("%02x    Vendor SN: %s\n", 0x44, string_16(d, 16));
}
static void
trig_i2c_decode_0_54(ems_u32 *d)
{
    char s[14];
    s[ 0]='2';
    s[ 1]='0';
    s[ 2]=d[0];
    s[ 3]=d[1];
    s[ 4]='-';
    s[ 5]=d[2];
    s[ 6]=d[3];
    s[ 7]='-';
    s[ 8]=d[4];
    s[ 9]=d[5];
    s[10]=' ';
    s[11]=d[6];
    s[12]=d[7];
    s[13]='\0';
    printf("%02x    Date: %s\n", 0x54, s);
}
static void
trig_i2c_decode_0_5c(ems_u32 *d)
{
    printf("%02x %02x Diagnostic Monitoring Type:\n", 0x5c, *d);
    if (*d&0x04)
        printf("        Address change required\n");
    printf("        Received power measurement type: %s\n",
            *d&0x08?"Average Power":"OMA");
    if (*d&0x10)
        printf("        Externally calibrated\n");
    if (*d&0x20)
        printf("        Internally calibrated\n");
    if (*d&0x40)
        printf("        Digital diagnostic monitoring implemented\n");
    if (*d&0x80)
        printf("        Legacy diagnostic implementation\n");
}
static void
trig_i2c_decode_0_5d(ems_u32 *d)
{
    printf("%02x %02x Enhanced Options: %02x\n", 0x5d, *d, *d);
}
static void
trig_i2c_decode_0_5e(ems_u32 *d)
{
    printf("%02x %02x SFF-8472 Compliance: %02x\n", 0x5e, *d, *d);
}
static void
trig_i2c_decode_1_60(ems_u32 *d)
{
    ems_u16 u;
    ems_i16 s;
    u=d[1]+(d[0]<<8);
    s=u;
    printf("%02x    Temperature: %f \260C\n", 0x60, s/256.);
}
static void
trig_i2c_decode_1_62(ems_u32 *d)
{
    printf("%02x    Voltage: %f V\n", 0x62, (d[1]+(d[0]<<8))/10000.);
}
static void
trig_i2c_decode_1_64(ems_u32 *d)
{
    printf("%02x    Bias current: %f mA\n", 0x64, (d[1]+(d[0]<<8))/500.);
}
static void
trig_i2c_decode_1_66(ems_u32 *d)
{
    printf("%02x    TX power: %f mW\n", 0x66, (d[1]+(d[0]<<8))/10000.);
}
static void
trig_i2c_decode_1_68(ems_u32 *d)
{
    printf("%02x    RX power: %f mW\n", 0x68, (d[1]+(d[0]<<8))/10000.);
}

void
trig_i2c_decode_mem(ems_u32 *d)
{
    ems_u32 cc_ext=0, cc_base=0;
    int i;

    for (i=0; i<=62; i++)
        cc_base+=d[i];
    for (i=64; i<=94; i++)
        cc_ext+=d[i];
    printf("trig_i2c data of transceiver XXX (base %svalid, ext %svalid):\n",
        (cc_base&0xff)==d[63]?"":"in", (cc_ext&0xff)==d[95]?"":"in");

    trig_i2c_decode_0_00(d+0);
    trig_i2c_decode_0_01(d+1);
    trig_i2c_decode_0_02(d+2);
    trig_i2c_decode_0_03(d+3);
    trig_i2c_decode_0_0b(d+11);
    trig_i2c_decode_0_0c(d+12);
    trig_i2c_decode_0_0e(d+14);
    trig_i2c_decode_0_0f(d+15);
    trig_i2c_decode_0_10(d+16);
    trig_i2c_decode_0_11(d+17);
    trig_i2c_decode_0_12(d+18);
    trig_i2c_decode_0_14(d+20);
    trig_i2c_decode_0_25(d+37);
    trig_i2c_decode_0_28(d+40);
    trig_i2c_decode_0_38(d+56);
    trig_i2c_decode_0_3c(d+60);

    trig_i2c_decode_0_40(d+64);
    trig_i2c_decode_0_42(d+66);
    trig_i2c_decode_0_43(d+67);
    trig_i2c_decode_0_44(d+68);
    trig_i2c_decode_0_54(d+84);
    trig_i2c_decode_0_5c(d+92);
    trig_i2c_decode_0_5d(d+93);
    trig_i2c_decode_0_5e(d+94);
}

void
trig_i2c_decode_dia(ems_u32 *d)
{
    printf("trig_i2c diagnostics of transceiver XXX:\n");
    trig_i2c_decode_1_60(d+96);
    trig_i2c_decode_1_62(d+98);
    trig_i2c_decode_1_64(d+100);
    trig_i2c_decode_1_66(d+102);
    trig_i2c_decode_1_68(d+104);
}

static plerrcode
wait_for_busy(struct i2c_addr* i2c)
{
    ems_u32 val;
    int n=50000;
    do {
        if (i2c->dev->read(i2c->dev, i2c->csr, 2, &val)<0) {
            printf("trig_i2c.c: error in wait_for_busy");
            return plErr_HW;
        }
        if (val&LVD_I2C_ERROR) {
            printf("trig_i2c.c: error in wait_for_busy: csr=0x%x\n", val);
            return plErr_HW;
        }
    } while (n-- && (val&LVD_I2C_BUSY));
    if (val&LVD_I2C_BUSY) {
        printf("trig_i2c.c: timeout in wait_for_busy: csr=0x%x\n", val);
        return plErr_Timeout;
    }
    return plOK;
}
static plerrcode
wait_for_ready(struct i2c_addr* i2c)
{
    ems_u32 val;
    int n=50000;
    do {
        if (i2c->dev->read(i2c->dev, i2c->csr, 2, &val)<0) {
            printf("trig_i2c.c: error in wait_for_ready\n");
            return plErr_HW;
        }
        if (val&LVD_I2C_ERROR) {
            printf("trig_i2c.c: error in wait_for_ready: csr=0x%x\n", val);
            return plErr_HW;
        }
    } while (n-- && !(val&LVD_I2C_DREADY));
    if (!(val&LVD_I2C_DREADY)) {
        printf("trig_i2c.c: timeout in wait_for_ready: csr=0x%x\n", val);
        return plErr_Timeout;
    }
    return plOK;
}

plerrcode
trig_i2c_read(struct i2c_addr* i2c, int region, int addr, int num,
    ems_u32 *ptr)
{
    plerrcode pres;
    ems_u32 val;
    //int max=128;
    int max=64;

    /* only 128 byte can be read in a single action */
    while (num) {
        int xnum=num;
        if (xnum>max)
            xnum=max;
        num-=xnum;

        /* reset (clear fifo) */
        if (i2c->dev->write(i2c->dev, i2c->csr, 2, LVD_I2C_RESET)<0)
            return plErr_HW;
        if ((pres=wait_for_busy(i2c))!=plOK) {
            printf("trig_i2c.c:   error after reset\n");
            return pres;
        }

        /* write read request */
        val=(addr&0xff) | ((xnum+1)<<8) | (region?0x8000:0);
        addr+=xnum;
        if (i2c->dev->write(i2c->dev, i2c->rd, 2, val)<0)
            return plErr_HW;
        if ((pres=wait_for_busy(i2c))!=plOK) {
            printf("trig_i2c.c:   error after writing read request\n");
            return pres;
        }

        /* wait for data and read fifo */
        for (; xnum; xnum--) {
            if ((pres=wait_for_ready(i2c))!=plOK) {
                printf("trig_i2c.c:   xnum=%d addr=%d\n", xnum, addr);
                return pres;
            }
            if (i2c->dev->read(i2c->dev, i2c->data, 2, &val)<0)
                return plErr_HW;
            //printf("got %02x\n", val);
            *ptr++=val;
            val&=0xff;
        }
        if ((pres=wait_for_busy(i2c))!=plOK) {
            printf("trig_i2c.c:   error after read\n");
            return pres;
        }
    }
    return plOK;
}

#if 0
plerrcode
trig_i2c_write(struct lvd_dev* dev, int port, int region, int addr, int num,
        ems_u32 *data)
{
    printf("trig_i2c_write is not implemented\n");
    return plErr_NoSuchProc;
}
#endif

#if 0
plerrcode
trig_i2c_dump(struct lvd_dev* dev, int port)
{
    plerrcode pres;
    ems_u32 d0[256];
    ems_u32 d1[256];

    pres=trig_i2c_read(dev, port, 0, 0, 256, d0);
    if (pres!=plOK)
        return pres;
    pres=trig_i2c_read(dev, port, 1, 0, 256, d1);
    if (pres!=plOK)
        return pres;

    trig_i2c_decode_mem(d0, port);
    trig_i2c_decode_dia(d1, port);

    return plOK;
}
#endif
