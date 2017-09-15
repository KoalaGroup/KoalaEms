/******************************************************************************
*                                                                             *
* out.h                                                                     *
*                                                                             *
* created 02.09.94                                                            *
* last changed 08.09.94                                                       *
*                                                                             *
******************************************************************************/

#ifndef _out_h_
#define _out_h_

/*****************************************************************************/

extern char s_err[1024];
extern char s_out[1024];

extern void emit_err();
extern void emit(char);
extern void emit_();
extern void emit_s(char*);

extern void out_initiate(char*);
extern void out_finish();

extern char* get_outstring();

#endif

/*****************************************************************************/
/*****************************************************************************/
