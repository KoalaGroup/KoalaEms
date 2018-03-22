/*
 * procs/unixvme/sis3316/sis3316.c
 * created 2015-Mar-11 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3316.c,v 1.14 2017/12/05 16:34:44 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/oscompat/oscompat.h"
#include "../../../lowlevel/unixvme/vme.h"
#include "../../../lowlevel/devices.h"
#include "../../../lowlevel/ipmod/ipmodules.h"
#include "../vme_verify.h"
#include "sis3316.h"

RCS_REGISTER(cvsid, "procs/unixvme/sis3316")

/*
 * SIS3316 125/250 MHz 16 channel FADC
 * A32D32/BLT32/MBLT64
 * occupies 16777216 Byte of address space (24 bit)
 */
/*
 *
0x000000..0x00001C R/W VME FPGA interface registers
0x000020..0x0000FC R/W VME FPGA registers
0x000400..0x00043C   W VME FPGA key addresses (with Broadcast functionality)

0x001000..0x001FFC R/W ADC FPGA 1: ch1-ch4 registers
0x002000..0x002FFC R/W ADC FPGA 2: ch5-ch8 registers
0x003000..0x003FFC R/W ADC FPGA 3: ch9-ch12 registers
0x004000..0x004FFC R/W ADC FPGA 4: ch13-ch16 registers

0x100000..0x1FFFFC R/W ADC FPGA 1: ch1-ch4 Memory Data FIFO
0x200000..0x2FFFFC R/W ADC FPGA 2: ch5-ch8 Memory Data FIFO
0x300000..0x3FFFFC R/W ADC FPGA 3: ch9-ch12 Memory Data FIFO
0x400000..0x4FFFFC R/W ADC FPGA 4: ch13-ch16 Memory Data FIFO

VME FPGA interface registers
0x00000000 R/W Control/Status Register (J-K register)
0x00000004 R   Module Id. and Firmware Revision register
0x00000008 R/W Interrupt configuration register
0x0000000C R/W Interrupt control register

0x00000010 R/W Interface Access Arbitration control/status register
0x00000014 R/W CBLT/Broadcast Setup register
0x00000018 R/W Internal test
0x0000001C R/W Hardware Version register

VME FPGA registers
0x00000020 R   Temperature register
0x00000024 R/W Onewire control/status register (EEPROM DS2430)
0x00000028 R   Serial number register
0x0000002C R/W Internal data transfer speed setting register
 */

/*
UDP VME/CTRL FPGA Link interface register space
00 W/R Control/Status Register (J-K register)
04 R only Module Id. and Firmware Revision register
08 W/R Interrupt configuration register
0c W/R Interrupt control register
10 W/R Interface Access Arbitration control/status register
14 W/R CBLT/Broadcast Setup register
18 W/R Internal test
1c W/R Hardware Version register
*/

/*
___ memory considerations for ems server ___

one module: 2GByte pro module (512 MByte per chanel group)
  halve of the memory can be read out for each meta event (double buffering)
  each cluster should have space for at least one complete event
  we should have memory for several clusters

  5 modules --> 5 GByte for each event
    --> more than 5 GByte for each cluster
  space for example for 4 clusters: more than 20 GByte

one module: max. 1 GBit/s
  5 modules --> 5 GBit/s == 625 MByte/s + overhead
*/


#define xstr(s) str(s)
#define str(s) #s
#define SIS3316PORT 0xE000

/* sequence is the sequence number for all UDP requests */
static ems_u8 sequence=0;
/* buffer_??t is used for UDP FIFO read and write and VME FIFO write
   its size (for UDP) is at least padding+header+(2048 words) --> 8196 Bytes
   (one byte more as needed for sanity check; data words aligned)
   in case of multiple packets per request each buffer has to be copied
   to the dataout before reading the next buffer */

#define MTU 9000                          /* one jumbo frame */
static u_int32_t buffer_32t[(MTU+3)/4+1]; /* + 1 word padding */
static u_int8_t *buffer_8t=(u_int8_t*)buffer_32t;

static struct timeval tv0;

/*****************************************************************************/
/*
 * sis3316 UDP interface requires words to be sent and received
 * in little endian order :-(
 */
static int
bytes2word(uint8_t b[4], uint32_t *v)
{
    int i;
    uint32_t v_=0;
    for (i=3; i>=0; i--) {
        v_<<=8;
        v_|=b[i];
    }
    *v=v_;
    return 4;
}

static int
word2bytes(uint8_t b[4], uint32_t v)
{
    int i;
    for (i=0; i<4; i++) {
        b[i]=v&0xff;
        v>>=8;
    }
    return 4;
}

