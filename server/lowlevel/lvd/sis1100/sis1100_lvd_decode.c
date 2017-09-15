/*
 * lowlevel/lvd/sis1100/sis1100_lvd.c
 * created 10.Dec.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis1100_lvd_decode.c,v 1.29 2012/09/12 14:49:28 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <rcs_ids.h>
#include "modultypes.h"
#include "sis1100_lvd.h"
#include "../lvd_map.h"

RCS_REGISTER(cvsid, "lowlevel/lvd/sis1100")

struct bus_descr {
    int start;
    int size;
    void(*decoder)(struct lvd_dev*, int, int, struct bus_descr*, int);
    char* name;
};

static void
decode_union24(struct lvd_dev* dev, int addr, int size,
        struct bus_descr* descr, int idx)
{
    if (size==descr[idx].size) {
        if (addr==0) {
            printf(".l");
        } else {
            printf(" illegal offset");
            printf("\n   idx=%d size=%d descr[%d].size=%d "
                    "addr=0x%x descr[%d].start=0x%x",
                idx, size, idx, descr[idx].size,
                addr, idx, descr[idx].start);
        }
    } else {
        printf(".s[%d]", (addr)/2);
    }
}

static void
decode_arr2(struct lvd_dev* dev, int addr, int size, struct bus_descr* descr,
        int i)
{
    int idx=addr/2;
    printf("[%d]", idx);
    if (size!=2) {
        printf(" illegal size %d(%d)", size, 2);
    }
}

struct bus_descr lvd_incard_all_descr[]={
    {  0,   2, 0, "ident"},
    {  2,   2, 0, "serial"},
    {  4,   4, 0, "res04"},
    {  8,   2, 0, "ro_data"},
    { 10,   2, 0, "res10"},
    { 12,   2, 0, "cr"},
    { 14,   2, 0, "sr"},
    { 16, 110, 0, "res16"},
    {126,   2, 0, "ctrl"},
    {128,   0, 0, "out of range"},
};

struct bus_descr lvd_incard_f1_descr[]={
    {  0,  2, 0, "ident"},
    {  2,  2, 0, "serial"},
    {  4,  2, 0, "trg_lat"},
    {  6,  2, 0, "trg_win"},
    {  8,  2, 0, "ro_data"},
    { 10,  2, 0, "f1_addr"},
    { 12,  2, 0, "cr"},
    { 14,  2, 0, "sr"},
    { 16, 16, decode_arr2, "f1_state"},
    { 32, 32, decode_arr2, "f1_reg"},
    { 64,  2, 0, "jtag_csr"},
    { 66,  2, 0, "res42"},
    { 68,  4, 0, "jtag_data"},
    { 72,  4, 0, "res48"},
    { 76,  2, 0, "f1_range"},
    { 78,  2, 0, "res4E"},
    { 80,  8, decode_arr2, "hit_ovfl"},
    { 88, 38, 0, "res58"},
    {126,  2, 0, "ctrl"},
    {128,  0, 0, "out of range"},
};

struct bus_descr lvd_incard_gpx_descr[]={
    {  0,  2, 0, "ident"},
    {  2,  2, 0, "serial"},
    {  4,  2, 0, "trg_lat"},
    {  6,  2, 0, "trg_win"},
    {  8,  2, 0, "ro_data"},
    { 10,  2, 0, "res0a"},
    { 12,  2, 0, "cr"},
    { 14,  2, 0, "sr"},
    { 16,  2, 0, "gpx_int_err"},
    { 18,  2, 0, "gpx_empty"},
    { 20, 12, 0, "res12"},
    { 32,  4, 0, "gpx_data"},
    { 36,  2, 0, "gpx_seladdr"},
    { 38,  2, 0, "dac_data"},
    { 40, 24, 0, "res26"},
    { 64,  2, 0, "jtag_csr"},
    { 66,  2, 0, "res42"},
    { 68,  4, 0, "jtag_data"},
    { 72,  4, 0, "res48"},
    { 76,  4, 0, "gpx_range"},
    { 80, 46, 0, "res50"},
    {126,  2, 0, "ctrl"},
    {128,  0, 0, "out of range"},
};

struct bus_descr lvd_incard_qdc1_descr[]={
    {  0,  2, 0, "ident"},
    {  2,  2, 0, "serial"},
    {  4,  2, 0, "trg_lat"},
    {  6,  2, 0, "trg_win"},
    {  8,  2, 0, "ro_data"},
    { 10,  2, 0, "res0a"},
    { 12,  2, 0, "cr"},
    { 14,  2, 0, "sr"},
    { 16,  2, 0, "trig_level"},
    { 18,  2, 0, "anal_ctrl"},
    { 20,  2, 0, "res14"},
    { 22,  2, 0, "aclk_shift"},
    { 24,  2, 0, "inhibit"},
    { 26,  2, 0, "channel_raw"},
    { 28,  4, decode_arr2, "res1c"},
    { 32, 32, decode_arr2, "noise_level"},
    { 64,  2, 0, "jtag_csr"},
    { 66,  2, 0, "res42"},
    { 68,  4, 0, "jtag_data"},
    { 72,  4, 0, "res48"},
    { 76,  2, 0, "int_length"},
    { 78,  2, 0, "res4e"},
    { 80,  4, 0, "gpx_range"},
    { 84, 12, 0, "res54"},
    { 96, 32, decode_arr2, "mean_level"},
    {128,  0, 0, "out of range"},
};

struct bus_descr lvd_incard_qdc3_descr[]={
    { 0x0,  2, 0, "ident"},
    { 0x2,  2, 0, "serial"},
    { 0x4,  2, 0, "trig_level"},
    { 0x6,  2, 0, "aclk_shift"},
    { 0x8,  2, 0, "ro_data"},
    { 0xa,  2, 0, "res0a"},
    { 0xc,  2, 0, "cr"},
    { 0xe,  2, 0, "sr"},
    {0x10,  2, 0, "cha_inh"},
    {0x12,  2, 0, "cha_raw"},
    {0x14,  2, 0, "iw_start"},
    {0x16,  2, 0, "iw_length"},
    {0x18,  2, 0, "sw_start"},
    {0x1a,  2, 0, "sw_length"},
    {0x1c,  2, 0, "sw_ilength"},
    {0x1e,  2, 0, "anal_ctrl"},
    {0x20, 32, decode_arr2, "q_threshold"},
    {0x40, 32, decode_arr2, "base_line"},
    {0x60,  4, 0, "tdc_range"},
    {0x64,  2, 0, "traw_cycle"},
    {0x66, 10, 0, "res66"},
    {0x70,  8, decode_arr2, "ofs_dac"},
    {0x78,  4, 0, "jtag_data"},
    {0x7c,  2, 0, "jtag_csr"},
    {0x7e,  2, 0, "res7e"},
    {0x80,  0, 0, "out of range"},
};
struct bus_descr lvd_incard_qdc80_descr[]={
    { 0x0,  2, 0, "ident"},
    { 0x2,  2, 0, "serial"},
    { 0x4,  2, 0, "channel"},
    { 0x6,  2, 0, "aclk_shift"},
    { 0x8,  2, 0, "ro_data"},
    { 0xa,  2, 0, "res0a"},
    { 0xc,  2, 0, "cr"},
    { 0xe,  2, 0, "sr"},
    {0x10,  2, 0, "sw_start"},
    {0x12,  2, 0, "sw_len"},
    {0x14,  2, 0, "iw_start"},
    {0x16,  2, 0, "iw_len"},
    {0x18,  2, 0, "coinc_tab"},
    {0x1a,  4, 0, "res1a"},
    {0x1e,  2, 0, "ch_ctrl"},
    {0x20,  2, 0, "outp"},
    {0x22,  2, 0, "cf_qrise"},
    {0x24,  2, 0, "tot_level"},
    {0x26,  2, 0, "logic_level"},
    {0x28,  2, 0, "iw_q_thr"},
    {0x2a,  2, 0, "sw_q_thr"},
    {0x2c,  2, 0, "sw_ilen"},
    {0x2e,  2, 0, "cl_q_st_len"},
    {0x30,  2, 0, "cl_q_del"},
    {0x32,  2, 0, "cl_len"},
    {0x34,  2, 0, "coinc_par"},
    {0x36,  2, 0, "dttau"},
    {0x38,  2, 0, "raw_table"},
    {0x3a,  2, 0, "baseline"},
    {0x3c,  2, 0, "tcor_noise"},
    {0x3e,  2, 0, "dttau_quality"},
    {0x40,  32, 0, "res40[16]"},
    {0x60,  4, 0, "tdc_range"},
    {0x64,  4, 0, "scaler"},
    {0x68,  2, 0, "scaler_rout"},
    {0x6a,  2, 0, "coinmin_traw"},
    {0x6c,  2, 0, "trigger_time"},
    {0x6e,  2, 0, "res6e"},
    {0x70,  2, 0, "grp_coinc"},
    {0x72,  2, 0, "tp_dac"},
    {0x74,  2, 0, "bl_cor_cycle"},
    {0x76,  2, 0, "res76"},
    {0x78,  4, 0, "jtag_data"},
    {0x7c,  2, 0, "jtag_csr"},
    {0x7e,  2, 0, "ctrl_ovr"},
    {0x80,  0, 0, "out of range"},
};

struct bus_descr lvd_incard_vertex_descr[]={
    {0x00,  2, 0, "ident"},
    {0x02,  2, 0, "serial"},
    {0x04,  2, 0, "seq_csr"},
    {0x06,  2, 0, "aclk_par"},
    {0x08,  2, 0, "ro_data"},
    {0x0a,  2, 0, "res0a"},
    {0x0c,  2, 0, "cr"},
    {0x0e,  2, 0, "sr"},
    {0x10,  4, 0, "sram_adr"},
    {0x14,  4, decode_union24, "sram_data"},
    {0x18,  4, 0, "ramt_adr"},
    {0x1c,  4, decode_union24, "ramt_data"},
    {0x20,  4, 0, "lv.swreg"},
    {0x24,  4, decode_arr2, "lv.counter"},
    {0x28,  4, decode_arr2, "lv.lp_counter"},
    {0x2c,  4, decode_arr2, "lv.dgap_len"},
    {0x30,  2, 0, "lv.clk_delay"},
    {0x32,  2, 0, "lv.nr_chan"},
    {0x34,  2, 0, "lv.noise_thr"},
    {0x36,  2, 0, "lv.comval"},
    {0x38,  2, 0, "lv.comcnt"},
    {0x3a,  2, 0, "lv.poti"},
    {0x3c,  2, 0, "lv.dac"},
    {0x3e,  2, 0, "lv.reg_cr"},
    {0x40,  2, 0, "lv.reg_data"},
    {0x42,  2, 0, "lv.adc_value"},
    {0x44,  2, 0, "lv.otr_counter"},
    {0x46,  2, 0, "lv.user_data"},
    {0x48,  4, 0, "res48"},
    {0x4C,  2, 0, "scaling"},
    {0x4E,  2, 0, "dig_tst"},
    {0x50,  4, 0, "hv.swreg"},
    {0x54,  4, decode_arr2, "hv.counter"},
    {0x58,  4, decode_arr2, "hv.lp_counter"},
    {0x5c,  4, decode_arr2, "hv.dgap_len"},
    {0x60,  2, 0, "hv.clk_delay"},
    {0x62,  2, 0, "hv.nr_chan"},
    {0x64,  2, 0, "hv.noise_thr"},
    {0x66,  2, 0, "hv.comval"},
    {0x68,  2, 0, "hv.comcnt"},
    {0x6a,  2, 0, "hv.poti"},
    {0x6c,  2, 0, "hv.dac"},
    {0x6e,  2, 0, "hv.reg_cr"},
    {0x70,  2, 0, "hv.reg_data"},
    {0x72,  2, 0, "hv.adc_value"},
    {0x74,  2, 0, "hv.otr_counter"},
    {0x76,  2, 0, "hv.user_data"},
    {0x78,  4, 0, "jtag_data"},
    {0x7C,  2, 0, "jtag_csr"},
    {0x7e,  2, 0, "action"},
    {0x80,  0, 0, "out of range"},
};
struct bus_descr lvd_incard_vertexm3_descr[]={
    {0x00,  2, 0, "ident"},
    {0x02,  2, 0, "serial"},
    {0x04,  2, 0, "seq_csr"},
    {0x06,  2, 0, "res06"},
    {0x08,  2, 0, "ro_data"},
    {0x0a,  2, 0, "res0a"},
    {0x0c,  2, 0, "cr"},
    {0x0e,  2, 0, "sr"},
    {0x10,  4, 0, "sram_adr"},
    {0x14,  4, decode_union24, "sram_data"},
    {0x18,  4, 0, "ramt_adr"},
    {0x1c,  4, decode_union24, "ramt_data"},
    {0x20,  4, 0, "lv.swreg"},
    {0x24,  4, decode_arr2, "lv.counter"},
    {0x28,  4, decode_arr2, "lv.lp_counter"},
    {0x2c,  4, decode_arr2, "lv.dgap_len"},
    {0x30,  2, 0, "lv.clk_delay"},
    {0x32,  2, 0, "lv.nr_chan"},
    {0x34,  2, 0, "lv.noise_thr"},
    {0x36,  2, 0, "lv.poti/comval"},
    {0x38,  2, 0, "lv.dac/comcnt"},
    {0x3a,  2, 0, "lv.i2c_csr"},
    {0x3c,  2, 0, "lv.i2c_data"},
    {0x3e,  2, 0, "lv.adc_value"},
    {0x40,  2, 0, "lv.user_data/otr_counter"},
    {0x42,  2, 0, "lv.aclk_par"},
    {0x44,  2, 0, "lv.xclk_del"},
    {0x46,  2, 0, "res46"},
    {0x48,  4, 0, "res48"},
    {0x4C,  2, 0, "scaling"},
    {0x4E,  2, 0, "dig_tst"},
    {0x50,  4, 0, "hv.swreg"},
    {0x54,  4, decode_arr2, "hv.counter"},
    {0x58,  4, decode_arr2, "hv.lp_counter"},
    {0x5c,  4, decode_arr2, "hv.dgap_len"},
    {0x60,  2, 0, "hv.clk_delay"},
    {0x62,  2, 0, "hv.nr_chan"},
    {0x64,  2, 0, "hv.noise_thr"},
    {0x66,  2, 0, "hv.poty/comval"},
    {0x68,  2, 0, "hv.dac/comcnt"},
    {0x6a,  2, 0, "hv.i2c_csr"},
    {0x6c,  2, 0, "hv.i2c_data"},
    {0x6e,  2, 0, "hv.adc_value"},
    {0x70,  2, 0, "hv.user_data/otr_counter"},
    {0x72,  2, 0, "hv.aclk_par"},
    {0x74,  2, 0, "hv.xclk_del"},
    {0x76,  2, 0, "res76"},
    {0x78,  4, 0, "jtag_data"},
    {0x7C,  2, 0, "jtag_csr"},
    {0x7e,  2, 0, "action"},
    {0x80,  0, 0, "out of range"},
};

struct bus_descr lvd_incard_msync_descr[]={
    {0x00, 2, 0, "ident"},
    {0x02, 2, 0, "serial"},
    {0x04, 2, 0, "sr1"},
    {0x06, 2, 0, "res06"},
    {0x08, 2, 0, "ro_data"},
    {0x0a, 2, 0, "res0a"},
    {0x0c, 2, 0, "cr"},
    {0x0e, 2, 0, "sr"},
    {0x10, 2, 0, "trig_inhibit"},
    {0x12, 2, 0, "log_inhibit"},
    {0x14, 2, 0, "trig_input"},
    {0x16, 2, 0, "trig_accepted"},
    {0x18, 2, 0, "res18"},
    {0x1a, 2, 0, "fast_clr"},
    {0x1c, 4, decode_arr2, "res1c"},
    {0x20, 16, decode_arr2, "sel_pw"},
    {0x30, 2, 0, "sel_xpw"},
    {0x32, 2, 0, "pw_ctrl"},
    {0x34, 4, 0, "timer"},
    {0x38, 4, decode_arr2, "res38"},
    {0x3c, 4, 0, "tdt"},
    {0x40, 2, 0, "jtag_csr"},
    {0x42, 2, 0, "res42"},
    {0x44, 4, 0, "jtag_data"},
    {0x48, 2, 0, "res48"},
    {0x4a, 2, 0, "fcat"},
    {0x4c, 4, 0, "evc"},
    {0x50, 46, decode_arr2, "res50"},
    {0x7e,  2, 0, "ctrl"},
    {0x80,  0, 0, "out of range"},
};

struct bus_descr lvd_incard_msync2_descr[]={
    {0x00, 2, 0, "ident"},
    {0x02, 2, 0, "serial"},
    {0x04, 2, 0, "sr1"},
    {0x06, 2, 0, "res06"},
    {0x08, 2, 0, "ro_data"},
    {0x0a, 2, 0, "res0a"},
    {0x0c, 2, 0, "cr"},
    {0x0e, 2, 0, "sr"},
    {0x10, 2, 0, "trig_inhibit"},
    {0x12, 2, 0, "log_inhibit"},
    {0x14, 2, 0, "trig_input"},
    {0x16, 2, 0, "trig_accepted"},
    {0x18, 2, 0, "res18"},
    {0x1a, 2, 0, "fast_clr"},
    {0x1c, 4, decode_arr2, "res1c"},
    {0x20, 16, decode_arr2, "sel_pw"},
    {0x30, 2, 0, "sel_xpw"},
    {0x32, 2, 0, "pw_ctrl"},
    {0x34, 4, 0, "timer"},
    {0x38, 4, decode_arr2, "res38"},
    {0x3c, 4, 0, "tdt"},
    {0x40, 2, 0, "jtag_csr"},
    {0x42, 2, 0, "res42"},
    {0x44, 4, 0, "jtag_data"},
    {0x48, 2, 0, "res48"},
    {0x4a, 2, 0, "fcat"},
    {0x4c, 4, 0, "evc"},
    {0x50, 46, decode_arr2, "res50"},
    {0x7e,  2, 0, "ctrl"},
    {0x80,  0, 0, "out of range"},
};

struct bus_descr lvd_incard_osync_descr[]={
    {0x00, 2, 0, "ident"},
    {0x02, 2, 0, "serial"},
    {0x04, 2, 0, "bsy_tmo"},
    {0x06, 2, 0, "res06"},
    {0x08, 2, 0, "ro_data"},
    {0x0a, 2, 0, "res0a"},
    {0x0c, 2, 0, "cr"},
    {0x0e, 2, 0, "sr"},
    {0x10, 2, 0, "trg_in"},
    {0x12, 2, 0, "sr_addr"},
    {0x14, 2, 0, "sr_data"},
    {0x16, 42, decode_arr2, "res16"},
    {0x40, 2, 0, "jtag_csr"},
    {0x42, 2, 0, "res42"},
    {0x44, 4, 0, "jtag_data"},
    {0x48, 54, decode_arr2, "res48"},
    {0x7e,  2, 0, "ctrl"},
    {0x80,  0, 0, "out of range"},
};

struct bus_descr lvd_incard_trig_descr[]={
    {0x00,   2, 0, "ident"},
    {0x02,   2, 0, "serial"},
    {0x04,   4, 0, "res04"},
    {0x08,   2, 0, "ro_data"},
    {0x0a,   2, 0, "res0a"},
    {0x0c,   2, 0, "cr"},
    {0x0e,   2, 0, "sr"},
    {0x10, 110, 0, "res16"},
    {0x7e,   2, 0, "ctrl"},
    {0x80,   0, 0, "out of range"},
};

struct bus_descr lvd_controller_f1_descr[]={
    {  0,  2, 0, "ident"},
    {  2,  2, 0, "serial"},
    {  4,  2, 0, "res04"},
    {  6,  2, 0, "card_id"},
    {  8,  2, 0, "cr"},
    { 10,  2, 0, "sr"},
    { 12,  4, 0, "event_nr"},
    { 16,  2, 0, "ro_data"},
    { 18,  2, 0, "res12"},
    { 20,  2, 0, "ro_delay"},
    { 22,  2, 0, "res16"},
    { 24,  4, 0, "trigger_time"},
    { 28,  4, 0, "event_info"},
    { 32, 32, decode_arr2, "f1_reg"},
    { 64,  2, 0, "jtag_csr"},
    { 66,  2, 0, "res42"},
    { 68,  4, 0, "jtag_data"},
    { 72, 54, 0, "res48"},
    {126,  2, 0, "ctrl"},
    {128,  0, 0, "out of range"},
};

struct bus_descr lvd_controller_gpx_descr[]={
    {  0,  2, 0, "ident"},
    {  2,  2, 0, "serial"},
    {  4,  2, 0, "res04"},
    {  6,  2, 0, "card_id"},
    {  8,  2, 0, "cr"},
    { 10,  2, 0, "sr"},
    { 12,  4, 0, "event_nr"},
    { 16,  2, 0, "ro_data"},
    { 18,  2, 0, "res12"},
    { 20,  2, 0, "ro_delay"},
    { 22,  2, 0, "res16"},
    { 24,  4, 0, "trigger_time"},
    { 28,  4, 0, "event_info"},
    { 32,  2, 0, "gpx_reg"},
    { 34,  2, 0, "res22"}, 
    { 36,  4, 0, "gpx_data"},
    { 40,  4, 0, "gpx_range"},
    { 44, 82, 0, "res2c"},
    {126,  2, 0, "ctrl"},
    {128,  0, 0, "out of range"},
};

struct bus_descr lvd_controller_gtp_descr[]={
    { 0x0,  2, 0, "ident"},
    { 0x2,  2, 0, "serial"},
    { 0x4,  4, 0, "res04"},
    { 0x8,  2, 0, "cr"},
    { 0xa,  2, 0, "sr"},
    { 0xc,  4, 0, "event_nr"},
    {0x10,  4, 0, "ro_data"},
    {0x14,  2, 0, "ro_delay"},
    {0x16,  2, 0, "res16"},
    {0x18,  4, 0, "trigger_time"},
    {0x1c,  4, 0, "res1c"},
    {0x20,  2, 0, "gpx_reg"},
    {0x22,  2, 0, "res22"}, 
    {0x24,  4, 0, "gpx_data"},
    {0x28,  4, 0, "gpx_range"},
    {0x2c, 20, 0, "res2c"},
    {0x40, 32, decode_arr2, "f1_reg"},
    {0x60,  2, 0, "i2c0_csr"},
    {0x62,  2, 0, "i2c0_data"},
    {0x64,  2, 0, "i2c0_wr"},
    {0x66,  2, 0, "i2c0_rd"},
    {0x68,  2, 0, "i2c1_csr"},
    {0x6a,  2, 0, "i2c1_data"},
    {0x6c,  2, 0, "i2c1_wr"},
    {0x6e,  2, 0, "i2c1_rd"},
    {0x70, 14, 0, "res70"},
    {0x7e,  2, 0, "ctrl"},
    {0x80,  0, 0, "out of range"},
};

struct bus_descr lvd_incard_bc_descr[]={
    {  0,  2, 0, "card_onl"},
    {  2,  2, 0, "card_offl"},
    {  4,  4, 0, "res04"},
    {  8,  2, 0, "ro_data"},
    { 10,  2, 0, "res10"},
    { 12,  2, 0, "cr"},
    { 14,  2, 0, "error"},
    { 16, 56, 0, "res16"},
    { 72,  4, 0, "trigger"},
    { 76, 50, 0, "res76"},
    {126,  2, 0, "ctrl"},
    {128,  0, 0, "out of range"},
};

struct bus_descr lvd_controller_bc_descr[]={
    {  0,  2, 0, "card_onl"},
    {  2,  2, 0, "card_offl"},
    {  4,  2, 0, "transp"},
    {  6,  2, 0, "card_id"},
    {  8,  2, 0, "cr"},
    { 10,  2, 0, "sr"},
    { 12,  4, 0, "event_nr"},
    { 16,  2, 0, "ro_data"},
    { 18,  2, 0, "fifo_pf"},
    { 20,  2, 0, "ro_delay"},
    { 22, 10, 0, "res16"},
    { 32, 32, decode_arr2, "f1_reg"},
    { 64,  2, 0, "jtag_csr"},
    { 66, 60, 0, "res42"},
    {126,  2, 0, "ctrl"},
    {128,  0, 0, "out of range"},
};

static void
decode_card(struct lvd_dev* dev, int addr, int size,
        struct bus_descr* descr)
{
    int i;
    for (i=0; descr[i].size; i++) {
        if ((addr>=descr[i].start) &&
                (addr<descr[i+1].start)) {
            printf("%s", descr[i].name);
            if (descr[i].decoder) {
                descr[i].decoder(dev, addr-descr[i].start, size, descr, i);
            } else {
                if (size!=descr[i].size) {
                    printf(" illegal size %d(%d)", size, descr[i].size);
                }
            }
            return;
        }
    }
    printf("out_of_range in decode_incard"); 
}

static void
decode_incards(struct lvd_dev* dev, int addr, int size,
        struct bus_descr* descr, int idx)
{
    int i=addr/0x80, indx;
    ems_u32 mtype;

    indx=lvd_find_acard(dev, i);
    if (indx>=0)
        mtype=dev->acards[indx].mtype;
    else
        mtype=-1;
    if (mtype<0) {
        printf("in_card_nix[%d].", i);
        decode_card(dev, addr%0x80, size, lvd_incard_all_descr);
    } else {
        switch (mtype) {
        case ZEL_LVD_TDC_F1:
            printf("in_card_f1[%d].", i);
            decode_card(dev, addr%0x80, size, lvd_incard_f1_descr);
            break;
        case ZEL_LVD_ADC_VERTEX:
            printf("in_card_vert[%d].", i);
            decode_card(dev, addr%0x80, size, lvd_incard_vertex_descr);
            break;
        case ZEL_LVD_ADC_VERTEXM3:
            printf("in_card_vertm3[%d].", i);
            decode_card(dev, addr%0x80, size, lvd_incard_vertexm3_descr);
            break;
        case ZEL_LVD_TDC_GPX:
            printf("in_card_gpx[%d].", i);
            decode_card(dev, addr%0x80, size, lvd_incard_gpx_descr);
            break;
        case ZEL_LVD_SQDC:
        case ZEL_LVD_FQDC:
        case ZEL_LVD_VFQDC:
            if (LVD_FWverH(dev->acards[indx].id)<3) {
                printf("in_card_qdc1[%d].", i);
                decode_card(dev, addr%0x80, size, lvd_incard_qdc1_descr);
            } else if (LVD_FWverH(dev->acards[indx].id)<8) {
                printf("in_card_qdc3[%d].", i);
                decode_card(dev, addr%0x80, size, lvd_incard_qdc3_descr);
            } else {
                printf("in_card_qdc8[%d].", i);
                decode_card(dev, addr%0x80, size, lvd_incard_qdc80_descr);
            }
            break;
        case ZEL_LVD_MSYNCH:
            printf("in_card_msync[%d].", i);
            decode_card(dev, addr%0x80, size, lvd_incard_msync_descr);
            break;
        case ZEL_LVD_OSYNCH:
            printf("in_card_osync[%d].", i);
            decode_card(dev, addr%0x80, size, lvd_incard_osync_descr);
            break;
        case ZEL_LVD_TRIGGER:
            printf("in_card_trig[%d].", i);
            decode_card(dev, addr%0x80, size, lvd_incard_trig_descr);
            break;
        default:
            printf("in_card_ill(0x%x)[%d].", mtype, i);
            decode_card(dev, addr%0x80, size, lvd_incard_all_descr);
        }
    }
}

static void
decode_controller(struct lvd_dev* dev, int addr, int size,
        struct bus_descr* descr, int idx)
{
    ems_u32 mtype;
    lvd_c_mtype(dev, &mtype);

    switch (mtype) {
    case ZEL_LVD_CONTROLLER_F1:
        printf("prim_contrF1.");
        decode_card(dev, addr%0x80, size, lvd_controller_f1_descr);
        break;
    case ZEL_LVD_CONTROLLER_GPX:
        printf("prim_contrGPX.");
        decode_card(dev, addr%0x80, size, lvd_controller_gpx_descr);
        break;
    case ZEL_LVD_CONTROLLER_F1GPX:
        printf("prim_contrGPX.");
        decode_card(dev, addr%0x80, size, lvd_controller_gtp_descr);
        break;
    default:
        printf("unknown primary controller");
    }
}

static void
decode_incard_bc(struct lvd_dev* dev, int addr, int size,
        struct bus_descr* descr, int idx)
{
    printf("incard_bc.");
    decode_card(dev, addr%0x80, size, lvd_incard_bc_descr);
}

static void
decode_illegal(struct lvd_dev* dev, int addr, int size,
        struct bus_descr* descr, int idx)
{
    printf("address in illegal space: 0x%x/%d", addr, size);
}

struct bus_descr lvd_bus1100_descr[]={
    {0x0000, 0x800, decode_incards,    "ic"},
    {0x0800, 0x800, decode_illegal,    "cs"}, /* not used anymore */
    {0x1000, 0x800, decode_illegal,    "ci"}, /* not used anymore */
    {0x1800,  0x80, decode_controller, "pc"},
    {0x1880,  0x80, decode_incard_bc,  "ib"},
    {0x1900,  0x80, decode_illegal,    "csb"}, /* not used anymore */
    {0x1980,  0x80, decode_illegal,    "cib"}, /* not used anymore */
    {0x1a00,     0, 0,                 "out of range"},
};

