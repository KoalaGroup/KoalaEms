/******************************************************************************
*                                                                             *
* handlers.h                                                                  *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 25.02.94                                                                    *
*                                                                             *
******************************************************************************/

#ifndef _handlers_h_
#define _handlers_h_
#ifdef NOVOLATILE
#define VOLATILE
#else
#define VOLATILE volatile
#endif
#define HANDLER_IDLE 0x7fffffff
#define HANDLER_MINVAL 0x80000000
#define HANDLER_MAXVAL 0x7ffffffe
#define HANDLER_START 0x7ffffff9
#define HANDLER_QUIT 0x7ffffffe
#define HANDLER_WRITE 0x7ffffffd
#define HANDLER_GETSTAT 0x7ffffffc
#define HANDLER_DISABLE 0x7ffffffb
#define HANDLER_ENABLE 0x7ffffffa
#define MAX_WIND 1000
#endif

/*****************************************************************************/
/*****************************************************************************/