static int
short2bytes(uint8_t b[2], uint16_t v)
{
    int i;
    for (i=0; i<2; i++) {
        b[i]=v&0xff;
        v>>=8;
    }
    return 2;
}
/*****************************************************************************/
static plerrcode
sis3316_init_udp(ml_entry* module)
{
    plerrcode pres;

    /* apply default values, if necessary */
    if (!module->address.ip.protocol) {
        module->address.ip.protocol=strdup("UDP");
    }
    if (!module->address.ip.rserv) {
        module->address.ip.rserv=strdup(xstr(SIS3316PORT));
    }
    if (!module->address.ip.lserv) {
        module->address.ip.lserv=strdup(xstr(SIS3316PORT));
    }
    module->address.ip.recvtimeout=50; /* 50 ms */

    pres=ipmod_init(module);

    return pres;
}
/*****************************************************************************/
static plerrcode
sis3316_vme_read_ctrl(ml_entry* module, ems_u32 reg, ems_u32* value)
{
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;

    if (dev->read_a32d32(dev, addr+reg, value)!=4) {
        printf("sis3316_vme_read(0x%08x) failed: %s\n",
                addr+reg, strerror(errno));
        return plErr_System;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
sis3316_udp_read_ctrl(ml_entry* module, ems_u32 reg, ems_u32* value)
{
    int protocol=((struct sis3316_priv_data*)module->private_data)->protocol;
    struct ipsock *sock=module->address.ip.sock;
    struct sockaddr src_addr;
    socklen_t addrlen;
    int idx, res, expected;

    uint8_t req[6];
    uint8_t ans[11];

    idx=0;
    req[idx++]=0x10;
    if (protocol>1)
        req[idx++]=++sequence;
    word2bytes(req+idx, reg);
    idx+=4;

#if 0
    {
        int i;
        printf("udp_read_ctrl(%04x):\n", reg);
        printf("  request:\n");
        for (i=0; i<idx; i++)
            printf("  [%d] %02x\n", i, req[i]);
    }
#endif

#if 0
    printf("sis3316 sendto(p=%d req=%p len=%d flags=0x%x addrlen=%d)\n",
            sock->p, req, idx, 0, sock->addrlen);
#endif
    res=sendto(sock->p, req, idx, 0, sock->addr, sock->addrlen);
    if (res!=idx) {
        printf("sis3316 udp_read:sendto: res=%d error=%s\n",
                res, strerror(errno));
        return plErr_System;
    }

    expected=protocol>1?10:9;
    //res=recv(sock->p, ans, expected+1, 0);
    addrlen=sizeof(src_addr);
    res=recvfrom(sock->p, ans, expected+1, 0, &src_addr, &addrlen);
#if 0
    printf("sis3316 udp_read_ctrl:recvfrom: addrlen=%d\n", addrlen);
    print_sockaddr(&src_addr);
#endif
#if 0
    if (res>=0) {
        int i;
        printf("  answer: res=%d\n", res);
        for (i=0; i<res; i++)
            printf("  [%d] %02x\n", i, ans[i]);
    }
#endif

    if (res!=expected) {
        plerrcode pres=plErr_System;
        if (res<0 && errno==EAGAIN)
            pres=plErr_Timeout;
        printf("sis3316 udp_read:recv: res=%d error=%s\n",
                res, strerror(errno));
        return pres;
    }

    idx=protocol>1?6:5;
    bytes2word(ans+idx, value);

    return plOK;
}
/*****************************************************************************/
static plerrcode
sis3316_udp_check_protocol(ml_entry* module)
{
    struct sis3316_priv_data *priv=
            (struct sis3316_priv_data*)module->private_data;
    struct ipsock *sock=module->address.ip.sock;
    int i, res;

    /* read ctrl reg. 4 */
    uint8_t req_0[]={0x10, 0x04, 0x00, 0x00, 0x00};
    uint8_t req_2[]={0x10, 0x01, 0x04, 0x00, 0x00, 0x00};
    uint8_t ans[11];

    /* check for version 0 */
#if 0
printf("sis3316_udp_check_protocol(0):will send\n");
for (i=0; i<(ssize_t)sizeof(req_0); i++)
    printf(" %02x", req_0[i]);
printf("\n");
#endif
    res=sendto(sock->p, req_0, sizeof(req_0), 0, sock->addr, sock->addrlen);
    if (res!=sizeof(req_0)) {
        printf("sis3316 check_protocol(0):sendto: res=%d error=%s\n",
                res, strerror(errno));
        return plErr_System;
    }

    res=recv(sock->p, ans, 11, 0);
    if (res<0) {
        plerrcode pres=plErr_System;
        if (errno==EAGAIN)
            pres=plErr_Timeout;
        printf("sis3316 check_protocol(0):recv: error=%s\n", strerror(errno));
        return pres;
    }

#if 0
printf("received %d bytes:\n", res);
for (i=0; i<res; i++)
    printf(" %02x", ans[i]);
printf("\n");
#endif

    if (res==9) { /* protocol 0 */
        u_int32_t v;
        bytes2word(ans+1, &v);
        /* check returned constants */
        if (ans[0]!=0x10 || v!=4) {
            printf("sis3316 check_protocol(0): unexpected answer:\n");
            for (i=0; i<9; i++)
                printf(" %02x", ans[i]);
            printf("\n");
            return plErr_HWTestFailed;
        }
        bytes2word(ans+5, &priv->module_id);
        printf("sis3316 check_protocol: set protocol to 0\n");
        priv->protocol=0;
        return plOK;
    }
    /* res!=9: must be a different protocol */


    /* check for version 2 */
#if 0
printf("sis3316_udp_check_protocol(2):will send\n");
for (i=0; i<(ssize_t)sizeof(req_2); i++)
    printf(" %02x", req_2[i]);
printf("\n");
#endif

    res=sendto(sock->p, req_2, sizeof(req_2), 0, sock->addr, sock->addrlen);
    if (res!=sizeof(req_2)) {
        printf("sis3316 check_protocol(2):sendto: res=%d error=%s\n",
                res, strerror(errno));
        return plErr_System;
    }

    res=recv(sock->p, ans, 11, 0);
    if (res<0) {
        plerrcode pres=plErr_System;
        if (errno==EAGAIN)
            pres=plErr_Timeout;
        printf("sis3316 check_protocol(2):recv: error=%s\n", strerror(errno));
        return pres;
    }

#if 0
printf("received %d bytes:\n", res);
for (i=0; i<res; i++)
    printf(" %02x", ans[i]);
printf("\n");
#endif

    if (res==10) { /* protocol 0 */
        u_int32_t v;
        bytes2word(ans+2, &v);
        /* check returned constants */
        if (ans[0]!=0x10 || ans[1]!=1 || v!=4 ) {
            printf("sis3316 check_protocol(2): unexpected answer:\n");
            for (i=0; i<10; i++)
                printf(" %02x", ans[i]);
            printf("\n");
            return plErr_HWTestFailed;
        }
        bytes2word(ans+6, &priv->module_id);
        printf("sis3316 check_protocol: set protocol to 2\n");
        priv->protocol=2;
        return plOK;
    }

    printf("sis3316 check_protocol(2): wrong length of answer: "
            "%d instead of 10:\n", res);
    for (i=0; i<10; i++)
        printf(" %02x", ans[i]);
    printf("\n");

    return plErr_HWTestFailed;
}
/*****************************************************************************/
static plerrcode
sis3316_vme_write_ctrl(ml_entry* module, ems_u32 reg, ems_u32 value)
{
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;

    if (dev->write_a32d32(dev, addr+reg, value)!=4) {
        printf("sis3316_vme_write(0x%08x, x%08x) failed: %s\n",
                addr+reg, value, strerror(errno));
        return plErr_System;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
sis3316_udp_write_ctrl(ml_entry* module, ems_u32 reg, ems_u32 value)
{
    struct ipsock *sock=module->address.ip.sock;
    int idx, res;

    uint8_t req[10];

    idx=0;
    req[idx++]=0x11;
    word2bytes(req+idx, reg);
    idx+=4;
    word2bytes(req+idx, value);
    idx+=4;

#if 0
    {
        int i;
        printf("sis3316_udp_write_ctrl(%08x, %08x)\n", reg, value);
        for (i=0; i<idx; i++)
            printf("%02x ", req[i]);
        printf("\n");
    }
#endif

    res=sendto(sock->p, req, idx, 0,
            sock->addr, sock->addrlen);
    if (res!=idx) {
        printf("sis3316 udp sendto: res=%d error=%s\n", res, strerror(errno));
        return plErr_System;
    }

    return plOK;
}
/*****************************************************************************/
static plerrcode
sis3316_vme_read_reg(ml_entry* module, ems_u32 reg, ems_u32* value)
{
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;

    if (dev->read_a32d32(dev, addr+reg, value)!=4) {
        printf("sis3316_vme_read_reg(0x%08x) failed: %s\n",
                addr+reg, strerror(errno));
        return plErr_System;
    }
#if 0
    printf("sis3316_vme_read_reg(%08x/%4x): %08x\n", addr, reg, *value);
#endif
    return plOK;
}
/*****************************************************************************/
/*****************************************************************************/
static int
find_mod_by_addr(ml_entry** modules, struct sockaddr *addr, int nummod)
{
    ml_entry *mod;
    uint32_t searched_for;
    uint32_t to_compare;
    int i;

    /* we assume IPv4 only */
    searched_for=((struct sockaddr_in*)addr)->sin_addr.s_addr;

    printf("we search for %08x\n", searched_for);

    for (i=0; i<nummod; i++) {
        mod=modules[i];
        to_compare=
((struct sockaddr_in*)(mod->address.ip.sock->addr))->sin_addr.s_addr;
        printf("we compare with %08x\n", to_compare);
        if (searched_for==to_compare)
            return i;
    }
    return -1;
}
/*****************************************************************************/
/*
 * sis3316_udp_read_reg_multi reads one register of many modules
 * all have to use the same UDP socket
 * At first all requests are sent, then all answers are received
 */
static plerrcode
sis3316_udp_read_reg_multi(ml_entry** modules, ems_u32 reg, ems_u32* values,
    int nummod)
{
    uint8_t buf[10]; /* code, (seq,) num (2 bytes), addr_or_value +++ */
    int p, modidx, idx, res, i;
    struct sockaddr src_addr;
    socklen_t addrlen;
    ml_entry *mod;
    plerrcode pres=plErr_System;

    if (!nummod) {
        complain("sis3316 udp_read_reg_multi: nummod=0");
        return plErr_Program;
    }

    /* the same socket for ALL modules! */
    p=modules[0]->address.ip.sock->p; /* the same socket for ALL modules! */
    for (i=0; i<nummod; i++) {
        mod=modules[i];
        idx=0;
        buf[idx++]=0x20;
        if (((struct sis3316_priv_data*)mod->private_data)->protocol>1)
            buf[idx++]=++sequence;
        idx+=short2bytes(buf+idx, 0); /* N-1 */
        idx+=word2bytes(buf+idx, reg);
        res=sendto(p, buf, idx, 0, mod->address.ip.sock->addr,
                mod->address.ip.sock->addrlen);
        if (res!=idx) {
            complain("sis3316 udp_read_reg_multi(ser=%d):sendto: %s",
                ((struct sis3316_priv_data*)mod->private_data)->serial,
                strerror(errno));
            goto error;
        }
    }

    for (i=0; i<nummod; i++) {
        addrlen=sizeof(src_addr);
        res=recvfrom(p, buf, 10, 0, &src_addr, &addrlen);
        if (res<0) {
            complain("sis3316 udp_read_reg_multi:recvfrom: %s",
                strerror(errno));
            goto error;
        }
        printf("sis3316 udp_read_reg_multi:recvfrom: addrlen=%d\n", addrlen);
        print_sockaddr(&src_addr); printf("\n");

        modidx=find_mod_by_addr(modules, &src_addr, nummod);
        if (modidx<0) {
            complain("sis3316 udp_read_reg_multi: address not found: ");
            goto error;
        }

        idx=1; /* skip command code */
        if (((struct sis3316_priv_data*)mod->private_data)->protocol>1)
            idx++; /* skip sequence number */

        /* we should check the number of bytes received */

        /* check status */
        if (buf[idx]&0x70) { /* some error */
            mod=modules[modidx];
            printf("sis3316 udp_read_reg_multi(ser=%d): status=0x%02x\n",
                ((struct sis3316_priv_data*)mod->private_data)->serial,
                buf[idx]);
            if (buf[idx]&0x10)
                printf(" no grant");
            if (buf[idx]&0x20)
                printf(" access timeout");
            if (buf[idx]&0x40)
                printf(" protocol error");
            printf("\n");
            goto error;
        }
        idx++;
        bytes2word(buf+idx, values+modidx);
    }

    pres=plOK;
error:
    return pres;
}
/*****************************************************************************/
/*
 * sis3316_udp_read_regs_multi reads multiple register of many modules
 * all have to use the same UDP socket
 * At first all requests are sent, then all answers are received
 * values is a list of pointers to ems_u32*
 */
static plerrcode
sis3316_udp_read_regs_multi(__attribute__((unused)) ml_entry** modules, __attribute__((unused)) const ems_u32 *regs,
    __attribute__((unused)) ems_u32** values,
    __attribute__((unused)) int nummod, __attribute__((unused)) int numregs)
{
#if 0
XXXX
#endif
return plErr_NotImpl;
}
/*****************************************************************************/
/*****************************************************************************/
static plerrcode
sis3316_udp_read_reg(ml_entry* module, ems_u32 reg, ems_u32* value)
{
    int protocol=((struct sis3316_priv_data*)module->private_data)->protocol;
    struct ipsock *sock=module->address.ip.sock;
    uint8_t sbuf[8]; /* code, (seq,) num (2 bytes), addr */
    uint8_t rbuf[8]; /* code, (seq,) state, value */
    int sidx, ridx, res, expected, minrecv, trycnt, dont_sent, i;
    plerrcode pres=plErr_System;

    sidx=0;
    sbuf[sidx++]=0x20;
    if (protocol>1)
        sbuf[sidx++]=++sequence;
    sidx+=short2bytes(sbuf+sidx, 0); /* N-1 */
    sidx+=word2bytes(sbuf+sidx, reg);

#if 0
    printf("sis3316 udp_read_reg(p=%d): before send:\n", protocol);
    printf("idx=%d\n    ", idx);
    for (i=0; i<idx; i++)
        printf("%02x ", buf[i]);
    printf("\n");
#endif

    if (protocol>1) {
        expected=3+4; /* code, seq, status, data */
        minrecv=3;
        ridx=2;
    } else {
        expected=2+4; /* code, status, data */
        minrecv=2;
        ridx=1;
    }

   /*
    * give us several tries:
    *     send request
    *     try to receive answer
    *     if recv failes with EAGAIN --> send request again
    *     if sequence number is too small --> discard answer,
    *         wait for the next answer
    *     repeat this ten(?) times
    */
    trycnt=0;
    dont_sent=0;
    do {
        if (trycnt++>10) {
            complain("sis3316 udp_read_reg: no success after 10 tries");
            goto error;
        }

        if (!dont_sent) {
            res=sendto(sock->p, sbuf, sidx, 0, sock->addr, sock->addrlen);
            if (res!=sidx) {
                complain("sis3316 udp_read_reg:sendto: res=%d error=%s",
                        res, strerror(errno));
                goto error;
            }
        } else {
            dont_sent=0;
        }

        res=recv(sock->p, rbuf, expected+1, 0);
        if (res<0) {
            if (errno==EAGAIN) {
                complain("sis3316 udp_read_reg(ser %d, reg 0x%x):recv: "
                        "res=%d error=%s",
                    ((struct sis3316_priv_data*)module->private_data)->serial,
                    reg, res, strerror(errno));
                continue;
            } else {
                complain("sis3316 udp_read_reg(ser %d, reg 0x%x):recv: "
                        "res=%d error=%s",
                    ((struct sis3316_priv_data*)module->private_data)->serial,
                    reg, res, strerror(errno));
                goto error;
            }
        }

        /* check sequence before other tests */
        if (protocol>1) {
            if (res<2) {
                complain("sis3316 udp_read_reg(ser %d, reg 0x%x):recv: "
                        "res=%d, no seq nr.",
                    ((struct sis3316_priv_data*)module->private_data)->serial,
                    reg, res);
                goto error;
            }
            if (rbuf[1]<sequence) {
                complain("sis3316 udp_read_reg(ser %d, reg 0x%x): wrong seq: "
                        "%d instead of %d",
                    ((struct sis3316_priv_data*)module->private_data)->serial,
                        reg, rbuf[1], sequence);
                dont_sent=1;
                continue;
            }
            if (rbuf[1]!=sequence) {
                complain("sis3316 udp_read_reg(ser %d, reg 0x%x): wrong seq: "
                        "%d instead of %d",
                    ((struct sis3316_priv_data*)module->private_data)->serial,
                        reg, rbuf[1], sequence);
                continue;
            }
        }

        /* here we have the answer with the correct sequence number
           or we don't know the sequence number (old protocol) */
        
        if (res<minrecv) {
            complain("sis3316 udp_read_reg(ser %d, reg 0x%x):recv: "
                    "res=%d; %d required",
                    ((struct sis3316_priv_data*)module->private_data)->serial,
                    reg, res, minrecv);
            goto error;
        }

        if (rbuf[ridx]&0x70) { /* some error */
            printf("sis3316 udp_read_reg: status=0x%02x\n", rbuf[ridx]);
            if (rbuf[ridx]&0x10)
                printf(" no grant");
            if (rbuf[ridx]&0x20)
                printf(" access timeout");
            if (rbuf[ridx]&0x40)
                printf(" protocol error");
            printf("\n");
            goto error;
        }

        if (res!=expected) {
            printf("sis3316 udp_read_reg: read %d bytes instead of %d\n",
                res, expected);
            for (i=0; i<res; i++)
                printf("%02x ", rbuf[i]);
            printf("\n");
            goto error;
        }

        ridx++; /* set ridx to data */
        bytes2word(rbuf+ridx, value);
//printf("reg 0x%x: %08x\n", reg, *value);
        pres=plOK;
        break;

    } while (1);

error:
    return pres;
}
/*****************************************************************************/
static plerrcode
sis3316_vme_write_reg(ml_entry* module, ems_u32 reg, ems_u32 value)
{
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;

    if (dev->write_a32d32(dev, addr+reg, value)!=4) {
        printf("sis3316_vme_write_reg(0x%08x, x%08x) failed: %s\n",
                addr+reg, value, strerror(errno));
        return plErr_System;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
sis3316_udp_write_reg(ml_entry* module, ems_u32 reg, ems_u32 value)
{
    int protocol=((struct sis3316_priv_data*)module->private_data)->protocol;
    struct ipsock *sock=module->address.ip.sock;
    uint8_t sbuf[12]; /* code, (seq,) num (2 bytes), addr, value */
    uint8_t rbuf[4];  /* code, (seq,) state */
    int sidx, res, expected, trycnt, dont_sent, i;
    ems_u32 status;
    plerrcode pres=plErr_System;

    sidx=0;
    sbuf[sidx++]=0x21;
    if (protocol>1)
        sbuf[sidx++]=++sequence;
    sidx+=short2bytes(sbuf+sidx, 0); /* N-1 */
    sidx+=word2bytes(sbuf+sidx, reg);
    sidx+=word2bytes(sbuf+sidx, value);

#if 0
    printf("sis3316 udp_write_reg(p=%d): before send:\n", protocol);
    printf("idx=%d\n", sidx);
    for (i=0; i<sidx; i++)
        printf("%02x ", sbuf[si]);
    printf("\n");
#endif

    trycnt=0;
    dont_sent=0;
    do {
        if (trycnt++>10) {
            complain("sis3316 udp_read_reg: no success after 10 tries");
            goto error;
        }

        if (!dont_sent) {
            res=sendto(sock->p, sbuf, sidx, 0, sock->addr, sock->addrlen);
            if (res!=sidx) {
                complain("sis3316 udp_write_reg:sendto: res=%d error=%s",
                        res, strerror(errno));
                goto error;
            }
        } else {
            dont_sent=0;
        }

        expected=protocol>1?3:2;
        res=recv(sock->p, rbuf, expected+1, 0);
        if (res<0) {
            complain("sis3316 udp_write_reg(ser %d, reg 0x%x):recv: "
                    "res=%d error=%s",
                ((struct sis3316_priv_data*)module->private_data)->serial,
                reg, res, strerror(errno));
            if (errno==EAGAIN)
                continue;
            else
                goto error;
        }

#if 0
        printf("sis3316 udp_write_reg(p=%d): after recv:\n", protocol);
        printf("res=%d\n", res);
        for (i=0; i<res; i++)
            printf("%02x ", rbuf[i]);
        printf("\n");
#endif

        /* check sequence before other tests */
        if (protocol>1) {
            if (res<2) {
                complain("sis3316 udp_write_reg(ser %d, reg 0x%x):recv: "
                        "res=%d, no seq nr.",
                    ((struct sis3316_priv_data*)module->private_data)->serial,
                    reg, res);
                goto error;
            }
            if (rbuf[1]<sequence) {
                complain("sis3316 udp_write_reg(ser %d, reg 0x%x): wrong seq: "
                        "%d instead of %d",
                    ((struct sis3316_priv_data*)module->private_data)->serial,
                        reg, rbuf[1], sequence);
                dont_sent=1;
                continue;
            }
            if (rbuf[1]!=sequence) {
                complain("sis3316 udp_write_reg(ser %d, reg 0x%x): wrong seq: "
                        "%d instead of %d",
                    ((struct sis3316_priv_data*)module->private_data)->serial,
                        reg, rbuf[1], sequence);
                continue;
            }
        }

        if (res!=expected) {
            complain("sis3316 udp_write_reg(ser %d, reg 0x%x):recv: "
                    "res=%d, but %d expected",
                ((struct sis3316_priv_data*)module->private_data)->serial,
                reg, res, expected);
            goto error;
        }

        status=rbuf[protocol>1?2:1];
        if (status&0x70) {
            for (i=0; i<res; i++)
                printf("%02x ", rbuf[i]);
            printf("\n");
            printf("sis3316 udp_write_reg: status=0x%02x\n", status);
            if (status&0x10)
                printf(" no grant");
            if (status&0x20)
                printf(" access timeout");
            if (status&0x40)
                printf(" protocol error");
            printf("\n");
            goto error;
        }

        pres=plOK;
        break;

    } while (1);

    pres=plOK;

error:
    return pres;
}
/*****************************************************************************/
static plerrcode
sis3316_vme_read_regs(ml_entry* module, ems_u32 *regs, ems_u32* values, int num)
{
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
    int i;

    for (i=0; i<num; i++) {
        if (dev->read_a32d32(dev, addr+regs[i], values+i)!=4) {
            printf("sis3316_vme_read_regs(0x%08x) failed: %s\n",
                addr+regs[i], strerror(errno));
            return plErr_System;
        }
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
sis3316_udp_read_regs(ml_entry* module, ems_u32 *regs, ems_u32* values, int num)
{
    int protocol=((struct sis3316_priv_data*)module->private_data)->protocol;
    struct ipsock *sock=module->address.ip.sock;
    uint8_t *buf;
    int sidx, ridx, res, expected, minrecv, start, trycnt, dont_sent, i;
    plerrcode pres=plErr_System;
    ems_u32 status;

    if (!num)
        return plOK;
/*
 * If num is very large we have to split this action into smaller pieces.
 */

    buf=malloc(num*4+4); /* code, (seq,) num (2 byte), num words */
    if (!buf) {
        complain("sis3316 udp_read_regs: malloc: %s", strerror(errno));
        goto error;
    }

    if (protocol>1) {
        expected=3+num*4; /* code, seq, status, data */
        minrecv=3;
        start=2;
    } else {
        expected=2+num*4; /* code, status, data */
        minrecv=2;
        start=1;
    }

    sequence++;

#if 0
    printf("sis3316 udp_read_regs: before send:\n");
    printf("idx=%d\n", idx);
    for (i=0; i<idx; i++)
        printf("%02x ", buf[i]);
    printf("\n");
#endif

    trycnt=0;
    dont_sent=0;
    do {
        if (trycnt++>10) {
            complain("sis3316 udp_read_reg: no success after 10 tries");
            goto error;
        }

        if (!dont_sent) {
            sidx=0;
            buf[sidx++]=0x20;
            if (protocol>1)
                buf[sidx++]=sequence;
            sidx+=short2bytes(buf+sidx, num-1);
            for (i=0; i<num; i++)
                sidx+=word2bytes(buf+sidx, regs[i]);

            res=sendto(sock->p, buf, sidx, 0, sock->addr, sock->addrlen);
            if (res!=sidx) {
                complain("sis3316 udp_read_regs:sendto: res=%d error=%s",
                        res, strerror(errno));
                goto error;
            }
        } else {
            dont_sent=0;
        }

        /* this is always shorter than the size of buf */
        res=recv(sock->p, buf, expected+1, 0);
        if (res<0) {
            if (errno==EAGAIN) {
                complain("sis3316 udp_read_regs(ser %d, reg 0x%x):recv: "
                        "res=%d error=%s",
                    ((struct sis3316_priv_data*)module->private_data)->serial,
                    regs[0], res, strerror(errno));
                continue;
            } else {
                complain("sis3316 udp_read_regs(ser %d, reg 0x%x):recv: "
                        "res=%d error=%s",
                    ((struct sis3316_priv_data*)module->private_data)->serial,
                    regs[0], res, strerror(errno));
                goto error;
            }
        }

#if 0
        printf("sis3316 udp_read_regs: after recv:\n");
        for (i=0; i<res; i++)
            printf("%02x ", buf[i]);
        printf("\n");
#endif

        /* check sequence before other tests */
        if (protocol>1) {
            if (res<2) {
                complain("sis3316 udp_read_regs(ser %d, reg 0x%x):recv: "
                        "res=%d, no seq nr.",
                    ((struct sis3316_priv_data*)module->private_data)->serial,
                    regs[0], res);
                goto error;
            }
            if (buf[1]<sequence) {
                complain("sis3316 udp_read_regs(ser %d, reg 0x%x): wrong seq: "
                        "%d instead of %d",
                    ((struct sis3316_priv_data*)module->private_data)->serial,
                        regs[0], buf[1], sequence);
                dont_sent=1;
                continue;
            }
            if (buf[1]!=sequence) {
                complain("sis3316 udp_read_regs(ser %d, reg 0x%x): wrong seq: "
                        "%d instead of %d",
                    ((struct sis3316_priv_data*)module->private_data)->serial,
                        regs[0], buf[1], sequence);
                continue;
            }
        }

        if (res<minrecv) {
            complain("sis3316 udp_read_regs(ser %d, reg 0x%x):recv: "
                    "res=%d; %d required",
                    ((struct sis3316_priv_data*)module->private_data)->serial,
                    regs[0], res, minrecv);
            goto error;
        }

        /* set idx to status */
        ridx=start;
        status=buf[ridx];
        if (status&0x70) { /* some error */
            printf("sis3316 udp_read_regs: status=0x%02x\n", status);
            if (status&0x10)
                printf(" no grant");
            if (status&0x20)
                printf(" access timeout");
            if (status&0x40)
                printf(" protocol error");
            printf("\n");
            goto error;
        }

        if (res!=expected) {
            printf("sis3316 udp_read_reg: read %d bytes instead of %d\n",
                res, expected);
            for (i=0; i<res; i++)
                printf("%02x ", buf[i]);
            printf("\n");
            goto error;
        }

        ridx++; /* set idx to data */
        for (i=0; i<num; i++)
            ridx+=bytes2word(buf+ridx, values+i);

        pres=plOK;
        break;

    } while (1);

error:
    free(buf);
    return pres;
}
/*****************************************************************************/
static plerrcode
sis3316_vme_write_regs(ml_entry* module, ems_u32 *regs, ems_u32 *values, int num)
{
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
    int i;

    for (i=0; i<num; i++) {
        if (dev->write_a32d32(dev, addr+regs[i], values[i])!=4) {
            printf("sis3316_vme_write_regs(0x%08x) failed: %s\n",
                addr+regs[i], strerror(errno));
            return plErr_System;
        }
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
sis3316_udp_write_regs(ml_entry* module, ems_u32 *regs, ems_u32 *values, int num)
{
    int protocol=((struct sis3316_priv_data*)module->private_data)->protocol;
    struct ipsock *sock=module->address.ip.sock;
    uint8_t *buf;
    int idx, res, expected, trycnt, dont_sent, i;
    plerrcode pres=plErr_System;
    ems_u32 status;

    if (!num)
        return plOK;
/*
 * If num is very large we have to split this action into smaller pieces.
 */

    buf=malloc(num*8+4); /* code, (seq,) num (2 byte), 2*num words */
    if (!buf) {
        complain("sis3316 udp_write_regs: malloc: %s", strerror(errno));
        goto error;
    }

    sequence++;

    trycnt=0;
    dont_sent=0;
    do {
        if (trycnt++>10) {
            complain("sis3316 udp_read_reg: no success after 10 tries");
            goto error;
        }

        if (!dont_sent) {
            idx=0;
            buf[idx++]=0x21;
            if (protocol>1)
                buf[idx++]=++sequence;
            idx+=short2bytes(buf+idx, num-1);
            for (i=0; i<num; i++) {
                idx+=word2bytes(buf+idx, regs[i]);
                idx+=word2bytes(buf+idx, values[i]);
            }

#if 0
            printf("sis3316 udp_write_regs: before send:\n");
            printf("idx=%d\n", idx);
            for (i=0; i<idx; i++)
                printf("%02x ", buf[i]);
            printf("\n");
#endif

            res=sendto(sock->p, buf, idx, 0, sock->addr, sock->addrlen);
            if (res!=idx) {
                complain("sis3316 udp_write_regs:sendto: res=%d error=%s",
                        res, strerror(errno));
                goto error;
            }
        } else {
            dont_sent=0;
        }

        expected=protocol>1?3:2;
        res=recv(sock->p, buf, expected+1, 0);
        if (res<0) {
            complain("sis3316 udp_write_regs(ser %d, reg 0x%x):recv: "
                    "res=%d error=%s",
                ((struct sis3316_priv_data*)module->private_data)->serial,
                regs[0], res, strerror(errno));
            if (errno==EAGAIN)
                continue;
            else
                goto error;
        }

#if 0
        printf("sis3316 udp_write_regs: after recv:\n");
        for (i=0; i<res; i++)
            printf("%02x ", buf[i]);
        printf("\n");
#endif

        /* check sequence before other tests */
        if (protocol>1) {
            if (res<2) {
                complain("sis3316 udp_write_regs(ser %d, reg 0x%x):recv: "
                        "res=%d, no seq nr.",
                    ((struct sis3316_priv_data*)module->private_data)->serial,
                    regs[0], res);
                goto error;
            }
            if (buf[1]<sequence) {
                complain("sis3316 udp_write_regs(ser %d, reg 0x%x): wrong seq: "
                        "%d instead of %d",
                    ((struct sis3316_priv_data*)module->private_data)->serial,
                        regs[0], buf[1], sequence);
                dont_sent=1;
                continue;
            }
            if (buf[1]!=sequence) {
                complain("sis3316 udp_write_regs(ser %d, reg 0x%x): wrong seq: "
                        "%d instead of %d",
                    ((struct sis3316_priv_data*)module->private_data)->serial,
                        regs[0], buf[1], sequence);
                continue;
            }
        }

        if (res<expected) {
            if (res<0 && errno==EAGAIN)
                pres=plErr_Timeout;
            complain("sis3316 udp_write_regs(ser %d, reg 0x%x):recv: "
                    "res=%d ; %d required",
                ((struct sis3316_priv_data*)module->private_data)->serial,
                regs[0],  res, expected);
            goto error;
        }

        status=buf[protocol>1?2:1];
        if (status&0x70) {
            for (i=0; i<res; i++)
                printf("%02x ", buf[i]);
            printf("\n");
            printf("sis3316 udp_write_regs: status=0x%02x\n", status);
            if (status&0x10)
                printf(" no grant");
            if (status&0x20)
                printf(" access timeout");
            if (status&0x40)
                printf(" protocol error");
            printf("\n");
            goto error;
        }

        pres=plOK;
        break;

    } while (1);

error:
    free(buf);
    return pres;
}
/*****************************************************************************/
static plerrcode
sis3316_vme_read_mem(ml_entry* module, int fpga, int space, ems_u32 addr,
        ems_u32* values, int num)
{
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 vaddr=module->address.vme.addr;
    ems_u32 fifoaddr=0x00100000*(fpga+1);
    ems_u32 command, creg;
    ems_u32 sreg, val;
    plerrcode pres=plOK;
    int res;

    creg=0x80+4*fpga;        /* data transfer ctrl register */
    sreg=0x90+4*fpga;        /* data transfer status register */
#if 1
    /* check status */
    dev->read_a32d32(dev, vaddr+sreg, &val);
    if (val&0x80000000) {
#if 1
        printf("sis3316_vme_read_mem: fpga %d: logic busy      : %08x\n",
                fpga, val);
#endif
        /* reset transfer logic */
        dev->write_a32d32(dev, vaddr+creg, 0);
        microsleep(5);
        dev->read_a32d32(dev, vaddr+sreg, &val);
#if 0
        if (val&0x80000000)
            printf("sis3316_vme_read_mem: fpga %d: logic still busy: %08x\n",
                    fpga, val);
#endif
    }
#endif

    /* start transfer */
    command=0x80000000;      /* start read transfer */
    command|=(space&3)<<28;  /* space */
    command|=addr&0xfffffff; /* start address */

#if 0
//    printf("creg=%02x sreg=%02x\n", creg, sreg);
//    dev->read_a32d32(dev, vaddr+sreg, &val);
//    printf("initial status: %08x\n", val);
//    printf("command       : %08x\n", command);
#endif

    dev->write_a32d32(dev, vaddr+creg, command);
    microsleep(2);
#if 0
//    dev->read_a32d32(dev, vaddr+sreg, &val);
//    printf("status 1      : %08x\n", val);
#endif

    if (space<2) {
        /* read fifo */
#if 0
        printf("sis3316_vme_read_mem: fifo addr=%08x mem addr=%08x\n",
                fifoaddr, addr);
#endif
        res=dev->read_a32_fifo(dev, vaddr+fifoaddr, values, 4*num, 4, 0);
        if (res!=4*num) {
            printf("sis3316_vme_read_mem: res=%d instead of %d\n", res, 4*num);
            pres=plErr_System;
        }
    } else {
        /* special handling of statistics (to avoid disabling interrupts
           in the driver) */
        int i;
#if 0
        printf("sis3316_vme_read_mem(group=%d space=%d addr=%07x num=%d)\n",
                fpga, space, addr, num);
#endif
        for (i=0; i<num; i++) {
            res=dev->read_a32d32(dev, vaddr+fifoaddr, values+i);
            if (res!=4) {
                printf("sis3316_vme_read_mem(statist): res=%d instead of 4\n",
                        res);
                pres=plErr_System;
            }
        }
    }

#if 1
    /* reset transfer logic */
    dev->write_a32d32(dev, vaddr+creg, 0);

#if 0
    /* check status */
    dev->read_a32d32(dev, vaddr+sreg, &val);
    if (val&0x80000000) {
        printf("sis3316_vme_read_mem after read: fpga %d: logic still busy: "
                "%08x\n",
                fpga, val);
    }
#endif
#endif

    return pres;
}
/*****************************************************************************/
/*
 * Vorher einstellen:
 * 
 * enable/disable Jumbo frames (hardware feature)
 * define the number of packets per request (software feature)
 *     UDP protocol configuration register (0x8)
 */
/*
 * maximum packet size is:
 *     2048 words for jumbo frames
 *      360 words without jumbo frames
 * maximum number of  per request is
 *       32
 *     --> 65536 words with jumbo frames
 *     --> 11520 words without jumbo frames
 * warning:
 *     packets can be reordered by network devices
 */
static plerrcode
sis3316_udp_read_mem(ml_entry* module, int fpga, int space, ems_u32 addr,
        ems_u32* values, int num)
{
    struct sis3316_priv_data *priv=
            (struct sis3316_priv_data*)module->private_data;
    int protocol=priv->protocol;
    struct ipsock *sock=module->address.ip.sock;
    ems_u32 fifoaddr=0x100000*(fpga+1);
    ems_u32 command, creg;
    plerrcode pres=plErr_System;
    int idx, res, i, loop, pkt, padding, received;
    int maxnum_per_packet, maxnum_per_request;

#if 0
    printf("udp_read_mem(num=%d):\n", num);
#endif

    /* start transfer from memory to FIFO */
    command=0x80000000;      /* start read transfer */
    command|=(space&3)<<28;  /* space */
    command|=addr&0xfffffff; /* start address */
    creg=0x80+4*fpga;        /* data transfer ctrl register */

    pres=sis3316_udp_write_reg(module, creg, command);
    if (pres!=plOK)
        goto reset;

    microsleep(2);

#if 0
printf("udp_read_mem: priv %p:  max_udp_packets=%d jumbo_enabled=%d\n",
        priv, priv->max_udp_packets, priv->jumbo_enabled);
#endif
    maxnum_per_packet=priv->jumbo_enabled?2048:360;
    maxnum_per_request=priv->max_udp_packets*maxnum_per_packet;
    padding=protocol>1?1:2; /* data words will be aligned */
#if 0
printf("priv->jumbo_enabled=%d\n", priv->jumbo_enabled);
printf("maxnum_per_packet=%d\n", maxnum_per_packet);
printf("maxnum_per_request=%d\n", maxnum_per_request);
printf("padding=%d\n", padding);
#endif

    while (num) {
        int xnum=num;
        if (xnum>maxnum_per_request)
            xnum=maxnum_per_request;
#if 0
printf("num=%d xnum=%d\n", num, xnum);
#endif

        idx=0;
        buffer_8t[idx++]=0x30;
        if (protocol>1)
            buffer_8t[idx++]=++sequence;
        idx+=short2bytes(buffer_8t+idx, xnum-1);
        idx+=word2bytes(buffer_8t+idx, fifoaddr);

#if 0
printf("udp_read_mem(xnum=%d) send:\n", xnum);
for (i=0; i<idx; i++)
    printf("%02x ", buffer_8t[i]);
printf("\n");
#endif

        res=sendto(sock->p, buffer_8t, idx, 0, sock->addr, sock->addrlen);
        if (res!=idx) {
            complain("sis3316 udp_read_mem:sendto: res=%d error=%s",
                    res, strerror(errno));
            goto reset;
        }

        received=0;
        loop=0;
        while (received<xnum) {
            ems_u8 val; /* used for header bytes */
            res=recv(sock->p, buffer_8t+padding, 9000, 0);
            if (res<=0) {
                complain("sis3316 udp_read_mem:recv: res=%d error=%s",
                    res, strerror(errno));
                goto reset;
            }

#if 0
            printf("res=%d, %d words\n", res, res/4);
#endif

            idx=padding;
#if 0
printf("udp_read_mem received:\n");
for (i=idx; i<16; i++)
    printf("%02x ", buffer_8t[i]);
printf("\n");
#endif

            /* check header */
            if (res<(protocol>1?3:2)) {
                complain("sis3316 udp_read_mem: packet too short");
                goto reset;
            }

            val=buffer_8t[idx++];
            res--;
            if (val!=0x30) {
                complain("sis3316 udp_read_mem: illegal command byte %02x",
                    val);
                goto reset;
            }

            /* check sequence number */
            if (protocol>1) {
                val=buffer_8t[idx++];
                res--;
                if (val!=sequence) {
                    complain("sis3316 udp_read_mem: wrong sequence number: "
                            "%d->%d", sequence, val);
                    /* XXX
                       This is a hack only. We have to retry... */
                    pres=plErr_Timeout;
                    goto reset;
                }
            }

            /* check status */
            val=buffer_8t[idx++];
            res--;
            pkt=val&0xf;
            if (pkt!=(loop&0xf)) /* packet counter is 4 bit only */
                printf("sis3316_udp_read_mem: unexpected packet: %d->%d\n",
                        loop, pkt);

            if (val&0x70) {
                for (i=0; i<16; i++)
                    printf("%02x ", buffer_8t[i]);
                printf("\n");
                printf("sis3316_udp_read_mem: status=0x%02x\n", val);
                if (val&0x10)
                    printf(" no grant");
                if (val&0x20)
                    printf(" access timeout");
                if (val&0x40)
                    printf(" protocol error");
                printf("\n");
                goto reset;
            }
            if (res&3) {
                complain("sis3316 udp_read_mem: illegal size: %d", res);
                goto reset;
            }
            if (res/4>xnum) {
                complain("sis3316 udp_read_mem: packet too long: %d->%d",
                        xnum, res/4);
                goto reset;
            }

            /*
             * XXX
             * This is not correct! We have to convert the data from little
             * endian (modules ignore the convention of network byte
             * ordering) to host byte ordering. But (at least today) all our
             * hosts are little endian also.
             */

            values=mempcpy(values, buffer_8t+idx, res);
            received+=res/4;
            loop++;
        }
        num-=xnum;
    }
    pres=plOK;

reset:
    /* reset transfer logic */
    {
        plerrcode ppres; /* do not modify original error code */
        command=0x00000000; /* stop transfer */
        ppres=sis3316_udp_write_reg(module, creg, command);
        if (pres==plOK)
            pres=ppres;
    }
    return pres;
}
/*****************************************************************************/
static plerrcode
sis3316_vme_write_mem(ml_entry* module, int fpga, int space, ems_u32 addr,
        ems_u32* values, int num)
{
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 vaddr=module->address.vme.addr;
    ems_u32 fifoaddr=0x100000*(fpga+1);
    ems_u32 command, val, creg, sreg;
    plerrcode pres=plOK;
    int rem, res;

    //printf("\nsis3316_vme_write_mem\n");

#if 1
    /* check status */
    sreg=0x90+4*fpga;        /* data transfer status register */
    dev->read_a32d32(dev, vaddr+sreg, &val);
    if (val&0x80000000) {
        printf("sis3316_vme_write_mem: fpga %d: logic busy: %08x\n",
                fpga, val);
    }
#endif

    /* we have to write multiples of 64 words,
       may be we need a padding */
    rem=((num+63)/64)*64-num;

#if 0
    printf("vme_write_mem: num=%d rem=%d\n", num, rem);
#endif


#if 0 /* write_mem_buf is already initialised with 0... */
    if (rem) {
        /*
         * in theory we have to read the old values first,
         * but this is a test feature only ...
         */
        memset(write_mem_buf, 0, rem*4);
    }
#endif

    /* start transfer */
    command=0xc0000000;      /* start write transfer */
    command|=(space&3)<<28;  /* space */
    command|=addr&0xfffffff; /* start address */
    creg=0x80+4*fpga;        /* data transfer ctrl register */
    sreg=0x90+4*fpga;        /* data transfer status register */

#if 0
    dev->read_a32d32(dev, vaddr+sreg, &val);
    printf("initial status: %08x\n", val);
    printf("vme_write_mem: 0x%06x <== %08x\n", creg, command);
#endif
    dev->write_a32d32(dev, vaddr+creg, command);


    //dev->read_a32d32(dev, vaddr+sreg, &val);
    //printf("status 0      : %08x\n", val);

    /* write fifo*/
#if 0
    printf("vme_write_mem: 0x%06x <== %08x %08x %08x num=%d\n", fifoaddr,
            values[0], values[1], values[2], num);
#endif
    res=dev->write_a32_fifo(dev, vaddr+fifoaddr, values, 4*num, 4);
    if (res!=4*num) {
        printf("sis3316_vme_write_mem: res=%d instead of %d\n", res, 4*num);
        pres=plErr_System;
        goto error;
    }
    if (rem) {
#if 0
        printf("vme_write_mem: 0x%06x <== nix (rem=%d)\n", fifoaddr+num, rem);
#endif
        res=dev->write_a32_fifo(dev, vaddr+fifoaddr+num, buffer_32t, 4*rem, 4);
        if (res!=4*rem) {
            printf("sis3316_vme_write_mem: res=%d instead of %d\n", res, 4*rem);
            pres=plErr_System;
            goto error;
        }
    }

#if 0
    /* wait until data transfer logic is not busy */
    int loopdel=1;
    int again;
    do {
        dev->read_a32d32(dev, vaddr+sreg, &val);
        again=val&0x80000000;
        if (again) {
            microsleep(loopdel);
            loopdel*=2;
        }
    } while (again && loopdel<1000000);
#if 1
    if (again) {
        complain("sis3316_vme_write_mem: timeout; stat=%08x", val);
        //return plErr_Timeout;
    }
#endif
#else
    microsleep(10);
#endif
    /* reset transfer logic */
    dev->write_a32d32(dev, vaddr+creg, 0);

error:
    return pres;
}
/*****************************************************************************/
static plerrcode
sis3316_udp_write_mem(ml_entry* module, int fpga, int space, ems_u32 addr,
        ems_u32* values, int num)
{
    struct sis3316_priv_data *priv=
            (struct sis3316_priv_data*)module->private_data;
    int protocol=priv->protocol;
    struct ipsock *sock=module->address.ip.sock;
    ems_u32 fifoaddr=0x100000*(fpga+1);
    ems_u32 command, creg;
    plerrcode pres=plErr_System;
    int idx, res, i, written;

#if 0
    printf("udp_write_mem(num=%d):\n", num);
#endif

    creg=0x80+4*fpga;        /* data transfer ctrl register */

#if 0
    {
        /* check status */
        ems_u32 sreg=0x90+4*fpga;        /* data transfer status register */
        u_int32_t val;
        sis3316_udp_read_reg(module, sreg, &val);
        if (val&0x80000000) {
            printf("sis3316_udp_write_mem: fpga %d: logic busy: %08x\n",
                    fpga, val);
        }
    }
#endif

    /* start transfer from FIFO to memory */
    command=0xc0000000;      /* start write transfer */
    command|=(space&3)<<28;  /* memory space */
    command|=addr&0xfffffff; /* destinamtion address in memory */

    pres=sis3316_udp_write_reg(module, creg, command);
    if (pres!=plOK)
        goto reset;

    written=0;
    while (written<num) {
        ems_u8 status;
        /* We want to write num words, but we can write a maximum of
         * 256 words in a single request (--> xnum) and we can only
         * write multiples of 64 words (--> rxnum) 
         */
        int xnum;  /* number of words to be written */
        int rxnum; /* xnum rounded up to the next multiple of 64 */
        xnum=num;
        if (xnum>256)
            xnum=256;
        rxnum=((xnum+63)/64)*64;

        /*
         * We have to send header and data in the same UDP packet,
         * but do not want to copy the data to an extra buffer.
         * We could use writev, but our socket is not bound, we have
         * to use sendto.
         * Solution: modify the socket initialisation to use independent
         * and bound sockets for each module.
         */

        idx=0;
        buffer_8t[idx++]=0x31;                    /* command code */
        if (protocol>1)
            buffer_8t[idx++]=++sequence;
        idx+=short2bytes(buffer_8t+idx, rxnum-1); /* N-1 */
        idx+=word2bytes(buffer_8t+idx, fifoaddr); /* address of FIFO */
        for (i=0; i<xnum; i++)
            idx+=word2bytes(buffer_8t+idx, *values++);
        idx+=(rxnum-xnum)*4;                      /* size of the dummy words */

#if 0
        printf("udp_write_mem: write %d bytes\n", idx);
        printf("udp_write_mem(xnum=%d, rxnum=%d) send:\n", xnum, rxnum);
        for (i=0; i<idx && i<16; i++)
            printf("%02x ", buffer_8t[i]);
        printf("\n");
#endif
        res=sendto(sock->p, buffer_8t, idx, 0, sock->addr, sock->addrlen);
        if (res!=idx) {
            complain("sis3316 udp_write_mem:sendto: res=%d error=%s",
                    res, strerror(errno));
            return plErr_System;
        }

        res=recv(sock->p, buffer_8t, 4, 0);
        if (res<=0) {
            complain("sis3316 udp_write_mem:recv: res=%d error=%s",
                res, strerror(errno));
            return plErr_System;
        }

#if 0
        printf("udp_write_mem: got %d bytes:\n", res);
        for (i=0; i<res; i++)
            printf("%02x ", buffer_8t[i]);
        printf("\n");
#endif
        /* check header */
        if (res<(protocol>1?3:2)) {
            complain("sis3316 udp_write_mem: packet too short");
            return plErr_System;
        }

        if (buffer_8t[0]!=0x31) {
            complain("sis3316 udp_write_mem: illegal command byte %02x",
                    buffer_8t[0]);
            return plErr_System;
        }

        /* check sequence number */
        if (protocol>1 && buffer_8t[1]!=sequence) {
            complain("sis3316 udp_write_mem: wrong sequence number: "
                    "%d->%d", sequence, buffer_8t[1]);
            return plErr_System;
        }

        /* check status */
        status=buffer_8t[protocol>1?2:1];
        if (status&0x70) {
            for (i=0; i<16; i++)
                printf("%02x ", buffer_8t[i]);
            printf("\n");
            printf("sis3316_udp_write_mem: status=0x%02x\n", status);
            if (status&0x10)
                printf(" no grant");
            if (status&0x20)
                printf(" access timeout");
            if (status&0x40)
                printf(" protocol error");
            printf("\n");
            return plErr_System;
        }
        written+=xnum;
    }
    microsleep(2);
    pres=plOK;

reset:
    /* reset transfer logic */
    {
        plerrcode ppres; /* do not modify original error code */
        command=0x00000000; /* stop transfer */
        ppres=sis3316_udp_write_reg(module, creg, command);
        if (pres==plOK)
            pres=ppres;
    }
    return pres;
}
/*****************************************************************************/
static plerrcode
sis3316_init_soft_settings(ml_entry* module)
{
    struct sis3316_priv_data *priv=
            (struct sis3316_priv_data*)module->private_data;

    priv->max_udp_packets=1;
    priv->jumbo_enabled=0;
printf("init_soft_settings: module %p priv %p:  max_udp_packets=%d jumbo_enabled=%d\n",
        module, priv, priv->max_udp_packets, priv->jumbo_enabled);

    return plOK;
}
/*****************************************************************************/
static plerrcode
sis3316_init_private(ml_entry* module)
{
    struct sis3316_priv_data *priv;
    ems_u32 value;
    plerrcode pres;

    if (module->private_data)
        return plOK;

    priv=calloc(1, sizeof(struct sis3316_priv_data));
    if (!priv) {
        complain("sis3316_initialise_private: %s", strerror(errno));
        return plErr_NoMem;
    }
    module->private_data=priv;

    if ((pres=sis3316_init_soft_settings(module))!=plOK)
        return pres;

    switch (module->modulclass) {
    case modul_vme:
        priv->sis3316_read_ctrl=sis3316_vme_read_ctrl;
        priv->sis3316_write_ctrl=sis3316_vme_write_ctrl;
        priv->sis3316_read_reg=sis3316_vme_read_reg;
        priv->sis3316_write_reg=sis3316_vme_write_reg;
        priv->sis3316_read_regs=sis3316_vme_read_regs;
        priv->sis3316_write_regs=sis3316_vme_write_regs;
        priv->sis3316_read_reg_multi=0;
        priv->sis3316_read_regs_multi=0;
        priv->sis3316_read_mem=sis3316_vme_read_mem;
        priv->sis3316_write_mem=sis3316_vme_write_mem;
        pres=plOK;
        break;
    case modul_ip:
        priv->sis3316_read_ctrl=sis3316_udp_read_ctrl;
        priv->sis3316_write_ctrl=sis3316_udp_write_ctrl;
        priv->sis3316_read_reg=sis3316_udp_read_reg;
        priv->sis3316_write_reg=sis3316_udp_write_reg;
        priv->sis3316_read_regs=sis3316_udp_read_regs;
        priv->sis3316_write_regs=sis3316_udp_write_regs;
        priv->sis3316_read_reg_multi=sis3316_udp_read_reg_multi;
        priv->sis3316_read_regs_multi=sis3316_udp_read_regs_multi;
        priv->sis3316_read_mem=sis3316_udp_read_mem;
        priv->sis3316_write_mem=sis3316_udp_write_mem;
        pres=sis3316_init_udp(module);
        break;
    default:
        complain("sis3316_read: unexpected class %d", module->modulclass);
        pres=plErr_BadModTyp;
    }

    if (pres!=plOK)
        goto error;

    switch (module->modulclass) {
    case modul_vme:
        /* try to read ident */
        pres=priv->sis3316_read_ctrl(module, 0x4, &value);
        if (pres!=plOK)
            goto error;
        priv->module_id=value;
        break;
    case modul_ip:
        /* try to determine protocol version and ident */
        pres=sis3316_udp_check_protocol(module);
        if (pres!=plOK)
            goto error;
        break;
    default: /* all other module types are not possible here */
        ;
    }
    printf("module ID: %04x, firmware: 0x%x.0x%x\n",
            (priv->module_id>>16)&0xffff,
            (priv->module_id>>8)&0xff,
            priv->module_id&0xff);

    /* request access grant */
    pres=priv->sis3316_write_ctrl(module, 0x10, 0x1);
    if (pres!=plOK)
        goto error;
    pres=priv->sis3316_read_ctrl(module, 0x10, &value);
    if (pres!=plOK)
        goto error;
    printf("access: %08x\n", value);

    /* read serial number */
    pres=priv->sis3316_read_reg(module, 0x28, &value);
    if (pres!=plOK)
        goto error;
    if (value&0x10000) {
        complain("sis3316: serial number not valid");
        pres=plErr_HWTestFailed;
        goto error;
    }

    priv->serial=value&0xffff;
    complain("sis3316: serial=%d, MAC 00:00:56:31:6%01x:%02x",
            value&0xffff, (value>>8)&0xf, value&0xff);

    priv->memsize=((value&0x800000)?512:256)*1024ULL*1024ULL;

error:
    if (pres!=plOK) {
        module->private_data=0;
        free(priv);
    }
    return pres;
}
/*****************************************************************************/
static plerrcode
__attribute__((unused))
sis3316_read_ctrl(ml_entry* module, ems_u32 reg, ems_u32* value)
{
    struct sis3316_priv_data *priv;
    priv=(struct sis3316_priv_data*)module->private_data;

    return priv->sis3316_read_ctrl(module, reg, value);
}
/*****************************************************************************/
static plerrcode
__attribute__((unused))
sis3316_write_ctrl(ml_entry* module, ems_u32 reg, ems_u32 value)
{
    struct sis3316_priv_data *priv;
    priv=(struct sis3316_priv_data*)module->private_data;

    return priv->sis3316_write_ctrl(module, reg, value);
}
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
sis3316_write_reg(ml_entry* module, ems_u32 reg, ems_u32 value)
{
    struct sis3316_priv_data *priv;
    priv=(struct sis3316_priv_data*)module->private_data;

    return priv->sis3316_write_reg(module, reg, value);
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
sis3316_write_regs(ml_entry* module, ems_u32 *regs, ems_u32 *values, int num)
{
    struct sis3316_priv_data *priv;
    priv=(struct sis3316_priv_data*)module->private_data;

    return priv->sis3316_write_regs(module, regs, values, num);
}
/*****************************************************************************/
static plerrcode
__attribute__((unused))
sis3316_read_mem(ml_entry* module, int fpga, int space, ems_u32 addr,
        ems_u32* values, int num)
{
    struct sis3316_priv_data *priv;
    priv=(struct sis3316_priv_data*)module->private_data;

    /* end addr too large or integer overflow */
    if (addr+num>=0x4000000 || addr+num<addr) {
        printf("sis3316_read_mem: illegal addr/num values: %08x %08x\n",
                addr, num);
        return plErr_ArgRange;
    }

    return priv->sis3316_read_mem(module, fpga, space, addr, values, num);
}
/*****************************************************************************/
static plerrcode
__attribute__((unused))
sis3316_write_mem(ml_entry* module, int fpga, int space, ems_u32 addr,
        ems_u32 *values, int num)
{
    struct sis3316_priv_data *priv;
    priv=(struct sis3316_priv_data*)module->private_data;

    /* end addr too large or integer overflow */
    if (addr+num>=0x4000000 || addr+num<addr) {
        printf("sis3316_write_mem: illegal addr/num values: %08x %08x\n",
                addr, num);
        return plErr_ArgRange;
    }

    return priv->sis3316_write_mem(module, fpga, space, addr, values, num);
}
/*****************************************************************************/
__attribute__((unused))
static plerrcode
sis3316_rset_bits(ml_entry* module, ems_u32 reg, int nr_bits,
    int set_shift, int res_shift, ems_u32 value)
{
    ems_u32 mask, xval;

    mask=0xffffffffUL>>(32-nr_bits);
    value&=mask;
    xval=(((value&mask)<<set_shift)
        | ((~value&mask)<<res_shift));

    if (reg<0x20)
        return sis3316_write_ctrl(module, reg, xval);
    else
        return sis3316_write_reg(module, reg, xval);
}
/*****************************************************************************/
/**
 * 'for_each_member' calls 'proc' for each member of the IS modullist or
 * (if the IS modullist does not exist) for each member of the global
 * modullist.
 * p[1] has to be the module index (given as -1). It will be replaced by
 * the actual module idx.
 *
 * if write_count>0 the output will be preceded by the number of modules
 * if write_count==1 this is supressed if the procedures made no output
 * if write_count==2 this is not supressed
 *
 * Warning: because of the static variable (for_each_member_idx) this
 * function ist not reenrant. But ems server does not use threads...
 *
 * Warning 2: unlike check_sis3316 this procedure does not check whether
 * the hardware is really a SIS3316
 */

static int member_idx_=-1;
static int
__attribute__((unused))
member_idx(void)
{
    return member_idx_;
}

static int
is_sis3316_module(ml_entry* module)
{
    return (module->modulclass==modul_vme || module->modulclass==modul_ip)
                && module->modultype==SIS_3316;
}

static int
is_mcst_module(ml_entry* module)
{
    return module->modulclass==modul_vme && module->modultype==vme_mcst;
}

static plerrcode
for_each_sis3316_member(ems_u32 *p, plerrcode(*proc)(ems_u32*), int write_count)
{
    ml_entry* module;
    ems_u32 *help;
    ems_u32 *pc;
    plerrcode pres=plOK;
    unsigned int i, psize;

    if (!modullist)
        return plOK;

    /* create a copy of p */
    psize=4*(p[0]+1);
    pc=malloc(psize);
    if (!pc) {
        complain("sis3316:for_each: %s", strerror(errno));
        return plErr_NoMem;
    }
    memcpy(pc, p, psize);
    member_idx_=-1;
    if (write_count)
        help=outptr++;

    if (memberlist) { /* iterate over memberlist */
        for (i=1; i<=memberlist[0]; i++) {
            module=&modullist->entry[memberlist[i]];
            if (is_sis3316_module(module)) {
                pc[1]=i;
                member_idx_++;
                pres=proc(pc);
                if (pres!=plOK)
                    break;
            }
        }
    } else {          /* iterate over modullist */
        for (i=0; i<modullist->modnum; i++) {
            module=&modullist->entry[i];
            if (is_sis3316_module(module)) {
                pc[1]=i;
                member_idx_++;
                pres=proc(pc);
                if (pres!=plOK)
                    break;
            }
        }
    }

    if (write_count) {
        if (write_count==1 && outptr==help+1)
            outptr=help;
        else
            *help=member_idx_+1;
    }
    member_idx_=-1;
    free(pc);
    return pres;
}
/*****************************************************************************/
/**
 * 'for_each_module' calls 'proc' for each member of the IS modullist or
 * (if the IS modullist does not exist) for each member of the global
 * modullist.
 */
static plerrcode
for_each_sis3316_module(plerrcode(*proc)(ml_entry*))
{
    ml_entry* module;
    plerrcode pres=plOK;
    unsigned int i;

    if (memberlist) { /* iterate over memberlist */
        for (i=1; i<=memberlist[0]; i++) {
            module=&modullist->entry[memberlist[i]];
            if (is_sis3316_module(module)) {
                pres=proc(module);
                if (pres!=plOK)
                    break;
            }
        }
    } else {          /* iterate over modullist */
        for (i=0; i<modullist->modnum; i++) {
            module=&modullist->entry[i];
            if (is_sis3316_module(module)) {
                pres=proc(module);
                if (pres!=plOK)
                    break;
            }
        }
    }

    return pres;
}
/*****************************************************************************/
/**
 * 'for_each_group' calls 'proc' for each channel group by replacing
 * p[2] with the numers 0..3
 */
static plerrcode
for_each_group(ems_u32 *p, plerrcode(*proc)(ems_u32*))
{
    ems_u32 *pc;
    plerrcode pres=plOK;
    int group, psize;

    /* create a copy of p */
    psize=4*(p[0]+1);
    pc=malloc(psize);
    if (!pc) {
        complain("sis3316:for_each: %s", strerror(errno));
        return plErr_NoMem;
    }
    memcpy(pc, p, psize);

    for (group=0; group<4; group++) {
        pc[2]=group;
        pres=proc(pc);
        if (pres!=plOK)
            break;
    }

    free(pc);
    return pres;
}
/*****************************************************************************/
/*
 * check_sis3316 makes some simple test to check whether we can use a given
 * module (or modules) as sis3316.
 *
 * check_sis3316 calls sis3316_init_private to initiate the private
 * data structure of the module(s). Therefore each test_... procedure is
 * required to use check_sis3316.
 *
 * Actually it does not verify the hardware of the module.
 * In num it returns an upper limit of the number of modules used.
 */
static plerrcode
check_sis3316(ems_i32 idx, int allow_mcst, int *num, const char *caller)
{
    ml_entry *module;
    plerrcode pres;

    if (!modullist)
        return Err_NoDomain;

    if (num) {
        if (idx>=0)
            *num=1;
        else
            *num=memberlist?memberlist[0]:modullist->modnum;
    }

    /* if idx<0 all sis3316 modules are requested
     * 'for_each_sis3316_member' will iterate over all sis3316 modules
     * therefore we do not test anything here,
     * but we initialise the private module data, if necessary
     */
    if (idx<0) {
        pres=for_each_sis3316_module(sis3316_init_private);
        if (pres!=plOK)
            complain("check_sis3316(%s, idx=-1): communication init failed",
                    caller);
        return pres;
    }

    pres=get_modent(idx, &module);
    if (pres!=plOK) {
        complain("check_sis3316(%s): get_modent(idx=%u) failed", caller, idx);
        return pres;
    }

    if (!is_sis3316_module(module)) {
        if (!allow_mcst) {
            complain("check_sis3316(%s, idx=%u): no SIS3316", caller, idx);
            return plErr_BadModTyp;
        } else {
            if (!is_mcst_module(module)) {
                complain("check_sis3316(%s, idx=%u): neither SIS3316 "
                        "nor multicast", caller, idx);
                return plErr_BadModTyp;
            }
        }
    }

    pres=sis3316_init_private(module);
    if (pres!=plOK) {
        complain("check_sis3316[%s, idx=%d]: communication init failed",
                caller, idx);
        return pres;
    }

#if 0
    if ((pres=verify_vme_module(module, 0))!=plOK) {
        complain("sis3316: idx=%u: hardware not verified (see server log)",
            idx);
        return pres;
    }
#endif

    return plOK;
}
/*****************************************************************************/
/*****************************************************************************/
static plerrcode
dump_acq_state(ml_entry* module, const char *txt)
{
        ems_u32 address_regs[]={
            0x60,
            0x1120, 0x1124, 0x1128, 0x112c,
            0x2120, 0x2124, 0x2128, 0x212c,
            0x3120, 0x3124, 0x3128, 0x312c,
            0x4120, 0x4124, 0x4128, 0x412c,
        };
        const int n=sizeof(address_regs)/sizeof(ems_u32);
        ems_u32 vals[n];
        plerrcode pres;
        int i;

        printf("dump_acq_state: %s\n", txt);
        pres=sis3316_read_regs(module, address_regs, vals, n);
        if (pres!=plOK) {
            printf("dump_acq_state: read_regs failed\n");
            return plErr_System;
        }

        for (i=0; i<n; i++) {
            printf("%04x: %08x\n", address_regs[i], vals[i]);
        }

        return plOK;
}
/*****************************************************************************/
/**
 * "Link Interface Access Arbitration Control Register"
 *
 * sis3316_access requests access to the internal register space through
 * the currently used interface.
 * With an argument '0' it just requests the access.
 * With an argument '1' it also 'kills' the grant of the other interface.
 * In both cases it returns the new register content.
 * Without an argument it returns the actual register content.
 * 
 * @param p[0] argcount(1 or 2)
 * @param p[1] index in memberlist (or -1 for all sis3316 modules)
 * @param [p[2]] argument
 */
plerrcode proc_sis3316_access(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_access, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 val;

        if (p[0]>1) {
            val=1;
            if (p[2])
                val|=0x80000000;
            if ((pres=sis3316_write_ctrl(module, 0x10, val))!=plOK)
                return pres;
        }
        if ((pres=sis3316_read_ctrl(module, 0x10, &val))!=plOK)
            return pres;
        *outptr++=val;
    }

    return plOK;
}

plerrcode test_proc_sis3316_access(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]<1 || p[0]>2) {
        complain("sis3316_access: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_access"))!=plOK)
        return pres;

    wirbrauchen=p[0]>2?num+1:0;
    return plOK;
}

char name_proc_sis3316_access[] = "sis3316_access";
int ver_proc_sis3316_access = 1;
/*****************************************************************************/
/**
 * sis3316_comm explicitly initialises the read and write procedures for
 * one or all sis3316 modules.
 * In case of UDP communication the necessary socket is created.
 * To call this procedure is optional, the initialisation is done on demand
 * if needed.
 * It makes no harm if the procedure is called more than once.
 * The socket(s) will persist until free_modullist is executed.
 * 
 * p[0]: argcount(==1)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 */
plerrcode proc_sis3316_comm(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_comm, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);

        pres=sis3316_init_private(module);
        if (pres!=plOK)
            complain("communication init for sis3316[%d] failed", p[1]);
    }

    return pres;
}

plerrcode test_proc_sis3316_comm(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;

    if (p[0]!=1) {
        complain("sis3316_comm: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    wirbrauchen=0;
    return check_sis3316(ip[1], 0, 0, "sis3316_comm");
}

char name_proc_sis3316_comm[] = "sis3316_comm";
int ver_proc_sis3316_comm = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(2 or 3)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: register
 * [p[3]: value]
 */
plerrcode proc_sis3316_reg(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_reg, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);

        if (p[0]>2) {
            if (p[2]<0x20) {
                pres=sis3316_write_ctrl(module, p[2], p[3]);
            } else {
                pres=sis3316_write_reg(module, p[2], p[3]);
            }
       } else {
            ems_u32 val;
            if (p[2]<0x20) {
                pres=sis3316_read_ctrl(module, p[2], &val);
            } else {
                pres=sis3316_read_reg(module, p[2], &val);
            }
            if (pres==plOK)
                *outptr++=val;
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_reg(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]<2 || p[0]>3) {
        complain("sis3316_reg: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_reg"))!=plOK)
        return pres;

    if (p[2]>0x4ffc) {
        complain("sis3316_reg: illegal reg address: 0x%x", p[2]);
        return plErr_ArgRange;
    }

    wirbrauchen=p[0]>2?num+1:0;
    return plOK;
}

char name_proc_sis3316_reg[] = "sis3316_reg";
int ver_proc_sis3316_reg = 1;
/*****************************************************************************/
/*
 * After Power Up or after any Sample Clock changes require the following 
 * sequence:
 * 1. Issue a Key Register Reset command (disarms the sample logic, too)
 * 2. Setup the ADC Sample Clock distribution logic via the ADC Sample Clock 
 *    distribution control register
 * 3. In case of use
 *      Internal CLK: program internal CLK oscillator and wait until the clock
 *                    is stable (min. 10ms)
 *      ...
 * 4. Issue a Key ADC Clock DCM/PLL Reset command.
 * 5. Calibrate and configure the ADC FPGA input logic of the ADC data inputs 
 * via the ADC Input tap delay registers. 
 * 6. Setup sample logic
 */
/**
 * p[0]: argcount(==1)
 * p[1]: index in memberlist (or -1 for all modules)
 */
plerrcode proc_sis3316_init(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_init, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);
#if 1
        struct sis3316_priv_data *priv=
                (struct sis3316_priv_data*)module->private_data;
#endif
        ems_u32 val;

        /* claim access */
        if ((pres=sis3316_write_ctrl(module, 0x10, 0x80000001))!=plOK) {
            complain("sis3316_init: error claiming access");
            goto error;
        }
        if ((pres=sis3316_read_ctrl(module, 0x10, &val))!=plOK)
            return pres;
        if (!(val&0x00100000)) {
            complain("sis3316_init: access not granted: %08x, x=%08x, y=%08x",
                    val, (val&0x00100000), !(val&0x00100000));
            pres=plErr_HWTestFailed;
            goto error;
        }

        /* reset module */
        if ((pres=sis3316_write_reg(module, 0x400, 0))!=plOK)
            goto error;

        /* reset software settings */
        if ((pres=sis3316_init_soft_settings(module))!=plOK)
            goto error;

        /* claim access */
        if ((pres=sis3316_write_ctrl(module, 0x10, 0x00000001))!=plOK) {
            complain("sis3316_init: error claiming access");
            goto error;
        }

        /* read ID and version */
        if ((pres=sis3316_read_ctrl(module, 0x4, &val))!=plOK)
            goto error;
        complain("sis3316[%d]: ID reg: %08x", p[1], val);
        complain("sis3316[%d]: FW version: %x.%x", p[1],
                (val>>8)&0xff, val&0xff);

        /* read serial number */
        if ((pres=sis3316_read_reg(module, 0x28, &val))!=plOK)
            goto error;
        if (val&0x10000) {
            complain("sis3316[%d]: serial number not valid", p[1]);
        } else {
            complain("sis3316[%d]: serial=%d, MAC 00:00:56:31:6%01x:%02x",
                p[1], val&0xffff, (val>>8)&0xf, val&0xff);
        }
        complain("sis3316[%d]: %d MByte Memory", p[1], (val&0x800000)?512:256);

        /* disarm sample logic */
        if ((pres=sis3316_write_reg(module, 0x414, 0))!=plOK)
            goto error;

        /* read temperature */
        if ((pres=sis3316_read_reg(module, 0x20, &val))!=plOK)
            goto error;
        complain("sis3316[%d]: temperature=%.2f centigrade", p[1], val*.25);

        /* check state of ADC FPGAs */
        if (module->modulclass==modul_ip) {
            complain("sis3316_init(ser %d): ADC FPGA state not checked!",
                    priv->serial);
        } else {
            if ((pres=sis3316_read_reg(module, 0x30, &val))!=plOK)
                goto error;
            if (val!=0x01f00000) {
                complain("sis3316 init(ser %d): ADC FPGAs not ready: %08x",
                        priv->serial, val);
                //pres=plErr_HWTestFailed;
                //goto error;
            }
        }
#
#if 1
        /* reset sample clock to factory defaults */
        if ((pres=sis3316_reset_clock(module, 0))!=plOK)
            goto error;
        priv->clocks[0].initial_clock_valid=0;
#endif

        if ((pres=sis3316_adc_spi_setup(module))!=plOK)
            goto error;

        /* it seems that the interrupt control register is not cleared
            during reset; we do it here */
        if ((pres=sis3316_write_ctrl(module, 0xc, 0xff0000))!=plOK) {
            complain("sis3316_init: error clearing IRQs");
            goto error;
        }

        /* switch user LED on */
        sis3316_write_ctrl(module, 0x0, 0x1 /*0x10000: off*/);

        pres=plOK;
    }

error:
gettimeofday(&tv0, 0);
    return pres;
}

plerrcode test_proc_sis3316_init(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;

    if (p[0]!=1)
        return plErr_ArgNum;

    wirbrauchen=0;
    return check_sis3316(ip[1], 0, 0, "sis3316_init");
}

char name_proc_sis3316_init[] = "sis3316_init";
int ver_proc_sis3316_init = 1;
/*****************************************************************************/
/**
 * sis3316_reset just issues a Key Register Reset command
 * p[0]: argcount(==1)
 * p[1]: index in memberlist (or -1 for all modules)
 */
plerrcode proc_sis3316_reset(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_reset, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);

        /* reset module */
        pres=sis3316_write_reg(module, 0x400, 0);
    }

    return pres;
}

plerrcode test_proc_sis3316_reset(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;

    if (p[0]!=1)
        return plErr_ArgNum;

    wirbrauchen=0;
    return check_sis3316(ip[1], 0, 0, "sis3316_reset");
}

char name_proc_sis3316_reset[] = "sis3316_reset";
int ver_proc_sis3316_reset = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(5)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: group (0..3)
 * p[3]: space (0: mem1, 1: mem2, 3: statistic counter)
 * p[4]: addr
 * p[5]: num
 */
plerrcode proc_sis3316_read_mem(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_read_mem, 2);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        int num=p[5];

        pres=sis3316_read_mem(module, p[2], p[3], p[4], outptr+1, num);
        if (pres==plOK) {
            *outptr++=num;
            outptr+=num;
        } else {
            *outptr++=0;
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_read_mem(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=5) {
        complain("sis3316_read_mem: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_read_mem"))!=plOK)
        return pres;

    if (p[2]>3) {
        complain("sis3316_read_mem: illegal group: %d", p[2]);
        return plErr_ArgRange;
    }
    if (p[3]>3 || p[3]==2) {
        complain("sis3316_read_mem: illegal space: %d", p[3]);
        return plErr_ArgRange;
    }
    if (p[4]>0x3ffffff) {
        complain("sis3316_read_mem: illegal address: 0x%x", p[4]);
        return plErr_ArgRange;
    }

    /* (end address too large) || (integer overflow) */
    if (p[4]+p[5]>0x4000000 || p[4]+p[5]<p[4]) {
        complain("sis3316_read_mem: illegal num: %d --> 0x%x", p[5], p[4]+p[5]);
        return plErr_ArgRange;
    }

    wirbrauchen=(p[5]+1)*num+1;
    return plOK;
}

