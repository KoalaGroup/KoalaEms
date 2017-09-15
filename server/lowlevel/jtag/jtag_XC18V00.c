/*
 * lowlevel/jtag/jtag_XC18V00.c
 * created 2006-Jul-05 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: jtag_XC18V00.c,v 1.7 2011/04/06 20:30:24 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
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
    priv1     = 0xF1,
    ISCTESTSTATUS  = 0xE3,
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
/*
 * returns:
 *      0: success
 *     -1: fatal error
 *     -2: security bit set
 */
int
jtag_read_XC18V00(struct jtag_chip* chip, ems_u8* data, ems_u32* usercode)
{
    struct jtag_chain* chain=chip->chain;
    u_int32_t dout, size, ret;

    printf("reading device %s\n", chip->chipdata->name);

    if (chain->state) {
        printf("chain.state=%d\n", chain->state);
        return -1;
    }

    if (jtag_instruction(chip, IDCODE, 0)) {
        return -1;
    }
    if (jtag_data32(chip, -1, &dout, 32)<0)
        return -1;
    if ((dout/*&0x0fffffff*/)!=chip->id) {
        printf("wrong IDCODE: %08X (chip->id=%08X)\n", dout, chip->id);
        return -1;
    }

    size=chip->chipdata->mem_size;

    /* read usercode (used as serial number) */
    jtag_instruction(chip, USERCODE, 0);
    jtag_data32(chip, 0, usercode, 32);
    if (!data) /* only usercode requested */
        return 0;

    jtag_instruction(chip, ISPEN, 0);
    jtag_data32(chip, 0x34, 0, 6);

    jtag_instruction(chip, FADDR, &ret);
#if 1
    if (ret!=0x11) {
        if ((ret&~0x18)!=0x1) {
            printf("illegal capture pattern 0x%x\n", ret);
            return -1;
        } else {
            if (!(ret&0x10))
                printf("chip not in ISP status\n");
            if (ret&0x8) {
                printf("security bit set\n");
                //return -2;
            }
            //return -1;
        }
    }
#endif

    jtag_data32(chip, 0, 0, 16);
    jtag_instruction(chip, FVFY1, 0);
    jtag_sleep(chain, 10);

    if ((ret=jtag_rd_data(chip, (u_int32_t*)data, size)) != 0) {
        printf("\n");
        return ret;
    }
    jtag_rd_data(chip, 0, 0);
    jtag_instruction(chip, NORMRST, 0);
    jtag_sleep(chain, 11000);
    jtag_instruction(chip, BYPASS, 0);

    return 0;
}
/****************************************************************************/
int
jtag_write_XC18V00(struct jtag_chip* chip, ems_u8* data, ems_u32 usercode)
{
    struct jtag_chain* chain=chip->chain;
    u_int32_t ret;
    int	size, len, i, tmp;

    printf("writing device %s\n", chip->chipdata->name);

    if (chain->state) {
        printf("chain.state=%d\n", chain->state);
        return -1;
    }

    size=chip->chipdata->mem_size;
    len=chip->chipdata->write_size;

    jtag_instruction(chip, ISPEN, 0);
    jtag_data32(chip, 0x34, 0, 6);

    jtag_instruction(chip, FADDR, &ret);
    if (ret!=0x11) {
        printf("FADDR.0 %02X\n", ret);
        //return -1;
    }
    jtag_data32(chip, 1, 0, 16);
    jtag_sleep(chain, 1);

    jtag_instruction(chip, FERASE, 0);
    jtag_sleep(chain, chip->chipdata->erasetest);

    jtag_instruction(chip, NORMRST, 0);
    jtag_sleep(chain, 11000);

    jtag_instruction(chip, ISPEN, 0);
    jtag_data32(chip, 0x34, 0, 6);

    tmp=0;
    for (i=0; i<size/len; i++, data+=len) {
        jtag_instruction(chip, FDATA0, 0);
        if (jtag_wr_data(chip, data, len, -1))
            return -1;
        jtag_sleep(chain, 1);

        if (!i) {
            jtag_instruction(chip, FADDR, &ret);
            if (ret!=0x11)
                printf("FADDR.1 %02X\n", ret);
            jtag_data32(chip, tmp, 0, 16);
            tmp+=0x20;
            jtag_sleep(chain, 1);
        }

        jtag_instruction(chip, FPGM, 0);
        jtag_sleep(chain, 14000);

        printf("."); fflush(stdout);
    }
    printf("\n");

    jtag_instruction(chip, FADDR, &ret);
    if (ret!=0x11)
        printf("FADDR.2 %02X\n", ret);

    jtag_data32(chip, 1, 0, 16);
    jtag_sleep(chain, 1);

    jtag_instruction(chip, SERASE, 0);
    jtag_sleep(chain, 3700);

    jtag_instruction(chip, NORMRST, 0);
    jtag_sleep(chain, 11000);

    printf("writing user code %d\n", usercode);
    switch (chip->chipdata->part_id) {
    case DID_XC18V01a:
    case DID_XC18V01b:
        printf("using codepath for XC18V01\n");
        jtag_instruction(chip, ISPEN, 0);
        jtag_data32(chip, 0x34, 0, 6);

        jtag_instruction(chip, FADDR, 0);
        jtag_data32(chip, 0x4000, 0, 16);
        jtag_sleep(chain, 1);

        jtag_instruction(chip, USERCODE, 0);
        jtag_data32(chip, usercode, 0, 32);

        jtag_instruction(chip, FPGM, 0);
        jtag_sleep(chain, 140000);

        jtag_instruction(chip, FVFY6, 0);
        jtag_sleep(chain, 500);

        {
            ems_u32 newcode;
            jtag_data32(chip, 0, &newcode, 32);
            if (newcode!=usercode) {
                printf("error writing user code\n");
            }
        }

        jtag_instruction(chip, NORMRST, 0);
        jtag_sleep(chain, 11000);

        jtag_instruction(chip, ISPEN, 0);
        jtag_data32(chip, 0x34, 0, 6);

        jtag_instruction(chip, ISPEN, 0);
        jtag_data32(chip, 0x34, 0, 6);

        jtag_instruction(chip, FADDR, 0);
        jtag_data32(chip, 0x4000, 0, 16);
        jtag_sleep(chain, 1);

        jtag_instruction(chip, FDATA3, 0);
        jtag_data32(chip, 0x1F, 0, 6);

        jtag_instruction(chip, FPGM, 0);
        jtag_sleep(chain, 1400);

        jtag_instruction(chip, FVFY3, 0);
        jtag_sleep(chain, 5);
        jtag_data32(chip, 0x1F, 0, 6);
        break;
    case DID_XC18V02a:
    case DID_XC18V04a:
    case DID_XC18V04b:
        printf("using codepath for XC18V04\n");
        jtag_instruction(chip, ISPEN, 0);
        jtag_data32(chip, 0x34, 0, 6);

        jtag_instruction(chip, FADDR, 0);
        jtag_data32(chip, 0x8000, 0, 16);
        jtag_sleep(chain, 1);

        jtag_instruction(chip, USERCODE, 0);
        jtag_data32(chip, usercode, 0, 32);

        jtag_instruction(chip, FPGM, 0);
        jtag_sleep(chain, 140000);

        jtag_instruction(chip, FVFY6, 0);
        jtag_sleep(chain, 500);

        {
            ems_u32 newcode;
            jtag_data32(chip, 0, &newcode, 32);
            if (newcode!=usercode) {
                printf("error writing user code\n");
            }
        }

        jtag_instruction(chip, NORMRST, 0);
        jtag_sleep(chain, 11000);

        jtag_instruction(chip, ISPEN, 0);
        jtag_data32(chip, 0x34, 0, 6);

        jtag_instruction(chip, ISPEN, 0);
        jtag_data32(chip, 0x34, 0, 6);

        jtag_instruction(chip, FADDR, 0);
        jtag_data32(chip, 0x8000, 0, 16);
        jtag_sleep(chain, 1);

        jtag_instruction(chip, FDATA3, 0);
        jtag_data32(chip, 0x7, 0, 6);

        jtag_instruction(chip, FPGM, 0);
        jtag_sleep(chain, 1400);

        jtag_instruction(chip, FVFY3, 0);
        jtag_sleep(chain, 5);
        jtag_data32(chip, 0x7, 0, 6);
        break;
    default:
        printf("I don't know how to write the usercode to this chip type.\n");
    }

    jtag_instruction(chip, NORMRST, 0);
    jtag_sleep(chain, 11000);

    jtag_instruction(chip, ISPEN, 0);
    jtag_data32(chip, 0x34, 0, 6);

    jtag_instruction(chip, NORMRST, 0);
    jtag_sleep(chain, 11000);

    jtag_instruction(chip, BYPASS, &ret);
    if (ret!=0x01)
        printf("BYPASS.1 %02X\n", ret);

    printf("%d bytes written\n", size);

    return 0;
}
/****************************************************************************/
/****************************************************************************/