void
lvd_decode_addr(struct lvd_dev* dev, int addr, int size)
{
    DEFINE_LVDTRACE(trace);
    int i, res=-1;

    lvd_settrace(dev, &trace, 0);
    for (i=0; lvd_bus1100_descr[i].size; i++) {
        if ((addr>=lvd_bus1100_descr[i].start) &&
                (addr<lvd_bus1100_descr[i+1].start)) {
            /*printf("%s ", lvd_bus1100_descr[i].name);*/
            lvd_bus1100_descr[i].decoder(dev, addr-lvd_bus1100_descr[i].start,
                    size, 0, size);
            res=0;
            break;
        }
    }

    if (res)
        printf("out_of_range");

    lvd_settrace(dev, 0, trace);
}

static int
check_integrity(struct bus_descr* descr, const char* name)
{
    int ofs, i, head_printed=0;
    int res=0;

    ofs=0;
    for (i=0; descr[i].size; i++) {
        if (descr[i].start!=ofs) {
            if (!head_printed) {
                printf("check_integrity(%s):\n", name);
                head_printed=1;
            }
            printf("  offs 0x%x size %d: 0x%x expected\n", descr[i].start,
                descr[i].size, ofs);
            ofs=descr[i].start;
            res=-1;
        }
        ofs+=descr[i].size;
    }
    if (descr[i].start!=ofs) {
        if (!head_printed) {
            printf("check_integrity(%s):\n", name);
            head_printed=1;
        }
        printf("  offs 0x%x size %d: 0x%x expected\n", descr[i].start,
            descr[i].size, ofs);
        res=-1;
    }
    return res;
}