char name_proc_sis3316_read_mem[] = "sis3316_read_mem";
int ver_proc_sis3316_read_mem = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(>4)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: group (0..3)
 * p[3]: space (0: mem1, 1: mem2, 3: statistic counter)
 * p[4]: addr
 * p[5...]: data
 */
plerrcode proc_sis3316_write_mem(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_write_mem, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        pres=sis3316_write_mem(module, p[2], p[3], p[4], p+5, p[0]-4);
    }

    return pres;
}

plerrcode test_proc_sis3316_write_mem(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    int num;

    if (p[0]<5) {
        complain("sis3316_write_mem: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (p[2]>3) {
        complain("sis3316_write_mem: illegal group: %d", p[2]);
        return plErr_ArgRange;
    }
    if (p[3]>3 || p[3]==2) {
        complain("sis3316_write_mem: illegal space: %d", p[3]);
        return plErr_ArgRange;
    }
    if (p[4]>0x3ffffff) {
        complain("sis3316_write_mem: illegal address: 0x%x", p[4]);
        return plErr_ArgRange;
    }

    num=p[0]-4;
    /* (end address too large) || (integer overflow) */
    if (p[4]+num>0x4000000 || p[4]+num<p[4]) {
        complain("sis3316_write_mem: illegal num: %d --> 0x%x", p[5], p[4]+num);
        return plErr_ArgRange;
    }

    wirbrauchen=0;
    return check_sis3316(ip[1], 0, 0, "sis3316_write_mem");
}

char name_proc_sis3316_write_mem[] = "sis3316_write_mem";
int ver_proc_sis3316_write_mem = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(3 or 4)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: chan
 * p[3]: bank
 * [p[4]: num]
 */
plerrcode proc_sis3316_read_chan(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_read_chan, 2);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        int num, chan, bank, group, space;
        ems_u32 start;

        chan=p[2];
        bank=p[3];
        group=chan/4;
        chan=chan%4;
        space=chan/2;
        start=(chan%2)*0x2000000+bank*0x1000000;

        if (p[0]<4) {
            /* read the number */
            num=20;
        } else {
            num=p[4];
        }

        pres=sis3316_read_mem(module, group, space, start, outptr+1, num);
        if (pres==plOK) {
            *outptr++=num;
            outptr+=num;
        } else {
            *outptr++=0;
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_read_chan(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]<3 || p[0]>4) {
        complain("sis3316_read_chan: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_read_chan"))!=plOK)
        return pres;

    if (p[2]>15) {
        complain("sis3316_read_chan: illegal channel: %d", p[2]);
        return plErr_ArgRange;
    }
    if (p[3]>1) {
        complain("sis3316_read_chan: illegal bank: %d", p[3]);
        return plErr_ArgRange;
    }

    wirbrauchen=(p[5]+1)*num+1;
    return plOK;
}

