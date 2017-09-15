/******************************************************************************
*                                                                             *
* in.h                                                                        *
*                                                                             *
* created 02.09.94                                                            *
* last changed 23.09.94                                                       *
*                                                                             *
******************************************************************************/

#ifndef _in_h_
#define _in_h_

/*****************************************************************************/

extern int key();
extern int input_scan(char*, char);
extern int input_counted_scan(char*, char, int);

extern void input_flush();
extern int include_file(char*);

extern int input_eof();
extern char* input_source();
extern int input_line();
extern char* input_info();
extern void input_skipspace();

extern void in_initiate();
extern void in_finish();

#endif

/*****************************************************************************/
/*****************************************************************************/
