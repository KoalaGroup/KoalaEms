/******************************************************************************
*                                                                             *
* ccommcallback.h                                                             *
*                                                                             *
* created 30.01.96                                                            *
* last changed 31.01.96                                                       *
*                                                                             *
* P. W.                                                                       *
*                                                                             *
******************************************************************************/
#ifndef _ccommcallback_h_
#define _ccommcallback_h_

/*****************************************************************************/

/*
void callback(int action, int reason, int path, void* data);
meldet, wenn Clientcomm_init, _done oder _abort ausgefuehrt wurde
action: -1 Abort
         0 Done
         1 init
reason:  EMSerrcode bei Abort, sonst 0
path  :  betroffener Pfad
*/

__BEGIN_DECLS
typedef void (*cbackproc)(int, int, int, void*);

cbackproc install_ccommback(cbackproc, void*);
__END_DECLS

#endif

/*****************************************************************************/
/*****************************************************************************/