char name_proc_sis3316_read_chan[] = "sis3316_read_chan";
int ver_proc_sis3316_read_chan = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(3 or 4)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * [p[2]: channel mask (default=0xffff --> all channels enabled)]
 *
 * for the logic see page 28 of the manual
 *
 * A "broadcast module" in the modullist is tolerated, but not used.
 * In this case the for_each_sis3316_member construct has to be replaced
 * by a more sophisticated logic.
 */
#if 0
plerrcode proc_sis3316_readout(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_readout, 2);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 address_regs[16]={
            0x1120, 0x1124, 0x1128, 0x112c,
            0x2120, 0x2124, 0x2128, 0x212c,
            0x3120, 0x3124, 0x3128, 0x312c,
            0x4120, 0x4124, 0x4128, 0x412c,
        };
        ems_u32 address_vals[16];
        ems_u32 channelmask=0xffff;
        ems_u32 acqstate/*, irqconf, irqstate*/;
        ems_u32 *help;
        int chan, bank, xbank, again, count, delay;

        if (p[0]>1)
            channelmask=p[2];

        /* read ACQ status */
        pres=sis3316_read_reg(module, 0x60, &acqstate);
        if (pres!=plOK) {
            complain("sis3316_readout: cannot read ACQ status");
            goto error;
        }
        if (!(acqstate&(1<<16))) {
            complain("sis3316_readout[%d]: sample logic not armed",
                    member_idx());
            pres=plErr_Verify;
            goto error;
        }

dump_acq_state(module, "before readout");

        /* Readout data should be consistent regardless whether we
           read out all modules (p[1]==-1) or one module only.
           
           */
        if (member_idx()<0)
            *outptr++=1;

        bank=!!(acqstate&(1<<17)); /* actual bank */
        //printf("acqstate   : %08x\n", acqstate);
        //printf("actual bank: %d\n", bank);

        /* disable and clear interrupt */
        sis3316_write_ctrl(module, 0xc, 0xc<<16);

        /* disarm/arm banks */
        pres=sis3316_write_reg(module, bank?0x420:0x424, 0);
        if (pres!=plOK) {
            complain("sis3316_readout: cannot switch banks");
            goto error;
        }

        count=30;
        delay=1;
        int deltime=0;
        do {
            /* read the "previous bank sample address" registers;
               bit 24 indicates the correspondin bank */
            pres=sis3316_read_regs(module, address_regs, address_vals, 16);
            if (pres!=plOK) {
                complain("sis3316_readout: cannot read sample addresses");
                goto error;
            }
            /* check whether banks are switched */
            again=0;
            for (chan=0; chan<16; chan++) {
                /* old bank still active? */
                xbank=!!(address_vals[chan]&(1<<24));
                if (bank != xbank) {
#if 0
                    printf("sis3316_readout: old bank[chan=%d]: %08x\n",
                            chan, address_vals[chan]);
#endif
                    microsleep(delay);
                    deltime+=delay;
                    delay*=2;
                    again=1;
                    break;
                }
            }
        } while (--count && again);
#if 1
        printf("sis3316_readout: swapcount=%d, %d us\n", count, deltime);
#endif
dump_acq_state(module, "after swap");
        if (again) {
            complain("sis3316_readout: banks not switched");
            pres=plErr_Timeout;
            goto error;
        }

        /* reenable interrupt */
        sis3316_write_ctrl(module, 0xc, 0x4);

        help=outptr++;
        for (chan=0; chan<16; chan++) {
            /* If the channel is not selected we skip it. */
            if (!((1<<chan)&channelmask)) {
                continue;
            }
            /*
             * for each channel we need three derived values: group space start
             *     group  ==FPGA ==chan/4 (0..4)
             *     space  mem1 (==0), mem2 (==1), statistics (==3)
             *     space  ==(chan/2)%2
             *     start  ==(chan%2)*0x2000000+bank*0x1000000;
             */
            int group=chan/4; /* ADC FPGA */
            int space=(chan/2)%2; /* memory inside group */
            ems_u32 start=(chan%2)*0x2000000+bank*0x1000000;
            ems_u32 *help_ch=outptr++;
            int num=address_vals[chan]&0xffffff;

            pres=sis3316_read_mem(module, group, space, start, outptr, num);
            if (pres!=plOK) {
                complain("sis3316_readout: read_mem failed");
                goto error;
            }
            *help_ch=num;
            outptr+=num;
#if 0
            printf("mod %d chan %d num=%d\n", p[1], chan, num);
#endif
        }
        *help=outptr-help-1;
    }

error:
    return pres;
}
#endif

static plerrcode
try_restart(ml_entry* module)
{
    ems_u32 acqstate;
    int bank;


    microsleep(20);

    /* read ACQ status */
    if (sis3316_read_reg(module, 0x60, &acqstate)!=plOK) {
        complain("sis3316 restart: cannot read ACQ status");
        return plErr_HW;
    }
    bank=!!(acqstate&(1<<17)); /* actual bank */

    microsleep(20);

    /* disarm sample logic */
    sis3316_write_reg(module, 0x414, 0);

    microsleep(20);

    /* swap banks */
    sis3316_write_reg(module, bank?0x420:0x424, 0);

        /* reenable interrupt */
        sis3316_write_ctrl(module, 0xc, 0x4);
dump_acq_state(module, "after restart");

    return plOK;
}

//ems_u32 clearmem[0x20000];

plerrcode proc_sis3316_readout(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;
    static int oldbank=0;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_readout, 2);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 address_regs[16]={
            0x1120, 0x1124, 0x1128, 0x112c,
            0x2120, 0x2124, 0x2128, 0x212c,
            0x3120, 0x3124, 0x3128, 0x312c,
            0x4120, 0x4124, 0x4128, 0x412c,
        };
        ems_u32 address_vals[16];
        ems_u32 channelmask=0xffff;
        ems_u32 acqstate/*, irqconf, irqstate*/;
        ems_u32 *help;
        int chan, bank, xbank, again, count, delay, deltime, i;
        ems_u32 state[10];

        if (p[0]>1)
            channelmask=p[2];

//dump_acq_state(module, "before readout");
#if 0
printf("READOUT mod %d", p[1]);
#endif

        /* read ACQ status */
        pres=sis3316_read_reg(module, 0x60, &acqstate);
        if (pres!=plOK) {
            complain("sis3316_readout: cannot read ACQ status");
            goto error;
        }
        if (!(acqstate&(1<<16))) {
            complain("sis3316_readout[%d]: sample logic not armed",
                    member_idx());
            pres=plErr_Verify;
            goto error;
        }

        bank=!!(acqstate&(1<<17)); /* actual bank */
#if 1
        if (bank==oldbank) {
            printf("wrong bank: old=%d current=%d\n", oldbank, bank);
            printf("acqstate   : %08x\n", acqstate);
            printf("actual bank: %d\n", bank);
        }
#endif
        oldbank=bank;

        /* disable and clear interrupt */
        sis3316_write_ctrl(module, 0xc, 0xc<<16);

        /* disarm/arm banks */
        pres=sis3316_write_reg(module, bank?0x420:0x424, 0);
        if (pres!=plOK) {
            complain("sis3316_readout: cannot switch banks");
            goto error;
        }

        /* check for lost arm bit */
        sis3316_read_reg(module, 0x60, &acqstate);
        if (!(acqstate&(1<<16))) {
            state[0]=acqstate;
            for (i=1; i<10; i++) {
                sis3316_read_reg(module, 0x60, state+i);
                microsleep(10);
            }
            printf("arm bit lost");
            for (i=0; i<10; i++) {
                printf("%08x\n", state[i]);
            }
        }

        /* poll for changed bank flag */
        count=20;
        delay=1;
        deltime=0;
        do {
            again=0;
            sis3316_read_reg(module, 0x60, &acqstate);
            if (bank==!!(acqstate&(1<<17))) { /* still the old bank */
                microsleep(delay);
                deltime+=delay;
                delay*=2;
                again=1;
            }
        } while (count-- && again);
//dump_acq_state(module, "after swap");
        if (again) {
            complain("sis3316_readout: banks not switched");
            //pres=plErr_Timeout;
            //goto error;
            return try_restart(module);
        }

if (!(acqstate&(1<<16))) {
            complain("arm bit lost; restarting");
            return try_restart(module);
}


        /* wait for end addresses to become valid */
        count=20;
        delay=1;
        deltime=0;
        do {
            /* read the "previous bank sample address" registers;
               bit 24 indicates the correspondin bank */
            pres=sis3316_read_regs(module, address_regs, address_vals, 16);
            if (pres!=plOK) {
                complain("sis3316_readout: cannot read sample addresses");
                goto error;
            }
            /* check whether banks are switched */
            again=0;
            for (chan=0; chan<16; chan++) {
                /* old bank still active? */
                xbank=!!(address_vals[chan]&(1<<24));
                if (bank!=xbank) {
#if 0
                    printf("sis3316_readout: old bank[chan=%d]: %08x\n",
                            chan, address_vals[chan]);
#endif
                    microsleep(delay);
                    deltime+=delay;
                    delay*=2;
                    again=1;
                    break;
                }
            }
        } while (count-- && again);
//dump_acq_state(module, "after address read");
        if (again) {
            complain("sis3316_readout: addresses not valid");
            pres=plErr_Timeout;
            goto error;
        }

        /* reenable interrupt */
        sis3316_write_ctrl(module, 0xc, 0x4);

        /* Readout data should be consistent regardless whether we
           read out all modules (p[1]==-1) or one module only. */
        if (member_idx()<0)
            *outptr++=1;

        help=outptr++;
        for (chan=0; chan<16; chan++) {
            /* If the channel is not selected we skip it. */
            if (!((1<<chan)&channelmask)) {
                continue;
            }
            /*
             * for each channel we need three derived values: group space start
             *     group  ==FPGA ==chan/4 (0..4)
             *     space  mem1 (==0), mem2 (==1), statistics (==3)
             *     space  ==(chan/2)%2
             *     start  ==(chan%2)*0x02000000+bank*0x01000000;
             */
            int group=chan/4; /* ADC FPGA */
            int space=(chan/2)%2; /* memory inside group */
            ems_u32 start=(chan%2)*0x02000000+bank*0x01000000;
            ems_u32 *help_ch=outptr++;
            int num=address_vals[chan]&0xffffff;
#if 0
printf("readout: chan %x bank %d group %d space %d start %08x num %d\n",
    chan, bank, group, space, start, num);
#endif
            pres=sis3316_read_mem(module, group, space, start, outptr, num);
            if (pres!=plOK) {
                complain("sis3316_readout: read_mem failed");
                goto error;
            }
#if 0
printf("head: %08x %08x\n", outptr[0], outptr[1]);
#endif
            *help_ch=num;
            outptr+=num;

#if 0
            /* clear channel memory */
            for (i=0; i<0x20000; i++) {
                clearmem[i]=0x80008000|(i&0xfff)|(chan<<16);
            }
            pres=sis3316_write_mem(module, group, space, start, clearmem,
                    0x20000);
            if (pres!=plOK) {
                complain("sis3316_readout: write_mem failed");
                goto error;
            }
#endif
        }
        *help=outptr-help-1;
    }

error:
    return pres;
}


plerrcode test_proc_sis3316_readout(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]<1 || p[0]>2) {
        complain("sis3316_readout: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_readout"))!=plOK)
        return pres;

    wirbrauchen=-1;
    return plOK;
}

char name_proc_sis3316_readout[] = "sis3316_readout";
int ver_proc_sis3316_readout = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(2 or more)
 * p[1..]: indices in memberlist (no -1 allowed)
 *
 * The logic is explained at page 28 of the manual (sis3316-m-1-1-v117.pdf).
 * 1..3: done in sis3316_readout_start
 * 4: done in trigger procedure (trig_sis3316_poll)
 * 5..8: in this procedure
 *
 * The name (sis3316_readout_udp) is probably wrong. The procedure does not
 * use any VME specifica. But it should be usable for both UDP and VME access.
 */
plerrcode
proc_sis3316_readout_udp(ems_u32* p)
{
    ems_u32 address_regs[16]={
        0x1120, 0x1124, 0x1128, 0x112c,
        0x2120, 0x2124, 0x2128, 0x212c,
        0x3120, 0x3124, 0x3128, 0x312c,
        0x4120, 0x4124, 0x4128, 0x412c,
    };
    ems_u32 address_vals[16];
    ems_u32 *help, *orig_outptr=outptr;
    ems_u32 acqstate;
    plerrcode pres=plOK;
    ml_entry* module;
    struct sis3316_priv_data *priv;
    unsigned int m, count, delay, deltime, again, chan;
    int fb_used=1;

    /* read and store all acq state registers */
    for (m=1; m<=p[0]; m++) {
        module=ModulEnt(p[m]);
        priv=(struct sis3316_priv_data*)module->private_data;
        pres=sis3316_read_reg(module, 0x60, &acqstate);
        if (pres!=plOK) {
            complain("sis3316_readout_udp(ser. %d): cannot read ACQ status",
                    priv->serial);
            goto error;
        }
        if (!(acqstate&(1<<16))) {
            complain("sis3316_readout_udp(ser. %d): sample logic not armed",
                    priv->serial);
            pres=plErr_Verify;
            goto error;
        }

        //priv->last_acq_state=acqstate;
        priv->previous_bank=!!(acqstate&(1<<17)); /* actual bank */
#if 0
        printf("(%d) %d%s\n", priv->serial, priv->previous_bank,
                priv->last_acq_state_valid?" v":"");
#endif
    }

    /* swap banks
     * If the front bus is used to distribute the swap signal only the
     * first module has to be accessed.
     */
    for (m=1; m<=p[0]; m++) {
        module=ModulEnt(p[m]);
        priv=(struct sis3316_priv_data*)module->private_data;

        /* disarm/arm banks */
        pres=sis3316_write_reg(module, priv->previous_bank?0x420:0x424, 0);
        if (pres!=plOK) {
            complain("sis3316_readout_udp(ser. %d): cannot switch banks",
                    priv->serial);
            goto error;
        }

        /* check for lost arm bit */
            /* nein, keine Lust */

        if (fb_used)
            break;
    }

    *outptr++=p[0]; /* number of modules */

    /* wait for bank swap and read out */
    for (m=1; m<=p[0]; m++) {
        module=ModulEnt(p[m]);
        priv=(struct sis3316_priv_data*)module->private_data;

        /*
         * actual bank swap occures after the actual event is completely
         * stored; we may have to wait here
         */
        count=10;
        delay=1;
        deltime=0;
        do { /* poll for changed bank flag */
            again=0;
            pres=sis3316_read_reg(module, 0x60, &acqstate);
            if (pres!=plOK) {
                complain("sis3316_readout_udp(ser. %d): cannot read ACQ state",
                        priv->serial);
                goto error;
            }
            /* still the old bank? */
            if (!!(acqstate&(1<<17))==priv->previous_bank) {
                microsleep(delay);
                deltime+=delay;
                delay*=2;
                again=1;
            }
        } while (count-- && again);
        if (again) {
//MIT gettimeofday reale Zeit messen (erst zum Test, spaeter statt microsleep?)
            complain("sis3316_readout(ser. %d): banks not switched (bank %d)",
                priv->serial, priv->previous_bank);
#if 1
            pres=plErr_Timeout;
            goto error;
#else
            return try_restart(module);
#endif
        }

        /* read the amount of data to be read (Previous Bank Sample address) */
        pres=sis3316_read_regs(module, address_regs, address_vals, 16);
        if (pres!=plOK) {
            complain("sis3316_readout_udp(ser. %d): cannot read sample "
                    "addresses", priv->serial);
            goto error;
        }
        /* check again whether banks are switched */
        for (chan=0; chan<16; chan++) {
            /* old bank still active? */
            if (!!(address_vals[chan]&(1<<24))!=priv->previous_bank) {
#if 1
                printf("sis3316_readout_udp(ser. %d): old bank[chan=%d]: "
                        "%08x\n", priv->serial,
                        chan, address_vals[chan]);
#endif
            }
        }

        help=outptr++;  /* space for word count */
        for (chan=0; chan<16; chan++) {
            /*
             * for each channel we need three derived values: group space start
             *     group  ==FPGA ==chan/4 (0..4)
             *     space  mem1 (==0), mem2 (==1), statistics (==3)
             *     space  ==(chan/2)%2
             *     start  ==(chan%2)*0x2000000+bank*0x1000000;
             */
            int group=chan/4; /* ADC FPGA */
            int space=(chan/2)%2; /* memory inside group */
            ems_u32 start=(chan%2)*0x2000000+priv->previous_bank*0x1000000;
            ems_u32 *help_ch=outptr++;
            int num=address_vals[chan]&0xffffff;

            pres=sis3316_read_mem(module, group, space, start, outptr, num);
            if (pres!=plOK) {
                if (pres==plErr_Timeout) {
                    complain("sis3316_readout: read_mem: timeout; "
                            "event discarded");
                    /* force event to be empty */
                    outptr=orig_outptr;
                    /* don't stop readout */
                    pres=plOK;
                    goto error;
                } else {
                    complain("sis3316_readout: read_mem failed");
                }
                goto error;
            }
            *help_ch=num;
            outptr+=num;
#if 0
            printf("mod %d chan %d num=%d\n", priv->serial, chan, num);
#endif
        }
        *help=outptr-help-1;
    }

error:
    /* too much data in error output are dangerous */
    if (pres!=plOK && (outptr>orig_outptr+10))
        outptr=orig_outptr+10;
    return pres;
}

