/*
 * lowlevel/jtag/jtag_XCFS.c
 * created 2006-Aug-25 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: jtag_XCFS.c,v 1.6 2017/10/22 22:12:39 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <rcs_ids.h>

#include "jtag_chipdata.h"

enum instructions {
    BYPASS    = 0xFF,
    IDCODE    = 0xFE,
    ISPEN     = 0xE8, /* enable in-system programming mode */
    FPGM      = 0xEA, /* Programs specific bit values at specified addresses */
    FADDR     = 0xEB, /* Sets the PROM array address register */
    FVFY1     = 0xF8, /* Reads the fuse values at specified addresses */
    NORMRST   = 0xF0, /* Exits ISP Mode ? */
    /* CONLD     = 0xF0, ==NORMST */
    FERASE    = 0xEC, /* Erases a specified program memory block */
    SERASE    = 0x0A, /* Globally refines the programmed values in the array */
    FDATA0    = 0xED, /* Accesses the array word-line register ? */
    FDATA3    = 0xF3, /* 6 */
    CONFIG    = 0xEE,
    SAMPLE    = 0x01,
    EXTEST    = 0x00,
    USERCODE  = 0xFD,
    HIGHZ     = 0xFC,
    CLAMP     = 0xFA,
    ISPENC    = 0xE9,
    FVFY0     = 0xEF,
    FVFY3     = 0xE2,
    FVFY6     = 0xE6,
    FBLANK0   = 0xE5,
    FBLANK3   = 0xE1,
    FBLANK6   = 0xE4,
    /*XSC_OP_STATUS = 0xE3,   ISCTESTSTATUS*/
    ISCTESTSTATUS = 0xE3,
    priv1     = 0xF1,
    priv3     = 0xE7,
    priv4     = 0xF6,
    priv5     = 0xE0,
    priv6     = 0xF7,
    priv7     = 0xF2,
    ISCCLRSTATUS  = 0xF4,
    priv9     = 0xF5,
};

RCS_REGISTER(cvsid, "lowlevel/jtag")

