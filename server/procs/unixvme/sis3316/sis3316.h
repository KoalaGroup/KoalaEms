/*
 * procs/unixvme/sis3316/sis3316.h
 * created 2016-Feb-09 PeWue
 *
 * $ZEL: sis3316.h,v 1.6 2017/10/09 21:09:24 wuestner Exp $
 */

#ifndef _sis3316_h_
#define _sis3316_h_

#include <sconf.h>
#include <debug.h>
#include <sys/types.h>
#include <errorcodes.h>
#include "../../../objects/domain/dom_ml.h"

#define maxwordsperchannel 0x1000000/* max. 64 MByte for one channel */

struct sis3316_clock {
    u_int8_t initial_clock[6];
    double rfreq5g;
    int initial_clock_valid;
};

struct sis3316_priv_data {
    plerrcode (*sis3316_read_ctrl)(ml_entry*, ems_u32, ems_u32*);
    plerrcode (*sis3316_write_ctrl)(ml_entry*, ems_u32, ems_u32);
    plerrcode (*sis3316_read_reg)(ml_entry*, ems_u32, ems_u32*);
    plerrcode (*sis3316_write_reg)(ml_entry*, ems_u32, ems_u32);
    plerrcode (*sis3316_read_regs)(ml_entry*, ems_u32*, ems_u32*, int num);
    plerrcode (*sis3316_write_regs)(ml_entry*, ems_u32*, ems_u32*, int num);
    plerrcode (*sis3316_read_reg_multi)(ml_entry**, ems_u32, ems_u32*, int);
    plerrcode (*sis3316_read_regs_multi)(ml_entry**, const ems_u32*, ems_u32**,
            int, int);
    plerrcode (*sis3316_read_mem)(ml_entry*, int, int, ems_u32, ems_u32*, int);
    plerrcode (*sis3316_write_mem)(ml_entry*, int, int, ems_u32, ems_u32*, int);
    struct sis3316_clock clocks[4];
    ems_u32 module_id;
    ems_u32 serial;
    ems_u32 protocol;
    size_t memsize;
    int max_udp_packets;
    int jumbo_enabled;
    ems_u32 last_acq_state; /* reg 0x60; filled in by the trigger procedure */
    int last_acq_state_valid; /* initialise with FALSE at readout start */
    int previous_bank; /* no initialisation needed, used in readout_udp */

    /* for readout_interleaved */
    ems_u32 data[maxwordsperchannel]; /* temorary space for one channel */
    ems_u32 address_vals[16]; /* == number of data words for each channel */
    int maxwords_per_packet;
    int maxwords_per_request;
    int active_channel;
    int words_to_read; /* initially == address_vals[active_channel] */
    int words_outstanding; /* words to read in the actual request */
    //int words_requested; /* words actually requested (<=maxwords_per_request) */
    //int words_received;  /* words received for actual request */
    int next_word;     /* index in data */
    u_int8_t sequence;
};

plerrcode sis3316_set_clock(ml_entry*, unsigned int clock, unsigned int freq);
plerrcode sis3316_get_clock(ml_entry*, unsigned int clock, ems_u32* val);
plerrcode sis3316_set_clock_any(ml_entry* module, unsigned int clock, int Fnew);
plerrcode sis3316_reset_clock(ml_entry* module, unsigned int clock);

plerrcode sis3316_set_dac(ml_entry* module, unsigned int group, ems_u32 *dac);

plerrcode sis3316_adc_spi_setup(ml_entry* module);

#endif