plerrcode
test_proc_sis3316_readout_udp(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    unsigned int m;

    if (ip[0]<1) {
        complain("sis3316_readout_udp: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }
    if (ip[1]<0) {
        complain("sis3316_readout_udp: -1 not allowed as modules address");
        return plErr_ArgRange;
    }

    for (m=1; m<=p[0]; m++) {
        if ((pres=check_sis3316(p[m], 0, 0, "sis3316_readout_udp"))!=plOK)
            return pres;
    }

    wirbrauchen=-1;
    return plOK;
}

char name_proc_sis3316_readout_udp[] = "sis3316_readout_udp";
int ver_proc_sis3316_readout_udp = 1;
/*****************************************************************************/
/*
 * p[0]: argcount(3 or more)
 * p[1]: value for the acquisition control register (0x60)
 * p[2...]: index in memberlist (or -1 for all sis3316 modules)
 *
 * The logic is explained at page 28 of the manual (sis3316-m-1-1-v117.pdf).
 * 1..3: to be done here
 *        disarm or reset command
 *        set address threshold registers
 *        disarm active rank and arm rank2 command (start with the second bank)
 * 4: done in trigger procedure (trig_sis3316_poll)
 * 5..8: to be done in the readout procedure
 *
 * Herausforderung: p[1...] kann -1 sein (komplette Memberlist)
 *   oder eine beliebige Liste von Modulen.
 *   Es koennen sis3316-Modules sein, multicast- oder cblt-Module.
 *   Manche Kommandos muessen an alle sis3316-Module gehen, manche
 *   solten an die multicast-Adresse gehen (wenn vorhanden), sonst
 *   an alle sis3316-Module.
 *   Manche Kommandos duerfen nur an das erste Modul gehen (und werden
 *   ueber FP-Bus oder LEMO verteilt.
 * Implementiert ist das im Moment nicht, es wird nur erstes oder alle
 * Module verwendet.
 *
 * Die Liste muss also alle sis3316-Module enthalten (kein -1) und das
 * erste Modul muss der FP-Bus-Master sein.
 */
plerrcode
proc_sis3316_readout_start(ems_u32* p)
{
    ml_entry* module;
    struct sis3316_priv_data *priv;
    plerrcode pres=plOK;
    unsigned int m;

#if 0
    /* find a multicast module in the member list (if any) */
    mcst_mod=find_mcst_mod(p[1]);
    ... to be implemented later
#endif

    /* disarm */
    for (m=2; m<=p[0]; m++) {
        module=ModulEnt(p[m]);
        priv=(struct sis3316_priv_data*)module->private_data;

        priv->last_acq_state_valid=0;

        pres=sis3316_write_reg(module, 0x414, 0);
        if (pres!=plOK) {
            complain("sis3316_readout_start(ser. %d): cannot disarm",
                    priv->serial);
            goto error;
        }
    }

    /* write acquisition control register */
    for (m=2; m<=p[0]; m++) {
        module=ModulEnt(p[m]);

        pres=sis3316_write_reg(module, 0x60, p[1]);
        if (pres!=plOK) {
            complain("sis3316_readout_start(ser. %d): cannot write acq reg",
                    priv->serial);
            goto error;
        }
    }

    /* clear timestamp counter */
    module=ModulEnt(p[2]);
    pres=sis3316_write_reg(module, 0x41c, 0);
    if (pres!=plOK) {
        complain("sis3316_readout_start(ser. %d): cannot clear timestamp",
                priv->serial);
        goto error;
    }

    /* disarm bankx and arm bank2 */
    module=ModulEnt(p[2]);
    pres=sis3316_write_reg(module, 0x424, 0);
    if (pres!=plOK) {
        complain("sis3316_readout_start(ser. %d): cannot arm bank2",
                priv->serial);
        goto error;
    }

error:
    return pres;
}

plerrcode
test_proc_sis3316_readout_start(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;
    unsigned int m;

    if (p[0]<2) {
        complain("sis3316_readout_start: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }
    if (p[0]==2 && ip[2]==-1) {
#if 0
        pres=check_sis3316(ip[2], 0, 0, "sis3316_readout_start");
#else
        complain("sis3316_readout_start: -1 as module is not allowed yet");
#endif
    } else {
        for (m=2; m<=p[0]; m++) {
            if (ip[m]<0) {
                complain("sis3316_readout_start: -1 as module is not allowed "
                        "if more than one argument is given");
                pres=plErr_ArgRange;
            } else {
                pres=check_sis3316(p[m], 0, 0, "sis3316_readout_start");
            }
            if (pres!=plOK)
                break;
        }
    }

    wirbrauchen=0;
    return pres;
}

char name_proc_sis3316_readout_start[] = "sis3316_readout_start";
int ver_proc_sis3316_readout_start = 1;
/*****************************************************************************/
/*
 * p[0]: argcount(3 or more)
 * p[1]: flags
 * p[2]: value for the acquisition control register (0x60)
 * p[3...]: index in memberlist
 *          only one module is usefull here
 *
 * The logic is explained at page 28 of the manual (sis3316-m-1-1-v117.pdf).
 * 1..3: to be done here
 *        disarm or reset command
 *        set address threshold registers
 *        disarm active rank and arm rank2 command (start with the second bank)
 * 4: done in trigger procedure (trig_sis3316_poll)
 * 5..8: to be done in the readout procedure
 *
 */
plerrcode
proc_sis3316_readout_start_split(ems_u32* p)
{
    ml_entry* module;
    struct sis3316_priv_data *priv;
    plerrcode pres=plOK;
    unsigned int m;
    ems_u32 flags=p[1];

    /* disarm */
    for (m=3; m<=p[0]; m++) {
        module=ModulEnt(p[m]);
        priv=(struct sis3316_priv_data*)module->private_data;

        priv->last_acq_state_valid=0;

        pres=sis3316_write_reg(module, 0x414, 0);
        if (pres!=plOK) {
            complain("sis3316_readout_start_split(ser. %d): cannot disarm",
                    priv->serial);
            goto error;
        }
    }

    /* write acquisition control register */
    for (m=3; m<=p[0]; m++) {
        module=ModulEnt(p[m]);

        pres=sis3316_write_reg(module, 0x60, p[2]);
        if (pres!=plOK) {
            complain("sis3316_readout_start_split(ser. %d): cannot write acq reg",
                    priv->serial);
            goto error;
        }
    }


    /* clear timestamp counter */
       /* Darf nur das erste Modul! */
    if (flags&1) {
        module=ModulEnt(p[3]);
        pres=sis3316_write_reg(module, 0x41c, 0);
        if (pres!=plOK) {
            complain("sis3316_readout_start_split(ser. %d):"
                    "cannot clear timestamp",
                    priv->serial);
            goto error;
        }
    }

    /* disarm bankx and arm bank2 */
    module=ModulEnt(p[3]);
    pres=sis3316_write_reg(module, 0x424, 0);
    if (pres!=plOK) {
        complain("sis3316_readout_start_split(ser. %d): cannot arm bank2",
                priv->serial);
        goto error;
    }

error:
    return pres;
}

plerrcode
test_proc_sis3316_readout_start_split(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;
    unsigned int m;

    if (p[0]<3) {
        complain("sis3316_readout_start_split: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }
    if (p[0]==3 && ip[3]==-1) {
#if 0
        pres=check_sis3316(ip[2], 0, 0, "sis3316_readout_start_split");
#else
        complain("sis3316_readout_start_split: -1 as module is not allowed yet");
#endif
    } else {
        for (m=3; m<=p[0]; m++) {
            if (ip[m]<0) {
                complain("sis3316_readout_start_split: -1 as module is not allowed "
                        "if more than one argument is given");
                pres=plErr_ArgRange;
            } else {
                pres=check_sis3316(p[m], 0, 0, "sis3316_readout_start_split");
            }
            if (pres!=plOK)
                break;
        }
    }

    wirbrauchen=0;
    return pres;
}

char name_proc_sis3316_readout_start_split[] = "sis3316_readout_start_split";
int ver_proc_sis3316_readout_start_split = 1;
/*****************************************************************************/
//#include "readout_interleaved.code"
/*****************************************************************************/
/**
 * p[0]: argcount(==2)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: channel group (or -1 for all)
 */
plerrcode proc_sis3316_statistics(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_statistics, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        int group, num=24;

        if (member_idx()<0)
            *outptr++=1; /* module count if p[1]>=0 */
        if (ip[2]<0) {
            for (group=0; group<4; group++) {
                pres=sis3316_read_mem(module, group, 3, 0, outptr+1, num);
                if (pres==plOK) {
                    *outptr++=num;
                    outptr+=num;
                } else {
                    if (pres==plErr_Timeout) {
                        complain("sis3316_statistics: read_mem: timeout; "
                                "data discarded");
                        /* don't stop readout */
                        pres=plOK;
                    }
                    *outptr++=0;
                }
            }
        } else {
            pres=sis3316_read_mem(module, p[2], 3, 0, outptr+1, num);
            if (pres==plOK) {
                *outptr++=num;
                outptr+=num;
            } else {
                if (pres==plErr_Timeout) {
                    complain("sis3316_statistics: read_mem: timeout; "
                            "data discarded");
                    /* don't stop readout */
                    pres=plOK;
                }
                *outptr++=0;
            }

        }
    }

    return pres;
}

plerrcode test_proc_sis3316_statistics(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=2) {
        complain("sis3316_statistics: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_statistics"))!=plOK)
        return pres;

    if (ip[2]>3) {
        complain("sis3316_statistics: illegal group: %d", p[2]);
        return plErr_ArgRange;
    }

    wirbrauchen=25*num+1;
    return plOK;
}

char name_proc_sis3316_statistics[] = "sis3316_statistics";
int ver_proc_sis3316_statistics = 1;
/*****************************************************************************/
/*****************************************************************************/
/**
 * p[0]: argcount(2 or 3)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * [p[2]: val]
 *        bits:
 *          0 : swith on LED U
 *          1 : swith on LED 1
 *          2 : swith on LED 2
 *
 *          4 : set LED U aplication mode
 *          5 : set LED 1 aplication mode
 *          6 : set LED 2 aplication mode
 *
 *          16: swith off LED U
 *          17: swith off LED 1
 *          18: swith off LED 2
 *
 *          19: clear LED aplication mode
 *          20: clear LED U aplication mode
 *          21: clear LED 1 aplication mode
 *          22: clear LED 2 aplication mode
 */
plerrcode proc_sis3316_led(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;
    const ems_u32 mask=0x007f0077; /* only LED bits allowed */

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_led, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);

        if (p[0]>1) {
            pres=sis3316_write_reg(module, 0x0, p[2]&mask);
        } else {
            pres=sis3316_read_reg(module, 0x0, outptr);
            if (pres==plOK)
                outptr++;
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_led(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]<1 || p[0]>2) {
        complain("sis3316_led: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_led"))!=plOK)
        return pres;

    wirbrauchen=num+1;
    return plOK;
}

char name_proc_sis3316_led[] = "sis3316_led";
int ver_proc_sis3316_led = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(2 or 4)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: level (level==0 disables IRQ)
 * [p[3]: vector
 *         <0: -vector is used for all modules
 *         >=0: vector is used for the first module and incremented for each
 *              following module
 *  p[4]: 0: RORA; 1: ROAK]
 */
plerrcode proc_sis3316_irq(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_irq, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);

        if (!p[2]) {
            pres=sis3316_write_reg(module, 0x8, 0);
        } else {
            ems_u32 vector, val;
            val=p[2]<<8;
            if (ip[3]<0) {
                vector=-ip[3];
            } else {
                vector=ip[3];
                if (member_idx()>=0)
                    vector+=member_idx();
            }
            val|=vector;
            val|=1<<11;     /* enable IRQ */
            if (p[4])
                val|=1<<12; /* ROAK */

            pres=sis3316_write_reg(module, 0x8, val);
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_irq(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;

    if (p[0]!=2 && p[0]!=4) {
        complain("sis3316_irq: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (p[2] && p[0]!=4) {
        complain("sis3316_irq: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (p[2]) {
        if (p[2]>7) {
            complain("sis3316_irq: illegal value for level: %d", p[2]);
            return plErr_ArgRange;
        }
        if (p[3]>0xff) {
            complain("sis3316_irq: illegal value for vector: %d", p[3]);
            return plErr_ArgRange;
        }
    }

    wirbrauchen=0;
    return check_sis3316(ip[1], 0, 0, "sis3316_irq");
}

char name_proc_sis3316_irq[] = "sis3316_irq";
int ver_proc_sis3316_irq = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(2 or 3)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * [p[2]: val]
 */
plerrcode proc_sis3316_irq_ctrl(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_irq_ctrl, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);

        if (p[0]>1) {
            pres=sis3316_write_reg(module, 0xc, p[2]);
        } else {
            pres=sis3316_read_reg(module, 0xc, outptr);
            if (pres==plOK)
                outptr++;
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_irq_ctrl(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]<1 || p[0]>2) {
        complain("sis3316_irq_ctrl: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_irq_ctrl"))!=plOK)
        return pres;

    wirbrauchen=num+1;
    return plOK;
}

char name_proc_sis3316_irq_ctrl[] = "sis3316_irq_ctrl";
int ver_proc_sis3316_irq_ctrl = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(==3)
 * p[1]: index in memberlist (or -1 for all modules)
 * p[2]: mcst_module
 * [p[3]: 0: disable broadcast 1: enable 2: enable+master
 *        enable is default]
 * if p[1]==-1 the first module will become the master
 */
plerrcode proc_sis3316_init_mcast(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

printf("sis3316_init_mcast(%d)\n", p[1]);

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_init_mcast, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 val;


        if (p[1]>2 && p[3]==0) {       /* disable multicast */
            val=0;
        } else {
            ml_entry* mcstmod=ModulEnt(p[2]);
            val=mcstmod->address.vme.addr&0xff000000;
            val|=0x10;                 /* enable multicast */
            if (member_idx()<0) {      /* for_each_member not active */
                if (p[1]>2 && p[3]==2) /* master */
                    val|=0x20;
            } else {
                if (member_idx()==0)   /* first module --> master */
                    val|=0x20;
            }
        }
        if ((pres=sis3316_write_ctrl(module, 0x14, val))!=plOK)
            complain("sis3316_init_mcast: error writing register");
    }

    return pres;
}

plerrcode test_proc_sis3316_init_mcast(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;

    if (p[0]!=2 && p[0]!=3)
        return plErr_ArgNum;

    if (p[1]<3 || p[3]!=0) { /* check the multicast module */
        ml_entry* module;
        module=ModulEnt(p[2]);
        if (module->modultype!=vme_mcst) {
            complain("sis3316_init_mcast: p[2](==%u): no mcst module", p[2]);
            return plErr_BadModTyp;
        }
    }

    wirbrauchen=0;
    return check_sis3316(ip[1], 0, 0, "sis3316_init_mcast");
}

char name_proc_sis3316_init_mcast[] = "sis3316_init_mcast";
int ver_proc_sis3316_init_mcast = 1;
/*****************************************************************************/
/*****************************************************************************/
/**
 * p[0]: argcount (1 or 3)
 * p[1]: index in memberlist (or -1 for all modules)
 * p[2]: transmit gap
 * p[3]: jumbo frames
 * p[4]: max. number of UDP packets per request (<=32)
 */
plerrcode proc_sis3316_udp_config(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_udp_config, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct sis3316_priv_data *priv=
                (struct sis3316_priv_data*)module->private_data;
        ems_u32 val;

        if (p[0]==1) {
            pres=sis3316_read_ctrl(module, 0x8, &val);
            if (pres==plOK) {
                *outptr++=val;
                *outptr++=priv->max_udp_packets;
            }
        } else {
            val=p[2]&0xf;  /* transmit gap */
            if (p[3])      /* jumbo frames */
                val|=0x10;
            pres=sis3316_write_ctrl(module, 0x8, val);
            if (pres==plOK) {
                priv->jumbo_enabled=!!p[3];
                priv->max_udp_packets=p[4];
            }
        }
    }
    return pres;
}

plerrcode test_proc_sis3316_udp_config(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=1 && p[0]!=4) {
        complain("sis3316_udp_config: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }
    if (p[0]>1) {
        if (p[2]>0xf) {
            *outptr++=2;
            return plErr_ArgRange;
        }
        if (p[4]>32) {
            *outptr++=4;
            return plErr_ArgRange;
        }
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_udp_config"))!=plOK)
        return pres;

    wirbrauchen=num+1;
    return plOK;
}

char name_proc_sis3316_udp_config[] = "sis3316_udp_config";
int ver_proc_sis3316_udp_config = 1;
/*****************************************************************************/
/**
 * p[0]: argcount (==1)
 * p[1]: index in memberlist (or -1 for all modules)
 */
plerrcode proc_sis3316_udp_ack_status(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_udp_ack_status, 2);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 val;

        pres=sis3316_read_ctrl(module, 0xc, &val);
        if (pres==plOK)
            *outptr++=val;
    }
    return pres;
}

plerrcode test_proc_sis3316_udp_ack_status(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=1) {
        complain("sis3316_udp_ack_status: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_udp_ack_status"))!=plOK)
        return pres;

    wirbrauchen=num+1;
    return plOK;
}

char name_proc_sis3316_udp_ack_status[] = "sis3316_udp_ack_status";
int ver_proc_sis3316_udp_ack_status = 1;
/*****************************************************************************/
/**
 * p[0]: argcount (==1)
 * p[1]: index in memberlist (or -1 for all modules)
 */
plerrcode proc_sis3316_udp_errors(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_udp_errors, 2);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 val;

        pres=sis3316_read_ctrl(module, 0xc, &val);
        if (pres==plOK) {
            *outptr++=(val>>28)&0xf;  /* Tx error */
            *outptr++=(val>>24)&0xf;  /* Rx error */
            *outptr++=(val>>16)&0xff; /* GMII Badframe error */
            *outptr++=(val>>8) &0xff; /* GMII Tx error */
            *outptr++= val     &0xff; /* GMII Rx error */
        }
    }
    return pres;
}

plerrcode test_proc_sis3316_udp_errors(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=1) {
        complain("sis3316_udp_errors: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_udp_errors"))!=plOK)
        return pres;

    wirbrauchen=5*num+1;
    return plOK;
}

char name_proc_sis3316_udp_errors[] = "sis3316_udp_errors";
int ver_proc_sis3316_udp_errors = 1;
/*****************************************************************************/
/*****************************************************************************/
/**
 * p[0]: argcount(==1)
 * p[1]: index in memberlist (or -1 for all modules)
 *
 * the temperature is originally measured in units of 0.25 C
 * we return T*25 --> in units of 0.01 C
 */
plerrcode proc_sis3316_temperature(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_temperature, 2);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 val;

        pres=sis3316_read_reg(module, 0x20, &val);
        if (pres==plOK)
            *outptr++=val*25;
    }
    return pres;
}

plerrcode test_proc_sis3316_temperature(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=1) {
        complain("sis3316_temperature: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_temperature"))!=plOK)
        return pres;

    wirbrauchen=num+1;
    return plOK;
}

char name_proc_sis3316_temperature[] = "sis3316_temperature";
int ver_proc_sis3316_temperature = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(==1)
 * p[1]: index in memberlist (or -1 for all modules)
 */
plerrcode proc_sis3316_serial(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_serial, 2);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 val;

        pres=sis3316_read_reg(module, 0x28, &val);
        if (pres==plOK) {
            if (val&0x10000) {
                complain("sis3316_serial: number not valid (val=%08x)", val);
                return plErr_HW;
            }
            *outptr++=val&0xffff;
        }
    }
    return pres;
}

plerrcode test_proc_sis3316_serial(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=1) {
        complain("sis3316_serial: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_serial"))!=plOK)
        return pres;

    wirbrauchen=num+1;
    return plOK;
}

char name_proc_sis3316_serial[] = "sis3316_serial";
int ver_proc_sis3316_serial = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(==1)
 * p[1]: index in memberlist (or -1 for all modules)
 */
plerrcode proc_sis3316_memsize(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_memsize, 2);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 val;

        pres=sis3316_read_reg(module, 0x28, &val);
        if (pres==plOK)
            *outptr++=(val&0x800000)?512:256;
    }
    return pres;
}

plerrcode test_proc_sis3316_memsize(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=1) {
        complain("sis3316_memsize: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_memsize"))!=plOK)
        return pres;

    wirbrauchen=num+1;
    return plOK;
}

char name_proc_sis3316_memsize[] = "sis3316_memsize";
int ver_proc_sis3316_memsize = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(==1)
 * p[1]: index in memberlist (or -1 for all modules)
 *
 * this procedure reads the firmware version of the VME FPGA
 */
plerrcode proc_sis3316_version_vme(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_version_vme, 2);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 val;

        pres=sis3316_read_ctrl(module, 0x4, &val);
        if (pres==plOK)
            *outptr++=val&0xffff;
    }
    return pres;
}

plerrcode test_proc_sis3316_version_vme(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=1) {
        complain("sis3316_version_vme: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_version_vme"))!=plOK)
        return pres;

    wirbrauchen=num+1;
    return plOK;
}

char name_proc_sis3316_version_vme[] = "sis3316_version_vme";
int ver_proc_sis3316_version_vme = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(==1)
 * p[1]: index in memberlist (or -1 for all modules)
 *
 * this procedure reads the firmware version of the ADC FPGAs
 * (it is assumed that all four versions are identical, therefore the
 * version of the first ADC FPGA only is used)
 */
plerrcode proc_sis3316_version_adc(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_version_adc, 2);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 val;

        pres=sis3316_read_reg(module, 0x1100, &val);
        if (pres==plOK)
            *outptr++=val;
    }
    return pres;
}

plerrcode test_proc_sis3316_version_adc(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=1) {
        complain("sis3316_version_adc: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_version_adc"))!=plOK)
        return pres;

    wirbrauchen=num+1;
    return plOK;
}

char name_proc_sis3316_version_adc[] = "sis3316_version_adc";
int ver_proc_sis3316_version_adc = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(==1)
 * p[1]: index in memberlist (or -1 for all modules)
 *
 * this procedure reads the hardware version of the module
 */
plerrcode proc_sis3316_version_hw(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_version_hw, 2);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 val;

        pres=sis3316_read_ctrl(module, 0x1c, &val);
        if (pres==plOK) {
            *outptr++=val&0xf;
        }
    }
    return pres;
}

plerrcode test_proc_sis3316_version_hw(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=1) {
        complain("sis3316_version_hw: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_version_hw"))!=plOK)
        return pres;

    wirbrauchen=num+1;
    return plOK;
}

char name_proc_sis3316_version_hw[] = "sis3316_version_hw";
int ver_proc_sis3316_version_hw = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(1 or 2)
 * p[1]: index in memberlist (or -1 for all modules)
 * p[2]: val
 */
plerrcode proc_sis3316_veto_delay(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_veto_delay, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);

        if (p[0]>1) {
            pres=sis3316_write_reg(module, 0x3c, p[2]);
        } else {
            pres=sis3316_read_reg(module, 0x3c, outptr);
            if (pres==plOK)
                outptr++;
        }
    }
    return pres;
}

plerrcode test_proc_sis3316_veto_delay(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=1 && p[0]!=2) {
        complain("sis3316_veto_delay: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_veto_delay"))
            !=plOK)
        return pres;

    wirbrauchen=num+1;
    return plOK;
}

char name_proc_sis3316_veto_delay[] = "sis3316_veto_delay";
int ver_proc_sis3316_veto_delay = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(1 or 2)
 * p[1]: index in memberlist (or -1 for all modules)
 * [p[2]: 0: 62.5 MHz 1: 128 MHz 2: 250 MHz]
 */
plerrcode proc_sis3316_clock(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_clock, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);

        if (p[0]>1) {
            pres=sis3316_set_clock(module, 0, p[2]);
        } else {
            pres=sis3316_get_clock(module, 0, outptr);
            if (pres==plOK)
                outptr++;
        }
    }
    return pres;
}

