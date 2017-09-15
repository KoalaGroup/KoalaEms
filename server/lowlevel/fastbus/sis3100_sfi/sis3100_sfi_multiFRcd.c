/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_multiFRcd.c
 * 
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_multiFRcd.c,v 1.6 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

static volatile ems_u32 mist=0x0815;

/*
both versions of sis3100_sfi_multiFRcd would work in any case
but the performance ...
*/

static int
sis3100_sfi_multiFRcd(struct fastbus_dev* dev, unsigned int num, ems_u32* pa,
        ems_u32 sa, ems_u32* data, ems_u32* ss, int mod)
{
    int res=0, i;
#ifdef SIS3100_SFI_MMAPPED

    SIS3100_CHECK_BALANCE(dev, "multiFRcd_0");

/* SEQ_W can't return an error if SIS3100_SFI_MMAPPED is defined */
    for (i=0; i<num; i++) {
        SEQ_W(dev, seq_prim_dsr|mod, pa[i]);
        SEQ_W(dev, seq_secad_w, sa);
        SEQ_W(dev, seq_rndm_r|SFI_FUNC_RNDM_DIS, mist);
    }

    res=sis3100_sfi_read_fifo_num(dev, data, num, ss);
    if (res<0) {
        printf("sis3100_sfi_multiFRcd(b) pa[0]=%d: res=%d\n", pa[0], res);
        return res;
    }
    SIS3100_CHECK_BALANCE(dev, "multiFRcd_1");

#else /* SIS3100_SFI_MMAPPED */

    static ems_u32 *list=0;
    static int listsize=0;
    ems_u32 *lp;
    if (listsize<num) {
        free(list);
        list=malloc(num*sizeof(u_int32_t)*12); /* twice as many as we need */
        if (!list) {
            printf("sis3100_sfi_multiFRcd: malloc(%lld): %s\n",
                (unsigned long long)(num*sizeof(u_int32_t)*12),
                strerror(errno));
            listsize=0;
            return -1;
        }
        listsize=2*num;
    }

    lp=list;    
    for (i=0; i<num; i++) {
        /* SEQ_W(dev, seq_prim_dsr|mod, pa[i]) */
        *lp++=SFI_BASE+ofs(struct sfi_w, seq[seq_prim_dsr|mod]);
        *lp++=pa[i];

        /* SEQ_W(dev, seq_secad_w, sa) */
        *lp++=SFI_BASE+ofs(struct sfi_w, seq[seq_secad_w]);
        *lp++=sa;

        /* SEQ_W(dev, seq_rndm_r|SFI_FUNC_RNDM_DIS, mist) */
        *lp++=SFI_BASE+ofs(struct sfi_w, seq[seq_rndm_r|SFI_FUNC_RNDM_DIS]);
        *lp++=mist;
    }

    res=sis3100_sfi_multi_write(dev, 3*num, list);

    if (res) {
        printf("sis3100_sfi_multiFRcd(a) pa[0]=%d: res=%d\n", pa[0], res);
        return res;
    }

    res=sis3100_sfi_read_fifo_num(dev, data, num, ss);

    if (res<0) {
        printf("sis3100_sfi_multiFRcd(b) pa[0]=%d: res=%d\n", pa[0], res);
        return res;
    }
#endif
    return 0;
}

int
sis3100_sfi_multiFRC(struct fastbus_dev* dev, unsigned int num, ems_u32* pa,
        ems_u32 sa, ems_u32* data, ems_u32* ss)
{
    return sis3100_sfi_multiFRcd(dev, num, pa, sa, data, ss, SFI_MS0);
}
int
sis3100_sfi_multiFRD(struct fastbus_dev* dev, unsigned int num, ems_u32* pa,
        ems_u32 sa, ems_u32* data, ems_u32* ss)
{
    return sis3100_sfi_multiFRcd(dev, num, pa, sa, data, ss, 0);
}
/*****************************************************************************/
/*****************************************************************************/
