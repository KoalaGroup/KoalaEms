/*
 * lowlevel/lvd/vertex/vertex.h
 * $ZEL: vertex.h,v 1.27 2013/01/17 22:44:55 wuestner Exp $
 * created 2005-Feb-25 PW
 */

#ifndef _vertex_h_
#define _vertex_h_

#include "../lvdbus.h"
plerrcode   lvd_vertex_start_init_mode(struct lvd_dev* dev, int addr);
plerrcode   lvd_vertex_module_start(struct lvd_dev* dev, int addr);
plerrcode   lvd_vertex_module_stop(struct lvd_dev* dev, int addr);

plerrcode   lvd_vertex_seq_init(struct lvd_dev* dev, int addr, int unit, ems_u32* par);
plerrcode   lvd_vertex_set_ready(struct lvd_dev* dev, int addr, int unit, ems_u32 value);
plerrcode   start_vertex_init_mode(struct lvd_dev* dev, struct lvd_acard* acard);

plerrcode   lvd_vertex_mreset(struct lvd_dev* dev, int addr, int unit);
plerrcode   lvd_vertex_ro_reset(struct lvd_dev* dev, int addr, int unit);
plerrcode   lvd_vertex_exec_seq_func(struct lvd_dev* dev, int addr, int unit, ems_u32 sw);


plerrcode   lvd_vertex_seq_halt(struct lvd_dev* dev, int addr, int unit, ems_u32 value);
plerrcode   lvd_vertex_info_ro_par_set(struct lvd_dev* dev, int addr, int unit, 
				       ems_u32 len, ems_u32* par);
int         lvd_vertex_count_chips(struct lvd_dev* dev, int addr, int idx,
                int maxchips);
plerrcode   lvd_vertex_chip_info(struct lvd_dev* dev, int addr,
                ems_u32* chipargs);
plerrcode   lvd_vertex_load_vata(struct lvd_dev* dev, int addr, int unit,
                int /*bits*/words, ems_u32 *data);
plerrcode   lvd_vertex_read_vata(struct lvd_dev* dev, int addr, int unit,
                ems_u32 *data, int *num);
plerrcode   lvd_vertex_seq_wait(struct lvd_dev* dev, int addr, int unit);

#if 0
/* plerrcode   lvd_vertex_module_init(struct lvd_dev* dev, int addr, ems_u32 mode); */
/* plerrcode   lvd_vertex_init(struct lvd_dev* dev, int addr, int daqmode, int*); */
plerrcode   lvd_vertex_rw_mem(struct lvd_dev*, int, int, int, size_t, ems_u32*,
                int);
plerrcode   lvd_vertex_load_mem_file(struct lvd_dev*, int, int, int, char*,
                int mode);
plerrcode   lvd_vertex_fill_mem(struct lvd_dev*, int, int, int, size_t, char);
plerrcode   lvd_vertex_check_mem(struct lvd_dev*, int, int);
plerrcode   lvd_vertex_mem_size(struct lvd_dev*, int, int, ems_u32*);
#endif

plerrcode   lvd_vertex_read_mem_file(struct lvd_dev*, int addr, int memtype,
                int memaddr, int num, const char* filename);
plerrcode   lvd_vertex_adc_dac(struct lvd_dev* dev, int addr, int unit,
                int data);
plerrcode   lvd_vertex_vata_dac(struct lvd_dev* dev, int addr, int unit,
                int num, ems_u32 *data);
plerrcode   lvd_vertex_vata_dac_change(struct lvd_dev* dev, int addr, int unit,
				       int num, ems_u32 value);
plerrcode   lvd_vertex_aclk_par_read(struct lvd_dev* dev, int addr, int unit, int* value);
plerrcode   lvd_vertex_aclk_par_write(struct lvd_dev* dev, int addr, int unit, int value);
plerrcode   lvd_vertex_dgap_len_read(struct lvd_dev* dev, int addr, int unit, int ind,
                int* value);
plerrcode   lvd_vertex_dgap_len_write(struct lvd_dev* dev, int addr, int unit, int ind,
                int value);
plerrcode   lvd_vertex_aclk_delay_read(struct lvd_dev* dev, int addr, int unit,
                int* value);
plerrcode   lvd_vertex_aclk_delay_write(struct lvd_dev* dev, int addr, int unit,
                int value);
plerrcode   lvd_vertex_seq_csr(struct lvd_dev* dev, int addr, int value);
plerrcode   lvd_vertex_get_seq_csr(struct lvd_dev* dev, int addr,
                ems_u32* value);
plerrcode   lvd_vertex_seq_switch(struct lvd_dev* dev, int addr, int unit,
                int idx, ems_u32 value, int enable_cr);
plerrcode   lvd_vertex_get_seq_switch(struct lvd_dev* dev, int addr, int unit,
                ems_u32* data);
plerrcode   lvd_vertex_seq_count(struct lvd_dev* dev, int addr, ems_u32* data);
plerrcode   lvd_vertex_seq_loopcount(struct lvd_dev* dev, int addr, int unit,
                int idx, int value);
plerrcode   lvd_vertex_seq_get_loopcount(struct lvd_dev* dev, int addr,
                ems_u32* counter);
