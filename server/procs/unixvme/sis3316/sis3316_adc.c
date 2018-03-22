/*
 * procs/unixvme/sis3316/sis3316_adc.c
 * created 2016-Feb-17 PeWue
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3316_adc.c,v 1.2 2016/05/12 21:52:57 wuestner Exp $";

/*
 * This code sets the DAC offsets of the sis3316 module.
 */

#include "sis3316.h"
#include <errno.h>
#include <time.h>
#include <string.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../../lowlevel/oscompat/oscompat.h"
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
    printf("adc_reg: %04x <-- %08x\n", reg, value);
#endif
    return priv->sis3316_write_reg(module, reg, value);
}
/*****************************************************************************/
/*****************************************************************************/
static plerrcode
adc_spi_write(ml_entry* module, int group, int adc,
        ems_u32 spi_addr, ems_u32 spi_data)
{
    ems_u32 addr, uint_adc_mux_select, data;
    plerrcode pres;
    int pollcounter;

    if (adc==0) {
        uint_adc_mux_select = 0 ;	// adc chip ch1/ch2	
    } else {
        uint_adc_mux_select = 0x400000 ; // adc chip ch3/ch4 		
    }

    // read register to get the information of bit 24 (adc output enabled)
    addr=0x100c+group*0x1000;
    if ((pres=sis3316_read_reg(module, addr, &data))!=plOK)
        return pres;
    data = data & 0x01000000 ; // save bit 24

    data =  data 
        + 0x80000000 
        + uint_adc_mux_select 
        + ((spi_addr & 0xffff) << 8) 
        + (spi_data & 0xff) ;
    addr = 0x100c+group*0x1000;
    if ((pres=sis3316_write_reg(module, addr, data))!=plOK)
        return pres;

    addr = 0xa4 ;
    pollcounter = 1000;
    do { // the logic is appr. 7us busy
        if ((pres=sis3316_read_reg(module, addr, &data))!=plOK)
            return pres;
        pollcounter--;
    } while ((data&0x80000000)==0x80000000 && pollcounter>0);
    //printf("poll_on_spi_busy: pollcounter = 0x%08x   data = 0x%08x \n", pollcounter, data);

    if ((data&0x80000000)==0x80000000)
        return plErr_Timeout;

    return plOK;
}
/*****************************************************************************/
static plerrcode
adc_spi_read(ml_entry* module, int group, int adc,
        ems_u32 spi_addr, ems_u32 *spi_data)
{
    ems_u32 addr, uint_adc_mux_select, data;
    plerrcode pres;
    int pollcounter;


    if (adc == 0) {
        uint_adc_mux_select = 0 ;	// adc chip ch1/ch2	
    } else {
        uint_adc_mux_select = 0x400000 ; // adc chip ch3/ch4 		
    }

    // read register to get the information of bit 24 (adc output enabled)
    addr=0x100c+group*0x1000;
    if ((pres=sis3316_read_reg(module, addr, &data))!=plOK)
        return pres;
    data = data & 0x01000000 ; // save bit 24


    data=data 
        + 0xC0000000 
        + uint_adc_mux_select 
        + ((spi_addr & 0xffff) << 8)  ;
    addr = 0x100c+group*0x1000;
    if ((pres=sis3316_write_reg(module, addr, data))!=plOK)
        return pres;  

    addr=0xa4; /* SIS3316_ADC_FPGA_SPI_BUSY_STATUS_REG */
    pollcounter=1000;
    do { // the logic is appr. 7us busy  (20us)
        if ((pres=sis3316_read_reg(module, addr, &data))!=plOK)
            return pres;  
        pollcounter--;
    } while (((data&0x80000000)==0x80000000) && (pollcounter>0));
    if ((data&0x80000000)==0x80000000)
        return plErr_Timeout;

    microsleep(1000);

    addr=0x110C; /* SIS3316_ADC_CH1_4_SPI_READBACK_REG */
    if ((pres=sis3316_read_reg(module, addr, &data))!=plOK)
        return pres;  

    *spi_data = data & 0xff ;

    return plOK;
}
/*****************************************************************************/
/*****************************************************************************/
plerrcode
sis3316_adc_spi_setup(ml_entry* module)
{
    int group, adc, adc_125MHz_flag;
    ems_u32 addr, adc_chip_id, data;
    plerrcode pres;

    /* disable ADC output */
    for (group=0; group<4; group++) {
        addr=0x100c+group*0x1000;
        if ((pres=sis3316_write_reg(module, addr, 0))!=plOK)
            return pres;
    }
 
    /* reset */
    for (group=0; group<4; group++) {
        for (adc=0; adc<2; adc++) {
            /* soft reset */
            if ((pres=adc_spi_write(module, group, adc, 0x0, 0x24))!=plOK)
                return pres;
        }
        microsleep(10) ; /* after reset */
    }

    /* read chip Id from adc chips ch1/2 */
    if ((adc_spi_read(module, 0, 0, 1, &adc_chip_id))!=plOK)
        return pres;

    for (group=0; group<4; group++) {
        for (adc=0; adc<2; adc++) {
            if ((pres=adc_spi_read(module, group, adc, 1, &data))!=plOK)
                return pres;
            if (data!=adc_chip_id) {
                printf("group %d adc %d data=0x%08x adc_chip_id=0x%08x \n",
                        group, adc, data, adc_chip_id);
                return -1 ;
            }
        }
    }

    adc_125MHz_flag=0;
    if ((adc_chip_id&0xff)==0x32) {
        adc_125MHz_flag=1;
    }
    printf("adc_125MHz_flag=%d\n", adc_125MHz_flag);

    /* reg 0x14 : Output mode */
    if (!adc_125MHz_flag) { // 250 MHz chip AD9643
        data = 0x04 ; 	//  Output inverted (bit2 = 1)
    } else { // 125 MHz chip AD9268
        data = 0x40 ; 	// Output type LVDS (bit6 = 1), Output inverted (bit2 = 0) !
    }
    for (group=0; group<4; group++) {
        for (adc=0; adc<2; adc++) {
           adc_spi_write(module, group, adc, 0x14, data);
        }
    }


    /* reg 0x18 : Reference Voltage / Input Span */
    if (!adc_125MHz_flag) { // 250 MHz chip AD9643
        data = 0x0 ; 	//  1.75V
    } else { // 125 MHz chip AD9268
        //data = 0x8 ; 	//  1.75V
        data = 0xC0 ; 	//  2.0V
    }
    for (group = 0; group < 4; group++) {
        for (adc = 0; adc < 2; adc++) {
            adc_spi_write(module, group, adc, 0x18, data);
        }
    }




    /* reg 0xff : register update */
    data = 0x01 ; 	// update
    for (group = 0; group < 4; group++) {
        for (adc = 0; adc < 2; adc++) {
            adc_spi_write(module, group, adc, 0xff, data);
        }
    }

    /* enable ADC output */
    for (group = 0; group < 4; group++) {
        addr = 0x100c +  ((group & 0x3) * 0x1000) ;
        sis3316_write_reg(module, addr, 0x1000000); //  set bit 24
    }

    return plOK;
}
/*****************************************************************************/
/*****************************************************************************/
