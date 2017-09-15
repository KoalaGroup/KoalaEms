/*
 * procs/camac/fera/fera.h
 * 02.Aug.2001 PW: multicrate support
 */
/* $ZEL: fera.h,v 1.7 2006/02/23 22:57:24 wuestner Exp $ */

#ifndef _fera_h_
#define _fera_h_

#include <emsctypes.h>
#include "../../../objects/domain/dom_ml.h"

#define FERA_ped_stat 0x0400
#define SILENA_ped_stat 0x2e00
#define BPU_ped_stat 0

#define FERA_ADC_readout_stat 0x0700
#define FERA_TDC_readout_stat 0x8700
#define SILENA_readout_stat 0x3c00
#define CNRS_QDC_readout_stat 0x0107
#define CNRS_TDC_readout_stat 0x8700

#define SILENA_def_comthr 10
#define SILENA_def_lthr 50
#define SILENA_def_uthr 255
#define SILENA_def_offs 128

#define BPU_def_chan 255

#define messped_min_events 100

ems_u32 *SetupFERAmodul __P((ml_entry*, ems_u32*, int*));
plerrcode test_SetupFERA __P((ml_entry*, ems_u32**));
int SetupFERAmesspeds __P((ml_entry*, int, int*, int*));

#endif