plerrcode   lvd_vertex_nrchan_write(struct lvd_dev* dev, int addr, int unit,
                int value);
plerrcode   lvd_vertex_nrchan_read(struct lvd_dev* dev, int addr, int unit,
                int* value);
plerrcode   lvd_vertex_noisewin_read(struct lvd_dev* dev, int addr, int unit,
                int* value);
plerrcode   lvd_vertex_noisewin_write(struct lvd_dev* dev, int addr, int unit,
                int value);
plerrcode   lvd_vertex_cmodval(struct lvd_dev* dev, int addr, ems_u32* data);
plerrcode   lvd_vertex_nrwinval(struct lvd_dev* dev, int addr, ems_u32* data);
plerrcode   lvd_vertex_sample(struct lvd_dev* dev, int addr, ems_u32* data);

plerrcode   lvd_vertex_load_sequencer(struct lvd_dev* dev, int addr, int unit,
                int seq_addr, int num, ems_u32* data);
plerrcode   lvd_vertex_fill_sequencer(struct lvd_dev* dev, int addr, int unit,
                int seq_addr, int num, ems_u16 val);
plerrcode   lvd_vertex_read_sequencer(struct lvd_dev* dev, int addr, int unit,
                int seq_addr, int num, ems_u32* data);
plerrcode   lvd_vertex_load_ram(struct lvd_dev* dev, int addr, int unit,
                int ram_addr, int num, ems_u32* data);
plerrcode   lvd_vertex_fill_ram(struct lvd_dev* dev, int addr, int unit,
                int ram_addr, int num, ems_u16 val);
plerrcode   lvd_vertex_read_ram(struct lvd_dev* dev, int addr, int unit,
                int ram_addr, int num, ems_u32* data);
ems_u32     lvd_vertex_thr_offs(struct lvd_dev* dev, int addr, int idx);

plerrcode   lvd_vertex_write_uw(struct lvd_dev* dev, int addr, int unit, 
				ems_u32 value);
plerrcode   lvd_vertex_seq_prep_ro(struct lvd_dev* dev, int addr, int unit, 
				   ems_u32 len, ems_u32* value);
plerrcode   lvd_vertex_analog_chan_tp(struct lvd_dev* dev, int addr, int idx, ems_u32 par);
plerrcode   lvd_vertex_sel_chan_tp(struct lvd_dev* dev, int addr, int unit, 
				ems_u32* par);
int         lvd_cardstat_vertex(struct lvd_dev* dev, struct lvd_acard* acard,
                void*, int level);

int               IsVertexCard(struct lvd_dev* dev, int card);
struct lvd_acard* GetVertexCard(struct lvd_dev* dev, int addr);
struct lvd_acard* GetVertexMate3Card(struct lvd_dev* dev, int addr);
plerrcode lvd_vertex_cycles(struct lvd_dev* dev, int addr, int idx, ems_u32* p);

/*   MATE3 specific functions */

plerrcode   lvd_vertex_i2c_delay(struct lvd_dev* dev, int addr, int idx, ems_u32 del);

plerrcode   lvd_vertex_i2c_period_m3(struct lvd_dev* dev, int addr, ems_u32 p); /* VertexADCM3 */
plerrcode   lvd_vertex_i2c_period_un(struct lvd_dev* dev, int addr, ems_u32 p); /* VertexADCUN */
plerrcode   lvd_vertex_i2c_get_period_m3(struct lvd_dev* dev, int addr, ems_u32* p); /* VertexADCM3 */
plerrcode   lvd_vertex_i2c_get_period_un(struct lvd_dev* dev, int addr, ems_u32* p); /* VertexADCUN */

plerrcode   lvd_vertex_read_mate3_reg(struct lvd_dev* dev, int addr,
				      int idx, int chip, int reg, ems_u32* data);
int         lvd_vertex_read_mate3_chip_number(struct lvd_dev* dev, int addr, int idx);
plerrcode   lvd_vertex_read_mate3_board_reg(struct lvd_dev* dev, int addr,
					  int idx, ems_u32* data);
plerrcode   lvd_vertex_init_mate3(struct lvd_dev* dev, int addr, int unit,
				int words, ems_u32* data);
plerrcode   lvd_vertex_load_mate3_chip(struct lvd_dev* dev, int addr, int unit,
				int chip, int words, ems_u32* data);
plerrcode   lvd_vertex_xclk_delay(struct lvd_dev* dev, int addr, int unit, int value);

void        lvd_vertex_never_call(struct lvd_dev* dev, int addr, int unit); /* for unused static funcs */
plerrcode   lvd_vertex_mode(struct lvd_dev* dev, ems_u32 addr, ems_u32 mode);
plerrcode   lvd_vertex_mon_chan(struct lvd_dev* dev, ems_u32 addr, ems_u32 unit, ems_u32 ch, ems_u32 all);

plerrcode   lvd_vertex_calibr_ch(struct lvd_dev* dev, int chan, int all); /* for all VertexADC */


plerrcode   lvd_vertex_load_chips(struct lvd_dev* dev, int addr, int unit, int nw, ems_u32* data);

extern const int  VTX_INIT_MODE;
extern const int  VTX_STOP_MODE;

#endif
