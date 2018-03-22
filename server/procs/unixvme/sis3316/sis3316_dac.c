/*
 * procs/unixvme/sis3316/sis3316_dac.c
 * created 2016-Feb-17 PeWue
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3316_dac.c,v 1.1 2016/02/25 22:50:47 wuestner Exp $";

/*
 * This code sets the DAC offsets of the sis3316 module.
 */

#include "sis3316.h"
#include <errno.h>
#include <time.h>
#include <string.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../../lowlevel/unixvme/vme.h"

RCS_REGISTER(cvsid, "procs/unixvme/sis3316")

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
#if 0
    printf("dac_offset: %04x <-- %08x\n", reg, value);
#endif
    return priv->sis3316_write_reg(module, reg, value);
}
/*****************************************************************************/
/*****************************************************************************/
static plerrcode
poll_on_adc_dac_offset_busy(ml_entry* module)
{
    ems_u32 reg, val;
    plerrcode pres;
    int poll_counter;

    reg=0xa4;
    poll_counter=1000;
    do {
        poll_counter--;
        pres=sis3316_read_reg(module, reg, &val);
    } while (pres==plOK && (val&0xf) && poll_counter>0);
#if 0
    printf("poll_counter=%d\n", poll_counter);
#endif
    if (pres==plOK && (val&0xf))
        pres=plErr_Timeout;
        
    return pres;
}
/*****************************************************************************/
static plerrcode
write_adc_dac_offset(ml_entry* module, unsigned int group, ems_u32 *dacs)
{
    ems_u32 reg, val;
    plerrcode pres;
    int chan;

    reg=group*0x1000+0x1008;

    for (chan=0; chan<4; chan++) {
        /* clear error Latch bits (???) */
        val=0x80000000+0x2000000+chan*0x100000+((dacs[chan] & 0xffff)<<4);
        if ((pres=sis3316_write_reg(module, reg, val))!=plOK)
            return plOK;
        if ((pres=poll_on_adc_dac_offset_busy(module))!=plOK)
            return plOK;
    }

    /* MD: DAC LDAC (load) */
    val=0xC0000000;
    if ((pres=sis3316_write_reg(module, reg, val))!=plOK)
        return plOK;
    pres=poll_on_adc_dac_offset_busy(module);
    return pres;
}
/*****************************************************************************/
static plerrcode
configure_adc_dac_offset(ml_entry* module, unsigned int group)
{
    ems_u32 reg, val;
    plerrcode pres;

    reg=group*0x1000+0x1008;

    /* Standalone mode, set Internal Reference */
    val=0x80000000+0x8000000+0xf00000+0x1;
    if ((pres=sis3316_write_reg(module, reg, val))!=plOK)
        return plOK;
    if ((pres=poll_on_adc_dac_offset_busy(module))!=plOK)
        return plOK;

    /* CMD: DAC LDAC (load) */
    if ((pres=sis3316_write_reg(module, reg, 0xC0000000))!=plOK)
        return plOK;
    pres=poll_on_adc_dac_offset_busy(module);
    return pres;
}
/*****************************************************************************/
/*****************************************************************************/
plerrcode
sis3316_set_dac(ml_entry* module, unsigned int group, ems_u32 *dacs)
{
    plerrcode pres;

    if ((pres=configure_adc_dac_offset(module, group))!=plOK)
        return pres;

    if ((pres=write_adc_dac_offset(module, group, dacs))!=plOK)
        return pres;

    return plOK;
}
/*****************************************************************************/
/*****************************************************************************/