plerrcode test_proc_sis3316_clock(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=1 && p[0]!=2) {
        complain("sis3316_clock: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_clock"))!=plOK)
        return pres;

    if (p[0]>1 && p[2]>2) {
        complain("sis3316_clock: illegal frequency argument: %d", p[2]);
        return plErr_ArgRange;
    }

    wirbrauchen=num+1;
    return plOK;
}

char name_proc_sis3316_clock[] = "sis3316_clock";
int ver_proc_sis3316_clock = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(1 or 2)
 * p[1]: index in memberlist (or -1 for all modules)
 * p[2]: frequency in Hz
 */
plerrcode proc_sis3316_clock_any(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_clock_any, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        pres=sis3316_set_clock_any(module, 0, p[2]);
    }
    return pres;
}

plerrcode test_proc_sis3316_clock_any(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;

    if (p[0]!=2) {
        complain("sis3316_clock_any: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    wirbrauchen=0;
    return check_sis3316(ip[1], 0, 0, "sis3316_clock_any");
}

char name_proc_sis3316_clock_any[] = "sis3316_clock_any";
int ver_proc_sis3316_clock_any = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(1)
 * p[1]: index in memberlist (or -1 for all modules)
 */
plerrcode proc_sis3316_clock_reset(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_clock_reset, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        pres=sis3316_reset_clock(module, 0);
    }
    return pres;
}

plerrcode test_proc_sis3316_clock_reset(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;

return plErr_NotImpl;

    if (p[0]!=1) {
        complain("sis3316_clock_reset: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    wirbrauchen=0;
    return check_sis3316(ip[1], 0, 0, "sis3316_clock_reset");
}

char name_proc_sis3316_clock_reset[] = "sis3316_clock_reset";
int ver_proc_sis3316_clock_reset = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(1 or 2)
 * p[1]: index in memberlist (or -1 for all modules)
 * p[2]: val
 *       0: onboard oscillator
 *       1: VXS clock
 *       2: FP-bus clock
 *       3: NIM input
 */
plerrcode proc_sis3316_clock_distribution(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_clock_distribution, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);

        if (p[0]>1) {
            pres=sis3316_write_reg(module, 0x50, p[2]);
        } else {
            pres=sis3316_read_reg(module, 0x50, outptr);
            if (pres==plOK)
                outptr++;
        }
    }
    return pres;
}

plerrcode test_proc_sis3316_clock_distribution(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=1 && p[0]!=2) {
        complain("sis3316_clock_distribution: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_clock_distribution"))
            !=plOK)
        return pres;

    wirbrauchen=num+1;
    return plOK;
}

char name_proc_sis3316_clock_distribution[] = "sis3316_clock_distribution";
int ver_proc_sis3316_clock_distribution = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(1 or 2)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * [p[2]: val]
 *        bits:
 *          0: enable control lines output
 *          1: enable status lines output
 *
 *          4: enable sample clock to the FP bus
 *          5: source of the FP bus sample clock
 *             source: 0: onbord oscillator
 *                     1: external clock from NIM connector CI
 *
 * bit 0 and 4 must be set on one module only
 * if p[1]<0 (all modules) and bit 0 or bit 4 are set they are set
 * on the first module only
 */
plerrcode proc_sis3316_fb_control(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;
    const ems_u32 mask0=0x33;  /* defined bits only  */
    const ems_u32 mask1=~0x11; /* reset bits 0 and 4 */

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_fb_control, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);

printf("fb_control: p[0]=%d p[1]=%d member_idx=%d\n", p[0], p[1], member_idx());

        if (p[0]>1) {
            ems_u32 val=p[2]&mask0;
            if (member_idx()>0)
                val&=mask1;
            pres=sis3316_write_reg(module, 0x58, val);
        } else {
            pres=sis3316_read_reg(module, 0x58, outptr);
            if (pres==plOK)
                outptr++;
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_fb_control(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]<1 || p[0]>2) {
        complain("sis3316_fb_control: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_fb_control"))!=plOK)
        return pres;

    wirbrauchen=num+1;
    return plOK;
}

char name_proc_sis3316_fb_control[] = "sis3316_fb_control";
int ver_proc_sis3316_fb_control = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(2 or 3)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * [p[2]: val]
 *        bits:
 *           0: enable NIM input CI
 *           1: invert NIM input CI
 *           2: NIM input CI level sensitive
 *           3: set NIM input CI function
 *           4: enable NIM input TI as trigger
 *           5: invert NIM input TI
 *           6: NIM input TI level sensitive
 *           7: set NIM input TI function
 *           8: enable NIM input UI as timestamp clear enable
 *           9: invert NIM input UI
 *          10: NIM input UI level sensitive
 *          11: set NIM input UI function
 *          12: NIM input UI as veto enable
 *          13: NIM input UI as PPS enable
 *
 *          16: status of external NIM input CI
 *          17: status of NIM input signal CI
 *
 *          20: status of external NIM input TI
 *          21: status of NIM input signal TI
 *
 *          24: status of external NIM input UI
 *          25: status of NIM input signal UI
 */
plerrcode proc_sis3316_lemo_in(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_lemo_in, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);

        if (p[0]>1) {
            pres=sis3316_write_reg(module, 0x5c, p[2]);
        } else {
            pres=sis3316_read_reg(module, 0x5c, outptr);
            if (pres==plOK)
                outptr++;
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_lemo_in(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]<1 || p[0]>2) {
        complain("sis3316_lemo_in: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_lemo_in"))!=plOK)
        return pres;

    wirbrauchen=num+1;
    return plOK;
}

char name_proc_sis3316_lemo_in[] = "sis3316_lemo_in";
int ver_proc_sis3316_lemo_in = 1;
/*****************************************************************************/
/**
 * p[0]: argcount (1 or 2)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * [p[2]: val]
 */
plerrcode proc_sis3316_acq_ctrl(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_acq_ctrl, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);

        if (p[0]>1) {
            pres=sis3316_write_reg(module, 0x60, p[2]);
        } else {
            pres=sis3316_read_reg(module, 0x60, outptr);
            if (pres==plOK) {
                ems_u32 armed=((*outptr)>>16)&3;
                printf("acq_ctrl: armed bank=%d\n", armed);
                outptr++;
            }
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_acq_ctrl(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]<1 || p[0]>2) {
        complain("sis3316_acq_ctrl: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_acq_ctrl"))!=plOK)
        return pres;

    wirbrauchen=num+1;
    return plOK;
}

char name_proc_sis3316_acq_ctrl[] = "sis3316_acq_ctrl";
int ver_proc_sis3316_acq_ctrl = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(2 or 3)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * [p[2]: val]
 *        bits:
 *           0: select sample clock
 *          16: select internal high energy trigger stretched pulse ch1-4
 *          17: select internal high energy trigger stretched pulse ch5-8
 *          18: select internal high energy trigger stretched pulse ch9-12
 *          19: select internal high energy trigger stretched pulse ch13-16
 *          20: select "sample bank swap control with nim input ti/ui"
 *              logic enabled
 *          21: select sample logic bankx armed
 *          22: select sample logic bank2 flag
 */
plerrcode proc_sis3316_lemo_out_co(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_lemo_out_co, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);

        if (p[0]>1) {
            pres=sis3316_write_reg(module, 0x70, p[2]);
        } else {
            pres=sis3316_read_reg(module, 0x70, outptr);
            if (pres==plOK)
                outptr++;
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_lemo_out_co(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]<1 || p[0]>2) {
        complain("sis3316_lemo_out_co: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_lemo_out_co"))!=plOK)
        return pres;

    wirbrauchen=num+1;
    return plOK;
}

char name_proc_sis3316_lemo_out_co[] = "sis3316_lemo_out_co";
int ver_proc_sis3316_lemo_out_co = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(2 or 3)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * [p[2]: val]
 *        bits:
 */
plerrcode proc_sis3316_lemo_out_to(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_lemo_out_to, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);

        if (p[0]>1) {
            pres=sis3316_write_reg(module, 0x74, p[2]);
        } else {
            pres=sis3316_read_reg(module, 0x74, outptr);
            if (pres==plOK)
                outptr++;
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_lemo_out_to(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]<1 || p[0]>2) {
        complain("sis3316_lemo_out_to: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_lemo_out_to"))!=plOK)
        return pres;

    wirbrauchen=num+1;
    return plOK;
}

char name_proc_sis3316_lemo_out_to[] = "sis3316_lemo_out_to";
int ver_proc_sis3316_lemo_out_to = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(2 or 3)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * [p[2]: val]
 *        bits:
 */
plerrcode proc_sis3316_lemo_out_uo(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_lemo_out_uo, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);

        if (p[0]>1) {
            pres=sis3316_write_reg(module, 0x78, p[2]);
        } else {
            pres=sis3316_read_reg(module, 0x78, outptr);
            if (pres==plOK)
                outptr++;
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_lemo_out_uo(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]<1 || p[0]>2) {
        complain("sis3316_lemo_out_uo: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_lemo_out_uo"))!=plOK)
        return pres;

    wirbrauchen=num+1;
    return plOK;
}

char name_proc_sis3316_lemo_out_uo[] = "sis3316_lemo_out_uo";
int ver_proc_sis3316_lemo_out_uo = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(2 or 3)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * [p[2]: val]
 */
plerrcode proc_sis3316_trigg_feedback(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_trigg_feedback, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);

        if (p[0]>1) {
            pres=sis3316_write_reg(module, 0x7c, p[2]);
        } else {
            pres=sis3316_read_reg(module, 0x7c, outptr);
            if (pres==plOK)
                outptr++;
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_trigg_feedback(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]<1 || p[0]>2) {
        complain("sis3316_trigg_feedback: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_trigg_feedback"))!=plOK)
        return pres;

    wirbrauchen=num+1;
    return plOK;
}

char name_proc_sis3316_trigg_feedback[] = "sis3316_trigg_feedback";
int ver_proc_sis3316_trigg_feedback = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(2 or 3)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * [p[2]: divider (sample_clock/(divider+1); divider==0: disable)
 *  p[3]: length (in sample clock units)]
 */
plerrcode proc_sis3316_prescaler(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_prescaler, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 regs[2]={0xb8, 0xbc};
        ems_u32 vals[2];

        if (p[0]>1) {
            vals[0]=p[2];
            vals[1]=p[3];
            pres=sis3316_write_regs(module, regs, vals, 2);
        } else {
            pres=sis3316_read_regs(module, regs, outptr, 2);
            if (pres==plOK)
                outptr+=2;
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_prescaler(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=1 && p[0]!=4) {
        complain("sis3316_prescaler: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_prescaler"))!=plOK)
        return pres;

    wirbrauchen=2*num+1;
    return plOK;
}

char name_proc_sis3316_prescaler[] = "sis3316_prescaler";
int ver_proc_sis3316_prescaler = 1;
/*****************************************************************************/
/**
 * p[0]: argcount (==1)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 */
plerrcode proc_sis3316_itc(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_itc, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 regs[16]={
            0xc0, 0xc4, 0xc8, 0xcc,
            0xd0, 0xd4, 0xd8, 0xdc,
            0xe0, 0xe4, 0xe8, 0xec,
            0xf0, 0xf4, 0xf8, 0xfc,
        };

        pres=sis3316_read_regs(module, regs, outptr, 16);
        if (pres==plOK)
            outptr+=16;
    }

    return pres;
}
plerrcode test_proc_sis3316_itc(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=1) {
        complain("sis3316_itc: illegal # of arguments: %d",
                p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_itc"))
            !=plOK)
        return pres;

    wirbrauchen=16*num+1;
    return plOK;
}

char name_proc_sis3316_itc[] = "sis3316_itc";
int ver_proc_sis3316_itc = 1;
/*****************************************************************************/
/*****************************************************************************/
/**
 * sis3316_trigger just issues a Key Register Trigger command
 * p[0]: argcount(==1)
 * p[1]: index in memberlist (or -1 for all modules)
 */
plerrcode proc_sis3316_trigger(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_trigger, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);

        pres=sis3316_write_reg(module, 0x418, 0);
    }

    return pres;
}

plerrcode test_proc_sis3316_trigger(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;

    if (p[0]!=1)
        return plErr_ArgNum;

    wirbrauchen=0;
    return check_sis3316(ip[1], 1, 0, "sis3316_trigger");
}

char name_proc_sis3316_trigger[] = "sis3316_trigger";
int ver_proc_sis3316_trigger = 1;
/*****************************************************************************/
/**
 * sis3316_clear_timestamp just issues a Key Register clear timestamp command
 * p[0]: argcount(==1)
 * p[1]: index in memberlist (or -1 for all modules)
 *
 * !!! This procedure can not work!
 * We have to clear the timestamps of all modules at the same time.
 * Therfore we have to use a VME multyicast or the FP ECL bus.
 */
plerrcode proc_sis3316_clear_timestamp(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_clear_timestamp, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);

        pres=sis3316_write_reg(module, 0x41c, 0);
    }

    return pres;
}

plerrcode test_proc_sis3316_clear_timestamp(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;

    if (p[0]!=1)
        return plErr_ArgNum;

    wirbrauchen=0;
    return check_sis3316(ip[1], 1, 0, "sis3316_clear_timestamp");
}

char name_proc_sis3316_clear_timestamp[] = "sis3316_clear_timestamp";
int ver_proc_sis3316_clear_timestamp = 1;
/*****************************************************************************/
/**
 * sis3316_disarm_arm just issues a Key Register disarm arm command
 * p[0]: argcount(==1)
 * p[1]: index in memberlist (or -1 for all modules)
 * p[2]: bank: 0, 1, -1 (-1: die andere)
 */
