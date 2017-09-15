/******************************************************************************
*                                                                             *
* lowlevel/lowbuf/lowbuf.h                                                    *
*                                                                             *
* created      16.05.97                                                       *
* last changed 20.05.97                                                       *
*                                                                             *
******************************************************************************/

#ifndef _lowbuf_h_
#define _lowbuf_h_

#include <sconf.h>
#include <stdio.h>
#include <errorcodes.h>

char* lowbuffername(void);
int* lowbuf_buffer(void);
int* lowbuf_outbuffer(void);
int* lowbuf_dataoutbuffer(void);
int lowbuf_buffersize(void);
int lowbuf_outbuffersize(void);
int lowbuf_dataoutbuffersize(void);
#ifdef LOWBUFFER_EXTRA
int* lowbuf_extrabuffer(void);
int lowbuf_extrabuffersize(void);
#endif
errcode lowbuffer_init(char*);
errcode lowbuffer_done(void);
errcode lowbuf_low_init(char*);
errcode lowbuf_low_done(void);
int lowbuf_low_printuse(FILE* outfilepath);

#endif
/*****************************************************************************/
/*****************************************************************************/