/****************************************************************************/
static int
jtag_xcfs_status(struct jtag_chip* chip, char *msg)
{
    struct jtag_chain* chain=chip->chain;
    u_int32_t wout;
    u_int tmo;

    jtag_sleep(chain, 1000); /* in us */
    jtag_instruction(chip, ISCTESTSTATUS, 0);
    tmo=0;
    while (1) {
        jtag_sleep(chain, 1000);
        jtag_data32(chip, -1, &wout, 8);
        if (wout &4)
            return 0;

        if (++tmo >= 5000) {
            printf("timeout %s, status=%02X\n", msg, wout);
            jtag_instruction(chip, NORMRST, 0);
            jtag_sleep(chain, 110000);

            jtag_instruction(chip, ISPEN, 0);
            jtag_data32(chip, 0x03, 0, 6);
            jtag_sleep(chain, 12000);

            jtag_instruction(chip, NORMRST, 0);
            jtag_sleep(chain, 110000);
            jtag_instruction(chip, BYPASS, 0);
            return -1;
        }
    }
}
/****************************************************************************/
int
jtag_write_XCFS(struct jtag_chip* chip, ems_u8* data, ems_u32 usercode)
{
    struct jtag_chain* chain=chip->chain;
    u_int32_t dout, ret;
    int	size, len, i;
    unsigned int blk;

    printf("writing device %s\n", chip->chipdata->name);

printf("NOT WRITING!\n");
return -1;

    if (chain->state) {
        printf("chain.state=%d\n", chain->state);
        return -1;
    }

    size=chip->chipdata->mem_size;
    len=chip->chipdata->write_size;


    if (jtag_instruction(chip, IDCODE, 0)) {
        return -1;
    }
    if (jtag_data32(chip, -1, &dout, 32)<0)
        return -1;
    if ((dout /*&0x0FFFFFFF*/)!=chip->id) {
        printf("wrong IDCODE: %08X (exp: %08X)\n", dout, chip->id);
        return -1;
    }

    printf("Check for Read/Write Protect\n");
    jtag_instruction(chip, BYPASS, &ret);
    if ((ret &0x7) != 1) {
        printf("jtag_write_XCFS: protected (%02X)\n", ret);
        return -1;
    }

    jtag_instruction(chip, ISPEN, 0);
    jtag_data32(chip, 0x37, 0, 6);

    jtag_instruction(chip, FADDR, 0);
    jtag_data32(chip, 1, 0, 16);

    printf("loading with 'ferase' instruction\n");
    jtag_instruction(chip, FERASE, 0);
    jtag_sleep(chain, 5000000); /* in us */

    if (jtag_xcfs_status(chip, "erasing")) {
        return -1;
    }

    jtag_instruction(chip, NORMRST, 0);
    jtag_sleep(chain, 130000);

    jtag_instruction(chip, ISPEN, 0);
    jtag_data32(chip, 0x37, 0, 6);

    blk=0;
    for (i=0; i<size/len; i++, data+=len) {
        jtag_instruction(chip, FDATA0, 0);
        if ((ret=jtag_wr_data(chip, data, len, -1)) != 0)
            return ret;

        jtag_instruction(chip, FADDR, 0);
        jtag_data32(chip, blk, 0, 16);
        blk+=0x20;	/* (ddesc->data0 >>4)
        (ist aber fuer XCF01S und XCF04S verschieden???) */

        jtag_instruction(chip, FPGM, 0);
        jtag_sleep(chain, 3000);

        jtag_instruction(chip, BYPASS, 0);
        if (jtag_xcfs_status(chip, "writing data"))
            return -1;

        /* printf("."); fflush(stdout); */
    }
    printf("\nlast FADDR: %04X\n", blk-0x20);

    jtag_sleep(chain, 3000);
    jtag_instruction(chip, FADDR, 0);
    jtag_data32(chip, 0x01, 0, 16);

    jtag_instruction(chip, SERASE, 0);
    jtag_sleep(chain, 37000);

    jtag_instruction(chip, NORMRST, 0);
    jtag_sleep(chain, 110000);

    jtag_instruction(chip, ISPEN, 0);
    jtag_data32(chip, 0x03, 0, 6);
    jtag_sleep(chain, 10000);

    jtag_instruction(chip, FADDR, 0);
    jtag_data32(chip, 0x8000, 0, 16);

    blk=0;
    while (usercode != 0xFFFFFFFF) {
        jtag_instruction(chip, USERCODE, 0);
        jtag_data32(chip, usercode, 0, 32);

        jtag_instruction(chip, FPGM, 0);
        jtag_sleep(chain, 3000);

        jtag_instruction(chip, BYPASS, 0);
        if (jtag_xcfs_status(chip, "writing user code"))
            return -1;

        jtag_instruction(chip, FVFY6, 0);
        jtag_sleep(chain, 50000);
        jtag_data32(chip, 0, &dout, 32);

        jtag_instruction(chip, NORMRST, 0);
        jtag_sleep(chain, 110000);

        jtag_instruction(chip, ISPEN, 0);
        jtag_data32(chip, 0x03, 0, 6);
        jtag_sleep(chain, 7000);

        jtag_instruction(chip, ISPEN, 0);
        jtag_data32(chip, 0x03, 0, 6);
        jtag_sleep(chain, 10000);

        jtag_instruction(chip, FADDR, 0);
        jtag_data32(chip, 0x8000, 0, 16);
        jtag_sleep(chain, 1000);

        if (dout == usercode) {
            printf("user code ok\n");
            break;
        }

        /*
        printf("error writing user code is: %d\n", dout);
        */
        if (++blk > 3)
            break;
    }

    jtag_instruction(chip, FDATA3, 0);
    jtag_data32(chip, 0x07, 0, 3);

    jtag_instruction(chip, FPGM, 0);
    jtag_sleep(chain, 3000);

    jtag_instruction(chip, BYPASS, 0);
    if (jtag_xcfs_status(chip, "ending programming"))
        return -1;

    jtag_instruction(chip, FVFY3, 0);
    jtag_data32(chip, 0x07, 0, 3);

    jtag_instruction(chip, NORMRST, 0);
    jtag_sleep(chain, 110000);

    jtag_instruction(chip, ISPEN, 0);
    jtag_data32(chip, 0x03, 0, 6);
    jtag_sleep(chain, 12000);

    jtag_instruction(chip, NORMRST, 0);
    jtag_sleep(chain, 110000);

    jtag_instruction(chip, BYPASS, 0);

    printf("%d bytes written\n", size);

    return 0;
}
/****************************************************************************/
int
jtag_read_XCFS(struct jtag_chip* chip, ems_u8* data, ems_u32* usercode)
{
    struct jtag_chain* chain=chip->chain;
    u_int32_t dout, size, ret;
    int i;

    unsigned int chipid=
            (chip->chipdata->vendor_id<<1)|(chip->chipdata->part_id<<12)|1;

    printf("reading device %s\n", chip->chipdata->name);
printf("NOT READING!\n");
return -1;

    if (chain->state) {
        printf("chain.state=%d\n", chain->state);
        return -1;
    }

    size=chip->chipdata->mem_size;

    if (jtag_instruction(chip, IDCODE, 0)) {
        return -1;
    }
    if (jtag_data32(chip, -1, &dout, 32)<0)
        return -1;
    if ((dout &0x0FFFFFFF)!=chipid) {
        printf("wrong IDCODE: %08X (exp: %08X)\n", dout, chipid);
        return -1;
    }

    /* read usercode (used as serial number) */
    jtag_instruction(chip, USERCODE, 0);
    jtag_data32(chip, 0, usercode, 32);


#if 0
    {
        int i;
        sleepcheck(&chain->jtag_dev);
        for (i=0; i<1000; i++) {
            chain->jtag_dev.jt_mark(&chain->jtag_dev, 0xf);
            chain->jtag_dev.jt_sleep(&chain->jtag_dev, 5);
            chain->jtag_dev.jt_mark(&chain->jtag_dev, 0x0);
            chain->jtag_dev.jt_sleep(&chain->jtag_dev, 10);
        }
    }
#endif

    if (!data) /* only usercode requested */
        return 0;

    jtag_instruction(chip, ISPEN, 0);
    jtag_data32(chip, 0x34, 0, 6);

    jtag_instruction(chip, FADDR, &ret);
    if (ret!=0x11) {
        if ((ret&~0x18)!=0x1) {
            printf("illegal capture pattern 0x%x\n", ret);
            return -1;
        } else {
            if (!(ret&0x10))
                printf("chip not in ISP status\n");
            if (ret&0x8)
                printf("security bit set\n");
            return -1;
        }
    }

    jtag_data32(chip, 0, 0, 16);
    jtag_instruction(chip, FVFY1, 0);

    for (i=0; i<50; i++) {      /* mindestens 20 */
        ems_u32 v;
        jtag_data(chain, &v);
        dout +=v;
    }

    if ((ret=jtag_rd_data(chip, (u_int32_t*)data, size)) != 0) {
        printf("\n");
        return ret;
    }
    jtag_rd_data(chip, 0, 0);

    return 0;
}
/****************************************************************************/
/****************************************************************************/