plerrcode proc_sis3316_disarm_arm(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

#if 1
        printf("sis3316_disarm_arm[%d]: bank %d\n", ip[1], p[2]);
#endif
    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_disarm_arm, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        if (ip[2]<0) {

            ems_u32 acqstate;
            int bank;

            /* read ACQ status */
            pres=sis3316_read_reg(module, 0x60, &acqstate);
            if (pres!=plOK) {
                complain("sis3316 restart: cannot read ACQ status");
                return plErr_System;
            }
            bank=!!(acqstate&(1<<17)); /* actual bank */
                //printf("acqstate   : %08x\n", acqstate);
                //printf("actual bank: %d\n", bank);
            sis3316_write_reg(module, bank?0x420:0x424, 0);
dump_acq_state(module, "before disarm_arm");

        } else {
dump_acq_state(module, "before disarm_arm");
        pres=sis3316_write_reg(module, p[2]?0x424:0x420, 0);
dump_acq_state(module, "after disarm_arm");
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_disarm_arm(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;

    if (p[0]!=2)
        return plErr_ArgNum;

    wirbrauchen=0;
    return check_sis3316(ip[1], 1, 0, "sis3316_disarm_arm");
}

char name_proc_sis3316_disarm_arm[] = "sis3316_disarm_arm";
int ver_proc_sis3316_disarm_arm = 1;
/*****************************************************************************/
/*****************************************************************************/
/**
 * p[0]: argcount(2 or 3)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: channel group (or -1 for all)
 * [p[3]: val]
 *       for each byte:
 *           bit 0 gain control bit 0
 *           bit 1 gain Control bit 1
 *           bit 2 disable 50 Ohm termination
 *           bit 3..7 reserved
 *           gain: 
 *               00 5 V
 *               01 2 V
 *               10 1.9 V
 *               11 1.9 v
 */
plerrcode proc_sis3316_analog_ctrl(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_analog_ctrl, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 regs[4]={0x1004, 0x2004, 0x3004, 0x4004};

        if (p[0]>2) {
            if (ip[2]<0) {
                ems_u32 vals[4];
                int i;

                for (i=0; i<4; i++)
                    vals[i]=p[3];
                pres=sis3316_write_regs(module, regs, vals, 4);
            } else {
                pres=sis3316_write_reg(module, 0x1004+0x1000*p[2], p[3]);
            }
        } else {
            if (ip[2]<0) {
                pres=sis3316_read_regs(module, regs, outptr, 4);
                if (pres==plOK)
                    outptr+=4;
            } else {
                pres=sis3316_read_reg(module, 0x1004+0x1000*p[2], outptr);
                if (pres==plOK)
                    outptr++;
            }
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_analog_ctrl(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]<2 || p[0]>3) {
        complain("sis3316_analog_ctrl: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[2]>3) {
        complain("sis3316_analog_ctrl: illegal channel group: %d", ip[2]);
        return plErr_ArgRange;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_analog_ctrl"))!=plOK)
        return pres;

    wirbrauchen=num+1;
    return plOK;
}

char name_proc_sis3316_analog_ctrl[] = "sis3316_analog_ctrl";
int ver_proc_sis3316_analog_ctrl = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(==6)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: channel group (or -1 for all)
 * p[3]: value chan0
 * p[4]: value chan1
 * p[5]: value chan2
 * p[6]: value chan3
 */
plerrcode proc_sis3316_set_offset(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_set_offset, 0);
    } else {
        if (ip[2]<0) {
            pres=for_each_group(p, proc_sis3316_set_offset);
        } else {
            ml_entry* module=ModulEnt(p[1]);
            pres=sis3316_set_dac(module, p[2], p+3);
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_set_offset(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;

    if (p[0]!=6) {
        complain("sis3316_set_offset: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[2]>3) {
        complain("sis3316_set_offset: illegal channel group: %d", ip[2]);
        return plErr_ArgRange;
    }

    wirbrauchen=0;
    return check_sis3316(ip[1], 0, 0, "sis3316_set_offset");
}

char name_proc_sis3316_set_offset[] = "sis3316_set_offset";
int ver_proc_sis3316_set_offset = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(==2)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 */
plerrcode proc_sis3316_get_offset(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_get_offset, 2);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 regs[4]={0x1108, 0x2108, 0x3108, 0x4108};

        pres=sis3316_read_regs(module, regs, outptr, 4);
        if (pres==plOK)
            outptr+=4;
    }

    return pres;
}

plerrcode test_proc_sis3316_get_offset(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=1) {
        complain("sis3316_get_offset: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_get_offset"))!=plOK)
        return pres;

    wirbrauchen=4*num+1;
    return plOK;
}

char name_proc_sis3316_get_offset[] = "sis3316_get_offset";
int ver_proc_sis3316_get_offset = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(2 or 4)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: channel group (or -1 for all)
 * [p[3]: conf
 *  p[4]: ext. conf]
 *       for each byte in conf:
 *           bit 0 invert input
 *           bit 1 enable internal sum-trigger
 *           bit 2 enable internal trigger
 *           bit 3 enable external trigger
 *           bit 4 enable internal gate 1
 *           bit 5 enable internal gate 2
 *           bit 6 enable external gate
 *           bit 7 enable external veto
 *
 *       for each byte in ext conf:
 *           bit 0 enable internal pileup trigger
 *           bit 1..7: reserved
 */
plerrcode proc_sis3316_event_conf(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_event_conf, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);

        if (p[0]>2) {
            if (ip[2]<0) {
                ems_u32 regs[8]={
                    0x1010, 0x2010, 0x3010, 0x4010,
                    0x109c, 0x209c, 0x309c, 0x409c};
                ems_u32 vals[8];
                int i;

                for (i=0; i<4; i++)
                    vals[i]=p[3];
                for (i=4; i<8; i++)
                    vals[i]=p[4];
                pres=sis3316_write_regs(module, regs, vals, 8);
            } else {
                ems_u32 regs[2];
                ems_u32 vals[2];
                regs[0]=0x1010+0x1000*p[2];
                regs[1]=0x109c+0x1000*p[2];
                vals[0]=p[3];
                vals[1]=p[4];
                pres=sis3316_write_regs(module, regs, vals, 2);
            }
        } else {
            if (ip[2]<0) {
                ems_u32 regs[8]={
                    0x1010, 0x109c, 0x2010, 0x209c,
                    0x3010, 0x309c, 0x4010, 0x409c};
                pres=sis3316_read_regs(module, regs, outptr, 8);
                if (pres==plOK)
                    outptr+=8;
            } else {
                ems_u32 regs[2];
                regs[0]=0x1010+0x1000*p[2];
                regs[1]=0x109c+0x1000*p[2];
                pres=sis3316_read_regs(module, regs, outptr, 2);
                if (pres==plOK)
                    outptr+=2;
            }
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_event_conf(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=2 && p[0]!=4) {
        complain("sis3316_event_conf: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[2]>3) {
        complain("sis3316_event_conf: illegal channel group: %d", ip[2]);
        return plErr_ArgRange;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_event_conf"))!=plOK)
        return pres;

    wirbrauchen=2*num+1;
    return plOK;
}

char name_proc_sis3316_event_conf[] = "sis3316_event_conf";
int ver_proc_sis3316_event_conf = 1;
/*****************************************************************************/
/**
 * p[0]: argcount
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * [p[2 ...]: val ...] (0..255)
 */
plerrcode proc_sis3316_channel_id(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_channel_id, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 regs[4]={0x1014, 0x2014, 0x3014, 0x4014};

        if (p[0]>1) {
            ems_u32 vals[4];
            int idx, id, i;

            idx=member_idx();
            /* p[...] if given or
             * last given p[...] incremented for each module
             */
            id=p[0]-idx>1?p[idx+2]:p[p[0]]+idx-(p[0]-2);
            id<<=24; /* only the highest 8 bit are available */
            for (i=0; i<4; i++)
                vals[i]=id|i<<22; /* bit 23..22 are the group number */
                                  /* bit 21..20 are set to the channel */

            pres=sis3316_write_regs(module, regs, vals, 4);
        } else {
            pres=sis3316_read_regs(module, regs, outptr, 4);
            if (pres==plOK)
                outptr+=4;
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_channel_id(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]<1) {
        complain("sis3316_channel_id: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_channel_id"))!=plOK)
        return pres;

    wirbrauchen=4*num+1;
    return plOK;
}

char name_proc_sis3316_channel_id[] = "sis3316_channel_id";
int ver_proc_sis3316_channel_id = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(2 or 4)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: channel group (or -1 for all)
 * [p[3]: val]
 */
plerrcode proc_sis3316_mem_threshold(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_mem_threshold, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 regs[4]={0x1018, 0x2018, 0x3018, 0x4018};

        if (p[0]>1) {
            if (ip[2]<0) {
                ems_u32 vals[4];
                int i;

                for (i=0; i<4; i++)
                    vals[i]=p[3];
                pres=sis3316_write_regs(module, regs, vals, 4);
            } else {
                pres=sis3316_write_reg(module, regs[p[2]], p[3]);
            }
        } else {
            pres=sis3316_read_regs(module, regs, outptr, 4);
            if (pres==plOK)
                outptr+=4;
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_mem_threshold(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=2 && p[0]!=3) {
        complain("sis3316_mem_threshold: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[2]>3) {
        complain("sis3316_mem_threshold: illegal channel group: %d", ip[2]);
        return plErr_ArgRange;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_mem_threshold"))!=plOK)
        return pres;

    wirbrauchen=4*num+1;
    return plOK;
}
char name_proc_sis3316_mem_threshold[] = "sis3316_mem_threshold";
int ver_proc_sis3316_mem_threshold = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(2 or 4)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: channel group (or -1 for all)
 * [p[3]: val]
 */
plerrcode proc_sis3316_trigger_gate_length(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_trigger_gate_length, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 regs[4]={0x101c, 0x201c, 0x301c, 0x401c};

        if (p[0]>1) {
            if (ip[2]<0) {
                ems_u32 vals[4];
                int i;

                for (i=0; i<4; i++)
                    vals[i]=p[3];

                pres=sis3316_write_regs(module, regs, vals, 4);
            } else {
                pres=sis3316_write_reg(module, regs[p[2]], p[3]);
            }
        } else {
            pres=sis3316_read_regs(module, regs, outptr, 4);
            if (pres==plOK)
                outptr+=4;
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_trigger_gate_length(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=2 && p[0]!=3) {
        complain("sis3316_trigger_gate_length: illegal # of arguments: %d",
                p[0]);
        return plErr_ArgNum;
    }

    if (ip[2]>3) {
        complain("sis3316_trigger_gate_length: illegal channel group: %d",
                ip[2]);
        return plErr_ArgRange;
    }

    if (p[0]>2 && (p[3]&0xffff0001)) {
        complain("sis3316_trigger_gate_length: illegal value: %d", ip[3]);
        return plErr_ArgRange;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_trigger_gate_length"))!=
            plOK)
        return pres;

    wirbrauchen=4*num+1;
    return plOK;
}
char name_proc_sis3316_trigger_gate_length[] = "sis3316_trigger_gate_length";
int ver_proc_sis3316_trigger_gate_length = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(2 or 4)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: channel group (or -1 for all)
 * [p[3]: raw buffer start index
 *  p[4]: raw buffer sample length]
 */
plerrcode proc_sis3316_rawdata_conf(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_rawdata_conf, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 regs[8]={
            0x1020, 0x1098, /* 'normal', 'extended' configuration */
            0x2020, 0x2098,
            0x3020, 0x3098,
            0x4020, 0x4098
        };
        ems_u32 vals[8];

        if (p[0]>2) {
            if (p[4]&0xffff0000) { /* have to use ext. raw buffer length */
                vals[0]=p[3]&0xffff;
                vals[1]=p[4];
            } else {
                vals[0]=(p[3]&0xffff) | p[4]<<16;
                vals[1]=0;
            }
            if (ip[2]<0) {
                int i;
                for (i=2; i<8; i+=2) {
                    vals[i]=vals[0];
                    vals[i+1]=vals[1];
                }
                pres=sis3316_write_regs(module, regs, vals, 8);
            } else {
                pres=sis3316_write_regs(module, regs+2*p[2], vals, 2);
            }
        } else {
            if (ip[2]<0) {
                pres=sis3316_read_regs(module, regs, outptr, 8);
                if (pres==plOK)
                    outptr+=8;
            } else {
                pres=sis3316_read_regs(module, regs+2*p[2], outptr, 2);
                if (pres==plOK)
                    outptr+=2;
            }
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_rawdata_conf(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=2 && p[0]!=4) {
        complain("sis3316_rawdata_conf: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[2]>3) {
        complain("sis3316_rawdata_conf: illegal channel group: %d", ip[2]);
        return plErr_ArgRange;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_rawdata_conf"))!=plOK)
        return pres;

    wirbrauchen=8*num+1;
    return plOK;
}
char name_proc_sis3316_rawdata_conf[] = "sis3316_rawdata_conf";
int ver_proc_sis3316_rawdata_conf = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(2..3)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: channel group (or -1 for all)
 * [p[3]: delay (max. 16378)]
 */
plerrcode proc_sis3316_pre_trigger_delay(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_pre_trigger_delay, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 regs[4]={0x1028, 0x2028, 0x3028, 0x4028};
        ems_u32 vals[4];

        if (p[0]>2) {
            if (ip[2]<0) {
                int i;
                for (i=0; i<4; i++)
                    vals[i]=p[3];
                pres=sis3316_write_regs(module, regs, vals, 4);
            } else {
                pres=sis3316_write_reg(module, regs[p[2]], p[3]);
            }
        } else {
            if (ip[2]<0) {
                pres=sis3316_read_regs(module, regs, outptr, 4);
                if (pres==plOK)
                    outptr+=4;
            } else {
                pres=sis3316_read_reg(module, regs[p[2]], outptr);
                if (pres==plOK)
                    outptr++;
            }
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_pre_trigger_delay(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=2 && p[0]!=3) {
        complain("sis3316_pre_trigger_delay: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[2]>3) {
        complain("sis3316_pre_trigger_delay: illegal channel group: %d", ip[2]);
        return plErr_ArgRange;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_pre_trigger_delay"))!=plOK)
        return pres;

    wirbrauchen=4*num+1;
    return plOK;
}

char name_proc_sis3316_pre_trigger_delay[] = "sis3316_pre_trigger_delay";
int ver_proc_sis3316_pre_trigger_delay = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(3)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: channel group (or -1 for all)
 * [p[3]: conf]
 *       for each byte in conf:
 *           bit 0 peak high values + 6*accumulator values
 *           bit 1 2*accumulator values (gates 7, 8)
 *           bit 2 3*fast trigger MAW values
 *           bit 3 start and max. energy MAW value
 *           bit 4 enable MAW test buffer
 *           bit 5 select energy MAW test buffer bit
 *           bit 6 reserved
 *           bit 7 reserved
 */
plerrcode proc_sis3316_format_conf(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_format_conf, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 regs[4]={0x1030, 0x2030, 0x3030, 0x4030};
        ems_u32 vals[4];

        if (p[0]>2) {
            if (ip[2]<0) {
                int i;
                for (i=0; i<4; i++)
                    vals[i]=p[3];
                pres=sis3316_write_regs(module, regs, vals, 4);
            } else {
                pres=sis3316_write_reg(module, 0x1030+0x1000*p[2], p[3]);
            }
        } else {
            if (ip[2]<0) {
                pres=sis3316_read_regs(module, regs, outptr, 4);
                if (pres==plOK)
                    outptr+=4;
            } else {
                pres=sis3316_read_reg(module, 0x1030+0x1000*p[2], outptr);
                if (pres==plOK)
                    outptr++;
            }
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_format_conf(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=2 && p[0]!=3) {
        complain("sis3316_format_conf: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[2]>3) {
        complain("sis3316_format_conf: illegal channel group: %d", ip[2]);
        return plErr_ArgRange;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_format_conf"))!=plOK)
        return pres;

    wirbrauchen=4*num+1;
    return plOK;
}

char name_proc_sis3316_format_conf[] = "sis3316_format_conf";
int ver_proc_sis3316_format_conf = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(2 or 4)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: channel group (or -1 for all)
 * [p[3]: length            (16 bit; bit0==0)
 *  p[4]: pretrigger delay] (10 bit; bit0==0)
 */
plerrcode proc_sis3316_maw_test_buffer(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    ems_u32 regs[]={0x1034, 0x2034, 0x3034, 0x4034};

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_maw_test_buffer, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);

        if (p[0]>2) { /* write */
            ems_u32 val, vals[4];
            val=(p[4]&0x3ff)<<16 | (p[3]&0xffff);
            if (ip[2]<0) {
                int i;
                for (i=0; i<4; i++)
                    vals[i]=val;
                pres=sis3316_write_regs(module, regs, vals, 4);
            } else {
                pres=sis3316_write_reg(module, regs[p[2]], val);
            }
        } else { /* read */
            if (ip[2]<0) {
                pres=sis3316_read_regs(module, regs, outptr, 4);
                if (pres==plOK)
                    outptr+=4;
            } else {
                pres=sis3316_read_reg(module, regs[p[2]], outptr);
                if (pres==plOK)
                    outptr++;
            }
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_maw_test_buffer(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    int num, res;

    if (p[0]!=2 && p[0]!=4) {
        complain("sis3316_maw_test_buffer: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[2]>3) {
        complain("sis3316_maw_test_buffer: illegal channel group: %d", ip[2]);
        return plErr_ArgRange;
    }

    res=check_sis3316(ip[1], 0, &num, "sis3316_maw_test_buffer");
    wirbrauchen=p[0]>2?0:num*(ip[2]<0?4:1)+1;
    return res;
}

char name_proc_sis3316_maw_test_buffer[] = "sis3316_maw_test_buffer";
int ver_proc_sis3316_maw_test_buffer = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(2 or 6)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: channel group (or -1 for all)
 * [p[3]: channel (0..4; 4 is sum of 0..3) or -1 for 0..3
 *  p[4]: peaking time
 *  p[5]: gap time
 *  p[6]: pulse length]
 *
 * peaking time: 2, 4, 6, ..., 510; bit0==0
 * gap time    : 2, 4, 6, ..., 510; bit0==0
 * pulse length: 2, 4, 6, ..., 254; bit0==0
 */
plerrcode proc_sis3316_fir_setup(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    ems_u32 regs[]={0x1040, 0x1050, 0x1060, 0x1070, 0x1080,
                    0x2040, 0x2050, 0x2060, 0x2070, 0x2080,
                    0x3040, 0x3050, 0x3060, 0x3070, 0x3080,
                    0x4040, 0x4050, 0x4060, 0x4070, 0x4080};

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_fir_setup, 0);
    } else {
        if (p[0]>2) { /* write registers */
            if (ip[2]<0) {
                pres=for_each_group(p, proc_sis3316_fir_setup);
            } else {
                ml_entry* module=ModulEnt(p[1]);
                ems_u32 val, vals[4];

                val=(p[6]&0xff)<<24 |        
                    (p[5]&0xfff)<<12 |       
                    (p[4]&0xfff);              

                if (ip[3]<0) {
                    int i;
                    for (i=0; i<4; i++)
                        vals[i]=val;
                    pres=sis3316_write_regs(module, regs+5*p[2], vals, 4);
                } else {
                    pres=sis3316_write_reg(module, regs[5*p[2]+p[3]], val);
                }
            }
        } else {      /* read registers*/
            ml_entry* module=ModulEnt(p[1]);
            if (ip[2]<0) {
                pres=sis3316_read_regs(module, regs, outptr, 20);
                if (pres==plOK)
                    outptr+=20;
            } else {
                pres=sis3316_read_regs(module, regs+5*p[2], outptr, 5);
                if (pres==plOK)
                    outptr+=5;
            }
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_fir_setup(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    int num, res;

    if (p[0]!=2 && p[0]!=6) {
        complain("sis3316_fir_setup: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[2]>3) {
        complain("sis3316_fir_setup: illegal channel group: %d", ip[2]);
        return plErr_ArgRange;
    }

    if (p[0]>2 && ip[3]>4) {
        complain("sis3316_fir_setup: illegal channel: %d", ip[3]);
        return plErr_ArgRange;
    }

    res=check_sis3316(ip[1], 0, &num, "sis3316_fir_setup");
    wirbrauchen=p[0]>2?0:num*5*(ip[2]<0?4:1)+1;
    return res;
}

char name_proc_sis3316_fir_setup[] = "sis3316_fir_setup";
int ver_proc_sis3316_fir_setup = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(2 or 7)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: channel group (or -1 for all)
 * [p[3]: value chan0
 *  p[4]: value chan1
 *  p[5]: value chan2
 *  p[6]: value chan3
 *  p[7]: value sum of 0..3]
 *
 * value:
 *     31 trigger enable
 *     30 high energy suppress trigger mode
 *     29..28 CFD control bits
 *         0 disabled
 *         1 disabled
 *         2 enabled with zero crossing
 *         3 enabled with 50%
 *     27..0 threshold value
 */
plerrcode proc_sis3316_threshold(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    ems_u32 regs[]={0x1044, 0x1054, 0x1064, 0x1074, 0x1084,
                    0x2044, 0x2054, 0x2064, 0x2074, 0x2084,
                    0x3044, 0x3054, 0x3064, 0x3074, 0x3084,
                    0x4044, 0x4054, 0x4064, 0x4074, 0x4084};

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_threshold, 0);
    } else {
        if (p[0]>2) { /* write registers */
            if (ip[2]<0) {
                pres=for_each_group(p, proc_sis3316_threshold);
            } else {
                ml_entry* module=ModulEnt(p[1]);
                pres=sis3316_write_regs(module, regs+5*p[2], p+3, 5);
            }
        } else {      /* read registers*/
            ml_entry* module=ModulEnt(p[1]);
            if (ip[2]<0) {
                pres=sis3316_read_regs(module, regs, outptr, 20);
                if (pres==plOK)
                    outptr+=20;
            } else {
                pres=sis3316_read_regs(module, regs+5*p[2], outptr, 5);
                if (pres==plOK)
                    outptr+=5;
            }
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_threshold(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    int num, res;

    if (p[0]!=2 && p[0]!=7) {
        complain("sis3316_threshold: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[2]>3) {
        complain("sis3316_threshold: illegal channel group: %d", ip[2]);
        return plErr_ArgRange;
    }

    res=check_sis3316(ip[1], 0, &num, "sis3316_threshold");
    wirbrauchen=p[0]>2?0:num*5*(ip[2]<0?4:1)+1;
    return res;
}

char name_proc_sis3316_threshold[] = "sis3316_threshold";
int ver_proc_sis3316_threshold = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(2 or 7)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: channel group (or -1 for all)
 * [p[3]: value chan0
 *  p[4]: value chan1
 *  p[5]: value chan2
 *  p[6]: value chan3
 *  p[7]: value sum of 0..3]
 *
 * value:
 *     31 trigger on both edges enable
 *     30 high energy stretched output pulse to VME FPGA multiplexer
 *     29..28 internal stretched output pulse to VME FPGA multiplexer
 *         0 internal trigger
 *         1 high energy trigger
 *         2 pileup pulse
 *         3 reserved
 *     27..0 threshold value
 */
plerrcode proc_sis3316_he_threshold(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    ems_u32 regs[]={0x1048, 0x1058, 0x1068, 0x1078, 0x1088,
                    0x2048, 0x2058, 0x2068, 0x2078, 0x2088,
                    0x3048, 0x3058, 0x3068, 0x3078, 0x3088,
                    0x4048, 0x4058, 0x4068, 0x4078, 0x4088};

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_he_threshold, 0);
    } else {
        if (p[0]>2) { /* write registers */
            if (ip[2]<0) {
                pres=for_each_group(p, proc_sis3316_he_threshold);
            } else {
                ml_entry* module=ModulEnt(p[1]);
                pres=sis3316_write_regs(module, regs+5*p[2], p+3, 5);
            }
        } else {      /* read registers*/
            ml_entry* module=ModulEnt(p[1]);
            if (ip[2]<0) {
                pres=sis3316_read_regs(module, regs, outptr, 20);
                if (pres==plOK)
                    outptr+=20;
            } else {
                pres=sis3316_read_regs(module, regs+5*p[2], outptr, 5);
                if (pres==plOK)
                    outptr+=5;
            }
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_he_threshold(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    int num, res;

    if (p[0]!=2 && p[0]!=7) {
        complain("sis3316_he_threshold: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[2]>3) {
        complain("sis3316_he_threshold: illegal channel group: %d", ip[2]);
        return plErr_ArgRange;
    }

    res=check_sis3316(ip[1], 0, &num, "sis3316_he_threshold");
    wirbrauchen=p[0]>2?0:num*5*(ip[2]<0?4:1)+1;
    return res;
}

char name_proc_sis3316_he_threshold[] = "sis3316_he_threshold";
int ver_proc_sis3316_he_threshold = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(2 or 4)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: channel group (or -1 for all)
 * p[3]: gate (0..7)
 * [p[4]: start   (16 bit)
 *  p[5]: length] (samples-1; 9 bit)
 */
plerrcode proc_sis3316_gate_config(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    ems_u32 regs[8][4]={
        {0x10a0, 0x20a0, 0x30a0, 0x40a0},
        {0x10a4, 0x20a4, 0x30a4, 0x40a4},
        {0x10a8, 0x20a8, 0x30a8, 0x40a8},
        {0x10ac, 0x20ac, 0x30ac, 0x40ac},
        {0x10b0, 0x20b0, 0x30b0, 0x40b0},
        {0x10b4, 0x20b4, 0x30b4, 0x40b4},
        {0x10b8, 0x20b8, 0x30b8, 0x40b8},
        {0x10bc, 0x20bc, 0x30bc, 0x40bc},
    };

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_gate_config, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        if (p[0]>3) { /* write */
            ems_u32 val, vals[4];
 
            val=(p[4]&0xffff)|((p[5]&0x1ff)<<16);
#if 0
if (ip[2]<0) {
            printf("gate_config ch %d gt %d regs %04x val %08x\n",
                        ip[2], ip[3], regs[p[3]][0], val);
} else {
            printf("gate_config ch %d gt %d reg %04x val %08x\n",
                        ip[2], ip[3], regs[p[3]][p[2]], val);
}
#endif
            if (ip[2]<0) { /* all channel groups */
                int i;
                for (i=0; i<4; i++)
                    vals[i]=val;
                pres=sis3316_write_regs(module, regs[p[3]], vals, 4);
            } else {       /* one selected channel group */
#if 0
                printf("gate_config ch %d gt %d reg %04x\n",
                        p[2], p[3], regs[p[3]][p[2]]);
#endif
                pres=sis3316_write_reg(module, regs[p[3]][p[2]], val);
            }
        } else {      /* read */
            if (ip[2]<0) { /* all channel groups */
                pres=sis3316_read_regs(module, regs[p[3]], outptr, 4);
                if (pres==plOK)
                    outptr+=4;
            } else {       /* one selected channel group */
                pres=sis3316_read_reg(module, regs[p[3]][p[2]], outptr);
                if (pres==plOK)
                    outptr++;
            }
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_gate_config(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    int num, res;

    if (p[0]!=3 && p[0]!=5) {
        complain("sis3316_gate_config: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (ip[2]>3) {
        complain("sis3316_gate_config: illegal channel group: %d", ip[2]);
        return plErr_ArgRange;
    }

    if (p[3]>7) {
        complain("sis3316_gate_config: illegal gate: %d", p[3]);
        return plErr_ArgRange;
    }

    res=check_sis3316(ip[1], 0, &num, "sis3316_gate_config");
    wirbrauchen=p[0]>2?0:num*(ip[2]<0?4:1)+1;
    return res;
}

char name_proc_sis3316_gate_config[] = "sis3316_gate_config";
int ver_proc_sis3316_gate_config = 1;
/*****************************************************************************/
/**
 * p[0]: argcoun (==1)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 */
plerrcode proc_sis3316_fpga_fw(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_fpga_fw, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 regs[4]={0x1100, 0x2100, 0x3100, 0x4100};

        pres=sis3316_read_regs(module, regs, outptr, 4);
        if (pres==plOK)
            outptr+=4;
    }

    return pres;
}

plerrcode test_proc_sis3316_fpga_fw(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=1) {
        complain("sis3316_fpga_fw: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_fpga_fw"))!=plOK)
        return pres;

    wirbrauchen=4*num+1;
    return plOK;
}

char name_proc_sis3316_fpga_fw[] = "sis3316_fpga_fw";
int ver_proc_sis3316_fpga_fw = 1;
/*****************************************************************************/
/**
 * p[0]: argcoun (==1)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 */
plerrcode proc_sis3316_sample_address(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_sample_address, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 regs[16]={
            0x1110, 0x1114, 0x1118, 0x111c,
            0x2110, 0x2114, 0x2118, 0x211c,
            0x3110, 0x3114, 0x3118, 0x311c,
            0x4110, 0x4114, 0x4118, 0x411c,
        };

        pres=sis3316_read_regs(module, regs, outptr, 16);
        if (pres==plOK)
            outptr+=16;
    }

    return pres;
}

plerrcode test_proc_sis3316_sample_address(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=1) {
        complain("sis3316_sample_address: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_sample_address"))!=plOK)
        return pres;

    wirbrauchen=16*num+1;
    return plOK;
}

char name_proc_sis3316_sample_address[] = "sis3316_sample_address";
int ver_proc_sis3316_sample_address = 1;
/*****************************************************************************/
/**
 * p[0]: argcoun (==1)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 */
plerrcode proc_sis3316_prev_sample_address(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_prev_sample_address, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 regs[16]={
            0x1120, 0x1124, 0x1128, 0x112c,
            0x2120, 0x2124, 0x2128, 0x212c,
            0x3120, 0x3124, 0x3128, 0x312c,
            0x4120, 0x4124, 0x4128, 0x412c,
        };

        pres=sis3316_read_regs(module, regs, outptr, 16);
        if (pres==plOK)
            outptr+=16;
    }

    return pres;
}
plerrcode test_proc_sis3316_prev_sample_address(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]!=1) {
        complain("sis3316_prev_sample_address: illegal # of arguments: %d",
                p[0]);
        return plErr_ArgNum;
    }

    if ((pres=check_sis3316(ip[1], 0, &num, "sis3316_prev_sample_address"))
            !=plOK)
        return pres;

    wirbrauchen=16*num+1;
    return plOK;
}

char name_proc_sis3316_prev_sample_address[] = "sis3316_prev_sample_address";
int ver_proc_sis3316_prev_sample_address = 1;
/*****************************************************************************/
/*****************************************************************************/
/**
 * p[0]: argcount(==1)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 */
plerrcode proc_sis3316_dump_mod(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_dump_mod, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);

        printf("module %d:\n", p[1]);
        if (module->modulclass==modul_ip) {
            printf("  [%s %s %s %s]\n",
                module->address.ip.node,
                module->address.ip.protocol,
                module->address.ip.rserv,
                module->address.ip.lserv);
            printf("  private_data=%p\n", module->private_data);
            if (module->private_data) {
                struct sis3316_priv_data *priv=
                        (struct sis3316_priv_data*)module->private_data;
                printf("    sis3316_read_ctrl =%p\n", priv->sis3316_read_ctrl);
                printf("    sis3316_write_ctrl=%p\n", priv->sis3316_write_ctrl);
                printf("    sis3316_read_reg  =%p\n", priv->sis3316_read_reg);
                printf("    sis3316_write_reg =%p\n", priv->sis3316_write_reg);
                printf("    sis3316_read_regs =%p\n", priv->sis3316_read_regs);
                printf("    sis3316_write_regs=%p\n", priv->sis3316_write_regs);
                printf("    sis3316_read_mem  =%p\n", priv->sis3316_read_mem);
                printf("    sis3316_write_mem =%p\n", priv->sis3316_write_mem);
            }
            printf("  sock=%p\n", module->address.ip.sock);
            if (module->address.ip.sock) {
                struct ipsock *sock=module->address.ip.sock;
                printf("    p=%d\n", sock->p);
                printf("    addrlen=%d\n", sock->addrlen);
                printf("    addr=");
                print_sockaddr(sock->addr);
                printf("\n");
            }
            printf("\n");
        } else {
            printf("not an ip module\n");
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_dump_mod(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;

    if (p[0]!=1) {
        complain("sis3316_dump_mod: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    wirbrauchen=0;
    return check_sis3316(ip[1], 0, 0, "sis3316_dump_mod");
}

char name_proc_sis3316_dump_mod[] = "sis3316_dump_mod";
int ver_proc_sis3316_dump_mod = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(==1)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 */

static const char *ctrl_names_vme[]={
    /*00*/ "status       ",
    /*04*/ "ident        ",
    /*08*/ "irq conf     ",
    /*0c*/ "irq contr    ",
    /*10*/ "interface csr",
    /*14*/ "cblt setup   ",
    /*18*/ "reserved     ",
    /*1c*/ "hw version   ",
};
static const char *ctrl_names_ip[]={
    /*00*/ "status       ",
    /*04*/ "ident        ",
    /*08*/ "UDP conf     ",
    /*0c*/ "reserved     ",
    /*10*/ "interface csr",
    /*14*/ "reserved     ",
    /*18*/ "speed counter",
    /*1c*/ "hw version   ",
};

plerrcode proc_sis3316_dump_stat(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_dump_stat, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        const char **names;
        ems_u32 val;
        int i;

        if (module->modulclass==modul_ip)
            names=ctrl_names_ip;
        else
            names=ctrl_names_vme;

        for (i=0; i<8; i++) {
            pres=sis3316_read_ctrl(module, 4*i, &val);
            if (pres!=plOK)
                break;
            printf("[%d, %02x] %s: 0x%08x\n", p[1], 4*i, names[i], val);
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_dump_stat(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;

    if (p[0]!=1) {
        complain("sis3316_dump_stat: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    wirbrauchen=0;
    return check_sis3316(ip[1], 0, 0, "sis3316_dump_stat");
}

char name_proc_sis3316_dump_stat[] = "sis3316_dump_stat";
int ver_proc_sis3316_dump_stat = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(==1)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * [p[2]: 0 or absent: suppress zero bits, 1: print all bits]
 */

static const char *acq_names[]={
/* 0 */ "Status of Single Bank Mode (reserved)",
/* 1 */ "reserved",
/* 2 */ "reserved",
/* 3 */ "reserved",
/* 4 */ "FP-Bus-In Control 1 as Trigger Enable",
/* 5 */ "FP-Bus-In Control 1 as Veto Enable",
/* 6 */ "FP-Bus-In Control 2 Enable",
/* 7 */ "FP-Bus-In Sample Control Enable",
/* 8 */ "External Trigger function as Trigger Enable",
/* 9 */ "External Trigger function as Veto Enable",
/*10 */ "External Timestamp-Clear function Enable",
/*11 */ "Local Veto function as Veto Enable",
/*12 */ "NIM Input TI as disarm/arm Enable",
/*13 */ "NIM Input UI as disarm/arm Enable",
/*14 */ "Feedback Internal Trigger as External Trigger",
/*15 */ "External Trigger Disable with internal Busy select",
/*16 */ "ADC Sample Logic Armed",
/*17 */ "ADC Sample Logic Armed On Bank2",
/*18 */ "Logic Busy (OR)",
/*19 */ "Memory Address Threshold flag (OR)",
/*20 */ "FP-Bus-In Status 1: Sample Logic busy",
/*21 */ "FP-Bus-In Status 2: Address Threshold flag",
/*22 */ "Sample Bank Swap Control with NIM Input TI/UI enabled",
/*23 */ "PPS latch bit",
/*24 */ "Sample Logic Busy Ch 1-4",
/*25 */ "Memory Address Threshold Flag Ch 1-4",
/*26 */ "Sample Logic Busy Ch 5-8",
/*27 */ "Memory Address Threshold Flag Ch 5-8",
/*28 */ "Sample Logic Busy Ch 9-12",
/*29 */ "Memory Address Threshold Flag Ch 9-12",
/*30 */ "Sample Logic Busy Ch 13-16",
/*31 */ "Memory Address Threshold Flag Ch 13-16",
};

plerrcode proc_sis3316_dump_acq(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_dump_acq, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 val;
        int print_all, i;

        print_all=p[0]<2?0:p[2];
        pres=sis3316_read_ctrl(module, 0x60, &val);
        printf("\nsis3316_dump_acq:\n");
        for (i=0; i<32; i++, val>>=1) {
            if (val&1 || print_all)
                printf("[%d, %2d] %d %s\n", p[1], i, val&1, acq_names[i]);
        }
    }

    return pres;
}

plerrcode test_proc_sis3316_dump_acq(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;

    if (p[0]!=1 && p[0]!=2) {
        complain("sis3316_dump_acq: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    wirbrauchen=0;
    return check_sis3316(ip[1], 0, 0, "sis3316_dump_acq");
}

char name_proc_sis3316_dump_acq[] = "sis3316_dump_acq";
int ver_proc_sis3316_dump_acq = 1;
/*****************************************************************************/
struct regdescr {
    ems_u32 addr;
    const char *name;
};
static const struct regdescr fpga_regs[]={
    {0x0, "LED"},
    {0x20, "temperature"},
    {0x24, "one wire control"},
    {0x28, "serial number"},
    {0x2c, "internal data transfer speed"},

    {0x30, "ADC FPGAs boot controller"},
    {0x34, "SPI flash control status"},
    {0x38, "SPI flash data"},
    {0x3c, "external veto gate delay"},

    {0x40, "programmable ADC clock"},
    {0x44, "programmable MGT1 clock"},
    {0x48, "programmable MGT2 clock"},
    {0x4c, "programmable DDR3 clock"},

    {0x50, "ADC sample clock distributoin control register"},
//    {0x54, "extrnal NIM clock multiplier"},
    {0x58, "FP-bus control"},
    {0x5c, "NIM-IN control/status"},

    {0x60, "acquisition control/status"},
//    {0x64, "coincidence lookup table control"},
//    {0x68, "coincidence lookup table address"},
//    {0x6c, "coincidence lookup table data"},

    {0x70, "LEMO out CO select"},
    {0x74, "LEMO out TO select"},
    {0x78, "LEMO out UO select"},
    {0x7c, "internal trigger feedback select"},

    {0x80, "ADC FPGA 1 ch01-04 data transfer control"},
    {0x84, "ADC FPGA 2 ch05-08 data transfer control"},
    {0x88, "ADC FPGA 3 ch09-12 data transfer control"},
    {0x8c, "ADC FPGA 4 ch13-16 data transfer control"},

    {0x90, "ADC FPGA 1 ch01-04 data transfer status"},
    {0x94, "ADC FPGA 2 ch05-08 data transfer status"},
    {0x98, "ADC FPGA 3 ch09-12 data transfer status"},
    {0x9c, "ADC FPGA 4 ch13-16 data transfer status"},

    {0xa0, "VME FPGA ADC FPGAs data link status"},
    {0xa4, "ADC FPGA SPI busy status"},

#if 0
    {0xc0, "channel  1 internal trigger counter"},
    {0xc4, "channel  2 internal trigger counter"},
    {0xc8, "channel  3 internal trigger counter"},
    {0xcc, "channel  4 internal trigger counter"},

    {0xd0, "channel  5 internal trigger counter"},
    {0xd4, "channel  6 internal trigger counter"},
    {0xd8, "channel  7 internal trigger counter"},
    {0xdc, "channel  8 internal trigger counter"},

    {0xe0, "channel  9 internal trigger counter"},
    {0xe4, "channel 10 internal trigger counter"},
    {0xe8, "channel 11 internal trigger counter"},
    {0xec, "channel 12 internal trigger counter"},

    {0xf0, "channel 13 internal trigger counter"},
    {0xf4, "channel 14 internal trigger counter"},
    {0xf8, "channel 15 internal trigger counter"},
    {0xfc, "channel 16 internal trigger counter"},
#endif

    {0x1004, "group 1 analog ctrl"},
    {0x2004, "group 2 analog ctrl"},
    {0x3004, "group 3 analog ctrl"},
    {0x4004, "group 4 analog ctrl"},

    {0x1010, "group 1 event conf"},
    {0x2010, "group 2 event conf"},
    {0x3010, "group 3 event conf"},
    {0x4010, "group 4 event conf"},
    {0x109c, "group 1 event ext. conf"},
    {0x209c, "group 2 event ext. conf"},
    {0x309c, "group 3 event ext. conf"},
    {0x409c, "group 4 event ext. conf"},

    {0x1014, "group 1 channel ID"},
    {0x2014, "group 2 channel ID"},
    {0x3014, "group 3 channel ID"},
    {0x4014, "group 4 channel ID"},

    {0x1018, "group 1 mem threshold"},
    {0x2018, "group 2 mem threshold"},
    {0x3018, "group 3 mem threshold"},
    {0x4018, "group 4 mem threshold"},

    {0x101c, "group 1 trigger gate length"},
    {0x201c, "group 2 trigger gate length"},
    {0x301c, "group 3 trigger gate length"},
    {0x401c, "group 4 trigger gate length"},

    {0x1020, "group 1 rawdata conf"},
    {0x2020, "group 2 rawdata conf"},
    {0x3020, "group 3 rawdata conf"},
    {0x4020, "group 4 rawdata conf"},

    {0x1098, "group 1 rawdata ext. conf"},
    {0x2098, "group 2 rawdata ext. conf"},
    {0x3098, "group 3 rawdata ext. conf"},
    {0x4098, "group 4 rawdata ext. conf"},

    {0x1028, "group 1 pre_trigger_delay"},
    {0x2028, "group 2 pre_trigger_delay"},
    {0x3028, "group 3 pre_trigger_delay"},
    {0x4028, "group 4 pre_trigger_delay"},

    {0x1030, "group 1 format conf"},
    {0x2030, "group 2 format conf"},
    {0x3030, "group 3 format conf"},
    {0x4030, "group 4 format conf"},

    {0x1034, "group 1 maw test buffer"},
    {0x2034, "group 2 maw test buffer"},
    {0x3034, "group 3 maw test buffer"},
    {0x4034, "group 4 maw test buffer"},

    {0x1040, "channel  1 FIR setup"},
    {0x1050, "channel  2 FIR setup"},
    {0x1060, "channel  3 FIR setup"},
    {0x1070, "channel  4 FIR setup"},
    {0x1080, "channel 1-4 sum FIR setup"},
    {0x2040, "channel  5 FIR setup"},
    {0x2050, "channel  6 FIR setup"},
    {0x2060, "channel  7 FIR setup"},
    {0x2070, "channel  8 FIR setup"},
    {0x2080, "channel 5-8 sum FIR setup"},
    {0x3040, "channel  9 FIR setup"},
    {0x3050, "channel 10 FIR setup"},
    {0x3060, "channel 11 FIR setup"},
    {0x3070, "channel 12 FIR setup"},
    {0x3080, "channel 9-12 sum FIR setup"},
    {0x4040, "channel 13 FIR setup"},
    {0x4050, "channel 14 FIR setup"},
    {0x4060, "channel 15 FIR setup"},
    {0x4070, "channel 16 FIR setup"},
    {0x4080, "channel 13-16 sum FIR setup"},

    {0x1044, "channel  1 threshold"},
    {0x1054, "channel  2 threshold"},
    {0x1064, "channel  3 threshold"},
    {0x1074, "channel  4 threshold"},
    {0x1084, "channel 1-4 sum threshold"},
    {0x2044, "channel  5 threshold"},
    {0x2054, "channel  6 threshold"},
    {0x2064, "channel  7 threshold"},
    {0x2074, "channel  8 threshold"},
    {0x2084, "channel 5-8 sum threshold"},
    {0x3044, "channel  9 threshold"},
    {0x3054, "channel 10 threshold"},
    {0x3064, "channel 11 threshold"},
    {0x3074, "channel 12 threshold"},
    {0x3084, "channel 9-12 sum threshold"},
    {0x4044, "channel 13 threshold"},
    {0x4054, "channel 14 threshold"},
    {0x4064, "channel 15 threshold"},
    {0x4074, "channel 16 threshold"},
    {0x4084, "channel 13-16 sum threshold"},

    {0x1048, "channel  1 HE HE threshold"},
    {0x1058, "channel  2 HE threshold"},
    {0x1068, "channel  3 HE threshold"},
    {0x1078, "channel  4 HE threshold"},
    {0x1088, "channel 1-4 sum HE threshold"},
    {0x2048, "channel  5 HE threshold"},
    {0x2058, "channel  6 HE threshold"},
    {0x2068, "channel  7 HE threshold"},
    {0x2078, "channel  8 HE threshold"},
    {0x2088, "channel 5-8 sum HE threshold"},
    {0x3048, "channel  9 HE threshold"},
    {0x3058, "channel 10 HE threshold"},
    {0x3068, "channel 11 HE threshold"},
    {0x3078, "channel 12 HE threshold"},
    {0x3088, "channel 9-12 sum HE threshold"},
    {0x4048, "channel 13 HE threshold"},
    {0x4058, "channel 14 HE threshold"},
    {0x4068, "channel 15 HE threshold"},
    {0x4078, "channel 16 HE threshold"},
    {0x4088, "channel 13-16 sum HE threshold"},

    {0x10a0, "group 1 gate_1 config"},
    {0x20a0, "group 2 gate_1 config"},
    {0x30a0, "group 3 gate_1 config"},
    {0x40a0, "group 4 gate_1 config"},
    {0x10a4, "group 1 gate_2 config"},
    {0x20a4, "group 2 gate_2 config"},
    {0x30a4, "group 3 gate_2 config"},
    {0x40a4, "group 4 gate_2 config"},
    {0x10a8, "group 1 gate_3 config"},
    {0x20a8, "group 2 gate_3 config"},
    {0x30a8, "group 3 gate_3 config"},
    {0x40a8, "group 4 gate_3 config"},
    {0x10ac, "group 1 gate_4 config"},
    {0x20ac, "group 2 gate_4 config"},
    {0x30ac, "group 3 gate_4 config"},
    {0x40ac, "group 4 gate_4 config"},

    {0x10b0, "group 1 gate_5 config"},
    {0x20b0, "group 2 gate_5 config"},
    {0x30b0, "group 3 gate_5 config"},
    {0x40b0, "group 4 gate_5 config"},
    {0x10b4, "group 1 gate_6 config"},
    {0x20b4, "group 2 gate_6 config"},
    {0x30b4, "group 3 gate_6 config"},
    {0x40b4, "group 4 gate_6 config"},
    {0x10b8, "group 1 gate_7 config"},
    {0x20b8, "group 2 gate_7 config"},
    {0x30b8, "group 3 gate_7 config"},
    {0x40b8, "group 4 gate_7 config"},
    {0x10bc, "group 1 gate_8 config"},
    {0x20bc, "group 2 gate_8 config"},
    {0x30bc, "group 3 gate_8 config"},
    {0x40bc, "group 4 gate_8 config"},
};

static ems_u32 dump_regs_addr[sizeof(fpga_regs)/sizeof(struct regdescr)];
static ems_u32 dump_regs_vals[sizeof(fpga_regs)/sizeof(struct regdescr)];

/**
 * Dumps registers
 *
 * @param p[0]: argcount(==1)
 * @param p[1]: index in memberlist (or -1 for all sis3316 modules)
 */
plerrcode proc_sis3316_dump_regs(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_dump_regs, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        int num, i;


        num=sizeof(fpga_regs)/sizeof(struct regdescr);
        printf("sis3316_dump_regs(ser. %d): num=%d\n",
                ((struct sis3316_priv_data*)module->private_data)->serial,
                num);
        for (i=0; i<num; i++)
            dump_regs_addr[i]=fpga_regs[i].addr;
        pres=sis3316_read_regs(module, dump_regs_addr, dump_regs_vals, num);
        if (pres!=plOK) {
            printf("sis3316_dump_regs: read_regs failed\n");
            goto error;
        }

        printf("sis3316_dump_regs(%d):\n", p[1]);
        for (i=0; i<num; i++) {
            printf("%04x %08x %s\n", dump_regs_addr[i], dump_regs_vals[i],
                    fpga_regs[i].name);
        }
    }

error:
    return pres;
}

plerrcode test_proc_sis3316_dump_regs(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;

    if (p[0]!=1) {
        complain("sis3316_dump_regs: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    wirbrauchen=0;
    return check_sis3316(ip[1], 0, 0, "sis3316_dump_regs");
}

char name_proc_sis3316_dump_regs[] = "sis3316_dump_regs";
int ver_proc_sis3316_dump_regs = 1;
/*****************************************************************************/
/*****************************************************************************/
/**
 * p[0]: argcount(>4)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: group (0..3)
 * p[3]: space (0: mem1, 1: mem2, (3: statistic counter not allowed))
 * p[4]: chan (0 or 1; statistic counter ignored)
 * p[5]: bank (0 or 1;  statistic counter ignored)
 * p[6]: addr (in 32 bit words)
 * p[7]: num (in 32 bit words)
 */
plerrcode proc_sis3316_fill_mem(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_fill_mem, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 *buf, addr;
        int num=p[7], i;

        buf=malloc(num*sizeof(ems_u32));
        if (!buf) {
            return plErr_NoMem;
        }

        for (i=0; i<num; i++) {
            ems_u32 val;
            val=i&0x1fffffff;
            val|=p[2]<<28;
            val|=p[3]<<27;
            val|=p[4]<<26;
            val|=p[5]<<25;
            buf[i]=val;
        }

        addr=p[4]<<24 | p[5]<<23 | p[6];
        printf("write_mem: %08x .. %08x\n", addr, addr+p[7]-1);
        pres=sis3316_write_mem(module, p[2], p[3], addr, buf, num);

        free(buf);
    }

    return pres;
}

plerrcode test_proc_sis3316_fill_mem(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;

    if (p[0]!=7) {
        complain("sis3316_fill_mem: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (p[2]>3) {
        complain("sis3316_fill_mem: illegal group: %d", p[2]);
        return plErr_ArgRange;
    }
    if (p[3]>1) {
        complain("sis3316_fill_mem: illegal space: %d", p[3]);
        return plErr_ArgRange;
    }
    if (p[4]>1){
        complain("sis3316_fill_mem: illegal chan: 0x%x", p[4]);
        return plErr_ArgRange;
    }
    if (p[5]>1){
        complain("sis3316_fill_mem: illegal buf: 0x%x", p[4]);
        return plErr_ArgRange;
    }

    if (p[6]>0xffffff) {
        complain("sis3316_fill_mem: illegal address: 0x%x", p[4]);
        return plErr_ArgRange;
    }

    /* (end address too large) || (integer overflow) */
    if (p[6]+p[7]>0x1000000 || p[6]+p[7]<p[7]) {
        complain("sis3316_fill_mem: illegal end address: 0x%x", p[6]+p[7]-1);
        return plErr_ArgRange;
    }

    wirbrauchen=0;
    return check_sis3316(ip[1], 0, 0, "sis3316_fill_mem");
}

char name_proc_sis3316_fill_mem[] = "sis3316_fill_mem";
int ver_proc_sis3316_fill_mem = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(>4)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: group (0..3)
 * p[3]: space (0: mem1, 1: mem2, (3: statistic counter not allowed))
 * p[4]: chan (0 or 1; statistic counter ignored)
 * p[5]: bank (0 or 1;  statistic counter ignored)
 * p[6]: addr (in 32 bit words)
 * p[7]: num (in 32 bit words)
 */
plerrcode proc_sis3316_comp_mem(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_comp_mem, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 *buf, addr;
        int num=p[7], i;
#if 1
        int64_t offs=0;
        int bnum;
#endif

        buf=malloc(num*sizeof(ems_u32));
        if (!buf) {
            return plErr_NoMem;
        }

        addr=p[4]<<24 | p[5]<<23 | p[6];
        printf("comp_mem: %08x .. %08x\n", addr, addr+p[7]-1);
        pres=sis3316_read_mem(module, p[2], p[3], addr, buf, num);

        for (i=0; i<num; i++) {
            ems_u32 val;
            val=i&0x1fffffff;
            val|=p[2]<<28;
            val|=p[3]<<27;
            val|=p[4]<<26;
            val|=p[5]<<25;
#if 1
            if (buf[i]!=val) {
                if (offs==0) {
                    printf("%06x %08x --> %08x\n", i, val, buf[i]);
                    offs=(int64_t)val-(int64_t)buf[i];
                    bnum=1;
                    printf("offs=%ld bnum=%d\n", offs, bnum);
                } else {
                    int64_t xoffs=(int64_t)val-(int64_t)buf[i];
                    if (xoffs!=offs) {
                        printf("xoffs=%ld offs=%ld\n", xoffs, offs);
                        printf("prev. block ends after 0x%x word%s\n",
                                bnum, bnum==1?"":"s");
                        printf("%06x %08x --> %08x;\n", i, val, buf[i]);
                        offs=xoffs;
                        bnum=1;
                    } else {
                        bnum++;
                    }
                }
            }
#else
            printf("%06x %08x --> %08x\n", i, val, buf[i]);
#endif

        }

        free(buf);

    }

    return pres;
}

plerrcode test_proc_sis3316_comp_mem(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;

    if (p[0]!=7) {
        complain("sis3316_comp_mem: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (p[2]>3) {
        complain("sis3316_comp_mem: illegal group: %d", p[2]);
        return plErr_ArgRange;
    }
    if (p[3]>1) {
        complain("sis3316_comp_mem: illegal space: %d", p[3]);
        return plErr_ArgRange;
    }
    if (p[4]>1){
        complain("sis3316_comp_mem: illegal chan: 0x%x", p[4]);
        return plErr_ArgRange;
    }
    if (p[5]>1){
        complain("sis3316_comp_mem: illegal buf: 0x%x", p[4]);
        return plErr_ArgRange;
    }

    if (p[6]>0xffffff) {
        complain("sis3316_comp_mem: illegal address: 0x%x", p[4]);
        return plErr_ArgRange;
    }

    /* (end address too large) || (integer overflow) */
    if (p[6]+p[7]>0x1000000 || p[6]+p[7]<p[7]) {
        complain("sis3316_comp_mem: illegal end address: 0x%x", p[6]+p[7]-1);
        return plErr_ArgRange;
    }

    wirbrauchen=0;
    return check_sis3316(ip[1], 0, 0, "sis3316_comp_mem");
}

char name_proc_sis3316_comp_mem[] = "sis3316_comp_mem";
int ver_proc_sis3316_comp_mem = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(>4)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: group (0..3)
 * p[3]: space (0: mem1, 1: mem2, (3: statistic counter not allowed))
 * p[4]: chan (0 or 1; statistic counter ignored)
 * p[5]: bank (0 or 1;  statistic counter ignored)
 * p[6]: addr (in 32 bit words)
 * p[7]: num (in 32 bit words)
 * p[8]: data word to be written
 */
plerrcode proc_sis3316_set_mem(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_set_mem, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 *buf, addr;
        int num=p[7], i;

        buf=malloc(num*sizeof(ems_u32));
        if (!buf) {
            return plErr_NoMem;
        }

        for (i=0; i<num; i++) {
            buf[i]=p[8];
        }

        addr=p[4]<<24 | p[5]<<23 | p[6];
        printf("set_mem: %08x .. %08x\n", addr, addr+p[7]-1);
        pres=sis3316_write_mem(module, p[2], p[3], addr, buf, num);

        free(buf);
    }

    return pres;
}

plerrcode test_proc_sis3316_set_mem(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;

    if (p[0]!=8) {
        complain("sis3316_set_mem: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (p[2]>3) {
        complain("sis3316_set_mem: illegal group: %d", p[2]);
        return plErr_ArgRange;
    }
    if (p[3]>1) {
        complain("sis3316_set_mem: illegal space: %d", p[3]);
        return plErr_ArgRange;
    }
    if (p[4]>1){
        complain("sis3316_set_mem: illegal chan: 0x%x", p[4]);
        return plErr_ArgRange;
    }
    if (p[5]>1){
        complain("sis3316_set_mem: illegal buf: 0x%x", p[4]);
        return plErr_ArgRange;
    }

    if (p[6]>0xffffff) {
        complain("sis3316_set_mem: illegal address: 0x%x", p[4]);
        return plErr_ArgRange;
    }

    /* (end address too large) || (integer overflow) */
    if (p[6]+p[7]>0x1000000 || p[6]+p[7]<p[7]) {
        complain("sis3316_set_mem: illegal end address: 0x%x", p[6]+p[7]-1);
        return plErr_ArgRange;
    }

    wirbrauchen=0;
    return check_sis3316(ip[1], 0, 0, "sis3316_set_mem");
}

char name_proc_sis3316_set_mem[] = "sis3316_set_mem";
int ver_proc_sis3316_set_mem = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(>4)
 * p[1]: index in memberlist (or -1 for all sis3316 modules)
 * p[2]: group (0..3)
 * p[3]: space (0: mem1, 1: mem2, (3: statistic counter not allowed))
 * p[4]: chan (0 or 1; statistic counter ignored)
 * p[5]: bank (0 or 1;  statistic counter ignored)
 * p[6]: addr (in 32 bit words)
 * p[7]: num (in 32 bit words)
 * p[8]: expected data word
 */
plerrcode proc_sis3316_check_mem(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (ip[1]<0) {
        pres=for_each_sis3316_member(p, proc_sis3316_check_mem, 0);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        ems_u32 *buf, addr;
        int num=p[7], i;
        ems_u32 expected_val=p[8];
        ems_u32 last_val=expected_val;
        int errors=0;
        int err_blocks=0;

        buf=malloc(num*sizeof(ems_u32));
        if (!buf) {
            return plErr_NoMem;
        }

        addr=p[4]<<24 | p[5]<<23 | p[6];
        printf("check_mem: %08x .. %08x\n", addr, addr+p[7]-1);
        pres=sis3316_read_mem(module, p[2], p[3], addr, buf, num);

        for (i=0; i<num; i++) {
            ems_u32 val;
            val=buf[i];

            if (val!=expected_val)
                errors++;
            if (val!=last_val) {
                err_blocks++;
                complain("bad val %08x -> %08x (%08x expected) at addr %08x",
                        last_val, val, expected_val, i);
                last_val=val;
            }
        }
        free(buf);
    }

    return pres;
}

plerrcode test_proc_sis3316_check_mem(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;

    if (p[0]!=8) {
        complain("sis3316_check_mem: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (p[2]>3) {
        complain("sis3316_check_mem: illegal group: %d", p[2]);
        return plErr_ArgRange;
    }
    if (p[3]>1) {
        complain("sis3316_check_mem: illegal space: %d", p[3]);
        return plErr_ArgRange;
    }
    if (p[4]>1){
        complain("sis3316_check_mem: illegal chan: 0x%x", p[4]);
        return plErr_ArgRange;
    }
    if (p[5]>1){
        complain("sis3316_check_mem: illegal buf: 0x%x", p[4]);
        return plErr_ArgRange;
    }

    if (p[6]>0xffffff) {
        complain("sis3316_check_mem: illegal address: 0x%x", p[4]);
        return plErr_ArgRange;
    }

    /* (end address too large) || (integer overflow) */
    if (p[6]+p[7]>0x1000000 || p[6]+p[7]<p[7]) {
        complain("sis3316_check_mem: illegal end address: 0x%x", p[6]+p[7]-1);
        return plErr_ArgRange;
    }

    wirbrauchen=0;
    return check_sis3316(ip[1], 0, 0, "sis3316_check_mem");
}

char name_proc_sis3316_check_mem[] = "sis3316_check_mem";
int ver_proc_sis3316_check_mem = 1;
/*****************************************************************************/
plerrcode proc_sis3316_read_mem_wrong(__attribute__((unused)) ems_u32* p)
{
    return plOK;
}
plerrcode test_proc_sis3316_read_mem_wrong(__attribute__((unused)) ems_u32* p)
{
    return plOK;
}
char name_proc_sis3316_read_mem_wrong[] = "sis3316_read_mem_wrong";
int ver_proc_sis3316_read_mem_wrong = 1;
/*****************************************************************************/
plerrcode proc_sis3316_chaotic_mem(__attribute__((unused)) ems_u32* p)
{
    return plOK;
}
plerrcode test_proc_sis3316_chaotic_mem(__attribute__((unused)) ems_u32* p)
{
    return plOK;
}
char name_proc_sis3316_chaotic_mem[] = "sis3316_chaotic_mem";
int ver_proc_sis3316_chaotic_mem = 1;
/*****************************************************************************/
/*****************************************************************************/
