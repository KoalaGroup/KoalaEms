#ifndef _ringbuffers_h_
#define _ringbuffers_h_

#include "../../objects/domain/dom_dataout.h"

DATAOUT *init_dataoutraw(int* addr, int size);
DATAOUT *init_dataoutmod(char* name, int size);
void done_dataoutraw(DATAOUT* dto);
void done_dataoutmod(DATAOUT* dto);
void _init_dataout(DATAOUT* dto);
void _done_dataout(DATAOUT* dto);
void _schau_nach_speicher(DATAOUT* dto);
void _flush_databuf(DATAOUT* dto, volatile int* intp);
errcode _get_last_databuf(DATAOUT* dto);

#endif
