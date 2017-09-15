/******************************************************************************
*                                                                             *
* bustrap.h                                                                   *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* created: 11.10.94                                                           *
* last changed: 21.10.94                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#ifndef _bustrap_h_
#define _bustrap_h_

typedef void (*trapfunc)();
typedef void (*trappc)();
char* check_access(char* addr, int size, int needpermit);
void add_bustrapfunc(void (*func)(short*));
void change_bustrap(void *trapfunc);

#endif

/*****************************************************************************/
/*****************************************************************************/
