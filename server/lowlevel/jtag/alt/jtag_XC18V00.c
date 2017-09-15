/*
 * lowlevel/jtag/jtag_XC18V00.c
 * created 2006-Jul-05 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: jtag_XC18V00.c,v 1.2 2011/04/06 20:30:24 wuestner Exp $";

#include <sconf.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>
#include <rcs_ids.h>

#include "jtag_low.h"
#include "jtag_int.h"

enum instructions {
BYPASS    = 0xFF,
IDCODE    = 0xFE,
ISPEN     = 0xE8, /* enable in-system programming mode */
FPGM      = 0xEA, /* Programs specific bit values at specified addresses */
FADDR     = 0xEB, /* Sets the PROM array address register */
FVFY1     = 0xF8, /* Reads the fuse values at specified addresses */
NORMRST	  = 0xF0, /* Exits ISP Mode ? */
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
int
jtag_read_XC18V00(struct jtag_chip* chip, u_int8_t* data)
{
    struct jtag_chain* chain=chip->chain;
    u_int32_t dout, size, ret;
    int i;

    if (chain->state) {
        printf("chain.state=%d\n", chain->state);
        return -1;
    }

    printf("reading device %s\n", chip->chipdata->name);

    if (jtag_instruction(chip, IDCODE, 0)) {
        return -1;
    }
    if (jtag_data32(chip, -1, &dout, 32)<0)
        return -1;
    if ((dout&0x0fffffff)!=chip->id) {
        printf("wrong IDCODE: %08X\n", dout);
        return -1;
    }

    size=chip->chipdata->mem_size;

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
int
jtag_write_XC18V00(struct jtag_chip* chip, u_int8_t* data)
{
    struct jtag_chain* chain=chip->chain;
    u_int32_t dout, ret;
    int	size, len, i;

    if (chain->state) {
        printf("chain.state=%d\n", chain->state);
        return -1;
    }

    printf("writing device %s\n", chip->chipdata->name);

    size=chip->chipdata->mem_size;
    len=chip->chipdata->write_size;
    jtag_instruction(chip, ISPEN, 0);
    jtag_data32(chip, 0x04, 0, 6);

    jtag_instruction(chip, FADDR, &ret);
    if (ret!=0x11) {
        printf("FADDR.0 %02X\n", ret);
        return -1;
    }
    jtag_data32(chip, 1, 0, 16);
    jtag_instruction(chip, FERASE, 0);

    for (i=0; i<1000; i++) {   /* mindestens 500 */
        ems_u32 v;
        jtag_data(chain, &v);
        dout +=v;
    }

    jtag_instruction(chip, NORMRST, 0);

    jtag_instruction(chip, BYPASS, &ret);
    if (ret!=0x01)
        printf("BYPASS.0 %02X\n", ret);

    jtag_instruction(chip, ISPEN, 0);
    jtag_data32(chip, 0x04, 0, 6);


    for (i=0; i<size/len; i++, data+=len) {
        int j;
        jtag_instruction(chip, FDATA0, 0);
        if (jtag_wr_data(chip, data, len, -1))
            return -1;

        if (!i) {
            jtag_instruction(chip, FADDR, &ret);
            if (ret!=0x11)
                printf("FADDR.1 %02X\n", ret);
            jtag_data32(chip, 0, 0, 16);
        }

        jtag_instruction(chip, FPGM, 0);

        for (j=0; j<1000; j++) {   /* mindestens 500 */
            ems_u32 v;
            jtag_data(chain, &v);
            dout +=v;
        }
        printf("."); fflush(stdout);
    }
    printf("\n");

    jtag_instruction(chip, FADDR, &ret);
    if (ret!=0x11)
        printf("FADDR.2 %02X\n", ret);

    jtag_data32(chip, 1, 0, 16);
    jtag_instruction(chip, SERASE, 0);

    for (i=0; i<1000; i++) {   /* mindestens 500 */
        ems_u32 v;
        jtag_data(chain, &v);
        dout +=v;
    }

    jtag_instruction(chip, NORMRST, 0);

    jtag_instruction(chip, BYPASS, &ret);
    if (ret!=0x01)
        printf("BYPASS.1 %02X\n", ret);

    printf("%d bytes written\n", size);

    return 0;
}
/****************************************************************************/
/****************************************************************************/
