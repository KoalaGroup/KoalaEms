/*
 * dataout/cluster/dataout.h
 * $ZEL: dataout.h,v 1.8 2010/04/19 14:26:10 wuestner Exp $
 */

#ifndef _dataout_h_
#define _dataout_h_

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <emsctypes.h>

extern int buffer_free;
extern ems_u32 *next_databuf;

errcode dataout_init(char *arg);
errcode dataout_done(void);
errcode start_dataout(void);
errcode stop_dataouts(void);

plerrcode current_filename(unsigned int idx, const char **name);

void flush_databuf(ems_u32* bufend);
void schau_nach_speicher(void);

#ifdef READOUT_CC
errcode get_last_databuf(void);
#endif

#ifdef DATAOUT_MULTI
errcode insert_dataout(unsigned int index);
errcode remove_dataout(unsigned int index);
#ifdef OSK
int *get_dataout_addr(unsigned int index);
#endif
#endif

#ifdef DATAOUT_SIMPLE
#include "../objects/domain/dom_dataout.h"
extern dataout_info dataout[];
#ifdef OSK
int *get_dataout_addr(unsigned int index);
#endif
#endif

errcode windtape(unsigned int idx, int offset);
errcode writeoutput(unsigned int idx, int header, ems_u32* data);
errcode getoutputstatus(unsigned int do_idx, int old_format);
errcode enableoutput(unsigned int do_idx);
errcode disableoutput(unsigned int do_idx);
void extra_do_data(void);
void add_used_trigger(ems_u32);


errcode init_dataout_handler(void);
errcode done_dataout_handler(void);

int printuse_output(FILE* outfilepath);

#endif