struct structures {
    int esize;
    int isize;
    char* name;
};

struct structures structures[]={
    {  0x80, sizeof(struct lvd_controller_bc_1), "lvd_controller_bc"},
    {  0x80, sizeof(struct lvd_controller_1), "lvd_controller_1"},
    {  0x80, sizeof(struct lvd_controller_2), "lvd_controller_2"},
    {  0x80, sizeof(struct lvd_controller_3), "lvd_controller_3"},
    {  0x80, sizeof(struct lvd_controller_common), "lvd_controller_common"},
    {  0x80, sizeof(struct lvd_card), "lvd_card"},
    {  0x80, sizeof(union  lvd_in_card), "lvd_in_card"},
    {  0x80, sizeof(struct lvd_in_card_bc), "lvd_in_card_bc"},
    {0x1A00, sizeof(struct lvd_bus1100), "lvd_bus1100"},
    {  0x80, sizeof(struct lvd_f1_card), "lvd_f1_card"},
    {  0x80, sizeof(struct lvd_gpx_card), "lvd_gpx_card"},
    {  0x80, sizeof(struct lvd_vertex_card), "lvd_vertex_card"},
    {  0x80, sizeof(struct lvd_qdc1_card), "lvd_qdc1_card"},
    {  0x80, sizeof(struct lvd_qdc3_card), "lvd_qdc3_card"},
    {  0x80, sizeof(struct lvd_qdc6_card), "lvd_qdc6_card"},
    {  0x80, sizeof(struct lvd_qdc6s_card), "lvd_qdc6s_card"},
    {  0x80, sizeof(struct lvd_qdc80_card), "lvd_qdc80_card"},
    {  0x80, sizeof(struct lvd_msync_card), "lvd_msync_card"},
    {  0x80, sizeof(struct lvd_osync_card), "lvd_osync_card"},
    {  0x80, sizeof(struct lvd_trig_card), "lvd_trig_card"},
    {     0, 0, 0},
};

struct descriptors {
    struct bus_descr *descr;
    char* name;
};

struct descriptors descriptors[]={
    {lvd_bus1100_descr, "lvd_bus1100_descr"},
    {lvd_incard_all_descr, "lvd_incard_all_descr"},
    {lvd_incard_f1_descr, "lvd_incard_f1_descr"},
    {lvd_incard_gpx_descr, "lvd_incard_gpx_descr"},
    {lvd_incard_vertex_descr, "lvd_incard_vertex_descr"},
    {lvd_incard_vertexm3_descr, "lvd_incard_vertexm3_descr"},
    {lvd_incard_qdc1_descr, "lvd_incard_qdc1_descr"},
    {lvd_incard_qdc3_descr, "lvd_incard_qdc3_descr"},
    {lvd_incard_qdc80_descr, "lvd_incard_qdc80_descr"},
    {lvd_incard_msync_descr, "lvd_incard_msync_descr"},
    {lvd_incard_osync_descr, "lvd_incard_osync_descr"},
    {lvd_incard_trig_descr, "lvd_incard_trig_descr"},
    {lvd_controller_f1_descr, "lvd_controller_f1_descr"},
    {lvd_controller_gpx_descr, "lvd_controller_gpx_descr"},
    {lvd_controller_gtp_descr, "lvd_controller_gtp_descr"},
    {lvd_incard_bc_descr, "lvd_incard_bc_descr"},
    {lvd_controller_bc_descr, "lvd_controller_bc_descr"},
    {0, 0},
};

#define ofs(what, elem) ((off_t)&(((what *)0)->elem))

int
lvd_decode_check_integrity(void)
{
    int res=0, i;

    for (i=0; descriptors[i].descr!=0; i++) {
        res|=check_integrity(descriptors[i].descr, descriptors[i].name);
    }

    for (i=0; structures[i].esize!=0; i++) {
        if (structures[i].esize!=structures[i].isize) {
            printf("struct %s has wrong size %d!\n", structures[i].name,
                    structures[i].isize);
            res=-1;
        }
    }

    return res;
}
